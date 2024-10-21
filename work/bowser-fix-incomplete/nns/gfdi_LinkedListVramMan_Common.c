#include <nnsys/gfd/gfd_common.h>
#include "gfdi_LinkedListVramMan_Common.h"



//------------------------------------------------------------------------------
// Obtains the region from the block.
NNS_GFD_INLINE void 
GetRegionOfMemBlock_
(
    NNSiGfdLnkMemRegion*            pRegion,
    const NNSiGfdLnkVramBlock*      pBlk
)
{
    NNS_GFD_NULL_ASSERT( pBlk );
    NNS_GFD_NON_ZERO_ASSERT( pBlk->szByte );
    
    pRegion->start = pBlk->addr;
    pRegion->end   = pBlk->addr + pBlk->szByte;
    
    NNS_GFD_ASSERT( pRegion->end > pRegion->start );
}

//------------------------------------------------------------------------------
// Initializes the block from the region.
NNS_GFD_INLINE void InitBlockFromRegion_
( 
    NNSiGfdLnkVramBlock*        pBlk, 
    const NNSiGfdLnkMemRegion*  pRegion
)
{
    NNS_GFD_NULL_ASSERT( pBlk );
    NNS_GFD_ASSERT( pRegion->end > pRegion->start );
    
    pBlk->addr      = pRegion->start;
    pBlk->szByte    = (u32)(pRegion->end - pRegion->start);
    pBlk->pBlkPrev  = NULL;
    pBlk->pBlkNext  = NULL;
}

//------------------------------------------------------------------------------
// Initializes the block from the parameter types.
NNS_GFD_INLINE void InitBlockFromPrams_
( 
    NNSiGfdLnkVramBlock*    pBlk, 
    u32                     addr, 
    u32                     szByte 
)
{   
    NNS_GFD_NULL_ASSERT( pBlk );
    NNS_GFD_NON_ZERO_ASSERT( szByte );
    
    pBlk->addr      = addr;
    pBlk->szByte    = szByte;
    pBlk->pBlkPrev  = NULL;
    pBlk->pBlkNext  = NULL;
}

//------------------------------------------------------------------------------
// Inserts elements in beginning of list.
NNS_GFD_INLINE void InsertBlock_
(
    NNSiGfdLnkVramBlock**   pListHead,
    NNSiGfdLnkVramBlock*    prev
)
{
    NNS_GFD_NULL_ASSERT( prev );
    
    if( (*pListHead) != NULL )
    {
        (*pListHead)->pBlkPrev    = prev;
    }
    
    prev->pBlkNext          = *pListHead;
    prev->pBlkPrev          = NULL;
    *pListHead = prev;
    
}

//------------------------------------------------------------------------------
// Removes elements from list.
NNS_GFD_INLINE void RemoveBlock_
(
    NNSiGfdLnkVramBlock**  pListHead,
    NNSiGfdLnkVramBlock*   pBlk
)
{
    NNS_GFD_NULL_ASSERT( pBlk );
    {
    
        NNSiGfdLnkVramBlock *const pPrev = pBlk->pBlkPrev;
        NNSiGfdLnkVramBlock *const pNext = pBlk->pBlkNext;

        // previous reference link
        if ( pPrev )
        {
            pPrev->pBlkNext = pNext;
        }else{
            *pListHead = pNext;
        }
        

        // next reference link
        if ( pNext )
        {
            pNext->pBlkPrev = pPrev;
        }
    }
}

//------------------------------------------------------------------------------
// Obtains new block.
NNS_GFD_INLINE NNSiGfdLnkVramBlock* 
GetNewBlock_( NNSiGfdLnkVramBlock**   ppBlockPoolList )
{
    NNS_GFD_NULL_ASSERT( ppBlockPoolList );
    {
        // Removes from beginning of list.
        NNSiGfdLnkVramBlock*    pNew = *ppBlockPoolList;
        if( pNew )
        {
            *ppBlockPoolList = pNew->pBlkNext;
        }
        
        return pNew;
    }
}   


