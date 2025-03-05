/*============================================================\
|  NWAV player core.                                          |
|  This is main code that allows for music playback on DS.    |
\============================================================*/

#include "nwav.hpp"

#include <nsmb/core/filesystem.hpp>
#include <nsmb/core/system/memory.hpp>

/*==============================================================\
|  Player settings, change these according to your game needs.  |
\==============================================================*/

#define CHANNEL_L_NUM 6
#define CHANNEL_R_NUM 7
#define CHANNEL_MASK (1 << CHANNEL_L_NUM | ((1 << CHANNEL_R_NUM) * hInfo.stereo))
#define ALARM_NUM 1
#define STREAM_THREAD_PRIO 12
#define THREAD_STACK_SIZE 512
#define STRM_BUF_PAGESIZE (64 * 32)
#define STRM_BUF_PAGESIZE_STEREO (STRM_BUF_PAGESIZE * 2)
#define STRM_BUF_SIZE (STRM_BUF_PAGESIZE * 2)

namespace NWAV
{
	/*==========================\
	|  Structure declarations.  |
	\==========================*/

	struct EventInfo
	{
		u32 eventID;
		u32 sample;
	};

	struct Header
	{
		u32 magic;
		u32 fileSize;
		u32 sampleRate;
		u32 loopStart;
		u32 loopEnd;
		SNDWaveFormat format : 8;
		u8 stereo;
		u8 numEvents;
		u8 padding;
	};

	struct StreamInfo
	{
		u8 isPlaying;
		u8 isPaused;
		u8 reserved[2];

		fx32 speed;
		u32 volume;

		u32 bufPage;
		u32 musicEnd;
		u32 musicCursor;
		u32 samplesPerUpdate;
		u32 playRate;
		u32 targetVolume;
		u32 fadeDec;
		u32 fadeFrame;
		u8 loops;
		u8 stopMode;
		u8 chCount;
		u8 bytesPerSample;

		u32 eventIDBlockSize;
		u32 eventBlockSize;

		EventHandler eventHandler;
	};

	constexpr u32 headerSize = sizeof(Header);
	constexpr u32 eventInfoSize = sizeof(EventInfo);

	/*=======================\
	|  Static declarations.  |
	\=======================*/

	static Header hInfo;
	static StreamInfo sInfo = {
		.isPlaying = false,
		.isPaused = true,
		.speed = 0x1000,
		.volume = 127,
		.fadeFrame = 0
	};
	static FSFile file;

	static u8 strmThreadStack[THREAD_STACK_SIZE];
	static OSThread strmThread;
	static OSMessageQueue msgQ;
	static OSMessage msgBuf;

	static EventInfo* events;

	typedef u8(*pStrmBufT)[2][STRM_BUF_SIZE];
	static pStrmBufT pStrmBuf;

	/*=========================\
	|  Function declarations.  |
	\=========================*/

	void update(StreamInfo* sInfo);

	//Aligns a sample so it doesn't mess up on stereo.
	u32 alignSample(u32 pos)
	{
#ifndef NWAV_NO_STEREO
		if (hInfo.stereo)
		{
			u32 sampleAlign = sInfo.samplesPerUpdate;
			u32 unaligned = (pos % sampleAlign);

			if (unaligned > (sampleAlign / 2))
				pos += (sampleAlign - unaligned);
			else
				pos -= unaligned;
		}
#endif
		return pos;
	}

	//Goes to a certain position in the file, either based on byte index or music sample.
	void seek(u32 pos, bool sample)
	{
		if (sample)
		{
			//Calculate file absolute file position.
			pos = alignSample(pos);
			sInfo.musicCursor = pos;

			pos *= sInfo.chCount;
			pos *= sInfo.bytesPerSample;
			pos += headerSize;
			pos += sInfo.eventBlockSize;
		}

		//Seek position in file.
		FS_SeekFile(&file, pos, FS_SEEK_SET);
	}

	//Use this before playing or resuming music. (Keeps buffer aligned and makes it update instantly)
	void prepareBuffer()
	{
		sInfo.bufPage = 0;
		update(&sInfo);
		update(&sInfo);
	}

	//Gets if the music is paused.
	bool getPaused() { return sInfo.isPaused; }

