#include <nsmb_nitro.hpp>

#include "coop/Coop.hpp"

class Player;
class StageEntity;

namespace CoopFixes::Flagpole {

extern StageEntity* instance;
extern u8 playerOrdinals[Coop::MaxPlayerCount];
extern u8 waitPlayerCountdown;

void getPlayersGrabbing(u32* polePlayerCount, ::Player** polePlayers, u32* notPolePlayerCount, ::Player** notPolePlayers, bool* allGrabbing);
void switchToPlayerSlideState(StageEntity* self);
bool allPlayersSlidingPole();
fx32 getAccumulatedHeightsBelow(::Player* targetPlayer);

} // namespace CoopFixes::Flagpole
