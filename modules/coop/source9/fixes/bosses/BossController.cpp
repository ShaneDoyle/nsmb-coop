#include "coop/fixes/bosses/BossController.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/player/door.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/graphics/3d/modelanm.hpp>
#include <nsmb/core/graphics/3d/blendmodelanm.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "coop/CoopActor.hpp"
#include "coop/CoopStage.hpp"
#include "coop/CoopPlayer.hpp"
#include "coop/CoopStage.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

extern "C" {
	void CoopSym_BossController_bindCameraToZone(StageEntity3DAnm* self);
	void CoopSym_BossController_switchState(StageEntity3DAnm* self, CoopFixes::BossController::PTMF* ptmf);
	extern CoopFixes::BossController::PTMF CoopSym_BossController_sTransition;
}

namespace CoopFixes::BossController {

PTMF sCustomTransition = { nullptr, 0 };

ncp_over(0x02143994, 40)
const static PTMF* sCustomTransition_ptr = &sCustomTransition;

BossControllerCommon::CoopTransitionStateInfo coopTransitionStateInfo =
{
	.getZoneX = [](StageEntity* self)
	{
		return rcast<fx32*>(self)[0x514/4];
	},

	.exitState = [](StageEntity* self)
	{
		CoopSym_BossController_switchState(scast<StageEntity3DAnm*>(self), &CoopSym_BossController_sTransition);
	},

	.commonEnd = [](StageEntity* self)
	{
		scast<StageEntity3DAnm*>(self)->model.frameController.update();
	}
};

bool coopTransitionState(StageEntity3DAnm* self)
{
	s8& step = rcast<s8*>(self)[0x53A];
	s32 stepArg = step;
	bool result = BossControllerCommon::coopTransitionState(self, stepArg, &coopTransitionStateInfo);
	step = stepArg;
	return result;
}

// Setup the cutscene transition
ncp_set_call(0x021438AC, 40, BossControllerCommon::setupCoopTransitionState)

ncp_call(0x0214393C, 40)
ncp_thumb void customSetCameraBoundAll(StageLayout* self, fx32 bound, u32 side)
{
	if (Game::getPlayerCount() == 1)
		CoopStage::setCameraBoundAll(self, bound, side);

	// For coop it gets handled by coopTransitionState
}

ncp_call(0x02142A38, 40)
ncp_thumb void customBindCameraToZone(StageEntity3DAnm* self)
{
	if (Game::getPlayerCount() == 1)
		CoopSym_BossController_bindCameraToZone(self);

	// For coop it gets handled by the unfreezing of each actor
}

ncp_repl(0x0214310C, 40, "MOV R0, R4")
ncp_set_call(0x02143120, 40, CoopActor::isOutsideCamera)

} // namespace CoopFixes::BossController
