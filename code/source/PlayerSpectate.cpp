#include "PlayerSpectate.hpp"

#include "nsmb/game.hpp"
#include "nsmb/math.hpp"
#include "nsmb/stage/player/player.hpp"

// Notes:
//   - Commenting out the playerID substitution in StageEntity::skipRender
//     can be helpful for identifying desync issues.
//     Specifically, these desyncs may occur when the bone position doesn't
//     update because the animation isn't playing.

namespace PlayerSpectate {

u32 localTarget;
u8 playerTarget[2];
u8 playerLerping[2];

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
	}
}

//ncp_set_call(0x020201A8, getTargetPlayer)

ncp_call(0x020201B0)
void PlayerBase_spectateFollowCamera(PlayerBase* self, u32 playerID)
{
	/*Vec3* cameraPos = rcast<Vec3*>(0x020CAEB8);

	for (s32 i = 0; i < 2; i++)
		Game::getPlayer(i)->followCamera(i);

	cameraPos[playerID] = (cameraPos[0] + cameraPos[1]) / 2.0fx;*/

	auto followCamera = [&](){
		getTargetPlayer(playerID)->followCamera(playerID);
	};

	if (playerLerping[playerID])
	{
		Vec3& cameraPos = rcast<Vec3*>(0x020CAEB8)[playerID];

		// cameraPos holds the old position
		Vec3 resCameraPos = cameraPos;

		followCamera(); // Update cameraPos to hold the new position

		fx32 distanceX = Math::lerpFx32(resCameraPos.x, cameraPos.x, 0x200, 0x6000, 0x1000);
		fx32 distanceY = Math::lerpFx32(resCameraPos.y, cameraPos.y, 0x200, 0x6000, 0x1000);

		cameraPos = resCameraPos;

		if (distanceX == 0 && distanceY == 0)
			playerLerping[playerID] = false;
	}
	else
	{
		followCamera();
	}
}

}
