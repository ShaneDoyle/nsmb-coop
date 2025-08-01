#include <nsmb_nitro.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>

#ifdef GAME_LANGUAGE_FR

constexpr s32 FileSelectNumberXOffset = 10;
constexpr s32 WorldmapStarCoin1XOffset = -34;
constexpr s32 WorldmapStarCoin2XOffset = 24;
constexpr s32 WorldmapStarCoin3XOffset = -4;
constexpr s32 WorldmapStarCoin4XOffset = -4;

#elif GAME_LANGUAGE_GE

constexpr s32 FileSelectNumberXOffset = 7;

#define SKIP_COMMON_WORLDMAP_OVERRIDES

/*
// Shfit NSMB USA OAM offsets and print them

OAM::File* file = rcast<OAM::File*>(0x0216FB90);
for (s32 i = 0; i < file->oamAttrCount; i++)
{
	GXOamAttr* oamAttrs = file->pOamAttrs[i];
	GXOamAttr* oamAttr = oamAttrs;

	while (true)
	{
		if (oamAttr->charNo > 56)
		{
			oamAttr->charNo += 2;
			Log::print("ncp_over(0x%08X, 54) u32 over_%08X[2] = { 0x%08X, 0x%08X };\n", rcast<u32>(oamAttr), rcast<u32>(oamAttr), rcast<u32*>(oamAttr)[0], rcast<u32*>(oamAttr)[1]);
		}

		if (oamAttr->_3 == 0xFFFF)
			break;
		oamAttr++;
	};
}
*/

// Auto-generated from code above
ncp_over(0x0216EA64, 54) u32 over_0216EA64[2] = { 0x40000000, 0x0000003C };
ncp_over(0x0216EA6C, 54) u32 over_0216EA6C[2] = { 0x40010001, 0xFFFF003C };
ncp_over(0x0216E9B4, 54) u32 over_0216E9B4[2] = { 0x40000000, 0x0000003C };
ncp_over(0x0216E9BC, 54) u32 over_0216E9BC[2] = { 0x40010001, 0xFFFF003C };
ncp_over(0x0216E974, 54) u32 over_0216E974[2] = { 0x81E800E8, 0x0000103D };
ncp_over(0x0216E97C, 54) u32 over_0216E97C[2] = { 0x800880E8, 0x00001041 };
ncp_over(0x0216E984, 54) u32 over_0216E984[2] = { 0x81E84008, 0x00001043 };
ncp_over(0x0216E98C, 54) u32 over_0216E98C[2] = { 0x40080008, 0xFFFF1045 };
ncp_over(0x0216E764, 54) u32 over_0216E764[2] = { 0x81F000F0, 0xFFFFD046 };
ncp_over(0x0216E72C, 54) u32 over_0216E72C[2] = { 0x81F000F0, 0xFFFFB04A };
ncp_over(0x0216E76C, 54) u32 over_0216E76C[2] = { 0x81F000F0, 0xFFFFC04E };
ncp_over(0x0216E77C, 54) u32 over_0216E77C[2] = { 0x81F000F0, 0xFFFFD052 };
ncp_over(0x0216E784, 54) u32 over_0216E784[2] = { 0x41F800F8, 0xFFFFC056 };
ncp_over(0x0216E78C, 54) u32 over_0216E78C[2] = { 0x41F800F8, 0xFFFF3057 };
ncp_over(0x0216E73C, 54) u32 over_0216E73C[2] = { 0x41F800F8, 0xFFFF0058 };
ncp_over(0x0216E75C, 54) u32 over_0216E75C[2] = { 0x41F800F8, 0xFFFF0059 };
ncp_over(0x0216E74C, 54) u32 over_0216E74C[2] = { 0x41F800F8, 0xFFFF205A };
ncp_over(0x0216E754, 54) u32 over_0216E754[2] = { 0x81F040F8, 0xFFFF205B };

#elif GAME_LANGUAGE_IT

constexpr s32 FileSelectNumberXOffset = 0;
constexpr s32 WorldmapStarCoin1XOffset = -34;
constexpr s32 WorldmapStarCoin2XOffset = 24;
constexpr s32 WorldmapStarCoin3XOffset = -4;
constexpr s32 WorldmapStarCoin4XOffset = -4;

#elif GAME_LANGUAGE_SP

