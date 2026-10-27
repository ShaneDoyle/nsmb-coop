/*============================================================\
|  NWAV sound system hooks.                                   |
|  Starts the player with the game's sound system and steps   |
|  it once per frame, right before NNS_SndMain flushes the    |
|  frame's sound commands to the ARM7.                        |
\============================================================*/

#include "nwav/hooks.hpp"

#include <nsmb/core/system/memory.hpp>

static void* NWAV_allocate(u32 size)
{
	return Memory::rootHeapPtr->allocate(size, 32);
}

static void NWAV_deallocate(void* ptr)
{
	Memory::rootHeapPtr->deallocate(ptr);
}

// SND::Internal::createHeap
ncp_call(0x0204F2F0)
void NWAV_hookSndInit()
{
	NNS_SndInit(); // Keep replaced instruction
	NWAV::init({ NWAV_allocate, NWAV_deallocate });
}

// SND::Internal::updateSound
ncp_call(0x0204EC74)
void NWAV_hookSndMain()
{
	NWAV::Hooks::update();
	NNS_SndMain(); // Keep replaced instruction
}
