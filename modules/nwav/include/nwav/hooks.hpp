/*============================================================\
|  NWAV game integration for New Super Mario Bros.            |
|  Declarations shared by the module's hook components.       |
\============================================================*/

#pragma once

#include "nwav/nwav.hpp"

// BGMEntry::hurryFileID of a stream with no hurry-up version. With
// BGMFollowTempo, the stream itself speeds up instead (raising its pitch).
#define NO_HURRY_TRACK u32(-1)

namespace NWAV::Hooks
{
	// Priority of streams that replace the game's BGM. Streams below it can never
	// take a BGM stream's slot.
	constexpr u8 BGMPriority = 128;

	// Tag (PlayParams::tags) of streams that are sound effects. With SoundIntegration,
	// they stop whenever the game stops its own sound effects.
	constexpr u32 SFXTag = 1 << 0;

	struct BGMEntry
	{
		u32 seqID;			// The sequence to replace.
		u32 fileID;			// The WAV file that plays instead, as an extended file ID ("fid" literal).
		u32 hurryFileID;	// The WAV file that plays from its start on hurry up, or NO_HURRY_TRACK.
		s32 decibels;		// Attenuation in tenths of a decibel. Value range = [-723, 0]
	};

	/// <summary>
	/// Gets the BGM replacement table. The module defines it weakly as empty;
	/// a project replaces it by defining this function.
	/// </summary>
	const BGMEntry* getBGMTable(u32& count);

	/// <summary>
	/// Called on the main thread for every event (cue point) a BGM stream reaches,
	/// after the module's own handling of it. The module defines it weakly as empty;
	/// a project replaces it by defining this function.
	/// </summary>
	/// <param name="handle">The BGM stream.</param>
	/// <param name="eventID">The event's ID. Compare it with NWAV::EventID("name").</param>
	void onBGMEvent(Handle handle, u32 eventID);

	/// <summary>Starts streaming a WAV file by its extended file ID, as used by the game and the "fid" literal.</summary>
	/// <returns>The playback handle, or InvalidHandle if the file is invalid or no stream was free.</returns>
	Handle playExtFile(u32 extFileID, const PlayParams& params = {});

	/// <summary>
	/// Steps the player and the module's components once per frame. The Hooks component
	/// calls it before NNS_SndMain; a project that switches Hooks off calls it in place
	/// of NWAV::update, after its own NWAV::init.
	/// </summary>
	void update();

	/// <summary>Ends the frame's bahp and forgets a finished BGM stream. (Called by NWAV::Hooks::update)</summary>
	void updateBGM();

	/// <summary>Gets the stream that plays in place of the current BGM sequence, if any.</summary>
	Handle getBGMStream();
}
