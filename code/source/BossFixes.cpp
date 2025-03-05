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
#include "Player.hpp"
#include "Stage.hpp"

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

//============================= Boss Controller Common =============================

struct BossControllerCommon_CoopTransitionStateInfo
{
	fx32(*getZoneX)(StageEntity*);
	void(*exitState)(StageEntity*);
	void(*commonEnd)(StageEntity*);
};

bool BossControllerCommon_coopTransitionState(
	StageEntity* self,
	s32& step,
	BossControllerCommon_CoopTransitionStateInfo* info
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

		Player* closestPlayer = ActorFixes_getClosestPlayer(self);
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

		Player* closestPlayer = ActorFixes_getClosestPlayer(self);

		fx32 zoneX = info->getZoneX(self);

		BossFixes_setCameraBoundAll(Stage::stageLayout, zoneX >> FX32_SHIFT, 1);

		// Match the camera of the player in front

		if (Stage::cameraX[closestPlayer->linkedPlayerID] > zoneX)
		{
			for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			{
				if (closestPlayer->linkedPlayerID == playerID || Game::getPlayerDead(playerID))
					continue;

				BossFixes_matchPlayerCameraBounds(playerID, closestPlayer->linkedPlayerID);
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

void BossControllerCommon_setupCoopTransitionState(Player* closestPlayer);

//============================= Boss Controller =============================

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

BossControllerCommon_CoopTransitionStateInfo BossController_coopTransitionStateInfo =
{
	.getZoneX = [](StageEntity* self)
	{
		return rcast<fx32*>(self)[0x514/4];
	},

	.exitState = [](StageEntity* self)
	{
		BossController_switchState(scast<StageEntity3DAnm*>(self), &BossController_sTransition);
	},

	.commonEnd = [](StageEntity* self)
	{
		scast<StageEntity3DAnm*>(self)->model.frameController.update();
	}
};

bool BossController_coopTransitionState(StageEntity3DAnm* self)
{
	s8& step = rcast<s8*>(self)[0x53A];
	s32 stepArg = step;
	bool result = BossControllerCommon_coopTransitionState(self, stepArg, &BossController_coopTransitionStateInfo);
	step = stepArg;
	return result;
}

// Setup the cutscene transition
ncp_set_call(0x021438AC, 40, BossControllerCommon_setupCoopTransitionState)

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

ncp_repl(0x0214310C, 40, "MOV R0, R4")
ncp_set_call(0x02143120, 40, ActorFixes_isOutsideCamera)

//============================= Final Boss Controller =============================

struct FinalBossController_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

static FinalBossController_PTMF FinalBossController_sCustomTransition = { nullptr, 0 };

ncp_over(0x02148574, 43)
const static FinalBossController_PTMF* FinalBossController_sCustomTransition_ptr = &FinalBossController_sCustomTransition;

asm(R"(
	FinalBossController_transitionState = 0x021480A0
	FinalBossController_switchState = 0x02148580
	FinalBossController_sTransition = 0x02148AF0
)");

extern "C"
{
	void FinalBossController_switchState(StageEntity* self, FinalBossController_PTMF ptmf);
	bool FinalBossController_transitionState(StageEntity* self);
	FinalBossController_PTMF FinalBossController_sTransition;
}

BossControllerCommon_CoopTransitionStateInfo FinalBossController_coopTransitionStateInfo =
{
	.getZoneX = [](StageEntity* self)
	{
		Rectangle<fx32> zoneBox;
		StageZone::get(0, &zoneBox);
		return zoneBox.x;
	},

	.exitState = [](StageEntity* self)
	{
		FinalBossController_switchState(self, FinalBossController_sTransition);
	},

	.commonEnd = [](StageEntity* self)
	{

	},
};

bool FinalBossController_coopTransitionState(StageEntity* self)
{
	s16& step = rcast<s16*>(self)[0xAC0/2];
	s32 stepArg = step;
	bool result = BossControllerCommon_coopTransitionState(self, stepArg, &FinalBossController_coopTransitionStateInfo);
	step = stepArg;
	return result;
}

// Setup the cutscene transition
ncp_set_call(0x021482AC, 43, BossControllerCommon_setupCoopTransitionState)

// For coop binding the camera bounds gets handled by FinalBossController_coopTransitionState
asm(R"(
ncp_jump(0x021482F0, 43)
	BL      _ZN9StageZone3getEhP9RectangleIlE // Keep replaced instruction
	LDR     R0, =_ZN4Game11playerCountE
	LDR     R0, [R0]
	CMP     R0, #1
	BNE     0x02148330
	B       0x021482F4
)");

//============================= Boss Controller Common =============================

void BossControllerCommon_setupCoopTransitionState(Player* closestPlayer)
{
	s32 playerCount = Game::getPlayerCount();

	if (playerCount == 1)
	{
		Game::getPlayer(Game::localPlayerID)->beginCutscene(true);

		BossController_sCustomTransition.func = BossController_transitionState;
		FinalBossController_sCustomTransition.func = FinalBossController_transitionState;
	}
	else
	{
		for (s32 playerID = 0; playerID < playerCount; playerID++)
			Game::getPlayer(playerID)->beginCutscene(false); // Gets set to true later in BossControllerCommon_coopTransitionState

		BossController_sCustomTransition.func = BossController_coopTransitionState;
		FinalBossController_sCustomTransition.func = FinalBossController_coopTransitionState;
	}
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

// Stage clearer is always player 0
ncp_repl(0x0213D1AC, 28, "MOV R2, #0")

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
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
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
)");

extern "C"
{
	s32 Bowser_getBattleState();
	bool BossBattleSwitch_switchState(StageEntity* self, BossBattleSwitch_PTMF ptmf);
}

fx32 BossBattleSwitch_linkedPlayerCameraX = 0;

Player* BossBattleSwitch_onBowserDead(Player* linkedPlayer)
{
	if (Game::getPlayerCount() == 1)
		return linkedPlayer;

	Player_beginBossDefeatCutsceneCoop(linkedPlayer, true);
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

ncp_over(0x02133150, 14) const auto MegaGoomba_skipRender = ActorFixes_safeSkipRender;

// Unfreeze both players
ncp_set_call(0x0213137C, 14, BossFixes_endCutsceneAllPlayers)

//============================= World 5: Petey Piranha =============================

ncp_over(0x02134790, 15) const auto PeteyPiranha_skipRender = ActorFixes_safeSkipRender;

// Make better use of overlay memory by putting the animations in the overlay instead of the model

ncp_set_call(0x02134390, 15, FS::Cache::loadFile) // boss_packun.nsbmd
ncp_set_call(0x0213439C, 15, FS::Cache::loadFileToOverlay) // boss_packun.nsbca

// Must be saved because the actor delays the hit
static u8 PeteyPiranha_sLinkedPlayerID = 0;

Player* PeteyPiranha_getPlayerOnSpecialHit(s32 linkedPlayerID)
{
	PeteyPiranha_sLinkedPlayerID = linkedPlayerID;
	return Game::getPlayer(linkedPlayerID); // Keep replaced instruction
}

ncp_set_call(0x021335BC, 15, PeteyPiranha_getPlayerOnSpecialHit) // Save player ID on ground pound
ncp_set_call(0x021338CC, 15, PeteyPiranha_getPlayerOnSpecialHit) // Save player ID on blue shell

Player* PeteyPiranha_getLinkedPlayer()
{
	return Game::getPlayer(PeteyPiranha_sLinkedPlayerID);
}

ncp_set_call(0x021307DC, 15, PeteyPiranha_getLinkedPlayer)
ncp_set_call(0x02130CB0, 15, PeteyPiranha_getLinkedPlayer)
ncp_set_call(0x02130E10, 15, PeteyPiranha_getLinkedPlayer)
ncp_set_call(0x02133098, 15, PeteyPiranha_getLinkedPlayer)
ncp_set_call(0x021330A4, 15, PeteyPiranha_getLinkedPlayer)
ncp_set_call(0x02133120, 15, PeteyPiranha_getLinkedPlayer)

// Save player ID on head hit

asm(R"(
.type PeteyPiranha_getPlayerOnPlatform, %function
PeteyPiranha_getPlayerOnPlatform:
	MOV     R5, R1
	PUSH    {R0-R1}
	LDR     R0, [R3,#4]
	LDRB    R0, [R0,#0x11E]
	LDR     R1, =_ZL28PeteyPiranha_sLinkedPlayerID
	STRB    R0, [R1]
	POP     {R0-R1}
	BX      LR
)");

ncp_repl(0x02132FD8, 15, "BLEQ PeteyPiranha_getPlayerOnPlatform")
ncp_repl(0x0213301C, 15, "BLEQ PeteyPiranha_getPlayerOnPlatform")
ncp_repl(0x02133060, 15, "BLEQ PeteyPiranha_getPlayerOnPlatform")

ncp_set_call(0x02132A58, 15, BossFixes_endCutsceneAllPlayers)
ncp_set_call(0x02132BE8, 15, BossFixes_endCutsceneAllPlayers)

// TODO: look into ov15 0x02132990

//============================= World 6: Monty Tank =============================

ncp_set_call(0x021361E4, 19, BossFixes_endCutsceneAllPlayers)

ncp_call(0x0213624C, 19)
void MontyTank_doPlayerBossBump(Player* r0, const Vec2& velocity)
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		player->doBossBump(velocity);
	}
}

//============================= World 7: Lakithunder =============================

ncp_over(0x02133C38, 17) const auto Lakithunder_skipRender = ActorFixes_safeSkipRender;

ncp_repl(0x0213215C, 17, "NOP")
ncp_set_call(0x02132168, 17, ActorFixes_isOutsideCamera)

ncp_repl(0x02132420, 17, "ADD R0, R5, #0x100")
ncp_repl(0x0213242C, 17, "LDRSB R0, [R0,#0x1E]") // Blue shell hit

ncp_repl(0x021325EC, 17, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Ground pound hit

ncp_repl(0x021329C4, 17, "ADD R0, R5, #0x100")
ncp_repl(0x021329D8, 17, "LDRSB R0, [R0,#0x1E]") // Stomp hit

ncp_set_call(0x02131554, 17, BossFixes_endCutsceneAllPlayers)

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

ncp_call(0x0214617C, 40)
Player* BossKey_getPlayerWhoWon(s32 winnerPlayerID)
{
	Player* winnerPlayer = Game::getPlayer(winnerPlayerID);

	Player_beginBossDefeatCutsceneCoop(winnerPlayer, false);

	Stage::setEvent(50); // Trigger event 50 for World 7 boss

	return winnerPlayer;
}

//============================= Mini-mushroom Cutscene =============================

ncp_repl(0x02144DCC, 40, "B 0x02144E00") // Do not load world signs
ncp_repl(0x02144D0C, 40, "B 0x02144D88") // Do not setup world signs
ncp_repl(0x02143D8C, 40, "B 0x02143E0C") // Do not render world signs

ncp_repl(0x02144AF8, 40, "B 0x02144B84") // Skip world sign effects

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
