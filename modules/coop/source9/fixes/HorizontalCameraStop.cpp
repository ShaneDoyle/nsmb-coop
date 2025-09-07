#include <nsmb/game/stage/entity.hpp>

#include "coop/CoopStage.hpp"

namespace CoopFixes::HorizontalCameraStop {

ncp_asmfunc s32 HorizontalCameraStop_onCreate_SUPER(StageEntity* self)
{asm(R"(
	PUSH    {LR}
	B       0x020D6604
)");}

ncp_jump(0x020D6600, 10)
s32 HorizontalCameraStop_onCreate_OVERRIDE(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		CoopStage::tempVar = playerID;
		HorizontalCameraStop_onCreate_SUPER(self);
	}

	return 1;
}

ncp_repl(0x020D6784, 10, ".int _ZN9CoopStage7tempVarE")

} // namespace CoopFixes::HorizontalCameraStop
