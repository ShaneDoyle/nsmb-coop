#include "coop/fixes/bosses/BossControllerCommon.hpp"

#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/system/function.hpp>

#include "coop/CoopActor.hpp"
#include "coop/CoopStage.hpp"
#include "coop/fixes/bosses/BossController.hpp"
#include "coop/fixes/bosses/FinalBossController.hpp"

extern "C" {
	void CoopSym_BossController_bindCameraToZone(StageEntity3DAnm* self);
	bool CoopSym_BossController_transitionState(StageEntity3DAnm* self);
	bool CoopSym_FinalBossController_transitionState(StageEntity* self);
}

namespace CoopFixes::BossControllerCommon {

ncp_thumb bool coopTransitionState(
	StageEntity* self,
	s32& step,
	CoopTransitionStateInfo* info
)
{
	const u32 FadeWaitDurationFrames = 30;

	if (step == Func::Init)
	{
		step++;

		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			// Begin fade out
			Game::fader.fadeMaskShape[playerID] = FadeMask::getCharacterFadeMaskID(Game::getPlayerCharacter(playerID));
			Game::fader.fadingState[playerID] |= 0x28;
		}

		return true;
	}

	if (step == Func::Exit)
	{
		return true;
	}

	if (step == 1)
	{
		// Wait for fade out
		if ((Game::fader.fadingState[0] & 8) == 0)
			step++;

		goto commonEnd;
	}

	if (step == 2)
	{
		step++;

		Player* closestPlayer = CoopActor::getClosestPlayer(self);
		closestPlayer->beginCutscene(true);

		// Other players
		s32 order = 1;
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (closestPlayer->linkedPlayerID == playerID || Game::getPlayerDead(playerID))
				continue;

			Player* player = Game::getPlayer(playerID);
			player->position.x = closestPlayer->position.x - 16fx * order;
			player->position.y = closestPlayer->position.y;
			player->updateCollision(false); // Prevent falling through semi-solid
			player->beginCutscene(true);

			order++;
		}

		goto commonEnd;
	}

	if (step == 3)
	{
		step++;

		Player* closestPlayer = CoopActor::getClosestPlayer(self);

		fx32 zoneX = info->getZoneX(self);

		CoopStage::setCameraBoundAll(Stage::stageLayout, zoneX >> FX32_SHIFT, 1);

		// Match the camera of the player in front

		if (Stage::cameraX[closestPlayer->linkedPlayerID] > zoneX)
		{
			for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			{
				if (closestPlayer->linkedPlayerID == playerID || Game::getPlayerDead(playerID))
					continue;

				CoopStage::matchPlayerCameraBounds(playerID, closestPlayer->linkedPlayerID);
			}
		}

		goto commonEnd;
	}

	if (step < 3 + FadeWaitDurationFrames)
	{
		step++;
		// Make up time for the background to catch up with the camera
		goto commonEnd;
	}

	if (step == 3 + FadeWaitDurationFrames)
	{
		step++;

		// Begin fade in
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			Game::fader.fadingState[playerID] |= 0x5;

		goto commonEnd;
	}

	if (step == 4 + FadeWaitDurationFrames)
	{
		// Wait for fade in
		if ((Game::fader.fadingState[0] & 1) != 0)
			goto commonEnd;

		info->exitState(self);
		goto commonEnd;
	}

commonEnd:
	info->commonEnd(self);
	return true;
}

ncp_thumb void setupCoopTransitionState(Player* closestPlayer)
{
	s32 playerCount = Game::getPlayerCount();

	if (playerCount == 1)
	{
		Game::getPlayer(Game::localPlayerID)->beginCutscene(true);

#ifdef COOP_FIX_BOSS_CONTROLLER
		BossController::sCustomTransition.func = CoopSym_BossController_transitionState;
#endif
#ifdef COOP_FIX_FINAL_BOSS_CONTROLLER
		FinalBossController::sCustomTransition.func = CoopSym_FinalBossController_transitionState;
#endif
	}
	else
	{
		for (s32 playerID = 0; playerID < playerCount; playerID++)
			Game::getPlayer(playerID)->beginCutscene(false); // Gets set to true later in coopTransitionState

#ifdef COOP_FIX_BOSS_CONTROLLER
		BossController::sCustomTransition.func = BossController::coopTransitionState;
#endif
#ifdef COOP_FIX_FINAL_BOSS_CONTROLLER
		FinalBossController::sCustomTransition.func = FinalBossController::coopTransitionState;
#endif
	}
}

ncp_thumb void beginCutsceneAllPlayers()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->beginCutscene(false);
	}
}

ncp_thumb void endCutsceneAllPlayers()
{
	s32 playerCount = Game::getPlayerCount();

	for (s32 playerID = 0; playerID < playerCount; playerID++)
	{
		Game::getPlayer(playerID)->endCutscene();
	}

	if (playerCount > 1)
	{
		// End the coop transition by restoring the camera
		StageEntity3DAnm* bossController = scast<StageEntity3DAnm*>(ProcessManager::getNextObjectByObjectID(114));
		if (bossController != nullptr)
		{
			CoopSym_BossController_bindCameraToZone(bossController);
		}
	}
}

}
