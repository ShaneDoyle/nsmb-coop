#include "DSiModeScene.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/misc.hpp>

#define BASE_SHARED			0x027FF000
#define SRAMCHECKER7		BASE_SHARED + 0xA7E

static inline bool isDSiDevice()
{
	// Read from variable set in ARM7
	return *rcast<vu16*>(SRAMCHECKER7) & 0x8000;
}

static inline bool isDSCpuMode()
{
	// Return true if the instruction was not patched
	return *rcast<vu32*>(0x01FFA780) == 0xE2500004;
}

void BootScene_goToTitlescreenHook(u32 sceneID, u32 settings)
{
	if (isDSiDevice() && isDSCpuMode())
	{
		Scene::switchScene(253);
		return;
	}
	Scene::switchScene(sceneID, settings);
}

ncp_set_call(0x020CCAD0, 1, BootScene_goToTitlescreenHook)
ncp_set_call(0x020CC720, 1, BootScene_goToTitlescreenHook)

constexpr u32 topScreenImgFileID = 2091 - 131;

ObjectProfile DSiModeScene::profile = {
	&constructObject<DSiModeScene>,
	0, 0
};

s32 DSiModeScene::onCreate()
{
	Game::fader.fadingTarget[0] = 1; // Top screen only

	GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);

	GX_ResetBankForBG();
	GX_ResetBankForOBJ();
	GX_ResetBankForBGExtPltt();
	GX_ResetBankForOBJExtPltt();
	GX_ResetBankForTex();
	GX_ResetBankForTexPltt();
	GX_ResetBankForClearImage();
	GX_ResetBankForSubBG();
	GX_ResetBankForSubOBJ();
	GX_ResetBankForSubBGExtPltt();
	GX_ResetBankForSubOBJExtPltt();

	GX_SetBankForBG(GX_VRAM_BG_128_D);

	G2_SetBG3Control256Bmp(GX_BG_SCRSIZE_256BMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);

	void* bgData = FS::loadFileLZ77(topScreenImgFileID);
	MI_DmaCopy32(2, bgData, G2_GetBG3ScrPtr(), 0x10000);
	MI_DmaCopy32(3, scast<u8*>(bgData) + 0x10000, rcast<void*>(HW_BG_PLTT), 0x200);
	FS::unloadFile(bgData);

    G2_SetBG3Priority(0);
    G2_BG3Mosaic(false);

	Game::setVisiblePlane(GX_PLANEMASK_BG3);
	Game::fader.setAlphaBlending(0, 0, 0, 0);

	SND::playSFX(238);

	return 1;
}

s32 DSiModeScene::onUpdate()
{
	u16 pressed = Input::consoleKeys[0].pressed;

	if (pressed & Keys::A)
	{
		SND::playSFX(233);
		Scene::switchScene(SceneID::TitleScreen);
	}

	return 1;
}

s32 DSiModeScene::onDestroy()
{
	Game::fader.fadingTarget[0] = 3; // Both screens
	return 1;
}
