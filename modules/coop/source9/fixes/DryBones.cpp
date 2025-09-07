#include "coop/CoopActor.hpp"

namespace CoopFixes::DryBones {

ncp_over(0x0213F470, 24) const auto vtbl_skipRender = CoopActor::safeSkipRender;

} // namespace CoopFixes::DryBones
