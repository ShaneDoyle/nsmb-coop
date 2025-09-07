#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/math/vector.hpp>

#include "coop/CoopActor.hpp"
#include "coop/CoopStage.hpp"

namespace CoopFixes::PrincessPeach {

ncp_repl(0x021443C8, 40, "MOV R0, R4")
ncp_set_call(0x021443CC, 40, CoopActor::getClosestPlayer)

ncp_repl(0x021444CC, 40, "MOV R0, R4")
ncp_set_call(0x021444D0, 40, CoopActor::getClosestPlayer)

ncp_repl(0x021447A4, 40, "MOV R0, R4")
ncp_set_call(0x021447AC, 40, CoopActor::getClosestPlayer)

ncp_set_call(0x02144530, 40, CoopStage::setZoomAll_impl)

// If player 0 is not alive when we reach the cutscene
// where Bowser Jr. runs away with Peach, the game soft locks.
// I have no idea what the issue is, so I just force player 0 to spawn.
ncp_call(0x02144A04, 40)
void fixPeachKidnap(s32 sfxID, Vec3* pos)
{
	SND::playSFX(sfxID, pos); // Keep replaced instruction

	if (Game::getPlayerDead(0))
		Game::getPlayer(0)->spawnDefault();
}

} // namespace CoopFixes::PrincessPeach