constexpr s32 FileSelectNumberXOffset = 8;
constexpr s32 WorldmapStarCoin1XOffset = -34;
constexpr s32 WorldmapStarCoin2XOffset = 24;
constexpr s32 WorldmapStarCoin3XOffset = -4;
constexpr s32 WorldmapStarCoin4XOffset = -4;

#elif GAME_LANGUAGE_JP

constexpr s32 FileSelectNumberXOffset = 6;
constexpr s32 WorldmapStarCoin1XOffset = -34;
constexpr s32 WorldmapStarCoin2XOffset = 23;
constexpr s32 WorldmapStarCoin3XOffset = -5;
constexpr s32 WorldmapStarCoin4XOffset = -5;

/*
// Shfit NSMB USA OAM offsets and print them

OAM::File* file = rcast<OAM::File*>(0x020DB1A4);
for (s32 i = 0; i < file->oamAttrCount; i++)
{
	GXOamAttr* oamAttrs = file->pOamAttrs[i];
	GXOamAttr* oamAttr = oamAttrs;

	while (true)
	{
		if (oamAttr->charNo > 97)
		{
			oamAttr->charNo += 10;
			Log::print("ncp_over(0x%08X, 9) u32 over_%08X[2] = { 0x%08X, 0x%08X };\n", rcast<u32>(oamAttr), rcast<u32>(oamAttr), rcast<u32*>(oamAttr)[0], rcast<u32*>(oamAttr)[1]);
		}

		if (oamAttr->_3 == 0xFFFF)
			break;
		oamAttr++;
	};
}
*/

// Auto-generated from code above
ncp_over(0x020DBA98, 9) u32 over_020DBA98[2] = { 0x805C0010, 0x0000206C };
ncp_over(0x020DBAA0, 9) u32 over_020DBAA0[2] = { 0xC18400D0, 0x00002070 };
ncp_over(0x020DBAA8, 9) u32 over_020DBAA8[2] = { 0xC1C400D0, 0x00002080 };
ncp_over(0x020DBAB0, 9) u32 over_020DBAB0[2] = { 0xC00400D0, 0x00002090 };
ncp_over(0x020DBAB8, 9) u32 over_020DBAB8[2] = { 0x804480F0, 0x000020A0 };
ncp_over(0x020DBAC0, 9) u32 over_020DBAC0[2] = { 0xC1844010, 0x000020A2 };
ncp_over(0x020DBAC8, 9) u32 over_020DBAC8[2] = { 0xC1C44010, 0x000020AA };
ncp_over(0x020DBAD0, 9) u32 over_020DBAD0[2] = { 0xC0044010, 0x000020B2 };
ncp_over(0x020DBAD8, 9) u32 over_020DBAD8[2] = { 0x80448010, 0x000020BA };
ncp_over(0x020DBAE0, 9) u32 over_020DBAE0[2] = { 0x40548010, 0xFFFF20BC };
ncp_over(0x020DBAE8, 9) u32 over_020DBAE8[2] = { 0x807000F8, 0x0000206C };
ncp_over(0x020DBAF0, 9) u32 over_020DBAF0[2] = { 0xC19800B8, 0x00002070 };
ncp_over(0x020DBAF8, 9) u32 over_020DBAF8[2] = { 0xC1D800B8, 0x00002080 };
ncp_over(0x020DBB00, 9) u32 over_020DBB00[2] = { 0xC01800B8, 0x00002090 };
ncp_over(0x020DBB08, 9) u32 over_020DBB08[2] = { 0x805880D8, 0x000020A0 };
ncp_over(0x020DBB10, 9) u32 over_020DBB10[2] = { 0xC19840F8, 0x000020A2 };
ncp_over(0x020DBB18, 9) u32 over_020DBB18[2] = { 0xC1D840F8, 0x000020AA };
ncp_over(0x020DBB20, 9) u32 over_020DBB20[2] = { 0xC01840F8, 0x000020B2 };
ncp_over(0x020DBB28, 9) u32 over_020DBB28[2] = { 0x805880F8, 0x000020BA };
ncp_over(0x020DBB30, 9) u32 over_020DBB30[2] = { 0x406880F8, 0x000020BC };
ncp_over(0x020DB9EC, 9) u32 over_020DB9EC[2] = { 0x01F800F8, 0xFFFF00BD };

