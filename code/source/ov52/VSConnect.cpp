#include "nsmb/entity/scene.h"
#include "nsmb/vsconnect/scene.h"
// The fields of the VSConnectScene class are not yet documented, raw access to fields is required.
#include "nsmb/sound.h"

ncp_call(0x021592DC, 52)
u32 VSConnect_skipFirstSubMenu()
{
	register VSConnectScene* _this asm("r4");

	((u32*)_this)[0x15C / 4] = (u32)&VSConnectScene::charSelectSM; // Sub-menu updater
	((u32*)_this)[0x160 / 4] = 1; // Sub-menu swap timer

	return 1;
}

ncp_call(0x02158800, 52)
void VSConnect_modifyReturn(VSConnectScene* _this, VSConnectScene::SubMenu* subMenu, s32 delay, bool playSound)
{
	_this->scheduleSubMenuChange(subMenu, delay, playSound); // Call replaced function

	// Variable that happens to not be 0 during connection menu
	if (((u32*)_this)[0x134 / 4])
	{
		Scene::switchScene(SceneID::TitleScreen, 0);
		Sound::stopBGM(30);
	}
}
