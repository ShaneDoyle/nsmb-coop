#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/core/filesystem/cache.hpp>

#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::PeteyPiranha {

ncp_over(0x02134790, 15) const auto vtbl_skipRender = CoopActor::safeSkipRender;

// Make better use of overlay memory by putting the animations in the overlay instead of the model

ncp_set_call(0x02134390, 15, FS::Cache::loadFile) // boss_packun.nsbmd
ncp_set_call(0x0213439C, 15, FS::Cache::loadFileToOverlay) // boss_packun.nsbca

// Must be saved because the actor delays the hit
u8 linkedPlayerID = 0;

Player* getPlayerOnSpecialHit(s32 _linkedPlayerID)
{
	linkedPlayerID = _linkedPlayerID;
	return Game::getPlayer(_linkedPlayerID); // Keep replaced instruction
}

ncp_set_call(0x021335BC, 15, getPlayerOnSpecialHit) // Save player ID on ground pound
ncp_set_call(0x021338CC, 15, getPlayerOnSpecialHit) // Save player ID on blue shell

Player* getLinkedPlayer()
{
	return Game::getPlayer(linkedPlayerID);
}

ncp_set_call(0x021307DC, 15, getLinkedPlayer)
ncp_set_call(0x02130CB0, 15, getLinkedPlayer)
ncp_set_call(0x02130E10, 15, getLinkedPlayer)
ncp_set_call(0x02133098, 15, getLinkedPlayer)
ncp_set_call(0x021330A4, 15, getLinkedPlayer)
ncp_set_call(0x02133120, 15, getLinkedPlayer)

// Save player ID on head hit

ncp_asmfunc void getPlayerOnPlatform()
{asm(R"(
	MOV     R5, R1
	PUSH    {R0-R1}
	LDR     R0, [R3,#4]
	LDRB    R0, [R0,#0x11E]
	LDR     R1, =_ZN9CoopFixes12PeteyPiranha14linkedPlayerIDE
	STRB    R0, [R1]
	POP     {R0-R1}
	BX      LR
)");}

ncp_repl(0x02132FD8, 15, "BLEQ _ZN9CoopFixes12PeteyPiranha19getPlayerOnPlatformEv")
ncp_repl(0x0213301C, 15, "BLEQ _ZN9CoopFixes12PeteyPiranha19getPlayerOnPlatformEv")
ncp_repl(0x02133060, 15, "BLEQ _ZN9CoopFixes12PeteyPiranha19getPlayerOnPlatformEv")

ncp_set_call(0x02132A58, 15, BossControllerCommon::endCutsceneAllPlayers)
ncp_set_call(0x02132BE8, 15, BossControllerCommon::endCutsceneAllPlayers)

// TODO: look into ov15 0x02132990

} // namespace CoopFixes::PeteyPiranha