	//Sets if the music is paused by stopping or starting the timers.
	void setPaused(bool paused)
	{
		//If music is playing and stopped flag isn't the one already set.
		if (sInfo.isPlaying && sInfo.isPaused != paused)
		{
			if (paused)
			{
				SND_StopTimer(CHANNEL_MASK, 0, 1 << ALARM_NUM, 0);
			}
			else
			{
				prepareBuffer();
				SND_StartTimer(CHANNEL_MASK, 0, 1 << ALARM_NUM, 0);
			}
			sInfo.isPaused = paused;
		}
	}

	//Gets the current volume.
	u32 getVolume() { return sInfo.volume; }

	//Sets the volume by shifting it during the specified frame period.
	void setVolume(u32 volume, u32 frames)
	{
		//Do nothing if volume doesn't change.
		if (sInfo.volume == volume)
			return;

		if (frames == 0)
		{
			SND_SetChannelVolume(CHANNEL_MASK, volume, SND_CHANNEL_DATASHIFT_NONE);
			sInfo.volume = volume;
		}
		else
		{
			s32 volumeDiff = sInfo.volume - volume;
			s32 remove = volumeDiff > 0 ? 1 : -1;

			sInfo.fadeDec = (volumeDiff / frames) + remove;
			sInfo.fadeFrame = frames;
			sInfo.targetVolume = volume;
		}
	}

	//Stops the music. (For internal use ONLY)
	//WARNING: You can either use frames or waitForUpdate, not both!!!
	void stop(u32 frames, bool waitForUpdate)
	{
		//Do nothing if not playing.
		if (!sInfo.isPlaying)
			return;

		if (frames || waitForUpdate)
		{
			//Begin special sound stop.
			if (frames)
				setVolume(0, frames);
			sInfo.stopMode = waitForUpdate + 1;
		}
		else
		{
			//Stop music timers.
			setPaused(true);
			sInfo.isPlaying = false;

			//Close the file.
			FS_CloseFile(&file);

			//Free memory.
			if (hInfo.numEvents)
				Memory::rootHeapPtr->deallocate(events);
			Memory::rootHeapPtr->deallocate(pStrmBuf);
		}
	}

	//Stops the music. (For external use ONLY)
	void stop(u32 frames)
	{
		stop(frames, false);
	}

#ifndef NWAV_NO_EVENTS
	//Sets the event handler function.
	void setEventHandler(EventHandler func) { sInfo.eventHandler = func; }

	//Updates the events.
	void updateEvents(StreamInfo* sInfo)
	{
		if (!hInfo.numEvents || !sInfo->eventHandler)
			return;

		for (int i = 0; i < hInfo.numEvents; i++)
		{
			EventInfo info = events[i];
			if (info.sample > sInfo->musicCursor &&
				info.sample < sInfo->musicCursor + sInfo->samplesPerUpdate)
			{
				sInfo->eventHandler(info.eventID);
			}
		}
	}
#endif

	//Updates the music fading.
	bool updateFade()
	{
		if (sInfo.isPlaying && sInfo.fadeFrame)
		{
			int newVolume = sInfo.volume - sInfo.fadeDec;
			newVolume = MATH_CLAMP(newVolume, 0, 127);

			setVolume(newVolume, 0);
			sInfo.fadeFrame--;

			if (sInfo.fadeFrame == 0)
			{
				setVolume(sInfo.targetVolume, 0);
				if (sInfo.stopMode == 1)
				{
					stop(0, false);
					return false;
				}
			}
		}
		return true;
	}

	//Checks if the music has reached end or loop point and updates the music state accordingly.
	void updateCheckEnd(StreamInfo* sInfo, pStrmBufT pBuf, u32 len)
	{
		s32 leftOver = STRM_BUF_PAGESIZE - len;
		if (sInfo->loops)
		{
			if (sInfo->musicCursor > hInfo.loopEnd)
			{
				seek(hInfo.loopStart, true);
				if (leftOver > 0)
				{
#ifndef NWAV_NO_STEREO
					for (u32 i = 0; i < sInfo->chCount; i++)
						FS_ReadFile(&file, &(*pBuf)[i][len], leftOver);
#else
					FS_ReadFile(&file, &(*pBuf)[0][len], leftOver);
#endif
				}
				seek(hInfo.loopStart + (leftOver / sInfo->bytesPerSample), true);
			}
		}
		else
		{
			if (sInfo->musicCursor > sInfo->musicEnd)
			{
				//Instead of instantly stopping the music, we must wait for the buffer end, otherwise the music will stop sooner.
				if (leftOver > 0)
				{
#ifndef NWAV_NO_STEREO
					for (u32 i = 0; i < sInfo->chCount; i++)
						MI_CpuClear8(&(*pBuf)[i][len], leftOver); //Clear the buffer end to avoid garble.
#else
					MI_CpuClear8(&(*pBuf)[0][len], leftOver);
#endif
				}
				stop(0, true); //Stop the music and wait for the buffer end.
			}
		}
	}

