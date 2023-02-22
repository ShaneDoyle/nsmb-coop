#include "PlayerSpectate.hpp"
#include "nsmb/game.h"
#include "nsmb/stage/player/player.h"

namespace PlayerSpectate {

u32 sLocalTarget;

u32 getTarget()
{
	return sLocalTarget;
};

void setTarget(u32 playerID, u32 targetPlayerID)
{
	if (playerID == Game::localPlayerID)
		sLocalTarget = targetPlayerID;
};

static Player* getLocalTargetPlayer()
{
	return Game::getPlayer(sLocalTarget);
}

asm(R"(
ncp_jump(0x02006AE4)
	STR     LR, [R12]
	LDR     R12, =_ZN14PlayerSpectate12sLocalTargetE
	STR     LR, [R12]
	B       0x02006AE8

/*ncp_jump(0x020BAC90, 0)
	LDR     R1, =_ZN4Game13localPlayerIDE
	LDR     R1, [R1]
	B       0x020BAC94*/
)");

ncp_repl(0x020BADC4, 0, "NOP")

ncp_repl(0x01FFFD34, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x01FFFC20) StageActor::doHdma
ncp_repl(0x01FFFF74, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x01FFFE10)
ncp_repl(0x0200D858, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x0200D578) OAM::drawSprite
ncp_repl(0x0200DD74, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x0200DC48) OAM::loadAffineSets
ncp_repl(0x02012024, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x02011F5C) SND::updateScreenBoundaries
ncp_repl(0x020121D4, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x02012038) SND::playSFX
ncp_repl(0x0209ADAC, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x0209AD1C) StageEntity::skipRender
//ncp_repl(0x020A9D34, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020A9D34) CollisionMgr::getTileTypeAbs
//ncp_repl(0x020ACF38, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020ACE0C) StageLayout::unk_20ACE0C
ncp_repl(0x020AD26C, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020AD06C) StageLayout::scrollLevelDirect
ncp_repl(0x020AEA0C, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020AEA0C) StageLayout::bgStuffLoop
ncp_repl(0x020AF748, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020AF30C) StageLayout::changeTile
ncp_repl(0x020B8D20, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020B8CA8) StageLayout::unk_20B8CA8
ncp_repl(0x020BACC0, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020BAA5C) StageLayout::onUpdate
ncp_repl(0x020BB45C, 0, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020BB430) StageLayout::onRender

// (0x020BBBDC) Player View Setup ----------------

//ncp_repl(0x020BC61C, 0, ".int _ZN14PlayerSpectate12sLocalTargetE")
//ncp_repl(0x020BC668, 0, ".int _ZN14PlayerSpectate12sLocalTargetE")

// StageCamera ----------------

ncp_repl(0x020CDF6C, 10, ".int _ZN14PlayerSpectate12sLocalTargetE")
ncp_set_call(0x020CE024, 10, getLocalTargetPlayer)
ncp_set_call(0x020CE244, 10, getLocalTargetPlayer)
ncp_repl(0x020CE2F0, 10, ".int _ZN14PlayerSpectate12sLocalTargetE")
ncp_repl(0x020CE654, 10, ".int _ZN14PlayerSpectate12sLocalTargetE")
ncp_repl(0x020CE408, 10, ".int _ZN14PlayerSpectate12sLocalTargetE")
ncp_repl(0x020CE460, 10, ".int _ZN14PlayerSpectate12sLocalTargetE")

ncp_repl(0x020F7E30, 10, ".int _ZN14PlayerSpectate12sLocalTargetE") // (0x020F7D74) PipeBase::onRender

}
