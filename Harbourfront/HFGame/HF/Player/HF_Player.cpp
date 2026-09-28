#include "PCH/HFGame_PCH.h"
#include "HF_Player.h"

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Math/YK_VectorMath.h"
#include "YK/Time/YK_Time.h"
#include "YK/Types/Math/YK_Quaternion.h"
#include "YK/Types/Math/YK_Vector.h"

#include "CG/CG_RenderModule.h"
#include "CG/Camera/CG_CameraComponent.h"
#include "CG/ECS/CG_Components.h"
#include "CG/Renderer/CG_RenderBinding.h"
#include "CG/Resource/Animation/CG_Animation.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Skeleton/CG_Skeleton.h"

#include "AM/ECS/AM_AnimationComponent.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/YakuEngine.h"

#include <utility>

namespace HF_Player_Private
{
    CG_Animation const* IdleAnimation;
    CG_Animation const* HammerJumpAnimation;

    // Not good
    void PollInput(float& p_outRaise,
                   HIDra::Vec2f& p_outDPad,
                   HIDra::Vec2f& p_outLeftStick,
                   HIDra::Vec2f& p_outRightStick)
    {
        p_outRaise = HIDra::GetButton(HIDra::BID_BUMPER_L) ? -1.0f :
                     HIDra::GetButton(HIDra::BID_BUMPER_R) ? 1.0f :
                                                             0.0f;

        p_outDPad = { 0.0f, 0.0f };
        if (HIDra::GetKey(HIDra::KEYCODE_S) || HIDra::GetButton(HIDra::BID_DPAD_SOUTH))
        {
            p_outDPad.m_y = -1;
        }
        else if (HIDra::GetKey(HIDra::KEYCODE_W) || HIDra::GetButton(HIDra::BID_DPAD_NORTH))
        {
            p_outDPad.m_y = 1;
        }

        if (HIDra::GetKey(HIDra::KEYCODE_A) || HIDra::GetButton(HIDra::BID_DPAD_WEST))
        {
            p_outDPad.m_x = -1;
        }
        else if (HIDra::GetKey(HIDra::KEYCODE_D) || HIDra::GetButton(HIDra::BID_DPAD_EAST))
        {
            p_outDPad.m_x = 1;
        }

        p_outLeftStick = HIDra::GetAxis2D(HIDra::AID_STICK_L);
        p_outRightStick = HIDra::GetAxis2D(HIDra::AID_STICK_R);
    }

} // namespace HF_Player_Private

HF_Player::HF_Player()
{
    YakuEngine& engine = YakuEngine::GetEngine();
    Zen::Garden& entityGarden = engine.GetZenGarden();
    YK_AssetManager& assetManager = engine.GetAssetManager();

    // Spawn player
    YK_TransformComponent playerTransform;
    playerTransform.m_position = YK_Vector3f{ 0.0f, 0.0f, -5.0f };

    YK_FilePath playerMeshPath = YK_FilePath("Models/Debugger.glb");
    CG_SkeletalMeshComponent skeletalMesh;
    skeletalMesh.m_mesh = &assetManager.GetAsset<CG_Mesh>(playerMeshPath);
    skeletalMesh.m_skeleton = &assetManager.GetAsset<CG_Skeleton>(playerMeshPath);

    CG_RendererComponent rendererComponent;
    rendererComponent.m_material = &assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/MainSkeletal.YKM"));

    CG_PoseComponent poseComponent;
    poseComponent.m_pose.resize(skeletalMesh.m_skeleton->m_bones.size());

    AM_AnimationComponent animationComponent;
    HF_Player_Private::IdleAnimation = &assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Idle.YKA"));
    HF_Player_Private::HammerJumpAnimation =
      &assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Hammer_Jump.YKA"));
    animationComponent.m_animation = HF_Player_Private::IdleAnimation;

    CG_CameraComponent playerCamera;
    playerCamera.m_transform.m_position = YK_Vector3f::Up();

    m_playerEntity = entityGarden.Spawn(std::move(playerTransform),
                                        std::move(skeletalMesh),
                                        std::move(rendererComponent),
                                        std::move(poseComponent),
                                        std::move(animationComponent),
                                        std::move(playerCamera));

    engine.GetModules().GetRenderModule().SetActiveCamera(m_playerEntity);
}

HF_Player::~HF_Player()
{
    YakuEngine& engine = YakuEngine::GetEngine();
    Zen::Garden& entityGarden = engine.GetZenGarden();

    entityGarden.Destroy(m_playerEntity);
}

void HF_Player::Update()
{
    float const deltaTime = YK_Time::DeltaTime();
    CG_CameraComponent* const camera = m_playerEntity.GetComponent<CG_CameraComponent>();
    YK_TransformComponent* const playerTransform = m_playerEntity.GetComponent<YK_TransformComponent>();
    AM_AnimationComponent* const animationComponent = m_playerEntity.GetComponent<AM_AnimationComponent>();

    float raise = 0.0f;
    HIDra::Vec2f dpad;
    HIDra::Vec2f leftStick;
    HIDra::Vec2f rightStick;
    HF_Player_Private::PollInput(raise, dpad, leftStick, rightStick);

    YK_Vector3f const cameraForward2D = camera->m_transform.Forward2D();
    YK_Vector3f const cameraRight = YK_Vector::Cross(YK_Vector3f::Up(), cameraForward2D);

    YK_Vector3f cameraMovementDelta =
      (cameraRight * dpad.m_x) + (cameraForward2D * -dpad.m_y) + (YK_Vector3f(0.0f, raise, 0.0f));
    cameraMovementDelta *= deltaTime * 2.0f;
    camera->m_transform.m_position += cameraMovementDelta;
    camera->m_transform.m_orientation = camera->m_transform.m_orientation * YK_Quaternion(YK_Vector3f::Up(), -rightStick.m_x * deltaTime);

    playerTransform->m_position +=
      (cameraForward2D * -leftStick.m_y + cameraRight * leftStick.m_x) * YK_Time::DeltaTime() * 5.0f;

    // Very bad animation swap test
    static float hammerTime = 0.0f;
    if (animationComponent->m_animation == HF_Player_Private::HammerJumpAnimation)
    {
        hammerTime += YK_Time::DeltaTime();
        if (hammerTime > animationComponent->m_animation->m_duration)
        {
            hammerTime = 0.0f;
            animationComponent->m_animation = HF_Player_Private::IdleAnimation;
        }
    }
    else if (HIDra::GetButtonDown(HIDra::BID_SOUTH))
    {
        animationComponent->m_animation = HF_Player_Private::HammerJumpAnimation;
        animationComponent->m_sampleTime = 0.0f;
    }
}