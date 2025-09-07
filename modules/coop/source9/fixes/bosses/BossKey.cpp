#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/misc.hpp>

#include "coop/CoopPlayer.hpp"

namespace CoopFixes::BossKey {

ncp_call(0x0214617C, 40)
ncp_thumb Player* getPlayerWhoWon(s32 winnerPlayerID)
{
	Player* winnerPlayer = Game::getPlayer(winnerPlayerID);

	CoopPlayer::beginBossDefeatCutsceneCoop(winnerPlayer, false);

	Stage::setEvent(50); // Trigger event 50 for World 7 boss

	return winnerPlayer;
}

} // namespace CoopFixes::BossKey
