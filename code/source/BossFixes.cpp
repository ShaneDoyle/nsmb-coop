#include "nsmb/game.hpp"
#include "nsmb/player.hpp"
#include "nsmb/stage/player/player.hpp"
#include "nsmb/stage/entity.hpp"
#include "nsmb/stage/misc.hpp"
#include "nsmb/filesystem/cache.hpp"
#include "nsmb/graphics/fader.hpp"
#include "nsmb/graphics/3d/modelanm.hpp"
#include "nsmb/graphics/3d/blendmodelanm.hpp"
#include "nsmb/system/function.hpp"

#include "ActorFixes.hpp"


// VERY IMPORTANT
// CURRENTLY THE BOWSER FRAME HEAP ALLOCATION IS DEALLOCATED UNSAFELY
// IT IS ALMOST PURE LUCK THAT IT WORKS, SO PLEASE LOOK AT IT AFTER THIS RELEASE!


//============================= Main Camera Push =============================

u32 BossFixes_currentLoopPlayerID = 0;

// Allow camera to be pushed for all players

asm(R"(
BossFixes_pushPlayerCameraFix:
	LDR     R12, =BossFixes_currentLoopPlayerID
	B       0x020ACF54
)");

extern "C" void BossFixes_pushPlayerCameraFix(StageLayout* self, fx32 bound, u32 side);

ncp_jump(0x020ACF50, 0)
void BossFixes_pushPlayerCamera(StageLayout* self, fx32 bound, u32 side)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		BossFixes_currentLoopPlayerID = playerID;
		BossFixes_pushPlayerCameraFix(self, bound, side);
	}
}

//============================= Main Boss Controller =============================

const u8 CutsceneZoneID = 65;

struct BossController_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

static BossController_PTMF BossController_sCustomTransitionState = { nullptr, 0 };

ncp_over(0x02143994, 40)
const static BossController_PTMF* BossController_sCustomTransitionState_ptr = &BossController_sCustomTransitionState;

asm(R"(
	Zone_get = 0x0201EEF8
	BossController_limitFightToZone = 0x02142F14
	BossController_transitionState = 0x02143550
	BossController_switchState = 0x021439EC
	BossController_sTransitionState = 0x02146C08
)");

extern "C"
{
	void* Zone_get(u8 id, FxRect* area);
	void BossController_limitFightToZone(StageEntity* self);
	void BossController_switchState(StageEntity* self, BossController_PTMF* ptmf);
	bool BossController_transitionState(StageEntity* self);
	BossController_PTMF BossController_sTransitionState;
}

bool BossController_coopTransitionState(StageEntity* self)
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

	if (step == 1)
	{
		// Wait for fade out
		if ((Game::fader.fadingState[0] & 8) == 0)
			step++;

		return true;
	}

	if (step == 2)
	{
		step++;

		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			Game::getPlayer(playerID)->beginCutscene(true);

		Player* closestPlayer = self->getClosestPlayer(nullptr, nullptr);

		// Move player that reached the Boss barrier closer to Bowser to give space to the other player
		//closestPlayer->position.x += 24fx;

		// Other player
		Player* otherPlayer = Game::getPlayer(closestPlayer->linkedPlayerID ^ 1);
		otherPlayer->position.x = closestPlayer->position.x - 16fx;
		otherPlayer->position.y = closestPlayer->position.y;

		// Below code is for an eventual 4-player update.
		// Requires fixing the camera first, right now it must be aligned
		// to the right side because Bowser Jr. spawns the boss when Bowser Jr.
		// goes out of the camera.

		// Other players
		/*s32 order = 1;
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (closestPlayer->linkedPlayerID == playerID)
				continue;

			Player* player = Game::getPlayer(playerID);
			player->beginCutscene(true);
			player->position.x = closestPlayer->position.x - (1.25fx * 16fx) * order;
			player->position.y = closestPlayer->position.y;

			order++;
		}*/

		return true;
	}

	if (step == 3)
	{
		step++;

		FxRect zoneArea;
		Zone_get(CutsceneZoneID, &zoneArea);
		BossFixes_pushPlayerCamera(Stage::stageLayout, scast<s16>(zoneArea.x >> 12), 1);

		return true;
	}

	if (step < 3 + FadeWaitDurationFrames)
	{
		step++;
		// Make up time for the background to catch up with the camera
		return true;
	}

	if (step == 3 + FadeWaitDurationFrames)
	{
		step++;

		// Begin fade in
		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			Game::fader.fadingState[playerID] |= 0x5;

		return true;
	}

	if (step == 4 + FadeWaitDurationFrames)
	{
		// Wait for fade in
		if ((Game::fader.fadingState[0] & 1) != 0)
            return true;

		BossController_switchState(self, &BossController_sTransitionState);
		return true;
	}

	return true;
}

