/*H********************************************************************************/
/*!
    \File netconncommon.cpp

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
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "netconncommon.h"

#include "IEAUser/IEAUser.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static NetConnCommonRefT  *_NetConn_pRef = NULL;          //!< module state pointer

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonEnqueueIEAUserEvent

    \Description
        Add the specified entry at the head of the specified list.

    \Input *pUserEventEntry - event entry to be enqueued
    \Input **pList          - list to add the event to

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnCommonEnqueueIEAUserEvent(NetConnIEAUserEventT *pUserEventEntry, NetConnIEAUserEventT **pList)
{
    if (*pList != NULL)
    {
        pUserEventEntry->pNext = *pList;
    }

    *pList = pUserEventEntry;
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonDequeueIEAUserEvent

    \Description
        Get an event entry from the tail fo the specified list.

    \Input **pList              - list to dequeue from

    \Output
        NetConnIEAUserEventT *  - pointer to free IEAUserEvent entry

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static NetConnIEAUserEventT * _NetConnCommonDequeueIEAUserEvent(NetConnIEAUserEventT **pList)
{
    NetConnIEAUserEventT *pUserEventEntry = NULL;

    // find tail
    if (*pList != NULL)
    {
        NetConnIEAUserEventT *pPrevious = NULL;;
        for (pUserEventEntry = *pList; pUserEventEntry->pNext != NULL; pUserEventEntry = pUserEventEntry->pNext)
        {
            pPrevious = pUserEventEntry;
        }

        if (pPrevious)
        {
            pPrevious->pNext = NULL;
        }
        else
        {
           *pList = NULL;
        }
    }

    return(pUserEventEntry);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonGetFreeIEAUserEvent

    \Description
        Get a free IEAUserEvent entry from the free list.

    \Input *pCommonRef          - netconn module state

    \Output
        NetConnIEAUserEventT *  - pointer to free IEAUserEvent entry

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static NetConnIEAUserEventT * _NetConnCommonGetFreeIEAUserEvent(NetConnCommonRefT *pCommonRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    // check if free list is empty
    if (pCommonRef->pIEAUserFreeEventList == NULL)
    {
        int32_t iLoop = 0;

        // add 4 entries to the free list
        for (iLoop = 0; iLoop < 4; iLoop++)
        {
            pUserEventEntry = (NetConnIEAUserEventT *) DirtyMemAlloc(sizeof(NetConnIEAUserEventT), NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);

            if (pUserEventEntry)
            {
                ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));

                _NetConnCommonEnqueueIEAUserEvent(pUserEventEntry, &pCommonRef->pIEAUserFreeEventList);

                NetPrintfVerbose((pCommonRef->iDebugLevel, 0, "netconncommon: [%p] allocated a new free user event entry\n", pUserEventEntry));
            }
            else
            {
                NetPrintf(("netconncommon: failed to allocate a new user event entry\n"));
            }
        }
    }

    pUserEventEntry = _NetConnCommonDequeueIEAUserEvent(&pCommonRef->pIEAUserFreeEventList);

    return(pUserEventEntry);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonAddIEAUserEvent

    \Description
        Add an entry to the list of IEAUserEvents

    \Input *pCommonRef      - netconn module state
    \Input iLocalUserIndex  - local user index
    \Input *pIEAUser        - pointer to IEAUser object
    \Input eEvent           - event type

    \Output
        int32_t             - 0 for success; negative for error

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnCommonAddIEAUserEvent(NetConnCommonRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser, NetConnIEAUserEventTypeE eEvent)
{
    int32_t iResult = 0;

    NetCritEnter(&pCommonRef->crit);

    NetConnIEAUserEventT *pUserEventEntry = _NetConnCommonGetFreeIEAUserEvent(pCommonRef);

    if (pUserEventEntry)
    {
        pUserEventEntry->eEvent = eEvent;
        pUserEventEntry->iLocalUserIndex = iLocalUserIndex;
        pUserEventEntry->pIEAUser = pIEAUser;
        pUserEventEntry->pIEAUser->AddRef();

        _NetConnCommonEnqueueIEAUserEvent(pUserEventEntry, &pCommonRef->pIEAUserEventList);

        NetPrintfVerbose((pCommonRef->iDebugLevel, 0, "netconncommon: [%p] IEAUser event queued (local user index = %d, local user id = 0x%08x, event = %s)\n",
            pUserEventEntry, iLocalUserIndex, (int32_t)pIEAUser->GetUserID(), (eEvent==NETCONN_EVENT_IEAUSER_ADDED?"added":"removed")));
    }
    else
    {
        iResult = -1;
    }

    NetCritLeave(&pCommonRef->crit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonClearIEAUserEventList

    \Description
        Clear list of pending IEAUser events.

    \Input *pCommonRef  - netconn module state

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnCommonClearIEAUserEventList(NetConnCommonRefT *pCommonRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    NetCritEnter(&pCommonRef->crit);

    while ((pUserEventEntry = _NetConnCommonDequeueIEAUserEvent(&pCommonRef->pIEAUserEventList)) != NULL)
    {
        // return event entry to free list
        pUserEventEntry->pIEAUser->Release();
        ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));
        _NetConnCommonEnqueueIEAUserEvent(pUserEventEntry, &pCommonRef->pIEAUserFreeEventList);
    }

    NetCritLeave(&pCommonRef->crit);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCommonClearIEAUserFreeEventList

    \Description
        Clear list of free IEAUser events.

    \Input *pCommonRef  - netconn module state

    \Version 05/09/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnCommonClearIEAUserFreeEventList(NetConnCommonRefT *pCommonRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    NetCritEnter(&pCommonRef->crit);

    while ((pUserEventEntry = _NetConnCommonDequeueIEAUserEvent(&pCommonRef->pIEAUserFreeEventList)) != NULL)
    {
        DirtyMemFree(pUserEventEntry, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);

        NetPrintfVerbose((pCommonRef->iDebugLevel, 0, "netconncommon: [%p] freed user event entry\n", pUserEventEntry));
    }

    NetCritLeave(&pCommonRef->crit);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   NetConnCommonGetRef

    \Description
        Return current module reference.

    \Output
        NetConnCommonRefT * - reference pointer, or NULL if the module is not active

    \Version 05/21/2014 (mclouatre)
*/
/********************************************************************************F*/
NetConnCommonRefT *NetConnCommonGetRef(void)
{
    // return pointer to module state
    return(_NetConn_pRef);
}

