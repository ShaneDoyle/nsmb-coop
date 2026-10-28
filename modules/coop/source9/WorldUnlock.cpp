#include <nsmb_nitro.hpp>
#include <nsmb/game/stage.hpp>

#include "glue/level.hpp"
#include "fid.hpp"

extern "C" {
	bool CoopSym_LoadBGDatFile(u32 areaID, void* dest);
}

namespace Coop::WorldUnlock {

// Boss ----------------

ncp_repl(0x02144DCC, 40, "B 0x02144E00") // Do not load world signs
ncp_repl(0x02144D0C, 40, "B 0x02144D88") // Do not setup world signs
ncp_repl(0x02143D8C, 40, "B 0x02143E0C") // Do not render world signs

ncp_repl(0x02144AF8, 40, "B 0x02144B84") // Skip world sign effects

// The cutscene areas take the area slots of the first two MvsL levels, but have
// files of their own. The game looks an area's files up in these tables, so the
// slot points at the cutscene files only while that area loads. An entry is an
// extended file ID; MvsL sets its upper bits to pick the level out of an archive.
static u32* const courseFileIDs = rcast<u32*>(0x0203CC44);
static u32* const bgdatFileIDs = rcast<u32*>(0x0203C960);

static constexpr u32 cutsceneCourseFileIDs[] = {
	"coop/W2UnlockCutscene.bin"fid,
	"coop/W5UnlockCutscene.bin"fid
};
static constexpr u32 cutsceneBgdatFileIDs[] = {
	"coop/W2UnlockCutscene_bgdat.bin"fid,
	"coop/W5UnlockCutscene_bgdat.bin"fid
};

static u32 redirectedAreaID = -1;
static u32 originalCourseFileID;
static u32 originalBgdatFileID;

static void restoreCutsceneAreaFiles()
{
	if (redirectedAreaID == u32(-1))
		return;

	courseFileIDs[redirectedAreaID] = originalCourseFileID;
	bgdatFileIDs[redirectedAreaID] = originalBgdatFileID;
	redirectedAreaID = -1;
}

// The bgdat is the last of the area's files to load
ncp_call(0x020B3A04, 0)
ncp_thumb bool loadBGDatFile_hook(u32 areaID, void* dest)
{
	bool loaded = CoopSym_LoadBGDatFile(areaID, dest);
	restoreCutsceneAreaFiles();
	return loaded;
}

void switchToCutsceneArea(u8 stage)
{
	u32 areaID = Stage::getAreaID(9, stage, 0);

	restoreCutsceneAreaFiles();
	originalCourseFileID = courseFileIDs[areaID];
	originalBgdatFileID = bgdatFileIDs[areaID];
	courseFileIDs[areaID] = cutsceneCourseFileIDs[stage];
	bgdatFileIDs[areaID] = cutsceneBgdatFileIDs[stage];
	redirectedAreaID = areaID;

	Entrance::targetAreaID = areaID;
	Entrance::targetEntranceID = 0;
	Entrance::switchArea();
}

ncp_call(0x020A1D00, 0)
ncp_thumb void levelEnd_hook(Stage::ExitFlag flag)
{
	if (Glue::Level::hasFlag("coop.endLevelToW2UnlockCutscene"))
	{
		switchToCutsceneArea(0);
	}
	else if (Glue::Level::hasFlag("coop.endLevelToW5UnlockCutscene"))
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
	if (Glue::Level::hasFlag("coop.worldUnlockCutscene"))
	{
		goToMiniWorld = Entrance::targetEntranceID == 2;
		Stage::exitLevel(Stage::ExitFlag::LevelBeaten);
		return;
	}
	Entrance::switchArea();
}

} // namespace Coop::WorldUnlock
