#include "nsmb/player.hpp"
#include "nsmb/stage/player/player.hpp"
#include "nsmb/filesystem/cache.hpp"
#include "nsmb/graphics/3d/modelanm.hpp"
#include "nsmb/graphics/3d/blendmodelanm.hpp"

#include "ActorFixes.hpp"

asm("SetupFSCacheToUseOverlay55 = 0x021726C0");
extern "C" void SetupFSCacheToUseOverlay55();

int NNS_EXTRA_G3dTexUnload(NNSG3dResTex* pTex);

//============================= Main Camera Push =============================

u32 BossFixes_sCurrentLoopPlayerID = 0;

// Allow camera to be pushed for all players

asm(R"(
_Z29BossFixes_pushPlayerCameraFixiii:
	LDR     R12, =BossFixes_sCurrentLoopPlayerID
	B       0x020ACF54
)");

void BossFixes_pushPlayerCameraFix(int a, int b, int c);

ncp_jump(0x020ACF50, 0)
void BossFixes_pushPlayerCamera(int a, int b, int c)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		BossFixes_sCurrentLoopPlayerID = playerID;
		BossFixes_pushPlayerCameraFix(a, b, c);
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

ncp_call(0x0213F4B4, 28)
void call_0213F4B4_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->beginCutscene(0);
	}
}

// Unfreeze when battle is ready to start

ncp_call(0x0213F550, 28)
void call_0213F550_ov28()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Game::getPlayer(playerID)->endCutscene();
	}
}

// Fix Blue Shell hit

ncp_repl(0x0213FF54, 28, "ADD R0, R4, #0x100")
ncp_repl(0x0213FF5C, 28, "LDRSB R0, [R0,#0x1E]")

// Fix Ground Pound

ncp_repl(0x0213FFC4, 28, "ADD R1, R0, #0x100")
ncp_repl(0x0213FFCC, 28, "LDRSB R0, [R1,#0x1E]")

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

//============================= World 2: Mummy Pokey =============================

ncp_over(0x02133AF0, 16) const auto MummyPokey_skipRender = ActorFixes_safeSkipRender;

ncp_repl(0x0213298C, 16, "ADD R0, R4, #0x100; LDRSB R0, [R0,#0x1E]") // Fix ground pound hit

ncp_repl(0x021327EC, 16, "ADD R0, R5, #0x100") // Fix shell hit
ncp_repl(0x021327FC, 16, "LDRSB R0, [R0,#0x1E]") // Fix shell hit
