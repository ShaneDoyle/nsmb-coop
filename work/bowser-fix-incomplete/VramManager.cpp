#include "nnsys.h"

// Use the LinkedList Tex and Pltt VRAM manager instead of Frame VRAM manager
// This will allow us to deallocate loaded textures in-game

#define SIZE_VRAMMAN        0x40000     // Size of managed texture region
#define SIZE_VRAMMAN_4X4    0x21000     // Size of managed texture region (for 4x4 texture)
#define NUM_VRAMMAN_MEMBLK  20          // Number of managed blocks (the maximum value for the divided free region))
#define SIZE_PLTTMAN        0x10000     // Size of managed palette region

#define VRAM_SLOT_SIZE    0x20000

namespace VramManager {

u8* pMgrWorkTex = nullptr;
u8* pMgrWorkPltt = nullptr;

void initStage() {
    const u32 szWorkTex = NNS_GfdGetLnkTexVramManagerWorkSize(NUM_VRAMMAN_MEMBLK);
    pMgrWorkTex = new u8[szWorkTex];
    const u32 myMgrSize = 0x8000;
    u32 szNrm = myMgrSize + VRAM_SLOT_SIZE;
    // // Slot 0 for 4x4 use
    // NNS_GfdInitLnkTexVramManager( szNrm,
    //                               0,
    //                               pMgrWorkTex, szWorkTex, TRUE );

        // Slot 1 and under for 4x4 use
        NNS_GfdInitLnkTexVramManager(szNrm, myMgrSize, pMgrWorkTex, szWorkTex, TRUE);
	//NNS_GfdInitLnkTexVramManager(SIZE_VRAMMAN, SIZE_VRAMMAN_4X4, pMgrWorkTex, szWorkTex, TRUE);

    u32 szWorkPltt = NNS_GfdGetLnkPlttVramManagerWorkSize(NUM_VRAMMAN_MEMBLK);
    pMgrWorkPltt = new u8[szWorkPltt];
    NNS_GfdInitLnkPlttVramManager(SIZE_PLTTMAN, pMgrWorkPltt, szWorkPltt, TRUE);
}

void deinitStage() {
	delete[] pMgrWorkPltt;
	delete[] pMgrWorkTex;
}

}

asm(R"(
ncp_over(0x020A3294, 0)
	BL      _ZN11VramManager9initStageEv
	B       0x020A32A4
ncp_endover()

ncp_jump(0x02013A88)
	BL      _ZN2FS5Cache5clearEv // Keep replaced instruction
	BL      _ZN11VramManager11deinitStageEv
	B       0x02013A8C
)");
