/*============================================================\
|  NWAV game glue that every setup needs.                     |
|  Compiled whatever the components, so a project that starts |
|  and steps the player from its own hooks keeps playExtFile  |
|  and the BGM and scene components.                          |
\============================================================*/

#include "nwav/hooks.hpp"

// The core takes NitroFS file IDs, which come after the game's 131 overlays.
constexpr u32 FileIDOffset = 131;

NWAV::Handle NWAV::Hooks::playExtFile(u32 extFileID, const PlayParams& params)
{
	return NWAV::play(extFileID + FileIDOffset, params);
}

void NWAV::Hooks::update()
{
#ifdef NWAV_HOOK_BGM
	// Ends last frame's bahp before NWAV::update dispatches this frame's events.
	NWAV::Hooks::updateBGM();
#endif
	NWAV::update();
}
