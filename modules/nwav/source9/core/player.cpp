/*============================================================\
|  NWAV player core.                                          |
|  Each stream plays a ring of pages per channel on repeating |
|  hardware channels. A sound alarm fires every page and a    |
|  thread refills the page that just finished, wrapping to    |
|  the loop start on the exact frame.                         |
\============================================================*/

#include "nwav/nwav.hpp"

#include "stream.hpp"


namespace NWAV
{
	/*==========================\
	|  Structure declarations.  |
	\==========================*/

	constexpr u32 NoFrame = 0xFFFFFFFF;
	constexpr u32 MaxPendingEvents = 8;
	constexpr u32 VolumeShift = 8;

	struct PendingEvent
	{
		u32 id;
		u32 due;	// The alarm count at which the page holding the event starts playing.
	};

	struct Slot
	{
		Core::Info info;
		Core::Decoder dec;
		FSFile file;

		void* memory;
		u8* ring[2];
		u32 ringSize;			// Bytes per channel.
		void* garbage;			// A ring the hardware may still be reading.
		u32 garbageFrames;

		Handle handle;
		bool active;
		bool paused;
		bool suspended;			// Paused for sleep mode, resumed on wake.
		volatile bool finished;
		u8 priority;
		u8 pan;
		u32 serial;
		u32 tags;

		volatile u32 epoch;		// Bumped whenever the timers stop, so stale alarms are dropped.
		volatile u32 irqCount;	// Alarms received.
		u32 alarmCount;			// Alarms serviced.
		u32 fillCount;			// Pages filled since the timers started.
		u32 endAt;				// Alarm count at which the last page has played.
		u32 pageFrame[NWAV_PAGES];

		Core::Playhead ph;
		fx32 speed;

		s32 volume;				// [0, 127] << VolumeShift
		s32 fadeTarget;
		s32 fadeStep;
		u32 fadeFrames;
		bool stopAfterFade;
		s32 decibelOffset;
		u32 channelVolume;		// Last value sent to the hardware.

		EventHandler onEvent;
		void* userData;
		PendingEvent pending[MaxPendingEvents];
		u32 pendingHead;
		u32 pendingTail;
	};

	/*=======================\
	|  Static declarations.  |
	\=======================*/

	static const u8 slotChannels[] = { NWAV_CHANNELS };
	static const u8 slotAlarms[] = { NWAV_ALARMS };

	static_assert(sizeof(slotChannels) >= NWAV_MAX_STREAMS * 2, "NWAV_CHANNELS needs a pair per stream");
	static_assert(sizeof(slotAlarms) >= NWAV_MAX_STREAMS, "NWAV_ALARMS needs an alarm per stream");
	static_assert(NWAV_MAX_STREAMS <= 0x100, "Handles store the slot in 8 bits");

	static Slot slots[NWAV_MAX_STREAMS];
	static InitParams initParams;
	static FSArchive* romArchive;
	static u32 nextGeneration = 1;
	static u32 nextSerial;

	static OSMutex lock;
	static OSMessageQueue msgQ;
	static OSMessage msgBuf[NWAV_MAX_STREAMS * 2];
	static OSThread strmThread;
	static PMSleepCallbackInfo preSleepInfo;
	static PMSleepCallbackInfo postSleepInfo;
	static u64 strmThreadStack[NWAV_THREAD_STACK_SIZE / sizeof(u64)];

	/*=========================\
	|  Function declarations.  |
	\=========================*/

	static u32 slotIndex(const Slot& s) { return u32(&s - slots); }

	static u32 channelMask(const Slot& s)
	{
		u32 i = slotIndex(s);
		u32 mask = 1 << slotChannels[i * 2];
		if (s.info.channels == 2)
			mask |= 1 << slotChannels[i * 2 + 1];
		return mask;
	}

	static u32 bytesPerSample(const Slot& s)
	{
		return s.info.format == Core::Format::Pcm8 ? 1 : 2;
	}

	// Resolves a handle to its slot, or nullptr if it went stale.
	static Slot* getSlot(Handle handle)
	{
		u32 i = handle & 0xFF;
		if (handle == InvalidHandle || i >= NWAV_MAX_STREAMS)
			return nullptr;
		Slot& s = slots[i];
		return s.active && s.handle == handle ? &s : nullptr;
	}

