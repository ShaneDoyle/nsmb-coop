#include <nsmb/game/stage/player/player.hpp>

static void ToadHouse_freezePlayers()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->beginCutscene(0);
}

static void ToadHouse_unfreezePlayers()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->endCutscene();
}

ncp_set_call(0x0218CD60, 123, ToadHouse_freezePlayers) // Mushroom houses (freeze)
ncp_set_call(0x0218CE94, 123, ToadHouse_unfreezePlayers) // Mushroom houses (unfreeze)
ncp_set_call(0x0218D1EC, 123, ToadHouse_freezePlayers) // 1up houses (freeze)
ncp_set_call(0x0218D480, 123, ToadHouse_unfreezePlayers) // 1up houses (unfreeze)

ncp_call(0x0218CF20, 123) void call_0218CF20_ov123()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->physicsFlag.flag20000000 = false;
}

ncp_call(0x0218E630, 123) void call_0218E630_ov123()
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		Game::getPlayer(i)->physicsFlag.flag20000000 = true;
}
