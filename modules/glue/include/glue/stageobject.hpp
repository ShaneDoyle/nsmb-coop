#pragma once

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/game/stage/layout/data/object.hpp>

// The generated table is written as MODULE_OBJ rows so that the name a level
// editor shows can be compiled out of a release build without the generator
// having to know which build this is.
#ifdef GLUE_STAGEOBJECT_DEBUG
#define MODULE_OBJ(hash, objectId, info, name) { hash, objectId, info, name }
#else
#define MODULE_OBJ(hash, objectId, info, name) { hash, objectId, info }
#endif

namespace Glue::StageObject {

// Where the vanilla stage object tables end. All three of them hold 326 entries,
// indices 0..325, which is checkable four independent ways:
//
//   - the reference declares `extern const u16 objectIDTable[326]`;
//   - objectBankTable (0x020C5010) ends exactly where objectInfoTable
//     (0x020C529C) begins, 326 * 2 bytes on;
//   - objectInfoTable ends at 0x020C6C14, 326 * 20 bytes on;
//   - row 325 is populated in all three, not padding.
//
// Do not confuse this with the 325 the game compares actor IDs against at
// 0x0209BF38 and 0x0209C194. That is a sentinel *value* stored in objectIDTable
// meaning "this stage object spawns nothing"; it says nothing about where the
// tables end, and reading it as a bound is off by one -- it aliases stage
// object 325, which is a real vanilla object spawning actor 300.
//
// The editor allocates 326 + n per level, one n per unique hash, so these IDs
// mean nothing outside the level that assigned them -- the hash is the identity.
constexpr u16 BaseObjectID = 326;

// One row per stage: variant, generated into extended_stageobjects.hpp.
//
// `objectId` is the *other* ID space: 0x182 + n, allocated by nsmbtool at build
// time, one per object, naming the class this stage object spawns. It is what
// the extension of objectIDTable is filled from. Two stage: variants of one
// object share an objectId and differ only in their ObjectInfo.
struct ModuleStageObjectInfo {
	u32 hash;
	u16 objectId;
	const ObjectInfo* info;
#ifdef GLUE_STAGEOBJECT_DEBUG
	const char* name;
#endif
};

// Custom StageObject, must be compatible with StageObject
// The editor is responsible for placing contiguous IDs!
// This is a crucial for optimization.
//
// `::StageObject` is spelled out because the enclosing namespace has the same
// name and would otherwise win the lookup.
struct ModuleStageObject : ::StageObject {

	// u16 id; // 326 + unique id (editor generates the unique id, there is 1 per unique hash, unique id starts at 0)
	// u16 x;
	// u16 y;
	// u8 events[2];
	// u32 settings;

	u32 hash;

};
NTR_SIZE_GUARD(ModuleStageObject, 0x10);

// The three extensions the patches below read. Declared here because the naked
// stubs reach them by mangled name and nothing else would keep them alive.
//
// Both arrays are indexed by stage object ID minus BaseObjectID. They are sized
// by module object count rather than by anything the level says, because a level
// cannot place more distinct hashes than there are generated rows.
// alignas because MI_CpuCopyFast wants a word-aligned destination and
// ObjectInfo is only halfword-aligned. It has to be stated on the declaration
// as well as the definition or the two disagree.
alignas(4) extern ObjectInfo extObjInfos[];
extern u16 extObjIDs[];
extern const ObjectBank moduleObjectBank;

// Fills the two arrays from the level's glue block. Hooked into
// Stage::loadAreaObjects after it has loaded the stage blocks and before it
// reads any of the three tables.
void beforeLoadSprites();

}