	static void queueEvents(Slot& s, u32 start, u32 frames, u32 fillIndex)
	{
		if (!s.onEvent)
			return;

		for (u32 i = 0; i < s.info.numEvents; i++)
		{
			const Core::Event& event = s.info.events[i];
			if (event.frame < start || event.frame >= start + frames)
				continue;

			u32 next = (s.pendingTail + 1) % MaxPendingEvents;
			if (next == s.pendingHead)
			{
				NWAV_LOG("NWAV stream: handle=" << s.handle << " event queue full, dropped the events at frame " << event.frame << "\n");
				return;
			}
			s.pending[s.pendingTail] = { event.id, fillIndex };
			s.pendingTail = next;
		}
	}

	// Fills the next page of the ring.
	static void fillPage(Slot& s)
	{
		const u32 bps = bytesPerSample(s);
		const u32 page = s.fillCount % NWAV_PAGES;
		const u32 fillIndex = s.fillCount++;

		u8* left = s.ring[0] + page * NWAV_PAGE_FRAMES * bps;
		u8* right = s.info.channels == 2 ? s.ring[1] + page * NWAV_PAGE_FRAMES * bps : nullptr;

		bool wasEnded = s.ph.ended;
		s.pageFrame[page] = wasEnded ? NoFrame : s.ph.cursor;

		Core::render(s.dec, s.ph, left, right, bps, NWAV_PAGE_FRAMES, [&](u32 start, u32 frames) {
			queueEvents(s, start, frames, fillIndex);
		});

		if (s.ph.ended && !wasEnded)
			s.endAt = fillIndex + 1;

		// The ARM7 reads the ring straight from main memory.
		DC_FlushRange(left, NWAV_PAGE_FRAMES * bps);
		if (right)
			DC_FlushRange(right, NWAV_PAGE_FRAMES * bps);
	}

	// Catches up with the alarms received for a stream. (Stream thread, lock held)
	static void service(Slot& s)
	{
#ifdef NWAV_DEBUG
		// Every page the hardware played since the last service needs a refill; once
		// that is the whole ring, the hardware is replaying pages that were not refilled.
		// (Not left to NWAV_LOG: the volatile reads of the check would stay in release.)
		if (s.irqCount - s.alarmCount >= NWAV_PAGES && !s.ph.ended)
			NWAV_LOG("NWAV stream: handle=" << s.handle << " underrun, " << (s.irqCount - s.alarmCount) << " pages behind\n");
#endif

		while (s.alarmCount != s.irqCount)
		{
			s.alarmCount++;
			if (s.ph.ended && s.alarmCount >= s.endAt)
			{
				s.finished = true;
				return;
			}
			fillPage(s);
		}
	}

	// Runs in the PXI interrupt, so it only counts and wakes the thread.
	static void alarmHandler(void* arg)
	{
		u32 value = reinterpret_cast<u32>(arg);
		Slot& s = slots[value & 0xFF];
		if (s.epoch == (value >> 8))
		{
			s.irqCount = s.irqCount + 1;
			OS_SendMessage(&msgQ, nullptr, OS_MESSAGE_NOBLOCK);
		}
	}

	static void streamThread(void*)
	{
		OSMessage message;
		while (true)
		{
			OS_ReceiveMessage(&msgQ, &message, OS_MESSAGE_BLOCK);

			OS_LockMutex(&lock);
			for (Slot& s : slots)
			{
				if (s.active && !s.paused && !s.finished)
					service(s);
			}
			OS_UnlockMutex(&lock);
		}
	}

	static u32 calcChannelVolume(const Slot& s)
	{
		s32 db = SND_CalcDecibel(s.volume >> VolumeShift) + s.decibelOffset;
		if (db < SND_VOLUME_DB_MIN)
			db = SND_VOLUME_DB_MIN;
		else if (db > SND_VOLUME_DB_MAX)
			db = SND_VOLUME_DB_MAX;
		return SND_CalcChannelVolume(db);
	}

