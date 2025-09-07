#include <nsmb/game/stage/player/player.hpp>

namespace CoopPlayer {

NTR_INLINE static bool isOnFlagpole(Player* self)
{
	return self->actionFlag.flagpoleGrab;
}

void updateJumpedOnAnimation(Player* self);
void beginJumpedOnAnimation(Player* self);
void beginBossDefeatCutsceneCoop(Player* linkedPlayer, bool battleSwitch);

} // namespace CoopPlayer
