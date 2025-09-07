#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/core/graphics/3d/modelanm.hpp>

extern "C" {
	void CoopSym_setupFSCacheToUseOverlay55();
	void CoopSym_FinalBowser_loadResources();
}

namespace CoopFixes::FinalBowser {

ncp_set_call(0x021487E0, 43, FS::Cache::loadFile) // Load pot.nsbmd to memory instead of overlay
ncp_set_call(0x021487EC, 43, FS::Cache::loadFile) // Load pot.nsbca to memory instead of overlay

ncp_repl(0x02148814, 43, "NOP") // Do not load Final Bowser resources yet

ncp_call(0x0214793C, 43)
ncp_thumb void loadFix(ModelAnm* self, u32 animID, FrameCtrl::Type type, fx32 speed, u16 startFrame)
{
	// -- Unload the model from RAM

	FS::Cache::unloadFile(1235);
	FS::Cache::unloadFile(1236);
	FS::Cache::unloadFile(1237);
	FS::Cache::unloadFile(1238);
	FS::Cache::unloadFile(1239);

	// -- Free all files in overlay 55

	CoopSym_setupFSCacheToUseOverlay55();

	// -- Load the Final Bowser model

	CoopSym_FinalBowser_loadResources();

	self->init(animID, type, speed, startFrame); // Keep replaced instruction
}

} // namespace CoopFixes::FinalBowser
