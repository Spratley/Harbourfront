#include "PCH/HFGame_PCH.h"

#include "ECS/HF_ECSRegistry.h"
#include "Game/HF_Game.h"

#include "EN/YakuEngine.h"

YakuEngine engine(HF_ComponentTypes{}, HF_SystemTypes{});

// TODO: Don't depend on Windows here
// Main signature should be in some Yaku Engine define
#if YAKU_RETAIL && YK_PLATFORM == YK_WINDOWS
int WinMain()
#else
int main()
#endif // YAKU_RETAIL
{
    engine.GetEngineSystemManager().RegisterSystem<HF_Game>();
    engine.Run();
    return 0;
}