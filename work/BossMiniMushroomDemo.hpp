#pragma once

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/core/system/function.hpp>

class BossMiniMushroomDemo : public StageEntity
{
public:
	void (*updateFunc)(BossMiniMushroomDemo*);
	s8 updateStep;
	u8 currentRestrictZoneID;

	static ActorProfile profile;

	constexpr static ObjectInfo objectInfo = {
		0, 0, // position
		0, 0, // renderSize
		0, 0, // spawnOffset
		0, 0, // viewOffset
		CollisionSwitch::None, // collisionSwitch
		SpawnSettings::None // spawnSettings
	};

	s32 onCreate() override;
	s32 onUpdate() override;

	void switchState(void (*updateFunc)(BossMiniMushroomDemo*));

	inline void switchState(void (BossMiniMushroomDemo::*updateFunc)()) {
		switchState(ptmf_cast(updateFunc));
	}

	void waitEnterState();
	void restrictState();
	bool allPlayersMini();
	bool allPlayersInZone(u8 zoneID);
	void restrictToZone(u8 zoneID);
};
