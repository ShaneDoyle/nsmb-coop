#pragma once

#include <nsmb/game/stage/entity3d.hpp>
#include <nsmb/core/system/function.hpp>

#include "objectids/coop.hpp"

class CoopWorldUnlockSign : public StageEntity3D
{
public:
	static ActorProfile Profile;

	constexpr static ObjectInfo objectInfo = {
		0, 0, // position
		4, 4, // size
		0, 0, // spawnOffset
		0, -16, // viewOffset
		EntityProperties::None, // properties
		SpawnSettings::None // spawnSettings
	};

	constexpr static u32 ObjectID = ObjectID::Coop::CoopWorldUnlockSign;

	s32 onCreate() override;
	s32 onUpdate() override;
	bool onPrepareResources() override;

	static bool loadResources();
	static u32 getModelFileID();
};
