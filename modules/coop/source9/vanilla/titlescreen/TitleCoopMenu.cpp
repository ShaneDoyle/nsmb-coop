#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/game/ui.hpp>
#include <nsmb/game/vsconnect/common.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/filesystem/file.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/graphics/2d/font.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

#include <new>

#include "fid.hpp"
#include "coop/Coop.hpp"
#include "coop/CoopSave.hpp"
#include "coop/Packet.hpp"

#ifdef MODULE_WIDESCREEN
#include "widescreen/widescreen.hpp"
#endif

/*
	The second title screen button opens this menu instead of the VS Connect scene.

	Both consoles connect from here using the VS Connect graphics and messages.
	Once linked, only Mario's console opens the file select. The file Mario picks
	is then sent to Luigi with Packet, and both consoles switch to the world map
	together.

	The title screen scene is not documented in the code reference, so it is
	accessed through raw offsets, taken from the NSMB decompilation.
*/

extern "C" {
	void CoopSym_TitleScreen_loadFileSelectGraphics(void* self);
	void CoopSym_TitleScreen_loadMainMenuGraphics(void* self);
	void CoopSym_CommonMenu_loadSubButtonGraphics();
	void CoopSym_TextLabel_loadScript(void* self, u32 scriptID);
	void CoopSym_TextLabel_unloadScript(void* self);
	void CoopSym_TextLabel_render(void* self);
	void CoopSym_TextLabel_setMessage(void* self, u32* messageID);
	void CoopSym_TextLabel_loadFireballIcon(void* self);
	void CoopSym_TextLabel_updateFireballIcon(void* self);
	void CoopSym_TextLabel_hideFireballIcon(void* self);
	void CoopSym_Fader_enableMainScreenFading(Fader* self);
	void CoopSym_Fader_startSceneFade(Fader* self);
	void CoopSym_Wifi_loadIconPalette(u32 objIndex, u32 palette, u32 subScreen);

	extern NicknameCache CoopSym_nicknameCache;
	extern u8 CoopSym_vsConsoleCharacters[2];
	extern u8 CoopSym_vsConnectCancelled;
	extern u8 CoopSym_touchX;
	extern u8 CoopSym_touchValid;
	extern u8 CoopSym_touchPressed;
	extern u8 CoopSym_touchY;
}

namespace Net::Core {
	BOOL startParentBroadcast();
}

