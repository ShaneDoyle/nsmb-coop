#pragma once

#include <nds/ndstypes.h>
#include "nsmb/hw.h"

typedef enum
{
	GX_BEGIN_TRIANGLES = 0,
	GX_BEGIN_QUADS = 1,
	GX_BEGIN_TRIANGLE_STRIP = 2,
	GX_BEGIN_QUAD_STRIP = 3
}
GXBegin;

typedef enum
{
	GX_BG0_AS_2D = 0,
	GX_BG0_AS_3D = 1
}
GXBG0As;

typedef enum
{
	GX_BG_CHARBASE_0x00000 = 0,
	GX_BG_CHARBASE_0x04000 = 1,
	GX_BG_CHARBASE_0x08000 = 2,
	GX_BG_CHARBASE_0x0c000 = 3,
	GX_BG_CHARBASE_0x10000 = 4,
	GX_BG_CHARBASE_0x14000 = 5,
	GX_BG_CHARBASE_0x18000 = 6,
	GX_BG_CHARBASE_0x1c000 = 7,
	GX_BG_CHARBASE_0x20000 = 8,
	GX_BG_CHARBASE_0x24000 = 9,
	GX_BG_CHARBASE_0x28000 = 10,
	GX_BG_CHARBASE_0x2c000 = 11,
	GX_BG_CHARBASE_0x30000 = 12,
	GX_BG_CHARBASE_0x34000 = 13,
	GX_BG_CHARBASE_0x38000 = 14,
	GX_BG_CHARBASE_0x3c000 = 15
}
GXBGCharBase;

typedef enum
{
	GX_BG_COLORMODE_16 = 0,
	GX_BG_COLORMODE_256 = 1
}
GXBGColorMode;

typedef enum
{
	GX_BG_SCRBASE_0x0000 = 0,
	GX_BG_SCRBASE_0x0800 = 1,
	GX_BG_SCRBASE_0x1000 = 2,
	GX_BG_SCRBASE_0x1800 = 3,
	GX_BG_SCRBASE_0x2000 = 4,
	GX_BG_SCRBASE_0x2800 = 5,
	GX_BG_SCRBASE_0x3000 = 6,
	GX_BG_SCRBASE_0x3800 = 7,
	GX_BG_SCRBASE_0x4000 = 8,
	GX_BG_SCRBASE_0x4800 = 9,
	GX_BG_SCRBASE_0x5000 = 10,
	GX_BG_SCRBASE_0x5800 = 11,
	GX_BG_SCRBASE_0x6000 = 12,
	GX_BG_SCRBASE_0x6800 = 13,
	GX_BG_SCRBASE_0x7000 = 14,
	GX_BG_SCRBASE_0x7800 = 15,
	GX_BG_SCRBASE_0x8000 = 16,
	GX_BG_SCRBASE_0x8800 = 17,
	GX_BG_SCRBASE_0x9000 = 18,
	GX_BG_SCRBASE_0x9800 = 19,
	GX_BG_SCRBASE_0xa000 = 20,
	GX_BG_SCRBASE_0xa800 = 21,
	GX_BG_SCRBASE_0xb000 = 22,
	GX_BG_SCRBASE_0xb800 = 23,
	GX_BG_SCRBASE_0xc000 = 24,
	GX_BG_SCRBASE_0xc800 = 25,
	GX_BG_SCRBASE_0xd000 = 26,
	GX_BG_SCRBASE_0xd800 = 27,
	GX_BG_SCRBASE_0xe000 = 28,
	GX_BG_SCRBASE_0xe800 = 29,
	GX_BG_SCRBASE_0xf000 = 30,
	GX_BG_SCRBASE_0xf800 = 31
}
GXBGScrBase;

typedef enum
{
	GX_BG_SCRSIZE_TEXT_256x256 = 0,
	GX_BG_SCRSIZE_TEXT_512x256 = 1,
	GX_BG_SCRSIZE_TEXT_256x512 = 2,
	GX_BG_SCRSIZE_TEXT_512x512 = 3
}
GXBGScrSizeText;

typedef enum
{
	GX_DISPMODE_GRAPHICS = 0x01,
	GX_DISPMODE_VRAM_A = 0x02,
	GX_DISPMODE_VRAM_B = 0x06,
	GX_DISPMODE_VRAM_C = 0x0a,
	GX_DISPMODE_VRAM_D = 0x0e,
	GX_DISPMODE_MMEM = 0x03
}
GXDispMode;

