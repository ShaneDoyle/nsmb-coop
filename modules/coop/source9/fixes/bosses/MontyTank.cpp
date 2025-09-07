#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/core/math/vector.hpp>

#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::MontyTank {

ncp_over(0x021374C0, 19) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_set_call(0x021361E4, 19, BossControllerCommon::endCutsceneAllPlayers)

ncp_call(0x0213624C, 19)
ncp_thumb void MontyTank_doPlayerBossBump(Player* r0, const Vec2& velocity)
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		player->doBossBump(velocity);
	}
}

} // namespace CoopFixes::MontyTank
