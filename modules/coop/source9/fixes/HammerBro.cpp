#include "coop/CoopActor.hpp"

namespace CoopFixes::HammerBro {

ncp_over(0x021754F8, 56) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::HammerBro
