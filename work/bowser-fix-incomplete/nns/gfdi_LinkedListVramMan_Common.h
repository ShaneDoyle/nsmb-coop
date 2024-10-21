#ifndef NNS_GFDI_LINKEDLISTVRAMMAN_COMMON_H_
#define NNS_GFDI_LINKEDLISTVRAMMAN_COMMON_H_


#include <nnsys/gfd.h>

//------------------------------------------------------------------------------
typedef struct NNSiGfdLnkVramBlock NNSiGfdLnkVramBlock;

/*---------------------------------------------------------------------------*
  Name:         NNSiGfdLnkVramBlock

  Description:  Memory region management block
  
 *---------------------------------------------------------------------------*/
struct NNSiGfdLnkVramBlock
{
    u32                         addr;       // Region start address
    u32                         szByte;     // Region size (zero may not be used)
    
    NNSiGfdLnkVramBlock*        pBlkPrev;   // The previous region (No address-positional relation)
    NNSiGfdLnkVramBlock*        pBlkNext;   // The next region (No address-positional relation)
    
};

/*---------------------------------------------------------------------------*
  Name:         NNSiGfdLnkMemRegion

  Description:  Memory interval
                For items satisfying end > start
  
 *---------------------------------------------------------------------------*/
typedef struct NNSiGfdLnkMemRegion
{
    u32       start;
    u32       end;
    
}NNSiGfdLnkMemRegion;

/*---------------------------------------------------------------------------*
  Name:         NNSiGfdLnkVramMan

  Description:  Manager
                Unlike ordinary heaps, there is no management information list for used regions
                This is because with no relationship between the management information region (in main memory) and
                the management region address (in VRAM), it is difficult to use the look-up for management information
                for the used regions from the TextureKey (having the address and size information).
                When freeing, returns to the free area of the area used from the address + size. 
                
                
 *---------------------------------------------------------------------------*/
typedef struct NNSiGfdLnkVramMan
{
    NNSiGfdLnkVramBlock*         pFreeList;         // Unused region block list
      
}NNSiGfdLnkVramMan;




//------------------------------------------------------------------------------
// Function Declaration
//------------------------------------------------------------------------------
void NNSi_GfdDumpLnkVramManFreeListInfo
( 
    const NNSiGfdLnkVramBlock*      pFreeListHead,
    u32                             szReserved 
);

void 
NNSi_GfdInitLnkVramMan( NNSiGfdLnkVramMan* pMgr );


NNSiGfdLnkVramBlock* 
NNSi_GfdInitLnkVramBlockPool
( 
    NNSiGfdLnkVramBlock*    pArrayHead, 
    u32                     lengthOfArray 
); 

BOOL
NNSi_GfdAddNewFreeBlock
(
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32                     baseAddr,
    u32                     szByte
);


BOOL
NNSi_GfdAllocLnkVram
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32*                    pRetAddr,
    u32                     szByte
); 

BOOL
NNSi_GfdAllocLnkVramAligned
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32*                    pRetAddr,
    u32                     szByte,
    u32                     alignment
);

BOOL NNSi_GfdFreeLnkVram
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32                     addr,
    u32                     szByte
);





#endif // NNS_GFDI_LINKEDLISTVRAMMAN_COMMON_H_


