#pragma once

#include "nsmb/stage/player/player.hpp"

namespace PlayerDragger
{
	void beginDrag(Player* player);

	inline void endDrag(Player* player)
	{
		player->endCutscene();
	}
};
