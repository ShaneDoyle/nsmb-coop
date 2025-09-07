#include "coop/CoopActor.hpp"

#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::CheepSkipper {

ncp_over(0x02132560, 18) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_set_call(0x02131748, 18, BossControllerCommon::endCutsceneAllPlayers)

} // namespace CoopFixes::CheepSkipper
