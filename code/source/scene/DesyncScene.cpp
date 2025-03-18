#include "DesyncScene.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/misc.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

#include "Widescreen.hpp"
#include "DesyncGuard.hpp"
#include "Save.hpp"

static void copyRegion(u32 srcX, u32 srcY, u32 dstX, u32 dstY, u32 width, u32 height)
{
	u8* bg = rcast<u8*>(G2S_GetBG3ScrPtr());

	for (u32 row = 0; row < height; row++)
	{
		MI_CpuCopy8(bg + ((srcY + row) * 256) + srcX, bg + ((dstY + row) * 256) + dstX, width);
	}
}

static void writeChar(u32 x, u32 y, const char c)
{
	u32 xOff, yOff;

	if (c == ' ')
		return;

	if (c >= 'a' && c <= 'z')
	{
		xOff = c - 'a';
		yOff = 208;
	}
	else if (c >= 'A' && c <= 'Z')
	{
		xOff = c - 'A';
		yOff = 224;
	}
	else if (c >= '0' && c <= '9')
	{
		xOff = c - '0';
		yOff = 192;
	}
	else if (c == ':')
	{
		xOff = 11;
		yOff = 192;
	}
	else if (c == '(')
	{
		xOff = 12;
		yOff = 192;
	}
	else if (c == ')')
	{
		xOff = 13;
		yOff = 192;
	}
	else
	{
		xOff = 10;
		yOff = 192;
	}

	copyRegion(xOff * 8, yOff, x, y, 8, 16);
}

static void writeString(u32 x, u32 y, u32 spacing, const char* text)
{
	u32 i = 0;
	while (text[i] != '\0')
	{
		writeChar(x + (i * spacing), y, text[i]);
		i++;
	}
}

constexpr u32 topScreenImgFileID = 2093 - 131;
constexpr u32 subScreenImgFileID = 2094 - 131;

const char* DesyncScene::levelNames[] = {
	"Mushroom House",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"A",
	"B",
	"C",
	"Ghost House",
	"Tower",
	"Castle",
	"Pipe",
	"Cannon",
	"Mushroom House",
	"Mushroom House",
	"Mushroom House",
	"Mushroom House",
	"Tower 2",
	"Final Castle",
	"Mushroom House"
};

ObjectProfile DesyncScene::profile = {
	&constructObject<DesyncScene>,
	0, 0
};

s32 DesyncScene::onCreate()
{
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

	GX_SetBGScrOffset(GX_BGSCROFFSET_0x00000);
	GX_SetBGCharOffset(GX_BGCHAROFFSET_0x00000);

	initTopGfx();
	initSubGfx();

	SND::playSFX(238);

	u32& areaNum = *rcast<u32*>(0x02085A94);

	OS_SPrintf(textBuffer, "Level: %d-%s (%d)", Game::stageGroup, levelNames[Game::currentWorldNode], areaNum);
	writeString(16, 128 - 8, 7, textBuffer);

	OS_SPrintf(textBuffer, "RNG: 0x%08X Tick: 0x%04X", Net::random.value, scast<u32>(Net::packetTick));
	writeString(16, 144 - 8, 7, textBuffer);

	char* buildTime = rcast<char*>(0x02088BB4);
	writeString(16, 160, 7, buildTime);

	switchState(&DesyncScene::mainState);
	return 1;
}

s32 DesyncScene::onUpdate()
{
	this->updateFunc(this);
	return 1;
}

s32 DesyncScene::onDestroy()
{
	return 1;
}

void DesyncScene::switchState(void (*updateFunc)(DesyncScene*))
{
	if (this->updateFunc != updateFunc)
	{
		if (this->updateFunc)
		{
			this->updateStep = Func::Exit;
			this->updateFunc(this);
		}

		this->updateFunc = updateFunc;

		this->updateStep = Func::Init;
		this->updateFunc(this);
	}
}

void DesyncScene::mainState()
{
	if (updateStep == Func::Init)
	{
		updateStep = 1;
		return;
	}
	if (updateStep == Func::Exit)
	{
		return;
	}

	u16 pressed = Input::consoleKeys[0].pressed;

	if (pressed & Keys::A)
	{
		SND::playSFX(233);
		Game::fader.prepareFadeOut();

		switchState(&DesyncScene::syncState);
	}
}

void DesyncScene::syncState()
{
	if (updateStep == Func::Init)
	{
		updateStep = 1;
		return;
	}
	if (updateStep == Func::Exit)
	{
		return;
	}

	if (updateStep == 1)
	{
		if (Game::fader.fadedOut())
			updateStep = 2;
		return;
	}

	if (updateStep == 2)
	{
		updateStep = 3;

		DesyncGuard::restoreState();

		Scene::switchScene(SceneID::Worldmap, Save::mainSave.currentWorld);

		// DesyncGuard::restoreSync(packet, 0, synchingComplete, this);
		return;
	}
}

void DesyncScene::initTopGfx()
{
	GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);

	GX_SetBankForBG(GX_VRAM_BG_128_D);

	G2_SetBG3Control256Bmp(GX_BG_SCRSIZE_256BMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);

	void* bgData = FS::loadFileLZ77(topScreenImgFileID);
	MI_DmaCopy32(2, bgData, G2_GetBG3ScrPtr(), 0x10000);
	MI_DmaCopy32(3, scast<u8*>(bgData) + 0x10000, rcast<void*>(HW_BG_PLTT), 0x200);
	FS::unloadFile(bgData);

	bool wide = Widescreen::enabled[Net::localAid];
	G2_SetBG3Affine(&Widescreen::affineBgMtx[wide], 0, 0, Widescreen::affineBgOffX[wide], 0);

	G2_SetBG3Priority(0);
	G2_BG3Mosaic(false);

	Game::setVisiblePlane(GX_PLANEMASK_BG3);
}

void DesyncScene::initSubGfx()
{
	GXS_SetGraphicsMode(GX_BGMODE_5);

	GX_SetBankForSubBG(GX_VRAM_SUB_BG_128_C);

	G2S_SetBG3Control256Bmp(GX_BG_SCRSIZE_256BMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);

	void* bgData = FS::loadFileLZ77(subScreenImgFileID);
	MI_DmaCopy32(2, bgData, G2S_GetBG3ScrPtr(), 0x10000);
	MI_DmaCopy32(3, scast<u8*>(bgData) + 0x10000, rcast<void*>(HW_DB_BG_PLTT), 0x200);
	FS::unloadFile(bgData);

	G2S_SetBG3Priority(0);
	G2S_BG3Mosaic(false);

	GXS_SetVisiblePlane(GX_PLANEMASK_BG3);
}

/*
void DesyncScene::synchingComplete(u16 aid, void* arg)
{
	// DesyncScene* self = rcast<DesyncScene*>(arg);

	SaveExt::reloadMainSave();

	Scene::switchScene(SceneID::Worldmap, Save::mainSave.currentWorld);
}
*/
