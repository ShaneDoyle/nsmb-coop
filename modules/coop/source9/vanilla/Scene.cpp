#include <nsmb/game/game.hpp>
#include <nsmb/core/system/save.hpp>

#ifdef MODULE_WIDESCREEN
#include "widescreen/widescreen.hpp"
#endif

ncp_call(0x020CC6E8, 1)
void Scene_prepareFirstScene_setSoundModeHook(OptionSave::Sound type)
{
	Game::setSoundMode(type); // Keep replaced instruction
#ifdef MODULE_WIDESCREEN
	Widescreen::loadSaveOption();
#endif
}
