#include "coop/CoopActor.hpp"

namespace CoopFixes::Boo {

ncp_over(0x021793EC, 71) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_repl(0x0217716C, 71, "NOP") // Pass Boo* instead of &Boo*->position
ncp_set_call(0x02177178, 71, CoopActor::isOutsideCamera)

} // namespace CoopFixes::Boo
