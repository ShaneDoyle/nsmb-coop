#pragma once

#include <nsmb/game/stage/entity3d.hpp>
#include <nsmb/game/stage/render/texture.hpp>
#include <nsmb/game/physics/collider.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/graphics/2d/font.hpp>
#include <nsmb/core/graphics/3d/model.hpp>

class InfoTextBoxBlock : public StageEntity3D
{
public:
	bool dialogVisible;
	void* bmg;
	Collider collider;
	u8 triggerPlayerID;

	static ActorProfile profile;

	constexpr static ObjectInfo objectInfo = {
		8, 8, // position
		4, 4, // size
		0, 0, // spawnOffset
		0, 0, // viewOffset
		CollisionSwitch::None, // collisionSwitch
		SpawnSettings::None // spawnSettings
	};

	static ColliderInfo colliderInfo;

	s32 onCreate() override;
	s32 onUpdate() override;
	s32 onRender() override;
	bool onPrepareResources() override;

	void showDialog(u32 playerID);

	static bool loadResources();
	static void hitFromTop(StageActor& self, StageActor& other);
	static void hitFromBottom(StageActor& self, StageActor& other);
};
