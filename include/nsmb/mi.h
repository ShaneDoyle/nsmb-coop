#pragma once

#include <nds/ndstypes.h>

typedef struct
{
	u32     compParam : 4;
	u32     compType : 4;
	u32     destSize : 24;
}
MICompressionHeader;

typedef struct
{
	u8* destp;                     // Write-destination pointer:                     4B
	s32     destCount;                 // Remaining size to write:                     4B
	u32     length;                    // Remaining length of continuous write:              4B
	u16     destTmp;                   // Data being written:                     2B
	u8      destTmpCnt;                // Size of data being written:             1B
	u8      flags;                     // Compression flag:                      1B
	u8      flagIndex;                 // Current compression flag index:  1B
	u8      lengthFlg;                 // 'length' obtained flag:              1B
	u8      exFormat;                  // LZ77 compression extension option:          1B
	u8      _padding[1];               //                                 1B
	//                             Total is 20B
}
MIUncompContextLZ;

typedef void (*MIDmaCallback) (void*);

#ifdef __cplusplus
extern "C"
{
#endif

	static inline void MI_CpuClearFast(void* dest, u32 size);
	static inline void MI_CpuFillFast(void* dest, u32 data, u32 size);
	void MIi_CpuClear16(const void* srcp, void* destp, u32 size);
	void MI_CpuCopy8(const void* srcp, void* destp, u32 size);
	void MIi_CpuCopy8(const void* srcp, void* destp, u32 size);
	void MIi_CpuCopy16(const void* srcp, void* destp, u32 size);
	void MIi_CpuCopy32(const void* srcp, void* destp, u32 size);
	void MI_DmaCopy16(u32 dmaNo, const void* src, void* dest, u32 size);
	void MI_DmaCopy32(u32 dmaNo, const void* src, void* dest, u32 size);

	void MI_CpuFill8(void* dest, u8 data, u32 size);
	void MIi_CpuClearFast(u32 data, void* destp, u32 size);

	void MI_DmaCopy32Async(u32 dmaNo,
		const void* src,
		void* dest,
		u32 size,
		MIDmaCallback callback,
		void* arg);

	static inline void MI_CpuClearFast(void* dest, u32 size)
	{
		MI_CpuFillFast(dest, 0, size);
	}

	static inline void MI_CpuFillFast(void* dest, u32 data, u32 size)
	{
		MIi_CpuClearFast(data, dest, size);
	}

	void MI_InitUncompContextLZ(MIUncompContextLZ* context, u8* dest, const MICompressionHeader* header);
	s32 MI_ReadUncompLZ8(MIUncompContextLZ* context, const u8* data, u32 len);

#ifdef __cplusplus
}
#endif