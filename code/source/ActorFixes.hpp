#pragma once

#include "nitro/types.h"

class Player;
class StageEntity;
class StageEntity3DAnm;
struct FxRect;

bool ActorFixes_safeSkipRender(StageEntity3DAnm* self);
Player* ActorFixes_getClosestPlayer(StageEntity* self);
bool ActorFixes_isOutsideCamera(StageEntity* self, const FxRect& boundingBox, u8 playerID);
Player* ActorFixes_getClosestPlayerInZone(StageEntity* self, u32 zoneID);
