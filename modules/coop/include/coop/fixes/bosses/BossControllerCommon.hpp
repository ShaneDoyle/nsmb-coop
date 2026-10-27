#include <nsmb_nitro.hpp>

class Player;
class StageEntity;

namespace CoopFixes::BossControllerCommon {

struct CoopTransitionStateInfo
{
	fx32(*getZoneX)(StageEntity*);
	void(*exitState)(StageEntity*);
	void(*commonEnd)(StageEntity*);
};

bool coopTransitionState(StageEntity* self, s32& step, CoopTransitionStateInfo* info);
void setupCoopTransitionState(Player* closestPlayer);

void beginCutsceneAllPlayers();
void endCutsceneAllPlayers();

}