	//Updates the music.
	void update(StreamInfo* sInfo)
	{
		//Check for delayed stop.
		if (sInfo->stopMode == 2)
		{
			stop(0, false);
			return;
		}

		//Get buffer page and swap.
		pStrmBufT pBuf = reinterpret_cast<pStrmBufT>(**pStrmBuf + (STRM_BUF_PAGESIZE * sInfo->bufPage));
		sInfo->bufPage = !sInfo->bufPage;

		//Get read length.
		int len = STRM_BUF_PAGESIZE;
		int limit = sInfo->loops ? hInfo.loopEnd : sInfo->musicEnd;
		int remain = (limit - sInfo->musicCursor) * sInfo->bytesPerSample;
		if (remain < len)
			len = remain;

#ifndef NWAV_NO_STEREO
		//Read the data to the buffer.
		for (int i = 0; i < sInfo->chCount; i++)
			FS_ReadFile(&file, (*pBuf)[i], len);
#else
		FS_ReadFile(&file, (*pBuf)[0], len);
#endif

		//Increment the music cursor.
		sInfo->musicCursor += sInfo->samplesPerUpdate;

		//Update the events.
		updateCheckEnd(sInfo, pBuf, len);
#ifndef NWAV_NO_EVENTS
		updateEvents(sInfo);
#endif
	}

	//The sound alarm function that unblocks the thread.
	static void SoundAlarmHandler(void* arg)
	{
		//Unblock updater thread.
		OS_SendMessage(&msgQ, (OSMessage)arg, OS_MESSAGE_NOBLOCK);
	}

	//Setups the music channels, timers and sound alarm.
	void setup()
	{
		//Calculate timer values.
		s32 timerValue = SND_TIMER_CLOCK / sInfo.playRate;
		u32 alarmPeriod = timerValue * (STRM_BUF_PAGESIZE / sInfo.bytesPerSample) / 32;

		s32 loopLen = STRM_BUF_SIZE / sizeof(u32);

		//Setup channels.
#ifndef NWAV_NO_STEREO
		for (u32 i = 0; i < sInfo.chCount; i++)
		{
			bool left = i == 0;
			SND_SetupChannelPcm(
				left ? CHANNEL_L_NUM : CHANNEL_R_NUM,
				hInfo.format,
				left ? (*pStrmBuf)[0] : (*pStrmBuf)[1],
				SND_CHANNEL_LOOP_REPEAT,
				0,
				loopLen,
				sInfo.volume,
				SND_CHANNEL_DATASHIFT_NONE,
				timerValue,
				!hInfo.stereo ? 64 : (left ? 0 : 127)
			);
		}
#else
		SND_SetupChannelPcm(
			CHANNEL_L_NUM,
			hInfo.format,
			(*pStrmBuf)[0],
			SND_CHANNEL_LOOP_REPEAT,
			0,
			loopLen,
			sInfo.volume,
			SND_CHANNEL_DATASHIFT_NONE,
			timerValue,
			64
		);
#endif

		//Setup sound alarm for updater thread.
		SND_SetupAlarm(ALARM_NUM, alarmPeriod, alarmPeriod, SoundAlarmHandler, &sInfo);
	}

	//Reloads the current timers to apply new settings.
	void reloadTimers()
	{
		bool notPaused = !sInfo.isPaused;
		if (notPaused)
			setPaused(true);
		setup();
		if (notPaused)
			setPaused(false);
	}

	//Gets the music speed.
	fx32 getSpeed() { return sInfo.speed; }

