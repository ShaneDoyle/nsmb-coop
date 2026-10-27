#include "coop/CoopActor.hpp"

namespace CoopFixes::Item {

ncp_repl(0x020D24EC, 10, "MOV R2, #0")

// Give the item to both players
ncp_asmfunc void fixToadhouse_ASM()
{asm(R"(
ncp_jump(0x020D2520, 10)
	MOV     R5, #0xFFFFFFFF
item_loop_start:
	ADD     R5, R5, #1
	BL      _ZN4Game14getPlayerCountEv
	CMP     R5, R0
	MOVEQ   R0, R4 // Keep replaced instruction
	BEQ     0x020D259C // Return to destroy
	B       0x020D2524

ncp_over(0x020D2598, 10)
	B       item_loop_start
ncp_endover()
)");}

ncp_set_call(0x020D0D3C, 10, CoopActor::megaRewardSpawnActor)

} // namespace CoopFixes::Item
