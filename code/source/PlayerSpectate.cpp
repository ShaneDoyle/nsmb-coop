#include "PlayerSpectate.hpp"

#include <nsmb/nm/game.hpp>
#include <nsmb/nm/sound.hpp>
#include <nsmb/nm/stage/player/player.hpp>
#include <nsmb/nm/stage/layout/data/entrance.hpp>
#include <nsmb/core/math.hpp>
#include <nsmb/core/graphics/fader.hpp>

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
u8 sharedCamera;

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

void enableSharedCamera()
{
	sharedCamera = true;
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
	sharedCamera = false;
}

ncp_call(0x020BB7DC, 0)
void StageLayout_onCreateHook(s32 seqID)
{
	SND::stopRequestedBGM(seqID); // Keep replaced instruction

	for (u32 playerID = 0; playerID < NTR_ARRAY_SIZE(playerTarget); playerID++)
	{
		playerZoom[playerID] = 0x1000;
	}

	// TODO: make this not hardcoded
	if (Entrance::targetAreaID == 174)
		enableSharedCamera();

	*rcast<u8*>(0x020CACA8) = 0;
	*rcast<u8*>(0x020CACD8) = 0;
}

void StageLayout_onUpdateHook()
{
	if (!sharedCamera)
		return;

	fx32* cameraX = rcast<fx32*>(0x020CAE1C);
	fx32* cameraWidth = rcast<fx32*>(0x020CADA4);

	for (u32 i = 0; i < Game::getPlayerCount(); i++)
	{
		Player* player = Game::getPlayer(i);
		if (player->position.x < cameraX[i])
			player->position.x = cameraX[i];
		else if (player->position.x > cameraX[i] + cameraWidth[i])
			player->position.x = cameraX[i] + cameraWidth[i];
	}
}

asm(R"(
ncp_jump(0x020BAC24, 0)
	BL      _ZN14PlayerSpectate24StageLayout_onUpdateHookEv
	LDR     R0, =0x020CA850
	B       0x020BAC28
)");

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

	if (sharedCamera)
	{
		if (playerID == 0)
		{
			for (u32 i = 0; i < Game::getPlayerCount(); i++)
			{
				followCamera(i);
				updateZoom(i);
			}

			Vec3* cameraPos = rcast<Vec3*>(0x020CAEB8);
			cameraPos[0] = (cameraPos[0] + cameraPos[1]) / 2.0fx;
			cameraPos[1] = cameraPos[0];
		}
		return;
	}

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
ncp_repl(0x020AD530, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AD270) StageLayout::setBGLayerCoords
ncp_repl(0x020ADA74, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AD690) StageLayout::updateCameraCoords
ncp_repl(0x020ADB44, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADADC) StageLayout::setCameraSize
ncp_repl(0x020ADC60, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADB50) StageLayout::setScreenAffine
ncp_repl(0x020ADFD4, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020ADC74) StageLayout::setScreenCoords
ncp_repl(0x020AF0B8, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020AEF4C) StageLayout::renderTextures
ncp_repl(0x020B2F8C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B2C58) StageLayout::scrollUpdateGuideTop
ncp_repl(0x020B6E28, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B6B5C) StageLayout::animBGVolcanoEruption
ncp_repl(0x020B8BB8, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B88BC) StageLayout::cameraUpdateScrolling
ncp_repl(0x020B8C50, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B8BDC) StageLayout::getTransitionZoomOffset
ncp_repl(0x020B8D24, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B8CA8) StageLayout::getAbsTilesetPosition
ncp_repl(0x020B963C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B9428) StageLayout::updateVScrollFollow
ncp_repl(0x020B99E0, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020B9900) StageLayout::limitVScroll
ncp_repl(0x020BA32C, 0, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020BA1B4) StageLayout::limitHScroll
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

ncp_call(0x0211881C, 10)
u32 Player_viewTransitState_beginFadeInHook(u8 transitPlayerID)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if ((playerID == transitPlayerID) || (playerTarget[playerID] != transitPlayerID))
			continue;

		// Set the destination entrance
		Entrance::spawnEntrance[playerID] = Entrance::spawnEntrance[transitPlayerID];
		Entrance::spawnEntranceID[playerID] = Entrance::spawnEntranceID[transitPlayerID];

		// Use entrance
		Player* player = Game::getPlayer(playerID);
		player->switchTransitionState(&Player::viewTransitState);
	}

	return Entrance::getSpawnMusic(transitPlayerID); // Keep replaced instruction
}

ncp_repl(0x02118E18, 10, ".int _ZN14PlayerSpectate11localTargetE") // (0x0211870C) Player::viewTransitState

// Misc ----------------

ncp_repl(0x020F6240, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F60F0) LargeFlipFenceRenderer::render
ncp_repl(0x020F7E34, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F7D74) PipeBase::onRender
ncp_repl(0x020F89E4, 10, ".int _ZN14PlayerSpectate10playerZoomE") // (0x020F8854)
ncp_repl(0x02164984, 54, ".int _ZN14PlayerSpectate10playerZoomE") // (0x02164838) Liquid::updateMain

}
