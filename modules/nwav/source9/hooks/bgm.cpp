/*============================================================\
|  NWAV BGM replacement.                                      |
|  A sequence that has a stream in the BGM table never starts:|
|  the stream plays in its place. The game's controls for the |
|  music (volume and fades, pause, stop, tempo) are forwarded |
|  to the stream. On hurry up, an entry's hurry track replaces|
|  the stream. With no sequence player, the game does not     |
|  copy the sequence variables either, so only the stream's   |
|  own "bahp" events reach the enemies.                       |
\============================================================*/

#include "nwav/hooks.hpp"

#include <nsmb/game/stage/player/common.hpp>
#include <nsmb/game/sound.hpp>

namespace NWAV::Hooks
{
	static Handle bgmStream = InvalidHandle;
	static const BGMEntry* bgmEntry;	// The entry bgmStream plays.
	static bool bgmHurry;				// Whether bgmStream plays the entry's hurry track.
	static bool playingTransition;		// Inside Game::playTransitionMusic, which plays at the sequence's own tempo.

#ifdef NWAV_BGM_BAHP
	// The game's "bahp" flag, which makes the enemies dance on the beat. A BGM
	// stream marks the beats with cue points named "bahp". The flag stays set for
	// one frame, as the sequence's own bahp did, and only in single player: there
	// the enemies that read it (see CoopFixes::Blockhopper) use their own timing.
	static u8& bahpFlag = *rcast<u8*>(0x02088B9C);
	static bool bahpRaised;
#endif

	[[gnu::weak]] void onBGMEvent(Handle, u32)
	{
	}

	static void dispatchBGMEvent(Handle handle, u32 eventID, void*)
	{
#ifdef NWAV_BGM_BAHP
		if (eventID == EventID("bahp") && Game::getPlayerCount() == 1)
		{
			bahpFlag = 1;
			bahpRaised = true;
		}
#endif
		onBGMEvent(handle, eventID);
	}

	[[gnu::weak]] const BGMEntry* getBGMTable(u32& count)
	{
		count = 0;
		return nullptr;
	}

	static const BGMEntry* findEntry(u32 seqID)
	{
		u32 count;
		const BGMEntry* table = getBGMTable(count);
		for (u32 i = 0; i < count; i++)
		{
			if (table[i].seqID == seqID)
				return &table[i];
		}
		return nullptr;
	}

	Handle getBGMStream()
	{
		return bgmStream;
	}

	void updateBGM()
	{
#ifdef NWAV_BGM_BAHP
		// Called by NWAV::Hooks::update right before NWAV::update dispatches this frame's events.
		if (bahpRaised)
		{
			bahpFlag = 0;
			bahpRaised = false;
		}
#endif

		if (!NWAV::isPlaying(bgmStream))
		{
			bgmStream = InvalidHandle;
			bgmEntry = nullptr;
		}
	}

	static Handle playBGMFile(const BGMEntry* entry, u32 fileID, u8 volume)
	{
		Handle handle = playExtFile(fileID, { .volume = volume, .priority = BGMPriority, .onEvent = dispatchBGMEvent });
		if (handle != InvalidHandle)
			NWAV::setDecibelOffset(handle, entry->decibels);
		return handle;
	}

	// The game's music controls are each replaced by a jump to a function below, which
	// runs the original through a copy of its first instruction and a branch past it.
	// Hooking the functions themselves, not their calls, covers every caller,
	// including overlays and other hacks.
	ncp_asmfunc static BOOL startSeqOriginal(int seqID) { asm("STMFD SP!, {R4-R6,LR}\nB 0x0204E5B0"); }
	ncp_asmfunc static void fadeSeqVolumeOriginal(int volume, int frames) { asm("STMFD SP!, {R4,LR}\nB 0x0204DCB8"); }
	ncp_asmfunc static void stopSeqOriginal(int frames) { asm("STMFD SP!, {R4,LR}\nB 0x0204E424"); }
	ncp_asmfunc static BOOL setTempoRatioOriginal(int ratio) { asm("STMFD SP!, {R4,LR}\nB 0x0204E530"); }
	ncp_asmfunc static void playTransitionMusicOriginal(s32 seqID) { asm("STMFD SP!, {R4,LR}\nB 0x020210E0"); }

