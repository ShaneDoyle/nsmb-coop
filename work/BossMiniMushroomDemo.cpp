#include "BossMiniMushroomDemo.hpp"

#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/system/function.hpp>

#include "ActorFixes.hpp"

asm(R"(
	StageLayout_setCameraBound = 0x020ACF50
	Stage_exitLevel = 0x020A189C

Item_checkConsume_SUPER:
	PUSH    {R4-R8,LR}
	B       0x020D4760
)");
extern "C" {
	void StageLayout_setCameraBound(StageLayout* self, s16 bound, u32 side);
	void Stage_exitLevel(u32 flag);
	bool Item_checkConsume_SUPER(void* item);
}

ncp_jump(0x020D475C, 10)
bool Item_checkConsume_OVERRIDE(void* item)
{
	Player* player = *rcast<Player**>(&rcast<u8*>(item)[0x544]);
	u32 itemType = *rcast<u32*>(&rcast<u8*>(item)[0x574]);

	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (!player || (itemType == 25 && player->currentPowerup == PowerupState::Mini && (areaNum == 180 || areaNum == 181)))
		return false;

	return Item_checkConsume_SUPER(item);
}

ncp_call(0x02119684, 10)
void Player_transitEntranceWarp_AT_02119684_CALL()
{
	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (areaNum == 180 || areaNum == 181)
	{
		Stage_exitLevel(1);
		return;
	}
	Entrance::switchArea();
}

constexpr u8 WalkZoneID = 0;
constexpr u8 CollectMiniZoneID = 1;
constexpr u8 MiniZoneID = 2;

ActorProfile BossMiniMushroomDemo::profile = {
	&constructObject<BossMiniMushroomDemo>,
	0xFB, 0x69,
	nullptr
};

s32 BossMiniMushroomDemo::onCreate()
{
	this->switchState(&BossMiniMushroomDemo::waitEnterState);
	return 1;
}

s32 BossMiniMushroomDemo::onUpdate()
{
	updateFunc(this);
	return 1;
}

void BossMiniMushroomDemo::switchState(void (*updateFunc)(BossMiniMushroomDemo*))
{
	if (this->updateFunc != updateFunc)
	{
		if (this->updateFunc)
		{
			this->updateStep = Func::Exit;
			this->updateFunc(this);
		}

		this->updateFunc = updateFunc;

		this->updateStep = Func::Init;
		this->updateFunc(this);
	}
}

void BossMiniMushroomDemo::waitEnterState()
{
	if (this->updateStep == Func::Init)
	{
		this->updateStep = 1;
		restrictToZone(WalkZoneID);
		return;
	}
	if (this->updateStep == Func::Exit)
	{
		return;
	}

	if (allPlayersInZone(CollectMiniZoneID))
	{
		this->switchState(&BossMiniMushroomDemo::restrictState);
	}
}

void BossMiniMushroomDemo::restrictState()
{
	if (this->updateStep == Func::Init)
	{
		this->updateStep = 1;
		restrictToZone(CollectMiniZoneID);
		return;
	}
	if (this->updateStep == Func::Exit)
	{
		return;
	}

	if (allPlayersMini() && allPlayersInZone(MiniZoneID))
	{
		if (this->currentRestrictZoneID != MiniZoneID)
			restrictToZone(MiniZoneID);
	}
	else
	{
		if (this->currentRestrictZoneID != CollectMiniZoneID)
			restrictToZone(CollectMiniZoneID);
	}
}

bool BossMiniMushroomDemo::allPlayersMini()
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->currentPowerup != PowerupState::Mini)
			return false;
	}
	return true;
}

bool BossMiniMushroomDemo::allPlayersInZone(u8 zoneID)
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (!ActorFixes_isPlayerInZone(player, zoneID))
			return false;
	}
	return true;
}

void BossMiniMushroomDemo::restrictToZone(u8 zoneID)
{
	Rectangle<fx32> zoneBox;
	if (StageZone::get(zoneID, &zoneBox))
	{
		StageLayout_setCameraBound(Stage::stageLayout, zoneBox.x >> FX32_SHIFT, 1);
		StageLayout_setCameraBound(Stage::stageLayout, (zoneBox.x + zoneBox.width) >> FX32_SHIFT, 0);
	}
	this->currentRestrictZoneID = zoneID;
}
