#pragma once

#include <nsmb_nitro.hpp>
#include <nsmb/game/stage/entity3d.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/core/math.hpp>

namespace CoopActor {

constexpr u8 NO_MATCH = 0xFF;

NTR_INLINE static bool isPlayerInZone(Player* player, u32 zoneID)
{
	// Force R0 to remain undefined as it is unused
    register StageEntity* r0 asm("r0");
	return r0->isPlayerInZone(*player, zoneID);
}

bool safeSkipRender(StageEntity3DAnm* self);
Player* getClosestPlayer(StageActor* self);
bool isOutsideCamera(StageActor* self, const FxRect& boundingBox/*, u8 playerID*/);
Player* getClosestPlayerInZone(StageActor* self, u32 zoneID);
u8 getHorizontalDirectionToPlayer(StageEntity* self, const Vec3& position, u32 playerID);
bool isPlayerInRange(StageActor* self, Player* player);
bool isInRangeOfAllPlayers(StageEntity* self);

} // namespace CoopActor
