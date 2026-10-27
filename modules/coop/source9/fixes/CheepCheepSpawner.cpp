#include "coop/CoopActor.hpp"

namespace CoopFixes::CheepCheepSpawner {

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x0213CBC8, 25)
	MOV     R0, R5
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x0213CBCC
)");};

}