	//Sets the music speed.
	void setSpeed(fx32 speed)
	{
		//Set the music speed.
		fx32 sampleRate = hInfo.sampleRate << FX32_SHIFT;
		sInfo.playRate = FX_MulInline(sampleRate, speed) >> FX32_SHIFT;
		sInfo.speed = speed;
		reloadTimers();
	}

#ifndef NWAV_NO_EVENTS
	//Loads the NWAV events that will be used to trigger the current callback function set.
	void loadEvents()
	{
		//Allocate events.
		events = scast<EventInfo*>(Memory::rootHeapPtr->allocate(hInfo.numEvents * sizeof(EventInfo), 4));

		//Read event IDs.
		for (int i = 0; i < hInfo.numEvents; i++)
		{
			int val = 0;
			FS_ReadFile(&file, &val, 1);
			events[i].eventID = val;
		}

		//Jump to "samples for event IDs" block.
		seek(headerSize + sInfo.eventIDBlockSize, false);

		//Read samples for event IDs
		for (int i = 0; i < hInfo.numEvents; i++)
		{
			int val;
			FS_ReadFile(&file, &val, 4);
			events[i].sample = val;
		}
	}
#endif

	//Plays the music.
	void play(u32 fileID)
	{
		//If music is already playing, stop it.
		if (sInfo.isPlaying)
			stop(0, false);

		//Initialize file and try to open it, otherwise crash.
		FS_InitFile(&file);
		if (!FS_OpenFileFast(&file, FSFileID{ FS::romArchive, fileID }))
			OS_Terminate();

		//Read the file header.
		FS_ReadFile(&file, &hInfo, headerSize);

		//Reset variables
		sInfo.loops = hInfo.loopEnd != 0;
		sInfo.fadeDec = 0;
		sInfo.fadeFrame = 0;
		sInfo.stopMode = 0;
		sInfo.volume = 127;
		sInfo.chCount = hInfo.stereo ? 2 : 1;
		sInfo.bytesPerSample = hInfo.format ? 2 : 1;
		sInfo.samplesPerUpdate = (STRM_BUF_PAGESIZE / sInfo.bytesPerSample);

#ifndef NWAV_NO_EVENTS
		//Setup events.
		if (hInfo.numEvents)
		{
			int unalignedEvents = (hInfo.numEvents % 4);
			sInfo.eventIDBlockSize = hInfo.numEvents + (4 - unalignedEvents);
			sInfo.eventBlockSize = sInfo.eventIDBlockSize + (hInfo.numEvents * 4);
			loadEvents();
		}
		else
		{
			sInfo.eventIDBlockSize = 0;
			sInfo.eventBlockSize = 0;
		}
#else
		sInfo.eventIDBlockSize = 0;
		sInfo.eventBlockSize = 0;
#endif

		//Calculate music size.
		sInfo.musicEnd = (((hInfo.fileSize - headerSize - sInfo.eventBlockSize) / sInfo.chCount) / sInfo.bytesPerSample);

		//Allocate stream buffer.
		pStrmBuf = scast<pStrmBufT>(Memory::rootHeapPtr->allocate(STRM_BUF_SIZE * sInfo.chCount, 4));

		//Start music read and updater.
		seek(0, true);
		setSpeed(sInfo.speed);

		sInfo.isPlaying = true;
		sInfo.isPaused = true;
		setPaused(false);
	}

	//The OS thread that runs the updater.
	static void StrmThread(void* arg)
	{
		OSMessage message;

		//Main thread loop
		while (true)
		{
			OS_ReceiveMessage(&msgQ, &message, OS_MESSAGE_BLOCK); //Block thread until message is received by the sound alarm.
			update(scast<StreamInfo*>(message));       //Update the music.
		}
	}

	//Initializes the NWAV player.
	void init()
	{
		//Lock the channels.
#ifndef NWAV_NO_STEREO
		SND_LockChannel(1 << CHANNEL_L_NUM | 1 << CHANNEL_R_NUM, 0);
#else
		SND_LockChannel(1 << CHANNEL_L_NUM, 0);
#endif

		//Startup stream thread.
		OS_InitMessageQueue(&msgQ, &msgBuf, 1);
		OS_CreateThread(
			&strmThread,
			StrmThread,
			nullptr,
			&strmThreadStack[THREAD_STACK_SIZE],
			THREAD_STACK_SIZE,
			STREAM_THREAD_PRIO
		);
		OS_WakeupThreadDirect(&strmThread);
	}
}
