#pragma once

#include "YK/Libraries/Zen/Entity/Zen_Entity.h"

class HF_PlayerSpawner
{
public:
    static Zen::Entity SpawnPlayer(YK_Vector3f const& p_position);
};