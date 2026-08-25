#include <nsmb/extra/assert.hpp>
#include <nsmb/extra/log.hpp>

#include "glue/stageobject.hpp"
#include "glue/level.hpp"

// Defines GLUE_STAGEOBJECT_NONE when no module declares a stage: variant, in
// which case there is no table and nothing below has anything to do.
#include "generated/glue/extended_stageobjects.hpp"

#ifndef GLUE_STAGEOBJECT_NONE

namespace Glue::StageObject {

// Indexed by stage object ID minus 325. One ID exists per unique hash in the
// level, and every hash has to be one of the generated rows, so the table can
// never need more entries than there are rows.
//
// These are copies rather than pointers to the generated ObjectInfos: the game
// reads them by index, and a copy keeps the table readable no matter which
// overlay the object that supplied it came from. alignas because MI_CpuCopyFast
// wants a word-aligned destination and ObjectInfo is only halfword-aligned.
alignas(4) ObjectInfo extObjInfos[moduleStageObjectCount];

void beforeLoadSprites()
{
	// TODO: this block is actually going to be special, it will have a terminator with value 0 of 4 bytes

	auto stageObjModsOpt = Level::get<"glue.stageObjects">();
	if (!stageObjModsOpt) {
		return;
	}

	Level::SizedArray<ModuleStageObject> stageObjModsSizedArray = stageObjModsOpt.value();

	u32 stageObjModCount = stageObjModsSizedArray.size;
	ModuleStageObject* stageObjMods = stageObjModsSizedArray.values;

#ifdef NTR_DEBUG
	// No point in wasting memory by having this condition if we can just have the previous one fail
	if (stageObjModCount == 0) {
		ntr_terminate(
			"glue.stageObjects array is present but empty.\n"
			"This is not allowed. If there are no custom stage objects, the array should be omitted entirely.\n"
			"Check your level or data generator to ensure glue.stageObjects is only created when it contains objects."
		);
		return;
	}
#endif

	u32 prevObjID = 0xFFFFFFFF;
	for (u32 i = 0; i < stageObjModCount; i++) {
		ModuleStageObject* stageObjMod = &stageObjMods[i];
		u16 stageObjID = stageObjMod->id;

		// Do not load the same hash
		if (stageObjID == prevObjID)
			continue;

		u32 extObjIndex = stageObjID - BaseObjectID;

#ifdef NTR_DEBUG
		// Contiguous from 325 is what the editor promises. If it did not, this
		// would write past the table rather than draw the wrong object.
		if (stageObjID < BaseObjectID || extObjIndex >= moduleStageObjectCount) {
			ntr_terminate("Stage object ID %u is outside the module range.\n", stageObjID);
		}

		bool found = false;
#endif
		for (u32 j = 0; j < moduleStageObjectCount; j++) {
			ModuleStageObjectInfo* modObjInfo = &moduleStageObjectInfos[j];

			// We haven't found the object yet
			if (modObjInfo->hash != stageObjMod->hash)
				continue;

#ifdef GLUE_STAGEOBJECT_DEBUG
			Log::print("Loading custom stage object %s as %u\n", modObjInfo->name, stageObjID);
#endif

			// TODO: runtime modules

			MI_CpuCopyFast(modObjInfo->info, &extObjInfos[extObjIndex], sizeof(ObjectInfo));
#ifdef NTR_DEBUG
			found = true;
#endif
			break;
		}

#ifdef NTR_DEBUG
		if (!found) {
			ntr_terminate("Unknown ModuleStageObject hash: 0x%08X\n", stageObjMod->hash);
		}
#endif

		prevObjID = stageObjID;
	}

	// TODO: hook the extObjInfos
	//
	// The prototype also computed, and never used:
	//   u32 lastStageObjID = stageObjMods[stageObjModCount - 1].id;
	//   u32 extObjCount = lastStageObjID - BaseObjectID - 1;
	// which is presumably what the hook wants to be told -- how many
	// extended infos this level actually has. Kept here as a note rather
	// than as dead code, and note the arithmetic is a count short.
}

} // namespace Glue::StageObject

#endif // GLUE_STAGEOBJECT_NONE
