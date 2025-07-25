#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/viewshaker.hpp>

#include "ActorFixes.hpp"

// Whomp ---------------------------------------------------------------------------

static void Whomp_fixViewShake(u8 type, StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (ActorFixes_isPlayerInShakeRange(self, player))
			ViewShaker::start(3, self->viewID, playerID, false);
	}
}

ncp_repl(0x02143BE8, 34, "MOV R1, R4")
ncp_set_call(0x02143BF0, 34, Whomp_fixViewShake)
