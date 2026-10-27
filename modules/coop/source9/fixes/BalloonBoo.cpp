#include "coop/CoopActor.hpp"

namespace CoopFixes::BalloonBoo {

ncp_over(0x02179508, 71) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::BalloonBoo
