#include "coop/CoopActor.hpp"

namespace CoopFixes::Broozer {

ncp_repl(0x0218A898, 108, "NOP") // Pass Broozer* instead of &Broozer*->position
ncp_set_call(0x0218A8A4, 108, CoopActor::isOutsideCamera)

ncp_repl(0x0218AAA0, 108, "LDRB R0, [R5,#0x11E]") // Fix stomp hit

} // namespace CoopFixes::Broozer
