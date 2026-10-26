#pragma once

#include "PP/Types/PP_BodyID.h"

// TEMP
#include "YK/Libraries/Zen/Entity/Zen_Entity.h"

struct HF_HammerComponent
{
    HF_HammerComponent();
    HF_HammerComponent(HF_HammerComponent&& p_other)
        : m_hammerBody(YK_Exchange(p_other.m_hammerBody, PP_BodyID::Invalid))
    {}

    ~HF_HammerComponent();

    PP_BodyID m_hammerBody = PP_BodyID::Invalid;
};