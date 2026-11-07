#pragma once

#include "YK\Utils\YK_AlgorithmUtils.h"

#include "PP\Types\PP_BodyID.h"

struct HF_PlayerComponent
{
    HF_PlayerComponent(YK_Transform const& p_playerTransform);
    HF_PlayerComponent(HF_PlayerComponent&& p_other)
        : m_playerBody(YK_Exchange(p_other.m_playerBody, PP_BodyID::Invalid))
    {}

    ~HF_PlayerComponent();

    static constexpr float Radius = 0.6f;

    PP_BodyID m_playerBody = PP_BodyID::Invalid;
};