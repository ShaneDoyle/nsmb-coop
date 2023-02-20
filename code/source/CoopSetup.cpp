#include "nsmb/game.h"
#include "nsmb/system/save.h"
#include "nsmb/entity/scene.h"
#include "nsmb/wifi.h"

ncp_repl(0x021578F0, 52, "MOVEQ R1, #0") // Force MvsLMode = 0

// Replace MvsL load function
ncp_repl(0x021535A0, 52, R"(
	MOV R0, #2
	B _ZN4Game14setPlayerCountEl
)")

ncp_call(0x02157A44, 52) void call_02157A44_ov52(SceneID sceneID, u32 sceneSettings)
{
	Game::luigiMode = Wifi::currentAid; // Enable Luigi graphics in world map for Luigi console
	Save::loadMainSave(0, 5, &Save::mainSave);
	Scene::switchScene(SceneID::Worldmap, Save::mainSave.currentWorld);
}
