#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	void* NNS_G3dGetTex(void* ptrToFile);

	void* NNS_SndHeapCreate(void* startAddress, u32 size);
	void NNS_SndHeapDestroy(void* heap);

#ifdef __cplusplus
}
#endif