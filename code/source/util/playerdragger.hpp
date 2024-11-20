#pragma once

#include <nsmb/nm/stage/player/player.hpp>

namespace PlayerDragger
{
	void beginDrag(Player* player);

	inline void endDrag(Player* player)
	{
		player->endCutscene();
	}
};
