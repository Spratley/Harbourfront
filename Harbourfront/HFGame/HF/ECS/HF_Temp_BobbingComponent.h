#pragma once

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/Libraries/Zen/System/Zen_System.h"
#include "YK/Time/YK_Time.h"

#include <cmath>

struct HF_BobbingComponent
{
    float m_phase;
};

struct HF_BobbingSystem : public Zen::SystemBase<HF_BobbingSystem, YK_TransformComponent, HF_BobbingComponent>
{
    static void Tick(ComponentView const& p_components)
    {
        float elapsedTime = YK_Time::ElapsedTime();
        for (auto [transform, bobbing] : p_components)
        {
            transform.m_position.y = (std::sin(elapsedTime * 10.0f + bobbing.m_phase) * 0.25f) + 0.25f;
        }
    }
};