	static void applyVolume(Slot& s)
	{
		u32 value = calcChannelVolume(s);
		if (value == s.channelVolume)
			return;
		s.channelVolume = value;
		SND_SetChannelVolume(channelMask(s), value & 0xFF, SNDChannelDataShift(value >> 8));
	}

	static void stopTimers(Slot& s)
	{
		SND_StopTimer(channelMask(s), 0, 1 << slotAlarms[slotIndex(s)], 0);
		s.epoch = (s.epoch + 1) & 0xFFFFFF;
	}

	// Refills the whole ring from the cursor and starts the hardware. (Lock held)
	static void startTimers(Slot& s)
	{
		const u32 index = slotIndex(s);

		s.epoch = (s.epoch + 1) & 0xFFFFFF;
		s.irqCount = 0;
		s.alarmCount = 0;
		s.fillCount = 0;
		s.ph.ended = false;
		s.finished = false;
		s.pendingHead = 0;
		s.pendingTail = 0;

		Core::seek(s.dec, s.ph.cursor);
		for (u32 i = 0; i < NWAV_PAGES; i++)
			fillPage(s);

		u32 rate = u32((u64(s.info.sampleRate) * u32(s.speed)) >> FX32_SHIFT);
		s32 timer = rate ? s32(SND_TIMER_CLOCK / rate) : 0xFFFF;
		timer = MATH_CLAMP(timer, 0x10, 0xFFFF);
		u32 alarmPeriod = u32(timer) * NWAV_PAGE_FRAMES / 32;

		s.channelVolume = calcChannelVolume(s);
		SNDWaveFormat format = s.info.format == Core::Format::Pcm8 ? SND_WAVE_FORMAT_PCM8 : SND_WAVE_FORMAT_PCM16;

		for (u32 ch = 0; ch < s.info.channels; ch++)
		{
			s32 pan = s.info.channels == 1 ? s.pan : (ch == 0 ? 0 : 127);
			SND_SetupChannelPcm(
				slotChannels[index * 2 + ch],
				format,
				s.ring[ch],
				SND_CHANNEL_LOOP_REPEAT,
				0,
				s32(s.ringSize / sizeof(u32)),
				s.channelVolume & 0xFF,
				SNDChannelDataShift(s.channelVolume >> 8),
				timer,
				pan
			);
		}

		SND_SetupAlarm(slotAlarms[index], alarmPeriod, alarmPeriod, alarmHandler, reinterpret_cast<void*>((s.epoch << 8) | index));
		SND_StartTimer(channelMask(s), 0, 1 << slotAlarms[index], 0);
	}

	// The frame of the page the hardware is playing, NoFrame once past the end.
	static u32 playingFrame(const Slot& s)
	{
		return s.pageFrame[s.irqCount % NWAV_PAGES];
	}

	// Restarts the hardware from the page that was playing. (Lock held)
	static bool restart(Slot& s)
	{
		u32 frame = playingFrame(s);
		if (frame == NoFrame)
			return false;
		s.ph.cursor = frame;
		startTimers(s);
		return true;
	}

	static void releaseSlot(Slot& s)
	{
		if (!s.active)
			return;

		NWAV_LOG("NWAV stop: handle=" << s.handle << "\n");

		if (!s.paused)
			stopTimers(s);
		else
			s.epoch = (s.epoch + 1) & 0xFFFFFF;

		FS_CloseFile(&s.file);

		// The stop command reaches the ARM7 with the next NNS_SndMain, so the
		// ring must outlive it by a frame.
		if (s.garbage)
			initParams.deallocate(s.garbage);
		s.garbage = s.memory;
		s.garbageFrames = 2;

		s.memory = nullptr;
		s.active = false;
		s.handle = InvalidHandle;
	}

	static Slot* pickSlot(u8 priority)
	{
		Slot* best = nullptr;
		for (Slot& s : slots)
		{
			if (!s.active)
				return &s;
			if (s.priority > priority)
				continue;
			if (!best || s.priority < best->priority ||
				(s.priority == best->priority && s32(s.serial - best->serial) < 0))
				best = &s;
		}
		return best;
	}

