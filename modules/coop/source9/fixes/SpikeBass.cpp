#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>

#include "coop/CoopActor.hpp"

namespace CoopFixes::SpikeBass {

#if COOP_FIX_SPIKE_BASS_ADD_ZONE_ID_FIELD
ncp_repl(0x021731B4, 58, ".int 0x4BC") // Add a zoneID field
#endif

Player* customGetClosestPlayer(StageEntity* self)
{
	u32 zoneID = rcast<u32*>(self)[COOP_FIX_SPIKE_BASS_ZONE_ID_FIELD_OFFSET / 4];
	Player* player = CoopActor::getClosestPlayerInZone(self, zoneID);
	return player ? player : CoopActor::getClosestPlayer(self);
}

ncp_set_call(0x02172CB0, 58, customGetClosestPlayer)
ncp_set_call(0x02172E4C, 58, customGetClosestPlayer)

} // namespace CoopFixes::SpikeBass
