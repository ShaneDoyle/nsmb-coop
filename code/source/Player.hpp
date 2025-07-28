#include <nsmb/game/stage/player/player.hpp>

bool Player_isOnFlagpole(Player* self);
void Player_beginBossDefeatCutsceneCoop(Player* linkedPlayer, bool battleSwitch);
bool Player_missedPoleState(Player* self, void* arg);
void Player_beginMissedPoleState(Player* self);
