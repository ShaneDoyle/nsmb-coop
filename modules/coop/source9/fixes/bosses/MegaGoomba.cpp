#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

namespace CoopFixes::MegaGoomba {

ncp_over(0x02133150, 14) const auto vtbl_skipRender = CoopActor::safeSkipRender;

// Unfreeze both players
ncp_set_call(0x0213137C, 14, BossControllerCommon::endCutsceneAllPlayers)

} // namespace CoopFixes::MegaGoomba
