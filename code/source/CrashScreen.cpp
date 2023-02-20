#include "nitro_if.h"

#define DEBUG_WHITE 0xD000
#define DEBUG_YELLOW 0xE000
#define DEBUG_AQUA 0xF000

#define SCR_POS(scrn, x, y) (scrn + 32 * y + x)

asm(R"(
	debug_printf = 0x02006370
	debug_clear = 0x02005E68
	debug_drawTop = 0x02005EB0
	debug_drawBottom = 0x020061E4
)");
extern "C"
{
	void debug_printf(u16 colors[2], u16* dst, const char* str, ...);
	void debug_clear();
	void debug_drawTop();
	void debug_drawBottom();
}

namespace CrashScreen {

u16 isOpen = true;
u16 lastIsOpen = true;

static u16 colorWhite[2] = { DEBUG_WHITE, DEBUG_WHITE };
static u16 colorYellow[2] = { DEBUG_WHITE, DEBUG_YELLOW };
static u16 colorAqua[2] = { DEBUG_WHITE, DEBUG_AQUA };

void drawDebugScreen()
{
	debug_drawTop();
	debug_drawBottom();
}

void create()
{
	// not sure if this actually helps
	for (int i = 0; i < 4; i++)
		MI_StopDma(i);
}

void update()
{
	if (CARD_IsPulledOut())
	{
		drawDebugScreen();
		return;
	}

	u16 input = PAD_Read();

	if (input & PAD_BUTTON_START)
	{
		isOpen = true;
	}
	else if (input & PAD_BUTTON_SELECT)
	{
		isOpen = false;
	}
	else if ((input & PAD_BUTTON_L) && (input & PAD_BUTTON_R))
	{
		OS_EnableIrq();
		OS_EnableInterrupts();
		OS_ResetSystem(0);
		return;
	}

	if (lastIsOpen != isOpen)
	{
		lastIsOpen = isOpen;
		debug_clear();
	}

	if (!isOpen)
	{
		drawDebugScreen();
		return;
	}

	u16* topScrn = *rcast<u16**>(0x020859EC);
	u16* btmScrn = *rcast<u16**>(0x020859E8);

	char* buildTime = rcast<char*>(0x02088BB4);

	debug_printf(colorAqua, SCR_POS(topScrn, 5, 3), "The game has crashed");
	debug_printf(colorWhite, SCR_POS(topScrn, 2, 6), "Please contact the dev team");
	debug_printf(colorWhite, SCR_POS(topScrn, 4, 7), "Send them the crash dump");
	debug_printf(colorWhite, SCR_POS(topScrn, 7, 10), "Discord contacts:");
	debug_printf(colorWhite, SCR_POS(topScrn, 10, 12), "Shane#3754");
	debug_printf(colorWhite, SCR_POS(topScrn, 6, 14), "TheGameratorT#1850");
	debug_printf(colorYellow, SCR_POS(topScrn, 4, 19), "(`-`) kinda lonely here");

	debug_printf(colorAqua, SCR_POS(btmScrn, 2, 4), "Possible actions:");
	debug_printf(colorWhite, SCR_POS(btmScrn, 4, 7), "START  > This screen");
	debug_printf(colorWhite, SCR_POS(btmScrn, 4, 9), "SELECT > Crash dump");
	debug_printf(colorWhite, SCR_POS(btmScrn, 4, 11), "L + R  > Restart");

	// debug_printf(colorAqua, SCR_POS(btmScrn, 2, 14), "GitHub:");
	// debug_printf(colorAqua, SCR_POS(btmScrn, 2, 16), "NSMBHD:");
	// debug_printf(colorWhite, SCR_POS(btmScrn, 10, 14), "link1");
	// debug_printf(colorWhite, SCR_POS(btmScrn, 10, 16), "link2");

	debug_printf(colorYellow, SCR_POS(btmScrn, 1, 22), buildTime);
}

}

ncp_repl(0x0200460C, "NOP") // Do not disconnect the cartridge

ncp_repl(0x02005DD8, "MOV R2, #0x31") // Background color
ncp_repl(0x0203985C, ".short 0x037F") // Yellow font color
ncp_repl(0x0203987C, ".short 0x7F8F") // Aqua font color

ncp_repl(0x02005BA8, "MOV R5, #5; B 0x02005C1C") // Auto-open debug screen on crash

ncp_repl(0x02005C20, "NOP") // Do not run draw top twice
ncp_repl(0x02005C2C, "BL _ZN11CrashScreen6updateEv; NOP") // Hook our updater

asm(R"(
ncp_jump(0x02005B24)
	STRH    R0, [R2]
	BL      _ZN11CrashScreen6createEv
	B       0x02005B28
)");

/*ncp_hook(0x020CDB2C, 9)
void forceCrash()
{
	OS_Terminate();
}*/
