#include <nsmb/extra/assert.hpp>
#include <nsmb/extra/log.hpp>

#include "glue/stageobject.hpp"
#include "glue/level.hpp"

// Defines GLUE_STAGEOBJECT_NONE when no module declares a stage: variant, in
// which case there is no table and nothing below has anything to do.
#include "generated/glue/extended_stageobjects.hpp"

#ifndef GLUE_STAGEOBJECT_NONE

namespace Glue::StageObject {

// Indexed by stage object ID minus 326. One ID exists per unique hash in the
// level, and every hash has to be one of the generated rows, so the table can
// never need more entries than there are rows.
//
// These are copies rather than pointers to the generated ObjectInfos: the game
// reads them by index, and a copy keeps the table readable no matter which
// overlay the object that supplied it came from. alignas because MI_CpuCopyFast
// wants a word-aligned destination and ObjectInfo is only halfword-aligned.
alignas(4) ObjectInfo extObjInfos[moduleStageObjectCount];

// Extends Stage::objectIDTable: which object (0x182 + n) each extended stage
// object ID spawns. beforeLoadSprites writes every slot the level uses before
// anything reads one, which matters because 0 is a valid vanilla object ID --
// a slot left at its initial value would spawn object 0 rather than nothing.
u16 extObjIDs[moduleStageObjectCount];

// Extends Stage::objectBankTable. Module objects live in module code rather than
// in one of the game's ten object-bank overlays, so they are never gated on a
// bank being loaded, and bank 0 is what loadAreaObjects reads as "no
// requirement". One shared entry is enough because every module object answers
// the same, and `slot` is never read once `bank` is 0 -- the game's test
// short-circuits on it.
const ObjectBank moduleObjectBank = { 0, 0 };

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
		// Contiguous from 326 is what the editor promises. If it did not, this
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
			extObjIDs[extObjIndex] = modObjInfo->objectId;
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
}

} // namespace Glue::StageObject

// ---------------------------------------------------------------------------
// Making the game read the extensions
//
// Three tables are indexed by stage object ID, and a placed module object needs
// all three: objectBankTable decides whether loadAreaObjects considers it at
// all, objectIDTable says which class to spawn, and objectInfoTable carries the
// geometry. They sit contiguously and are all 326 entries -- see BaseObjectID.
//
// Every read site uses one of two idioms, and each patch replaces the single
// instruction that combines base and index, choosing the base first:
//
//   objectInfoTable   ldr <base>,=table ; mov <k>,#20 ; mla <dest>,<id>,<k>,<base>
//   objectIDTable     ldr <base>,=table ; mov <idx>,<id>,lsl #1 ; ldrh <dest>,[<base>,<idx>]
//
// The extension pointer is pre-biased by BaseObjectID entries so the untouched
// index arithmetic lands on element zero. That is what lets these stubs leave
// the ID register alone: nothing has to be renumbered, only the base swapped.
//
// The scratch register is the replaced instruction's own destination, which is
// dead on entry by definition. Where destination and base are the same register
// the stub reloads the base, so clobbering it costs nothing.
//
// 326 is not an encodable ARM data-processing immediate (nor are 324 and 325),
// so the mla sites split the comparison: subtract 320, which is encodable, and
// compare the remainder against 6 -- signed, because an ID below 320 makes the
// difference negative. The ldrh sites compare the already-doubled index against
// 652 instead, which is encodable, and unsigned, since a doubled u16 cannot go
// negative.
//
// Every stub clobbers the flags. That is safe at all thirteen sites: each one's
// next conditional instruction is preceded by its own flag-setting instruction,
// or by a bl, across which the flags are caller-clobbered anyway.
//
// Addresses are Ghidra addresses; overlay_offsets[0] and overlay_offsets[54] are
// both 0, so no translation applies. The stubs live in main ARM9 -- overlay 0
// sits at exactly its 0x33C00 maxsize and has no room for them -- which works
// because NCPatcher tracks a patch's source and destination regions separately.
// ---------------------------------------------------------------------------

// The biases below are BaseObjectID * sizeof(ObjectInfo) = 326 * 20 = 6520 for
// extObjInfos, and BaseObjectID * sizeof(u16) = 652 for extObjIDs. They are
// spelled out because the assembler cannot see the C++ constants.

// --- objectInfoTable: eight sites in seven functions -----------------------

