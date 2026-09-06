#pragma once

class YK_Core;

class HF_Game
{
public:
    static void Init(YK_Core& p_engine);
    static void Update(YK_Core& p_engine);
    static void ShutDown(YK_Core& /*p_engine*/) {}
};