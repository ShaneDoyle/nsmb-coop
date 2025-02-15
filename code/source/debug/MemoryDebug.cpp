#ifdef NTR_DEBUG

#include <nsmb/core/system/memory.hpp>
#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/extra/log.hpp>

// Logging for debugging memory

static u32 loadingFileID = 0;

asm(R"(
.type FS_Cache_CacheEntry_loadFile_SUPER, %function
FS_Cache_CacheEntry_loadFile_SUPER:
	PUSH    {R4-R6,LR}
	B       0x0200A238

.type FS_Cache_CacheEntry_loadFileToOverlay_SUPER, %function
FS_Cache_CacheEntry_loadFileToOverlay_SUPER:
	PUSH    {R4-R6,LR}
	B       0x0200A198

.type Heap_allocate_SUPER, %function
Heap_allocate_SUPER:
	PUSH    {R4-R7,LR}
	B       0x02045044

.type Heap_deallocate_SUPER, %function
Heap_deallocate_SUPER:
	PUSH    {R4,R5,LR}
	B       0x02044D98

ncp_jump(0x02019440)
	print   "Called ModelAnm::init (from %lr%)\n"
	PUSH    {R4-R7,LR}
	B       0x02019440+4
)");
extern "C" {
	void* FS_Cache_CacheEntry_loadFile_SUPER(FS::Cache::CacheEntry* self, u32 extFileID, bool compressed);
	void* FS_Cache_CacheEntry_loadFileToOverlay_SUPER(FS::Cache::CacheEntry* self, u32 extFileID, bool compressed);
	void* Heap_allocate_SUPER(Heap* self, u32 size, int align);
	void Heap_deallocate_SUPER(Heap* self, void* ptr);
}

static u32 getRealFileID(u32 extFileID)
{
	return (extFileID & 0xFFFF) + 131;
}

ncp_jump(0x0200A234)
void* FS_Cache_CacheEntry_loadFile_OVERRIDE(FS::Cache::CacheEntry* self, u32 extFileID, bool compressed)
{
	loadingFileID = extFileID;

	u32 fileSize = FS::getFileSize(extFileID);

	u32 maxAllocateableSize = Memory::currentHeapPtr->vMaxAllocatableSize(4);

	if (scast<s32>(maxAllocateableSize - fileSize) < 0)
	{
		Log() << "ERROR: Could not load file with ID " << getRealFileID(extFileID) << " (" << fileSize << " bytes). " << maxAllocateableSize << " bytes free.\n";
		OS_Terminate();
	}

	void* data = FS_Cache_CacheEntry_loadFile_SUPER(self, extFileID, compressed);

	Log() << "INFO: Loaded file with ID " << getRealFileID(extFileID) << " (" << self->size << " bytes). " << Memory::currentHeapPtr->vMaxAllocatableSize() << " bytes free.\n";

	return data;
}

ncp_jump(0x0200A194)
void* FS_Cache_CacheEntry_loadFileToOverlay_OVERRIDE(FS::Cache::CacheEntry* self, u32 extFileID, bool compressed)
{
	loadingFileID = extFileID;

	u32 fileSize = FS::getFileSize(extFileID);

	u32 requiredSize = (fileSize + 15) & 0xFFFFFFF0;

	if (scast<s32>(FS::Cache::overlayFileSize - requiredSize) < 0)
	{
		Log() << "ERROR: Could not load file with ID " << getRealFileID(extFileID) << " (" << requiredSize << " bytes) to overlay. " << FS::Cache::overlayFileSize << " bytes free.\n";
		OS_Terminate();
	}

	void* data = FS_Cache_CacheEntry_loadFileToOverlay_SUPER(self, extFileID, compressed);

	Log() << "INFO: Loaded file with ID " << getRealFileID(extFileID) << " (" << requiredSize << " bytes) to overlay. " << FS::Cache::overlayFileSize << " bytes free.\n";

	return data;
}

ncp_repl(0x020450D4, "NOP")

ncp_jump(0x02045040)
void* Heap_allocate_OVERRIDE(Heap* self, u32 size, int align)
{
	void* ptr = Heap_allocate_SUPER(self, size, align);

	if (!ptr && (self->flags & 0x4000) != 0)
	{
		Log() << "RAM: Out of memory. Tried to allocate " << size << " bytes. " << self->vMaxAllocatableSize(align) << " bytes free.\n";
		OS_Terminate();
	}

	//if (self == Memory::currentHeapPtr)
	//	Log() << "ALLOC RAM: " << self->vMemoryLeft() << " bytes free.\n";

	return ptr;
}

ncp_jump(0x02044D94)
void Heap_deallocate_OVERRIDE(Heap* self, void* ptf)
{
	Heap_deallocate_SUPER(self, ptf);

	//if (self == Memory::currentHeapPtr)
	//	Log() << "FREE  RAM: " << self->vMemoryLeft() << " bytes free.\n";
}

