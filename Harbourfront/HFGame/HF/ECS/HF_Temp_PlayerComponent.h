#pragma once

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/Libraries/Zen/System/Zen_System.h"
#include "YK/Time/YK_Time.h"

#include "EN/Libraries/HIDra/HIDra.h"

#include "YK/Math/YK_VectorMath.h"

#include <algorithm>
#include <cmath>

struct HF_PlayerComponent
{
    YK_Vector2f m_heading{ 0, 1 };
};

template <typename DataType>
constexpr DataType YK_Sign(DataType p_value)
{
    return static_cast<DataType>(static_cast<DataType>(0) <= p_value)
           - static_cast<DataType>(static_cast<DataType>(0) > p_value);
}

struct HF_PlayerSystem : public Zen::SystemBase<HF_PlayerSystem, YK_TransformComponent, HF_PlayerComponent>
{
    static void Tick(ComponentView const& p_components)
    {
        float elapsedTime = YK_Time::ElapsedTime();
        float deltaTime = YK_Time::DeltaTime();

        HIDra::Vec2f movement = HIDra::GetAxis2D(HIDra::AID_STICK_L);
        YK_Vector2f desiredHeading(movement.m_x, -movement.m_y);
        float movementScale = YK_Vector::NormalizeAndGetMagnitude(desiredHeading);

        if (movementScale < 0.05f)
        {
            return;
        }

        for (auto [transform, player] : p_components)
        {
            float angle = std::acos(std::min(YK_Vector::Dot(player.m_heading, desiredHeading), 1.0f));
            float direction =
              YK_Sign((player.m_heading.x * desiredHeading.y) - (desiredHeading.x * player.m_heading.y));

            float angleDiff = std::min(10.0f * deltaTime, angle) * direction;

            float cosAngle = std::cos(angleDiff);
            float sinAngle = std::sin(angleDiff);
            player.m_heading = YK_Vector2f{ (player.m_heading.x * cosAngle) - (player.m_heading.y * sinAngle),
                                            (player.m_heading.x * sinAngle) + (player.m_heading.y * cosAngle) };

            YK_Quaternion heading =
              YK_Quaternion(YK_Vector3f::Up(), std::atan2(player.m_heading.x, player.m_heading.y));
            YK_Quaternion tilt =
              YK_Quaternion(YK_Vector3f::Forward(), std::sin(elapsedTime * 12.0f) * 0.1f * movementScale);
            transform.m_orientation = heading * tilt;

            transform.m_position += heading * YK_Vector3f::Forward() * 3.0f * movementScale * deltaTime;
            transform.m_position.y = std::abs(std::sin(elapsedTime * 12.0f) * 0.2f * movementScale);
        }
    }
};