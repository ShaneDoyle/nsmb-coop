#include "nsmb/game.hpp"
#include "nsmb/system/save.hpp"
#include "nsmb/entity/scene.hpp"
#include "nsmb/net.hpp"

ncp_repl(0x021578F0, 52, "MOVEQ R1, #0") // Force Game::vsMode = 0

// Replace MvsL load function
ncp_repl(0x021535A0, 52, R"(
	MOV R0, #2
	B _ZN4Game14setPlayerCountEl
)")

ncp_call(0x02157A44, 52) void call_02157A44_ov52(SceneID sceneID, u32 sceneSettings)
{
	Game::luigiMode = Net::localAid; // Enable Luigi graphics in world map for Luigi console
	Scene::switchScene(SceneID::Worldmap, Save::mainSave.currentWorld);
}
