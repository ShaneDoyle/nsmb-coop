#pragma once

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/layout/data/object.hpp>

// The generated table is written as MODULE_OBJ rows so that the name a level
// editor shows can be compiled out of a release build without the generator
// having to know which build this is.
#ifdef GLUE_STAGEOBJECT_DEBUG
#define MODULE_OBJ(hash, info, name) { hash, info, name }
#else
#define MODULE_OBJ(hash, info, name) { hash, info }
#endif

namespace Glue::StageObject {

// Where the vanilla stage object table ends. The editor allocates 325 + n per
// level, one n per unique hash, so these IDs mean nothing outside the level
// that assigned them -- the hash is the identity.
constexpr u16 BaseObjectID = 325;

// One row per stage: variant, generated into extended_stageobjects.hpp.
struct ModuleStageObjectInfo {
	u32 hash;
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

	// u16 id; // 325 + unique id (editor generates the unique id, there is 1 per unique hash, unique id starts at 0)
	// u16 x;
	// u16 y;
	// u8 events[2];
	// u32 settings;

	u32 hash;

};
NTR_SIZE_GUARD(ModuleStageObject, 0x10);

}
