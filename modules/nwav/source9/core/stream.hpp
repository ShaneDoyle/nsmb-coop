/*============================================================\
|  NWAV internal declarations.                                |
|  WAV parsing and frame decoding, shared by the player and   |
|  the host-side tests (which define NWAV_HOST_TEST).         |
\============================================================*/

#pragma once

#ifdef NWAV_HOST_TEST
#include "host_sdk.hpp"
#else
#include <nsmb_nitro.hpp>
#endif

#include "nwav/config.hpp"

namespace NWAV::Core
{
	enum class Format : u8
	{
		Pcm8,
		Pcm16,
		Ima
	};

	struct Event
	{
		u32 frame;
		u32 id;
	};

	struct Info
	{
		u32 dataOffset;
		u32 dataSize;			// Truncated to whole frames (PCM) or words (IMA).
		u32 sampleRate;
		u32 numFrames;
		u32 loopStart;
		u32 loopEnd;			// Exclusive.
		u16 blockAlign;			// Bytes per frame (PCM) or per block (IMA).
		u16 framesPerBlock;		// IMA only.
		Format format;
		u8 channels;
		u8 numEvents;
		bool hasLoop;
		Event events[NWAV_MAX_EVENTS];
	};

	// Reads the RIFF header of an open file. Leaves the file position undefined.
	bool parseWav(FSFile* file, Info& info);

	struct Decoder
	{
		FSFile* file;
		const Info* info;

		u8* buf;				// NWAV_READ_SIZE bytes, 4 byte aligned.
		u32 bufPos;
		u32 bufLen;
		u32 filePos;
		u32 dataEnd;

		// IMA-ADPCM state.
		s32 pred[2];
		s32 index[2];
		u32 blockFrame;
		s16 stage[2][8];
		u8 stagePos;
		u8 stageLen;
	};

	void initDecoder(Decoder& dec, FSFile* file, const Info& info, u8* buf);

	// Moves to a frame. For IMA this decodes from the start of its block, so the
	// decoder state at the frame is the same as when reached by playing.
	void seek(Decoder& dec, u32 frame);

	// Decodes frames into one buffer per channel: s8 for PCM8, s16 otherwise.
	// right is ignored for mono files. Null outputs skip the frames. Frames past
	// the end of the data come out as silence.
	void decode(Decoder& dec, void* left, void* right, u32 frames);

	struct Playhead
	{
		u32 cursor;		// Next frame to decode.
		u32 limit;		// The loop end when looping, the last frame otherwise.
		u32 loopStart;
		bool looping;
		bool ended;
	};

	// Renders frames from the playhead. Reaching the loop end mid-buffer seeks
	// back and keeps going, so the wrap lands on the exact frame; past the end
	// of a non-looping file it renders silence and sets ended. onSegment(start,
	// frames) is called for every run of consecutive file frames rendered.
	// Returns how many frames were rendered before the end.
	template<typename OnSegment>
	u32 render(Decoder& dec, Playhead& ph, u8* left, u8* right, u32 bytesPerSample, u32 frames, OnSegment&& onSegment)
	{
		u32 pos = 0;
		while (pos < frames)
		{
			if (ph.ended)
			{
				u32 rest = (frames - pos) * bytesPerSample;
				MI_CpuFill8(left + pos * bytesPerSample, 0, rest);
				if (right)
					MI_CpuFill8(right + pos * bytesPerSample, 0, rest);
				return pos;
			}

			u32 count = ph.limit - ph.cursor;
			if (count > frames - pos)
				count = frames - pos;

			if (count)
			{
				onSegment(ph.cursor, count);
				decode(dec, left + pos * bytesPerSample, right ? right + pos * bytesPerSample : nullptr, count);
				ph.cursor += count;
				pos += count;
			}

			if (ph.cursor >= ph.limit)
			{
				if (ph.looping)
				{
					ph.cursor = ph.loopStart;
					seek(dec, ph.cursor);
				}
				else
				{
					ph.ended = true;
				}
			}
		}
		return pos;
	}
}
