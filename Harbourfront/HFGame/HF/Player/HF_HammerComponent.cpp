#include "PCH/HFGame_PCH.h"
#include "HF_HammerComponent.h"

#include <Jolt/Jolt.h>
#include "PP/JoltConfig/PP_JoltLayers.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyCreationSettings.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PP/Libraries/Jolt/Physics/Constraints/PointConstraint.h"
#include "PP/PP_PhysicsModule.h"

#include "HF/Player/HF_PlayerComponent.h"

HF_HammerComponent::HF_HammerComponent(YK_Transform const& p_attachedPlayerTransform, PP_BodyID p_attachedPlayer)
{
    PP_JoltLayers::Layers const dynamicLayer = PP_JoltLayers::DYNAMIC;

    JPH::BodyCreationSettings hammerSettings{
        new JPH::BoxShape{ JPH::RVec3{ 0.5f, 0.5f, 0.5f } },
        JPH::RVec3{ p_attachedPlayerTransform.m_position.x,
                          p_attachedPlayerTransform.m_position.y + HF_PlayerComponent::Radius,
                          p_attachedPlayerTransform.m_position.z - 2.5f },
        JPH::Quat{ 0.0f, 0.0f, 0.0f, 1.0f },
        JPH::EMotionType::Dynamic,
        dynamicLayer
    };
    hammerSettings.mFriction = 1.0f;
    hammerSettings.mLinearDamping = 1.0f;

    JPH::MassProperties hammerMassProperties;
    hammerMassProperties.mMass = 15.0f;
    hammerSettings.mMassPropertiesOverride = hammerMassProperties;
    hammerSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

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

    hammerBody->SetFriction(1.0f);
    m_hammerBody = hammerBody->GetID();

    JPH::PointConstraintSettings constraintSettings;

    constraintSettings.mPoint1 = JPH::RVec3{ p_attachedPlayerTransform.m_position.x,
                                             p_attachedPlayerTransform.m_position.y + HF_PlayerComponent::Radius,
                                             p_attachedPlayerTransform.m_position.z };
    constraintSettings.mPoint2 = constraintSettings.mPoint1;
    constraintSettings.mSpace = JPH::EConstraintSpace::WorldSpace;

    JPH::Body* playerBody = physicsSystem.GetBodyLockInterface().TryGetBody(p_attachedPlayer);
    YK_ASSERT(playerBody, "Hammer component created without valid player body!");

    m_constraint = constraintSettings.Create(*playerBody, *hammerBody);
    physicsSystem.AddConstraint(m_constraint.GetAs<JPH::PointConstraint>());
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

    if (m_constraint.IsValid())
    {
        physicsSystem.RemoveConstraint(m_constraint.GetAs<JPH::PointConstraint>());
    }
}