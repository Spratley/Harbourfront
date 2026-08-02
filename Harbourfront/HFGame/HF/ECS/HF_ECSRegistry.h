#pragma once

#include "YK/ECS/YK_ECSRegistry.h"

namespace HF_ECSRegistry
{
    using ComponentTypes = Zen::TypeList<>;
    using SystemTypes = Zen::TypeList<>;
} // namespace HF_ECSRegistry

using HF_ComponentTypes = Zen::TypeListUtils::Concat_T<YK_ComponentTypes, HF_ECSRegistry::ComponentTypes>;
using HF_SystemTypes = Zen::TypeListUtils::Concat_T<YK_SystemTypes, HF_ECSRegistry::SystemTypes>;