#include "PCH/HFGame_PCH.h"
#include "HF_Test.h"

#include "YK/Core/YK_Core.h"
#include "YK/Debugging/YK_Assert.h"
#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Entity/Zen_Entity.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Math/YK_AngleMath.h"
#include "YK/Math/YK_NumericLimits.h"
#include "YK/Time/YK_Time.h"
#include "YK/Types/Math/YK_Integer.h"
#include "YK/Types/Math/YK_Quaternion.h"
#include "YK/Types/Math/YK_Vector.h"

#include "CG/ECS/CG_Components.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"

#include <Jolt/Jolt.h>
#include "PP/ECS/PP_RigidBodyComponent.h"
#include "PP/JoltConfig/PP_JoltLayers.h"
#include "PP/Libraries/Jolt/Math/Quat.h"
#include "PP/Libraries/Jolt/Math/Real.h"
#include "PP/Libraries/Jolt/Physics/Body/Body.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyCreationSettings.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyID.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyInterface.h"
#include "PP/Libraries/Jolt/Physics/Body/MotionType.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/Shape.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/SphereShape.h"
#include "PP/Libraries/Jolt/Physics/Constraints/PointConstraint.h"
#include "PP/Libraries/Jolt/Physics/EActivation.h"
#include "PP/Libraries/Jolt/Physics/PhysicsSystem.h"
#include "PP/PP_PhysicsModule.h"
#include "PP/Types/PP_BodyID.h"

#include "EN/YakuEngine.h"

#include <utility>

Zen::Entity HF_TestBallSpawner::Spawn()
{
    YakuEngine& engine = static_cast<YakuEngine&>(YK_Core::GetEngine());
    YK_AssetManager& assetManager = engine.GetAssetManager();
    Zen::Garden& entityGarden = engine.GetZenGarden();

    YK_TransformComponent transform{
        YK_Vector3f{ 0.0f, 1.5f, 0.0f },
        YK_Quaternion{},
        YK_Vector3f::One()
    };

    CG_RendererComponent renderer{ .m_material =
                                     assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Error.YKM")) };

    CG_MeshComponent mesh{ .m_mesh = assetManager.GetAsset<CG_Mesh>(YK_FilePath("Models/Sphere.obj")) };

    HF_TestConstraintComponent testComponent;

    return entityGarden.Spawn(std::move(transform), std::move(renderer), std::move(mesh), std::move(testComponent));
}

void HF_TestBallSpawner::Synchronize(Zen::Entity& p_entity)
{
    HF_TestConstraintComponent* testConstraintComponent = p_entity.GetComponent<HF_TestConstraintComponent>();
    YK_TransformComponent* testTransform = p_entity.GetComponent<YK_TransformComponent>();

    YK_ASSERT(testConstraintComponent && testTransform,
              "Must call Synchronize only on entities spanwed via TestBallSpawner!");

    PP_PhysicsModule& physicsInstance = PP_PhysicsModule::GetPhysicsInstance();
    JPH::PhysicsSystem& physicsSystem = physicsInstance.GetJoltInstance();
    JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    JPH::RVec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(JPH::BodyID(testConstraintComponent->m_joltBodyBID), position, rotation);

    testTransform->m_position = { position.GetX(), position.GetY(), position.GetZ() };
    testTransform->m_orientation = { rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() };

    bodyInterface.MoveKinematic(JPH::BodyID(testConstraintComponent->m_joltBodyAID),
                                JPH::RVec3{ 0.0f, YK_Sine(YK_Time::ElapsedTime()) + 4.0f, 0.0f },
                                JPH::Quat{ 0.0f, 0.0f, 0.0f, 1.0f },
                                YK_Time::PhysicsDeltaTime());
}

HF_TestConstraintComponent::HF_TestConstraintComponent()
{
    PP_JoltLayers::Layers const dynamicLayer = PP_JoltLayers::DYNAMIC;

    JPH::BodyCreationSettings aSettings{
        new JPH::SphereShape{ 0.5f },
        JPH::RVec3{ 0.0f, 2.0f, 0.0f },
        JPH::Quat{ 0.0f, 0.0f, 0.0f, 1.0f },
        JPH::EMotionType::Kinematic,
        dynamicLayer
    };

    JPH::BodyCreationSettings bSettings{
        new JPH::SphereShape{ 0.5f },
        JPH::RVec3{ 1.0f, 2.0f, 0.0f },
        JPH::Quat{ 0.0f, 0.0f, 0.0f, 1.0f },
        JPH::EMotionType::Dynamic,
        dynamicLayer
    };

    PP_PhysicsModule& physicsInstance = PP_PhysicsModule::GetPhysicsInstance();
    JPH::PhysicsSystem& physicsSystem = physicsInstance.GetJoltInstance();
    JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    auto createBody = [&bodyInterface](JPH::BodyCreationSettings& p_settings) -> JPH::Body* {
        JPH::Body* body = bodyInterface.CreateBody(p_settings);
        if (!body)
        {
            return nullptr;
        }
        bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
        return body;
    };

    JPH::Body* bodyA = createBody(aSettings);
    JPH::Body* bodyB = createBody(bSettings);

    YK_ASSERT(bodyA && bodyB, "Failed to create bodies!");

    m_joltBodyAID = bodyA->GetID().GetIndexAndSequenceNumber();
    m_joltBodyBID = bodyB->GetID().GetIndexAndSequenceNumber();

    JPH::PointConstraintSettings constraintSettings;
    constraintSettings.mPoint1 = JPH::RVec3{ 0.0f, 2.0f, 0.0f };
    constraintSettings.mPoint2 = constraintSettings.mPoint1;

    m_constraint = constraintSettings.Create(*bodyA, *bodyB);
    physicsSystem.AddConstraint(m_constraint.GetAs<JPH::PointConstraint>());
}

HF_TestConstraintComponent::~HF_TestConstraintComponent()
{
    PP_PhysicsModule& physicsInstance = PP_PhysicsModule::GetPhysicsInstance();
    JPH::PhysicsSystem& physicsSystem = physicsInstance.GetJoltInstance();
    JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    auto removeBody = [&bodyInterface](PP_BodyID& p_body) {
        if (p_body.IsValid())
        {
            bodyInterface.RemoveBody(p_body);
            bodyInterface.DestroyBody(p_body);
        }
        p_body = PP_BodyID::Invalid;
    };

    removeBody(m_joltBodyAID);
    removeBody(m_joltBodyBID);

    if (m_constraint.IsValid())
    {
        physicsSystem.RemoveConstraint(m_constraint.GetAs<JPH::PointConstraint>());
    }
}