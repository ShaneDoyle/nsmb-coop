#include <nsmb/game/game.hpp>
#include <nsmb/core/system/save.hpp>

#include "Widescreen.hpp"

ncp_call(0x020CC6E8, 1)
void Scene_prepareFirstScene_setSoundModeHook(OptionSave::Sound type)
{
	Game::setSoundMode(type); // Keep replaced instruction

	Widescreen::enabled[0] = (Save::optionSave.flags & (1 << 1)) != 0;
}
