#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>

#include "ActorFixes.hpp"

// Spike Bass ---------------------------------------------------------------------------

ncp_repl(0x021731B4, 58, ".int 0x4BC") // Add a zoneID field

NTR_USED static Player* SpikeBass_fixGetClosestPlayer(StageEntity* self)
{
	u32 zoneID = rcast<u32*>(self)[0x4B8 / 4];
	Player* player = ActorFixes_getClosestPlayerInZone(self, zoneID);
	return player ? player : ActorFixes_getClosestPlayer(self);
}

ncp_set_call(0x02172CB0, 58, SpikeBass_fixGetClosestPlayer)
ncp_set_call(0x02172E4C, 58, SpikeBass_fixGetClosestPlayer)

NTR_USED static Player* SpikeBassSpawner_fixCheckPlayerInView(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->viewID == self->viewID)
		{
			return player;
		}
	}
	return nullptr;
}

asm(R"(
ncp_call(0x02173318, 58)
	MOV     R0, R4
	B       _ZL37SpikeBassSpawner_fixCheckPlayerInViewP11StageEntity

ncp_jump(0x021734C8, 58)
	STR     R2, [R0,#0x4AC] // Keep replaced instruction
	LDRB    R2, [R4,#0x402] // R2 = SpikeBassSpawner*->zoneID
	STR     R2, [R0,#0x4B8] // SpikeBass*->zoneID = R2
	B       0x021734CC      // Return to code
)");

ncp_call(0x02173370, 58)
bool SpikeBassSpawner_fixCheckPlayerInZone(StageEntity* self, Player* player, u32 zoneID)
{
	return ActorFixes_getClosestPlayerInZone(self, zoneID) != nullptr;
}
