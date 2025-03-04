#include "BossMiniMushroomDemo.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/core/filesystem/cache.hpp>

ActorProfile BossMiniMushroomDemo::profile = {
	&constructObject<BossMiniMushroomDemo>,
	0xFB, 0x69,
	BossMiniMushroomDemo::loadResources
};

s32 BossMiniMushroomDemo::onCreate()
{
	if (!prepareResourcesSafe(64, Memory::gameHeapPtr))
		return 0;

	rotationTranslation = Vec2(0);
	renderOffset = Vec2(0);
	fogFlag = false;
	alpha = -1;

	bool showBackside = settings & 1;
	if (showBackside)
		rotation.y = 0x8000;

	return 1;
}

s32 BossMiniMushroomDemo::onUpdate()
{
	destroyInactive(0);
	return 1;
}

bool BossMiniMushroomDemo::onPrepareResources()
{
	void* bmd = FS::Cache::getFile(getModelFileID());
	return model.create(bmd, 0, 0);
}

bool BossMiniMushroomDemo::loadResources()
{
	FS::Cache::loadFile(getModelFileID(), false);
	return true;
}

u32 BossMiniMushroomDemo::getModelFileID()
{
	return (Game::stageGroup == 1 ? 1653 : 1654) - 131;
}
