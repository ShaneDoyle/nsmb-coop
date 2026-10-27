#include <nsmb/game/stage/actors/ov54/warpentrance.hpp>

#include "coop/CoopStage.hpp"

namespace CoopFixes::WarpEntrance {

// Fix the warp entrance not working after one usage

void customWarpPlayer(::WarpEntrance* self, Player* player)
{
	self->warpPlayer(player);

	self->warpCountdown = 0;
	self->warpState = ::WarpEntrance::WarpState::None;
	self->warpTriggered = 0;
}

ncp_set_call(0x02156258, 54, customWarpPlayer)
ncp_set_call(0x02156350, 54, customWarpPlayer)

// Disable warp entrance if someone reached flagpole

bool customMainState(::WarpEntrance* self)
{
	if (CoopStage::isFlagpoleGrabbed())
		return true;

	return self->mainState();
}

ncp_over(0x0216D27C, 54) const auto vtbl_mainState = customMainState;

} // namespace CoopFixes::WarpEntrance
