#include "coop/CoopActor.hpp"

namespace CoopFixes::BoomerangBro {

ncp_over(0x02175614, 56) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::BoomerangBro
