#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>
#include <nsmb/core/wifi.hpp>

static GXOamAttr marioHeadList[] = {
	// Mario Head
	OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 0x4C, 9, 0, 0x0),
	OAM::getOBJAttr(16, 0, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0x50, 9, 0, 0x0),
	OAM::getOBJAttr(0, 16, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x8, GX_OAM_COLOR_16, 0x52, 9, 0, 0x0),
	OAM::getOBJAttr(16, 16, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 0x54, 9, 0, 0xFFFF)
};

static GXOamAttr luigiHeadList[] = {
	// Luigi Head
	OAM::getOBJAttr(0, 0, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 0x69, 9, 0, 0x0),
	OAM::getOBJAttr(16, 0, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0x6D, 9, 0, 0x0),
	OAM::getOBJAttr(0, 16, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x8, GX_OAM_COLOR_16, 0x6F, 9, 0, 0x0),
	OAM::getOBJAttr(16, 16, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x8, GX_OAM_COLOR_16, 0x71, 9, 0, 0xFFFF)
};

static GXOamAttr livesCounterList[] = {
	// Digit 1
	OAM::getOBJAttr(42, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0, 3, 0, 0x0),
	OAM::getOBJAttr(43, 5, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0, 3, 0, 0x0),
	// Digit 2
	OAM::getOBJAttr(51, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0, 3, 0, 0x0),
	OAM::getOBJAttr(52, 5, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_8x16, GX_OAM_COLOR_16, 0, 3, 0, 0x0),
	// Cross (X)
	OAM::getOBJAttr(24, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 0x48, 3, 0, 0x0),
	OAM::getOBJAttr(25, 5, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 0x48, 3, 0, 0x0),
	// Little Background
	OAM::getOBJAttr(508, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 0x55, 3, 0, 0x0),
	OAM::getOBJAttr(20, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 0x5D, 3, 0, 0x0),
	OAM::getOBJAttr(52, 4, 0, GX_OAM_MODE_NORMAL, 0, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 0x65, 3, 0, 0xFFFF)
};

NTR_USED static void Worldmap_drawCustomLivesCounter()
{
	OAM::drawSub(marioHeadList, 110, 4, OAM::Flags::None, 0, 0, OAM::Settings::None);
	OAM::drawSub(luigiHeadList, 182, 4, OAM::Flags::None, 0, 0, OAM::Settings::None);

	for (u32 i = 0; i < 2; i++)
	{
		GXOamAttr* oamDigitTable = livesCounterList;
		s32 lives = Game::getPlayerLives(i);

		if (lives < 0)
			lives = 0;
		else if (lives > 99)
			lives = 99;

		oamDigitTable[2].charNo = (lives % 10) * 2;
		oamDigitTable[3].charNo = (lives % 10) * 2;

		if (lives < 10)
		{
			oamDigitTable += 2; // Do not draw digit 1
		}
		else
		{
			oamDigitTable[0].charNo = (lives / 10) * 2;
			oamDigitTable[1].charNo = (lives / 10) * 2;
		}

		OAM::drawSub(oamDigitTable, 110 + (72 * i), 4, OAM::Flags::None, 0, 0, OAM::Settings::None);
	}
}

asm(R"(
ncp_jump(0x020D0B58, 8)
	LDR     R0, =_ZN4Wifi12consoleCountE
	LDR     R0, [R0]
	CMP     R0, #1
	BLNE    _ZL31Worldmap_drawCustomLivesCounterv
	BNE     0x020D0C38
	MOV     R0, R7
	B       0x020D0B5C
)");

struct UI_DrawInfo {
	s16 x;
	s16 y;
	u32 objectID;
};

asm(R"(
	UI_draw = 0x020042D8
)");
extern "C" {
	void UI_draw(const UI_DrawInfo* drawInfo, const UI_DrawInfo* idkDrawInfo, bool subScreen, const Vec2* scale, s16 rot, int a6, int a7, u8 palette, u8 flags);
}

ncp_call(0x020D0AB8, 8)
void Worldmap_drawCustomCompletedIcon(const UI_DrawInfo* drawInfo, const UI_DrawInfo* idkDrawInfo, bool subScreen, const Vec2* scale, s16 rot, int a6, int a7, u8 palette, u8 flags)
{
	if (Wifi::getCommunicatingConsoleCount() == 1)
	{
		UI_draw(drawInfo, idkDrawInfo, subScreen, scale, rot, a6, a7, palette, flags);
		return;
	}

	UI_DrawInfo customDrawInfo = { 10, 97, drawInfo->objectID };
	UI_draw(&customDrawInfo, idkDrawInfo, subScreen, scale, rot, a6, a7, palette, flags);
}
