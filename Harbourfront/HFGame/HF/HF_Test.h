#pragma once

#include "YK/Libraries/Zen/Zen_ForwardDeclare.h"
#include "YK/Math/YK_NumericLimits.h"
#include "YK/Types/Math/YK_Integer.h"
#include "YK/Utils/YK_AlgorithmUtils.h"

#include "PP/Types/PP_BodyID.h"
#include "PP/Types/PP_ConstraintReference.h"

class HF_TestBallSpawner
{
public:
    static Zen::Entity Spawn();

    static void Synchronize(Zen::Entity& p_entity);
};

struct HF_TestConstraintComponent
{
    HF_TestConstraintComponent();
    HF_TestConstraintComponent(HF_TestConstraintComponent&& p_other) noexcept
        : m_joltBodyAID(YK_Exchange(p_other.m_joltBodyAID, PP_BodyID::Invalid))
        , m_joltBodyBID(YK_Exchange(p_other.m_joltBodyBID, PP_BodyID::Invalid))
        , m_constraint(std::move(p_other.m_constraint))
    {}

    ~HF_TestConstraintComponent();

    PP_BodyID m_joltBodyAID = PP_BodyID::Invalid;
    PP_BodyID m_joltBodyBID = PP_BodyID::Invalid;
    PP_ConstraintReference m_constraint;
};