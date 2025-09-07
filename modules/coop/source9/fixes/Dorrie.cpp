#include "coop/CoopActor.hpp"

namespace CoopFixes::Dorrie {

ncp_over(0x02148348, 47) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::Dorrie