typedef enum
{
	GX_BG_EXTPLTT_01 = 0,
	GX_BG_EXTPLTT_23 = 1
}
GXBGExtPltt;

typedef enum
{
	GX_BGMODE_0 = 0,
	GX_BGMODE_1 = 1,
	GX_BGMODE_2 = 2,
	GX_BGMODE_3 = 3,
	GX_BGMODE_4 = 4,
	GX_BGMODE_5 = 5,
	GX_BGMODE_6 = 6
}
GXBGMode;

typedef enum
{
	GX_BUFFERMODE_Z = 0,
	GX_BUFFERMODE_W = 1
}
GXBufferMode;

typedef enum
{
	GX_MTXMODE_PROJECTION = 0,
	GX_MTXMODE_POSITION = 1,
	GX_MTXMODE_POSITION_VECTOR = 2,
	GX_MTXMODE_TEXTURE = 3
}
GXMtxMode;

typedef struct
{
	union
	{
		u32     attr01;
		struct
		{
			u16     attr0;
			u16     attr1;
		};
		struct
		{
			u32     y : 8;
			u32     rsMode : 2;
			u32     objMode : 2;
			u32     mosaic : 1;
			u32     colorMode : 1;
			u32     shape : 2;

			u32     x : 9;
			u32     rsParam : 5;
			u32     size : 2;
		};
		struct
		{
			u32     _0 : 28;
			u32     flipH : 1;
			u32     flipV : 1;
			u32     _1 : 2;
		};
	};
	union
	{
		struct
		{
			u16     attr2;
			u16     _3;
		};
		u32     attr23;
		struct
		{
			u32     charNo : 10;
			u32     priority : 2;
			u32     cParam : 4;
			u32     _2 : 16;
		};
	};
}
GXOamAttr;

typedef enum
{
	GX_PLANEMASK_NONE = 0x00,
	GX_PLANEMASK_BG0 = 0x01,
	GX_PLANEMASK_BG1 = 0x02,
	GX_PLANEMASK_BG2 = 0x04,
	GX_PLANEMASK_BG3 = 0x08,
	GX_PLANEMASK_OBJ = 0x10
}
GXPlaneMask;

typedef enum
{
	GX_SHADING_TOON = 0,
	GX_SHADING_HIGHLIGHT = 1
}
GXShading;

typedef enum
{
	GX_SORTMODE_AUTO = 0,
	GX_SORTMODE_MANUAL = 1
}
GXSortMode;

typedef enum
{
	GX_TEXFLIP_NONE = 0,
	GX_TEXFLIP_S = 1,
	GX_TEXFLIP_T = 2,
	GX_TEXFLIP_ST = 3
}
GXTexFlip;

typedef enum
{
	GX_TEXFMT_NONE = 0,
	GX_TEXFMT_A3I5 = 1,
	GX_TEXFMT_PLTT4 = 2,
	GX_TEXFMT_PLTT16 = 3,
	GX_TEXFMT_PLTT256 = 4,
	GX_TEXFMT_COMP4x4 = 5,
	GX_TEXFMT_A5I3 = 6,
	GX_TEXFMT_DIRECT = 7
}
GXTexFmt;

typedef enum
{
	GX_TEXGEN_NONE = 0,
	GX_TEXGEN_TEXCOORD = 1,
	GX_TEXGEN_NORMAL = 2,
	GX_TEXGEN_VERTEX = 3
}
GXTexGen;

typedef enum
{
	GX_TEXPLTTCOLOR0_USE = 0,
	GX_TEXPLTTCOLOR0_TRNS = 1
}
GXTexPlttColor0;

typedef enum
{
	GX_TEXREPEAT_NONE = 0,
	GX_TEXREPEAT_S = 1,
	GX_TEXREPEAT_T = 2,
	GX_TEXREPEAT_ST = 3
}
GXTexRepeat;

typedef enum
{
	GX_TEXSIZE_S8 = 0,
	GX_TEXSIZE_S16 = 1,
	GX_TEXSIZE_S32 = 2,
	GX_TEXSIZE_S64 = 3,
	GX_TEXSIZE_S128 = 4,
	GX_TEXSIZE_S256 = 5,
	GX_TEXSIZE_S512 = 6,
	GX_TEXSIZE_S1024 = 7
}
GXTexSizeS;

