#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>

#include "coop/CoopActor.hpp"

namespace CoopFixes::SpikeBassSpawner {

Player* customCheckPlayerInView(StageEntity* self)
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

ncp_call(0x02173370, 58)
bool customCheckPlayerInZone(StageEntity* self, Player* player, u32 zoneID)
{
	return CoopActor::getClosestPlayerInZone(self, zoneID) != nullptr;
}

ncp_asmfunc void ASM()
{asm(R"(
ncp_call(0x02173318, 58)
	MOV     R0, R4
	B       _ZN9CoopFixes16SpikeBassSpawner23customCheckPlayerInViewEP11StageEntity

ncp_jump(0x021734C8, 58)
	STR     R2, [R0,#0x4AC] // Keep replaced instruction
	LDRB    R2, [R4,#0x402] // R2 = SpikeBassSpawner*->zoneID
	STR     R2, [R0,#COOP_FIX_SPIKE_BASS_ZONE_ID_FIELD_OFFSET] // SpikeBass*->zoneID = R2
	B       0x021734CC      // Return to code
)");}

} // namespace CoopFixes::SpikeBassSpawner
