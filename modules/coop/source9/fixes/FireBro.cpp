#include "coop/CoopActor.hpp"

namespace CoopFixes::FireBro {

ncp_over(0x02175730, 56) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::FireBro
