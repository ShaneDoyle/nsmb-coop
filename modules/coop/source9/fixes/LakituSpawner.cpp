#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/actors/ov66/lakituspawner.hpp>

namespace CoopFixes::LakituSpawner {

bool customTargetAvailable(::LakituSpawner* self, Player* player)
{
	return !Game::getPlayerDead(player->linkedPlayerID) && self->targetAvailable(player);
}

ncp_set_call(0x02175BD8, 66, customTargetAvailable)
ncp_set_call(0x02175C4C, 66, customTargetAvailable)
ncp_set_call(0x02175C6C, 66, customTargetAvailable)
ncp_set_call(0x02175D0C, 66, customTargetAvailable)
ncp_set_call(0x02175D40, 66, customTargetAvailable)

} // namespace CoopFixes::LakituSpawner