typedef enum
{
	GX_TEXSIZE_T8 = 0,
	GX_TEXSIZE_T16 = 1,
	GX_TEXSIZE_T32 = 2,
	GX_TEXSIZE_T64 = 3,
	GX_TEXSIZE_T128 = 4,
	GX_TEXSIZE_T256 = 5,
	GX_TEXSIZE_T512 = 6,
	GX_TEXSIZE_T1024 = 7
}
GXTexSizeT;

typedef enum
{
	GX_VRAM_A = OS_VRAM_BANK_ID_A,     // VRAM-A
	GX_VRAM_B = OS_VRAM_BANK_ID_B,     // VRAM-B
	GX_VRAM_C = OS_VRAM_BANK_ID_C,     // VRAM-C
	GX_VRAM_D = OS_VRAM_BANK_ID_D,     // VRAM-D
	GX_VRAM_E = OS_VRAM_BANK_ID_E,     // VRAM-E
	GX_VRAM_F = OS_VRAM_BANK_ID_F,     // VRAM-F
	GX_VRAM_G = OS_VRAM_BANK_ID_G,     // VRAM-G
	GX_VRAM_H = OS_VRAM_BANK_ID_H,     // VRAM-H
	GX_VRAM_I = OS_VRAM_BANK_ID_I,     // VRAM-I
	GX_VRAM_ALL = OS_VRAM_BANK_ID_ALL
}
GXVRam;

typedef enum
{
	GX_VRAM_BG_NONE = 0x0000,
	GX_VRAM_BG_16_F = GX_VRAM_F,       // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_F_SIZE
	GX_VRAM_BG_16_G = GX_VRAM_G,       // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_G_SIZE
	GX_VRAM_BG_32_FG = GX_VRAM_F | GX_VRAM_G,   // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_F_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_BG_64_E = GX_VRAM_E,       // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_E_SIZE
	GX_VRAM_BG_80_EF = GX_VRAM_E | GX_VRAM_F,   // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_E_SIZE + HW_VRAM_F_SIZE
	GX_VRAM_BG_96_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G,      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_E_SIZE + HW_VRAM_F_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_BG_128_A = GX_VRAM_A,      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE
	GX_VRAM_BG_128_B = GX_VRAM_B,      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_B_SIZE
	GX_VRAM_BG_128_C = GX_VRAM_C,      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_C_SIZE
	GX_VRAM_BG_128_D = GX_VRAM_D,      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_D_SIZE
	GX_VRAM_BG_256_AB = GX_VRAM_A | GX_VRAM_B,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_B_SIZE
	GX_VRAM_BG_256_BC = GX_VRAM_B | GX_VRAM_C,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_B_SIZE + HW_VRAM_C_SIZE
	GX_VRAM_BG_256_CD = GX_VRAM_C | GX_VRAM_D,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_C_SIZE + HW_VRAM_D_SIZE
	GX_VRAM_BG_384_ABC = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C,     // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_B_SIZE + HW_VRAM_C_SIZE
	GX_VRAM_BG_384_BCD = GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,     // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_B_SIZE + HW_VRAM_C_SIZE + HW_VRAM_D_SIZE
	GX_VRAM_BG_512_ABCD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,        // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_B_SIZE + HW_VRAM_C_SIZE + HW_VRAM_D_SIZE

	// discontinuous on LCDC memory
	GX_VRAM_BG_80_EG = GX_VRAM_E | GX_VRAM_G,   // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_E_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_BG_256_AC = GX_VRAM_A | GX_VRAM_C,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_C_SIZE
	GX_VRAM_BG_256_AD = GX_VRAM_A | GX_VRAM_D,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_D_SIZE
	GX_VRAM_BG_256_BD = GX_VRAM_B | GX_VRAM_D,  // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_B_SIZE + HW_VRAM_D_SIZE
	GX_VRAM_BG_384_ABD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_D,     // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_B_SIZE + HW_VRAM_D_SIZE
	GX_VRAM_BG_384_ACD = GX_VRAM_A | GX_VRAM_C | GX_VRAM_D      // ARM9: HW_BG_VRAM   -->   HW_BG_VRAM + HW_VRAM_A_SIZE + HW_VRAM_C_SIZE + HW_VRAM_D_SIZE
}
GXVRamBG;

typedef enum
{
	GX_VRAM_SUB_BG_NONE = 0x0000,
	GX_VRAM_SUB_BG_128_C = GX_VRAM_C,
	GX_VRAM_SUB_BG_32_H = GX_VRAM_H,
	GX_VRAM_SUB_BG_48_HI = GX_VRAM_H | GX_VRAM_I
}
GXVRamSubBG;

