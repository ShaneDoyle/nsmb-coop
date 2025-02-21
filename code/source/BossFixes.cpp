#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/player/door.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/graphics/3d/modelanm.hpp>
#include <nsmb/core/graphics/3d/blendmodelanm.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "ActorFixes.hpp"
#include "PlayerSpectate.hpp"
#include "Stage.hpp"


// VERY IMPORTANT
// CURRENTLY THE BOWSER FRAME HEAP ALLOCATION IS DEALLOCATED UNSAFELY
// IT IS ALMOST PURE LUCK THAT IT WORKS, SO PLEASE LOOK AT IT AFTER THIS RELEASE!

asm(R"(
	_ZN5Stage9exitLevelEm = 0x020A189C
)");
namespace Stage {
	void exitLevel(u32 flag);
}

//============================= Main Camera Push =============================

u32 BossFixes_setCameraBoundPlayerID = 0;

// Allow camera to be pushed for all players

asm(R"(
BossFixes_setCameraBound:
	LDR     R12, =BossFixes_setCameraBoundPlayerID
	B       0x020ACF54
)");

extern "C" void BossFixes_setCameraBound(StageLayout* self, s16 bound, u32 side);

ncp_jump(0x020ACF50, 0)
void BossFixes_setCameraBoundAll(StageLayout* self, s16 bound, u32 side)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		BossFixes_setCameraBoundPlayerID = playerID;
		BossFixes_setCameraBound(self, bound, side);
	}
}

void BossFixes_matchPlayerCameraBounds(s32 playerID, s32 matchPlayerID)
{
	s16 boundX = Stage::cameraX[matchPlayerID] >> FX32_SHIFT;

	BossFixes_setCameraBoundPlayerID = playerID;
	BossFixes_setCameraBound(Stage::stageLayout, boundX, 1);
	BossFixes_setCameraBound(Stage::stageLayout, boundX + 256, 0);
}

void BossFixes_setZoomAll(fx32 zoom, u32 delay, u8 zero, u8 unk)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		Stage::setZoom(zoom, delay, playerID, unk);
}

//============================= Main Boss Controller =============================

struct BossController_PTMF
{
	bool (*func)(StageEntity3DAnm*);
	u32 adj;
};

static BossController_PTMF BossController_sCustomTransition = { nullptr, 0 };

ncp_over(0x02143994, 40)
const static BossController_PTMF* BossController_sCustomTransition_ptr = &BossController_sCustomTransition;

asm(R"(
	BossController_bindCameraToZone = 0x02142F14
	BossController_transitionState = 0x02143550
	BossController_switchState = 0x021439EC
	BossController_sTransition = 0x02146C08
)");

extern "C"
{
	void BossController_bindCameraToZone(StageEntity3DAnm* self);
	void BossController_switchState(StageEntity3DAnm* self, BossController_PTMF* ptmf);
	bool BossController_transitionState(StageEntity3DAnm* self);
	BossController_PTMF BossController_sTransition;
}

