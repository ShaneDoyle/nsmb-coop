#include "dsimodewarn/DSiModeScene.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/misc.hpp>

#include "fid.hpp"
#ifdef MODULE_WIDESCREEN
#include "widescreen/widescreen.hpp"
#endif

constexpr u32 topScreenImgFileID = "dsimode.enpg"fid;

ObjectProfile DSiModeScene::Profile = {
	&constructObject<DSiModeScene>,
	0, 0
};

ncp_thumb s32 DSiModeScene::onCreate()
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

#ifdef MODULE_WIDESCREEN
	bool wide = Widescreen::enabled[Net::localAid];
	G2_SetBG3Affine(&Widescreen::affineBgMtx[wide], 0, 0, Widescreen::affineBgOffX[wide], 0);
#endif

	G2_SetBG3Priority(0);
	G2_BG3Mosaic(false);

	Game::setVisiblePlane(GX_PLANEMASK_BG3);

	SND::playSFX(238);

	return 1;
}

ncp_thumb s32 DSiModeScene::onUpdate()
{
	if (!Game::fader.fadedIn())
		return 1;

	u16 pressed = Input::consoleKeys[0].pressed;

	if (pressed & Keys::A)
	{
		SND::playSFX(233);
		Scene::switchScene(SceneID::TitleScreen);
	}

	return 1;
}

ncp_thumb s32 DSiModeScene::onDestroy()
{
	Game::fader.fadingTarget[0] = 3; // Both screens
	return 1;
}
