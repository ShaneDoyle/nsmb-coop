#include "coop/actors/CoopWorldUnlockSign.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/core/filesystem/cache.hpp>

ActorProfile CoopWorldUnlockSign::Profile = {
	&constructObject<CoopWorldUnlockSign>,
	0xFB, 0x69,
	CoopWorldUnlockSign::loadResources
};

ncp_thumb s32 CoopWorldUnlockSign::onCreate()
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

ncp_thumb s32 CoopWorldUnlockSign::onUpdate()
{
	destroyInactive(0);
	return 1;
}

ncp_thumb bool CoopWorldUnlockSign::onPrepareResources()
{
	void* bmd = FS::Cache::getFile(getModelFileID());
	return model.create(bmd, 0, 0);
}

ncp_thumb bool CoopWorldUnlockSign::loadResources()
{
	FS::Cache::loadFile(getModelFileID(), false);
	return true;
}

ncp_thumb u32 CoopWorldUnlockSign::getModelFileID()
{
	return (Game::stageGroup == 1 ? 1653 : 1654) - 131;
}
