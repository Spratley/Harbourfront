#include "PCH/HFGame_PCH.h"
#include "HF_Player.h"

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Math/YK_AngleMath.h"
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
#include "AM/PoseSampler/AM_BlendPoseSampler.h"
#include "AM/PoseSampler/AM_LerpPoseSampler.h"
#include "AM/PoseSampler/AM_MaskedPoseSampler.h"
#include "AM/Types/AM_PoseMask.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/YakuEngine.h"

#include <utility>

namespace HF_Player_Private
{
    constexpr float RateOfTurn = YK_ToRadians(3.0f * 90.0f);

    constexpr float HammerDistance = -2.5f;

    AM_AnimationPoseSampler IdlePoseSampler;
    AM_AnimationPoseSampler WalkPoseSampler;
    AM_LerpPoseSampler MovementPoseSampler;

    // From L to R
    AM_AnimationPoseSampler HammerPoseSamplers[5];
    AM_BlendPoseSampler HammerBlendPoseSampler;

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

    // Load Pose Samplers
    HF_Player_Private::IdlePoseSampler =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Idle.YKA")));
    HF_Player_Private::WalkPoseSampler =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/Walk_F.YKA")));

    HF_Player_Private::HammerPoseSamplers[0] =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/P_H_L.YKA")));
    HF_Player_Private::HammerPoseSamplers[1] =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/P_H_BL.YKA")));
    HF_Player_Private::HammerPoseSamplers[2] =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/P_H_Back.YKA")));
    HF_Player_Private::HammerPoseSamplers[3] =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/P_H_BR.YKA")));
    HF_Player_Private::HammerPoseSamplers[4] =
      AM_AnimationPoseSampler(&assetManager.GetAsset<CG_Animation>(YK_FilePath("Animations/P_H_R.YKA")));

    // End Load Pose Samplers

    // Setup Intermediary Pose Samplers
    HF_Player_Private::MovementPoseSampler =
      AM_LerpPoseSampler(&HF_Player_Private::IdlePoseSampler, &HF_Player_Private::WalkPoseSampler);

    HF_Player_Private::HammerBlendPoseSampler = AM_BlendPoseSampler();
    HF_Player_Private::HammerBlendPoseSampler.AddSampler(&HF_Player_Private::HammerPoseSamplers[0], -1.0f);
    HF_Player_Private::HammerBlendPoseSampler.AddSampler(&HF_Player_Private::HammerPoseSamplers[1], -0.5f);
    HF_Player_Private::HammerBlendPoseSampler.AddSampler(&HF_Player_Private::HammerPoseSamplers[2], 0.0f);
    HF_Player_Private::HammerBlendPoseSampler.AddSampler(&HF_Player_Private::HammerPoseSamplers[3], 0.5f);
    HF_Player_Private::HammerBlendPoseSampler.AddSampler(&HF_Player_Private::HammerPoseSamplers[4], 1.0f);
    // End Setup Intermediary Pose Samplers

    AM_PoseMask armsAndHammerMask;
    armsAndHammerMask.Enable(1);
    armsAndHammerMask.Enable(2);
    armsAndHammerMask.Enable(3);
    armsAndHammerMask.Enable(4);
    armsAndHammerMask.Enable(20);
    armsAndHammerMask.Enable(21);
    armsAndHammerMask.Enable(22);

    animationComponent.m_poseSampler = AM_MaskedPoseSampler(armsAndHammerMask,
                                                            &HF_Player_Private::HammerBlendPoseSampler,
                                                            &HF_Player_Private::MovementPoseSampler);

    CG_CameraComponent playerCamera;
    playerCamera.m_transform.m_position = YK_Vector3f::Up();

    m_hammerPosition = playerTransform.m_position + playerTransform.Forward2D() * HF_Player_Private::HammerDistance;

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

    // TODO: Add YK_Vector::ClampMagnitude and add it here
    YK_Vector3f const worldMovementInput = (cameraForward2D * -leftStick.m_y) + (cameraRight * leftStick.m_x);

    YK_Vector3f const oldForward = playerTransform->Forward();
    float const angleToDesiredMovement = YK_SignedAngleBetween2D(oldForward, worldMovementInput);
    float const maximumTurnThisFrame = HF_Player_Private::RateOfTurn * YK_Time::DeltaTime();
    float const angleThisFrame = YK_Clamp(angleToDesiredMovement, -maximumTurnThisFrame, maximumTurnThisFrame);

    YK_Vector3f const worldDesiredMovement =
      YK_Quaternion(-YK_Vector3f::Up(), angleThisFrame) * oldForward * YK_Vector::Magnitude(worldMovementInput);

    YK_Vector3f const previousPosition = playerTransform->m_position;
    playerTransform->m_position += worldDesiredMovement * YK_Time::DeltaTime() * 8.0f;

    YK_Vector3f const positionToHammer = YK_Vector::GetNormalized(playerTransform->m_position - m_hammerPosition);
    m_hammerPosition = playerTransform->m_position + positionToHammer * HF_Player_Private::HammerDistance;

    if (!YK_Vector::ApproximatelyEqual(worldDesiredMovement, YK_Vector3f::Zero()))
    {
        playerTransform->m_orientation = YK_Matrix::LookAt(worldDesiredMovement, YK_Vector3f::Zero());
    }

    constexpr float radians90Degreees = YK_ToRadians(90.0f);

    YK_Vector3f const forward = playerTransform->Forward();
    float const unclampedAngleToHammer = YK_SignedAngleBetween2D(forward, positionToHammer);
    float const angleToHammer = YK_Clamp(unclampedAngleToHammer, -radians90Degreees, radians90Degreees);

    AM_MaskedPoseSampler& rootPoseSampler = animationComponent->m_poseSampler;
    AM_BlendPoseSampler& hammerSampler = static_cast<AM_BlendPoseSampler&>(rootPoseSampler.GetInMaskSampler());
    AM_LerpPoseSampler& movementSampler = static_cast<AM_LerpPoseSampler&>(rootPoseSampler.GetOutOfMaskSampler());

    hammerSampler.SetBlendValue(static_cast<double>(-angleToHammer / radians90Degreees));
    movementSampler.SetSampleFactor(YK_Vector::Magnitude(YK_Vector2f(leftStick.m_x, leftStick.m_y)));

    // Update camera position last to prevent jitter
    YK_Vector3f cameraForward = camera->m_transform.Forward();
    camera->m_transform.m_position = playerTransform->m_position;
    camera->m_transform.m_position += cameraForward * HF_Player_Private::CameraDistance;
}