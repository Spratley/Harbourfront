#include "PCH/HFGame_PCH.h"

#include "Game/HF_Game.h"

#include "YK/YakuEngine.h"

YakuEngine engine;

#if YAKU_RETAIL
int WinMain()
#else
int main()
#endif // YAKU_RETAIL
{
	engine.Run<HF_Game>();
	return 0;
}