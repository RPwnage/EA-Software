/*H********************************************************************************/
/*!
    \File friendapipriv.c

    \Description
        This file contains private code comon to all platforms

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 09/05/2012 (akirchner)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/friend/friendapi.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "friendapipriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function FriendApiPrivNotifyEvent

    \Description
        Invokes the different callbacks registered by higher-level modules.

    \Input pFriendApiRef    - module reference
    \Input pEventData       - pointer to event data to be passed to the callbacks

    \Version 06/15/2010 (mclouatre)
    \Version 09/06/2012 (akirchner) - ignore FRIENDAPI_EVENT_FRIEND_UPDATED, FRIENDAPI_EVENT_FRIEND_REMOVED and FRIENDAPI_EVENT_BLOCKLIST_UPDATED events if bInitFinished is false
*/
/********************************************************************************F*/
void FriendApiPrivNotifyEvent(FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData)
{
    FriendApiCommonRefT *pCommon = (FriendApiCommonRefT *)pFriendApiRef;

    int32_t iIndex;
    int8_t bExecuteCallback = FALSE;

    NetCritEnter(&pCommon->notifyCrit);

    for(iIndex = 0; iIndex < FRIENDAPI_MAX_CALLBACKS; iIndex++)
    {
        switch (pEventData->eventType)
        {
            case FRIENDAPI_EVENT_UNKNOWN:
                bExecuteCallback = TRUE;
                break;

            case FRIENDAPI_EVENT_FRIEND_UPDATED:
                bExecuteCallback = pCommon->callback[iIndex].bInitFinished[pEventData->uUserIndex];
                break;

            case FRIENDAPI_EVENT_FRIEND_REMOVED:
                bExecuteCallback = pCommon->callback[iIndex].bInitFinished[pEventData->uUserIndex];
                break;

            case FRIENDAPI_EVENT_BLOCKLIST_UPDATED:
                bExecuteCallback = pCommon->callback[iIndex].bInitFinished[pEventData->uUserIndex];
                break;

            case FRIENDAPI_EVENT_BLOCK_USER_RESULT:
                bExecuteCallback = TRUE;
                break;

            case FRIENDAPI_EVENT_MSG_RCVED:
                bExecuteCallback = TRUE;
                break;

            case FRIENDAPI_EVENT_INIT_COMPLETE:
                bExecuteCallback = TRUE;
                break;

            case FRIENDAPI_EVENT_MAX:
                NetPrintf(("friendapipriv: invalid event of type %d\n", pEventData->eventType));
                break;
        }

        if (pCommon->callback[iIndex].pNotifyCb)
        {
            if (bExecuteCallback)
            {
                pCommon->callback[iIndex].pNotifyCb(pFriendApiRef, pEventData, pCommon->callback[iIndex].pNotifyCbUserData);
            }
            else
            {
                NetPrintf(("friendapipriv: unable to call callback of index %d for event of type %d\n", iIndex, pEventData->eventType));
            }
        }
    }

    NetCritLeave(&pCommon->notifyCrit);
}
