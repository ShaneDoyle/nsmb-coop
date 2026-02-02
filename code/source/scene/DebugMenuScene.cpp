#include "DebugMenuScene.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/misc.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

#include "fid.hpp"
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

constexpr u32 fontImgID = "coop/debugfont.enpg"fid;

ObjectProfile DebugMenuScene::profile = {
	&constructObject<DebugMenuScene>,
	0, 0
};

void DebugMenuScene::clearScreen() {
	initSubGfx();
}

s32 DebugMenuScene::onCreate()
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

	switchState(&DebugMenuScene::mainState);
	return 1;
}

struct MenuEntry {
	const char *text;
	void (*callback)(void);
};

extern struct MenuEntry *sMenu;

#include <nsmb/game/vsconnect/scene.hpp>

extern void *gVsConnectSubMenu;
extern bool gForceLuigiMode;

static void main_menu() {
	Scene::switchScene(SceneID::TitleScreen);
}

static void load_file_internal(int slot) {
	Save::loadMainSave(slot, 4, &Save::mainSave);
	gVsConnectSubMenu = &VSConnectScene::searchSM;
	Scene::switchScene(SceneID::VSConnect);
}

static void load_file_slot_1() { load_file_internal(0); }
static void load_file_slot_2() { load_file_internal(1); }
static void load_file_slot_3() { load_file_internal(2); }

static void host() {
	static struct MenuEntry customMenu[4] = { 0 };
	static MainSave readSavesSlots[3];
	static char buffers[3][255];

	for (int i = 0; i < 3; ++i) {
		MainSave *slot = &readSavesSlots[i];
		Save::readMainSave(i, slot);


		if (slot->flags == 0) {
			OS_SNPrintf(buffers[i], 255, "Save slot %d - NEW", i + 1);
		} else {
			OS_SNPrintf(buffers[i], 255, "Save slot %d - World %d", i + 1, slot->currentWorld + 1);
		}
		customMenu[i].text = buffers[i];
	}
	customMenu[0].callback = &load_file_slot_1;
	customMenu[1].callback = &load_file_slot_2;
	customMenu[2].callback = &load_file_slot_3;

	sMenu = customMenu;
}

static void join() {
	gVsConnectSubMenu = &VSConnectScene::searchSM;
	gForceLuigiMode = true;
	Scene::switchScene(SceneID::VSConnect);
}

static void vsMode() {
	Scene::switchScene(SceneID::VSConnect);
}

static void worldmap() {
	Scene::switchScene(SceneID::Worldmap);
}

static struct MenuEntry sMainDebugMenu[] {
	{ "Host", &host },
	{ "Join", &join },
	{ "Title Screen", &main_menu },
	{ "VS Mode Menu", &vsMode },
	{ "Worldmap", &worldmap },
	{ nullptr, nullptr },
};

struct MenuEntry *sMenu = sMainDebugMenu;

#ifndef MAX
#define MAX(a, b)				((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)				((a) < (b) ? (a) : (b))
#endif
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))
#endif

s32 DebugMenuScene::onUpdate()
{
	int sizeOfMenu = 0;
	while (sMenu[sizeOfMenu].text != nullptr) { sizeOfMenu++; }

	static int pointer = 0;
	struct MenuEntry *currEntry = &sMenu[pointer];

	u16 pressed = Input::consoleKeys[0].pressed;
	if (pressed != 0) { clearScreen(); }
	if (pressed & Keys::A && currEntry->callback) {
		currEntry->callback();
	}
	if (pressed & Keys::B && sMenu != sMainDebugMenu) {
		sMenu = sMainDebugMenu;
		pointer = 0;
	}
	if (pressed & Keys::Up)   { pointer = MAX(pointer - 1, 0); }
	if (pressed & Keys::Down) { pointer = MIN(pointer + 1, sizeOfMenu - 1); }

	for (int i = 0; i < sizeOfMenu; ++i) {
		struct MenuEntry *entry = &sMenu[i];
		if (entry->text == nullptr) { continue; }
		OS_SPrintf(textBuffer, "%s", entry->text);
		writeString(16, i * 24, 7, textBuffer);
	}

	OS_SPrintf(textBuffer, "--");
	writeString(0, pointer * 24, 7, textBuffer);

	this->updateFunc(this);
	return 1;
}

s32 DebugMenuScene::onDestroy()
{
	return 1;
}

void DebugMenuScene::switchState(void (*updateFunc)(DebugMenuScene*))
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

void DebugMenuScene::mainState()
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
}

void DebugMenuScene::initTopGfx()
{
	GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);

	GX_SetBankForBG(GX_VRAM_BG_128_D);

	Game::setVisiblePlane(GX_PLANEMASK_BG3);
}

void DebugMenuScene::initSubGfx()
{
	GXS_SetGraphicsMode(GX_BGMODE_5);

	GX_SetBankForSubBG(GX_VRAM_SUB_BG_128_C);

	G2S_SetBG3Control256Bmp(GX_BG_SCRSIZE_256BMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);

	void* bgData = FS::loadFileLZ77(fontImgID);
	MI_DmaCopy32(2, bgData, G2S_GetBG3ScrPtr(), 0x10000);
	MI_DmaCopy32(3, scast<u8*>(bgData) + 0x10000, rcast<void*>(HW_DB_BG_PLTT), 0x200);
	FS::unloadFile(bgData);

	G2S_SetBG3Priority(0);
	G2S_BG3Mosaic(false);

	GXS_SetVisiblePlane(GX_PLANEMASK_BG3);
}
