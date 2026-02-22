#include "PCH/HFGame_PCH.h"

#include "Game/HF_Game.h"

#include "YK/YakuEngine.h"

YakuEngine engine;

// TODO: Don't depend on Windows here
// Main signature should be in some Yaku Engine define
#if YAKU_RETAIL && YK_Windows
int WinMain()
#else
int main()
#endif // YAKU_RETAIL
{
    engine.Run<HF_Game>();
    return 0;
}