namespace Coop::Vanilla::TitleCoopMenu {

// TitleScreenScene field offsets
constexpr u32 MenuIDOffset = 0x64;
constexpr u32 StateIDOffset = 0x65;
constexpr u32 FileSelectButtonOffset = 0x68;
constexpr u32 LabelOffset = 0xAB4;

constexpr u8 MenuMain = 0;
constexpr u8 MenuFileSelect = 1;
constexpr u8 StateInitialize = 0;

// TextLabel scripts
constexpr u32 TitleScript = 1;
constexpr u32 VSConnectScript = 4;

// VS Connect messages (script 4)
namespace Message {
	constexpr u32 ChooseCharacter = 2;
	constexpr u32 SearchingAsHost = 3;
	constexpr u32 SearchingAsGuest = 4;
	constexpr u32 ConfirmAsHost = 5;
	constexpr u32 ConfirmAsGuest = 6;
	constexpr u32 PlayerLeftHost = 7;
	constexpr u32 PlayerLeftGuest = 8;
	constexpr u32 PleaseWait = 9;
}

// Buttons of UI_O_2P_game_boot_select02 (BNCL cell, OAM attributes)
namespace Cell {
	constexpr u8 Back = 0;
	constexpr u8 Mario = 1;
	constexpr u8 Luigi = 2;
	constexpr u8 Yes = 3;
	constexpr u8 No = 4;
}

// Boxes of UI_O_2P_game_boot_select02 (BNBL)
namespace Box {
	constexpr s16 First = 0; // Mario / Yes
	constexpr s16 Second = 1; // Luigi / No
	constexpr s16 Back = 2;
}

// The wireless icon of the current scene is drawn from these OBJ tiles.
// 0x9C follows the VS Connect graphics, 0x13A follows the file select and common menu graphics.
constexpr u32 PageWifiIconOBJIndex = 0x9C;
constexpr u32 FileSelectWifiIconOBJIndex = 0x13A;
constexpr u32 WifiIconOBJPalette = 0xF;

constexpr u32 SettingsSize = 4;

// Co-op connects under its own game group ID, so a console in VS mode never finds
// a co-op console and the other way around (WH ignores beacons of other groups).
// Flipping a bit of the region's ID keeps the regions apart, like the vanilla IDs.
constexpr u32 CoopGgidFlag = 0x00010000;

constexpr u32 LinkTimeout = 900;
constexpr u32 SettingsTimeout = 0x1518;

// Same slide-in as the file select (TitleScreenScene::enterFileSelect)
constexpr s32 SlideFrames = 16;
constexpr s32 SlideButtonDelay = 2;

constexpr u32 SaveTransferMarker = 1;
constexpr u32 LaunchMarker = 2;

// The VS Connect message window, drawn on the top screen with its text.
// VS Connect draws it with OBJ palettes 0 to 2 (window 0, text and fireball 2),
// which are the title logo's here, so they get their own VRAM area and palette slots.
constexpr u32 WindowVramOffset = 0x10000; // After the logo, before the text label tiles
constexpr u32 WindowPalette = 12; // First of the 3 VS Connect palettes
constexpr s32 WindowX = 0x80;
constexpr s32 WindowY = 0x60;
constexpr s32 WindowSlideDistance = 160; // The window reaches 56 pixels above its center

// Copied from overlay 11 (VSConnectScene message window), tile numbers relative to the window graphics
alignas(4) static const u16 windowOamAttrsBase[][4] = {
	{0x0000, 0x9050, 0x0001, 0x0000}, {0x00E0, 0x9050, 0x0001, 0x0000}, {0x0018, 0xB050, 0x0005, 0x0000}, {0x0018, 0xA030, 0x0009, 0x0000},
	{0x0000, 0x8030, 0x000D, 0x0000}, {0x00E0, 0x8030, 0x000D, 0x0000}, {0x00C8, 0x8030, 0x0009, 0x0000}, {0x0018, 0xA010, 0x0009, 0x0000},
	{0x0000, 0x8010, 0x000D, 0x0000}, {0x00E0, 0x8010, 0x000D, 0x0000}, {0x00C8, 0x8010, 0x0009, 0x0000}, {0x00C8, 0x9050, 0x0005, 0x0000},
	{0x0018, 0xA1F0, 0x0009, 0x0000}, {0x0000, 0x81F0, 0x000D, 0x0000}, {0x00E0, 0x81F0, 0x000D, 0x0000}, {0x00C8, 0x81F0, 0x0009, 0x0000},
	{0x0018, 0xA1D0, 0x0009, 0x0000}, {0x0000, 0x81D0, 0x000D, 0x0000}, {0x00E0, 0x81D0, 0x000D, 0x0000}, {0x00C8, 0x81D0, 0x0009, 0x0000},
	{0x0018, 0xA1B0, 0x0009, 0x0000}, {0x0000, 0x81B0, 0x000D, 0x0000}, {0x00E0, 0x81B0, 0x000D, 0x0000}, {0x00C8, 0x81B0, 0x0009, 0x0000},
	{0x0000, 0x8190, 0x0001, 0x0000}, {0x00E0, 0x8190, 0x0001, 0x0000}, {0x0018, 0xA190, 0x0005, 0x0000}, {0x00C8, 0x8190, 0x0005, 0xFFFF}
};
constexpr u32 WindowOamCount = sizeof(windowOamAttrsBase) / sizeof(windowOamAttrsBase[0]);
alignas(4) static u16 windowOamAttrs[WindowOamCount][4];

// Copied from overlay 52 (VSConnectScene::buttonOamAttrs) so the overlay does not have to be loaded
alignas(4) static const u16 backOamAttrs[][4] = {
	{0x00F0, 0x81F0, 0x0000, 0xFFFF}
};
alignas(4) static const u16 marioOamAttrs[][4] = {
	{0x40F8, 0x81D4, 0x0010, 0x0000}, {0x40F8, 0x81F4, 0x0018, 0x0000}, {0x00F8, 0x4014, 0x0020, 0x0000},
	{0x40F9, 0x81D5, 0x0010, 0x0000}, {0x40F9, 0x81F5, 0x0018, 0x0000}, {0x00F9, 0x4015, 0x0020, 0x0000},
	{0x00F0, 0x8014, 0x0024, 0x0000}, {0x00F0, 0x81FC, 0x0034, 0x0000}, {0x00F0, 0x81E4, 0x0034, 0x0000},
	{0x00F0, 0x81CC, 0x0044, 0xFFFF}
};
alignas(4) static const u16 luigiOamAttrs[][4] = {
	{0x40F8, 0x81D4, 0x0054, 0x0000}, {0x40F8, 0x81F4, 0x005C, 0x0000}, {0x00F8, 0x4014, 0x0064, 0x0000},
	{0x40F9, 0x81D5, 0x0054, 0x0000}, {0x40F9, 0x81F5, 0x005C, 0x0000}, {0x00F9, 0x4015, 0x0064, 0x0000},
	{0x00F0, 0x8014, 0x0024, 0x0000}, {0x00F0, 0x81FC, 0x0034, 0x0000}, {0x00F0, 0x81E4, 0x0034, 0x0000},
	{0x00F0, 0x81CC, 0x0044, 0xFFFF}
};
alignas(4) static const u16 yesOamAttrs[][4] = {
	{0x40F8, 0x81E4, 0x0068, 0x0000}, {0x00F8, 0x4004, 0x0070, 0x0000},
	{0x40F9, 0x81E5, 0x0068, 0x0000}, {0x00F9, 0x4005, 0x0070, 0x0000},
	{0x00F0, 0x8014, 0x0024, 0x0000}, {0x00F0, 0x81FC, 0x0034, 0x0000}, {0x00F0, 0x81E4, 0x0034, 0x0000},
	{0x00F0, 0x81CC, 0x0044, 0xFFFF}
};
alignas(4) static const u16 noOamAttrs[][4] = {
	{0x40F8, 0x81E4, 0x0074, 0x0000}, {0x00F8, 0x4004, 0x007C, 0x0000},
	{0x40F9, 0x81E5, 0x0074, 0x0000}, {0x00F9, 0x4005, 0x007C, 0x0000},
	{0x00F0, 0x8014, 0x0024, 0x0000}, {0x00F0, 0x81FC, 0x0034, 0x0000}, {0x00F0, 0x81E4, 0x0034, 0x0000},
	{0x00F0, 0x81CC, 0x0044, 0xFFFF}
};

static const u16* const buttonOamAttrs[] = {
	backOamAttrs[0], marioOamAttrs[0], luigiOamAttrs[0], yesOamAttrs[0], noOamAttrs[0]
};

enum class Step : u8
{
	Closed,
	CharSelect,
	Search,
	Confirm,
	WaitHost,
	PlayerLeft,
	Link,
	HostFileSelect, // Mario is in the vanilla file select, Luigi waits in SyncSave
	SyncSave,
	Launched,
	Closing // Inverse of the opening slide, then Closed
};

struct Button
{
	u8 cell;
	u8 palette;
	s8 animFrame; // -1 when not animating
	s8 offset;

