#include <nsmb_nitro.hpp>
#include <nsmb/game/stage.hpp>

#include "coop/actors/CoopFlagActor.hpp"

namespace Coop::WorldUnlock {

// Boss ----------------

ncp_repl(0x02144DCC, 40, "B 0x02144E00") // Do not load world signs
ncp_repl(0x02144D0C, 40, "B 0x02144D88") // Do not setup world signs
ncp_repl(0x02143D8C, 40, "B 0x02143E0C") // Do not render world signs

ncp_repl(0x02144AF8, 40, "B 0x02144B84") // Skip world sign effects

void switchToCutsceneArea(u8 stage)
{
	Entrance::targetAreaID = Stage::getAreaID(9, stage, 0);
	Entrance::targetEntranceID = 0;
	Entrance::switchArea();
}

ncp_call(0x020A1D00, 0)
ncp_thumb void levelEnd_hook(Stage::ExitFlag flag)
{
	if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::EndLevelToW2UnlockCutscene))
	{
		switchToCutsceneArea(0);
	}
	else if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::EndLevelToW5UnlockCutscene))
	{
		switchToCutsceneArea(1);
	}
	else
	{
		Stage::exitLevel(flag);
	}
}

// Demo ----------------

static u8 goToMiniWorld = false;
static u32 getGoToMiniWorld() { return goToMiniWorld; }

ncp_repl(0x020CE2E0, 8, "CMP R0, #1")
ncp_repl(0x020CE300, 8, "CMP R0, #1")
ncp_set_call(0x020CE2A8, 8, getGoToMiniWorld)

ncp_call(0x02119684, 10)
ncp_thumb void finishLevelOnTransit_hook()
{
	if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::WorldUnlockCutscene))
	{
		goToMiniWorld = Entrance::targetEntranceID == 2;
		Stage::exitLevel(Stage::ExitFlag::LevelBeaten);
		return;
	}
	Entrance::switchArea();
}

} // namespace Coop::WorldUnlock
