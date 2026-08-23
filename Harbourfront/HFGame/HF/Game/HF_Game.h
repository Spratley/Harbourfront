#pragma once

class YK_Core;

// TODO Migrate out of this
class HF_Game
{
public:
    static bool Init(YK_Core& p_engine);
    static void Update(YK_Core& p_engine);
    static void ShutDown(YK_Core& p_engine);
};