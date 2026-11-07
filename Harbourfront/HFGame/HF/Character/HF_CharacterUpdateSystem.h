#pragma once

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/Libraries/Zen/System/Zen_System.h"

#include "HF/Character/HF_CharacterComponent.h"

class HF_CharacterUpdateSystem
    : public Zen::SystemBase<HF_CharacterUpdateSystem, YK_TransformComponent, HF_CharacterComponent>
{
public:
    static void Tick(ComponentView const& /*p_components*/) {}
};