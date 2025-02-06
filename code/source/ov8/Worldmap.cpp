#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

asm(R"(
	Worldmap_onCreate = 0x020CF7C8
)");
extern "C"
{
	s32 Worldmap_onCreate(Scene* self);
}

NTR_USED static u8 WorldmapInputOwner = 0; // The ID of the console controlling the worldmap input (MUST BE 0)
static u8 WorldmapSyncWaiting = 0;

// Sync the worldmap load
static s32 Worldmap_onCreate_ext(Scene* self)
{
	if (!WorldmapSyncWaiting)
	{
		Worldmap_onCreate(self);

		if (Net::isConnected())
			Net::setMarker(0);

		WorldmapSyncWaiting = 1;
	}

	if (Net::isConnected() && !Net::checkMarker(0))
		return -1;

	Net::clearMarker(0);
	WorldmapSyncWaiting = 0;
	return 1;
}

ncp_over(0x020E67EC, 8) const auto WorldmapScene_onCreate_vtbl = Worldmap_onCreate_ext;

ncp_repl(0x020CF880, 8, "NOP") // Prevent the worldmap from disconnecting multiplayer

// Replace world load level system
asm(R"(
ncp_jump(0x020CEF84, 8)
	LDR     R0, =_ZN4Wifi12consoleCountE
	LDR     R0, [R0]
	CMP     R0, #1
	BEQ     .return
	LDR     R0, =_ZN4Wifi10currentAidE
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
)");

// Life continues
ncp_hook(0x20CF7C8, 8) void WorldmapSetupCoop() {
	if (Wifi::getCommunicatingConsoleCount() > 1) {
		for (int i = 0; i < Wifi::getCommunicatingConsoleCount(); i++) {
			if (Game::getPlayerLives(i) <= 0) { Game::setPlayerLives(i, 5); }
		}
	}
}

ncp_repl(0x02006A04, "B 0x02006A1C") // Do not change powerup when loading level

// Disable pause menu on worldmap
/*void repl_020CE944_ov_08()
{
	if (GetConsoleCount() == 1)
		asm("BL 0x20C1F14");
}*/

// Disable options on pause menu
asm(R"(
ncp_jump(0x020CE944, 8)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BLEQ    0x020C1F14
	B       0x020CECA4
)");

static u32 Worldmap_getLocalAid() { return Net::localAid; }

// Fix top OAM powerup on worldmap
ncp_repl(0x020CF70C, 8, ".int _ZN3Net8localAidE")

// Fix player on worldmap
ncp_set_call(0x020CE2A4, 8, Worldmap_getLocalAid)
ncp_set_call(0x020D5C18, 8, Worldmap_getLocalAid)
ncp_set_call(0x020D5FC0, 8, Worldmap_getLocalAid)
ncp_set_call(0x020D83D4, 8, Worldmap_getLocalAid)
ncp_set_call(0x020D8D30, 8, Worldmap_getLocalAid)
ncp_set_call(0x020D8D44, 8, Worldmap_getLocalAid)

ncp_call(0x020D63C0, 8) u32 call_020D63C0_ov8() { return Game::getPlayerPowerup(Net::localAid); }

ncp_repl(0x020CE5C0, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020CED08, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D0608, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D063C, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D0660, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D0684, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D06A8, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D2B4C, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D3E40, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D4764, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020D6DFC, 8, ".int _ZL18WorldmapInputOwner")
ncp_repl(0x020E299C, 8, ".int _ZL18WorldmapInputOwner")
