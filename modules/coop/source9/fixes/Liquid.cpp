#include <nsmb/game/stage.hpp>

namespace CoopFixes::Liquid {

ncp_repl(0x020BBE88, 0, "NOP") // Prevent liquid type change on respawn

ncp_asmfunc void localPlayerFix_ASM()
{asm(R"(
ncp_call(0x020FE2D8, 10)
ncp_call(0x020FEB68, 10)
ncp_call(0x021033EC, 10)
ncp_call(0x0210E6AC, 10)
	LDR     R1, =_ZN4Game13localPlayerIDE
	LDR     R1, [R1]
	LDRB    R0, [R0,R1]
	BX      LR

// There is a non-local player access at 0x020A6EA4, but the code never reaches it
// Just in case, make it return -1 even if a playerID is specified
ncp_over(0x020A6E9C, 0)
	MOV     R0, #1
	NOP
	NOP
ncp_endover()
)");};

// Restore multiplayer lava rising logic
ncp_repl(0x02165170, 54, "CMP R2, #1")
ncp_repl(0x02165274, 54, ".int _ZN4Game11playerCountE")

// Do not use Game::localPlayerID for lava rising
ncp_asmfunc void updateRising_localPlayerFix_ASM()
{asm(R"(
ncp_over(0x021651B4, 54)
	LDRB    R2, [R2]
	LDR     R3, [R3]
ncp_endover()
)");};

ncp_repl(0x020BC224, 0, "NOP") // Do not reset liquid position on player setup
ncp_repl(0x020BC22C, 0, "NOP") // Do not reset liquid position on player setup

void reset()
{
	// Reset liquid on area/level load
	Stage::liquidPosition[Game::localPlayerID] = -0x1000000;
	Stage::lastLiquidPosition[Game::localPlayerID] = -0x1000000;
}

} // namespace CoopFixes::Liquid
