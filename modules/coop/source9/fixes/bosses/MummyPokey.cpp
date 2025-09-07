#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::MummyPokey {

ncp_over(0x02133AF0, 16) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_set_call(0x02131EDC, 16, BossControllerCommon::endCutsceneAllPlayers)

ncp_repl(0x0213298C, 16, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Fix ground pound hit

ncp_repl(0x021327EC, 16, "ADD R0, R5, #0x100") // Fix shell hit
ncp_repl(0x021327FC, 16, "LDRSB R0, [R0,#0x1E]") // Fix shell hit

ncp_repl(0x02132E88, 16, "ADD R0, R5, #0x100") // Fix stomp hit
ncp_repl(0x02132E9C, 16, "LDRSB R0, [R0,#0x1E]") // Fix stomp hit

} // namespace CoopFixes::MummyPokey
