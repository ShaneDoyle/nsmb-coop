#pragma once

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/layout/data/object.hpp>
#include <nsmb/core/system/function.hpp>

#include "objectids/coop.hpp"

class CoopFlagActor : public StageEntity
{
public:
	static ActorProfile Profile;

	constexpr static ObjectInfo objectInfo = {
		0, 0, // position
		0, 0, // size
		0, 0, // spawnOffset
		0, 0, // viewOffset
		EntityProperties::None, // properties
		SpawnSettings::None // spawnSettings
	};

	constexpr static u32 ObjectID = ObjectID::Coop::CoopFlagActor;

	enum class Action : u32
	{
		// Enables the shared camera when in player range
		EnableSharedCamera = 0,

		// Disables the shared camera when in player range
		DisableSharedCamera = 1,

		// Used to prevent liquid from reloading the area
		// Only use inside a view that won't take you to a view that has liquids within the same area
		BlockForcedAreaReloads = 2,

		// Flags the level as being the level where you can
		// choose which world to go after beating a boss
		WorldUnlockCutscene = 3,

		// Handle with findByAction
		EndLevelToW2UnlockCutscene = 4,

		// Handle with findByAction
		EndLevelToW5UnlockCutscene = 5,

		// Save on 3D memory
		// Handle with findByAction
		DoNotLoadDoorModels = 6,

		// Save on 3D memory
		// Handle with findByAction
		DoNotLoadCastleModel = 7
	};

	s32 onCreate() override;

	NTR_INLINE static Action getAction(u32 settings) { return scast<Action>(settings); }
	static StageObject* findByAction(Action action);
};
