#include "PCH/HFGame_PCH.h"
#include "HF_Player.h"

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Math/YK_MathUtils.h"
#include "YK/Math/YK_MatrixMath.h"
#include "YK/Math/YK_VectorMath.h"
#include "YK/Time/YK_Time.h"
#include "YK/Types/Math/YK_Quaternion.h"
#include "YK/Types/Math/YK_Vector.h"

#include "CG/CG_RenderModule.h"
#include "CG/Camera/CG_CameraComponent.h"
#include "CG/ECS/CG_Components.h"
#include "CG/Resource/Animation/CG_Animation.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Skeleton/CG_Skeleton.h"

#include "AM/ECS/AM_AnimationComponent.h"
#include "AM/PoseSampler/AM_AnimationPoseSampler.h"
#include "AM/PoseSampler/AM_LerpPoseSampler.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/YakuEngine.h"

#include <utility>

namespace HF_Player_Private
{
    AM_AnimationPoseSampler IdlePoseSampler;
    AM_AnimationPoseSampler WalkPoseSampler;

    // This should be ideally loaded from some data path
    // I don't have a mechanism to do that in YakuEn yet
    constexpr float CameraHeight = 2.0f;
    constexpr float CameraDistance = 7.0f;

    constexpr float CameraPanSpeedMin = 1.5f;
    constexpr float CameraPanSpeedMax = 2.0f;
    constexpr float CameraPanAccelerationDuration = 1.0f / 2.0f;
    constexpr float CameraPanAccelerationStartTime = 0.25f;

    YK_Vector3f UpdateCamera(CG_CameraComponent& p_camera)
    {
        HIDra::Vec2f rightStick = HIDra::GetAxis2D(HIDra::AID_STICK_R);
        // TODO: Integrate "Time since input started" directly into HIDra
        static float timeSinceRightStickInput = 0.0f;
        float const deltaTime = YK_Time::DeltaTime();
        // Float comparisons :')
        // Technically strongly defined for 0.0f by IEEE standard
        if (rightStick.m_x == 0.0f)
        {
            timeSinceRightStickInput = 0.0f;
        }
        else
        {
            timeSinceRightStickInput += deltaTime;
        }
        float const cameraPanLerp =
          (timeSinceRightStickInput - CameraPanAccelerationStartTime) * CameraPanAccelerationDuration;
        float const cameraPanSpeed = YK_LerpClamped(CameraPanSpeedMin, CameraPanSpeedMax, cameraPanLerp);

        p_camera.m_transform.m_orientation =
          YK_Quaternion(YK_Vector3f::Up(), -rightStick.m_x * deltaTime * cameraPanSpeed)
          * p_camera.m_transform.m_orientation;
        p_camera.m_transform.m_orientation *= YK_Quaternion(YK_Vector3f::Right(), rightStick.m_y * deltaTime);

        YK_Vector3f cameraForward = p_camera.m_transform.Forward2D();
        return YK_Vector::GetNormalized(cameraForward);
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

    HF_Player_Private::IdlePoseSampler =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Idle.YKA")));

    HF_Player_Private::WalkPoseSampler =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Walk_F.YKA")));

    animationComponent.m_poseSampler =
      AM_LerpPoseSampler(&HF_Player_Private::IdlePoseSampler, &HF_Player_Private::WalkPoseSampler);

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
    CG_CameraComponent* const camera = m_playerEntity.GetComponent<CG_CameraComponent>();
    YK_TransformComponent* const playerTransform = m_playerEntity.GetComponent<YK_TransformComponent>();
    AM_AnimationComponent* const animationComponent = m_playerEntity.GetComponent<AM_AnimationComponent>();

    YK_Vector3f const cameraForward2D = HF_Player_Private::UpdateCamera(*camera);
    YK_Vector3f const cameraRight = YK_Vector::Cross(YK_Vector3f::Up(), cameraForward2D);

    HIDra::Vec2f leftStick = HIDra::GetAxis2D(HIDra::AID_STICK_L);

    YK_Vector3f const previousPosition = playerTransform->m_position;
    YK_Vector3f const previousHammerPosition = previousPosition + (playerTransform->Forward2D() * -2.5f);

    playerTransform->m_position +=
      (cameraForward2D * -leftStick.m_y + cameraRight * leftStick.m_x) * YK_Time::DeltaTime() * 5.0f;
    playerTransform->m_orientation = YK_Matrix::LookAt(playerTransform->m_position, previousHammerPosition);

    animationComponent->m_poseSampler.SetSampleFactor(YK_Vector::Magnitude(YK_Vector2f(leftStick.m_x, leftStick.m_y)));

    // Update camera position last to prevent jitter
    YK_Vector3f cameraForward = camera->m_transform.Forward();
    camera->m_transform.m_position = playerTransform->m_position;
    camera->m_transform.m_position += cameraForward * HF_Player_Private::CameraDistance;
}