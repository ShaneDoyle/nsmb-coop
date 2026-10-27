/*============================================================\
|  NWAV pause hook.                                           |
|  Shared by BGMReplace and SoundIntegration, which both need |
|  the game's pause and can only hook it once between them.   |
\============================================================*/

#include "nwav/hooks.hpp"

#if defined(NWAV_HOOK_BGM) || defined(NWAV_HOOK_SOUND)

namespace NWAV::Hooks
{
	// Runs the original through a copy of its first instruction and a branch past it.
	ncp_asmfunc static void pauseSeqOriginal(BOOL paused) { asm("STMFD SP!, {R4,LR}\nB 0x0204E480"); }

	// SND::Internal::pauseSeq(paused), which pauses every sound player. Hooked at its
	// entry, because it returns early when no sequence plays, as when a stream took
	// the sequence's place.
	ncp_jump(0x0204E47C)
	void NWAV_pauseSeq(BOOL paused)
	{
		pauseSeqOriginal(paused);
#ifdef NWAV_HOOK_SOUND
		NWAV::setPausedAll(paused);
#else
		NWAV::setPaused(getBGMStream(), paused);
#endif
	}
}

#endif
