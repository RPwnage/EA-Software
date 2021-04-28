/*H********************************************************************************/
/*!
    \File dirtymem.c

    \Description
        DirtySock memory allocation routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/12/2005 (jbrookes) First Version
    \Version 11/19/2008 (mclouatre) Adding pMemGroupUserData to mem groups
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "dirtysock.h"
#include "dirtymem.h"

/*** Defines **********************************************************************/

#define DIRTYMEM_MAXGROUPS  (16)

/*** Type Definitions *************************************************************/

typedef struct DirtyMemInfoT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
} DirtyMemInfoT;


/*** Variables ********************************************************************/

static int32_t _DirtyMem_iGroup = 0;
static DirtyMemInfoT _DirtyMem_iGroupStack[DIRTYMEM_MAXGROUPS] = { {'dflt', NULL} };

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyMemGroupEnter

    \Description
        Set group that will be used for allocations.

    \Input iMemGroup            - group id
    \Input *pMemGroupUserData   - User-provided data pointer
    
    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/19/2008 (mclouatre) Adding pMemGroupUserData to mem groups
*/
/********************************************************************************F*/
void DirtyMemGroupEnter(int32_t iMemGroup, void *pMemGroupUserData)
{
    if (_DirtyMem_iGroup >= DIRTYMEM_MAXGROUPS)
    {
        NetPrintf(("dirtymem: group stack overflow\n"));
        return;
    }
    _DirtyMem_iGroup += 1;   
    _DirtyMem_iGroupStack[_DirtyMem_iGroup].iMemGroup = iMemGroup;
    _DirtyMem_iGroupStack[_DirtyMem_iGroup].pMemGroupUserData = pMemGroupUserData;
}

/*F********************************************************************************/
/*!
    \Function DirtyMemGroupLeave

    \Description
        Restore previous group that will be used for allocations.

    \Version 10/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyMemGroupLeave(void)
{
    if (_DirtyMem_iGroup <= 0)
    {
        NetPrintf(("dirtymem: group stack underflow\n"));
        return;
    }
    _DirtyMem_iGroup -= 1;
}

/*F********************************************************************************/
/*!
    \Function DirtyMemGroupQuery

    \Description
        Return current memory group data.

    \Input *pMemGroup           - [OUT param] pointer to variable to be filled with mem group id
    \Input **ppMemGroupUserData - [OUT param] pointer to variable to be filled with pointer to user data
    
    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) returned values now passed in [OUT] parameters
*/
/********************************************************************************F*/
void DirtyMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData)
{
    if (pMemGroup != NULL)
    {
        *pMemGroup = _DirtyMem_iGroupStack[_DirtyMem_iGroup].iMemGroup;
    }
    if (ppMemGroupUserData != NULL)
    {
        *ppMemGroupUserData = _DirtyMem_iGroupStack[_DirtyMem_iGroup].pMemGroupUserData;
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyMemDebugAlloc

    \Description
        Display memory allocation information to debug output.

    \Input *pMem                - address of memory being freed
    \Input iSize                - size of allocation
    \Input iMemModule           - memory module
    \Input iMemGroup            - memory group
    \Input *pMemGroupUserData   - pointer to user data
    
    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) adding pMemGroupUserData parameter
*/
/********************************************************************************F*/
#if DIRTYCODE_DEBUG
void DirtyMemDebugAlloc(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    NetPrintf(("dirtymem: [a] 0x%08x mod=%c%c%c%c grp=%c%c%c%c udataptr=0x%08x size=%d\n", (uintptr_t)pMem,
        (iMemModule>>24)&0xff, (iMemModule>>16)&0xff, (iMemModule>>8)&0xff, iMemModule&0xff,
        (iMemGroup>>24)&0xff, (iMemGroup>>16)&0xff, (iMemGroup>>8)&0xff, iMemGroup&0xff,
        pMemGroupUserData, iSize));
}
#endif

/*F********************************************************************************/
/*!
    \Function DirtyMemDebugFree

    \Description
        Display memory free information to debug output.

    \Input *pMem                - address of memory being freed
    \Input iSize                - size of allocation (if available), or zero
    \Input iMemModule           - memory module
    \Input iMemGroup            - memory group
    \Input *pMemGroupUserData   - pointer to user data
    
    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) adding pMemGroupUserData parameter
*/
/********************************************************************************F*/
#if DIRTYCODE_DEBUG
void DirtyMemDebugFree(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    NetPrintf(("dirtymem: [f] 0x%08x mod=%c%c%c%c grp=%c%c%c%c udataptr=0x%08x", (uintptr_t)pMem,
        (iMemModule>>24)&0xff, (iMemModule>>16)&0xff, (iMemModule>>8)&0xff, iMemModule&0xff,
        (iMemGroup>>24)&0xff, (iMemGroup>>16)&0xff, (iMemGroup>>8)&0xff, iMemGroup&0xff,
        pMemGroupUserData));
    if (iSize > 0)
    {
        NetPrintf((" size=%d\n", iSize));
    }
    else
    {
        NetPrintf((" size=0x%08x\n", iSize));
    }
}
#endif
