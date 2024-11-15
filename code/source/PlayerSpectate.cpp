#include "PlayerSpectate.hpp"

#include "nsmb/game.hpp"
#include "nsmb/math.hpp"
#include "nsmb/sound.hpp"
#include "nsmb/stage/player/player.hpp"

// Notes:
//   - Commenting out the playerID substitution in StageEntity::skipRender
//     can be helpful for identifying desync issues.
//     Specifically, these desyncs may occur when the bone position doesn't
//     update because the animation isn't playing.

asm(R"(
	_ZN5Stage4zoomE = 0x020CADB4
)");
namespace Stage {
	extern fx32 zoom[2];
}

namespace PlayerSpectate {

u32 localTarget;
u8 playerTarget[2];
u8 playerLerping[2];
u8 playerLerpingZoom[2];
fx32 playerZoom[2];

u32 getTarget(u32 playerID)
{
	return playerTarget[playerID];
}

void setTarget(u32 playerID, u32 targetPlayerID)
{
	playerTarget[playerID] = targetPlayerID;

	if (playerID == Game::localPlayerID)
		localTarget = targetPlayerID;
}

void setLerping(u32 playerID, bool lerping)
{
	playerLerping[playerID] = lerping;
	playerLerpingZoom[playerID] = lerping;
}

bool isSpectating(u32 playerID)
{
	return playerTarget[playerID] != playerID;
}

Player* getTargetPlayer(u32 playerID)
{
	return Game::getPlayer(playerTarget[playerID]);
}

Player* getLocalTargetPlayer()
{
	return Game::getPlayer(localTarget);
}

void reset()
{
	for (u32 playerID = 0; playerID < NTR_ARRAY_SIZE(playerTarget); playerID++)
	{
		setTarget(playerID, playerID);
		playerLerping[playerID] = false;
		playerLerpingZoom[playerID] = false;
	}
}

ncp_call(0x020BB7DC, 0)
void StageLayout_onCreateHook(s32 seqID)
{
	SND::stopRequestedBGM(seqID); // Keep replaced instruction

	for (u32 playerID = 0; playerID < NTR_ARRAY_SIZE(playerTarget); playerID++)
	{
		playerZoom[playerID] = 0x1000;
	}
}

ncp_call(0x020201B0)
void PlayerBase_spectateFollowCamera(PlayerBase* self, u32 playerID)
{
	auto followCamera = [](u32 playerID)
	{
		getTargetPlayer(playerID)->followCamera(playerID);
	};

	auto updateZoom = [](u32 playerID)
	{
		u8 targetPlayerID = playerTarget[playerID];

		if (playerLerpingZoom[playerID])
		{
			fx32 distance = Math::lerpFx32(playerZoom[playerID], Stage::zoom[targetPlayerID], 0x80, 0x800, 0x80);

			if (distance == 0)
				playerLerpingZoom[playerID] = false;
		}
		else
		{
			playerZoom[playerID] = Stage::zoom[targetPlayerID];
		}
	};

	if (playerLerping[playerID])
	{
		Vec3& cameraPos = rcast<Vec3*>(0x020CAEB8)[playerID];

		// cameraPos holds the old position
		Vec3 resCameraPos = cameraPos;

		followCamera(playerID); // Update cameraPos to hold the new position

		fx32 distanceX = Math::lerpFx32(resCameraPos.x, cameraPos.x, 0x200, 0x6000, 0x1000);
		fx32 distanceY = Math::lerpFx32(resCameraPos.y, cameraPos.y, 0x200, 0x6000, 0x1000);

		cameraPos = resCameraPos;

		if (distanceX < 48fx && distanceY < 48fx)
		{
			if (distanceX == 0 && distanceY == 0)
				playerLerping[playerID] = false;

			updateZoom(playerID);
		}
	}
	else
	{
		followCamera(playerID);
		updateZoom(playerID);
	}
}

ncp_repl(0x0200DD78, ".int _ZN14PlayerSpectate10playerZoomE") // (0x0200DC48) OAM::loadAffineSets
ncp_repl(0x02012028, ".int _ZN14PlayerSpectate10playerZoomE") // (0x02011F5C) SND::updateScreenBoundaries
ncp_repl(0x0209BA7C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x0209B7E8) ???::spawnActorsInRange
ncp_repl(0x020AD530, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AD270) StageLayoutBase::scrollTheScreen
ncp_repl(0x020ADA74, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AD690) StageLayoutBase::thisSetsCameraXandY
ncp_repl(0x020ADB44, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADADC) StageLayoutBase::updateCameraWidthAndHeight
ncp_repl(0x020ADC60, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADB50) StageLayoutBase::???
ncp_repl(0x020ADFD4, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADC74) StageLayoutBase::???
ncp_repl(0x020AF0B8, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AEF4C) StageLayoutBase::???
ncp_repl(0x020B2F8C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B2C58) StageLayoutBase::???
ncp_repl(0x020B6E28, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B6B5C) StageLayoutIdk::animateVolcanoBG
ncp_repl(0x020B8BB8, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B88BC) StageLayoutIdk::doSomeBgCameraThings
ncp_repl(0x020B8C50, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B8BDC) StageLayoutIdk::???
ncp_repl(0x020B8D24, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B8CA8) StageLayoutIdk::doNotLetZoomedCameraEscapeView
ncp_repl(0x020B963C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B9428) StageLayoutIdk::???
ncp_repl(0x020B99E0, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B9900) StageLayoutIdk::???
ncp_repl(0x020BA32C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020BA1B4) StageLayoutIdk::???
ncp_repl(0x020BACC8, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020BAA5C) StageLayout::onUpdate

// StageCamera ----------------

ncp_set_call(0x020CE024, 10, getLocalTargetPlayer)
ncp_set_call(0x020CE244, 10, getLocalTargetPlayer)
// //ncp_repl(0x020CE654, 10, ".int _ZN14PlayerSpectate11localTargetE")
ncp_repl(0x020CE424, 10, ".int _ZN14PlayerSpectate10playerZoomE")

// Player ----------------

// Fix the player rendering using this->linkedPlayerID instead of Game::localPlayerID
asm(R"(
ncp_jump(0x020FD0C4, 10)
	LDR     R2, =_ZN4Game13localPlayerIDE
	LDR     R2, [R2]
	B       0x020FD0C8
)");

// Misc ----------------

ncp_repl(0x020F6240, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F60F0) LargeFlipFenceRenderer::render
ncp_repl(0x020F7E34, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F7D74) PipeBase::onRender
ncp_repl(0x020F89E4, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F8854)
ncp_repl(0x02164984, 54, ".int _ZN14PlayerSpectate10playerZoomE") // (0x02164838) Liquid::updateMain

}
