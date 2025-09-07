#include "coop/fixes/bosses/FinalBossController.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/core/filesystem/cache.hpp>

#include "coop/CoopActor.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

extern "C" {
	void CoopSym_FinalBossController_switchState(StageEntity* self, CoopFixes::FinalBossController::PTMF ptmf);
	extern CoopFixes::FinalBossController::PTMF CoopSym_FinalBossController_sTransition;
}

namespace CoopFixes::FinalBossController {

ncp_over(0x02148B70, 43) const auto vtbl_skipRender = CoopActor::safeSkipRender;

PTMF sCustomTransition = { nullptr, 0 }; // TODO: move this outside so this module can be in an overlay

ncp_over(0x02148574, 43)
const static PTMF* sCustomTransition_ptr = &sCustomTransition;

BossControllerCommon::CoopTransitionStateInfo coopTransitionStateInfo =
{
	.getZoneX = [](StageEntity* self)
	{
		Rectangle<fx32> zoneBox;
		StageZone::get(0, &zoneBox);
		return zoneBox.x;
	},

	.exitState = [](StageEntity* self)
	{
		CoopSym_FinalBossController_switchState(self, CoopSym_FinalBossController_sTransition);
	},

	.commonEnd = [](StageEntity* self)
	{

	},
};

bool coopTransitionState(StageEntity* self)
{
	s16& step = rcast<s16*>(self)[0xAC0/2];
	s32 stepArg = step;
	bool result = BossControllerCommon::coopTransitionState(self, stepArg, &coopTransitionStateInfo);
	step = stepArg;
	return result;
}

// Setup the cutscene transition
ncp_set_call(0x021482AC, 43, BossControllerCommon::setupCoopTransitionState)

// For coop binding the camera bounds gets handled by coopTransitionState
ncp_jump(0x021482F0, 43)
ncp_asmfunc void jump_021482F0_ov43()
{asm(R"(
	BL      _ZN9StageZone3getEhP9RectangleIlE // Keep replaced instruction
	LDR     R0, =_ZN4Game11playerCountE
	LDR     R0, [R0]
	CMP     R0, #1
	BNE     0x02148330
	B       0x021482F4
)");}

} // namespace CoopFixes::FinalBossController
