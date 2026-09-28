#pragma once

#include "YK/Types/Other/YK_DeferredConstructible.h"

#include "HF/Player/HF_Player.h"

class YK_Core;

class HF_Game
{
public:
    static void Init(YK_Core& p_engine);
    static void Update(YK_Core& p_engine);
    static void ShutDown(YK_Core& /*p_engine*/);

private:
    static YK_DeferredConstructible<HF_Player> m_player;
};