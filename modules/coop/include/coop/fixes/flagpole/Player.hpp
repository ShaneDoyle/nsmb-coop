#include "coop/Coop.hpp"

class Player;
class StageEntity;

namespace CoopFixes::Player {

void beginMissedPoleState(::Player* self);
void beginSentFlyingAwayWithPoleState(::Player* self);

} // namespace CoopFixes::Player
