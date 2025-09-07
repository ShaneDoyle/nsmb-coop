#include <nsmb_nitro.hpp>

class StageEntity;

namespace CoopFixes::FinalBossController {

struct PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

extern PTMF sCustomTransition;

bool coopTransitionState(StageEntity* self);

}
