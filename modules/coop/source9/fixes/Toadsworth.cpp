#include <nsmb/game/stage/player/player.hpp>

#include "coop/CoopActor.hpp"

namespace CoopFixes::Toadsworth {

ncp_over(0x0218F450, 123) const auto vtbl_skipRender = CoopActor::safeSkipRender;

static void freezePlayers()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		Game::getPlayer(playerID)->beginCutscene(false);
}

static void unfreezePlayers()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		Game::getPlayer(playerID)->endCutscene();
}

ncp_set_call(0x0218CD60, 123, freezePlayers) // Mushroom houses (freeze)
ncp_set_call(0x0218CE94, 123, unfreezePlayers) // Mushroom houses (unfreeze)
ncp_set_call(0x0218D1EC, 123, freezePlayers) // 1up houses (freeze)
ncp_set_call(0x0218D480, 123, unfreezePlayers) // 1up houses (unfreeze)

ncp_call(0x0218CF20, 123) void call_0218CF20_ov123()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->physicsFlag.flag20000000 = false;
}

} // namespace CoopFixes::Toadsworth
