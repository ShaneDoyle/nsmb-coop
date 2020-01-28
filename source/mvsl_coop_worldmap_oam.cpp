#include "nsmb.h"

static OAMEntry marioHeadList[] = {
	//Mario Head
	{
		OBJ_Y(0) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(0) | ATTR1_SIZE_16,
		ATTR2_PALETTE(9) | 0x4C
	},
	{
		OBJ_Y(0) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(16) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x50
	},
	{
		OBJ_Y(16) | ATTR0_WIDE | ATTR0_COLOR_16,
		OBJ_X(0) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x52
	},
	{
		OBJ_Y(16) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(16) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x54 | OAM_DATA_END
	}
};

static OAMEntry luigiHeadList[] = {
	//Luigi Head
	{
		OBJ_Y(0) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(0) | ATTR1_SIZE_16,
		ATTR2_PALETTE(9) | 0x69
	},
	{
		OBJ_Y(0) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(16) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x6D
	},
	{
		OBJ_Y(16) | ATTR0_WIDE | ATTR0_COLOR_16,
		OBJ_X(0) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x6F
	},
	{
		OBJ_Y(16) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(16) | ATTR1_SIZE_8,
		ATTR2_PALETTE(9) | 0x71 | OAM_DATA_END
	}
};

static OAMEntry livesCounterList[] = {
	//Digit 1
	{
		OBJ_Y(4) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(42) | ATTR1_SIZE_8,
		ATTR2_PALETTE(3) | 0x0
	},
	{
		OBJ_Y(5) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(43) | ATTR1_SIZE_8,
		ATTR2_PALETTE(3) | 0x0
	},
	//Digit 2
	{
		OBJ_Y(4) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(51) | ATTR1_SIZE_8,
		ATTR2_PALETTE(3) | 0x0
	},
	{
		OBJ_Y(5) | ATTR0_TALL | ATTR0_COLOR_16,
		OBJ_X(52) | ATTR1_SIZE_8,
		ATTR2_PALETTE(3) | 0x0
	},
	//Cross (X)
	{
		OBJ_Y(4) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(24) | ATTR1_SIZE_16,
		ATTR2_PALETTE(3) | 0x48
	},
	{
		OBJ_Y(5) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(25) | ATTR1_SIZE_16,
		ATTR2_PALETTE(3) | 0x48
	},
	//Little Background
	{
		OBJ_Y(4) | ATTR0_WIDE | ATTR0_COLOR_16,
		OBJ_X(-4) | ATTR1_SIZE_32,
		ATTR2_PALETTE(3) | 0x55
	},
	{
		OBJ_Y(4) | ATTR0_WIDE | ATTR0_COLOR_16,
		OBJ_X(20) | ATTR1_SIZE_32,
		ATTR2_PALETTE(3) | 0x5D
	},
	{
		OBJ_Y(4) | ATTR0_SQUARE | ATTR0_COLOR_16,
		OBJ_X(52) | ATTR1_SIZE_16,
		ATTR2_PALETTE(3) | 0x65 | OAM_DATA_END
	}
};

void hook_020D0C38_ov_08() //Draw Mario's lives counter
{
	OAM_DrawHUDSub((OAMEntry*)marioHeadList, 110, 4, 0, 0, 0, 0, 0, 0, 0);
	OAM_DrawHUDSub((OAMEntry*)luigiHeadList, 182, 4, 0, 0, 0, 0, 0, 0, 0);
	for (int i = 0; i < 2; i++)
	{
		OAMEntry* oam_digit_table = (OAMEntry*)&livesCounterList;
		int lives = GetLivesForPlayer(i);

		if (lives < 0)
			lives = 0;
		else if (lives > 99)
			lives = 99;

		oam_digit_table[0].attr2 &= ~0x3FF;
		oam_digit_table[1].attr2 &= ~0x3FF;
		oam_digit_table[2].attr2 &= ~0x3FF;
		oam_digit_table[3].attr2 &= ~0x3FF;

		oam_digit_table[0].attr2 |= (lives / 10) * 2;
		oam_digit_table[1].attr2 |= (lives / 10) * 2;
		oam_digit_table[2].attr2 |= (lives % 10) * 2;
		oam_digit_table[3].attr2 |= (lives % 10) * 2;

		if (lives < 10)
			oam_digit_table += 2; //Do not draw digit 1

		OAM_DrawHUDSub(oam_digit_table, 110 + (72 * i), 4, 0, 0, 0, 0, 0, 0, 0);
	}
}

void nsub_020D0B58_ov_08() { asm("B 0x020D0C38"); }