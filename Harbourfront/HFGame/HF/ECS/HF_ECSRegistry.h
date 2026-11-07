#pragma once

#include "EN/ECS/EN_ECSRegistry.h"

#include "HF/Player/HF_HammerComponent.h"
#include "HF/Player/HF_PlayerComponent.h"

#include "HF/Character/HF_CharacterComponent.h"
#include "HF/Character/HF_CharacterUpdateSystem.h"

namespace HF_ECSRegistry
{
    using ComponentTypes = Zen::TypeList<HF_HammerComponent, HF_PlayerComponent, HF_CharacterComponent>;
    using SystemTypes = Zen::TypeList<HF_CharacterUpdateSystem>;
} // namespace HF_ECSRegistry

using HF_ComponentTypes = Zen::TypeListUtils::Concat_T<EN_ComponentTypes, HF_ECSRegistry::ComponentTypes>;
using HF_SystemTypes = Zen::TypeListUtils::Concat_T<EN_SystemTypes, HF_ECSRegistry::SystemTypes>;