	// Whether the tempo the game sets right after a sequence starts is the hurry-up one.
	// SND::playBGM, the only caller of SND::Internal::startSeq, applies SND::currentTempoRatio.
	static bool isHurryTempo(int ratio)
	{
		return ratio > 256 && !playingTransition;
	}

	// SND::Internal::startSeq(seqID)
	// Replaced sequences start their stream instead of the sequence.
	ncp_jump(0x0204E5AC)
	BOOL NWAV_startSeq(int seqID)
	{
		const BGMEntry* entry = findEntry(seqID);

		// Whatever played before is replaced, a sequence or a stream.
		NWAV::stop(bgmStream);
		bgmStream = InvalidHandle;
		bgmEntry = nullptr;
		bgmHurry = false;

		if (entry)
		{
			SND::Internal::stopSeq(0);

			// Starting hurried plays the hurry track right away, instead of the
			// normal one only for the tempo that follows to replace it.
			bool hurry = entry->hurryFileID != NO_HURRY_TRACK && isHurryTempo(SND::currentTempoRatio);
			bgmStream = playBGMFile(entry, hurry ? entry->hurryFileID : entry->fileID, 127);
			if (bgmStream != InvalidHandle)
			{
				NWAV_LOG("NWAV BGM: seq=" << seqID << " plays " << (hurry ? "hurried " : "") << "as handle=" << bgmStream << "\n");
				bgmEntry = entry;
				bgmHurry = hurry;
				return TRUE;
			}
			NWAV_LOG("NWAV BGM: seq=" << seqID << " stream failed, the sequence plays instead\n");
		}

		return startSeqOriginal(seqID);
	}

	// SND::Internal::fadeSeqVolume(volume, frames)
	ncp_jump(0x0204DCB4)
	void NWAV_fadeSeqVolume(int volume, int frames)
	{
		fadeSeqVolumeOriginal(volume, frames);
		NWAV::setVolume(bgmStream, volume, frames);
	}

	// SND::Internal::stopSeq(frames)
	ncp_jump(0x0204E420)
	void NWAV_stopSeq(int frames)
	{
		stopSeqOriginal(frames);
		NWAV::stop(bgmStream, frames);
	}

	// The pause is in pause.cpp, shared with SoundIntegration.

	// SND::Internal::setTempoRatio(ratio), where 256 is the sequence's own tempo.
	// Above it, an entry with a hurry track switches to it, starting from the
	// beginning, and switches back once the tempo returns. With BGMFollowTempo,
	// a stream without one speeds up instead.
	ncp_jump(0x0204E52C)
	BOOL NWAV_setTempoRatio(int ratio)
	{
		BOOL result = setTempoRatioOriginal(ratio);

		// The transition music gets the current tempo from SND::playBGM and then
		// 256: the stream stays at its own tempo throughout.
		if (playingTransition)
			ratio = 256;

		if (bgmEntry && bgmEntry->hurryFileID != NO_HURRY_TRACK)
		{
			bool hurry = isHurryTempo(ratio);
			if (hurry != bgmHurry)
			{
				u8 volume = NWAV::getVolume(bgmStream);
				NWAV::stop(bgmStream);
				bgmStream = playBGMFile(bgmEntry, hurry ? bgmEntry->hurryFileID : bgmEntry->fileID, volume);
				bgmHurry = hurry;
				NWAV_LOG("NWAV BGM: " << (hurry ? "hurry" : "normal") << " track plays as handle=" << bgmStream << "\n");
				if (bgmStream == InvalidHandle)
					bgmEntry = nullptr;
			}
			return result;
		}

#ifdef NWAV_BGM_FOLLOW_TEMPO
		NWAV::setSpeed(bgmStream, ratio << (FX32_SHIFT - 8));
#endif
		return result;
	}

	// Game::playTransitionMusic(seqID)
	ncp_jump(0x020210DC)
	void NWAV_playTransitionMusic(s32 seqID)
	{
		playingTransition = true;
		playTransitionMusicOriginal(seqID);
		playingTransition = false;
	}
}
