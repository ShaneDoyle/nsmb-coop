#pragma once

#include <nds/ndstypes.h>
#include "nsmb/vector.h"

#define OAM_DATA_END ((u32)(0xFFFF<<16))

typedef struct OAMEntry {
	u16 attr0;
	u16 attr1;
	u32 attr2;
} OAMEntry;

typedef struct
{
	u32 fileID;
	OAMEntry** entriesTable;
	u32 entryCount;
	u32 ncgOffsetShifted;
}	NCGFileEntry;

typedef struct
{
	NCGFileEntry* fileInfo;
	u32 loaded;
}	NCGFileList;

typedef struct
{
	OAMEntry* entries;
	u32 a0;
	u32 a1;
}	OAMFrameTableEntry;

typedef struct
{
	OAMFrameTableEntry* frameTable;
	u32 entries;
}	OAMAnimationsTableEntry;

typedef struct
{
	OAMFrameTableEntry* frameTable;
	u32 frameTableID;
	u32 currentFrameSize;
	u32 size;
	u16 currentFrame;
	u16 frames;
	u8 bitfield;	// 0x1 = always, 0x4 = size was adjusted to be positive
	u8 pad[3];
}	OAMSprite;

#ifdef __cplusplus
extern "C" {
#endif

	// Draws OAM relative to the level
	void OAM_DrawSprite(OAMEntry* entries, u32 posX, u32 posY, bool flip, u8 palette, u8, Vec2* scale, u16 rotX, u16* rotY, u32 priorityMaybe);

	// Draws OAM relative to a level sprite
	void OAM_DrawFromSprite(OAMSprite* sprite, u32 posX, u32 posY, bool flip, u8 palette, u8, Vec2* scale, u16 rotX, u16* rotY, u32 priorityMaybe);

	// Draws OAM relative to the top screen HUD
	void OAM_DrawHUD(OAMEntry* entries, u8 x, u8 y, bool flip, u8 palette, u8, Vec2* scale, u16 ZERO, u8, u8);

	// Draws OAM relative to the bottom screen HUD
	void OAM_DrawHUDSub(OAMEntry* entries, u8 x, u8 y, bool flip, u8 palette, u8, Vec2* scale, u16 ZERO, u8, u8);

	// Draws OAM relative to the top screen HUD offsetted by the position defined in the loaded BNCL file
	void OAM_DrawHUDFromLoadedBNCL(int index, OAMEntry* entries, bool flip, u8 palette, int unk1, Vec2* scale, int unk2, int unk3, int unk4, int x_relativeshift, int y_relativeshift);

	// Draws OAM relative to the bottom screen HUD offsetted by the position defined in the loaded BNCL file
	void OAM_DrawHUDSubFromLoadedBNCL(int index, OAMEntry* entries, bool flip, u8 palette, int unk1, Vec2* scale, int unk2, int unk3, int unk4, int x_relativeshift, int y_relativeshift);

	// Updates OAM digits
	void OAM_UpdateDigits(OAMEntry* entries, u32* entryTable, s32 value, s32 digitNo, u8 unk5);

	//////////////////////
	// Graphics loading //
	//////////////////////

	void loadNCGFile_intoVRam(u32 ListID);
	void loadNCGFile_intoVRam_OAM(NCGFileEntry* file);

	void loadNCGFile(NCGFileEntry* file);

#ifdef __cplusplus
}
#endif