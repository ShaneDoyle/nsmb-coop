#include <nnsys/gfd/gfd_common.h>
#include <nnsys/gfd/VramManager/gfd_LinkedListTexVramMan.h>
#include "gfdi_LinkedListVramMan_Common.h"

#define GFD_SLOT_SIZE        0x20000
#define GFD_SLOT0_BASEADDR   0x00000
#define GFD_SLOT1_BASEADDR   0x20000
#define GFD_SLOT2_BASEADDR   0x40000 
#define GFD_SLOT3_BASEADDR   0x60000 

#define NNS_GFD_LNK_FREE_ERROR_INVALID_SIZE 2

//
// Manager
//
typedef struct NNS_GfdLnkTexVramManager
{
    NNSiGfdLnkVramMan       mgrNrm;
    NNSiGfdLnkVramMan       mgr4x4;
    
    NNSiGfdLnkVramBlock*    pBlockPoolList;
    
    //
    // Member to use when reset
    //
    u32                     szByte;
    u32                     szByteFor4x4;
    NNSiGfdLnkVramBlock*    pWorkHead;
    u32                     szByteManagementWork;

}NNS_GfdLnkTexVramManager;


typedef struct SlotData
{
    u32     szFree; // empty region size
    u32     szNrm;  // size for standard texture
    u32     sz4x4;  // size for 4x4 compression texture

}SlotData;


static NNS_GfdLnkTexVramManager         mgr_;

//------------------------------------------------------------------------------
// debug function (This function is described only in the assert section. This function is limited to internal release.))
//------------------------------------------------------------------------------
static u32 Dbg_GetVramManTotalFreeBlockSize_( const NNSiGfdLnkVramMan* pMgr )
{
    u32   total = 0;
    const NNSiGfdLnkVramBlock* pBlk = pMgr->pFreeList;
    while( pBlk )
    {
        total   += pBlk->szByte;
        pBlk    = pBlk->pBlkNext;
    }
    return total;
}

//------------------------------------------------------------------------------
// Is the initialization size parameter valid
static BOOL Dbg_IsInitializeSizeParamsValid_( u32 szByte, u32 szByteFor4x4 )
{
    //
    // Is the size correct?
    //
    if( szByte > 0 && szByteFor4x4 <= GFD_SLOT_SIZE * 2 )
    {   
        //
        // When there is a size designation for 4x4
        //
        if( szByteFor4x4 > 0 )
        {   
            // When the size is 0x20000 or lower
            if( szByteFor4x4 <= GFD_SLOT_SIZE )
            {
                // It is mandatory that the index table section is of allocatable size
                return (BOOL)(szByte >= GFD_SLOT1_BASEADDR + szByteFor4x4 / 2);
            }else{
                // It is mandatory that the index table section is of allocatable size
                // GFD_SLOT_SIZE is the size of Slot 1 to be used as the index table
                return (BOOL)( szByte >= szByteFor4x4 + GFD_SLOT_SIZE );
            }
        }else{
            // Does this exceed the maximum size limits?
            return (BOOL)( szByte <= GFD_SLOT_SIZE * 4 );
        }
    }else{
        return FALSE;
    }
}

//------------------------------------------------------------------------------
// 
NNS_GFD_INLINE BOOL InitSlotFreeBlock_
( 
    NNSiGfdLnkVramMan*      pMgr, 
    NNSiGfdLnkVramBlock**   pPoolList, 
    u32                     baseAddr, 
    u32                     size 
)
{
    NNS_GFD_NULL_ASSERT( pMgr );
    NNS_GFD_NULL_ASSERT( pPoolList );

    NNS_GFD_ASSERT( GFD_SLOT_SIZE >= size );
    
    {
        BOOL result = TRUE;
        
        if( size > 0 )
        {
            result &= NNSi_GfdAddNewFreeBlock( pMgr, pPoolList, baseAddr, size );
        }
        return result;
    }
}

