#include "PCH/HFGame_PCH.h"
#include "HF_Player.h"

#include "YK/Debugging/YK_Assert.h"
#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Entity/Zen_Entity.h"
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
#include "CG/DebugDraw/CG_DebugDrawer.h"
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
#include "AM/Resource/AM_BoneMask.h"
#include "AM/Resource/AM_ResourceHandles.h"

#include <Jolt/Jolt.h>
#include "PP/Libraries/Jolt/Physics/Body/BodyInterface.h"
#include "PP/PP_PhysicsModule.h"
#include "PP/Types/PP_JoltTypeConversion.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/YakuEngine.h"

#include "HF/Player/HF_HammerComponent.h"
#include "HF/Player/HF_PlayerComponent.h"
#include "HF/Player/HF_PlayerSpawner.h"

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

        if (HIDra::GetKey(HIDra::KEYCODE_ARROW_RIGHT))
        {
            rightStick.m_x = 1.0f;
        }
        else if (HIDra::GetKey(HIDra::KEYCODE_ARROW_LEFT))
        {
            rightStick.m_x = -1.0f;
        }

        if (HIDra::GetKey(HIDra::KEYCODE_ARROW_UP))
        {
            rightStick.m_y = 1.0f;
        }
        else if (HIDra::GetKey(HIDra::KEYCODE_ARROW_DOWN))
        {
            rightStick.m_y = -1.0f;
        }

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

    YK_Vector3f SynchronizePlayerCollider(YK_TransformComponent const& p_playerTransform, PP_BodyID p_playerBody)
    {
        PP_PhysicsModule& physicsInstance = PP_PhysicsModule::GetPhysicsInstance();
        JPH::PhysicsSystem& physicsSystem = physicsInstance.GetJoltInstance();
        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

        bodyInterface.MoveKinematic(p_playerBody,
                                    JPH::Vec3{ p_playerTransform.m_position.x,
                                               p_playerTransform.m_position.y + HF_PlayerComponent::Radius,
                                               p_playerTransform.m_position.z },
                                    PP_ToJolt(YK_Quaternion{}),
                                    YK_Time::PhysicsDeltaTime());

        JPH::Vec3 bodyPosition = bodyInterface.GetPosition(p_playerBody);
        return YK_Vector3f{ bodyPosition.GetX(), bodyPosition.GetY(), bodyPosition.GetZ() };
    }
} // namespace HF_Player_Private

HF_Player::HF_Player() { m_playerEntity = HF_PlayerSpawner::SpawnPlayer(YK_Vector3f(0.0f, 0.0f, -3.0f)); }

HF_Player::~HF_Player()
{
    YakuEngine& engine = YakuEngine::GetEngine();
    Zen::Garden& entityGarden = engine.GetZenGarden();
    entityGarden.Destroy(m_playerEntity);
}

void HF_Player::Update()
{
#pragma region Temp
    HF_HammerComponent* const hammerComponent = m_playerEntity.GetComponent<HF_HammerComponent>();

    JPH::BodyInterface& bodyInterface = PP_PhysicsModule::GetPhysicsInstance().GetJoltInstance().GetBodyInterface();
    JPH::RVec3 hammerPosition;
    JPH::Quat orientation;

    bodyInterface.GetPositionAndRotation(hammerComponent->m_hammerBody, hammerPosition, orientation);
    YK_Debug::GetDrawer().AddSphere(YK_Vector3f{ hammerPosition.GetX(), hammerPosition.GetY(), hammerPosition.GetZ() },
                                    0.5f);
#pragma endregion

    CG_CameraComponent* const camera = m_playerEntity.GetComponent<CG_CameraComponent>();
    YK_TransformComponent* const playerTransform = m_playerEntity.GetComponent<YK_TransformComponent>();
    AM_AnimationComponent* const animationComponent = m_playerEntity.GetComponent<AM_AnimationComponent>();
    HF_PlayerComponent* const playerComponent = m_playerEntity.GetComponent<HF_PlayerComponent>();

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

    if (!YK_Vector::ApproximatelyEqual(worldDesiredMovement, YK_Vector3f::Zero()))
    {
        playerTransform->m_orientation = YK_Matrix::LookAt(worldDesiredMovement, YK_Vector3f::Zero());
    }

    YK_Vector3f playerPosition =
      HF_Player_Private::SynchronizePlayerCollider(*playerTransform, playerComponent->m_playerBody);

    // This is drawn one frame before its actual position
    YK_Debug::GetDrawer().AddSphere(playerPosition, HF_PlayerComponent::Radius);

    constexpr float radians90Degreees = YK_ToRadians(90.0f);

    YK_Vector3f const positionToHammer = YK_Vector::GetNormalized(playerPosition - PP_ToNative(hammerPosition));

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