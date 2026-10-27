/*============================================================\
|  NWAV sound effect hook.                                    |
|  Streams tagged as sound effects (Hooks::SFXTag) stop      |
|  whenever the game stops its own sound effects, as scenes   |
|  do when they end. (Pausing is in pause.cpp.)               |
\============================================================*/

#include "nwav/hooks.hpp"

namespace NWAV::Hooks
{
	// Runs the original through a copy of its first instruction and a branch past it.
	ncp_asmfunc static void stopAllSFXEmittersOriginal(int frames) { asm("STMFD SP!, {R4-R7,LR}\nB 0x0204DD04"); }

	// SND::Internal::stopAllSFXEmitters(frames), which SND::stopSounds calls.
	ncp_jump(0x0204DD00)
	void NWAV_stopAllSFXEmitters(int frames)
	{
		stopAllSFXEmittersOriginal(frames);
		NWAV::stopAllTagged(SFXTag, frames);
	}
}