// Stage::loadAreaObjects, reading spawnSettings for the MvsL/single-player test.
ncp_jump(0x0201F804)
NTR_NAKED void extendObjectInfoLoadAreaObjectsA() {asm(R"(
	sub		r1, r0, #320
	cmp		r1, #6
	ldrlt	r1, =0x020C529C
	ldrge	r1, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r1, r0, r2, r1
	b		0x0201F804 + 4
)");}

// Stage::loadAreaObjects, reading spawnSettings for the IgnoreView test.
ncp_jump(0x0201F8F0)
NTR_NAKED void extendObjectInfoLoadAreaObjectsB() {asm(R"(
	sub		r0, r2, #320
	cmp		r0, #6
	ldrlt	r0, =0x020C529C
	ldrge	r0, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r0, r2, r1, r0
	b		0x0201F8F0 + 4
)");}

// respawnSprites.
ncp_jump(0x0209B164, 0)
NTR_NAKED void extendObjectInfoRespawnSprites() {asm(R"(
	sub		r8, r2, #320
	cmp		r8, #6
	ldrlt	r8, =0x020C529C
	ldrge	r8, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r8, r2, r1, r8
	b		0x0209B164 + 4
)");}

// spawnSprite_MvsL.
ncp_jump(0x0209B40C, 0)
NTR_NAKED void extendObjectInfoSpawnSpriteMvsL() {asm(R"(
	sub		r10, r2, #320
	cmp		r10, #6
	ldrlt	r10, =0x020C529C
	ldrge	r10, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r10, r2, r1, r10
	b		0x0209B40C + 4
)");}

// spawnSprites. This is the one site whose `ldr` is hoisted out of the loop, so
// the base in r11 is shared by every ID the loop visits and cannot be swapped
// there. Patching the mla instead reaches the per-iteration ID in r1 and leaves
// r11 holding exactly what it held before, for whatever else reads it.
ncp_jump(0x0209B6CC, 0)
NTR_NAKED void extendObjectInfoSpawnSprites() {asm(R"(
	sub		r9, r1, #320
	cmp		r9, #6
	ldrlt	r9, =0x020C529C
	ldrge	r9, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r9, r1, r4, r9
	b		0x0209B6CC + 4
)");}

// FUN_0209BA94, reading spawnOffset and viewOffset to place the sprite.
ncp_jump(0x0209BB80, 0)
NTR_NAKED void extendObjectInfoPlaceSprite() {asm(R"(
	sub		r6, r2, #320
	cmp		r6, #6
	ldrlt	r6, =0x020C529C
	ldrge	r6, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r6, r2, r1, r6
	b		0x0209BB80 + 4
)");}

// spawnObject.
ncp_jump(0x0209C1AC, 0)
NTR_NAKED void extendObjectInfoSpawnObject() {asm(R"(
	sub		r4, r5, #320
	cmp		r4, #6
	ldrlt	r4, =0x020C529C
	ldrge	r4, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r4, r5, r3, r4
	b		0x0209C1AC + 4
)");}

// FUN_02168BAC, in overlay 54.
ncp_jump(0x02168BFC, 54)
NTR_NAKED void extendObjectInfoOv54() {asm(R"(
	sub		r6, r8, #320
	cmp		r6, #6
	ldrlt	r6, =0x020C529C
	ldrge	r6, =(_ZN4Glue11StageObject11extObjInfosE - 6520)
	mla		r6, r8, r12, r6
	b		0x02168BFC + 4
)");}

// --- objectIDTable: four sites ---------------------------------------------

// StageEntity::getActorID. The index is in r0, which is also the destination, so
// the base register r1 is the only scratch available -- safe, because this is a
// leaf returning in r0 and r1 is caller-saved.
ncp_jump(0x02099598, 0)
NTR_NAKED void extendObjectIDGetActorID() {asm(R"(
	cmp		r0, #652
	ldrhs	r1, =(_ZN4Glue11StageObject9extObjIDsE - 652)
	ldrh	r0, [r1, r0]
	b		0x02099598 + 4
)");}

// spawnSprite_spawn, testing the ID against the 325 "spawns nothing" sentinel.
// r4 holds the base and is live-ish, so the dead destination r12 is used
// instead and r4 is left untouched.
ncp_jump(0x0209BF2C, 0)
NTR_NAKED void extendObjectIDSpawnSpriteA() {asm(R"(
	cmp		r5, #652
	movlo	r12, r4
	ldrhs	r12, =(_ZN4Glue11StageObject9extObjIDsE - 652)
	ldrh	r12, [r12, r5]
	b		0x0209BF2C + 4
)");}

// spawnSprite_spawn, the actual spawn.
ncp_jump(0x0209C04C, 0)
NTR_NAKED void extendObjectIDSpawnSpriteB() {asm(R"(
	cmp		r2, #652
	ldrhs	r0, =(_ZN4Glue11StageObject9extObjIDsE - 652)
	ldrh	r0, [r0, r2]
	b		0x0209C04C + 4
)");}

// spawnObject.
ncp_jump(0x0209C18C, 0)
NTR_NAKED void extendObjectIDSpawnObject() {asm(R"(
	cmp		r3, #652
	ldrhs	r0, =(_ZN4Glue11StageObject9extObjIDsE - 652)
	ldrh	r0, [r0, r3]
	b		0x0209C18C + 4
)");}

// --- calling into beforeLoadSprites ----------------------------------------

// Stage::loadAreaObjects, right after LoadBlocks has made the level's stage
// blocks -- and so the glue block -- available, and after both early returns,
// but before anything reads one of the three tables.
//
// A plain jump rather than ncp_hook, because the hook bridge would push and pop
// r0-r3/r12/lr around the call and all six are dead here: the bl four
// instructions earlier already clobbered r12 and lr (this function's return
// address is on the stack, popped straight into pc), and r0-r3 are each written
// before they are next read, at 0x0201F6D4 through 0x0201F6E0. That leaves the
// stub with nothing to preserve -- only the replaced instruction to re-execute.
ncp_jump(0x0201F6D0)
NTR_NAKED void callBeforeLoadSprites() {asm(R"(
	bl		_ZN4Glue11StageObject17beforeLoadSpritesEv
	mov		r4, #0 // Keep replaced instruction
	b		0x0201F6D0 + 4 // Return to code
)");}

// --- objectBankTable: one site ---------------------------------------------

// StageEntity::getObjectBank, whose only caller is loadAreaObjects. It is
// `return objectBankTable + id`, so returning the shared zero entry for module
// IDs is enough. r1 already holds the vanilla base, so r2 is borrowed as scratch
// rather than reloading it; both are caller-saved and this is a leaf.
ncp_jump(0x0209878C, 0)
NTR_NAKED void extendObjectBank() {asm(R"(
	sub		r2, r0, #320
	cmp		r2, #6
	addlt	r0, r1, r0, lsl #1
	ldrge	r0, =_ZN4Glue11StageObject16moduleObjectBankE
	b		0x0209878C + 4
)");}

#endif // GLUE_STAGEOBJECT_NONE
