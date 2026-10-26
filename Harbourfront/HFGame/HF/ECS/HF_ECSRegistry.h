#pragma once

#include "EN/ECS/EN_ECSRegistry.h"

#include "HF/HF_Test.h"
#include "HF/Player/HF_HammerComponent.h"
#include "HF/Player/HF_PlayerComponent.h"

namespace HF_ECSRegistry
{
    using ComponentTypes = Zen::TypeList<HF_TestConstraintComponent, HF_HammerComponent, HF_PlayerComponent>;
    using SystemTypes = Zen::TypeList<>;
} // namespace HF_ECSRegistry

using HF_ComponentTypes = Zen::TypeListUtils::Concat_T<EN_ComponentTypes, HF_ECSRegistry::ComponentTypes>;
using HF_SystemTypes = Zen::TypeListUtils::Concat_T<EN_SystemTypes, HF_ECSRegistry::SystemTypes>;