	static void updateSlot(Slot& s, PendingEvent* events, u32& numEvents)
	{
		if (s.garbage && --s.garbageFrames == 0)
		{
			initParams.deallocate(s.garbage);
			s.garbage = nullptr;
		}

		if (!s.active)
			return;

		if (s.finished)
		{
			releaseSlot(s);
			return;
		}

		if (s.fadeFrames)
		{
			s.volume += s.fadeStep;
			if (--s.fadeFrames == 0)
			{
				s.volume = s.fadeTarget;
				if (s.stopAfterFade)
				{
					releaseSlot(s);
					return;
				}
			}
		}

		applyVolume(s);

		while (s.pendingHead != s.pendingTail && s.pending[s.pendingHead].due <= s.irqCount)
		{
			events[numEvents++] = s.pending[s.pendingHead];
			s.pendingHead = (s.pendingHead + 1) % MaxPendingEvents;
		}
	}

	static void startFade(Slot& s, u32 volume, u32 frames, bool stopAfter)
	{
		s32 target = s32(volume) << VolumeShift;
		s.stopAfterFade = stopAfter;
		if (frames == 0)
		{
			s.volume = target;
			s.fadeFrames = 0;
			return;
		}
		s.fadeTarget = target;
		s.fadeStep = (target - s.volume) / s32(frames);
		s.fadeFrames = frames;
	}

	/*======================\
	|  Public functions.    |
	\======================*/

	void update()
	{
		PendingEvent events[NWAV_MAX_STREAMS][MaxPendingEvents];
		u32 numEvents[NWAV_MAX_STREAMS] = {};
		EventHandler handlers[NWAV_MAX_STREAMS];
		void* userData[NWAV_MAX_STREAMS];
		Handle handles[NWAV_MAX_STREAMS];

		OS_LockMutex(&lock);
		for (u32 i = 0; i < NWAV_MAX_STREAMS; i++)
		{
			Slot& s = slots[i];
			updateSlot(s, events[i], numEvents[i]);
			handlers[i] = s.onEvent;
			userData[i] = s.userData;
			handles[i] = s.handle;
		}
		OS_UnlockMutex(&lock);

		// Outside the lock, so handlers can call back into the player.
		for (u32 i = 0; i < NWAV_MAX_STREAMS; i++)
		{
			for (u32 j = 0; j < numEvents[i]; j++)
				handlers[i](handles[i], events[i][j].id, userData[i]);
		}
	}

