#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/game/ui.hpp>
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

ncp_asmfunc void Worldmap_drawCustomLivesCounter_ASM()
{asm(R"(
ncp_jump(0x020D0B58, 8)
	LDR     R0, =_ZN4Wifi25communicatingConsoleCountE
	LDR     R0, [R0]
	CMP     R0, #1
	MOVEQ   R0, R7
	BEQ     0x020D0B5C
	BL      _ZL31Worldmap_drawCustomLivesCounterv
	B       0x020D0C38
)");}

ncp_call(0x020D0AB8, 8)
void Worldmap_drawCustomCompletedIcon(BNCL::Object* object, BNCL::Object* maybeBgObj, bool subScreen, const Vec2* scale, s16 rot, u32 xOffset, u32 yOffset, u8 palette, OAM::Flags flags)
{
	if (Wifi::getCommunicatingConsoleCount() == 1)
	{
		UI::drawObject(object, maybeBgObj, subScreen, scale, rot, xOffset, yOffset, palette, flags);
		return;
	}

	BNCL::Object customDrawInfo;
	customDrawInfo.x.raw = 10;
	customDrawInfo.y.raw = 97;
	customDrawInfo.bncdObjectID = object->bncdObjectID;
	UI::drawObject(&customDrawInfo, maybeBgObj, subScreen, scale, rot, xOffset, yOffset, palette, flags);
}
