#pragma once

#include "nsmb_nitro.hpp"

class Player;
class StageEntity;
class StageEntity3DAnm;
struct FxRect;

bool ActorFixes_safeSkipRender(StageEntity3DAnm* self);
Player* ActorFixes_getClosestPlayer(StageEntity* self);
bool ActorFixes_isOutsideCamera(StageEntity* self, const FxRect& boundingBox, u8 playerID);
Player* ActorFixes_getClosestPlayerInZone(StageEntity* self, u32 zoneID);

NTR_INLINE bool ActorFixes_isPlayerInZone(Player* player, u32 zoneID)
{
	return rcast<StageEntity*>(0)->isPlayerInZone(*player, zoneID);
}
