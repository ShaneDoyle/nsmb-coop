#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::Lakithunder {

ncp_over(0x02133C38, 17) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_repl(0x0213215C, 17, "NOP")
ncp_set_call(0x02132168, 17, CoopActor::isOutsideCamera)

ncp_repl(0x02132420, 17, "ADD R0, R5, #0x100")
ncp_repl(0x0213242C, 17, "LDRSB R0, [R0,#0x1E]") // Blue shell hit

ncp_repl(0x021325EC, 17, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Ground pound hit

ncp_repl(0x021329C4, 17, "ADD R0, R5, #0x100")
ncp_repl(0x021329D8, 17, "LDRSB R0, [R0,#0x1E]") // Stomp hit

ncp_set_call(0x02131554, 17, BossControllerCommon::endCutsceneAllPlayers)

} // namespace CoopFixes::Lakithunder