	void create(u8 cell)
	{
		this->cell = cell;
		palette = 0;
		animFrame = -1;
		offset = 0;
	}

	void click()
	{
		palette = 1;
		animFrame = 0;
	}

	// Same "slight move" animation as VSConnectScene::Button
	void update()
	{
		if (animFrame < 0)
			return;

		animFrame++;
		switch (animFrame)
		{
		case 1:
		case 3:
			offset++;
			break;
		case 5:
			offset--;
			break;
		case 7:
			offset--;
			animFrame = -1;
			break;
		}
	}

	void render(s32 slideX) const
	{
		UI::drawSub(
			cell, rcast<GXOamAttr*>(ccast<u16*>(buttonOamAttrs[cell])), OAM::Flags::None, palette,
			0, nullptr, 0, nullptr, OAM::Settings::None, offset + slideX, offset
		);
	}
};

// Sent from Mario's console to Luigi's (Packet requires a 4-byte aligned size)
struct BootPacket
{
	MainSave save;
	u32 slot;
};

struct State
{
	Step step;
	Step scheduledStep;
	s32 changeDelay;
	u8 subState;
	s32 timer;

	bool isHost;
	u8 selected;
	u8 buttonCount;
	Button buttons[3];
	s32 slideTimer;
	bool slideLabel;
	s32 windowOffsetY;

	u8 parentAid;
	u8 parentBssid[WM_SIZE_BSSID];
	bool hasParentBssid;
	s32 parentBssidTimer;

	bool visualsActive;
	u32 savedWifiIconOBJIndex;
	u32 savedWifiIconOBJPalette;
	bool penWasDown;

	bool packetBufferAlive;
	alignas(Net::PacketBuffer) u8 packetBufferStorage[sizeof(Net::PacketBuffer)];
	u8 settings[SettingsSize];
	u8 receivedSettings[Coop::MaxPlayerCount][SettingsSize];
	u8 syncedAidMask;