typedef enum
{
	GX_VRAM_CLEARIMAGE_NONE = 0x0000,
	GX_VRAM_CLEARIMAGE_256_AB = GX_VRAM_A | GX_VRAM_B,
	GX_VRAM_CLEARIMAGE_256_CD = GX_VRAM_C | GX_VRAM_D,  // TextureImageSlot 23
	GX_VRAM_CLEARDEPTH_128_A = GX_VRAM_A,
	GX_VRAM_CLEARDEPTH_128_B = GX_VRAM_B,
	GX_VRAM_CLEARDEPTH_128_C = GX_VRAM_C,
	GX_VRAM_CLEARDEPTH_128_D = GX_VRAM_D
}
GXVRamClearImage;

typedef enum
{
	GX_VRAM_LCDC_NONE = 0x0000,
	GX_VRAM_LCDC_A = GX_VRAM_A,        // ARM9: HW_LCDC_VRAM_A  -->  HW_LCDC_VRAM_B
	GX_VRAM_LCDC_B = GX_VRAM_B,        // ARM9: HW_LCDC_VRAM_B  -->  HW_LCDC_VRAM_C
	GX_VRAM_LCDC_C = GX_VRAM_C,        // ARM9: HW_LCDC_VRAM_C  -->  HW_LCDC_VRAM_D
	GX_VRAM_LCDC_D = GX_VRAM_D,        // ARM9: HW_LCDC_VRAM_D  -->  HW_LCDC_VRAM_E
	GX_VRAM_LCDC_E = GX_VRAM_E,        // ARM9: HW_LCDC_VRAM_E  -->  HW_LCDC_VRAM_F
	GX_VRAM_LCDC_F = GX_VRAM_F,        // ARM9: HW_LCDC_VRAM_F  -->  HW_LCDC_VRAM_G
	GX_VRAM_LCDC_G = GX_VRAM_G,        // ARM9: HW_LCDC_VRAM_G  -->  HW_LCDC_VRAM_H
	GX_VRAM_LCDC_H = GX_VRAM_H,        // ARM9: HW_LCDC_VRAM_H  -->  HW_LCDC_VRAM_I
	GX_VRAM_LCDC_I = GX_VRAM_I,        // ARM9: HW_LCDC_VRAM_I  -->  HW_LCDC_VRAM_END

	GX_VRAM_LCDC_ALL = GX_VRAM_ALL     // ARM9: HW_LCDC_VRAM_A  -->  HW_LCDC_VRAM_END
}
GXVRamLCDC;

typedef enum
{
	GX_VRAM_OBJ_NONE = 0x0000,
	GX_VRAM_OBJ_16_F = GX_VRAM_F,      // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_F_SIZE
	GX_VRAM_OBJ_16_G = GX_VRAM_G,      // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_G_SIZE
	GX_VRAM_OBJ_32_FG = GX_VRAM_F | GX_VRAM_G,  // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_F_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_OBJ_64_E = GX_VRAM_E,      // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_E_SIZE
	GX_VRAM_OBJ_80_EF = GX_VRAM_E | GX_VRAM_F,  // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_E_SIZE + HW_VRAM_F_SIZE
	GX_VRAM_OBJ_80_EG = GX_VRAM_E | GX_VRAM_G,  // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_E_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_OBJ_96_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G,     // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_E_SIZE + HW_VRAM_F_SIZE + HW_VRAM_G_SIZE
	GX_VRAM_OBJ_128_A = GX_VRAM_A,     // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_A_SIZE
	GX_VRAM_OBJ_128_B = GX_VRAM_B,     // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_B_SIZE
	GX_VRAM_OBJ_256_AB = GX_VRAM_A | GX_VRAM_B  // ARM9: HW_OBJ_VRAM  -->   HW_OBJ_VRAM + HW_VRAM_A_SIZE + HW_VRAM_B_SIZE
}
GXVRamOBJ;

typedef enum
{
	GX_VRAM_SUB_OBJ_NONE = 0x0000,
	GX_VRAM_SUB_OBJ_128_D = GX_VRAM_D,
	GX_VRAM_SUB_OBJ_16_I = GX_VRAM_I
}
GXVRamSubOBJ;