ncp_over(0x020CD914, 1)
const u8 Boot_logoPalette[32] = {
    0x00, 0x00, 0x00, 0x7D, 0x42, 0x7D, 0x64, 0x7D, 0xC7, 0x7D, 0xE9, 0x7D, 0x0A, 0x7E, 0x4D, 0x7E,
    0x90, 0x7E, 0xF4, 0x7E, 0x37, 0x7F, 0x79, 0x7F, 0x9C, 0x7F, 0xDD, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
};

#elif GAME_LANGUAGE_PT

constexpr s32 FileSelectNumberXOffset = 10;
constexpr s32 WorldmapStarCoin1XOffset = -34;
constexpr s32 WorldmapStarCoin2XOffset = 24;
constexpr s32 WorldmapStarCoin3XOffset = -4;
constexpr s32 WorldmapStarCoin4XOffset = -4;

asm(R"""(
ncp_over(0x020341C4)
.incbin "files/font_a_pt.NFTR"
ncp_endover()
)""");

#else // GAME_LANGUAGE_EN

constexpr s32 FileSelectNumberXOffset = 0;
constexpr s32 WorldmapStarCoin1XOffset = 0;
constexpr s32 WorldmapStarCoin2XOffset = 0;
constexpr s32 WorldmapStarCoin3XOffset = 0;
constexpr s32 WorldmapStarCoin4XOffset = 0;

#endif

ncp_over(0x020DB764, 9)
GXOamAttr TitleScreen_fileSelectTextAttrs[3][6] = {
    {
		OAM::getOBJAttr(55 + FileSelectNumberXOffset, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 93, 0, 0, 0x0),
		OAM::getOBJAttr(56 + FileSelectNumberXOffset, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 93, 0, 0, 0x0),
		OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(32, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0x0),
		OAM::getOBJAttr(1, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(33, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0xFFFF)
    },
    {
		OAM::getOBJAttr(52 + FileSelectNumberXOffset, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 111, 0, 0, 0x0),
		OAM::getOBJAttr(53 + FileSelectNumberXOffset, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 111, 0, 0, 0x0),
		OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(32, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0x0),
		OAM::getOBJAttr(1, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(33, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0xFFFF)
    },
    {
		OAM::getOBJAttr(52 + FileSelectNumberXOffset, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 115, 0, 0, 0x0),
		OAM::getOBJAttr(53 + FileSelectNumberXOffset, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 115, 0, 0, 0x0),
		OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(32, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0x0),
		OAM::getOBJAttr(1, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 95, 0, 0, 0x0),
		OAM::getOBJAttr(33, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0xFFFF)
    }
};

#ifndef SKIP_COMMON_WORLDMAP_OVERRIDES

ncp_over(0x0216E9FC, 54)
GXOamAttr Worldmap_allStarcoinsSpentAttrs[] = {
	OAM::getOBJAttr(483 + WorldmapStarCoin1XOffset, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLOR_16, 42, 2, 0, 0),
	OAM::getOBJAttr(454 + WorldmapStarCoin2XOffset, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 46, 1, 0, 0),
	OAM::getOBJAttr(2 + WorldmapStarCoin3XOffset, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 48, 1, 0, 0),
	OAM::getOBJAttr(34 + WorldmapStarCoin4XOffset, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 50, 1, 0, 0),
	OAM::getOBJAttr(455 + WorldmapStarCoin2XOffset, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 52, 1, 0, 0),
	OAM::getOBJAttr(3 + WorldmapStarCoin3XOffset, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 54, 1, 0, 0),
	OAM::getOBJAttr(35 + WorldmapStarCoin4XOffset, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 56, 1, 0, 0xFFFF)
};

/*
I reversed these two but they were the wrong ones...
I'll keep them here in case they are ever needed.

ncp_over(0x0216EA34, 54)
GXOamAttr Worldmap_allStarcoinsSpentAttrs[] = {
	OAM::getOBJAttr(33, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(25, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(17, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(34, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(26, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(18, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 58, 0, 0, 0),
	OAM::getOBJAttr(1, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 58, 0, 0, 0xFFFF)
};

ncp_over(0x0216E994, 54)
GXOamAttr Worldmap_allStarcoinsSpentAttrs[] = {
	OAM::getOBJAttr(25, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(17, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(26, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(18, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 9, 0, 0, 0),
	OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 58, 0, 0, 0),
	OAM::getOBJAttr(1, 1, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 58, 0, 0, 0xFFFF)
};
*/

#endif
