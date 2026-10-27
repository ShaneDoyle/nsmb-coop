#include "coop/CoopActor.hpp"

namespace CoopFixes::HammerBroSpawnPoint {

ncp_over(0x0216E1D4, 54) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::HammerBroSpawnPoint
