/*============================================================\
|  Frame decoder.                                             |
|  Turns interleaved WAV frames into one buffer per channel,  |
|  the layout the DS hardware channels play from.             |
\============================================================*/

#include "stream.hpp"

namespace NWAV::Core
{
	static const s16 imaStepTable[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};

	static const s8 imaIndexTable[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

	static inline s32 imaNibble(s32& pred, s32& index, u32 nibble)
	{
		s32 step = imaStepTable[index];
		s32 diff = step >> 3;
		if (nibble & 1) diff += step >> 2;
		if (nibble & 2) diff += step >> 1;
		if (nibble & 4) diff += step;

		if (nibble & 8)
		{
			pred -= diff;
			if (pred < -32768)
				pred = -32768;
		}
		else
		{
			pred += diff;
			if (pred > 32767)
				pred = 32767;
		}

		index += imaIndexTable[nibble & 7];
		if (index < 0)
			index = 0;
		else if (index > 88)
			index = 88;

		return pred;
	}

	static void seekByte(Decoder& dec, u32 offset)
	{
		FS_SeekFile(dec.file, s32(offset), FS_SEEK_SET);
		dec.filePos = offset;
		dec.bufPos = 0;
		dec.bufLen = 0;
	}

	// Reads the next chunk of data. Data is truncated to whole frames or words
	// and every read starts on one, so a buffer never splits a frame or word.
	static bool refill(Decoder& dec)
	{
		u32 len = dec.dataEnd - dec.filePos;
		if (len > NWAV_READ_SIZE)
			len = NWAV_READ_SIZE;
		if (len == 0)
			return false;

		s32 got = FS_ReadFile(dec.file, dec.buf, s32(len));
		if (got <= 0)
		{
			NWAV_LOG("NWAV decode: read failed at offset " << dec.filePos << ", the stream ends early\n");
			dec.filePos = dec.dataEnd;
			return false;
		}

		dec.filePos += u32(got);
		dec.bufPos = 0;
		dec.bufLen = u32(got);
		return true;
	}

	template<typename T>
	static void clear(T* left, T* right, u32 frames)
	{
		if (left)
			MI_CpuFill8(left, 0, frames * sizeof(T));
		if (right)
			MI_CpuFill8(right, 0, frames * sizeof(T));
	}

	static void decodePcm16(Decoder& dec, s16* left, s16* right, u32 frames)
	{
		const bool stereo = dec.info->channels == 2;
		const u32 frameSize = dec.info->blockAlign;

		while (frames)
		{
			if (dec.bufPos == dec.bufLen && !refill(dec))
			{
				clear(left, right, frames);
				return;
			}

			u32 count = (dec.bufLen - dec.bufPos) / frameSize;
			if (count > frames)
				count = frames;

			const u8* src = dec.buf + dec.bufPos;
			if (left)
			{
				if (stereo)
				{
					const u32* words = reinterpret_cast<const u32*>(src);
					for (u32 i = 0; i < count; i++)
					{
						u32 word = words[i];
						left[i] = s16(word);
						right[i] = s16(word >> 16);
					}
					right += count;
				}
				else
				{
					MI_CpuCopy8(src, left, count * 2);
				}
				left += count;
			}

			dec.bufPos += count * frameSize;
			frames -= count;
		}
	}

	static void decodePcm8(Decoder& dec, s8* left, s8* right, u32 frames)
	{
		const bool stereo = dec.info->channels == 2;
		const u32 frameSize = dec.info->blockAlign;

		while (frames)
		{
			if (dec.bufPos == dec.bufLen && !refill(dec))
			{
				clear(left, right, frames);
				return;
			}

			u32 count = (dec.bufLen - dec.bufPos) / frameSize;
			if (count > frames)
				count = frames;

			// WAV stores 8 bit samples unsigned, the hardware plays them signed.
			const u8* src = dec.buf + dec.bufPos;
			if (left)
			{
				if (stereo)
				{
					for (u32 i = 0; i < count; i++)
					{
						left[i] = s8(src[i * 2] ^ 0x80);
						right[i] = s8(src[i * 2 + 1] ^ 0x80);
					}
					right += count;
				}
				else
				{
					for (u32 i = 0; i < count; i++)
						left[i] = s8(src[i] ^ 0x80);
				}
				left += count;
			}

			dec.bufPos += count * frameSize;
			frames -= count;
		}
	}

	// Decodes the next word of every channel (or the block headers) into the stage.
	static bool decodeImaStage(Decoder& dec)
	{
		const Info& info = *dec.info;
		const u32 channels = info.channels;
		const u32 unit = 4 * channels;

		if (dec.bufPos == dec.bufLen && !refill(dec))
			return false;

		const u8* src = dec.buf + dec.bufPos;
		dec.bufPos += unit;

		if (dec.blockFrame == 0)
		{
			for (u32 ch = 0; ch < channels; ch++)
			{
				const u8* head = &src[ch * 4];
				s32 index = head[2];
				dec.pred[ch] = s16(head[0] | (head[1] << 8));
				dec.index[ch] = index > 88 ? 88 : index;
				dec.stage[ch][0] = s16(dec.pred[ch]);
			}
			dec.stageLen = 1;
		}
		else
		{
			for (u32 ch = 0; ch < channels; ch++)
			{
				const u8* word = &src[ch * 4];
				s16* out = dec.stage[ch];
				s32 pred = dec.pred[ch];
				s32 index = dec.index[ch];
				for (u32 i = 0; i < 4; i++)
				{
					u32 byte = word[i];
					out[i * 2] = s16(imaNibble(pred, index, byte & 0xF));
					out[i * 2 + 1] = s16(imaNibble(pred, index, byte >> 4));
				}
				dec.pred[ch] = pred;
				dec.index[ch] = index;
			}

			u32 left = info.framesPerBlock - dec.blockFrame;
			dec.stageLen = left < 8 ? u8(left) : 8;
		}

		dec.stagePos = 0;
		dec.blockFrame += dec.stageLen;
		if (dec.blockFrame >= info.framesPerBlock)
			dec.blockFrame = 0;
		return true;
	}

	static void decodeIma(Decoder& dec, s16* left, s16* right, u32 frames)
	{
		const bool stereo = dec.info->channels == 2;

		while (frames)
		{
			if (dec.stagePos == dec.stageLen && !decodeImaStage(dec))
			{
				clear(left, right, frames);
				return;
			}

			u32 count = dec.stageLen - dec.stagePos;
			if (count > frames)
				count = frames;

			if (left)
			{
				const s16* srcL = &dec.stage[0][dec.stagePos];
				for (u32 i = 0; i < count; i++)
					left[i] = srcL[i];
				left += count;

				if (stereo)
				{
					const s16* srcR = &dec.stage[1][dec.stagePos];
					for (u32 i = 0; i < count; i++)
						right[i] = srcR[i];
					right += count;
				}
			}

			dec.stagePos += u8(count);
			frames -= count;
		}
	}

	void initDecoder(Decoder& dec, FSFile* file, const Info& info, u8* buf)
	{
		MI_CpuFill8(&dec, 0, sizeof(Decoder));
		dec.file = file;
		dec.info = &info;
		dec.buf = buf;
		dec.dataEnd = info.dataOffset + info.dataSize;
	}

	void seek(Decoder& dec, u32 frame)
	{
		const Info& info = *dec.info;

		if (info.format != Format::Ima)
		{
			seekByte(dec, info.dataOffset + frame * info.blockAlign);
			return;
		}

		u32 block = frame / info.framesPerBlock;
		seekByte(dec, info.dataOffset + block * info.blockAlign);
		dec.blockFrame = 0;
		dec.stagePos = 0;
		dec.stageLen = 0;
		decodeIma(dec, nullptr, nullptr, frame % info.framesPerBlock);
	}

	void decode(Decoder& dec, void* left, void* right, u32 frames)
	{
		const Info& info = *dec.info;
		if (info.channels == 1 || !left)
			right = nullptr;

		switch (info.format)
		{
		case Format::Pcm8:
			decodePcm8(dec, static_cast<s8*>(left), static_cast<s8*>(right), frames);
			break;
		case Format::Pcm16:
			decodePcm16(dec, static_cast<s16*>(left), static_cast<s16*>(right), frames);
			break;
		case Format::Ima:
			decodeIma(dec, static_cast<s16*>(left), static_cast<s16*>(right), frames);
			break;
		}
	}
}
