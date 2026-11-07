#include "PCH/HFGame_PCH.h"
#include "HF_PlayerComponent.h"

#include <Jolt/Jolt.h>
#include "PP/JoltConfig/PP_JoltLayers.h"
#include "PP/Libraries/Jolt/Physics/Body/BodyCreationSettings.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/SphereShape.h"
#include "PP/PP_PhysicsModule.h"
#include "PP/Types/PP_JoltTypeConversion.h"

HF_PlayerComponent::HF_PlayerComponent(YK_Transform const& p_playerTransform)
{
    PP_JoltLayers::Layers const dynamicLayer = PP_JoltLayers::DYNAMIC;

    JPH::BodyCreationSettings playerSettings{ new JPH::SphereShape{ Radius },
                                              PP_ToJolt(p_playerTransform.m_position
                                                        + YK_Vector3f{ 0.0f, Radius, 0.0f }),
                                              PP_ToJolt(YK_Quaternion{}),
                                              JPH::EMotionType::Kinematic,
                                              dynamicLayer };

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

    JPH::Body* PlayerBody = createBody(playerSettings);
    YK_ASSERT(PlayerBody, "Failed to create body!");

    m_playerBody = PlayerBody->GetID();
}

HF_PlayerComponent::~HF_PlayerComponent()
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

    removeBody(m_playerBody);
}