bool BossController_coopTransitionState(StageEntity3DAnm* self)
{
	const u32 FadeWaitDurationFrames = 30;

	s8& step = rcast<s8*>(self)[0x53A];

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

	auto commonEnd = [self]() -> bool {
		self->model.frameController.update();
		return true;
	};

	if (step == 1)
	{
		// Wait for fade out
		if ((Game::fader.fadingState[0] & 8) == 0)
			step++;

		return commonEnd();
	}

	if (step == 2)
	{
		step++;

		Player* closestPlayer = ActorFixes_getClosestPlayer(self);
		closestPlayer->beginCutscene(true);

		// Other players
		s32 order = 1;
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (closestPlayer->linkedPlayerID == playerID)
				continue;

			Player* player = Game::getPlayer(playerID);
			player->position.x = closestPlayer->position.x - 16fx * order;
			player->position.y = closestPlayer->position.y;
			player->updateCollision(false); // Prevent falling through semi-solid
			player->beginCutscene(true);

			order++;
		}

		return commonEnd();
	}

	if (step == 3)
	{
		step++;

		// Match the camera of the player in front

		Player* closestPlayer = ActorFixes_getClosestPlayer(self);

		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (closestPlayer->linkedPlayerID == playerID)
				continue;

			BossFixes_matchPlayerCameraBounds(playerID, closestPlayer->linkedPlayerID);
		}

		return commonEnd();
	}

	if (step < 3 + FadeWaitDurationFrames)
	{
		step++;
		// Make up time for the background to catch up with the camera
		return commonEnd();
	}

	if (step == 3 + FadeWaitDurationFrames)
	{
		step++;

		// Begin fade in
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			Game::fader.fadingState[playerID] |= 0x5;

		return commonEnd();
	}

	if (step == 4 + FadeWaitDurationFrames)
	{
		// Wait for fade in
		if ((Game::fader.fadingState[0] & 1) != 0)
			return commonEnd();

		BossController_switchState(self, &BossController_sTransition);
		return commonEnd();
	}

	return commonEnd();
}

// Setup the cutscene transition
ncp_call(0x021438AC, 40)
void call_021438AC_ov40(Player* closestPlayer)
{
	s32 playerCount = Game::getPlayerCount();

	if (playerCount == 1)
	{
		Game::getPlayer(Game::localPlayerID)->beginCutscene(true);
		BossController_sCustomTransition.func = BossController_transitionState;
	}
	else
	{
		for (s32 playerID = 0; playerID < playerCount; playerID++)
			Game::getPlayer(playerID)->beginCutscene(false); // Gets set to true later in BossController_coopTransitionState

		BossController_sCustomTransition.func = BossController_coopTransitionState;
	}
}

ncp_call(0x0214393C, 40)
void BossController_customSetCameraBoundAll(StageLayout* self, fx32 bound, u32 side)
{
	if (Game::getPlayerCount() == 1)
		BossFixes_setCameraBoundAll(self, bound, side);

	// For coop it gets handled by BossController_coopTransitionState
}

ncp_call(0x02142A38, 40)
void BossController_customBindCameraToZone(StageEntity3DAnm* self)
{
	if (Game::getPlayerCount() == 1)
		BossController_bindCameraToZone(self);

	// For coop it gets handled by the unfreezing of each actor
}

// ============================= Misc =============================

asm("SetupFSCacheToUseOverlay55 = 0x021726C0");
extern "C" void SetupFSCacheToUseOverlay55();

void BossFixes_beginCutsceneAllPlayers()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->beginCutscene(false);
	}
}

void BossFixes_endCutsceneAllPlayers()
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
			BossController_bindCameraToZone(bossController);
		}
	}
}

//============================= Bowser Jr. =============================

ncp_repl(0x0213CA5C, 28, "MOV R0, R4")
ncp_set_call(0x0213CA70, 28, ActorFixes_isOutsideCamera)

ncp_repl(0x0213CC70, 28, "MOV R0, R4")
ncp_set_call(0x0213CC84, 28, ActorFixes_isOutsideCamera)

// Victory freeze on ground touch

ncp_call(0x0213D0BC, 28)
void call_0213D0BC_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->actionFlag.bowserJrBeaten = true;
	}
}

// ov28:021FF154 (dunno what it does, doesn't desync, might not need changing)

// Freeze the player while falling

ncp_repl(0x0213F49C, 28, "NOP")

ncp_call(0x0213F4A0, 28)
void call_0213F4A0_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		player->subActionFlag.releaseKeys = true;
		player->velH = 0;
	}
}

// Freeze player when touching the ground after falling

ncp_set_call(0x0213F4B4, 28, BossFixes_beginCutsceneAllPlayers)

// Unfreeze when battle is ready to start

ncp_set_call(0x0213F550, 28, BossFixes_endCutsceneAllPlayers)

// Fix Blue Shell hit