typedef enum
{
	GX_VRAM_TEX_NONE = 0x0000,         // none
	GX_VRAM_TEX_0_A = GX_VRAM_A,       // TextureImageSlot 0
	GX_VRAM_TEX_0_B = GX_VRAM_B,       // TextureImageSlot 0
	GX_VRAM_TEX_0_C = GX_VRAM_C,       // TextureImageSlot 0
	GX_VRAM_TEX_0_D = GX_VRAM_D,       // TextureImageSlot 0
	GX_VRAM_TEX_01_AB = GX_VRAM_A | GX_VRAM_B,  // TextureImageSlot 01
	GX_VRAM_TEX_01_BC = GX_VRAM_B | GX_VRAM_C,  // TextureImageSlot 01
	GX_VRAM_TEX_01_CD = GX_VRAM_C | GX_VRAM_D,  // TextureImageSlot 01
	GX_VRAM_TEX_012_ABC = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C,    // TextureImageSlot 012
	GX_VRAM_TEX_012_BCD = GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,    // TextureImageSlot 012
	GX_VRAM_TEX_0123_ABCD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,      // TextureImageSlot 0123

	// discontinuous on LCDC memory
	GX_VRAM_TEX_01_AC = GX_VRAM_A | GX_VRAM_C,  // TextureImageSlot 01
	GX_VRAM_TEX_01_AD = GX_VRAM_A | GX_VRAM_D,  // TextureImageSlot 01
	GX_VRAM_TEX_01_BD = GX_VRAM_B | GX_VRAM_D,  // TextureImageSlot 01
	GX_VRAM_TEX_012_ABD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_D,    // TextureImageSlot 012
	GX_VRAM_TEX_012_ACD = GX_VRAM_A | GX_VRAM_C | GX_VRAM_D     // TextureImageSlot 012
}
GXVRamTex;

typedef enum
{
	GX_VRAM_TEXPLTT_NONE = 0x0000,
	GX_VRAM_TEXPLTT_0_F = GX_VRAM_F,   // TexturePltt 0
	GX_VRAM_TEXPLTT_0_G = GX_VRAM_G,   // TexturePltt 0
	GX_VRAM_TEXPLTT_01_FG = GX_VRAM_F | GX_VRAM_G,      // TexturePltt 01
	GX_VRAM_TEXPLTT_0123_E = GX_VRAM_E, // TexturePltt 0-3
	GX_VRAM_TEXPLTT_01234_EF = GX_VRAM_E | GX_VRAM_F,   // TexturePltt 0-3.4
	GX_VRAM_TEXPLTT_012345_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G      // TexturePltt 0-3,45
}
GXVRamTexPltt;

typedef enum
{
	GX_POLYGONMODE_MODULATE = 0,
	GX_POLYGONMODE_DECAL = 1,
	GX_POLYGONMODE_TOON = 2,
	GX_POLYGONMODE_SHADOW = 3
}
GXPolygonMode;

typedef enum
{
	GX_CULL_ALL = 0,
	GX_CULL_FRONT = 1,
	GX_CULL_BACK = 2,
	GX_CULL_NONE = 3
}
GXCull;

typedef enum
{
	GX_VRAM_BGEXTPLTT_NONE = 0x0000,
	GX_VRAM_BGEXTPLTT_01_F = GX_VRAM_F,              // BGExtPltt 0-1 <--- Caution
	GX_VRAM_BGEXTPLTT_23_G = GX_VRAM_G,              // BGExtPltt 2-3 <--- Caution // BGCNT
	GX_VRAM_BGEXTPLTT_0123_E = GX_VRAM_E,              // BGExtPltt 0-3
	GX_VRAM_BGEXTPLTT_0123_FG = GX_VRAM_F | GX_VRAM_G   // BGExtPltt 0-1,2-3
}
GXVRamBGExtPltt;

typedef enum
{
	GX_VRAM_SUB_BGEXTPLTT_NONE = 0x0000,
	GX_VRAM_SUB_BGEXTPLTT_0123_H = GX_VRAM_H
}
GXVRamSubBGExtPltt;

typedef enum
{
	GX_VRAM_OBJEXTPLTT_NONE = 0,
	GX_VRAM_OBJEXTPLTT_0_F = GX_VRAM_F, // OBJExtPltt
	GX_VRAM_OBJEXTPLTT_0_G = GX_VRAM_G // OBJExtPltt
}
GXVRamOBJExtPltt;

typedef enum
{
	GX_VRAM_SUB_OBJEXTPLTT_NONE = 0x0000,
	GX_VRAM_SUB_OBJEXTPLTT_0_I = GX_VRAM_I
}
GXVRamSubOBJExtPltt;

