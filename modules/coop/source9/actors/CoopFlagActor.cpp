#include "coop/actors/CoopFlagActor.hpp"

#include <nsmb/game/game.hpp>

#include "coop/CoopCamera.hpp"
#include "coop/CoopStage.hpp"

ActorProfile CoopFlagActor::Profile = {
	&constructObject<CoopFlagActor>,
	0xFB, 0x69,
	nullptr
};

ncp_thumb s32 CoopFlagActor::onCreate()
{
	Action action = getAction(settings);

	switch (action)
	{
	case Action::EnableSharedCamera:
		CoopCamera::enableSharedCamera();
		break;
	case Action::DisableSharedCamera:
		CoopCamera::disableSharedCamera();
		break;
	case Action::BlockForcedAreaReloads:
		CoopStage::blockForcedAreaReloads();
		break;
	default:
		break;
	}

	return 0;
}

ncp_thumb StageObject* CoopFlagActor::findByAction(Action action)
{
	StageObject* stageObjs = Stage::stageBlocks.stageObjs;
	for (u32 i = 0; ; i++)
	{
		StageObject* stageObj = &stageObjs[i];
		u16 stageObjID = stageObj->id;
		if (stageObjID == 0xFFFF) // Array end
			break;
		if (stageObjID == ObjectID && getAction(stageObj->settings) == action)
			return stageObj;
	}
	return nullptr;
}