ncp_repl(0x0213FF54, 28, "ADD R0, R4, #0x100")
ncp_repl(0x0213FF5C, 28, "LDRSB R0, [R0,#0x1E]")

// Fix Ground Pound

ncp_repl(0x0213FFC4, 28, "ADD R1, R0, #0x100")
ncp_repl(0x0213FFCC, 28, "LDRSB R0, [R1,#0x1E]")

// Bowser Jr. camera spawn fix
asm(R"(
ncp_jump(0x0213BFA8, 28)
	LDR     R1, =0x1000001
	B       0x0213BFAC
)");

//============================= World 1: Bowser =============================

ncp_call(0x0215E850, 54)
void BossFixes_doNotLoadDoorModels()
{
	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (areaNum == 19 || areaNum == 175)
		return;
	Door::loadModels();
}

//ncp_set_call(0x02138808, 13, FS::Cache::loadFileToOverlay) // Bowser's Model
ncp_set_call(0x02138814, 13, FS::Cache::loadFileToOverlay) // Bowser's Animations
ncp_repl(0x02138820, 13, "NOP") // Dry Bones Bowser Model
ncp_repl(0x0213882C, 13, "NOP") // Dry Bones Bowser Animations

// Skip Dry Bones Bowser model setup (it's done later)
ncp_repl(0x021385BC, 13, R"(
	ADD     SP, SP, #0x24
	MOV     R0, #1
	POP     {R4-R7,PC}
)");

ncp_call(0x02133064, 13)
void BossFixes_bowserW1_loadFix(ModelAnm* self, u32 animID, FrameCtrl::Type type, fx32 speed, u16 startFrame)
{
	// -- Unload the model from RAM

	FS::Cache::unloadFile(0x576);
	FS::Cache::unloadFile(0x577);

	// -- Free all files in overlay 55

	SetupFSCacheToUseOverlay55();

	// -- Load the Dry Bones Bowser model

	void* bmd = FS::Cache::loadFileToOverlay(0x4D9, 0);
	void* bca = FS::Cache::loadFileToOverlay(0x4DA, 0);
	self->create(bmd, bca, 0, 0, 0);
	self->init(animID, type, speed, startFrame);
}

// Unfreeze both players

ncp_set_call(0x0213695C, 13, BossFixes_endCutsceneAllPlayers)

// Fix fireball tracking