ncp_hook(0x021726D8, 55)
void SetupFSCacheToUseOverlay55_AT_BEGIN()
{
	Log() << "INFO: Cache overlay reset. " << FS::Cache::overlayFileSize << " bytes free.\n";
}

#define NNS_GFD_TEX_SLOT_SIZE               0x20000
#define NNS_GFD_NUM_TEX_VRAM_REGION         5
#define NNS_GFD_NUM_TEX_VRAM_REGION_4x4     2

struct NNSGfdFrmTexRegionState
{
    u32 head;
    u32 tail;
    BOOL bActive;
    const BOOL bHalfSize;
    const u16 index;
    const u16 pad16_;
    const u32 baseAddress;
};

asm(R"(
	vramRegions_ = 0x02085364
	texNrmSearchArray_ = 0x02085350
	tex4x4SearchArray_ = 0x02085348
)");

extern NNSGfdFrmTexRegionState vramRegions_[NNS_GFD_NUM_TEX_VRAM_REGION];
extern NNSGfdFrmTexRegionState* texNrmSearchArray_[NNS_GFD_NUM_TEX_VRAM_REGION];
extern NNSGfdFrmTexRegionState* tex4x4SearchArray_[NNS_GFD_NUM_TEX_VRAM_REGION_4x4];

static inline NNSGfdFrmTexRegionState* getTex4x4IdxRegion(const NNSGfdFrmTexRegionState* pRegion)
{
    switch (pRegion->index)
    {
        case 0: return &vramRegions_[1];
        case 3: return &vramRegions_[2];
        default: break;
    }
    return nullptr;
}

static u32 getFreeTex4x4Vram(u32 i)
{
    u32 freeMemory = 0;

    NNSGfdFrmTexRegionState* pRegion = tex4x4SearchArray_[i];

    if (pRegion->bActive)
    {
		NNSGfdFrmTexRegionState* pPltRegion = getTex4x4IdxRegion(pRegion);

        if (pPltRegion->bActive)
        {
            u32 texCapacity = pRegion->tail - pRegion->head;
            u32 pltCapacity = (pPltRegion->tail - pPltRegion->head) * 2;

			freeMemory += Math::min(texCapacity, pltCapacity);
        }
    }

    return freeMemory;
}

static u32 getFreeTex4x4Vram()
{
    u32 freeMemory = 0;

    for (u32 i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION_4x4; i++)
        freeMemory += getFreeTex4x4Vram(i);

    return freeMemory;
}

static u32 getMaxAllocatableTex4x4Vram()
{
    u32 maxAllocatable = 0;

    for (u32 i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION_4x4; i++)
    {
        u32 freeMemory = getFreeTex4x4Vram(i);
		if (freeMemory > maxAllocatable)
			maxAllocatable = freeMemory;
    }

    return maxAllocatable;
}

static u32 getFreeTexNrmVram(u32 i)
{
    u32 freeMemory = 0;

	NNSGfdFrmTexRegionState* pRegion = texNrmSearchArray_[i];

	if (pRegion->bActive)
		freeMemory += pRegion->tail - pRegion->head;

    return freeMemory;
}

static u32 getFreeTexNrmVram()
{
    u32 freeMemory = 0;

    for (u32 i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++)
        freeMemory += getFreeTexNrmVram(i);

    return freeMemory;
}

static u32 getMaxAllocatableTexNrmVram()
{
    u32 maxAllocatable = 0;

    for (u32 i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++)
    {
        u32 freeMemory = getFreeTexNrmVram(i);
		if (freeMemory > maxAllocatable)
			maxAllocatable = freeMemory;
    }

    return maxAllocatable;
}

static void dump3DVramState()
{
	NNSGfdFrmPlttVramState plttState;

	NNS_GfdGetFrmPlttVramState(&plttState);

	Log() << "  TEXNRM VRAM: " << (getFreeTexNrmVram()) << " bytes free. " << getMaxAllocatableTexNrmVram() << " bytes allocatable.\n";
	Log() << "  TEX4X4 VRAM: " << (getFreeTex4x4Vram()) << " bytes free. " << getMaxAllocatableTex4x4Vram() << " bytes allocatable.\n";
	Log() << "  PLTT   VRAM: " << (plttState.address[1] - plttState.address[0]) << " bytes free.\n";
}

ncp_call(0x02009E1C)
BOOL FS_Cache_Internal_setup3DFile_AT_NNS_G3dResDefaultSetup_CALL(void* pResData)
{
	BOOL result = NNS_G3dResDefaultSetup(pResData);

	if (result == FALSE)
	{
		Log() << "ERROR: Failed to setup 3D resource (File ID: " << getRealFileID(loadingFileID) << "), possibly no more VRAM free.\n";
		dump3DVramState();
		OS_Terminate();
	}

	Log() << "INFO: Setup 3D resource (File ID: " << getRealFileID(loadingFileID) << ").\n";
	dump3DVramState();

	return result;
}

#endif
