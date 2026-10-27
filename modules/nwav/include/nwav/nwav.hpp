/*============================================================\
|  NWAV player core declarations.                             |
|  Streams standard RIFF WAV files (PCM8, PCM16, IMA-ADPCM,   |
|  mono or stereo) from the ROM file system. Loop points come |
|  from the smpl chunk and events from cue points. Event IDs  |
|  are the hashes of the event names (see EventID).           |
|                                                             |
|  The core only depends on the SDK. The game hooks that call |
|  init() and update() live in the module's Hooks component.  |
\============================================================*/

#pragma once

#include <nsmb_nitro.hpp>

#include "nwav/config.hpp"

namespace NWAV
{
	// Identifies one playback. A handle goes stale once its stream stops, and
	// every function silently ignores stale handles.
	using Handle = u32;
	constexpr Handle InvalidHandle = 0;

	/// <summary>
	/// The ID of the cue point that nwavtool writes for an event named `name`: the
	/// FNV-1a (32-bit) hash of the name. Compare it with the ID the event handler gets.
	/// </summary>
	consteval u32 EventID(const char* name)
	{
		u32 hash = 2166136261u;
		while (*name)
		{
			hash ^= static_cast<u8>(*name++);
			hash *= 16777619u;
		}
		return hash;
	}

	// Called from update() on the main thread when playback reaches a cue point.
	using EventHandler = void(*)(Handle handle, u32 eventID, void* userData);

	struct InitParams
	{
		void* (*allocate)(u32 size);
		void (*deallocate)(void* ptr);
	};

	enum class LoopMode : u8
	{
		File,	// Loop only if the file has a smpl loop.
		Off,	// Never loop.
		On		// Loop the file's loop, or the whole file if it has none.
	};

	struct PlayParams
	{
		u8 volume = 127;
		u8 pan = 64;			// Mono files only.
		u8 priority = 0;		// A stream may replace a playing one of equal or lower priority.
		u32 tags = 0;			// Bits the caller defines, which stopAllTagged matches.
		LoopMode loop = LoopMode::File;
		fx32 speed = FX32_ONE;
		u32 startFrame = 0;
		u32 fadeInFrames = 0;
		EventHandler onEvent = nullptr;
		void* userData = nullptr;	// Kept with the stream, for the caller and its event handler.
	};

	/// <summary>Locks the channels and starts the stream thread. (Hook after NNS_SndInit)</summary>
	void init(const InitParams& params);

	/// <summary>Steps fades, dispatches events and finishes stops. (Hook once per frame, before NNS_SndMain)</summary>
	void update();

	/// <summary>Starts streaming a WAV file.</summary>
	/// <param name="fileID">The NitroFS file ID, which counts the overlays.</param>
	/// <returns>The playback handle, or InvalidHandle if the file is invalid or no stream was free.</returns>
	Handle play(u32 fileID, const PlayParams& params = {});

	/// <summary>Stops a stream, optionally fading it out over a number of frames.</summary>
	void stop(Handle handle, u32 fadeFrames = 0);

	/// <summary>Stops every stream whose priority is below the given one.</summary>
	void stopAll(u32 fadeFrames = 0, u32 belowPriority = 256);

	/// <summary>Stops every stream that has any of the given tags (PlayParams::tags).</summary>
	void stopAllTagged(u32 tags, u32 fadeFrames = 0);

	bool isPlaying(Handle handle);

	bool isPaused(Handle handle);

	/// <summary>Pauses or resumes a stream. Resuming restarts from the page that was playing.</summary>
	void setPaused(Handle handle, bool paused);

	/// <summary>Pauses or resumes every stream.</summary>
	void setPausedAll(bool paused);

	u32 getVolume(Handle handle);

	/// <summary>Sets the volume. Value range = [0, 127]</summary>
	void setVolume(Handle handle, u32 volume, u32 frames = 0);

	/// <summary>Adds an attenuation in tenths of a decibel on top of the volume. Value range = [-723, 0]</summary>
	void setDecibelOffset(Handle handle, s32 decibels);

	fx32 getSpeed(Handle handle);

	/// <summary>Changes the playback rate (and pitch). FX32_ONE is the file's own rate.</summary>
	void setSpeed(Handle handle, fx32 speed);

	/// <summary>Gets the pointer the stream was played with (PlayParams::userData), or nullptr for a stale handle.</summary>
	void* getUserData(Handle handle);

	/// <summary>Gets the frame being played, accurate to one ring page.</summary>
	u32 getPosition(Handle handle);
}
