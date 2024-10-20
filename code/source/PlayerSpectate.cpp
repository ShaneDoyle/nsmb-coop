#include "PlayerSpectate.hpp"

#include "nsmb/game.hpp"
#include "nsmb/stage/player/player.hpp"

// Notes:
//   - Commenting out the playerID substitution in StageEntity::skipRender
//     can be helpful for identifying desync issues.
//     Specifically, these desyncs may occur when the bone position doesn't
//     update because the animation isn't playing.

namespace PlayerSpectate {

u32 localTarget;
u8 playerTarget[2];

u32 getTarget(u32 playerID)
{
	return playerTarget[playerID];
};

void setTarget(u32 playerID, u32 targetPlayerID)
{
	playerTarget[playerID] = targetPlayerID;

	if (playerID == Game::localPlayerID)
		localTarget = targetPlayerID;
};

bool isSpectating(u32 playerID)
{
	return playerTarget[playerID] != playerID;
}

Player* getLocalTargetPlayer()
{
	return Game::getPlayer(localTarget);
}

void reset()
{
	for (s32 playerID = 0; playerID < NTR_ARRAY_SIZE(playerTarget); playerID++)
	{
		setTarget(playerID, playerID);
	}
}

ncp_repl(0x01FFFD34, ".int _ZN14PlayerSpectate11localTargetE") // (0x01FFFC20) StageActor::doHdma
ncp_repl(0x01FFFF74, ".int _ZN14PlayerSpectate11localTargetE") // (0x01FFFE10)
ncp_repl(0x0200D858, ".int _ZN14PlayerSpectate11localTargetE") // (0x0200D578) OAM::drawSprite
ncp_repl(0x0200DD74, ".int _ZN14PlayerSpectate11localTargetE") // (0x0200DC48) OAM::loadAffineSets
ncp_repl(0x02012024, ".int _ZN14PlayerSpectate11localTargetE") // (0x02011F5C) SND::updateScreenBoundaries
ncp_repl(0x020121D4, ".int _ZN14PlayerSpectate11localTargetE") // (0x02012038) SND::playSFX
ncp_repl(0x0209ADAC, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x0209AD1C) StageEntity::skipRender
//ncp_repl(0x020A9D34, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020A9D34) CollisionMgr::getTileTypeAbs
//ncp_repl(0x020ACF38, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020ACE0C) StageLayout::unk_20ACE0C
ncp_repl(0x020AD26C, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020AD06C) StageLayout::scrollLevelDirect
ncp_repl(0x020AEA0C, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020AEA0C) StageLayout::bgStuffLoop
ncp_repl(0x020AF748, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020AF30C) StageLayout::changeTile
ncp_repl(0x020B8D20, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020B8CA8) StageLayout::unk_20B8CA8
ncp_repl(0x020BACC0, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020BAA5C) StageLayout::onUpdate
ncp_repl(0x020BB45C, 0, ".int _ZN14PlayerSpectate11localTargetE") // (0x020BB430) StageLayout::onRender

// (0x020BBBDC) Player View Setup ----------------

//ncp_repl(0x020BC61C, 0, ".int _ZN14PlayerSpectate11localTargetE")
//ncp_repl(0x020BC668, 0, ".int _ZN14PlayerSpectate11localTargetE")

// StageCamera ----------------

ncp_repl(0x020CDF6C, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_set_call(0x020CE024, 10, getLocalTargetPlayer)
ncp_set_call(0x020CE244, 10, getLocalTargetPlayer)
ncp_repl(0x020CE2F0, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x020CE654, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x020CE408, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x020CE460, 10, ".int _ZN14PlayerSpectate11localTargetE")

// Foreground Fog Effect ----------------

ncp_repl(0x020D709C, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x020D6FEC, 10, ".int _ZN14PlayerSpectate11localTargetE")

// Liquid ----------------

ncp_repl(0x02164824, 54, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x0216496C, 54, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x02165CA4, 54, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x02165E10, 54, ".int _ZN14PlayerSpectate11localTargetE")

// Misc ----------------

ncp_repl(0x020F7E30, 10, ".int _ZN14PlayerSpectate11localTargetE") // (0x020F7D74) PipeBase::onRender

// todo sound

}