//------------------------------------------------------------------------------
// Obtains the end address of the block.
NNS_GFD_INLINE u32 GetBlockEndAddr_( NNSiGfdLnkVramBlock* pBlk )
{
    NNS_GFD_NULL_ASSERT( pBlk );
    
    return (u32)(pBlk->addr + pBlk->szByte);
}
/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdDumpLnkVramManFreeListInfo

  Description:  Outputs free block information for debugging. 
                
  Arguments:    pFreeListHead          : Head of free block information list
                szReserved             : Size of reserved area
               
  Returns:      None
  
 *---------------------------------------------------------------------------*/
void NNSi_GfdDumpLnkVramManFreeListInfo
( 
    const NNSiGfdLnkVramBlock*      pFreeListHead,
    u32                             szReserved 
)
{
    
    u32                         szFreeTotal = 0; 
    const NNSiGfdLnkVramBlock*  pBlk        = pFreeListHead;
    
    // Display all free list information 
    while( pBlk )
    {
    
        if( pBlk->szByte != 0 )
        {
            OS_Printf("0x%08x:  0x%08x    \n", pBlk->addr, pBlk->szByte );   
            szFreeTotal    += pBlk->szByte;
        }
        
        pBlk          = pBlk->pBlkNext;
    }
    
    // If there is no free list, display dummy line
    if( szFreeTotal == 0 )
    {
        OS_Printf("0x--------:  0x--------    \n");
    }
    
    // Display utilization
    {
        const u32 szUsedTotal = (szReserved - szFreeTotal);
        OS_Printf("    %08d / %08d bytes (%6.2f%%) used \n", 
            szUsedTotal, szReserved, 100.f *  szUsedTotal / szReserved );   
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdInitLnkVramMan

  Description:  This function initializes NNSiGfdLnkVramMan.
                
  Arguments:    pMgr          : VRAM Manager
                
               
  Returns:      None
  
 *---------------------------------------------------------------------------*/
void 
NNSi_GfdInitLnkVramMan( NNSiGfdLnkVramMan* pMgr )
{
    NNS_GFD_NULL_ASSERT( pMgr );
    pMgr->pFreeList = NULL;
}


/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdInitLnkVramBlockPool

  Description:  This function initializes the management block pool.
                Number of management information elements ==  Number of memory segments that can be managed
                
  Arguments:    pArrayHead          : beginning of management information array
                lengthOfArray   : number of management information elements
               
  Returns:      beginning of list
  
 *---------------------------------------------------------------------------*/
NNSiGfdLnkVramBlock* 
NNSi_GfdInitLnkVramBlockPool
( 
    NNSiGfdLnkVramBlock*    pArrayHead, 
    u32                     lengthOfArray 
)
{
    NNS_GFD_NULL_ASSERT( pArrayHead );
    NNS_GFD_NON_ZERO_ASSERT( lengthOfArray );
    
    //
    // connected by list
    //
    {
        int i;
        for( i = 0; i < lengthOfArray - 1; i++ )
        {
            pArrayHead[i].pBlkNext      = &pArrayHead[i+1];
            pArrayHead[i+1].pBlkPrev    = &pArrayHead[i];
        }
        
        pArrayHead[0].pBlkPrev                  = NULL;  
        pArrayHead[lengthOfArray - 1].pBlkNext  = NULL;    
    }
    
    // Returns the beginning of list
    return &pArrayHead[0];
}

/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdInitLnkVramMan

  Description:  This function adds the blocks of newly freed areas to the manager.
                
                
  Arguments:    pMan                : Manager
                ppBlockPoolList     : common block management information
                baseAddr            : management area base address
                szByte: memory size set aside
               
  Returns:      None
  
 *---------------------------------------------------------------------------*/
BOOL NNSi_GfdAddNewFreeBlock
(
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32                     baseAddr,
    u32                     szByte
)
{
    NNS_GFD_NULL_ASSERT( pMan );
    NNS_GFD_NULL_ASSERT( ppBlockPoolList );
    NNS_GFD_NON_ZERO_ASSERT( szByte );
    
    // Creates free blocks.
    {
        NNSiGfdLnkVramBlock*        pNew  = GetNewBlock_( ppBlockPoolList );
        if( pNew )
        {
            InitBlockFromPrams_( pNew, baseAddr, szByte );
            InsertBlock_( &pMan->pFreeList, pNew );
            
            return TRUE;
        }else{
            return FALSE;
        }
    }
}



/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdAllocLnkVram

  Description:  This function sets aside memory.
                
                If there is sufficient memory remaining, but the management
                blocks are insufficient, memory allocation may fail. 
                
  Arguments:    pMan                : Manager
                ppBlockPoolList     : common block management information
                pRetAddr            : pointer to address set aside
                szByte: memory size set aside
                
  Returns:      memory set aside success or failure
  
 *---------------------------------------------------------------------------*/
BOOL
NNSi_GfdAllocLnkVram
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32*                    pRetAddr,
    u32                     szByte
)
{
    return NNSi_GfdAllocLnkVramAligned( pMan, ppBlockPoolList, pRetAddr, szByte, 0 );
}

//------------------------------------------------------------------------------
// memory set aside with alignment that can be specified
// Open areas that occurred during alignment are newly registered in the free list.
// Take note that if a management information block does not exist for the open area, the memory allocation will fail.
BOOL
NNSi_GfdAllocLnkVramAligned
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32*                    pRetAddr,
    u32                     szByte,
    u32                     alignment
)
{
    NNS_GFD_NULL_ASSERT( pMan );
    NNS_GFD_NULL_ASSERT( pRetAddr );
    
    NNS_GFD_NON_ZERO_ASSERT( szByte );
    {
        //
        // Searches for blocks from the free list that meet the requirements.
        //
        u32     alignedAddr;
        u32     szNeeded;
        u32     difference;
        
        NNSiGfdLnkVramBlock* pBlkFound  = NULL;
        NNSiGfdLnkVramBlock* pBlk       = pMan->pFreeList;
        
        
        while( pBlk )
        {
            //
            // If necessary, calculates the address rounded up to the align boundary.
            //
            if( alignment > 1 )
            {
                alignedAddr = (u32)(  (pBlk->addr + (alignment - 1)) & ~(alignment - 1) );
                // Increased by rounded up portion only, which is the actual size needed.
                difference  = ( alignedAddr - pBlk->addr );
                szNeeded    = szByte + difference;
            }else{
                alignedAddr = pBlk->addr;
                difference  = 0;
                szNeeded    = szByte;
            }
            
            
            // Does the size satisfy the request?
            if( pBlk->szByte >= szNeeded )
            {
                pBlkFound = pBlk;
                break;
            }
            pBlk = pBlk->pBlkNext;
        }
        
        //
        // If a block in requirements was found,
        //
        if ( pBlkFound ) 
        {
            // The area that was moved as alignment is registered as a free block.
            if( difference > 0 )
            {    
                NNSiGfdLnkVramBlock*        pNewFreeBlk = GetNewBlock_( ppBlockPoolList );
                if( pNewFreeBlk )
                {
                    // registration
                    InitBlockFromPrams_( pNewFreeBlk, pBlkFound->addr, difference );
                    InsertBlock_( &pMan->pFreeList, pNewFreeBlk );
                }else{
                    // Allocation failed
                    goto NG_CASE;
                }
            }
            
            // Updates information of free block found.
            {
                // Subtracts used area.
                pBlkFound->szByte   -= szNeeded;
                // Allocates memory from front of block.
                pBlkFound->addr     += szNeeded; 
                
                // perfect size
                if( pBlkFound->szByte == 0 )
                {
                    // Removes from free list
                    RemoveBlock_( &pMan->pFreeList, pBlkFound );
                    InsertBlock_( ppBlockPoolList, pBlkFound );
                }
            }
                    
            *pRetAddr = alignedAddr;
            return TRUE;
        }
        
NG_CASE:            
        //
        // Could not find a block that meets the requirements.
        //
        *pRetAddr = 0;
        return FALSE;
        
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNSi_GfdFreeLnkVram

  Description:  This function carries out the freeing of memory.
                Take note that if the management blocks are insufficient the
                freeing may fail (failure happens with generation of new free blocks).
                
                
  Arguments:    pMan                : Manager
                ppBlockPoolList     : common block management information
                addr                : address freed
                szByte: memory size freed
               
  Returns:      memory free success or failure
  
 *---------------------------------------------------------------------------*/
BOOL NNSi_GfdFreeLnkVram
( 
    NNSiGfdLnkVramMan*      pMan, 
    NNSiGfdLnkVramBlock**   ppBlockPoolList,
    u32                     addr,
    u32                     szByte
)
{
    NNS_GFD_NULL_ASSERT( pMan );
    NNS_GFD_NON_ZERO_ASSERT( szByte );
    
    {
        NNSiGfdLnkMemRegion     region;
        
        region.start    = addr;
        region.end      = addr + szByte;
        
        //------------------------------------------------------------------------------
        // Inserts the empty region in the free memory block.
        //      If it is next to the free block it expands the free block.
        //      If there is no adjacent block, or the block is not large enough
        //      to make free, the alignment value of the used block adjacent at the end is used.
        //      If there is no used block next to the end of the free block the function fails.
        {
            const NNSiGfdLnkMemRegion*      pRegion = &region;
            
            NNS_GFD_NULL_ASSERT( pMan );
            NNS_GFD_NULL_ASSERT( ppBlockPoolList );
            NNS_GFD_NULL_ASSERT( pRegion );
            {
                NNSiGfdLnkMemRegion         freeRgn     = *pRegion;

                // Searches for free area that is next to the specified one.
                {
                    NNSiGfdLnkVramBlock*        pCursor         = pMan->pFreeList;
                    NNSiGfdLnkVramBlock*        pNext           = NULL;
                    
                    
                    // About all of the free list elements
                    while( pCursor )
                    {
                        pNext = pCursor->pBlkNext;
                          
                        // Is it a block that is next to the end?
                        if ( pCursor->addr == pRegion->end )   
                        {
                            // Combines the empty region
                            freeRgn.end = GetBlockEndAddr_( pCursor );
                            // Removes from the list and returns to Pool.
                            RemoveBlock_( &pMan->pFreeList, pCursor );
                            InsertBlock_( ppBlockPoolList, pCursor );
                        }
                                        
                        // Is it a block that is next to the beginning?
                        if( GetBlockEndAddr_( pCursor ) == pRegion->start )
                        {
                            // Combines the empty region
                            freeRgn.start  = pCursor->addr;
                            // Removes from the list and returns to Pool.
                            RemoveBlock_( &pMan->pFreeList, pCursor );
                            InsertBlock_( ppBlockPoolList, pCursor );
                        }
                        
                        pCursor = pNext;
                    }
                }

                
                //
                // Registers new free blocks.
                //
                {
                    NNSiGfdLnkVramBlock*        pNewFreeBlk = GetNewBlock_( ppBlockPoolList );
                    if( pNewFreeBlk == NULL )
                    {
                        // The data for the management area is insufficient.
                        // Failed to free
                        return FALSE;
                    }else{
                    
                        InitBlockFromRegion_( pNewFreeBlk, &freeRgn );
                        InsertBlock_( &pMan->pFreeList, pNewFreeBlk );
                    }
                }
                
                return TRUE;
            }
        }
    }
}

