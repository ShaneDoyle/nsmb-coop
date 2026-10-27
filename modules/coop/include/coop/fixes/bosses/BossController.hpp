#include <nsmb_nitro.hpp>

class StageEntity3DAnm;

namespace CoopFixes::BossController {

struct PTMF
{
	bool (*func)(StageEntity3DAnm*);
	u32 adj;
};

extern PTMF sCustomTransition;

bool coopTransitionState(StageEntity3DAnm* self);

}
