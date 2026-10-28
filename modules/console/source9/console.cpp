#include "console/console.hpp"

#include <nsmb/core/filesystem/file.hpp>
#include <nsmb/core/system/memory.hpp>

#include "fid.hpp"

namespace Console {

// The console uses the start of the sub BG VRAM: the font at character base 0
// and the text map at screen base 0x2000, right after the font.
constexpr u32 FontSize = 0x1C00;
constexpr u32 ScreenOffset = 0x2000;
constexpr u32 VramBackupSize = ScreenOffset + Width * Height * sizeof(u16);
constexpr u32 PaletteSize = 0x200;

constexpr u8 CharOffset = 32; // The font starts at the space character

// BG1, 256x256, 16 colors, character base 0, screen base 0x2000, priority 0
constexpr u16 BG1Control = (ScreenOffset / 0x800) << REG_G2S_DB_BG1CNT_SCREENBASE_SHIFT;

struct ScreenBackup
{
	u32 dispcnt;
	u16 bg1cnt;
	u16 bldcnt;
	u16 masterBright;
	Heap* heap;
	u8* data; // VRAM followed by palette
};

struct State
{
	u16 textBuffer[Width * Height];
	u16 textColor;
	Align alignment;
	u8 col;
	u8 colOff;
	u8 row;
};

static State state;
static ScreenBackup backup;
static bool opened = false;

static void checkCursorBounds()
{
	if (state.col >= Width)
	{
		state.col = 0;
		state.row++;
	}

	if (state.row >= Height)
		state.row = Height - 1;
}

static void putChar(char c)
{
	state.textBuffer[state.col + state.row * Width] = (u8(c) - CharOffset) | (state.textColor << 12);
	state.col++;
	checkCursorBounds();
}

static void newLine()
{
	state.row++;
	state.col = 0;
	checkCursorBounds();
}

static void tab()
{
	state.col += 3;
	checkCursorBounds();
}

static void longLine(u8 color)
{
	u16 oldColor = state.textColor;
	state.textColor = color;

	if (state.col > 1)
		newLine();
	for (u32 i = 0; i < Width; i++)
		putChar(LineChar);

	state.textColor = oldColor;
}

static void formatBuffer(const char* text, va_list vlist)
{
	char dest[Width * Height];
	s32 length = OS_VSNPrintf(dest, sizeof(dest), text, vlist);
	if (length > s32(sizeof(dest)) - 1)
		length = sizeof(dest) - 1;

	s32 column = state.col;

	switch (state.alignment)
	{
	case Align::Left:
		break;
	case Align::Center:
		column = (s32(Width) - length) / 2;
		break;
	case Align::Right:
		column = s32(Width) - length;
		break;
	}

	if (column < 0)
		column = 0;

	state.col = column + state.colOff;
	checkCursorBounds();

	for (s32 i = 0; dest[i] != '\0'; i++)
	{
		char c = dest[i];

		switch (c)
		{
		case '\n':
			newLine();
			break;
		case '\t':
			tab();
			break;
		case '^':
			// Escape sequences are exactly ^<cmd><arg>^
			if (dest[i + 1] == '\0' || dest[i + 2] == '\0' || dest[i + 3] != '^')
			{
				putChar(c);
				break;
			}
			if (dest[i + 1] == 'c')
				state.textColor = dest[i + 2] - '0';
			else if (dest[i + 1] == 'l')
				longLine(dest[i + 2] - '0');
			i += 3;
			break;
		default:
			putChar(c);
			break;
		}
	}
}

// The scene may reprogram the bottom screen while the console is up, so this
// is applied every frame rather than once.
static void applyDisplayRegisters()
{
	reg_GXS_DB_DISPCNT = (backup.dispcnt & ~(REG_GXS_DB_DISPCNT_BGMODE_MASK | REG_GXS_DB_DISPCNT_DISPLAY_MASK)) | (GX_PLANEMASK_BG1 << REG_GXS_DB_DISPCNT_DISPLAY_SHIFT);
	reg_G2S_DB_BG1CNT = BG1Control;
	reg_G2S_DB_BLDCNT = 0;
	reg_GXS_DB_MASTER_BRIGHT = 0;
	G2S_SetBG1Offset(0, 0);
}

static void loadFile(u32 extFileID, void* dest)
{
	u32 oldDMA = FS::setDefaultDMA(FS_DMA_NOT_USE);
	FS::loadExtFile(extFileID, dest);
	FS::setDefaultDMA(oldDMA);
}

bool open()
{
	if (opened)
		return true;

	backup.heap = Memory::currentHeapPtr;
	backup.data = scast<u8*>(backup.heap->allocate(VramBackupSize + PaletteSize, 4));
	if (!backup.data)
		return false;

	backup.dispcnt = reg_GXS_DB_DISPCNT;
	backup.bg1cnt = reg_G2S_DB_BG1CNT;
	backup.bldcnt = reg_G2S_DB_BLDCNT;
	backup.masterBright = reg_GXS_DB_MASTER_BRIGHT;
	MI_CpuCopy32(rcast<void*>(HW_DB_BG_VRAM), backup.data, VramBackupSize);
	MI_CpuCopy32(rcast<void*>(HW_DB_BG_PLTT), backup.data + VramBackupSize, PaletteSize);

	loadFile("console/font_ncg.bin"fid, rcast<void*>(HW_DB_BG_VRAM));
	loadFile("console/font_ncl.bin"fid, rcast<void*>(HW_DB_BG_PLTT));
	setBackdrop(GX_RGB(0, 0, 0));

	opened = true;

	state.textColor = 0;
	clear();
	update();

	return true;
}

void close()
{
	if (!opened)
		return;

	MI_CpuCopy32(backup.data, rcast<void*>(HW_DB_BG_VRAM), VramBackupSize);
	MI_CpuCopy32(backup.data + VramBackupSize, rcast<void*>(HW_DB_BG_PLTT), PaletteSize);
	reg_GXS_DB_DISPCNT = backup.dispcnt;
	reg_G2S_DB_BG1CNT = backup.bg1cnt;
	reg_G2S_DB_BLDCNT = backup.bldcnt;
	reg_GXS_DB_MASTER_BRIGHT = backup.masterBright;

	backup.heap->deallocate(backup.data);
	backup.data = nullptr;
	opened = false;
}

bool isOpen()
{
	return opened;
}

void discard()
{
	backup.data = nullptr;
	opened = false;
}

void clear()
{
	for (u32 i = 0; i < Width * Height; i++)
		state.textBuffer[i] = ' ' - CharOffset;

	setColumnOff(0);
	gotoXY(0, 0);
	align(Align::Left);
}

void update()
{
	if (!opened)
		return;

	applyDisplayRegisters();
	MI_CpuCopy16(state.textBuffer, rcast<void*>(HW_DB_BG_VRAM + ScreenOffset), sizeof(state.textBuffer));
}

void print(const char* text, ...)
{
	va_list vlist;
	va_start(vlist, text);
	formatBuffer(text, vlist);
	va_end(vlist);

	state.textColor = 0;
}

void printxy(u8 x, u8 y, const char* text, ...)
{
	gotoXY(x, y);

	va_list vlist;
	va_start(vlist, text);
	formatBuffer(text, vlist);
	va_end(vlist);

	state.textColor = 0;
}

void gotoXY(u8 x, u8 y)
{
	state.col = x;
	state.row = y;
	checkCursorBounds();
}

void align(Align align)
{
	state.alignment = align;
}

void setBackdrop(GXRgb color)
{
	*rcast<u16*>(HW_DB_BG_PLTT) = color;
}

void setColumnOff(u8 off)
{
	state.colOff = off;
}

} // namespace Console
