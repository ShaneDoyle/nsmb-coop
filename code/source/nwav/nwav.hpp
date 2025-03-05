/*============================================================\
|  NWAV player core declarations.                             |
|  This is the header that allows the engine code to be       |
|  implemented into the game and be accessed externally.      |
\============================================================*/

#pragma once

#define NWAV_MAGIC 0x5641574E

#include <nsmb_nitro.hpp>

namespace NWAV
{
	//The function type of the function that will handle the events.
	typedef void(*EventHandler)(u32);

	/// <summary>Initializes the player system. (Hook after SND_Init)</summary>
	void init();

	/// <summary>Updates the game fading. (Hook after SND_Main)</summary>
	bool updateFade();

	/// <summary>Plays a music.</summary>
	/// <param name="fileID">The file ID of the music file to play.</param>
	void play(u32 fileID);

	/// <summary>Stops the music playing.</summary>
	/// <param name="frames">Number of frames where the volume shift occurs.</param>
	void stop(u32 frames);

	/// <summary>Gets the music volume.</summary>
	/// <returns>The music volume.</returns>
	u32 getVolume();

	/// <summary>Sets the music volume.</summary>
	/// <param name="volume">The target volume. Value range = [0, 127]</param>
	/// <param name="frames">Number of frames where the volume shift occurs.</param>
	void setVolume(u32 volume, u32 frames);

	/// <summary>Gets the music speed.</summary>
	/// <returns>The current music speed.</returns>
	fx32 getSpeed();

	/// <summary>Sets the music speed.</summary>
	/// <param name="speed">The target speed for the music to be played at.</param>
	void setSpeed(fx32 speed);

	/// <summary>Gets if the music is paused.</summary>
	/// <returns>True if the music is paused. False otherwise.</returns>
	bool getPaused();

	/// <summary>Sets if the music is paused.</summary>
	/// <param name="paused">Sets the music as paused when true, unpauses when false.</param>
	void setPaused(bool paused);

#ifndef NWAV_NO_EVENTS
	/// <summary>Sets the event handler function.</summary>
	/// <param name="func">The function pointer of the event handler.</param>
	void setEventHandler(EventHandler func);
#endif
}
