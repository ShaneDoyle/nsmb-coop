#include <nnsys/gfd/gfd_common.h>
#include <nnsys/gfd/VramManager/gfd_LinkedListPlttVramMan.h>
#include "gfdi_LinkedListVramMan_Common.h"

#define GFD_SLOT_SIZE        0x18000

#define NNS_GFD_BARPLTT_FREE_ERROR_INVALID_SIZE 1

//
// Manager
//
typedef struct NNS_GfdLnkPlttVramManager
{
    NNSiGfdLnkVramMan       mgr;
    NNSiGfdLnkVramBlock*    pBlockPoolList;
    
    //
    // Member to use when reset
    //
    u32                     szByte;
    NNSiGfdLnkVramBlock*    pWorkHead;
    u32                     szByteManagementWork;

}NNS_GfdLnkPlttVramManager;

static NNS_GfdLnkPlttVramManager         mgr_;


/*---------------------------------------------------------------------------*
  Name:         NNS_GfdDumpLnkPlttVramManager

  Description:  Manager internal status is output for debugging


                
                
  Arguments:    None

                
  Returns:      None

 *---------------------------------------------------------------------------*/
void NNS_GfdDumpLnkPlttVramManager()
{
    OS_Printf("=== NNS_Gfd LnkPlttVramManager Dump ===\n");
    OS_Printf("   address:        size    \n");   // Header line
    OS_Printf("=======================================\n");
    //
    // Display the entire free list of ordinary textures and compute the total usage amount. 
    //
    OS_Printf("------ Free Blocks                -----\n");   
    
    NNSi_GfdDumpLnkVramManFreeListInfo( mgr_.mgr.pFreeList, mgr_.szByte );
    
    OS_Printf("=======================================\n");   
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdGetLnkPlttVramManagerWorkSize

  Description:  Obtains the byte size of the memory required by VRAM manager for management information.
                Use the return value of this function to initialize the management region
                that is the manager initialization parameter. 


                
                
  Arguments:    numMemBlk   Number of blocks to be allocated. It becomes the maximum number of segmentation for managing empty regions. 

                
  Returns:      Byte size required for managing regions.

 *---------------------------------------------------------------------------*/
u32 NNS_GfdGetLnkPlttVramManagerWorkSize( u32 numMemBlk )
{
    return numMemBlk * sizeof( NNSiGfdLnkVramBlock );
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdInitLnkPlttVramManager

  Description:  Initializes the frame palette VRAM manager.
                Initializes the frame palette VRAM manager so that the region
                that starts from the beginning of the palette RAM with size that is specified by szByte is managed. 
                
                
  Arguments:    szByte                  Byte size of the region to manage (Maximum 0x18000)
                pManagementWork         Pointer to the memory region used as management information 
                szByteManagementWork    Size of the management information region 
                useAsDefault            Whether to use the linked list palette VRAM manager as a current manager. 

                
  Returns:      None

 *---------------------------------------------------------------------------*/
void NNS_GfdInitLnkPlttVramManager
( 
    u32     szByte,
    void*   pManagementWork, 
    u32     szByteManagementWork,
    BOOL    useAsDefault
)
{
    NNS_GFD_ASSERT( szByte <= GFD_SLOT_SIZE );
    NNS_GFD_NULL_ASSERT( pManagementWork );
    NNS_GFD_ASSERT( szByteManagementWork != 0 );
    
    {
        //
        mgr_.szByte                 = szByte;
        mgr_.pWorkHead              = pManagementWork;
        mgr_.szByteManagementWork   = szByteManagementWork;
        
        NNS_GfdResetLnkPlttVramState();
        
        //
        // Use as a default allocator
        //
        if( useAsDefault )
        {
            NNS_GfdDefaultFuncAllocPlttVram = NNS_GfdAllocLnkPlttVram;
            NNS_GfdDefaultFuncFreePlttVram  = NNS_GfdFreeLnkPlttVram;
        }
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdAllocLnkPlttVram

  Description:  Allocates memory for palette in the linked list palette VRAM manager.
                
  Arguments:    szByte:     Byte size of the region you want to allocate
                b4Pltt:     Whether it is a 4-color palette. TRUE if it is. 
                opt :       Option. This is no longer used. 
                
  Returns:      Returns a palette key that shows the allocated VRAM region.
                Returns NNS_GFD_ALLOC_ERROR_PLTTKEY, a key that shows an error,
                if the allocation failed.

 *---------------------------------------------------------------------------*/
NNSGfdPlttKey    NNS_GfdAllocLnkPlttVram( u32 szByte, BOOL b4Pltt, u32 opt )
{
#pragma unused(opt)
    u32     addr;
    BOOL    result;
    
    {
        //
        // If a size is allocated that is too small to be displayed with a palette key, this rounds the size up.
        //
        szByte = NNSi_GfdGetPlttKeyRoundupSize( szByte );
        //
        // If a size is allocated that is too large for the palette key to display, this returns an error key.
        //
        if( szByte >= NNS_GFD_PLTTSIZE_MAX )
        {
            NNS_GFD_WARNING("Allocation size is too big. : NNS_GfdAllocLnkPlttVram()");
            return NNS_GFD_ALLOC_ERROR_PLTTKEY;
        }
        
        NNS_GFD_MINMAX_ASSERT( szByte, NNS_GFD_PLTTSIZE_MIN, NNS_GFD_PLTTSIZE_MAX );
    }
    
    //
    // Sets the alignment in the palette format
    //
    if( b4Pltt )
    {
        result = NNSi_GfdAllocLnkVramAligned( &mgr_.mgr, 
                                              &mgr_.pBlockPoolList, 
                                              &addr, 
                                              szByte, 
                                              0x08 );
        // Checks whether the region cannot be referenced
        if( addr + szByte > NNS_GFD_4PLTT_MAX_ADDR )
        {
            // NG
            if( !NNSi_GfdFreeLnkVram( &mgr_.mgr, 
                                      &mgr_.pBlockPoolList, 
                                      addr, 
                                      szByte ) )
            {
                // Displays warning
            }
                                 
            return NNS_GFD_ALLOC_ERROR_PLTTKEY;
        }
    }else{
        result = NNSi_GfdAllocLnkVramAligned( &mgr_.mgr, 
                                              &mgr_.pBlockPoolList, 
                                              &addr, 
                                              szByte, 
                                              0x10 );
    }
    
    if( result )
    {
        return NNS_GfdMakePlttKey( addr, szByte );    
    }else{
        NNS_GFD_WARNING("failure in Vram Allocation. : NNS_GfdAllocLnkPlttVram()");
        return NNS_GFD_ALLOC_ERROR_PLTTKEY;
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdFreeLnkPlttVram

  Description:  Allocates the palette memory that was allocated
                in the linked list palette VRAM manager.
                
  Arguments:    plttKey:    The palette key representing the VRAM region to be freed.
                
  Returns:      Returns if the release succeeded or failed.
                If the release was successful, 0 is returned.
                If palette key showing an invalid size, such as 0, was released, 
                1 is returned. 

 *---------------------------------------------------------------------------*/
int             NNS_GfdFreeLnkPlttVram( NNSGfdPlttKey plttKey )
{
    
    const u32   addr     = NNS_GfdGetPlttKeyAddr( plttKey );
    const u32   szByte   = NNS_GfdGetPlttKeySize( plttKey );
    
    const BOOL  result   = NNSi_GfdFreeLnkVram( &mgr_.mgr, 
                                                &mgr_.pBlockPoolList, 
                                                addr, 
                                                szByte );
    
    if( result ) 
    {
        return 0;
    }else{
        return NNS_GFD_BARPLTT_FREE_ERROR_INVALID_SIZE;
    }
}

/*---------------------------------------------------------------------------*
  Name:         NNS_GfdResetLnkPlttVramState

  Description:  Resets the manager to the initial state.
                
  Arguments:    None
                
  Returns:      None
  
 *---------------------------------------------------------------------------*/
void            NNS_GfdResetLnkPlttVramState( void )
{
    
    //
    // Initialize the shared management block
    //
    mgr_.pBlockPoolList 
        = NNSi_GfdInitLnkVramBlockPool( 
            (NNSiGfdLnkVramBlock*)mgr_.pWorkHead, 
            mgr_.szByteManagementWork / sizeof( NNSiGfdLnkVramBlock ) );
    
    //
    // Initialize free list
    //
    {
        BOOL    result;
        NNSi_GfdInitLnkVramMan( &mgr_.mgr );
        
        result = NNSi_GfdAddNewFreeBlock( &mgr_.mgr, 
                                          &mgr_.pBlockPoolList,
                                          0,
                                          mgr_.szByte );
        NNS_GFD_ASSERT( result );        
    }
}


