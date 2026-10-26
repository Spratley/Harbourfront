#include "PCH/HFGame_PCH.h"
#include "HF_HammerComponent.h"

#include <Jolt/Jolt.h>
#include "PP/JoltConfig/PP_JoltLayers.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyCreationSettings.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PP/PP_PhysicsModule.h"

HF_HammerComponent::HF_HammerComponent()
{
    PP_JoltLayers::Layers const dynamicLayer = PP_JoltLayers::DYNAMIC;

    JPH::BodyCreationSettings hammerSettings{
        new JPH::BoxShape{ JPH::RVec3{ 0.5f, 0.5f, 0.5f } },
        JPH::RVec3{ 0.0f, 2.0f, 0.0f },
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

    JPH::Body* hammerBody = createBody(hammerSettings);
    YK_ASSERT(hammerBody, "Failed to create body!");

    m_hammerBody = hammerBody->GetID();
    //m_joltBodyAID = bodyA->GetID().GetIndexAndSequenceNumber();
    //m_joltBodyBID = bodyB->GetID().GetIndexAndSequenceNumber();

    //JPH::PointConstraintSettings constraintSettings;
    //constraintSettings.mPoint1 = JPH::RVec3{ 0.0f, 2.0f, 0.0f };
    //constraintSettings.mPoint2 = constraintSettings.mPoint1;

    //m_constraint = constraintSettings.Create(*bodyA, *bodyB);
    //physicsSystem.AddConstraint(m_constraint.GetAs<JPH::PointConstraint>());
}

HF_HammerComponent::~HF_HammerComponent()
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

    removeBody(m_hammerBody);

    /*if (m_constraint.IsValid())
    {
        physicsSystem.RemoveConstraint(m_constraint.GetAs<JPH::PointConstraint>());
    }*/
}