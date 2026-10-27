#include <nsmb/game/stage/player/player.hpp>

namespace CoopFixes::MushroomHouseBlock {

ncp_call(0x0218E630, 123) void call_0218E630_ov123()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->physicsFlag.flag20000000 = true;
}

} // namespace CoopFixes::MushroomHouseBlock