	Handle play(u32 fileID, const PlayParams& params)
	{
		if (!romArchive)
		{
			NWAV_LOG("NWAV play: file=" << fileID << " failed, init was not called\n");
			return InvalidHandle;
		}

		OS_LockMutex(&lock);

		Slot* slot = pickSlot(params.priority);
		if (!slot)
		{
			NWAV_LOG("NWAV play: file=" << fileID << " failed, every stream has a priority above " << u32(params.priority) << "\n");
			OS_UnlockMutex(&lock);
			return InvalidHandle;
		}

		Slot& s = *slot;
		releaseSlot(s);

		FS_InitFile(&s.file);
		if (!FS_OpenFileFast(&s.file, FSFileID{ romArchive, fileID }))
		{
			NWAV_LOG("NWAV play: file=" << fileID << " failed, the file could not be opened\n");
			OS_UnlockMutex(&lock);
			return InvalidHandle;
		}

		if (!Core::parseWav(&s.file, s.info))
		{
			NWAV_LOG("NWAV play: file=" << fileID << " failed, not a playable WAV\n");
			FS_CloseFile(&s.file);
			OS_UnlockMutex(&lock);
			return InvalidHandle;
		}

		const u32 ringSize = NWAV_PAGES * NWAV_PAGE_FRAMES * bytesPerSample(s);
		u8* memory = static_cast<u8*>(initParams.allocate(ringSize * s.info.channels + NWAV_READ_SIZE));
		if (!memory)
		{
			NWAV_LOG("NWAV play: file=" << fileID << " failed, could not allocate " << (ringSize * s.info.channels + NWAV_READ_SIZE) << " bytes\n");
			FS_CloseFile(&s.file);
			OS_UnlockMutex(&lock);
			return InvalidHandle;
		}

		s.memory = memory;
		s.ringSize = ringSize;
		s.ring[0] = memory;
		s.ring[1] = memory + ringSize;
		Core::initDecoder(s.dec, &s.file, s.info, memory + ringSize * s.info.channels);

		Core::Playhead& ph = s.ph;
		switch (params.loop)
		{
		case LoopMode::File:
			ph.looping = s.info.hasLoop;
			break;
		case LoopMode::Off:
			ph.looping = false;
			break;
		case LoopMode::On:
			ph.looping = true;
			break;
		}

		ph.loopStart = s.info.hasLoop ? s.info.loopStart : 0;
		ph.limit = ph.looping && s.info.hasLoop ? s.info.loopEnd : s.info.numFrames;
		ph.cursor = params.startFrame < ph.limit ? params.startFrame : ph.loopStart;
		ph.ended = false;

		s.handle = (nextGeneration << 8) | slotIndex(s);
		nextGeneration = (nextGeneration + 1) & 0xFFFFFF;
		if (nextGeneration == 0)
			nextGeneration = 1;

		s.active = true;
		s.paused = false;
		s.suspended = false;
		s.priority = params.priority;
		s.pan = params.pan;
		s.serial = nextSerial++;
		s.tags = params.tags;
		s.speed = params.speed;
		s.decibelOffset = 0;
		s.onEvent = params.onEvent;
		s.userData = params.userData;

		s.volume = params.fadeInFrames ? 0 : s32(params.volume) << VolumeShift;
		s.fadeFrames = 0;
		s.stopAfterFade = false;
		if (params.fadeInFrames)
			startFade(s, params.volume, params.fadeInFrames, false);

		startTimers(s);

		NWAV_LOG("NWAV play: file=" << fileID << " handle=" << s.handle << " priority=" << u32(params.priority)
			<< " " << (s.info.format == Core::Format::Pcm8 ? "PCM8" : s.info.format == Core::Format::Pcm16 ? "PCM16" : "IMA")
			<< " channels=" << u32(s.info.channels) << " rate=" << s.info.sampleRate << " frames=" << s.info.numFrames
			<< " events=" << u32(s.info.numEvents));
		if (ph.looping)
			NWAV_LOG(" loop=" << ph.loopStart << "-" << ph.limit);
		NWAV_LOG("\n");

		Handle handle = s.handle;
		OS_UnlockMutex(&lock);
		return handle;
	}

	static void stopSlot(Slot& s, u32 fadeFrames)
	{
		if (fadeFrames == 0 || s.paused)
			releaseSlot(s);
		else
			startFade(s, 0, fadeFrames, true);
	}

	void stop(Handle handle, u32 fadeFrames)
	{
		OS_LockMutex(&lock);
		if (Slot* s = getSlot(handle))
			stopSlot(*s, fadeFrames);
		OS_UnlockMutex(&lock);
	}

	void stopAll(u32 fadeFrames, u32 belowPriority)
	{
		OS_LockMutex(&lock);
		for (Slot& s : slots)
		{
			if (s.active && s.priority < belowPriority)
				stopSlot(s, fadeFrames);
		}
		OS_UnlockMutex(&lock);
	}

	void stopAllTagged(u32 tags, u32 fadeFrames)
	{
		OS_LockMutex(&lock);
		for (Slot& s : slots)
		{
			if (s.active && (s.tags & tags))
				stopSlot(s, fadeFrames);
		}
		OS_UnlockMutex(&lock);
	}

	bool isPlaying(Handle handle)
	{
		return getSlot(handle) != nullptr;
	}

	bool isPaused(Handle handle)
	{
		Slot* s = getSlot(handle);
		return s && s->paused;
	}

	static void setSlotPaused(Slot& s, bool paused)
	{
		if (s.paused == paused)
			return;

		if (paused)
		{
			stopTimers(s);
			s.paused = true;
			return;
		}

		s.paused = false;
		if (!restart(s))
			s.finished = true;
	}

	void setPaused(Handle handle, bool paused)
	{
		OS_LockMutex(&lock);
		if (Slot* s = getSlot(handle))
			setSlotPaused(*s, paused);
		OS_UnlockMutex(&lock);
	}

	void setPausedAll(bool paused)
	{
		OS_LockMutex(&lock);
		for (Slot& s : slots)
		{
			if (s.active)
				setSlotPaused(s, paused);
		}
		OS_UnlockMutex(&lock);
	}

