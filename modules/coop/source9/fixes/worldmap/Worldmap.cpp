#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

#include "coop/DesyncGuard.hpp"

extern "C" {
	s32 CoopSym_Worldmap_onCreate(Scene* self);
}

namespace CoopFixes::Worldmap {

u8 inputOwner = 0; // The ID of the console controlling the worldmap input (MUST BE 0)
u8 syncWaiting = 0;

void afterOnCreate(Scene* self)
{
	for (u32 playerID = 0; playerID < Wifi::getCommunicatingConsoleCount(); playerID++)
	{
		if (Game::getPlayerLives(playerID) <= 0)
			Game::setPlayerLives(playerID, 5);
	}
}

// Sync the worldmap load
s32 onCreate_ext(Scene* self)
{
	if (!syncWaiting)
	{
		CoopSym_Worldmap_onCreate(self);
		afterOnCreate(self);

		if (Net::isConnected())
			Net::Core::setMarker(0);

		syncWaiting = 1;
	}

	if (Net::isConnected() && !Net::Core::checkMarker(0))
		return -1;

	Net::Core::clearMarker(0);
	syncWaiting = 0;
	return 1;
}

ncp_over(0x020E67EC, 8) const auto vtbl_onCreate = onCreate_ext;

ncp_repl(0x020CF880, 8, "NOP") // Prevent the worldmap from disconnecting multiplayer

// Replace get player powerup (now unused) with pre-load code
ncp_call(0x020CEF1C, 8)
void beforeLevelLoad()
{
	DesyncGuard::storeState();
}

// Replace world load level system
ncp_asmfunc void worldLoadLevel_ASM()
{asm(R"(
ncp_jump(0x020CEF84, 8)
	LDR     R0, =_ZN4Wifi25communicatingConsoleCountE
	LDR     R0, [R0]
	CMP     R0, #1
	BEQ     .return
	LDR     R0, =_ZN3Net8localAidE
	LDR     R0, [R0]
	STR     R0, [SP,#4] // playerID
	MOV     R0, #3
	STR     R0, [SP,#8] // playerMask
	MOV     R0, #0
	STR     R0, [SP,#0xC] // character1
	MOV     R0, #1
	STR     R0, [SP,#0x10] // character2
.return:
	MOV     R0, #0xD
	BL      _ZN4Game9loadLevelEtmhhhhhhhhhhhhhhm
	B       0x020CEF88
)");}

ncp_repl(0x02006A04, "B 0x02006A1C") // Do not change powerup when loading level

// Disable options on pause menu
ncp_asmfunc void disablePauseOptions_ASM()
{asm(R"(
ncp_jump(0x020CE944, 8)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BLEQ    0x020C1F14
	B       0x020CECA4
)");}

static u32 getLocalAid() { return Net::localAid; }

// Fix top OAM powerup on worldmap
ncp_repl(0x020CF70C, 8, ".int _ZN3Net8localAidE")

// Fix player on worldmap
ncp_set_call(0x020CE2A4, 8, getLocalAid)
ncp_set_call(0x020D5C18, 8, getLocalAid)
ncp_set_call(0x020D5FC0, 8, getLocalAid)
ncp_set_call(0x020D83D4, 8, getLocalAid)
ncp_set_call(0x020D8D30, 8, getLocalAid)
ncp_set_call(0x020D8D44, 8, getLocalAid)

ncp_call(0x020D63C0, 8) u32 call_020D63C0_ov8() { return Game::getPlayerPowerup(Net::localAid); }

ncp_repl(0x020CE5C0, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020CED08, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D0608, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D063C, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D0660, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D0684, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D06A8, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D2B4C, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D3E40, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D4764, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020D6DFC, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")
ncp_repl(0x020E299C, 8, ".int _ZN9CoopFixes8Worldmap10inputOwnerE")

} // namespace CoopFixes::Worldmap
