#include "coop/CoopActor.hpp"

namespace CoopFixes::RotatingWallPlatform {

ncp_over(0x0218FF54, 118) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::RotatingWallPlatform
