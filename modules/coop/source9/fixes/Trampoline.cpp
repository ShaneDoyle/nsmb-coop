#include "coop/CoopActor.hpp"

namespace CoopFixes::Trampoline {

ncp_over(0x0216D1BC, 54) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::Trampoline
