#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/viewshaker.hpp>

#include "ActorFixes.hpp"

// Thwomp ---------------------------------------------------------------------------

static void Thwomp_fixViewShake(u8 type, StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (ActorFixes_isPlayerInShakeRange(self, player))
			ViewShaker::start(3, self->viewID, playerID, false);
	}
}

ncp_repl(0x0213DEE0, 24, "MOV R1, R4")
ncp_set_call(0x0213DEE8, 24, Thwomp_fixViewShake)

ncp_repl(0x0213DF30, 24, "MOV R1, R4")
ncp_set_call(0x0213DF38, 24, Thwomp_fixViewShake)

ncp_repl(0x0213DF28, 24, "NOP; NOP") // Try to shake view even if sound not in range
