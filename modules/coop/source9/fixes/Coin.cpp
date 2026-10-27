#include "coop/CoopActor.hpp"

namespace CoopFixes::Coin {

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x020D89A8, 10)
	MOV     R0, R5
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x020D89AC

ncp_jump(0x020D9D48, 10)
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x020D9D4C
)");};

ncp_repl(0x020D98DC, 10, "MOV R0, #1") // Allow coins to get killed by lava

ncp_set_call(0x020D8524, 10, CoopActor::isInRangeOfAllPlayers) // Fix coin permanent deletion

} // namespace CoopFixes::Coin