	u32 getVolume(Handle handle)
	{
		Slot* s = getSlot(handle);
		return s ? u32(s->volume >> VolumeShift) : 0;
	}

	void setVolume(Handle handle, u32 volume, u32 frames)
	{
		if (volume > 127)
			volume = 127;

		OS_LockMutex(&lock);
		if (Slot* s = getSlot(handle))
		{
			if (!s->stopAfterFade)
			{
				startFade(*s, volume, frames, false);
				applyVolume(*s);
			}
		}
		OS_UnlockMutex(&lock);
	}

	void setDecibelOffset(Handle handle, s32 decibels)
	{
		OS_LockMutex(&lock);
		if (Slot* s = getSlot(handle))
		{
			s->decibelOffset = decibels;
			applyVolume(*s);
		}
		OS_UnlockMutex(&lock);
	}

	fx32 getSpeed(Handle handle)
	{
		Slot* s = getSlot(handle);
		return s ? s->speed : 0;
	}

	void setSpeed(Handle handle, fx32 speed)
	{
		OS_LockMutex(&lock);
		if (Slot* s = getSlot(handle))
		{
			if (s->speed != speed)
			{
				s->speed = speed;
				if (!s->paused)
				{
					stopTimers(*s);
					if (!restart(*s))
						s->finished = true;
				}
			}
		}
		OS_UnlockMutex(&lock);
	}

	void* getUserData(Handle handle)
	{
		Slot* s = getSlot(handle);
		return s ? s->userData : nullptr;
	}

	u32 getPosition(Handle handle)
	{
		OS_LockMutex(&lock);
		u32 frame = 0;
		if (Slot* s = getSlot(handle))
		{
			frame = playingFrame(*s);
			if (frame == NoFrame)
				frame = s->ph.limit;
		}
		OS_UnlockMutex(&lock);
		return frame;
	}

	// Sleep mode powers the sound circuit down under the channels and the alarms, so
	// the ring and its refills come back out of phase. As NNS does for its capture
	// and stream players, the timers stop before sleep and restart on wake: a stream
	// resumes from the page that was playing, like an unpause.
	static void beginSleep(void*)
	{
		OS_LockMutex(&lock);
		for (Slot& s : slots)
		{
			s.suspended = s.active && !s.paused;
			if (s.suspended)
				setSlotPaused(s, true);
		}
		OS_UnlockMutex(&lock);

		// The stop has to reach the ARM7 before it sleeps.
		u32 tag = SND_GetCurrentCommandTag();
		SND_FlushCommand(SND_COMMAND_BLOCK);
		SND_WaitForCommandProc(tag);
	}

	static void endSleep(void*)
	{
		OS_LockMutex(&lock);
		for (Slot& s : slots)
		{
			if (s.suspended)
			{
				s.suspended = false;
				setSlotPaused(s, false);
			}
		}
		OS_UnlockMutex(&lock);
	}

	void init(const InitParams& params)
	{
		initParams = params;
		romArchive = FS_FindArchive("rom", 3);
		if (!romArchive)
			NWAV_LOG("NWAV init: the ROM archive was not found, nothing will play\n");

		u32 mask = 0;
		for (u32 i = 0; i < NWAV_MAX_STREAMS * 2; i++)
			mask |= 1 << slotChannels[i];
		SND_LockChannel(mask, 0);

		OS_InitMutex(&lock);
		OS_InitMessageQueue(&msgQ, msgBuf, NWAV_MAX_STREAMS * 2);

		// First in line, so the streams stop before the rest of the sound system sleeps.
		PM_SetSleepCallbackInfo(&preSleepInfo, beginSleep, nullptr);
		PM_SetSleepCallbackInfo(&postSleepInfo, endSleep, nullptr);
		PM_PrependPreSleepCallback(&preSleepInfo);
		PM_AppendPostSleepCallback(&postSleepInfo);
		OS_CreateThread(
			&strmThread,
			streamThread,
			nullptr,
			reinterpret_cast<u8*>(strmThreadStack) + sizeof(strmThreadStack),
			sizeof(strmThreadStack),
			NWAV_THREAD_PRIORITY
		);
		OS_WakeupThreadDirect(&strmThread);
	}
}
