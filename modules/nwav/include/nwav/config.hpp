/*============================================================\
|  NWAV player configuration.                                 |
|  Every value can be overridden through the Config component |
|  defines of the module, or by the project.                  |
\============================================================*/

#pragma once

// How many streams can play at the same time. Each one owns a pair of
// hardware channels and a sound alarm while the game runs.
#ifndef NWAV_MAX_STREAMS
#define NWAV_MAX_STREAMS 2
#endif

// Hardware channels per stream, as left/right pairs. Mono streams only use the
// left one. There must be at least NWAV_MAX_STREAMS pairs.
#ifndef NWAV_CHANNELS
#define NWAV_CHANNELS 6, 7, 4, 5
#endif

// Sound alarms per stream. There must be at least NWAV_MAX_STREAMS of them.
#ifndef NWAV_ALARMS
#define NWAV_ALARMS 1, 2
#endif

// Frames per ring page. Must be a multiple of 32, which keeps the alarm period
// an exact multiple of the channel timer so refills never drift.
#ifndef NWAV_PAGE_FRAMES
#define NWAV_PAGE_FRAMES 512
#endif

// Pages per ring. The stream thread may fall behind by up to PAGES - 1 pages
// (about 48 ms at 32 kHz with the defaults) before the output glitches.
#ifndef NWAV_PAGES
#define NWAV_PAGES 4
#endif

// Bytes read from the card at once per stream. Must be a multiple of 8.
#ifndef NWAV_READ_SIZE
#define NWAV_READ_SIZE 1024
#endif

// Maximum cue points loaded per stream.
#ifndef NWAV_MAX_EVENTS
#define NWAV_MAX_EVENTS 16
#endif

#ifndef NWAV_THREAD_PRIORITY
#define NWAV_THREAD_PRIORITY 12
#endif

#ifndef NWAV_THREAD_STACK_SIZE
#define NWAV_THREAD_STACK_SIZE 1024
#endif

// Define NWAV_DEBUG to log what the player and the hooks do through the game's
// Log, which only prints in NTR_DEBUG builds. The core is only SDK-dependent
// without it.
#ifdef NWAV_DEBUG
#include <nsmb/extra/log.hpp>
#define NWAV_LOG(...) (Log() << __VA_ARGS__)
#else
#define NWAV_LOG(...) ((void)0)
#endif

static_assert(NWAV_PAGE_FRAMES % 32 == 0, "NWAV_PAGE_FRAMES must be a multiple of 32");
static_assert(NWAV_PAGES >= 2, "NWAV_PAGES must be at least 2");
static_assert(NWAV_READ_SIZE % 8 == 0, "NWAV_READ_SIZE must be a multiple of 8");
