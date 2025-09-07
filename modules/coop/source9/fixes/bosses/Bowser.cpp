#include <nsmb/game/game.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/graphics/3d/modelanm.hpp>

#include "coop/CoopActor.hpp"
#include "coop/CoopStage.hpp"
#include "coop/CoopPlayer.hpp"
#include "coop/fixes/bosses/BossControllerCommon.hpp"

extern "C" {
	void CoopSym_setupFSCacheToUseOverlay55();
}

namespace CoopFixes::Bowser {

ncp_over(0x0213BB10, 13) const auto vtbl_skipRender = CoopActor::safeSkipRender;

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
ncp_thumb void loadFix(ModelAnm* self, u32 animID, FrameCtrl::Type type, fx32 speed, u16 startFrame)
{
	// -- Unload the model from RAM

	FS::Cache::unloadFile(0x576);
	FS::Cache::unloadFile(0x577);

	// -- Free all files in overlay 55

	CoopSym_setupFSCacheToUseOverlay55();

	// -- Load the Dry Bones Bowser model

	void* bmd = FS::Cache::loadFileToOverlay(0x4D9, 0);
	void* bca = FS::Cache::loadFileToOverlay(0x4DA, 0);
	self->create(bmd, bca, 0, 0, 0);
	self->init(animID, type, speed, startFrame);
}

// Unfreeze both players

ncp_set_call(0x0213695C, 13, BossControllerCommon::endCutsceneAllPlayers)

// Fix fireball tracking

ncp_jump(0x02138D7C, 13)
ncp_asmfunc void jump_02138D7C_ov13()
{asm(R"(
	PUSH    {R0,R2,LR}
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	ADD     R3, R0, #0x100
	LDRSB   R3, [R3,#0x1E] // linkedPlayerID
	POP     {R0,R2,LR}
	B       0x02138D80
)");}

ncp_set_call(0x02138738, 13, FS::Cache::loadFile) // koopa new nsbmd
ncp_set_call(0x02138744, 13, FS::Cache::loadFileToOverlay) // koopa new nsbca

ncp_set_call(0x0213A018, 13, FS::Cache::loadFileToOverlay) // koopa fire 1 nsbmd
ncp_set_call(0x0213A024, 13, FS::Cache::loadFileToOverlay) // koopa fire 1 nsbta
ncp_set_call(0x0213A030, 13, FS::Cache::loadFileToOverlay) // koopa fire 2 nsbmd
ncp_set_call(0x0213A03C, 13, FS::Cache::loadFileToOverlay) // koopa fire 2 nsbta

} // namespace CoopFixes::Bowser
