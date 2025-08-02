#pragma once

#include <nsmb_nitro.hpp>
#include <nsmb/game/stage/entity3danm.hpp>

class Player;
struct FxRect;

bool ActorFixes_safeSkipRender(StageEntity3DAnm* self);
Player* ActorFixes_getClosestPlayer(StageActor* self);
bool ActorFixes_isOutsideCamera(StageActor* self, const FxRect& boundingBox/*, u8 playerID*/);
Player* ActorFixes_getClosestPlayerInZone(StageActor* self, u32 zoneID);

NTR_INLINE bool ActorFixes_isPlayerInZone(Player* player, u32 zoneID)
{
	return rcast<StageEntity*>(0)->isPlayerInZone(*player, zoneID);
}

bool ActorFixes_isPlayerInShakeRange(StageActor* self, Player* player);
bool ActorFixes_isInRangeOfAllPlayers(StageEntity* self);

constexpr u16 ActorFixes_volcanoTimerInterval = 60 * 8;
extern u16 ActorFixes_volcanoTimer;
extern u8 ActorFixes_volcanoTargetPlayer;

void ActorFixes_updateVolcanoBackground();