asm(R"(
ncp_jump(0x02138D7C, 13)
	PUSH    {R0,R2,LR}
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
	ADD     R3, R0, #0x100
	LDRSB   R3, [R3,#0x1E] // linkedPlayerID
	POP     {R0,R2,LR}
	B       0x02138D80
)");

// Disables "StageZoom" for BowserBattleSwitch and applies "Victory" animation
/*ncp_call(0x0213A7A4, 13)
void call_0213A7A4_ov13()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->actionFlag.bowserJrBeaten = true;
	}
}*/

struct BossBattleSwitch_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

asm(R"(
	Bowser_getBattleState = 0x0213884C
	BossBattleSwitch_switchState = 0x0213B1DC
	Liquid_doWaves = 0x021646E0
)");

extern "C"
{
	s32 Bowser_getBattleState();
	bool BossBattleSwitch_switchState(StageEntity* self, BossBattleSwitch_PTMF ptmf);
	void Liquid_doWaves(fx32 x, u32 one);
}

Player* BossBattleSwitch_linkedPlayer = nullptr;
fx32 BossBattleSwitch_linkedPlayerCameraX = 0;
Vec3 BossBattleSwitch_cutsceneStartPos;
u8 BossBattleSwitch_fakePlayerDeathTimer[2];

// For the players that didn't hit the switch
bool Player_bossDefeatNotLinkedTransitState(Player* self, void* arg)
{
	const u32 FakeDeathDuration = 120;

	const u32 STEP_WaitVictoryPose = 1;
	const u32 STEP_WaitWallBreak = 2;
	const u32 STEP_WaitGroundLand = 3;
	const u32 STEP_WaitPeachReact = 4;
	const u32 STEP_MissedCutscene = 5;

	s32 playerID = self->linkedPlayerID;

	s8& step = self->transitionStateStep;
	u8& deathTimer = BossBattleSwitch_fakePlayerDeathTimer[playerID];

	if (step == Func::Init)
	{
		self->velocity.x = 0;
		self->velocity.y = 0;
		deathTimer = 0;

		bool grounded = (scast<u32>(self->collisionMgr.bottomResult) & CollisionMgr::Result::GroundAny);
		if (!grounded)
		{
			// Fall and try to join in on the animation later
			self->setAnimation(6, true, Player::FrameMode::Restart, 1fx);
			self->subActionFlag.releaseKeys = true;

			step = STEP_WaitGroundLand;
		}
		else
		{
			step = STEP_WaitVictoryPose;
		}

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	auto victoryDancing = [&]{ return BossBattleSwitch_linkedPlayer->transitionStateStep >= 3; };
	auto wallsBreaking = [&]{ return BossBattleSwitch_linkedPlayer->transitionStateStep >= 6; };
	auto peachReacting = [&]{ return BossBattleSwitch_linkedPlayer->transitionStateStep >= 10; };

	if (step == STEP_WaitVictoryPose)
	{
		// Wait for player that hit the button to start animation
		if (victoryDancing())
		{
			// Begin victory pose
			self->rotation.y = 0;
			self->bossBeginVictoryPose();

			step = STEP_WaitWallBreak;
		}
	}
	else if (step == STEP_WaitWallBreak)
	{
		// Wait for barrier blocks to start breaking
		if (wallsBreaking())
		{
			if (Math::abs(BossBattleSwitch_linkedPlayer->position.x - self->position.x) < (16fx * 4fx))
			{
				// Player is with the one that hit the switch
				self->switchTransitionState(&Player::bossVictoryTransitState);
				step = 5; // Skip some steps in the state we just switched to
			}
			else
			{
				// Player is not near the one that hit the switch
				self->setAnimation(0, true, Player::FrameMode::Restart, 1fx);
				PlayerSpectate::setLerping(playerID, true);
				PlayerSpectate::setTarget(playerID, BossBattleSwitch_linkedPlayer->linkedPlayerID);

				step = STEP_WaitPeachReact;
			}
		}
	}
	else if (step == STEP_WaitGroundLand)
	{
		self->updateGravityAcceleration();
		self->updateVerticalVelocityClamped();
		self->applyVelocity();

		bool grounded = (scast<u32>(self->collisionMgr.updatePlayerGroundCollision()) & CollisionMgr::Result::GroundAny);

		if (grounded)
		{
			if (wallsBreaking())
			{
				self->setAnimation(0, true, Player::FrameMode::Restart, 1fx);
				step = STEP_WaitWallBreak;
			}
			else
			{
				step = STEP_WaitVictoryPose;
			}
		}
		else
		{
			// If barrier blocks started breaking and we haven't landed yet
			if (wallsBreaking())
			{
				// Then the player missed the cutscene
				PlayerSpectate::setLerping(playerID, true);
				PlayerSpectate::setTarget(playerID, BossBattleSwitch_linkedPlayer->linkedPlayerID);

				step = STEP_MissedCutscene;
			}
		}
	}
	else if (step == STEP_WaitPeachReact)
	{
		// Wait for Peach react animation
		if (peachReacting())
		{
			self->position = BossBattleSwitch_cutsceneStartPos;

			self->switchTransitionState(&Player::bossVictoryTransitState);
			step = 10; // Skip some steps in the state we just switched to
		}
	}

	// Custom lava death
	if (deathTimer < FakeDeathDuration)
	{
		if (Game::getPlayerDead(playerID))
		{
			deathTimer++;
		}
		else if (self->position.y < Stage::liquidPosition[Game::localPlayerID])
		{
			self->playSFXUnique(338, &self->position);
			Liquid_doWaves(self->position.x, 1);
			Game::losePlayerLife(playerID);
			Game::setPlayerDead(playerID, true);
		}
	}
	else if (deathTimer == FakeDeathDuration)
	{
		Stage_isPlayerDead[playerID] = true;
		deathTimer++;
	}

	self->updateAnimation();

	return true;
}

void Player_beginBossDefeatCutsceneNotLinked(Player* self)
{
	self->switchMainState(&Player::idleState);
	self->switchTransitionState(ptmf_cast(Player_bossDefeatNotLinkedTransitState));
}

Player* BossBattleSwitch_onBowserDead(Player* linkedPlayer)
{
	if (Game::getPlayerCount() == 1)
		return linkedPlayer;

	BossBattleSwitch_linkedPlayer = linkedPlayer;
	BossBattleSwitch_cutsceneStartPos = linkedPlayer->position;

	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (playerID == linkedPlayer->linkedPlayerID || Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		Player_beginBossDefeatCutsceneNotLinked(player);
	}

	return linkedPlayer;
}

void BossBattleSwitch_afterHitState_beforeUpdate(StageEntity3DAnm* self)
{
	BossBattleSwitch_linkedPlayerCameraX = Stage::cameraX[self->linkedPlayerID];
}

asm(R"(
// Store the player ID that hit the switch
ncp_over(0x0213B2DC, 13) /* max over: 0x30 bytes, current: 0x2C bytes */
	LDRB    R2, [R1,#0x11C]
	CMP     R2, #1
	BXNE    LR
	LDRB    R1, [R1,#0x11E]
	STRB    R1, [R0,#0x11E]
	ADD     R0, R0, 0x400
	LDRH    R1, [R0,#0xA6]
	CMP     R1, #0
	MOVEQ   R1, #1
	STRHEQ  R1, [R0,#0xAE]
	BX      LR
ncp_endover()

ncp_jump(0x0213A7AC, 13)
	BL      _ZN4Game9getPlayerEl
	BL      _Z29BossBattleSwitch_onBowserDeadP6Player
	B       0x0213A7B0

ncp_jump(0x0213A53C, 13)
	PUSH    {R1}
	MOV     R0, R5
	BL      _Z43BossBattleSwitch_afterHitState_beforeUpdateP16StageEntity3DAnm
	POP     {R1}
	LDRSH   R0, [R1,#0xA2]
	B       0x0213A540
)");

// Use the stored player ID
ncp_repl(0x0213A718, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213A7A8, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213A8AC, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213AF14, 13, "LDRB R0, [R5,#0x11E]")

ncp_set_call(0x0213A6F8, 13, BossFixes_setZoomAll)
ncp_set_call(0x0213A784, 13, BossFixes_setZoomAll)
ncp_set_call(0x0213A7A4, 13, BossFixes_setZoomAll)
ncp_set_call(0x0213A914, 13, BossFixes_setZoomAll)
ncp_set_call(0x0213AA8C, 13, BossFixes_setZoomAll)
ncp_set_call(0x0213ABC8, 13, BossFixes_setZoomAll)

ncp_repl(0x0213AD10, 13, ".int BossBattleSwitch_linkedPlayerCameraX")

//============================= World 2: Mummy Pokey =============================

ncp_over(0x02133AF0, 16) const auto MummyPokey_skipRender = ActorFixes_safeSkipRender;

ncp_set_call(0x02131EDC, 16, BossFixes_endCutsceneAllPlayers)

ncp_repl(0x0213298C, 16, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Fix ground pound hit

ncp_repl(0x021327EC, 16, "ADD R0, R5, #0x100") // Fix shell hit
ncp_repl(0x021327FC, 16, "LDRSB R0, [R0,#0x1E]") // Fix shell hit

//============================= World 3: Cheepskipper =============================

ncp_set_call(0x02131748, 18, BossFixes_endCutsceneAllPlayers)

//============================= World 4: Mega Goomba =============================

// Unfreeze both players
ncp_set_call(0x0213137C, 14, BossFixes_endCutsceneAllPlayers)

//============================= World 8: Final Bowser =============================

asm(R"(
	func20F4660 = 0x020F4660
	FinalBowser_loadResources = 0x02138724
)");
extern "C" {
	void func20F4660();
	void FinalBowser_loadResources();
}

ncp_call(0x020AF2E4, 0)
void BossFixes_doNotLoadCastleModel()
{
	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (areaNum == 19 || areaNum == 175)
		return;
	func20F4660();
}

ncp_set_call(0x021487E0, 43, FS::Cache::loadFile) // Load pot.nsbmd to memory instead of overlay
ncp_set_call(0x021487EC, 43, FS::Cache::loadFile) // Load pot.nsbca to memory instead of overlay

ncp_repl(0x02148814, 43, "NOP") // Do not load Final Bowser resources yet

ncp_call(0x0214793C, 43)
void BossFixes_bowserFinal_loadFix(ModelAnm* self, u32 animID, FrameCtrl::Type type, fx32 speed, u16 startFrame)
{
	// -- Unload the model from RAM

	FS::Cache::unloadFile(1235);
	FS::Cache::unloadFile(1236);
	FS::Cache::unloadFile(1237);
	FS::Cache::unloadFile(1238);
	FS::Cache::unloadFile(1239);

	// -- Free all files in overlay 55

	SetupFSCacheToUseOverlay55();

	// -- Load the Final Bowser model

	FinalBowser_loadResources();

	self->init(animID, type, speed, startFrame); // Keep replaced instruction
}

ncp_set_call(0x02138738, 13, FS::Cache::loadFile) // koopa new nsbmd
ncp_set_call(0x02138744, 13, FS::Cache::loadFileToOverlay) // koopa new nsbca

ncp_set_call(0x0213A018, 13, FS::Cache::loadFileToOverlay) // koopa fire 1 nsbmd
ncp_set_call(0x0213A024, 13, FS::Cache::loadFileToOverlay) // koopa fire 1 nsbta
ncp_set_call(0x0213A030, 13, FS::Cache::loadFileToOverlay) // koopa fire 2 nsbmd
ncp_set_call(0x0213A03C, 13, FS::Cache::loadFileToOverlay) // koopa fire 2 nsbta

//============================= Boss Key =============================

// Victory freeze

ncp_call(0x0214619C, 40)
void call_0214619C_ov40()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->physicsFlag.bossDefeated = true;
	}
}

//============================= Mini-mushroom Cutscene =============================

ncp_call(0x020A1D00, 0)
void BossFixes_levelEnd_hook(u32 flag)
{
	auto switchToCutsceneArea = [](u8 stage){
		Entrance::targetAreaID = Stage::getAreaID(9, stage, 0);
		Entrance::targetEntranceID = 0;
		Entrance::switchArea();
	};

	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (areaNum == 42) // World 2
	{
		switchToCutsceneArea(0);
	}
	else if (areaNum == 105) // World 5
	{
		switchToCutsceneArea(1);
	}
	else
	{
		Stage::exitLevel(flag);
	}
}

static u8 s_goToMiniWorld = false;
static u32 getGoToMiniWorld() { return s_goToMiniWorld; }

ncp_repl(0x020CE2E0, 8, "CMP R0, #1")
ncp_repl(0x020CE300, 8, "CMP R0, #1")
ncp_set_call(0x020CE2A8, 8, getGoToMiniWorld)

ncp_call(0x02119684, 10)
void BossFixes_finishLevelOnTransit_hook()
{
	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (areaNum == 180 || areaNum == 181)
	{
		s_goToMiniWorld = Entrance::targetEntranceID == 2;
		Stage::exitLevel(1);
		return;
	}
	Entrance::switchArea();
}