// Setup the cutscene transition
ncp_call(0x021438AC, 40)
void call_021438AC_ov40(Player* closestPlayer)
{
	s32 playerCount = Game::getPlayerCount();

	if (playerCount == 1)
	{
		Game::getPlayer(Game::localPlayerID)->beginCutscene(true);
		BossController_sCustomTransitionState.func = BossController_transitionState;
	}
	else
	{
		for (s32 playerID = 0; playerID < playerCount; playerID++)
			Game::getPlayer(playerID)->beginCutscene(false); // Gets set to true later in BossController_coopTransitionState

		BossController_sCustomTransitionState.func = BossController_coopTransitionState;
	}
}

ncp_call(0x0214393C, 40)
void BossController_customPushPlayerCamera(StageLayout* self, fx32 bound, u32 side)
{
	if (Game::getPlayerCount() == 1)
		BossFixes_pushPlayerCamera(self, bound, side);

	// For coop it gets handled by BossController_coopTransitionState
}

ncp_call(0x02142A38, 40)
void BossController_customLimitFightToZone(StageEntity* self)
{
	if (Game::getPlayerCount() == 1)
		BossController_limitFightToZone(self);

	// For coop it gets handled by the unfreezing of each actor
}

// ============================= Misc =============================

asm("SetupFSCacheToUseOverlay55 = 0x021726C0");
extern "C" void SetupFSCacheToUseOverlay55();

int NNS_EXTRA_G3dTexUnload(NNSG3dResTex* pTex);

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
		StageEntity* bossController = scast<StageEntity*>(ProcessManager::getNextObjectByObjectID(114));
		if (bossController != nullptr)
		{
			BossController_limitFightToZone(bossController);
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
	u8* bowser = rcast<u8*>(self) - 0x4B8;
	BlendModelAnm* bowserMdl = rcast<BlendModelAnm*>(bowser + 0x3F4);

	// -- Unload the model's textures from VRAM

	NNS_EXTRA_G3dTexUnload(bowserMdl->texture);

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
ncp_call(0x0213A7A4, 13)
void call_0213A7A4_ov13()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->actionFlag.bowserJrBeaten = true;
	}
}

// Remove Freeze at BowserBattleSwitch
ncp_repl(0x0213AF54, 13, "NOP")

//============================= World 2: Mummy Pokey =============================

ncp_over(0x02133AF0, 16) const auto MummyPokey_skipRender = ActorFixes_safeSkipRender;

ncp_repl(0x0213298C, 16, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Fix ground pound hit

ncp_repl(0x021327EC, 16, "ADD R0, R5, #0x100") // Fix shell hit
ncp_repl(0x021327FC, 16, "LDRSB R0, [R0,#0x1E]") // Fix shell hit

//============================= World 3: Cheepskipper =============================

//============================= World 4: Mega Goomba =============================

// Unfreeze both players
ncp_set_call(0x0213137C, 14, BossFixes_endCutsceneAllPlayers)

//============================= Boss Key =============================

// Victory freeze

ncp_call(0x0214619C, 40)
void call_0214619C_ov40()
{
	Game::getPlayer(Game::localPlayerID)->physicsFlag.bossDefeated = true;

	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (playerID != Game::localPlayerID)
		{
			Game::getPlayer(playerID)->actionFlag.bowserJrBeaten = true;
		}
	}
}