	Packet packet;
	BootPacket bootPacket;
	bool saveTransferred;
};

static State state;

static Scene* activeScene;

NTR_INLINE u8& field8(void* scene, u32 offset) { return *(rcast<u8*>(scene) + offset); }
NTR_INLINE void* getLabel() { return rcast<u8*>(activeScene) + LabelOffset; }
NTR_INLINE Net::PacketBuffer* getPacketBuffer() { return rcast<Net::PacketBuffer*>(state.packetBufferStorage); }
NTR_INLINE vu32& subDisplayControl() { return *rcast<vu32*>(0x04001000); }
NTR_INLINE vu32& mainDisplayControl() { return *rcast<vu32*>(0x04000000); }
NTR_INLINE u32* controlModes() { return rcast<u32*>(0x02085AD4); } // Game::playerControlModes

static void setMessage(u32 messageID)
{
	CoopSym_TextLabel_setMessage(getLabel(), &messageID);
}

static void setButtons(u8 count, u8 cell0, u8 cell1, u8 cell2)
{
	state.buttonCount = count;
	state.buttons[0].create(cell0);
	state.buttons[1].create(cell1);
	state.buttons[2].create(cell2);
	state.slideTimer = count != 0 ? SlideFrames : 0;
}

// The buttons come in from the right one after the other, the first one leading
static s32 getButtonSlideX(u32 button)
{
	s32 delay = (state.buttonCount - 1 - button) * SlideButtonDelay;
	s32 x = ((state.slideTimer - delay) * 320) / 12;
	return x < 0 ? 0 : x;
}

// The buttons leave to the right in the same order, like TitleScreenScene::exitFileSelectToMainMenu
static s32 getButtonSlideOutX(u32 button)
{
	s32 x = ((state.timer - s32(button) * SlideButtonDelay) * 320) / 12;
	return x < 0 ? 0 : x;
}

// The message window and its text come down from above the screen
static void updateLabelSlide()
{
	if (!state.slideLabel)
		return;

	state.windowOffsetY = -((state.slideTimer * WindowSlideDistance) / 12);
	if (state.slideTimer == 0)
		state.slideLabel = false;
}

// Same palettes as VSConnectScene::selectButton
static void highlightSelected()
{
	state.buttons[state.selected].palette = 0;
	state.buttons[state.selected ^ 1].palette = 2;
}

enum class Sound : u8 { None, Forward, Back };

// Same sounds as VSConnectScene::scheduleSubMenuChange
static void schedule(Step step, s32 delay, Sound sound)
{
	state.scheduledStep = step;
	state.changeDelay = delay;
	if (sound != Sound::None)
		SND::playSFX(sound == Sound::Back ? 234 : 233);
}

// Returns the touched box, -1 if none
static s16 getTouchedBox()
{
	if (CoopSym_touchPressed == 0 || CoopSym_touchValid == 0)
		return -1;
	return UI::bnbl->getBox(CoopSym_touchX, CoopSym_touchY);
}

static u16 getPressedKeys()
{
	return Input::consoleKeys[Input::localConsoleID].pressed;
}

// ---------------- Networking ----------------

static void onSettingsReceived(u16 aid, void* arg)
{
	const u8* settings = getPacketBuffer()->buffers[aid];

	// Applied at launch, the title screen still uses index 0 as the local console's settings
	for (u32 i = 0; i < SettingsSize; i++)
		state.receivedSettings[aid][i] = settings[i];

	state.syncedAidMask |= 1 << aid;
}

static void onBootPacketTransferred(u16 aid, void* arg)
{
	state.saveTransferred = true;
}

static void createPacketBuffer()
{
	if (state.packetBufferAlive)
		return;

	// Created before connecting, like VS Connect, so both consoles share the sequencer ID
	new (state.packetBufferStorage) Net::PacketBuffer();
	getPacketBuffer()->create(SettingsSize, onSettingsReceived, nullptr);
	state.packetBufferAlive = true;
	state.syncedAidMask = 0;
}

static void destroyPacketBuffer()
{
	if (!state.packetBufferAlive)
		return;

	getPacketBuffer()->~PacketBuffer();
	state.packetBufferAlive = false;
}

static void shutdownNet()
{
	destroyPacketBuffer();
	Net::stopConnection();
	Net::shutdownWireless();
	FS::setReadModeFast();
}

// Mirrors VSConnectScene::getOpponentNickname
static const NicknameInfo* getOpponentNickname()
{
	if (!state.isHost)
	{
		for (u32 i = 0; i < 4; i++)
		{
			const MBPChildInfo* info = Net::getParentGameInfo(i);
			if (!info)
				continue;

			const u8* bssid = Net::getParentBssid(i);

			bool sameParent = state.hasParentBssid;
			for (u32 j = 0; sameParent && j < WM_SIZE_BSSID; j++)
			{
				if (bssid[j] != state.parentBssid[j])
					sameParent = false;
			}
			if (sameParent)
				continue;

			state.parentAid = i;
			for (u32 j = 0; j < WM_SIZE_BSSID; j++)
				state.parentBssid[j] = bssid[j];
			state.hasParentBssid = true;
			state.parentBssidTimer = 600;
			return rcast<const NicknameInfo*>(info);
		}
	}
	else if (Net::connectedConsoleCount == 2)
	{
		for (u32 aid = 1; aid < 4; aid++)
		{
			const MBPChildInfo* info = Net::getConsoleGameInfo(aid);
			if (info)
			{
				return rcast<const NicknameInfo*>(info);
			}
		}
	}
	return nullptr;
}

// ---------------- Visuals ----------------

static void loadWindow()
{
	FS::loadFileLZ77("uiStudio/UI_O_menu_common_window_o_u_ncg.bin"fid, rcast<void*>(HW_OBJ_VRAM + WindowVramOffset));

	FS::loadFileLZ77("uiStudio/d_2d_UI_O_2P_game_boot_o_ud_ncl.bin"fid, rcast<void*>(HW_OBJ_PLTT + WindowPalette * 0x20));

	// Tile numbers are in units of the 1D OBJ mapping boundary
	u32 boundaryShift = 5 + ((mainDisplayControl() >> 20) & 3);
	u16 baseTile = WindowVramOffset >> boundaryShift;
	for (u32 i = 0; i < WindowOamCount; i++)
	{
		for (u32 j = 0; j < 4; j++)
			windowOamAttrs[i][j] = windowOamAttrsBase[i][j];
		windowOamAttrs[i][2] += baseTile;
	}
}

// Text and fireball use the VS Connect palettes, move them to their new slots
static void renderLabel()
{
	u16 headBefore = OAM::objsFromHead;
	u16 tailBefore = OAM::objsFromTail;

	CoopSym_TextLabel_render(getLabel());

	// Also moves them with the window
	auto adjust = [](u32 index) {
		u16* attrs = rcast<u16*>(&OAM::objBuffer2[index]);
		attrs[2] += WindowPalette << 12;

		s32 y = attrs[0] & 0xFF;
		if (y >= 192)
			y -= 256;
		y += state.windowOffsetY;
		if (y <= -64 || y >= 192)
			attrs[0] = (attrs[0] & ~0x03FF) | 0x0200 | 192; // Hidden
		else
			attrs[0] = (attrs[0] & ~0x00FF) | (y & 0xFF);
	};
	for (u32 i = headBefore; i < OAM::objsFromHead; i++)
		adjust(i);
	for (u32 i = OAM::objsFromTail + 1; i <= tailBefore; i++)
		adjust(i);

	// Behind the text, objects drawn later have lower priority
	OAM::draw(
		rcast<const GXOamAttr*>(windowOamAttrs), WindowX, WindowY + state.windowOffsetY,
		OAM::Flags::None, WindowPalette, 0, OAM::Settings::None
	);
}

// Same sub screen OBJ setup as VSConnectScene::onCreate. The title screen background is kept.
static void loadVisuals()
{
	if (state.visualsActive)
		return;
	state.visualsActive = true;

	MI_CpuFillFast(rcast<void*>(HW_DB_OAM), 0xC0, HW_DB_OAM_SIZE);
	subDisplayControl() = (subDisplayControl() & 0xFFCFFFEF) | 0x10; // 1D OBJ mapping, 32 KB
	FS::loadFileLZ77("uiStudio/UI_O_2P_game_boot_o_d_ncg.bin"fid, rcast<void*>(HW_DB_OBJ_VRAM));
	CoopSym_CommonMenu_loadSubButtonGraphics();
	FS::loadFileLZ77("uiStudio/d_2d_UI_O_2P_game_boot_o_ud_ncl.bin"fid, rcast<void*>(HW_DB_OBJ_PLTT));

	BNCL* bncl = scast<BNCL*>(FS::Cache::loadFile("uiStudio/UI_O_2P_game_boot_select02_d.bncl"fid, false));
	BNBL* bnbl = scast<BNBL*>(FS::Cache::loadFile("uiStudio/UI_O_2P_game_boot_select02_d.bnbl"fid, false));
	UI::initSub(nullptr, bncl, bnbl);

	void* label = getLabel();
	CoopSym_TextLabel_unloadScript(label);
	CoopSym_TextLabel_loadScript(label, VSConnectScript);

	activeScene->wifiIconOBJIndex = PageWifiIconOBJIndex;
	activeScene->wifiIconOBJPalette = WifiIconOBJPalette;

	loadWindow();
}

// The title screen reloads its own OBJ graphics and layout when entering a menu
static void unloadVisuals()
{
	if (!state.visualsActive)
		return;
	state.visualsActive = false;

	void* label = getLabel();
	CoopSym_TextLabel_hideFireballIcon(label);
	CoopSym_TextLabel_unloadScript(label);
	CoopSym_TextLabel_loadScript(label, TitleScript);
}

// ---------------- Steps ----------------

static void changeStep(Step step);

static void close()
{
	shutdownNet();
	unloadVisuals();

	activeScene->wifiIconOBJIndex = state.savedWifiIconOBJIndex;
	activeScene->wifiIconOBJPalette = state.savedWifiIconOBJPalette;

	state.step = Step::Closed;
	Coop::isActive = false;
}

static void closeToMainMenu()
{
	close();
	field8(activeScene, StateIDOffset) = StateInitialize; // Reloads the main menu graphics
}

static void enterHostFileSelect()
{
	unloadVisuals();

	field8(activeScene, MenuIDOffset) = MenuFileSelect;
	field8(activeScene, StateIDOffset) = StateInitialize;
	field8(activeScene, FileSelectButtonOffset) = 0;
	CoopSym_TitleScreen_loadFileSelectGraphics(activeScene);

	// Copy and Erase are moved 16 pixels right to make room for the wireless icon
	BNCL* bncl = scast<BNCL*>(FS::Cache::loadFile("coop/UI_O_menu_select_coop_d.bncl"fid, false));
	BNBL* bnbl = scast<BNBL*>(FS::Cache::loadFile("coop/UI_O_menu_select_coop_d.bnbl"fid, false));
	UI::initSub(nullptr, bncl, bnbl);

	activeScene->wifiIconOBJIndex = FileSelectWifiIconOBJIndex;
	activeScene->wifiIconOBJPalette = WifiIconOBJPalette;
}

static void launch()
{
	// Each player's settings, indexed by player like the game expects in co-op
	for (u32 aid = 0; aid < Coop::MaxPlayerCount; aid++)
	{
		controlModes()[aid] = state.receivedSettings[aid][0];
#ifdef MODULE_WIDESCREEN
		Widescreen::enabled[aid] = state.receivedSettings[aid][1];
#endif
	}

	Game::vsMode = 0;
	Game::luigiMode = Net::localAid;
	CoopSym_vsConsoleCharacters[0] = 0;
	CoopSym_vsConsoleCharacters[1] = 1;
	Coop::isActive = true;

	destroyPacketBuffer();
	FS::setReadModeFast();

	Scene::switchScene(Save::getWorldmapSceneID(), Save::mainSave.currentWorld);
	CoopSym_Fader_enableMainScreenFading(&Game::fader);
	CoopSym_Fader_startSceneFade(&Game::fader);
}

static void changeStep(Step step)
{
	state.step = step;
	state.subState = 0;
	state.timer = 0;
	state.changeDelay = 0;
	state.slideTimer = 0;

	void* label = getLabel();
	CoopSym_TextLabel_hideFireballIcon(label);

	switch (step)
	{
	case Step::Closed:
		closeToMainMenu();
		return;

	case Step::CharSelect:
		Net::initializeWireless();
		setMessage(Message::ChooseCharacter);
		setButtons(3, Cell::Mario, Cell::Luigi, Cell::Back);
		state.selected = state.isHost ? 0 : 1;
		highlightSelected();
		return;

	case Step::Search:
		setMessage(state.isHost ? Message::SearchingAsHost : Message::SearchingAsGuest);
		setButtons(1, Cell::Back, 0, 0);
		return;

	case Step::Confirm:
		setMessage(state.isHost ? Message::ConfirmAsHost : Message::ConfirmAsGuest);
		setButtons(2, Cell::Yes, Cell::No, 0);
		state.selected = 0;
		highlightSelected();
		return;

	case Step::WaitHost:
		setMessage(Message::PleaseWait);
		CoopSym_TextLabel_loadFireballIcon(label);
		setButtons(1, Cell::Back, 0, 0);
		return;

	case Step::PlayerLeft:
		setMessage(state.isHost ? Message::PlayerLeftHost : Message::PlayerLeftGuest);
		setButtons(0, 0, 0, 0);
		state.timer = 120;
		return;

	case Step::Link:
		setMessage(Message::PleaseWait);
		CoopSym_TextLabel_loadFireballIcon(label);
		setButtons(0, 0, 0, 0);
		return;

	case Step::HostFileSelect:
		enterHostFileSelect();
		return;

	case Step::SyncSave:
		// Luigi keeps the "please wait" message from the link step
		if (!state.isHost)
			CoopSym_TextLabel_loadFireballIcon(label);
		Net::setMarker(SaveTransferMarker);
		state.saveTransferred = false;
		return;

	case Step::Launched:
		launch();
		return;

	case Step::Closing:
		// Keeps the current buttons and message to slide them out
		return;
	}
}

// Shared by the two-button steps, returns the chosen button or -1
static s32 updateTwoButtonChoice(u16 pressed, s16 touchedBox)
{
	if (pressed & Keys::A)
		return state.selected;
	if (touchedBox == Box::First || touchedBox == Box::Second)
		return touchedBox;

	u8 previous = state.selected;
	if (state.selected == 0 && (pressed & Keys::Down))
		state.selected = 1;
	else if (state.selected == 1 && (pressed & Keys::Up))
		state.selected = 0;

	if (state.selected != previous)
	{
		highlightSelected();
		SND::playSFX(229);
	}

	return -1;
}

static void updateCharSelect(u16 pressed, s16 touchedBox)
{
	if ((pressed & Keys::B) || touchedBox == Box::Back)
	{
		state.buttons[2].click();
		schedule(Step::Closing, 8, Sound::Back);
		return;
	}

	s32 choice = updateTwoButtonChoice(pressed, touchedBox);
	if (choice < 0)
		return;

	state.selected = choice;
	state.isHost = choice == 0;
	state.buttons[choice].click();
	schedule(Step::Search, 30, Sound::Forward);
}

static void updateSearch(u16 pressed, s16 touchedBox)
{
	CoopSym_TextLabel_updateFireballIcon(getLabel());

	if (state.parentBssidTimer > 0)
		state.parentBssidTimer--;
	else
		state.hasParentBssid = false;

	if ((pressed & Keys::B) || touchedBox == Box::Back)
	{
		state.buttons[0].click();
		schedule(Step::CharSelect, 30, Sound::Back);
		state.hasParentBssid = false;
		state.parentBssidTimer = 0;
		Net::stopConnection();
		CoopSym_TextLabel_hideFireballIcon(getLabel());
		return;
	}

	if (state.subState == 0)
	{
		// Wait for the previous connection to stop
		Net::ConnectionState cs = Net::connectionState;
		if (cs != Net::ConnectionState::Stopped && cs != Net::ConnectionState::Error && cs != Net::ConnectionState::Unused)
			return;

		// Same as Net::startParentBroadcast and Net::startChildScan, which would set the vanilla ID
		Net::ggid = Net::getLocalizedGgid() ^ CoopGgidFlag;
		bool started = state.isHost ? Net::Core::startParentBroadcast() : Net::Core::startChildScan();
		if (!started)
		{
			schedule(Step::CharSelect, 1, Sound::Back);
			return;
		}

		state.subState = 1;
		CoopSym_TextLabel_loadFireballIcon(getLabel());
		return;
	}

	// Do not offer the same console again right after declining it
	if (state.parentBssidTimer >= 540)
		return;

	const NicknameInfo* nickname = getOpponentNickname();
	if (!nickname)
		return;

	CoopSym_nicknameCache.stringifyOtherName(ccast<u16*>(nickname->chars), nickname->length);
	schedule(Step::Confirm, 1, Sound::Forward);
	CoopSym_TextLabel_hideFireballIcon(getLabel());
}

static void updateConfirm(u16 pressed, s16 touchedBox)
{
	if (state.subState == 1)
	{
		Net::stopConnection();
		schedule(Step::Search, 30, Sound::None);
		state.subState = 2;
		return;
	}
	if (state.subState == 2)
		return;

	// The guest left while the host was choosing
	if (Wifi::isParent() && Net::connectedConsoleCount < 2)
	{
		Net::stopConnection();
		schedule(Step::PlayerLeft, 30, Sound::None);
		state.subState = 2;
		return;
	}

	s32 choice = (pressed & Keys::B) ? 1 : updateTwoButtonChoice(pressed, touchedBox);
	if (choice < 0)
		return;

	state.selected = choice;
	state.buttons[choice].click();
	state.buttons[choice ^ 1].palette = 2;

	if (choice == 1)
	{
		SND::playSFX(233);
		state.subState = 1;
		return;
	}

	CoopSym_vsConnectCancelled = false;

	if (state.isHost)
	{
		Net::acceptChildren();
		schedule(Step::Link, 30, Sound::Forward);
	}
	else
	{
		schedule(Step::WaitHost, 30, Sound::Forward);
	}
}

static void updateWaitHost(u16 pressed, s16 touchedBox)
{
	CoopSym_TextLabel_updateFireballIcon(getLabel());

	if ((pressed & Keys::B) || touchedBox == Box::Back)
	{
		state.buttons[0].click();
		schedule(Step::Search, 30, Sound::Back);
		Net::stopConnection();
		CoopSym_vsConnectCancelled = true;
		return;
	}

	if (state.subState == 0)
	{
		if (Net::connectToParent(state.parentAid))
		{
			state.subState = 1;
			return;
		}
	}
	else
	{
		if (Net::packetTransIntegrity == Net::PacketTransferIntegrity::PacketOrderIntegrity)
		{
			schedule(Step::Link, 30, Sound::Forward);
			return;
		}
		if ((Net::errorState & Net::ErrorState::SilentError) == Net::ErrorState(0))
			return;
	}

	Net::stopConnection();
	schedule(Step::PlayerLeft, 1, Sound::Forward);
	CoopSym_vsConnectCancelled = true;
}

static void updateClosing()
{
	state.timer++;

	s32 offset = -((state.timer * WindowSlideDistance) / 12);
	state.windowOffsetY = offset < -WindowSlideDistance ? -WindowSlideDistance : offset;

	if (state.timer >= SlideFrames)
		changeStep(Step::Closed);
}

static void updatePlayerLeft()
{
	if (--state.timer > 0)
		return;
	schedule(Step::Search, 30, Sound::Back);
}

// Mirrors VSConnectScene::updateLoadGameSM, without loading the VS mode files
static void updateLink()
{
	CoopSym_TextLabel_updateFireballIcon(getLabel());

	switch (state.subState)
	{
	case 0:
		if (Net::packetTransIntegrity != Net::PacketTransferIntegrity::PacketOrderIntegrity)
		{
			if (++state.timer >= LinkTimeout)
			{
				Net::signalTimeout();
			}
			return;
		}

		if (Net::connectedConsoleCount != 2)
		{
			Net::signalTimeout();
			return;
		}

		if (Wifi::isParent())
			Wifi::disableChildEntry();

		state.settings[0] = controlModes()[0];
#ifdef MODULE_WIDESCREEN
		state.settings[1] = Widescreen::enabled[0];
#endif
		state.subState = 1;
		state.timer = 0;
		return;

	case 1:
		if (getPacketBuffer()->send(state.settings))
		{
			state.subState = 2;
		}
		break;

	case 2:
		if (state.syncedAidMask == 0b11)
		{
			changeStep(state.isHost ? Step::HostFileSelect : Step::SyncSave);
			return;
		}
		break;
	}

	if (++state.timer >= SettingsTimeout)
	{
		Net::signalTimeout();
	}
}

static void updateSyncSave()
{
	if (!state.isHost)
		CoopSym_TextLabel_updateFireballIcon(getLabel());

	switch (state.subState)
	{
	case 0:
		// Both consoles see the marker on the same frame, so they create the packet sequencers together
		if (!Net::checkMarker(SaveTransferMarker))
			return;
		Net::clearMarker(SaveTransferMarker);

		state.packet.transfer(0, &state.bootPacket, &state.bootPacket, sizeof(BootPacket), onBootPacketTransferred);
		state.subState = 1;
		return;

	case 1:
		if (!state.saveTransferred)
			return;

		Save::optionSave.activeSlot = state.bootPacket.slot;
		if (!state.isHost)
		{
			MI_CpuCopyFast(&state.bootPacket.save, &Save::mainSave, sizeof(MainSave));
			CoopSave::reloadMainSave();
		}

		Net::setMarker(LaunchMarker);
		state.subState = 2;
		return;

	case 2:
		if (!Net::checkMarker(LaunchMarker))
			return;
		Net::clearMarker(LaunchMarker);

		Net::syncRandomFast();
		changeStep(Step::Launched);
		return;
	}
}

// ---------------- Title screen interface ----------------

void open(Scene* scene)
{
	activeScene = scene;

	state.savedWifiIconOBJIndex = scene->wifiIconOBJIndex;
	state.savedWifiIconOBJPalette = scene->wifiIconOBJPalette;
	state.hasParentBssid = false;
	state.parentBssidTimer = 0;
	state.isHost = true;
	state.slideLabel = true;
	state.windowOffsetY = -WindowSlideDistance;
	state.settings[0] = 0;
	state.settings[1] = 0;
	state.settings[2] = 0;
	state.settings[3] = 0;

	Coop::isActive = true;

	FS::setReadModeSafe();
	createPacketBuffer();
	loadVisuals();
	changeStep(Step::CharSelect);
}

// Returns true when the title screen's own menu update must be skipped
bool update(Scene* scene)
{
	if (state.step == Step::Closed || state.step == Step::HostFileSelect)
		return false;

	if (state.step == Step::Launched)
		return true;

	for (u32 i = 0; i < state.buttonCount; i++)
		state.buttons[i].update();

	bool sliding = state.slideTimer > 0;
	if (sliding)
		state.slideTimer--;

	if (state.changeDelay > 0)
	{
		if (--state.changeDelay <= 0)
			changeStep(state.scheduledStep);
		return true;
	}

	// Like the file select, the buttons cannot be used while they slide in
	u16 pressed = sliding ? 0 : getPressedKeys();
	s16 touchedBox = sliding ? -1 : getTouchedBox();

	switch (state.step)
	{
	case Step::CharSelect: updateCharSelect(pressed, touchedBox); break;
	case Step::Search: updateSearch(pressed, touchedBox); break;
	case Step::Confirm: updateConfirm(pressed, touchedBox); break;
	case Step::WaitHost: updateWaitHost(pressed, touchedBox); break;
	case Step::PlayerLeft: updatePlayerLeft(); break;
	case Step::Link: updateLink(); break;
	case Step::SyncSave: updateSyncSave(); break;
	case Step::Closing: updateClosing(); break;
	default: break;
	}

	return true;
}

// Returns true when the title screen's own menu render must be skipped
bool render(Scene* scene)
{
	// The menu graphics overwrite the wireless icon colors, the scene only loads them on creation
	if (state.step != Step::Closed)
		CoopSym_Wifi_loadIconPalette(scene->wifiIconOBJIndex, scene->wifiIconOBJPalette, 1);

	if (!state.visualsActive)
		return false;

	updateLabelSlide();
	renderLabel();

	for (u32 i = 0; i < state.buttonCount; i++)
		state.buttons[i].render(state.step == Step::Closing ? getButtonSlideOutX(i) : getButtonSlideX(i));

	return true;
}

// Mario picked a file in the file select, Save::mainSave holds it.
// Returns true when the title screen must not switch scenes by itself.
bool onFileChosen(Scene* scene)
{
	if (state.step != Step::HostFileSelect)
		return false;

	u32 slot = field8(scene, FileSelectButtonOffset);
	MI_CpuCopyFast(&Save::mainSave, &state.bootPacket.save, sizeof(MainSave));
	state.bootPacket.slot = slot;

	changeStep(Step::SyncSave);
	return true;
}

// Mario left the file select to the main menu
static void onMainMenuInit()
{
	if (state.step == Step::HostFileSelect)
		close();
}

// ---------------- Hooks ----------------

ncp_asmfunc void hooks()
{asm(R"(
// Main menu button 2 (TitleScreenScene::executeMainMenuSelection)
ncp_jump(0x020D2F64, 9)
	BL      _ZN4Coop7Vanilla13TitleCoopMenu4openEP5Scene
	B       0x020D2FD8

// TitleScreenScene::onUpdate, before the menu update
ncp_jump(0x020D3438, 9)
	MOV     R0, R4
	BL      _ZN4Coop7Vanilla13TitleCoopMenu6updateEP5Scene
	CMP     R0, #0
	BNE     0x020D3470
	LDRB    R0, [R4, #0x7D]
	B       0x020D343C

// TitleScreenScene::onRender, before the menu render
ncp_jump(0x020CF848, 9)
	MOV     R0, R4
	BL      _ZN4Coop7Vanilla13TitleCoopMenu6renderEP5Scene
	CMP     R0, #0
	BNE     0x020CF874
	LDRB    R0, [R4, #0x64]
	B       0x020CF84C

// TitleScreenScene::executeFileSelectSelection, after the file was loaded
ncp_jump(0x020D2704, 9)
	MOV     R0, R4
	BL      _ZN4Coop7Vanilla13TitleCoopMenu12onFileChosenEP5Scene
	CMP     R0, #0
	POPNE   {R4, PC}
	LDRB    R0, [R4, #0x68]
	B       0x020D2708
)");}

// The main menu stops the music when picking buttons 2 and 3, keep it playing for the co-op menu
ncp_repl(0x020D30E0, 9, "B 0x020D30F4") // Touch, button 2
ncp_repl(0x020D3124, 9, "SUB R0, R4, #2") // Keys, only stop for button 3
ncp_repl(0x020D312C, 9, "CMP R0, #0")

// The title screen clouds use a repeating HBlank DMA on channel 1, which wireless needs:
// WM copies with DMA 1 and waits for the channel to be free, freezing forever.
// Move the clouds to channel 0, the one stages use for the top screen parallax.
ncp_repl(0x020CE060, 9, "MOV R0, #0") // MI_IsDmaBusy
ncp_repl(0x020CE070, 9, "MOV R0, #0") // MI_StopDma
ncp_repl(0x020CE07C, 9, "MOV R0, #0") // MI_StopDma
ncp_repl(0x020CE094, 9, "MOV R0, #0") // MI_HBlankDmaCopy16
ncp_repl(0x020CE1F4, 9, "MOV R0, #0") // MI_IsDmaBusy (clouds teardown)
ncp_repl(0x020CE204, 9, "MOV R0, #0") // MI_StopDma (clouds teardown)

// TitleScreenScene::initializeMainMenu
ncp_call(0x020D326C, 9)
void initializeMainMenu_hook(void* self)
{
	CoopSym_TitleScreen_loadMainMenuGraphics(self); // Keep replaced instruction
	onMainMenuInit();
}

// TitleScreenScene::onCreate, the connection is stopped here when coming back to the title screen
ncp_call(0x020D3580, 9)
void onCreate_hook()
{
	Net::stopConnection(); // Keep replaced instruction
	state.step = Step::Closed;
	state.visualsActive = false;
	state.packetBufferAlive = false;
}

} // namespace Coop::Vanilla::TitleCoopMenu