#ifdef __cplusplus
extern "C"
{
#endif

	void GX_BeginLoadTex();
	void GX_BeginLoadTexPltt();
	void GX_DisableBankForLCDC();
	void GX_EndLoadTex();
	void GX_EndLoadTexPltt();
	void GX_LoadBG1Char(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadBG1Scr(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadBG2Scr(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadBG3Scr(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadBGPltt(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadOAM(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadOBJPltt(const void* pSrc, u32 offset, u32 szByte);
	void GX_LoadTex(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GX_LoadTexPltt(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GX_SetBankForBG(GXVRamBG bg);
	void GX_SetBankForLCDC(int lcdc);
	void GX_SetBankForOBJ(GXVRamOBJ obj);
	void GX_SetBankForSubOBJ(GXVRamSubOBJ sub_obj);
	void GX_SetBankForTex(GXVRamTex tex);
	void GX_SetBankForTexPltt(GXVRamTexPltt texPltt);
	void GX_SetGraphicsMode(GXDispMode dispMode, GXBGMode bgMode, GXBG0As bg0_2d3d);

	void GXS_LoadBG0Char(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadBG0Scr(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadBG2Scr(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadBG3Scr(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadBGPltt(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadOAM(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadOBJ(const void* pSrc, u32 offset, u32 szByte);
	void GXS_LoadOBJPltt(const void* pSrc, u32 offset, u32 szByte);
	void GXS_SetGraphicsMode(GXBGMode bgMode);

	void GXx_SetMasterBrightness_(vu16* reg, int brightness);
	void GX_SetBankForBGExtPltt(GXVRamBGExtPltt bgExtPltt);
	void GX_SetBankForSubBG(GXVRamSubBG bg);
	void GX_SetBankForSubBGExtPltt(GXVRamSubBGExtPltt bgExtPltt);
	void GX_SetBankForOBJExtPltt(GXVRamOBJExtPltt objExtPltt);

	void GX_BeginLoadBGExtPltt(void);
	void GX_LoadBGExtPltt(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GX_EndLoadBGExtPltt(void);
	void GX_BeginLoadOBJExtPltt(void);
	void GX_LoadOBJExtPltt(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GX_EndLoadOBJExtPltt(void);
	void GXS_BeginLoadBGExtPltt(void);
	void GXS_LoadBGExtPltt(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GXS_EndLoadBGExtPltt(void);
	void GXS_BeginLoadOBJExtPltt(void);
	void GXS_LoadOBJExtPltt(const void* pSrc, u32 destSlotAddr, u32 szByte);
	void GXS_EndLoadOBJExtPltt(void);

	void GX_DispOn(void);
	void GX_DispOff(void);

	GXVRamBG GX_ResetBankForBG(void);
	GXVRamOBJ GX_ResetBankForOBJ(void);
	GXVRamBGExtPltt GX_ResetBankForBGExtPltt(void);
	GXVRamOBJExtPltt GX_ResetBankForOBJExtPltt(void);
	GXVRamTex GX_ResetBankForTex(void);
	GXVRamTexPltt GX_ResetBankForTexPltt(void);
	GXVRamClearImage GX_ResetBankForClearImage(void);
	GXVRamSubBG GX_ResetBankForSubBG(void);
	GXVRamSubOBJ GX_ResetBankForSubOBJ(void);
	GXVRamSubBGExtPltt GX_ResetBankForSubBGExtPltt(void);
	GXVRamSubOBJExtPltt GX_ResetBankForSubOBJExtPltt(void);

	u16* G2_GetBG0CharPtr();
	u16* G2_GetBG0ScrPtr();
	u16* G2_GetBG1CharPtr();
	u16* G2_GetBG1ScrPtr();
	u16* G2_GetBG2ScrPtr();
	u16* G2_GetBG2CharPtr();
	u16* G2_GetBG3ScrPtr();
	u16* G2_GetBG3CharPtr();

	u16* G2S_GetBG0CharPtr();
	u16* G2S_GetBG0ScrPtr();
	u16* G2S_GetBG1CharPtr();
	u16* G2S_GetBG1ScrPtr();
	u16* G2S_GetBG2CharPtr();
	u16* G2S_GetBG2ScrPtr();
	u16* G2S_GetBG3CharPtr();
	u16* G2S_GetBG3ScrPtr();

	void G3_LoadMtx43(void* m);

#ifdef __cplusplus
}
#endif