/*F********************************************************************************/
/*!
    \Function    NetConnCommonStartup

    \Description
        Start up common functionality.

    \Input iNetConnRefSize  - size of netconn ref to allocate
    \Input *pAddUserCb      - added user event handler
    \Input *pRemoveUserCb   - removed user event handler

    \Output
        NetConnCommonRefT * - netconn ref if successful; else NULL

    \Version 05/21/2014 (mclouatre)
*/
/********************************************************************************F*/
NetConnCommonRefT *NetConnCommonStartup(int32_t iNetConnRefSize, NetConnAddLocalUserCallbackT *pAddUserCb, NetConnRemoveLocalUserCallbackT *pRemoveUserCb)
{
    NetConnCommonRefT *pCommonRef = _NetConn_pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pCommonRef != NULL)
    {
        NetPrintf(("netconncommon: NetConnCommonStartup() called while module is already active\n"));
        return(NULL);
    }

    // alloc and init ref
    if ((pCommonRef = (NetConnCommonRefT *)DirtyMemAlloc(iNetConnRefSize, NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconncommon: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pCommonRef, iNetConnRefSize);
    pCommonRef->iMemGroup = iMemGroup;
    pCommonRef->pMemGroupUserData = pMemGroupUserData;
    pCommonRef->pAddUserCb = pAddUserCb;
    pCommonRef->pRemoveUserCb = pRemoveUserCb;

    NetCritInit(&pCommonRef->crit, "netconnCrit");

    // save ref
    _NetConn_pRef = pCommonRef;

    return(pCommonRef);
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
    _NetConnCommonClearIEAUserEventList(pCommonRef);
    _NetConnCommonClearIEAUserFreeEventList(pCommonRef);

    NetCritKill(&pCommonRef->crit);

    // dispose of ref
    DirtyMemFree(pCommonRef, NETCONN_MEMID, pCommonRef->iMemGroup, pCommonRef->pMemGroupUserData);
    _NetConn_pRef = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnCommonAddLocalUser

    \Description
        Use this function to tell netconn about a newly detected local user on the local console.

    \Input iLocalUserIndex  - index at which DirtySDK needs to insert this user it its internal user array
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnCommonAddLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    NetConnCommonRefT *pCommonRef = _NetConn_pRef;
    int32_t iRetCode = 0;   // default to success

    if (iLocalUserIndex >= 0 && iLocalUserIndex < NETCONN_MAXLOCALUSERS)
    {
        iRetCode = _NetConnCommonAddIEAUserEvent(pCommonRef, iLocalUserIndex, pLocalUser, NETCONN_EVENT_IEAUSER_ADDED);
    }
    else
    {
        NetPrintf(("netconncommon: NetConnCommonAddLocalUser() called with an invalid index (%d)\n", iLocalUserIndex));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnCommonRemoveLocalUser

    \Description
        Use this function to tell netconn about a local user that no longer exists

    \Input iLocalUserIndex  - index in the internal DirtySDK user array at which the user needs to be cleared
                              pass -1 when the index is unknown and a lookup will be done
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnCommonRemoveLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    NetConnCommonRefT *pCommonRef = _NetConn_pRef;
    int32_t iRetCode = 0;   // default to success

    if (iLocalUserIndex == -1)
    {
        NetCritEnter(&pCommonRef->crit);
        iLocalUserIndex = NetConnStatus('iusr', (int32_t)pLocalUser->GetUserID(), NULL, 0);
        NetCritLeave(&pCommonRef->crit);
    }

    if (iLocalUserIndex >= 0 && iLocalUserIndex < NETCONN_MAXLOCALUSERS)
    {
        iRetCode = _NetConnCommonAddIEAUserEvent(pCommonRef, iLocalUserIndex, pLocalUser, NETCONN_EVENT_IEAUSER_REMOVED);
    }
    else
    {
        NetPrintf(("netconncommon: NetConnCommonRemoveLocalUser() called with an invalid index (%d)\n", iLocalUserIndex));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function   NetConnCommonUpdateLocalUsers

    \Description
        Update our internally maintained array of NetConnUsers from the array
        of IEAUsers

    \Input *pCommonRef  - NetConnRefT reference

    \Version 01/18/2014 (mclouatre)
*/
/********************************************************************************F*/
void NetConnCommonUpdateLocalUsers(NetConnCommonRefT *pCommonRef)
{
    NetConnIEAUserEventT *pUserEventEntry;

    NetCritEnter(&pCommonRef->crit);

    // process events
    while ((pUserEventEntry = _NetConnCommonDequeueIEAUserEvent(&pCommonRef->pIEAUserEventList)) != NULL)
    {
        if (pUserEventEntry->eEvent == NETCONN_EVENT_IEAUSER_ADDED)
        {
            pCommonRef->pAddUserCb(pCommonRef, pUserEventEntry->iLocalUserIndex, pUserEventEntry->pIEAUser);
        }
        else
        {
            pCommonRef->pRemoveUserCb(pCommonRef, pUserEventEntry->iLocalUserIndex, pUserEventEntry->pIEAUser);
        }

        NetPrintfVerbose((pCommonRef->iDebugLevel, 0, "netconncommon: [%p] IEAUser event dequeued (local user index = %d, local user id = 0x%08x, event = %s)\n",
            pUserEventEntry, pUserEventEntry->iLocalUserIndex, (int32_t)pUserEventEntry->pIEAUser->GetUserID(), (pUserEventEntry->eEvent==NETCONN_EVENT_IEAUSER_ADDED?"added":"removed")));

        // return event entry to free list
        pUserEventEntry->pIEAUser->Release();
        ds_memclr(pUserEventEntry, sizeof(*pUserEventEntry));
        _NetConnCommonEnqueueIEAUserEvent(pUserEventEntry, &pCommonRef->pIEAUserFreeEventList);
    }

    NetCritLeave(&pCommonRef->crit);
}
