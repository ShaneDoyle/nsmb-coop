#include "coop/CoopSettings.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/system/input.hpp>

#include <cstring>

#include "console/console.hpp"

namespace CoopSettings {

constexpr s32 SfxCursorMove = 0xE5;
constexpr s32 SfxConfirm = 0xE9;

struct SettingEntry
{
	const char* name;
	const char* offText;
	const char* onText;
};

static const SettingEntry entries[Count] = {
	{ "Player collision", "Off", "On" },
	{ "Bumping", "Off", "On" },
	{ "Head jumps", "Bounce", "Knock down" },
	{ "Ground pounds", "Bump", "Hurt" },
	{ "Mega crush", "Off", "On" },
	{ "Fireballs", "Pass through", "Hit" },
};

enum Action : u8
{
	RestoreDefaults = Count,
	Back,

	RowCount
};

constexpr u8 FirstRowY = 3;
constexpr u8 RowSpacing = 2;

u8 values[Count];

// Offsets hardcoded in the assembly patches of CoopPlayer.cpp
static_assert(PlayerBump == 1);
static_assert(HeadJumpKnockdown == 2);
static_assert(FireballHit == 5);

static u8 defaultsMode = 0xFF; // The Game::vsMode the values were reset for
static bool menuOpen = false;
static u8 menuOwner;
static u8 cursor;

ncp_thumb void restoreDefaults()
{
	bool vs = Game::vsMode != 0;

	values[PlayerCollision] = true;
	values[PlayerBump] = vs;
	values[HeadJumpKnockdown] = vs;
	values[GroundPoundHurt] = vs;
	values[MegaCrush] = vs;
	values[FireballHit] = vs;

	defaultsMode = Game::vsMode;
}

// The settings carry over between levels and only reset when switching between co-op and VS
ncp_thumb void onLevelLoad()
{
	// Whatever had the console open went away with the previous scene
	if (Console::isOpen())
		Console::discard();
	menuOpen = false;

	if (defaultsMode != Game::vsMode)
		restoreDefaults();
}

static bool isDisabled(u32 row)
{
	return row != PlayerCollision && row < Count && !values[PlayerCollision];
}

ncp_thumb static void printRight(u8 y, u8 color, const char* text)
{
	Console::printxy(Console::Width - 1 - strlen(text), y, "^c%d^%s", color, text);
}

ncp_thumb static void draw()
{
	Console::clear();

	Console::printxy(0, 0, "^l7^");
	Console::align(Console::Align::Center);
	Console::printxy(0, 1, " Co-op Settings ");
	Console::align(Console::Align::Left);

	for (u32 row = 0; row < RowCount; row++)
	{
		u8 y = FirstRowY + row * RowSpacing;
		bool selected = row == cursor;
		u8 color = isDisabled(row) ? 7 : (selected ? 3 : 0);

		if (selected)
			Console::printxy(0, y, "^c3^>");

		if (row < Count)
		{
			const SettingEntry& entry = entries[row];
			Console::printxy(2, y, "^c%d^%s", color, entry.name);
			printRight(y, color, values[row] ? entry.onText : entry.offText);
		}
		else
		{
			Console::printxy(2, y, "^c%d^%s", color, row == RestoreDefaults ? "Restore defaults" : "Back");
		}
	}

	if (menuOwner == Game::localPlayerID)
	{
		Console::printxy(1, 20, "^c7^Up/Down: Choose");
		Console::printxy(1, 21, "^c7^Left/Right/A: Change  B: Back");
	}
	else
	{
		Console::printxy(1, 21, "^c7^Player %d is changing settings", menuOwner + 1);
	}

	Console::printxy(0, 23, "^l7^");

	Console::update();
}

ncp_thumb static void close()
{
	Console::close();
	menuOpen = false;
}

ncp_thumb void open(u32 ownerID)
{
	// If the screen can not be taken over, update reports the menu closed and the pause menu carries on
	if (!Console::open())
		return;

	menuOpen = true;
	menuOwner = ownerID;
	cursor = 0;
	draw();
}

ncp_thumb bool update()
{
	if (!menuOpen)
		return false;

	u16 pressed = Input::consoleKeys[menuOwner].pressed;
	u16 repeated = Input::consoleKeysRepeated[menuOwner];

	if (pressed & (Keys::B | Keys::Start))
	{
		SND::playSFX(SfxConfirm);
		close();
		return true;
	}

	if (repeated & Keys::Down)
	{
		cursor = cursor == RowCount - 1 ? 0 : cursor + 1;
		SND::playSFX(SfxCursorMove);
	}
	else if (repeated & Keys::Up)
	{
		cursor = cursor == 0 ? RowCount - 1 : cursor - 1;
		SND::playSFX(SfxCursorMove);
	}
	else if (cursor < Count)
	{
		if (pressed & (Keys::A | Keys::Horizontal))
		{
			values[cursor] = !values[cursor];
			SND::playSFX(SfxConfirm);
		}
	}
	else if (pressed & Keys::A)
	{
		SND::playSFX(SfxConfirm);

		if (cursor == Back)
		{
			close();
			return true;
		}

		restoreDefaults();
	}

	draw();
	return true;
}

bool isOpen()
{
	return menuOpen;
}

} // namespace CoopSettings
