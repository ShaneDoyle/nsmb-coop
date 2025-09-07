#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>

#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::BowserJr {

ncp_over(0x02141438, 28) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_repl(0x0213CA5C, 28, "MOV R0, R4")
ncp_set_call(0x0213CA70, 28, CoopActor::isOutsideCamera)

ncp_repl(0x0213CC70, 28, "MOV R0, R4")
ncp_set_call(0x0213CC84, 28, CoopActor::isOutsideCamera)

// Victory freeze on ground touch

ncp_call(0x0213D0AC, 28)
ncp_thumb void call_0213D0AC_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->actionFlag.bowserJrBeaten = true;
	}
}

ncp_repl(0x0213D0B0, 28, "NOP")
ncp_repl(0x0213D0B8, 28, "NOP; NOP")

// ov28:021FF154 (dunno what it does, doesn't desync, might not need changing)

// Freeze the player while falling

ncp_repl(0x0213F49C, 28, "NOP")

ncp_call(0x0213F4A0, 28)
ncp_thumb void call_0213F4A0_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		player->subActionFlag.releaseKeys = true;
		player->velH = 0;
	}
}

// Freeze player when touching the ground after falling

ncp_set_call(0x0213F4B4, 28, BossControllerCommon::beginCutsceneAllPlayers)

// Unfreeze when battle is ready to start

ncp_set_call(0x0213F550, 28, BossControllerCommon::endCutsceneAllPlayers)

// Fix Blue Shell hit

ncp_repl(0x0213FF54, 28, "ADD R0, R4, #0x100")
ncp_repl(0x0213FF5C, 28, "LDRSB R0, [R0,#0x1E]")

// Fix Ground Pound

ncp_repl(0x0213FFC4, 28, "ADD R1, R0, #0x100")
ncp_repl(0x0213FFCC, 28, "LDRSB R0, [R1,#0x1E]")

// Bowser Jr. camera spawn fix
ncp_jump(0x0213BFA8, 28)
ncp_asmfunc void jump_0213BFA8_ov28()
{asm(R"(
	LDR     R1, =0x1000001
	B       0x0213BFAC
)");}

// Stage clearer is always player 0
ncp_repl(0x0213D1AC, 28, "MOV R2, #0")

} // namespace CoopFixes::BowserJr
