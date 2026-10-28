#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::MegaGoomba {

ncp_over(0x02133150, 14) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_repl(0x021320F4, 14, "ADD R1, R0, #0x100")
ncp_repl(0x021320FC, 14, "LDRSB R0, [R1,#0x1E]") // Mega hit

ncp_repl(0x0212FCC8, 14, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]")

// Initialize the last linked player as 0 to not crash
ncp_call(0x0212FD9C, 14)
ncp_asmfunc void call_0212FD9C_ov14()
{asm(R"(
	MOV     R1, #0
	STRB    R1, [R4,#0x11E]
	B       0x021320F0 // (ov14) MegaGoomba::updateActiveCollider
)");}

// Unfreeze both players
ncp_set_call(0x0213137C, 14, BossControllerCommon::endCutsceneAllPlayers)

} // namespace CoopFixes::MegaGoomba
