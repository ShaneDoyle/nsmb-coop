#include "nsmb/stage/entity.hpp"

#include "ActorFixes.hpp"

// Unagi Eel ----------------------------------------------------------------------------

ncp_repl(0x02179A20, 79, "MOV R0, R4") // Pass Unagi* instead of &Unagi*->position
ncp_set_call(0x02179A44, 79, ActorFixes_safeSkipRender)
ncp_set_call(0x02179A28, 79, ActorFixes_isOutsideCamera)

asm(R"(
ncp_call(0x0217ABA4, 79)
ncp_call(0x0217AEC4, 79)
	MOV     R0, R4
	B       _Z27ActorFixes_getClosestPlayerP11StageEntity

ncp_jump(0x0217B8AC, 79) // Mega bump fix
	PUSH    {R4,LR}
	MOV     R4, R0
	B       0x0217B8B0
)");

ncp_repl(0x0217B900, 79, "POP {R4,PC}") // Mega bump fix

ncp_repl(0x0217B8C8, 79, "ADD R0, R4, #0x100") // Mega bump fix
ncp_repl(0x0217B8D0, 79, "LDRSB R0, [R0,#0x1E]") // Mega bump fix
