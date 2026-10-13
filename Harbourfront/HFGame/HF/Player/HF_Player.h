#pragma once

#include "YK/Libraries/Zen/Entity/Zen_Entity.h"
#include "YK/Utils/YK_TypeUtils.h"

// This should probably just be a factory that spits back a Zen::Entity handle
class HF_Player : YK_NotCopyableNotMovable
{
public:
    HF_Player();
    ~HF_Player();

    void Update();

private:
    Zen::Entity m_playerEntity;
};