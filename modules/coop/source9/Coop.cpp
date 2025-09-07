#include "coop/Coop.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/net.hpp>

namespace Coop {

u8 isActive;

bool getLocalPlayerID() { return Game::localPlayerID; }
bool isLuigiMode() { return Game::luigiMode; }
bool isMultiplayer() { return Game::getPlayerCount() > 1; }
bool isNotVS() { return Game::vsMode == 0; }

ncp_thumb void syncSwitchScene(u32 r0, u32 r1)
{
	if (!Net::isConnected())
		goto sceneSwitch;

	// Notify other consoles that this console has finished loading the level.
	Net::Core::setMarker(0);

	// Wait until all consoles have finished loading before switching to the stage scene.
	if (!Net::Core::checkMarker(0))
		return;

	// All consoles are ready; clear the marker and proceed to switch the scene.
	Net::Core::clearMarker(0);

sceneSwitch:
	Scene::switchScene(r0, r1);
}

// Allow everything to load in VS mode

ncp_repl(0x020093A8, "MOV R1, #0") // Avoid NARCs

// Do not load some overlays
ncp_repl(0x021535BC, 52, "NOP")
ncp_repl(0x021535C4, 52, "NOP")
ncp_repl(0x021535CC, 52, "NOP")
ncp_repl(0x021535D4, 52, "NOP")
ncp_repl(0x021535DC, 52, "NOP")

ncp_repl(0x021535EC, 52, "NOP") // Do not create ov53 heap

// Do not unload some overlays
ncp_repl(0x02152C34, 52, "NOP")
ncp_repl(0x02152C3C, 52, "NOP")
ncp_repl(0x02152C44, 52, "NOP")
ncp_repl(0x02152C4C, 52, "NOP")
ncp_repl(0x02152C54, 52, "NOP")

ncp_repl(0x02152C2C, 52, "NOP") // Do not destroy ov53 heap

ncp_repl(0x021535F0, 52, "MOV R0, #0") // Do not use ov53 heap
ncp_repl(0x02153600, 52, "MOV R0, #0") // Do not use ov53 heap

ncp_repl(0x02018B58, "NOP") // level NARC
ncp_repl(0x021535FC, 52, "NOP") // Dat_2d.narc
ncp_repl(0x0215360C, 52, "NOP") // Dat_enemy.narc

} // namespace Coop
