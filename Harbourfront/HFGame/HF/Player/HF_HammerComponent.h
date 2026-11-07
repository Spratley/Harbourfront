#pragma once

#include "PP/Types/PP_BodyID.h"
#include "PP/Types/PP_ConstraintReference.h"

struct HF_HammerComponent
{
    HF_HammerComponent(YK_Transform const& p_attachedPlayerTransform, PP_BodyID p_attachedPlayer);
    HF_HammerComponent(HF_HammerComponent&& p_other)
        : m_hammerBody(YK_Exchange(p_other.m_hammerBody, PP_BodyID::Invalid))
        , m_constraint(std::move(p_other.m_constraint))
    {}

    ~HF_HammerComponent();

    PP_BodyID m_hammerBody = PP_BodyID::Invalid;
    PP_ConstraintReference m_constraint;
};