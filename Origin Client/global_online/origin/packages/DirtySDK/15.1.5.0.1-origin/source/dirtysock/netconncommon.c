/*H********************************************************************************/
/*!
    \File netconncommon.c

    \Description
        Cross-platform netconn data types and functions.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 05/21/2014 (mclouatre)  First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "netconncommon.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    NetConnCommonStartup

    \Description
        Start up common functionality.

    \Input iNetConnRefSize  - size of netconn ref to allocate
    \Input *pParams         - startup parameters
    \Input *pRef            - (out) netconn ref if successful; else NULL

    \Output
        int32_t - 0 for success; negative for error

    \Version 05/21/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetConnCommonStartup(int32_t iNetConnRefSize, const char *pParams, NetConnCommonRefT **pRef)
{
    NetConnCommonRefT *pCommonRef = *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // refcount if already started
    if (pCommonRef != NULL)
    {
        ++(pCommonRef->iRefCount);
        NetPrintf(("netconncommon: NetConnStartup() called with params='%s' while the module is already started, refcounting to %d\n", pParams, pCommonRef->iRefCount));
        return(NETCONN_ERROR_ALREADY_STARTED);
    }

    // alloc and init ref
    if ((pCommonRef = (NetConnCommonRefT *)DirtyMemAlloc(iNetConnRefSize, NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconncommon: unable to allocate module state\n"));
        return(NETCONN_ERROR_NO_MEMORY);
    }
    ds_memclr(pCommonRef, iNetConnRefSize);
    pCommonRef->iMemGroup = iMemGroup;
    pCommonRef->pMemGroupUserData = pMemGroupUserData;
    pCommonRef->iRefCount = 1;

    // allocate external cleanup list
    pCommonRef->iExternalCleanupListMax = NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY;
    if ((pCommonRef->pExternalCleanupList = (NetConnExternalCleanupEntryT *)DirtyMemAlloc(pCommonRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData)) == NULL)
    {
        DirtyMemFree(pCommonRef, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);
        NetPrintf(("netconncommmon: unable to allocate memory for initial external cleanup list\n"));
        (*pRef) = NULL;
        return(NETCONN_ERROR_NO_MEMORY);
    }
    ds_memclr(pCommonRef->pExternalCleanupList, pCommonRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

    NetCritInit(&pCommonRef->crit, "netconnCrit");

    // save ref
    (*pRef) = pCommonRef;

    // successful completion
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnCommonShutdown

    \Description
        Shutdown common functionality.

    \Input *pCommonRef - common module state

    \Version 05/21/2014 (mclouatre)
*/
/********************************************************************************F*/
void NetConnCommonShutdown(NetConnCommonRefT *pCommonRef)
{
    NetCritKill(&pCommonRef->crit);

    // free the cleanup list memory
    if (pCommonRef->pExternalCleanupList != NULL)
    {
        DirtyMemFree(pCommonRef->pExternalCleanupList, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);
        pCommonRef->pExternalCleanupList = NULL;
    }

    // dispose of ref
    DirtyMemFree(pCommonRef, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function NetConnCommonAddToExternalCleanupList

    \Description
        Add an entry to the list of external module pending successful cleanup.

    \Input *pCommonRef      - NetConnCommonRefT reference
    \Input *pCleanupCb      - cleanup callback
    \Input *pCleanupData    - data to be passed to cleanup callback

    \Output
        uint32_t            -  0 for success; -1 for failure.

    \Version 12/07/2009 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetConnCommonAddToExternalCleanupList(NetConnCommonRefT *pCommonRef, NetConnExternalCleanupCallbackT pCleanupCb, void *pCleanupData)
{
    // if list if full, double its size.
    if (pCommonRef->iExternalCleanupListCnt == pCommonRef->iExternalCleanupListMax)
    {
        NetConnExternalCleanupEntryT *pNewList;

        // allocate new external cleanup list
        if ((pNewList = (NetConnExternalCleanupEntryT *) DirtyMemAlloc(2 * pCommonRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("netconncommon: unable to allocate memory for the external cleanup list\n"));
            return(-1);
        }
        ds_memclr(pNewList, 2 * pCommonRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // copy contents of old list in contents of new list
        ds_memcpy(pNewList, pCommonRef->pExternalCleanupList, pCommonRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // free old list and replace it with new list
        DirtyMemFree(pCommonRef->pExternalCleanupList, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);
        pCommonRef->pExternalCleanupList = pNewList;
        pCommonRef->iExternalCleanupListMax = pCommonRef->iExternalCleanupListMax * 2;
    }

    // add new entry to the list
    pCommonRef->pExternalCleanupList[pCommonRef->iExternalCleanupListCnt].pCleanupCb = pCleanupCb;
    pCommonRef->pExternalCleanupList[pCommonRef->iExternalCleanupListCnt].pCleanupData = pCleanupData;
    pCommonRef->iExternalCleanupListCnt += 1;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnCommonProcessExternalCleanupList

    \Description
        Walk external cleanup list and try to destroy each individual entry.

    \Input *pCommonRef  - NetConnCommonRefT reference

    \Output
        int32_t         - number of valid entries in the cleanup list, negative integer upon failure

    \Version 03/13/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetConnCommonProcessExternalCleanupList(NetConnCommonRefT *pCommonRef)
{
    int32_t iEntryIndex;
    int32_t iEntryIndex2;

    // early exit if cleanup list not yet allocated
    if (pCommonRef->pExternalCleanupList == NULL)
    {
        return(-1);
    }

    // make sure we don't re-enter this code; some modules may call NetConnStatus() during their cleanup
    if (pCommonRef->bProcessingCleanupList)
    {
        return(-2);
    }

    pCommonRef->bProcessingCleanupList = TRUE;

    for (iEntryIndex = 0; iEntryIndex < pCommonRef->iExternalCleanupListMax; iEntryIndex++)
    {
        if (pCommonRef->pExternalCleanupList[iEntryIndex].pCleanupCb == NULL)
        {
            // no more entry in list
            break;
        }

        if(pCommonRef->pExternalCleanupList[iEntryIndex].pCleanupCb(pCommonRef->pExternalCleanupList[iEntryIndex].pCleanupData) == 0)
        {
            pCommonRef->iExternalCleanupListCnt -= 1;

            NetPrintf(("netconncommon: successfully destroyed external module (cleanup data ptr = %p), external cleanup list count decremented to %d\n",
                pCommonRef->pExternalCleanupList[iEntryIndex].pCleanupData, pCommonRef->iExternalCleanupListCnt));

            // move all following entries one cell backward in the array
            for(iEntryIndex2 = iEntryIndex; iEntryIndex2 < pCommonRef->iExternalCleanupListMax; iEntryIndex2++)
            {
                if (iEntryIndex2 == pCommonRef->iExternalCleanupListMax-1)
                {
                    // last entry, reset to NULL
                    pCommonRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = NULL;
                    pCommonRef->pExternalCleanupList[iEntryIndex2].pCleanupData = NULL;
                }
                else
                {
                    pCommonRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = pCommonRef->pExternalCleanupList[iEntryIndex2+1].pCleanupCb;
                    pCommonRef->pExternalCleanupList[iEntryIndex2].pCleanupData = pCommonRef->pExternalCleanupList[iEntryIndex2+1].pCleanupData;
                }
            }
        }
    }

    // clear re-enter block
    pCommonRef->bProcessingCleanupList = FALSE;

    return(pCommonRef->iExternalCleanupListCnt);
}

/*F********************************************************************************/
/*!
    \Function NetConnCommonCheckRef

    \Description
        Decrement and verify the reference count.

    \Input *pCommonRef  - NetConnCommonRefT reference

    \Output
        int32_t         - 0 if ready to shutdown, NETCONN_ERROR_ISACTIVE otherwise

    \Version 10/23/2017 (amakoukji)
*/
/********************************************************************************F*/
int32_t NetConnCommonCheckRef(NetConnCommonRefT *pCommonRef)
{
    // check the refcount
    if (--(pCommonRef->iRefCount) > 0)
    {
        NetPrintf(("netconncommon: NetConnShutdown() called, new refcount is %d\n", pCommonRef->iRefCount));
        return(NETCONN_ERROR_ISACTIVE);
    }

    return(0);
}
