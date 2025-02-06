#include <nsmb/core/entity/scene.hpp>
#include <nsmb/game/vsconnect/scene.hpp>
// The fields of the VSConnectScene class are not yet documented, raw access to fields is required.
#include <nsmb/game/sound/sound.hpp>

ncp_call(0x021592DC, 52)
u32 VSConnect_skipFirstSubMenu()
{
	register VSConnectScene* self asm("r4");

	rcast<u32*>(self)[0x15C / 4] = rcast<u32>(&VSConnectScene::charSelectSM); // Sub-menu updater
	rcast<u32*>(self)[0x160 / 4] = 1; // Sub-menu swap timer

	return 1;
}

ncp_call(0x02158800, 52)
void VSConnect_modifyReturn(VSConnectScene* self, VSConnectScene::SubMenu* subMenu, s32 delay, bool playSound)
{
	self->scheduleSubMenuChange(subMenu, delay, playSound); // Call replaced function

	// Variable that happens to not be 0 during connection menu
	if (rcast<u32*>(self)[0x134 / 4])
	{
		Scene::switchScene(SceneID::TitleScreen, 0);
		SND::stopBGM(30);
	}
}
