#pragma once

#include <nsmb/game/stage/entity3d.hpp>
#include <nsmb/core/system/function.hpp>

class BossMiniMushroomDemo : public StageEntity3D
{
public:
	static ActorProfile profile;

	constexpr static ObjectInfo objectInfo = {
		0, 0, // position
		4, 4, // size
		0, 0, // spawnOffset
		0, -16, // viewOffset
		CollisionSwitch::None, // collisionSwitch
		SpawnSettings::None // spawnSettings
	};

	s32 onCreate() override;
	s32 onUpdate() override;
	bool onPrepareResources() override;

	static bool loadResources();
	static u32 getModelFileID();
};