//------------------------------------------------------------------------------
// Manager internal status is output for debugging
void NNS_GfdDumpLnkTexVramManager()
{
    OS_Printf("=== NNS_Gfd LnkTexVramManager Dump ====\n");
    OS_Printf("   address:        size    \n");   // Header line
    OS_Printf("=======================================\n");
    //
    // Display the entire free list of ordinary textures and compute the total usage amount. 
    //
    OS_Printf("------ Normal Texture Free Blocks -----\n");   
    NNSi_GfdDumpLnkVramManFreeListInfo( mgr_.mgrNrm.pFreeList, mgr_.szByte );
        
        
    //
    // Display the entire free list of 4x4 textures and compute the total usage amount. 
    //
    OS_Printf("------ 4x4    Texture Free Blocks -----\n");   
    if( mgr_.szByteFor4x4 != 0 )
    {
        NNSi_GfdDumpLnkVramManFreeListInfo( mgr_.mgr4x4.pFreeList, mgr_.szByteFor4x4 );
    }
    OS_Printf("=======================================\n");   
}

//------------------------------------------------------------------------------
u32 NNS_GfdGetLnkTexVramManagerWorkSize( u32 numMemBlk )
{
    return numMemBlk * sizeof( NNSiGfdLnkVramBlock );
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdInitLnkTexVramManager

  Description:  Frees the texture region in VRAM from the texture key.
                
  Arguments:    szByte                   The size in bytes of the VRAM region to manage
                                        (1 Slot = 0x20000, calculated as a maximum of 4 Slots)
                szByteFor4x4             The size in bytes of the region used in the 4x4 compression texture in the management region
                                        (1 Slot = 0x20000, calculated as a maximum of 2 Slots)
                pManagementWork         Pointer to the memory region used as management information 
                szByteManagementWork    Size of the management information region 
                useAsDefault             Whether or not to use the linked list texture VRAM manager
                                        as the current manager 

                
               
  Returns:      None
  
 *---------------------------------------------------------------------------*/
void NNS_GfdInitLnkTexVramManager
( 
    u32     szByte, 
    u32     szByteFor4x4,
    void*   pManagementWork, 
    u32     szByteManagementWork,
    BOOL    useAsDefault
)
{
    NNS_GFD_ASSERT( Dbg_IsInitializeSizeParamsValid_( szByte, szByteFor4x4 ) );
    
    NNS_GFD_NULL_ASSERT( pManagementWork );
    NNS_GFD_ASSERT( szByteManagementWork != 0 );
    
    {
        mgr_.szByte         = szByte;                                                         
        mgr_.szByteFor4x4   = szByteFor4x4;
        mgr_.pWorkHead      = pManagementWork;
        mgr_.szByteManagementWork = szByteManagementWork;
        
        
        NNS_GfdResetLnkTexVramState();
        
        //
        // Check to see if the total volume of the free block generated with the initialization process is correct
        //
        NNS_GFD_ASSERT( mgr_.szByte - ( mgr_.szByteFor4x4 + mgr_.szByteFor4x4 / 2 ) 
            == Dbg_GetVramManTotalFreeBlockSize_( &mgr_.mgrNrm ) );
        NNS_GFD_ASSERT( mgr_.szByteFor4x4 
            == Dbg_GetVramManTotalFreeBlockSize_( &mgr_.mgr4x4 ) );

        
        //
        // Use as a default allocator
        //
        if( useAsDefault )
        {
            NNS_GfdDefaultFuncAllocTexVram = NNS_GfdAllocLnkTexVram;
            NNS_GfdDefaultFuncFreeTexVram  = NNS_GfdFreeLnkTexVram;
        }
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdAllocLnkTexVram

  Description:  Allocates the texture region from VRAM
                
  Arguments:    szByte: The number of region bytes to allocate
                is4x4comp: 4x4 compressed texture?
                opt: options ( not used )
                
               
  Returns:      Texture
                If allocation fails, returns NNS_GFD_ALLOC_ERROR_TEXKEY, the key that shows the error

  
 *---------------------------------------------------------------------------*/
NNSGfdTexKey    NNS_GfdAllocLnkTexVram( u32 szByte, BOOL is4x4comp, u32 opt )
{
#pragma unused(opt)
    u32     addr;
    BOOL    result;
    
    {
        //
        // If a size is allocated that is too small to be displayed with a texture key, this rounds the size up.
        //
        szByte = NNSi_GfdGetTexKeyRoundupSize( szByte );
        //
        // If a size is allocated that is too large for the texture key to display, this returns an error key.
        //
        if( szByte >= NNS_GFD_TEXSIZE_MAX )
        {
            NNS_GFD_WARNING("Allocation size is too big. : NNS_GfdAllocLnkTexVram()");
            return NNS_GFD_ALLOC_ERROR_TEXKEY;
        }
        
        NNS_GFD_MINMAX_ASSERT( szByte, NNS_GFD_TEXSIZE_MIN, NNS_GFD_TEXSIZE_MAX );
    }
    
    
    if( is4x4comp )
    {
        result = NNSi_GfdAllocLnkVram( &mgr_.mgr4x4, &mgr_.pBlockPoolList, &addr, szByte );
    }else{
        result = NNSi_GfdAllocLnkVram( &mgr_.mgrNrm, &mgr_.pBlockPoolList, &addr, szByte );
    }
    
        
    if( result )
    {
        return NNS_GfdMakeTexKey( addr, szByte, is4x4comp );    
    }else{
        NNS_GFD_WARNING("failure in Vram Allocation. : NNS_GfdAllocLnkTexVram()");
        return NNS_GFD_ALLOC_ERROR_TEXKEY;
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdFreeLnkTexVram

  Description:  Frees the texture region from VRAM


                
                
  Arguments:    memKey : Texture

                
  Returns:      None

 *---------------------------------------------------------------------------*/
int             NNS_GfdFreeLnkTexVram( NNSGfdTexKey memKey )
{
    BOOL        result;
    const u32   addr     = NNS_GfdGetTexKeyAddr( memKey );
    const u32   szByte   = NNS_GfdGetTexKeySize( memKey );
    const BOOL  b4x4     = NNS_GfdGetTexKey4x4Flag( memKey );
    
    if( szByte != 0 )
    { 
        if( b4x4 )
        {
            result = NNSi_GfdFreeLnkVram( &mgr_.mgr4x4, &mgr_.pBlockPoolList, addr, szByte );
        }else{
            result = NNSi_GfdFreeLnkVram( &mgr_.mgrNrm, &mgr_.pBlockPoolList, addr, szByte );
        }
        
        if( result ) 
        {
            return 0;
        }else{
            return 1;
        }
    }else{
        return NNS_GFD_LNK_FREE_ERROR_INVALID_SIZE;    
    }
}






/*---------------------------------------------------------------------------*
  Name:         NNS_GfdGetLnkPlttVramManagerWorkSize

  Description:  Restores the memory allocation state for the texture of the linked list texture VRAM manager to is initial state.
                 
      
  Arguments:    None

                
  Returns:      None

 *---------------------------------------------------------------------------*/
void            NNS_GfdResetLnkTexVramState( void )
{
    SlotData        sd[4] = 
    { 
        // empty region size, size for the standard texture, size for the 4x4 compression texture
        { 0x20000, 0, 0 },
        { 0x20000, 0, 0 },
        { 0x20000, 0, 0 },
        { 0x20000, 0, 0 }
    };
    
    // the region size for the index table to be allocated to slot 1
    // (however, the manager will not be managed as a free block)
    const u32   szIndexTbl  = mgr_.szByteFor4x4 / 2; 
    // standard texture   size
    u32         restNrm     = mgr_.szByte - ( mgr_.szByteFor4x4 + szIndexTbl );
    // 4x4 compression  texture  size
    u32         rest4x4     = mgr_.szByteFor4x4;
    u32         slotNo;
    u32         val;
    
    
    //------------------------------------------------------------------------------
    // Calculate the space used for 4x4, then store the result in SlotData
    //
    for( slotNo = 0; slotNo < 4; slotNo++ )
    {
        // The region for 4x4 will only be allocated in slot 0 or 2
        if( slotNo == 0 || slotNo == 2 )
        {
            if( sd[slotNo].szFree > 0 && rest4x4 > 0 )
            {
                if( sd[slotNo].szFree > rest4x4 )
                {
                    val = rest4x4;
                }else{
                    val = sd[slotNo].szFree;
                }
                
                sd[slotNo].sz4x4     += val;
                sd[slotNo].szFree    -= val;
                rest4x4              -= val;
            }
        }
    }
    
    //
    // slot  1 : Calculate the size of the index table region, and reduce from the free region
    //
    {
        sd[1].szFree    -= szIndexTbl;
        // the manager will not manage the region for the index table
    }
    
    //
    // Calculate the size of the standard texture region from the remaining regions
    //
    for( slotNo = 0; slotNo < 4; slotNo++ )
    {
        if( sd[slotNo].szFree > 0 && restNrm > 0 )
        {
            if( sd[slotNo].szFree > restNrm )
            {
                val = restNrm;
            }else{
                val = sd[slotNo].szFree;
            }
            
            sd[slotNo].szNrm     += val;
            sd[slotNo].szFree    -= val;
            restNrm              -= val;
        }
    }
    
    //------------------------------------------------------------------------------
    //
    // initialization process (initializes the free block based on the computed SlotData))
    //
    {
        BOOL result = TRUE;    
        
        NNSi_GfdInitLnkVramMan( &mgr_.mgrNrm );
        NNSi_GfdInitLnkVramMan( &mgr_.mgr4x4 );
        
        
        //
        // Initialize the shared management block
        //
        mgr_.pBlockPoolList 
            = NNSi_GfdInitLnkVramBlockPool( (NNSiGfdLnkVramBlock*)mgr_.pWorkHead, 
                                            mgr_.szByteManagementWork / sizeof( NNSiGfdLnkVramBlock ) );
        
        {
            //
            // Create a 4x4 in the lower side, and a normal free block immediately above it, then register them in the manager
            //
            // slot 0
            result &= 
            InitSlotFreeBlock_( &mgr_.mgr4x4, &mgr_.pBlockPoolList , GFD_SLOT0_BASEADDR              , sd[0].sz4x4 );
            result &= 
            InitSlotFreeBlock_( &mgr_.mgrNrm, &mgr_.pBlockPoolList , GFD_SLOT0_BASEADDR + sd[0].sz4x4, sd[0].szNrm );
            // slot 2
            result &= 
            InitSlotFreeBlock_( &mgr_.mgr4x4, &mgr_.pBlockPoolList , GFD_SLOT2_BASEADDR              , sd[2].sz4x4 );
            result &= 
            InitSlotFreeBlock_( &mgr_.mgrNrm, &mgr_.pBlockPoolList , GFD_SLOT2_BASEADDR + sd[2].sz4x4, sd[2].szNrm );
            
            // slot 3
            NNS_GFD_ASSERT( sd[3].sz4x4 == 0 ); 
            result &= 
            InitSlotFreeBlock_( &mgr_.mgrNrm, &mgr_.pBlockPoolList , GFD_SLOT3_BASEADDR              , sd[3].szNrm );
            
            // slot 1
            // The region for the palette index is not managed with the manager.
            // Initialize the regions other than the regions for the palette index as free blocks for the standard texture.
            result &= 
            InitSlotFreeBlock_( &mgr_.mgrNrm, &mgr_.pBlockPoolList , GFD_SLOT1_BASEADDR + szIndexTbl, sd[1].szNrm );
        }
        NNS_GFD_ASSERT( result );    
    }
}


