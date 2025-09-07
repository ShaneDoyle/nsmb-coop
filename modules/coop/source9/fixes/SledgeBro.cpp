#include <nsmb/game/sound.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/viewshaker.hpp>

#include "coop/CoopActor.hpp"

extern "C" {
	void CoopSym_SledgeBro_tryShakePlayer(StageEntity* self, s32 playerID);
}

namespace CoopFixes::SledgeBro {

ncp_over(0x02175880, 56) const auto SledgeBro_skipRender = CoopActor::safeSkipRender;

ncp_thumb void customShakePlayer(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (!CoopActor::isPlayerInRange(self, player))
			continue;

		ViewShaker::start(3, self->viewID, playerID, false);

		if (playerID == Game::localPlayerID)
			SND::playSFX(138, &self->position);

		if (!Game::getPlayerDead(player->linkedPlayerID))
			CoopSym_SledgeBro_tryShakePlayer(self, playerID);
	}
}

ncp_repl(0x02174614, 56, R"(
	MOV     R0, R4
	BL      _ZN9CoopFixes9SledgeBro17customShakePlayerEP11StageEntity
	B       0x02174658
)");

} // namespace CoopFixes::SledgeBro
