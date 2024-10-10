#include "nsmb/game.h"
#include "nsmb/sound.h"
#include "nsmb/stage/entity.h"
#include "nsmb/stage/viewshaker.h"
#include "ActorFixes.hpp"

asm(R"(
	SledgeBro_tryShakePlayer = 0x02174DE4
)");
extern "C" {
	void SledgeBro_tryShakePlayer(StageEntity* self, s32 playerID);
}

// Sledge Bro -----------------------------------------------------------

ncp_over(0x02175880, 56) const auto SledgeBro_skipRender = ActorFixes_safeSkipRender;

static bool SledgeBro_canTryShakePlayer(StageEntity* self, Player* player)
{
	const fx32 range = 0x100000; // 16 tiles

	bool inRange = Math::abs(self->position.x - player->position.x) < range &&
	               Math::abs(self->position.y - player->position.y) < range;

	return inRange && !Game::getPlayerDead(player->linkedPlayerID);
}

NTR_USED static void SledgeBro_fixShakePlayer(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (SledgeBro_canTryShakePlayer(self, player))
		{
			ViewShaker::start(3, self->viewID, playerID, false);
			if (playerID == Game::localPlayerID)
				SND::playSFX(138, &self->position);
			SledgeBro_tryShakePlayer(self, playerID);
		}
	}
}

ncp_repl(0x02174614, 56, R"(
	MOV     R0, R4
	BL      _ZL24SledgeBro_fixShakePlayerP11StageEntity
	B       0x02174658
)");
