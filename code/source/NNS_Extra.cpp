#include <nnsys.h>

#define NNS_GFD_TEX_SLOT_SIZE          		0x20000

#define NNS_GFD_NUM_TEX_VRAM_REGION         5
#define NNS_GFD_NUM_TEX_VRAM_REGION_4x4     2
#define NNS_GFD_INVALID_ADDR                0xFFFFFFFF

#define NNS_GFD_TEXPLTT_BASE_ADDR	0x00000

typedef struct NNSGfdFrmTexRegionState
{
    u32 head;
    u32 tail;

    BOOL bActive;

    const BOOL bHalfSize;        // TODO: Plan to change so that it saves size directly.

    const u16 index;
    const u16 pad16_;
    const u32 baseAddress;

} NNSGfdFrmTexRegionState;

typedef struct NNSGfdFramPlttVramManager
{
    u32 loAddr;
    u32 hiAddr;
    u32 szTotal;

} NNSGfdFrmPlttVramManager;

asm(R"(
	vramRegions_ = 0x02085364
	texNrmSearchArray_ = 0x02085350
	tex4x4SearchArray_ = 0x02085348
	s_managerState_ = 0x0208FEA0
)");

extern NNSGfdFrmTexRegionState vramRegions_[NNS_GFD_NUM_TEX_VRAM_REGION];
extern NNSGfdFrmTexRegionState* texNrmSearchArray_[NNS_GFD_NUM_TEX_VRAM_REGION];
extern NNSGfdFrmTexRegionState* tex4x4SearchArray_[NNS_GFD_NUM_TEX_VRAM_REGION_4x4];
extern NNSGfdFrmPlttVramManager s_managerState_;

NNS_GFD_INLINE NNSGfdFrmTexRegionState* Get4x4IdxRegion_( const NNSGfdFrmTexRegionState* pRegion )
{
    NNS_GFD_NULL_ASSERT( pRegion );
    switch( pRegion->index )
    {
        case 0:return &vramRegions_[1];
        case 3:return &vramRegions_[2];
        // NG
        default:NNS_GFD_WARNING("Invalid region is detected. in Get4x4IdxRegion_(). ");break;
    }
    return NULL;
}

/*NNS_GFD_INLINE BOOL NNS_GfdIs4x4TexKey(NNSGfdTexKey texKey)
{
    // The 31st bit is used for the b4x4Comp flag, so we shift right by 31 bits
    return (texKey >> 31) & 0x1;
}*/

int NNS_EXTRA_GfdFreeFrmTexVram(NNSGfdTexKey texKey, BOOL is4x4)
{
    //NNS_GFD_ASSERTMSG(IsVramManagerValid_(), "Make sure to Initialize the manager.");

    // Extract information from the texKey
    u32 addr = NNS_GfdGetTexKeyAddr(texKey);
    u32 szByte = NNS_GfdGetTexKeySize(texKey);
    //BOOL is4x4 = NNS_GfdIs4x4TexKey(texKey);

    if (addr == NNS_GFD_INVALID_ADDR || szByte == 0) {
        NNS_GFD_WARNING("Invalid texKey provided for free.");
        return -1; // Failure, invalid key
    }

    // Find the corresponding region
    if (is4x4) {
        // Handle 4x4 compressed texture freeing
        for (int i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION_4x4; i++) {
            NNSGfdFrmTexRegionState* pRegion = tex4x4SearchArray_[i];
            if (pRegion->bActive && addr >= pRegion->baseAddress && addr < pRegion->baseAddress + NNS_GFD_TEX_SLOT_SIZE) {
                // Free the memory from the region's head
                pRegion->head -= szByte;

                // Also free the corresponding index table region
                NNSGfdFrmTexRegionState* pIdxRegion = Get4x4IdxRegion_(pRegion);
                if (pIdxRegion != NULL && pIdxRegion->bActive) {
                    pIdxRegion->head -= szByte / 2;
                }
                return 0; // Success
            }
        }
    } else {
        // Handle normal texture freeing
        for (int i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++) {
            NNSGfdFrmTexRegionState* pRegion = texNrmSearchArray_[i];
            if (pRegion->bActive && addr >= pRegion->baseAddress && addr < pRegion->baseAddress + NNS_GFD_TEX_SLOT_SIZE) {
                // Free the memory from the region's tail
                pRegion->tail += szByte;
                return 0; // Success
            }
        }
    }

    // If we reach here, no valid region was found to free the memory
    NNS_GFD_WARNING("Failed to find corresponding region for texKey.");
    return -1; // Failure
}

int NNS_EXTRA_GfdFreeFrmPlttVram(NNSGfdPlttKey plttKey)
{
    //NNS_GFD_ASSERT( IsVramManagerValid_() );

    // Extract address and size from plttKey
    u32 addr = NNS_GfdGetPlttKeyAddr(plttKey);
    u32 size = NNS_GfdGetPlttKeySize(plttKey);

    // Validate the address and size
    if (addr == NNS_GFD_INVALID_ADDR || size == 0) {
        NNS_GFD_WARNING("Invalid palette key provided for free.");
        return -1; // Failure: invalid palette key
    }

    // Check if the address belongs to the lower or upper region and free accordingly
    if (addr >= s_managerState_.loAddr && addr < s_managerState_.hiAddr) {
        // If the address is from the lower region, we "free" it by reducing loAddr
        if (addr == s_managerState_.loAddr - size) {
            s_managerState_.loAddr -= size;
            return 0; // Success
        }
    } else if (addr < s_managerState_.hiAddr && addr >= NNS_GFD_TEXPLTT_BASE_ADDR) {
        // If the address is from the upper region, we "free" it by increasing hiAddr
        if (addr + size == s_managerState_.hiAddr) {
            s_managerState_.hiAddr += size;
            return 0; // Success
        }
    }

    // If the address didn't match any valid region
    NNS_GFD_WARNING("Failed to free palette VRAM: Address out of range.");
    return -1; // Failure
}

void NNS_EXTRA_G3dTexUnload(NNSG3dResTex* pTex)
{
	if (pTex->texInfo.vramKey)
		NNS_EXTRA_GfdFreeFrmTexVram(pTex->texInfo.vramKey, FALSE);
	else if (pTex->tex4x4Info.vramKey)
		NNS_EXTRA_GfdFreeFrmTexVram(pTex->tex4x4Info.vramKey, TRUE);

	if (pTex->plttInfo.vramKey)
		NNS_EXTRA_GfdFreeFrmPlttVram(pTex->texInfo.vramKey);
}
