/*H*************************************************************************************/
/*!
    \File dirtywebapips4.c

    \Description
        DirtyWebApiPS4 encapsulates use of the Sce's SessionInvitation API for 
        creating, modifying, inviting and joining.

    \Copyright
        Copyright (c) / Electronic Arts 20013.  ALL RIGHTS RESERVED.

    \Version 1.0 05/09/2013 (mcorcoran)  First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/
#include <sdk_version.h>
#include <string.h>
#include <scetypes.h>
#include <net.h>
#include <libhttp.h>
#include <np.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/ps4/dirtycontextmanagerps4.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/jsonparse.h"

#include "DirtySDK/dirtysock/ps4/dirtywebapips4.h"

/*** Defines ***************************************************************************/
#define DIRTY_WEBAPI_NET_HEAP_NAME    "DirtyWebApi"

//! default memory sizes
#define DIRTY_WEBAPI_NET_HEAP_SIZE    (16 * 1024)
#define DIRTY_WEBAPI_SSL_HEAP_SIZE    (384 * 1024)
#define DIRTY_WEBAPI_HTTP_HEAP_SIZE   (48 * 1024)
#define DIRTY_WEBAPI_WEBAPI_HEAP_SIZE (32 * 1024)
#define DIRTY_WEBAPI_READ_BUFFER_SIZE (2 * 1024)

//! default number of push events
#define DIRTY_WEBAPI_MAX_PUSH_EVENTS  (16)

//! maximum number of defered deletes
#define DIRTY_WEBAPI_MAX_DEFERED_DELETES  (100)
#define DIRTY_WEBAPI_MAX_DEFERED_DELETE_TIMEOUT (5000)

/*** Macros ****************************************************************************/


/*** Type Definitions ******************************************************************/
typedef struct DirtyWebApiWebRequestT
{
    struct DirtyWebApiWebRequestT *pNext;
    struct DirtyWebApiWebRequestT *pPrev;
    DirtyWebApiCallbackT *pCallback;             //!< the callback that is called when a response is received or an error occurs
    void *pUserData;
    int64_t iWebRequestId;
    int32_t iContentLength;
    int32_t iUserIndex;
    char pContent[1];
} DirtyWebApiWebRequestT;

typedef struct DirtyWebApiUserT
{
    SceUserServiceUserId    aSceUserId;         //!< simple id, similar to an int
    int32_t                 iWebapiUserCtxId;   //!< context for this user to use to execute webapi commands
} DirtyWebApiUserT;

//! registration for a single event (for all users)
typedef struct DirtyWebApPushEventT
{
    //standard fields
    int32_t aCallBackIds[NETCONN_MAXLOCALUSERS];        //!< stores a per user id for each event registered
    void *pUserData;                                    //!< user pointer which they receive back when the callback fires
    DirtyWebApiPushEventCallbackT *pPushEventCallback;  //!< callback that will fire each time the event happens
    const char *pSceEventDataType;                      //!< description of what type of event we're listening for (for generating filter), defined by sony
    int32_t iFilterId;                                  //!< id of generated event filter
    
    //for "Service" PushEvents Only
    const char *pNpServiceName;                         //!< further description on what type of event we're listening for
    SceNpServiceLabel npServiceLabel;                   //!< further description on what type of event we're listening for
    uint8_t bServicePushEvent;                          //!< indication that the "service" api is required for this event
    
   struct DirtyWebApPushEventT *pNext;                        //!< Next DirtyWebPushEvent this is use in the Adding Push Events Async
} DirtyWebApPushEventT;

//! a list of event registrations
typedef struct DirtyWebApPushEventsT
{
    DirtyWebApPushEventT aEvents[DIRTY_WEBAPI_MAX_PUSH_EVENTS]; //!< list of events we're listening for
    int32_t iEventCount;                                        //!< current number of events we are listening for
    int32_t iHandleId;                                          //!< webapi handle required for "service" api
} DirtyWebApPushEventsT;

//! a list of dirtywebapi callback to be executed
typedef struct DirtyWebApiCallbackEntryT
{
  struct DirtyWebApiCallbackEntryT *pNext;
  DirtyWebApiCallbackT *pCallback;
  int32_t iSceError;
  int32_t iUserIndex;
  int32_t iStatusCode;
  const char *pRespBody;
  int32_t iRespBodyLength;
  void *pUserData;

} DirtyWebApiCallbackEntryT;

typedef struct DirtyWebApiDereferedDeleteT
{
    int64_t iWebRequestId;
    uint32_t uTimeout;
    uint8_t bActive;
    uint8_t _pad[3];
} DirtyWebApiDereferedDeleteT;

//! internal module state
struct DirtyWebApiRefT
{
    void                *pMemGroupUserData;           //!< user data associated with mem group
    int32_t             iMemGroup;                    //!< module mem group id

    int32_t             iNetPoolCtxId;                //!< context for sceNetPool*
    int32_t             iSslCtxId;                    //!< context for sceSsl*
    int32_t             iHttpCtxId;                   //!< context for sceHttp*
    int32_t             iWebApiCtxId;                 //!< context for sceNpWebApi*
    struct DirtyWebApiWebRequestT * pCurrentWebRequest;     //!< web request in action
    
    ScePthread          WebApiThread;                 //!< thread to do blocking web api calls from (todo can we find a way to not use a thread for webapi calls)
    volatile int32_t    iThreadLife;                  //!< used in condition to let thread self terminate >=0 alive, < 0 terminate
    NetCritT            crit;                         //!< sychronize shared data between the threads
    NetCritT            abortCrit;                    //!< sychronize shared data between the threads

    char               *aReadBuffer;                  //!< buffer for data from WebApi
    uint32_t            uReadBufferSize;              //!< buffer size

    uint8_t             bQueueCallback;               //!< queue request callback

    struct DirtyWebApiWebRequestT *pRequestListFront; //!< the front of the FIFO web request queue - this request is the one that gets serviced
    struct DirtyWebApiWebRequestT *pRequestListBack;  //!< the back of the FIFO web request queue - additional requests are added to the back

    struct DirtyWebApiCallbackEntryT *pCallbackListFront;  //!< the front of the FIFO web request callback queue
    struct DirtyWebApiCallbackEntryT *pCallbackListBack;   //!< the back of the FIFO web request callback queue

    DirtyWebApiUserT    WebApiUsers[NETCONN_MAXLOCALUSERS]; //!< state for each user, they must each manipulate the session api
    DirtyWebApPushEventT *PendingAddPushEvents;          //!<List of events that needs to be registered async
    DirtyWebApPushEventsT PushEvents;                 //!< List of events the user has registered for

    ScePthreadCond dirtyWepApiThreadCond;             //!< condition variable to wakeup webapi thread
    ScePthreadMutex dirtyWebApiThreadMutex;           //!< mutex associated with pDirtyWepApiThreadCond

    DirtyWebApiDereferedDeleteT DeferredDeleteList[DIRTY_WEBAPI_MAX_DEFERED_DELETES]; //!< list of requests which could not be deleted
};

/*** Private Functions *****************************************************************/

static int32_t _DirtyWebApiSetupWebUserContext(DirtyWebApiRefT *pRef, int32_t iUserIndex);
static int32_t _DirtyWebApiCleanupRequestQueue(DirtyWebApiRefT *pRef, int32_t iUserIndex);
static void _DirtyWebApiCleanupWebUserContext(DirtyWebApiRefT *pRef, int32_t iUserIndex);

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiCallbackNpWebApiPushEvent

    \Description
        Intermediate callback, to call the users callback.

    \Input userCtxId        - user web api context
    \Input callbackId       - callback id generated at registration
    \Input pTo              - contains the onlineId of target user
    \Input pFrom            - contains the onlineId of source user
    \Input pDataType        - event type
    \Input pData            - any data coming with the event
    \Input dataLen          - length of data with the event
    \Input pUserArg         - pointer to the DirtyWebApPushEventT that generated this event

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
static void _DirtyWebApiCallbackNpWebApiPushEvent(int32_t userCtxId, int32_t callbackId, const SceNpPeerAddress *pTo, const SceNpPeerAddress *pFrom, const SceNpWebApiPushEventDataType *pDataType, const char *pData, size_t dataLen, void *pUserArg)
{
    DirtyWebApPushEventT *pEvent = (DirtyWebApPushEventT*)pUserArg;

    pEvent->pPushEventCallback(userCtxId, callbackId, NULL, SCE_NP_DEFAULT_SERVICE_LABEL, pTo, pFrom, pDataType, pData, dataLen, pEvent->pUserData);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiCallbackNpWebApiServicePushEvent

    \Description
        Intermediate callback, to call the users callback.

    \Input userCtxId        - user web api context
    \Input callbackId       - callback id generated at registration
    \Input pNpServiceName   - NP web service generating the event
    \Input npServiceLabel   - more detail on the web service
    \Input pTo              - contains the onlineId of target user
    \Input pFrom            - contains the onlineId of source user
    \Input pDataType        - event type
    \Input pData            - any data coming with the event
    \Input dataLen          - length of data with the event
    \Input pUserArg         - pointer to the DirtyWebApPushEventT that generated this event

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
static void _DirtyWebApiCallbackNpWebApiServicePushEvent(int32_t userCtxId, int32_t callbackId, const char *pNpServiceName, SceNpServiceLabel npServiceLabel, const SceNpPeerAddress *pTo, const SceNpPeerAddress *pFrom, const SceNpWebApiPushEventDataType *pDataType, const char *pData, size_t dataLen, void *pUserArg)
{
    DirtyWebApPushEventT *pEvent = (DirtyWebApPushEventT*)pUserArg;
    pEvent->pPushEventCallback(userCtxId, callbackId, pNpServiceName, npServiceLabel, pTo, pFrom, pDataType, pData, dataLen, pEvent->pUserData);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiInitPushEvents

    \Description
        Set the default state of the push event structures.

    \Input pRef             - module state

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
static void _DirtyWebApiInitPushEvents(DirtyWebApiRefT *pRef)
{
    if ((pRef->PushEvents.iHandleId = sceNpWebApiCreateHandle(pRef->iWebApiCtxId)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiInitPushEvents, sceNpWebApiCreateHandle() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->PushEvents.iHandleId)));
    }

    for (int32_t iEvent = 0; iEvent < DIRTY_WEBAPI_MAX_PUSH_EVENTS; iEvent++)
    {
        pRef->PushEvents.aEvents[iEvent].iFilterId = -1;

        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUser] = -1;
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiDestroyPushEvents

    \Description
        Unregister any active events and free resources

    \Input pRef             - module state

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
static void _DirtyWebApiDestroyPushEvents(DirtyWebApiRefT *pRef)
{
    int32_t iRet; 
    for (int32_t iEvent = 0; iEvent < pRef->PushEvents.iEventCount; iEvent++)
    {
        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            if (pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUser] >= 0)
            {
                if (pRef->PushEvents.aEvents[iEvent].bServicePushEvent == TRUE)
                {
                    if ((iRet = sceNpWebApiUnregisterServicePushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUser])) != SCE_OK)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiDestroyPushEvents, sceNpWebApiUnregisterServicePushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
                    }
                }
                else
                {
                    if ((iRet = sceNpWebApiUnregisterPushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUser])) != SCE_OK)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiDestroyPushEvents, sceNpWebApiUnregisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
                    }
                }
            }
            pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUser] = -1;
        }

        if (pRef->PushEvents.aEvents[iEvent].bServicePushEvent == TRUE)
        {
            if ((iRet = sceNpWebApiDeleteServicePushEventFilter(pRef->iWebApiCtxId, pRef->PushEvents.aEvents[iEvent].iFilterId)) != SCE_OK)
            {
                NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiDestroyPushEvents, sceNpWebApiDeleteServicePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
            }
        }
        else
        {
            if ((iRet = sceNpWebApiDeletePushEventFilter(pRef->iWebApiCtxId, pRef->PushEvents.aEvents[iEvent].iFilterId)) != SCE_OK)
            {
                NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiDestroyPushEvents, sceNpWebApiDeletePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
            }
        }
        pRef->PushEvents.aEvents[iEvent].iFilterId = -1;
    }

    if (pRef->PushEvents.iHandleId > 0)
    {
        if ((iRet = sceNpWebApiDeleteHandle(pRef->iWebApiCtxId, pRef->PushEvents.iHandleId)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiDestroyPushEvents, sceNpWebApiDeleteHandle() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
        pRef->PushEvents.iHandleId = -1;
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiAddPushEventAsync

    \Description
        This was add due to a performance problem due to blocking nature ogf first party call

    \Input pRef             - module state
    \Input eventType        - event type from sony
    \Input pNpServiceName   - NP web service generating the event
    \Input npServiceLabel   - more detail on the web service
    \Input callback         - function to call when the event triggers
    \Input pUserData        - data passed to the callback when it triggers

    \Version 10/09/2013 (tcho)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiAddPushEventAsync(DirtyWebApiRefT *pRef, const char * eventType, const char * pNpServiceName, SceNpServiceLabel npServiceLabel, DirtyWebApiPushEventCallbackT *callback, void *pUserData)
{
    DirtyWebApPushEventT *pPushEvent = (DirtyWebApPushEventT *)DirtyMemAlloc(sizeof(DirtyWebApPushEventT), DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    memset(pPushEvent, 0, sizeof(DirtyWebApPushEventT));

    pPushEvent->pNpServiceName = pNpServiceName;
    pPushEvent->npServiceLabel = npServiceLabel;
    pPushEvent->pPushEventCallback = callback;
    pPushEvent->pUserData = pUserData;
    pPushEvent->pSceEventDataType = eventType;

    //Adding Push Events to the Pending List
    scePthreadMutexLock(&pRef->dirtyWebApiThreadMutex);
    NetCritEnter(&pRef->crit);

    pPushEvent->pNext = pRef->PendingAddPushEvents;
    pRef->PendingAddPushEvents = pPushEvent;
    
    NetCritLeave(&pRef->crit);

    // signal webapi thread to do work 
    if (scePthreadCondSignal(&pRef->dirtyWepApiThreadCond) != SCE_OK)
    {
        NetPrintf(("dirtywebapips4: [%p] failed to signal webapi thread.\n", pRef));
    }
    scePthreadMutexUnlock(&pRef->dirtyWebApiThreadMutex);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiAddPushEvent

    \Description
        Adds a registeration for Push events

    \Input pRef             - module state
    \Input eventType        - event type from sony
    \Input pNpServiceName   - NP web service generating the event
    \Input npServiceLabel   - more detail on the web service
    \Input callback         - function to call when the event triggers
    \Input pUserData        - data passed to the callback when it triggers

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiAddPushEvent(DirtyWebApiRefT *pRef, const char * eventType, const char * pNpServiceName, SceNpServiceLabel npServiceLabel, DirtyWebApiPushEventCallbackT *callback, void *pUserData)
{
    int32_t iFilterId;

    if (pRef->PushEvents.iEventCount < DIRTY_WEBAPI_MAX_PUSH_EVENTS)
    {
        //add the new eventType to a filter
        SceNpWebApiPushEventDataType dataType;
        memset(&dataType, 0, sizeof(dataType));
        ds_snzprintf(dataType.val, SCE_NP_WEBAPI_PUSH_EVENT_DATA_TYPE_LEN_MAX, eventType);

        if (pNpServiceName != NULL)
        {
            if ((iFilterId = sceNpWebApiCreateServicePushEventFilter(pRef->iWebApiCtxId, pRef->PushEvents.iHandleId, pNpServiceName, npServiceLabel, &dataType, 1)) < 0)
            {
                NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiAddPushEvent, sceNpWebApiCreateServicePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iFilterId)));
                return (iFilterId);
            }
        }
        else
        {
            if ((iFilterId = sceNpWebApiCreatePushEventFilter(pRef->iWebApiCtxId, &dataType, 1)) < 0)
            {
                NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiAddPushEvent, sceNpWebApiCreatePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iFilterId)));
                return (iFilterId);
            }
        }

        //register callbacks for that filter
        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            SceNpOnlineId onlineId;
            if (NetConnStatus('soid', iUser, &onlineId, sizeof(onlineId)) >= 0)
            {
                if (pNpServiceName != NULL)
                {
                    if ((pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].aCallBackIds[iUser] = sceNpWebApiRegisterServicePushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, iFilterId, _DirtyWebApiCallbackNpWebApiServicePushEvent, &pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount])) < 0)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyAcquirePushEventResources, sceNpWebApiRegisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].aCallBackIds[iUser])));
                    }
                }
                else
                {
                    if ((pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].aCallBackIds[iUser] = sceNpWebApiRegisterPushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, iFilterId, _DirtyWebApiCallbackNpWebApiPushEvent, &pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount])) < 0)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyAcquirePushEventResources, sceNpWebApiRegisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].aCallBackIds[iUser])));
                    }
                }
            }
        }

        //store off all the state
        if (pNpServiceName != NULL)
        {
            pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].bServicePushEvent = TRUE;
            pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].pNpServiceName = pNpServiceName;
            pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].npServiceLabel = npServiceLabel;
        }
        else
        {
            pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].bServicePushEvent = FALSE;
        }
        pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].iFilterId = iFilterId;
        pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].pSceEventDataType = eventType;
        pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].pPushEventCallback = callback;
        pRef->PushEvents.aEvents[pRef->PushEvents.iEventCount].pUserData = pUserData;
        pRef->PushEvents.iEventCount++;
    }
    else
    {
        iFilterId = -1;
        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiAddPushEvent, could not add event, no more room.\n", pRef));
    }

    return(iFilterId);
}


/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiInitWebApi

    \Description
        Allocated memory and setup to use the sceNpWebApi*

    \Input *pRef            - pointer to module state
    \Input *pCreateParams   - params

    \Output
        int32_t             - >=0 success, <0 failure

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiInitWebApi(DirtyWebApiRefT *pRef, const DirtyWebApiCreateParamsT* pCreateParams)
{
    int32_t iRet = 1;

    if ((pRef->iNetPoolCtxId = DirtyContextManagerCreateNetPoolContext(DIRTY_WEBAPI_NET_HEAP_NAME, pCreateParams->netHeapSize)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerCreateNetPoolContext() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->iNetPoolCtxId)));
        return(pRef->iNetPoolCtxId);
    }

    if ((pRef->iSslCtxId = DirtyContextManagerCreateSslContext(pCreateParams->sslHeapSize)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerCreateSslContext() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->iSslCtxId)));
        return(pRef->iSslCtxId);
    }

    if ((pRef->iHttpCtxId = DirtyContextManagerCreateHttpContext(pRef->iNetPoolCtxId, pRef->iSslCtxId, pCreateParams->httpHeapSize)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerCreateHttpContext() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->iHttpCtxId)));
        return(pRef->iHttpCtxId);
    }

    if ((pRef->iWebApiCtxId = sceNpWebApiInitialize(pRef->iHttpCtxId, pCreateParams->webApiHeapSize)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] sceNpWebApiInitialize() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->iWebApiCtxId)));
        return(pRef->iWebApiCtxId);
    }

    if ((iRet = scePthreadCondInit(&pRef->dirtyWepApiThreadCond, NULL, "WakeupWebApiThread")) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] scePthreadCondInit() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        return(iRet);
    }

    if ((iRet = scePthreadMutexInit(&pRef->dirtyWebApiThreadMutex, NULL, "WakeupWebApiThreadMutex")) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] scePthreadMutexInit() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        return(iRet);
    }

    //try to initialize contexts for the users, we will adjust them later if needed
    for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        _DirtyWebApiSetupWebUserContext(pRef, iIndex);
    }

    return(iRet);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiDestroy

    \Description
        Free Dirty Session Manager, only gets called by the thread as its terminating.

    \Input *pRef    - pointer to module state

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static void _DirtyWebApiShutdownWebApi(DirtyWebApiRefT *pRef)
{
    int32_t iRet;
    NetPrintf(("dirtywebapips4: [%p] final destruction\n", pRef));

    _DirtyWebApiCleanupRequestQueue(pRef, -1);

    // release player webapi contexts
    for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        _DirtyWebApiCleanupWebUserContext(pRef, iIndex);
    }

    if (pRef->iWebApiCtxId >= 0)
    {
        if ((iRet = sceNpWebApiTerminate(pRef->iWebApiCtxId)) != 0)
        {
            NetPrintf(("dirtywebapips4: [%p] sceNpWebApiTerminate() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
    }
    
    if (pRef->iHttpCtxId >= 0)
    {
        if ((iRet = DirtyContextManagerFreeHttpContext(pRef->iHttpCtxId)) != 0)
        {
            NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerFreeHttpContext() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
    }

    if (pRef->iSslCtxId >= 0)
    {
        if ((iRet = DirtyContextManagerFreeSslContext(pRef->iSslCtxId)) != 0)
        {
            NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerFreeSslContext() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
    }

    if (pRef->iNetPoolCtxId >= 0)
    {
        if ((iRet = DirtyContextManagerFreeNetPoolContext(pRef->iNetPoolCtxId)) != 0)
        {
            NetPrintf(("dirtywebapips4: [%p] DirtyContextManagerFreeNetPoolContext() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiSetupWebUserContext

    \Description
        Prepares a user to use the webapi

    \Input *pRef        - pointer to module state
    \Input iUserIndex   - user index

    \Output
        int32_t         - >=0 success, <0 failure

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiSetupWebUserContext(DirtyWebApiRefT *pRef, int32_t iUserIndex)
{
    SceNpOnlineId onlineId;
    SceUserServiceUserId userId;
    int32_t iRet;

    NetCritEnter(&pRef->crit);
    // get the user id for the user at the specified index, and ensure a user is logged in to PSN there   
    if (
        ((iRet = userId = NetConnStatus('suid', iUserIndex, NULL, 0)) < 0) || 
        ((iRet = NetConnStatus('soid', iUserIndex, &onlineId, sizeof(onlineId))) < 0)
        ) 
    {
        if (pRef->WebApiUsers[iUserIndex].aSceUserId != 0)
        {
            _DirtyWebApiCleanupWebUserContext(pRef, iUserIndex);
        }        
        NetCritLeave(&pRef->crit);
        return(iRet);
    }

    // check to see if this user is already setup with a context
    if (userId != pRef->WebApiUsers[iUserIndex].aSceUserId)
    {
        if (pRef->WebApiUsers[iUserIndex].aSceUserId != 0)
        {
            _DirtyWebApiCleanupWebUserContext(pRef, iUserIndex);
        }

        if ((iRet = sceNpWebApiCreateContext(pRef->iWebApiCtxId, &onlineId)) >= 0) 
        {
            //save information about this context
            pRef->WebApiUsers[iUserIndex].aSceUserId = userId;
            pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId = iRet;

            //go through the list of events and register this user for them
            for (int32_t iEvent = 0; iEvent < pRef->PushEvents.iEventCount; iEvent++)
            {
                if (pRef->PushEvents.aEvents[iEvent].bServicePushEvent == TRUE)
                {
                    if ((pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex] = sceNpWebApiRegisterServicePushEventCallback(pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].iFilterId, _DirtyWebApiCallbackNpWebApiServicePushEvent, &pRef->PushEvents.aEvents[iEvent])) < 0)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiSetupWebUserContext, sceNpWebApiRegisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex])));
                    }
                }
                else
                {
                    if ((pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex] = sceNpWebApiRegisterPushEventCallback(pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].iFilterId, _DirtyWebApiCallbackNpWebApiPushEvent, &pRef->PushEvents.aEvents[iEvent])) < 0)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiSetupWebUserContext, sceNpWebApiRegisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex])));
                    }
                }
            }

        }
        else
        {
            NetPrintf(("dirtywebapips4: [%p] sceNpWebApiCreateContext() failed for user %s (err=%s)\n", pRef, onlineId.data, DirtyErrGetName(iRet)));
        }
    }
    NetCritLeave(&pRef->crit);

    return(iRet);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiCleanupWebUserContext

    \Description
        Stop any web requests and clear state for a given user.

    \Input *pRef        - pointer to module state
    \Input iUserIndex   - user index

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static void _DirtyWebApiCleanupWebUserContext(DirtyWebApiRefT *pRef, int32_t iUserIndex)
{
    int32_t iRet; 
    if (pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId != 0)
    {
        //go through all the event registrations and unhook the callbacks for this user
        for (int32_t iEvent = 0; iEvent < pRef->PushEvents.iEventCount; iEvent++)
        {
            if (pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex] >= 0)
            {
                if (pRef->PushEvents.aEvents[iEvent].bServicePushEvent == TRUE)
                {
                    if ((iRet = sceNpWebApiUnregisterServicePushEventCallback(pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex])) != SCE_OK)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiCleanupWebUserContext, sceNpWebApiUnregisterServicePushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
                    }
                }
                else
                {
                    if ((iRet = sceNpWebApiUnregisterPushEventCallback(pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId, pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex])) != SCE_OK)
                    {
                        NetPrintf(("dirtywebapips4: [%p] _DirtyWebApiCleanupWebUserContext, sceNpWebApiUnregisterPushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
                    }
                }
            }
            pRef->PushEvents.aEvents[iEvent].aCallBackIds[iUserIndex] = -1;
        }

        sceNpWebApiDeleteContext(pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId);
        pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId = 0;
        pRef->WebApiUsers[iUserIndex].aSceUserId = 0;
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiPushRequest

    \Description
        Push the request onto the back of the queue.

    \Input *pRef        - pointer to module state
    \Input *pWebRequest - The DirtyWebApiWebRequestT to push onto the queue

    \Version 05/08/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static void _DirtyWebApiPushRequest(DirtyWebApiRefT *pRef, DirtyWebApiWebRequestT *pWebRequest)
{
    scePthreadMutexLock(&pRef->dirtyWebApiThreadMutex);
    NetCritEnter(&pRef->crit);
    if (pRef->pRequestListFront)
    {
        pWebRequest->pPrev = pRef->pRequestListBack;
        pRef->pRequestListBack->pNext = pWebRequest;
    }
    else
    {
        pRef->pRequestListFront = pWebRequest;
    }
    pRef->pRequestListBack = pWebRequest;
    NetCritLeave(&pRef->crit);

    // signal webapi thread to do work 
    if (scePthreadCondSignal(&pRef->dirtyWepApiThreadCond) != SCE_OK)
    {
        NetPrintf(("dirtywebapips4: [%p] failed to signal webapi thread.\n", pRef));
    }
    scePthreadMutexUnlock(&pRef->dirtyWebApiThreadMutex);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiPopRequest

    \Description
        Pop a request from the front of the queue.

    \Input *pRef        - pointer to module state
    \Input iUserIndex   - if >= 0, this function will return the next DirtyWebApiWebRequestT for the user at the given index. If < 0, the item at the front of the queue is popped.

    \Output
        DirtyWebApiWebRequestT*  - The next web request, or NULL if queue is empty.  See iUserIndex for further granularity.

    \Version 05/08/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static DirtyWebApiWebRequestT *_DirtyWebApiPopRequest(DirtyWebApiRefT *pRef, int32_t iUserIndex)
{
    DirtyWebApiWebRequestT *pWebRequest;
    NetCritEnter(&pRef->crit);
    pWebRequest = pRef->pRequestListFront;
    while (pWebRequest != NULL)
    {
        if ((iUserIndex < 0) || (pWebRequest->iUserIndex == iUserIndex))
        {
            if (pWebRequest->pPrev != NULL)
                pWebRequest->pPrev->pNext = pWebRequest->pNext;
            else
                pRef->pRequestListFront = pWebRequest->pNext;

            if (pWebRequest->pNext != NULL)
                pWebRequest->pNext->pPrev = pWebRequest->pPrev;
            else
                pRef->pRequestListBack = pWebRequest->pPrev;

            pWebRequest->pNext = NULL;
            pWebRequest->pPrev = NULL;
            break;
        }
        pWebRequest = pWebRequest->pNext;
    }
    NetCritLeave(&pRef->crit);
    return pWebRequest;
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiAbortRequest

    \Description
        Stop a particular web request, will NOT evoke completion callback.

    \Input *pRef        - pointer to module state
    \Input *pWebRequest - pointer to request
    \Input bFree        - TRUE if it should free the memory for the web request too.

    \Version 08/19/2013 (cvienneau)
*/
/*************************************************************************************F*/
static void _DirtyWebApiAbortRequest(DirtyWebApiRefT *pRef, DirtyWebApiWebRequestT *pWebRequest, uint8_t bFree)
{
    int32_t iRet;
    NetCritEnter(&pRef->crit);
    if ((iRet = sceNpWebApiAbortRequest(pWebRequest->iWebRequestId)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] sceNpWebApiAbortRequest() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
    }

    //abort does not result in a callback, if we wanted this there should be a _DirtyWebApiCancelRequest added
    //hansoft://ears-hansoft.rws.ad.ea.com;EADP$20Projects;5519c001/Task/636029
    //pWebRequest->pCallback(SCE_NP_WEBAPI_ERROR_ABORTED, pWebRequest->iUserIndex, 0, NULL, 0, pWebRequest->pUserData);

    if ((iRet = sceNpWebApiDeleteRequest(pWebRequest->iWebRequestId)) < 0) 
    {
        uint32_t i = 0;
        uint8_t bDeferredDeleteFound = FALSE;
        NetPrintf(("dirtywebapips4: [%p] sceNpWebApiDeleteRequest() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));

        // look for a spot in the defered delete list
        for (i = 0; i < DIRTY_WEBAPI_MAX_DEFERED_DELETES; ++i)
        {
            if (pRef->DeferredDeleteList[i].bActive == FALSE)
            {
                NetPrintf(("dirtywebapips4: [%p] defering delete of request %lld\n", pRef, pWebRequest->iWebRequestId));
                pRef->DeferredDeleteList[i].iWebRequestId = pWebRequest->iWebRequestId;
                pRef->DeferredDeleteList[i].bActive = TRUE;
                pRef->DeferredDeleteList[i].uTimeout = NetTick();
                bDeferredDeleteFound = TRUE;
                break;
            }
        }

        if (bDeferredDeleteFound != TRUE)
        {
            NetPrintf(("dirtywebapips4: [%p] unable to defer delete of request %lld\n", pRef, pWebRequest->iWebRequestId));
        }
    }

    if (bFree)
    {
        DirtyMemFree(pWebRequest, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }
    NetCritLeave(&pRef->crit);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiCleanupRequestQueue

    \Description
        Stop any web requests for a given user, will NOT evoke completion callback.

    \Input *pRef        - pointer to module state
    \Input iUserIndex   - user index, if < 0 all users

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiCleanupRequestQueue(DirtyWebApiRefT *pRef, int32_t iUserIndex)
{
    int32_t iCount = 0;
    DirtyWebApiWebRequestT *pWebRequest;

    NetCritEnter(&pRef->crit);

    //clear the queue
    while ((pWebRequest = _DirtyWebApiPopRequest(pRef, iUserIndex)) != NULL)
    {
        _DirtyWebApiAbortRequest(pRef, pWebRequest, TRUE);
        ++iCount;
    }

    //do the current request

    if (pRef->pCurrentWebRequest != NULL)
    {
        _DirtyWebApiAbortRequest(pRef, pRef->pCurrentWebRequest, FALSE);    // always rely on _DirtyWebApiThread to free memory. To avoid race condition.
        NetCritEnter(&pRef->abortCrit);
        pRef->pCurrentWebRequest = NULL;                                    // Set it to NULL actually signals the _DirtyWebApiThread don’t do callback.
        NetCritLeave(&pRef->abortCrit);
        ++iCount;
        NetPrintf(("dirtywebapips4: [%p] aborted %d requests including one active.\n", pRef, iCount));
    }
    else if (iCount > 0)
    {
        NetPrintf(("dirtywebapips4: [%p] aborted %d requests.\n", pRef, iCount));
    }

    NetCritLeave(&pRef->crit);
    return iCount;
}

/*F*************************************************************************************/
/*!
    \Function    _DirtyWebApiReadResponseData

    \Description
        Read any incoming data from the specified request into the supplied buffer

    \Input *pRef        - pointer to module state
    \Input requestId    - id generated by sceNpWebApiCreateRequest
    \Input *pReadBuff   - pointer data will be written too
    \Input iBuffSize    - size of pReadBuff
    \Input *pReadSize   - output for the number of bytes read in this call

    \Output
        int32_t             - >=0 success, <0 failure

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************F*/
static int32_t _DirtyWebApiReadResponseData(DirtyWebApiRefT *pRef, int64_t requestId, char *pReadBuff, int32_t iBuffSize, int32_t *pReadSize)
{
    int32_t iRet;
    size_t iReadSize = 0;

    do 
    {
        iRet = sceNpWebApiReadData(requestId, pReadBuff + iReadSize, iBuffSize - iReadSize);
        if (iRet < 0) 
        {
            NetPrintf(("dirtywebapips4: [%p] sceNpWebApiReadData() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
            return(iRet);
        }
        iReadSize += iRet;
    } 
    while (iRet > 0);    

    if (pReadSize != NULL) 
    {
        *pReadSize = iReadSize;
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function    _DirtyWebApiAddCallbackToQueue

    \Description
        Add the callback to the callback queue if queuing of callbacks is enbled.
        If not it will directly initiate the callback

    \Input  *pRef           - pointer to module state
    \Input  *pCallback      - pointer to DirtyWebApiCallbackT
    \Input  iSceError       - sce error
    \Input  iUserIndex      - user index
    \Input  iStatusCode     - status code
    \Input  pRespBody       - response body
    \Input  iRespBodyLength - response body length
    \Input  *pUserData      - user data

    \Version 02/03/2015 (tcho)
*/
/********************************************************************************F*/
static void _DirtyWebApiAddCallbackToQueue(DirtyWebApiRefT *pRef, DirtyWebApiCallbackT *pCallback, int32_t iSceError, int32_t iUserIndex, int32_t iStatusCode, const char *pRespBody, int32_t iRespBodyLength, void *pUserData)
{
    if (pRef->bQueueCallback == TRUE)
    {
        // add an entry to the callback queue
        DirtyWebApiCallbackEntryT *pCallbackEntry = (DirtyWebApiCallbackEntryT *)DirtyMemAlloc(sizeof(*pCallbackEntry), DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        memset(pCallbackEntry, 0, sizeof(*pCallbackEntry));

        if (pCallbackEntry != NULL)
        {
            pCallbackEntry->pCallback       = pCallback;
            pCallbackEntry->iSceError       = iSceError;
            pCallbackEntry->iUserIndex      = iUserIndex;
            pCallbackEntry->iStatusCode     = iStatusCode;
            pCallbackEntry->pRespBody       = pRespBody;
            pCallbackEntry->iRespBodyLength = iRespBodyLength;
            pCallbackEntry->pUserData       = pUserData;

            if (pRef->pCallbackListBack == NULL)
            {
                if (pRef->pCallbackListFront != NULL)
                {
                    pRef->pCallbackListFront->pNext = pCallbackEntry;
                }
                else
                {
                    pRef->pCallbackListFront = pCallbackEntry;
                }
            }
            else
            {
                pRef->pCallbackListBack->pNext = pCallbackEntry;
            }

            pRef->pCallbackListBack = pCallbackEntry;
        }
    }
    else
    {
        // if we dont need to queue to the callback just call it
        pCallback(iSceError, iUserIndex, iStatusCode, pRespBody, iRespBodyLength, pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function    _DirtyWebApiUpdate

    \Description
        Service webapi requests.

    \Input  *pData    - pointer to DirtyWebApiRefT module state
    \Input  uTick     - time tick

    \Version 02/03/2015 (tcho)
*/
/********************************************************************************F*/
static void _DirtyWebApiUpdate(void *pData, uint32_t uTick)
{
    DirtyWebApiRefT *pRef = (DirtyWebApiRefT *)pData;
    uint32_t i = 0;

    if (pRef->pCallbackListBack != NULL)
    {
        if (NetCritTry(&pRef->crit))
        {
            if (NetCritTry(&pRef->abortCrit))
            {
                DirtyWebApiCallbackEntryT *pCallbackEntry = pRef->pCallbackListFront;

                while (pCallbackEntry != NULL)
                {
                    pCallbackEntry->pCallback(pCallbackEntry->iSceError, pCallbackEntry->iUserIndex, pCallbackEntry->iStatusCode, pCallbackEntry->pRespBody, pCallbackEntry->iRespBodyLength, pCallbackEntry->pUserData);
                    
                    // remove processed entry
                    pRef->pCallbackListFront = pCallbackEntry->pNext;
                    DirtyMemFree(pCallbackEntry, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                    pCallbackEntry = pRef->pCallbackListFront;

                    // we reach the end of the list
                    if (pRef->pCallbackListFront == NULL)
                    {
                        pRef->pCallbackListBack = NULL;
                    }
                }

                NetCritLeave(&pRef->abortCrit);
            }

            NetCritLeave(&pRef->crit);
        }
    }

    if (NetCritTry(&pRef->abortCrit))
    {
        // loop through the defered delete list and try to clean up the requests
        for (i = 0; i < DIRTY_WEBAPI_MAX_DEFERED_DELETES; ++i)
        {
            if (pRef->DeferredDeleteList[i].bActive == TRUE)
            {
                int32_t iRet = 0;
                // try to delete the request
                if (((iRet = sceNpWebApiDeleteRequest(pRef->DeferredDeleteList[i].iWebRequestId)) == SCE_OK) || (NetTickDiff(NetTick(), pRef->DeferredDeleteList[i].uTimeout) > DIRTY_WEBAPI_MAX_DEFERED_DELETE_TIMEOUT) )
                {
                    NetPrintf(("dirtywebapips4: [%p] defered delete of request %lld\n", pRef, pRef->DeferredDeleteList[i].iWebRequestId));
                    pRef->DeferredDeleteList[i].iWebRequestId = 0;
                    pRef->DeferredDeleteList[i].bActive = FALSE;
                }
            }
        }
        NetCritLeave(&pRef->abortCrit);
    }
}

/*F********************************************************************************/
/*!
    \Function    _DirtyWebApiThread

    \Description
        Service webapi requests.

    \Input  *pArg    - pointer to DirtyWebApiRefT module state

    \Version 05/09/2013 (mcorcoran)
*/
/********************************************************************************F*/
static void *_DirtyWebApiThread(void *pArg)
{
    DirtyWebApiRefT *pRef = (DirtyWebApiRefT *)pArg;
    int32_t iRet, iStatusCode, iBytesRead;
    uint8_t bAddEvent = 0;

    NetPrintf(("dirtywebapips4: [%p] Thread starts (thread id = %d)\n", pRef, scePthreadGetthreadid()));

    while (pRef->iThreadLife >= 0)
    {
        //wait for the condition variable to be signaled if there are no pending work to be done
        scePthreadMutexLock(&pRef->dirtyWebApiThreadMutex);
        while ((pRef->pRequestListFront == NULL) && (pRef->PendingAddPushEvents == NULL) && (pRef->iThreadLife >= 0))
        {
            scePthreadCondWait(&pRef->dirtyWepApiThreadCond, &pRef->dirtyWebApiThreadMutex);
        }
        scePthreadMutexUnlock(&pRef->dirtyWebApiThreadMutex);

        //check to be sure all users are setup, so that they have events registered
        NetCritEnter(&pRef->crit);
        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            _DirtyWebApiSetupWebUserContext(pRef, iUser);
        }

        pRef->pCurrentWebRequest = _DirtyWebApiPopRequest(pRef, -1);
        DirtyWebApiWebRequestT * pCurrentWebRequestLocal = pRef->pCurrentWebRequest;     // keep a reference to avoid set to NULL by another thread. We are about to leave the lock protected area.

        //This could take a long time we are doing one push event add every tick
        DirtyWebApPushEventT * pPushEventRef = pRef->PendingAddPushEvents;
        DirtyWebApPushEventT currentPushEvent;

        if (pPushEventRef != NULL)
        {
            bAddEvent = 1;
            ds_memcpy_s(&currentPushEvent, sizeof(currentPushEvent), pPushEventRef, sizeof(*pPushEventRef));  
            pRef->PendingAddPushEvents = pPushEventRef->pNext;
            DirtyMemFree(pPushEventRef, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        NetCritLeave(&pRef->crit); 

        if (bAddEvent)
        {
            bAddEvent = 0;
           _DirtyWebApiAddPushEvent(pRef, currentPushEvent.pSceEventDataType, currentPushEvent.pNpServiceName, currentPushEvent.npServiceLabel, currentPushEvent.pPushEventCallback, currentPushEvent.pUserData);
        }

        if (pCurrentWebRequestLocal == NULL)
        {
            continue;
        }
        #if SCE_ORBIS_SDK_VERSION >= 0x02508131u
        SceNpWebApiResponseInformationOption respInfoOption;
        memset(&respInfoOption, 0, sizeof(respInfoOption));
        
        if ((iRet = sceNpWebApiSendRequest2(
            pCurrentWebRequestLocal->iWebRequestId,
            (pCurrentWebRequestLocal->iContentLength ? &pCurrentWebRequestLocal->pContent[0] : NULL),
            pCurrentWebRequestLocal->iContentLength, &respInfoOption)) >= 0)
        {
            // store http status code
            iStatusCode = respInfoOption.httpStatus;
        
        #else
        if ((iRet = sceNpWebApiSendRequest(
            pCurrentWebRequestLocal->iWebRequestId,
            (pCurrentWebRequestLocal->iContentLength ? &pCurrentWebRequestLocal->pContent[0] : NULL),
            pCurrentWebRequestLocal->iContentLength)) >= 0)
        {
            // check the response code
            if ((iRet = sceNpWebApiGetHttpStatusCode(pCurrentWebRequestLocal->iWebRequestId, &iStatusCode)) >= 0) 
        #endif
            {
                // attempt to read the data, again long blocking (save one byte to safely null terminate)
                if ((iRet = _DirtyWebApiReadResponseData(pRef, pCurrentWebRequestLocal->iWebRequestId, pRef->aReadBuffer, pRef->uReadBufferSize - 1, &iBytesRead)) >= 0)
                {
                    //a convenience for strings
                    pRef->aReadBuffer[iBytesRead] = '\0';
                    
                    //we don't want to use our normal critical section since that will 'more' likely lead to deadlock
                    NetCritEnter(&pRef->abortCrit);
                    if (pRef->pCurrentWebRequest != NULL)
                    {
                        _DirtyWebApiAddCallbackToQueue(pRef, pRef->pCurrentWebRequest->pCallback, SCE_OK, pRef->pCurrentWebRequest->iUserIndex, iStatusCode, (iBytesRead ? pRef->aReadBuffer : NULL), iBytesRead, pRef->pCurrentWebRequest->pUserData);
                    }
                    NetCritLeave(&pRef->abortCrit);
                }
            }
        #if SCE_ORBIS_SDK_VERSION >= 0x02508131u
        }
        else
        {
            NetPrintf(("dirtywebapips4: [%p] sceNpWebApiSendRequest2() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
        #else
            else
            {
                NetPrintf(("dirtywebapips4: [%p] sceNpWebApiGetHttpStatusCode() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
            }
        }
        else
        {
            NetPrintf(("dirtywebapips4: [%p] sceNpWebApiSendRequest() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));
        }
        #endif

        NetCritEnter(&pRef->abortCrit);
        if ((pRef->pCurrentWebRequest != NULL) && (iRet != SCE_NP_WEBAPI_ERROR_ABORTED))
        {
            //callback for failed requets, so the user has a chance to cleanup
            if (iRet < 0)
            {
                _DirtyWebApiAddCallbackToQueue(pRef, pRef->pCurrentWebRequest->pCallback, iRet, pRef->pCurrentWebRequest->iUserIndex, 0, NULL, 0, pRef->pCurrentWebRequest->pUserData);
            }

            if ((iRet = sceNpWebApiDeleteRequest(pRef->pCurrentWebRequest->iWebRequestId)) < 0) 
            {
                uint32_t i = 0;
                uint8_t bDeferredDeleteFound = FALSE;
                NetPrintf(("dirtywebapips4: [%p] sceNpWebApiDeleteRequest() failed, (err=%s)\n", pRef, DirtyErrGetName(iRet)));

                // look for a spot in the defered delete list
                // note that sceNpWebApiDeleteRequest will probably never fail in this scenario, we do this as a precaution
                for (i = 0; i < DIRTY_WEBAPI_MAX_DEFERED_DELETES; ++i)
                {
                    if (pRef->DeferredDeleteList[i].bActive == FALSE)
                    {
                        NetPrintf(("dirtywebapips4: [%p] defering delete of request %lld\n", pRef, pRef->pCurrentWebRequest->iWebRequestId));
                        pRef->DeferredDeleteList[i].iWebRequestId = pRef->pCurrentWebRequest->iWebRequestId;
                        pRef->DeferredDeleteList[i].bActive = TRUE;
                        pRef->DeferredDeleteList[i].uTimeout = NetTick();
                        bDeferredDeleteFound = TRUE;
                        break;
                    }
                }

                if (bDeferredDeleteFound != TRUE)
                {
                    NetPrintf(("dirtywebapips4: [%p] unable to defer delete of request %lld\n", pRef, pRef->pCurrentWebRequest->iWebRequestId));
                }
            }
            pRef->pCurrentWebRequest = NULL;
        }
        else
        {
            NetPrintf(("dirtywebapips4: [%p] request was aborted\n", pRef));
        }
        DirtyMemFree(pCurrentWebRequestLocal, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        NetCritLeave(&pRef->abortCrit);
    }

    NetPrintf(("dirtywebapips4: [%p] Thread exit (thread id = %d)\n", pRef, scePthreadGetthreadid()));
    return(NULL);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiCreate

    \Description
        Allocate module state and prepare for use.

    \Input  pCreateParams - creation parameters as per user preference

    \Output
        DirtyWebApiRefT *   - reference pointer (must be passed to all other functions)

    \Version 09/06/2013 (abaldeva)
*/
/*************************************************************************************************F*/
DirtyWebApiRefT *DirtyWebApiCreate(const DirtyWebApiCreateParamsT* pCreateParams)
{
    DirtyWebApiRefT *pRef;
    ScePthreadAttr attr;
    int32_t iMemGroup, iResult, iThreadCpuAffinity;
    void *pMemGroupUserData;
    DirtyWebApiCreateParamsT createParamsLocal;

    if(pCreateParams)
    {
        createParamsLocal = *pCreateParams;
    }
    else
    {
        memset(&createParamsLocal, 0, sizeof(createParamsLocal));
    }

    createParamsLocal.netHeapSize    = (createParamsLocal.netHeapSize == 0)    ? DIRTY_WEBAPI_NET_HEAP_SIZE    : createParamsLocal.netHeapSize;
    createParamsLocal.sslHeapSize    = (createParamsLocal.sslHeapSize == 0)    ? DIRTY_WEBAPI_SSL_HEAP_SIZE    : createParamsLocal.sslHeapSize;
    createParamsLocal.httpHeapSize   = (createParamsLocal.httpHeapSize == 0)   ? DIRTY_WEBAPI_HTTP_HEAP_SIZE   : createParamsLocal.httpHeapSize;
    createParamsLocal.webApiHeapSize = (createParamsLocal.webApiHeapSize == 0) ? DIRTY_WEBAPI_WEBAPI_HEAP_SIZE : createParamsLocal.webApiHeapSize;
    createParamsLocal.readBufferSize = (createParamsLocal.readBufferSize == 0) ? DIRTY_WEBAPI_READ_BUFFER_SIZE : createParamsLocal.readBufferSize;

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = (DirtyWebApiRefT*)DirtyMemAlloc(sizeof(*pRef), DIRTYSESSMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtywebapips4: [%p] failed to allocate module state.\n", pRef));
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->uReadBufferSize = createParamsLocal.readBufferSize;
    pRef->bQueueCallback = FALSE;

    if ((pRef->aReadBuffer = (char*)DirtyMemAlloc(pRef->uReadBufferSize, DIRTYSESSMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemFree(pRef, DIRTYSESSMGR_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("dirtywebapips4: [%p] failed to allocate read buffer.\n", pRef));
        return(NULL);
    }

    NetCritInit(&pRef->crit, DIRTY_WEBAPI_NET_HEAP_NAME);
    NetCritInit(&pRef->abortCrit, DIRTY_WEBAPI_NET_HEAP_NAME);

    //prepare web api
    pRef->iThreadLife = 1;

    if (_DirtyWebApiInitWebApi(pRef,&createParamsLocal) < 0)
    {
        NetPrintf(("dirtywebapips4: failed to initialize Np WebApi\n"));
        DirtyWebApiDestroy(pRef);
        return(NULL);
    }

    //start a thread to service webapi
    if ((iResult = scePthreadAttrInit(&attr)) == SCE_OK)
    {
        SceKernelSchedParam defaultParam;

        if ((iResult = scePthreadAttrSetdetachstate(&attr, SCE_PTHREAD_CREATE_JOINABLE)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadAttrSetdetachstate failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        if ((iThreadCpuAffinity = NetConnStatus('affn', 0, NULL, 0)) > 0)
        {
            if ((iResult = scePthreadAttrSetaffinity(&attr, iThreadCpuAffinity)) != SCE_OK)
            {
                NetPrintf(("dirtywebapips4: scePthreadAttrSetaffinity %x failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }
        else
        {
            if ((iResult = scePthreadAttrSetaffinity(&attr, SCE_KERNEL_CPUMASK_USER_ALL)) != SCE_OK)
            {
                NetPrintf(("dirtywebapips4: scePthreadAttrSetaffinity SCE_KERNEL_CPUMASK_USER_ALL failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }

        // set scheduling policy to round-robin (for consistency with EAthread)
        if ((iResult = scePthreadAttrSetschedpolicy(&attr, SCHED_RR)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadAttrSetschedpolicy SCHED_RR failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // set thread priority to normal to avoid continuously context-switching out game's important threads (sim thread, job threads, etc) running at higher priority
        // (needs to be executed after scePthreadAttrSetschedpolicy() which resets the priority in the attr struct)
        defaultParam.sched_priority = SCE_KERNEL_PRIO_FIFO_DEFAULT;
        if ((iResult = scePthreadAttrSetschedparam(&attr, &defaultParam)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadAttrSetschedparam SCE_KERNEL_PRIO_FIFO_DEFAULT failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // set inheritance flag to EXPLICIT to avoid inheriting thread settings for parent thread
        if ((iResult = scePthreadAttrSetinheritsched(&attr, SCE_PTHREAD_EXPLICIT_SCHED)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadAttrSetinheritsched SCE_PTHREAD_EXPLICIT_SCHED failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        if ((iResult = scePthreadCreate(&pRef->WebApiThread, &attr, _DirtyWebApiThread, pRef, "WebApiThread")) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadCreate failed (err=%s)\n", DirtyErrGetName(iResult)));
            scePthreadAttrDestroy(&attr);
            DirtyWebApiDestroy(pRef);
            return(NULL);   
        }

        if ((iResult = scePthreadAttrDestroy(&attr)) != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: scePthreadAttrDestroy failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
    else
    {
        NetPrintf(("dirtywebapips4: scePthreadAttrInit failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyWebApiDestroy(pRef);
        return(NULL);
    }

    //setup initial state for push events
    _DirtyWebApiInitPushEvents(pRef);

    // add _dirtywebapiupdate to netconn idler
    NetConnIdleAdd(_DirtyWebApiUpdate, pRef);

    NetPrintf(("dirtywebapips4: [%p] initial creation completed.\n", pRef));

    // return ref to caller
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - reference pointer

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
void DirtyWebApiDestroy(DirtyWebApiRefT *pRef)
{
    NetPrintf(("dirtywebapips4: [%p] DirtyWebApiDestroy initiated\n", pRef));
    
    scePthreadMutexLock(&pRef->dirtyWebApiThreadMutex);
    pRef->iThreadLife = -1;

    // signal webapi thread to do work 
    if (scePthreadCondSignal(&pRef->dirtyWepApiThreadCond) != SCE_OK)
    {
        NetPrintf(("dirtywebapips4: [%p] failed to signal webapi thread.\n", pRef));
    }
    scePthreadMutexUnlock(&pRef->dirtyWebApiThreadMutex);

    if (pRef->WebApiThread != NULL)
    {
        int32_t iRet = scePthreadJoin(pRef->WebApiThread, NULL);
        if (iRet != SCE_OK)
        {
            NetPrintf(("dirtywebapips4: [%p] scePthreadJoin failed with error (%d)\n", pRef, iRet));
        }
    }
    
    NetConnIdleDel(_DirtyWebApiUpdate, pRef);
    _DirtyWebApiDestroyPushEvents(pRef);
    _DirtyWebApiShutdownWebApi(pRef);
  
    scePthreadCondDestroy(&pRef->dirtyWepApiThreadCond);
    scePthreadMutexDestroy(&pRef->dirtyWebApiThreadMutex);

    NetCritKill(&pRef->crit);
    NetCritKill(&pRef->abortCrit);
    DirtyMemFree(pRef->aReadBuffer, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    DirtyMemFree(pRef, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiDoRequestAsync

    \Description
        Destroy the module and release its state

    \Input *pRef           - reference pointer
    \Input  iUserIndex     - user index
    \Input *pApiGroup      - SCE API group
    \Input  eHttpMethod    - SCE HTTP method
    \Input *pPath          - the URI path to request
    \Input *pContent       - body content to be sent along with the HTTP request (e.g. POST or PUT requests)
    \Input *iContentLength - context length
    \Input *pContentType   - context type
    \Input *pCallback      - callback pointer
    \Input *pUserData      - user data

    \Output
        int32_t    - 0 on success, else negative=error

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
int32_t DirtyWebApiRequest(DirtyWebApiRefT *pRef, int32_t iUserIndex, const char *pApiGroup, SceNpWebApiHttpMethod eHttpMethod, const char *pPath, const uint8_t *pContent, int32_t iContentLength, const char *pContentType, DirtyWebApiCallbackT *pCallback, void *pUserData)
{
    NetPrintf(("dirtywebapips4: [%p] DirtyWebApiDoRequestAsync: pApiGroup: [%s] pPath [%s]\n", pRef, pApiGroup, pPath));
    int32_t iRet;
    int64_t iWebRequestId;

    if ((iRet = _DirtyWebApiSetupWebUserContext(pRef, iUserIndex)) < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] No valid user web context for user: %d\n", pRef, iUserIndex));
        return(-1);
    }

    SceNpWebApiContentParameter aContentParameter;
    if (iContentLength)
    {
        aContentParameter.contentLength = iContentLength;
        aContentParameter.pContentType = pContentType;
    }

    if ((iRet = sceNpWebApiCreateRequest(
        pRef->WebApiUsers[iUserIndex].iWebapiUserCtxId,
        pApiGroup,
        pPath,
        eHttpMethod,
        (iContentLength ? &aContentParameter : NULL),
        &iWebRequestId)) >= 0)
    {
        DirtyWebApiWebRequestT *pWebRequest;
        if ((pWebRequest = (DirtyWebApiWebRequestT*)DirtyMemAlloc(sizeof(DirtyWebApiWebRequestT) + iContentLength, DIRTYWEBAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("dirtywebapips4: could not allocate memory for DirtyWebApiWebRequestT\n"));
            return(-1);
        }

        memset(pWebRequest, 0, sizeof(DirtyWebApiWebRequestT));

        pWebRequest->iUserIndex = iUserIndex;
        pWebRequest->pCallback = pCallback;
        pWebRequest->pUserData = pUserData;
        pWebRequest->iWebRequestId = iWebRequestId;
        if (iContentLength)
        {
            pWebRequest->iContentLength = iContentLength;
            ds_memcpy(&pWebRequest->pContent[0], pContent, pWebRequest->iContentLength);
        }

        _DirtyWebApiPushRequest(pRef, pWebRequest);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiAbortRequests

    \Description
        Stop any web requests for a given user, will NOT evoke completion callback.
        Not to be called from a completion callback.

    \Input *pRef        - pointer to module state
    \Input iUserIndex   - user index, if < 0 all users

    \Output
        int32_t    - negative=error, else the number of requests aborted

    \Version 05/09/2013 (mcorcoran)
*/
/*************************************************************************************************F*/
int32_t DirtyWebApiAbortRequests(DirtyWebApiRefT *pRef, int32_t iUserIndex)
{
    if (pRef == NULL)
    {
        NetPrintf(("dirtywebapips4: invalid module ref pointer\n"));
        return(-1);
    }

    if ((iUserIndex < -1) || (iUserIndex >= NETCONN_MAXLOCALUSERS))
    {
        NetPrintf(("dirtywebapips4: [%p] iUserIndex(%d) is not a valid user index\n", iUserIndex));
        return(-1);
    }

    return _DirtyWebApiCleanupRequestQueue(pRef, iUserIndex);
}

/*F*************************************************************************************/
/*!
    \Function    DirtyWebApiAddPushEventListenerEx

    \Description
        Adds a registration for push events, gives access to the "service" apis

    \Input pRef             - module state
    \Input eventType        - event type from sony ie "np:service:invitation"
    \Input pNpServiceName   - NP web service generating the event ie "sessionInvitation"
    \Input npServiceLabel   - more detail on the web service, usually SCE_NP_DEFAULT_SERVICE_LABEL
    \Input callback         - function to call when the event triggers
    \Input pUserData        - data passed to the callback when it triggers

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
int32_t DirtyWebApiAddPushEventListenerEx(DirtyWebApiRefT *pRef, const char * eventType, const char * pNpServiceName, SceNpServiceLabel npServiceLabel, DirtyWebApiPushEventCallbackT *callback, void *pUserData)
{
    return _DirtyWebApiAddPushEventAsync(pRef, eventType, pNpServiceName, npServiceLabel, callback, pUserData);
}

/*F*************************************************************************************/
/*!
    \Function    DirtyWebApiAddPushEventListener

    \Description
        Adds a registration for basic push events

    \Input pRef             - module state
    \Input eventType        - event type from sony ie "np:service:presence:onlineStatus"
    \Input callback         - function to call when the event triggers
    \Input pUserData        - data passed to the callback when it triggers

    \Version 07/20/2013 (cvienneau)
*/
/*************************************************************************************F*/
int32_t DirtyWebApiAddPushEventListener(DirtyWebApiRefT *pRef, const char * eventType, DirtyWebApiPushEventCallbackT *callback, void *pUserData)
{
    return _DirtyWebApiAddPushEventAsync(pRef, eventType, NULL, SCE_NP_DEFAULT_SERVICE_LABEL, callback, pUserData);
}

/*F*************************************************************************************/
/*!
    \Function    DirtyWebApiRemovePushEventListener

    \Description
        Removes a registration for basic push events

    \Input *pRef            - module state
    \Input filterId         - filter id
    \Input *callback        - function to remove

    \Output
        int32_t             - 0 for success; negative for error

    \Version 09/04/2013 (amakoukji)
*/
/*************************************************************************************F*/
int32_t DirtyWebApiRemovePushEventListener(DirtyWebApiRefT *pRef, int32_t filterId, DirtyWebApiPushEventCallbackT *callback)
{
    // find the callback in the list
    int32_t iTarget = -1, iResult = 0;
    for(int32_t i = 0; i < pRef->PushEvents.iEventCount; ++i)
    {
        if (pRef->PushEvents.aEvents[i].iFilterId == filterId)
        {
            if (pRef->PushEvents.aEvents[i].pPushEventCallback == callback)
            {
                iTarget = i;
                break;
            }
            else
            {
                NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, filterId (%d) and callback pointer (%p) do not match.\n", pRef, filterId, callback));
                return(-1);
            }
        }
    }

    if (iTarget < 0)
    {
        NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, attempt to remove unregistered callback %p)\n", pRef, callback));
        return(-1);
    }

    // unresgister and clear the sce objects
    if (pRef->PushEvents.aEvents[iTarget].bServicePushEvent)
    {
        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            iResult = SCE_OK;
            if (pRef->PushEvents.aEvents[iTarget].aCallBackIds[iUser] >= 0)
            {
                iResult = sceNpWebApiUnregisterServicePushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, pRef->PushEvents.aEvents[iTarget].aCallBackIds[iUser]);
            }

            if (iResult < SCE_OK)
            {
                NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, sceNpWebApiUnregisterServicePushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iResult)));
            }
        }

        iResult = sceNpWebApiDeleteServicePushEventFilter(pRef->iWebApiCtxId, pRef->PushEvents.aEvents[iTarget].iFilterId);
        if (iResult < SCE_OK)
        {
            NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, sceNpWebApiDeleteServicePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iResult)));
        }
    }
    else
    {
        for (int32_t iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            iResult = SCE_OK;
            if (pRef->PushEvents.aEvents[iTarget].aCallBackIds[iUser] >= 0)
            {
                iResult = sceNpWebApiUnregisterPushEventCallback(pRef->WebApiUsers[iUser].iWebapiUserCtxId, pRef->PushEvents.aEvents[iTarget].aCallBackIds[iUser]);
            }

            if (iResult < SCE_OK)
            {
                NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, sceNpWebApiUnregisterServicePushEventCallback() failed, (err=%s)\n", pRef, DirtyErrGetName(iResult)));
            }
        }

        iResult = sceNpWebApiDeletePushEventFilter(pRef->iWebApiCtxId, pRef->PushEvents.aEvents[iTarget].iFilterId);
        if (iResult < SCE_OK)
        {
            NetPrintf(("dirtywebapips4: [%p] DirtyWebApiRemovePushEventListener, sceNpWebApiDeleteServicePushEventFilter() failed, (err=%s)\n", pRef, DirtyErrGetName(iResult)));
        }
    }

    // move the rest of the array 1 down
    if (iTarget < pRef->PushEvents.iEventCount - 1)
    {
        memmove((void*)&pRef->PushEvents.aEvents[iTarget], (void*)&pRef->PushEvents.aEvents[iTarget+1], (sizeof(DirtyWebApPushEventT) * (pRef->PushEvents.iEventCount - (iTarget+1))));
    }

    // decrement count
    --pRef->PushEvents.iEventCount;

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiControl

    \Description
        Destroy the module and release its state

    \Input *pRef    - reference pointer
    \Input iControl - control selector
    \Input iValue   - control value
    \Input iValue2  - control value
    \Input *pValue  - [in/out] control value

    \Output
        int32_t    - negative=error, else success

        \verbatim
            qcal: TRUE to enable callback queuing else FALSE. All non push event callback if queue will be executed by the NetConnIdle thread
            rbuf: set the read buffer size.
        \endverbatim

    \Version 05/22/2013 (amakoukji)
*/
/*************************************************************************************************F*/
int32_t DirtyWebApiControl(DirtyWebApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (pRef == NULL)
    {
        return(-1);
    }

    // enable or disble queuing of callbacks to be call later by _dirtywebapiupdate()
    if (iControl == 'qcal')
    {
        NetPrintf(("dirtywebapips4: %s the queuing of dirtywebapi request callback.\n", iValue ? "Enable" : "Disable"));
        pRef->bQueueCallback = iValue;
    }

    // set the read buffer size
    if (iControl == 'rbuf')
    {
        if (iValue < 0)
        {
            NetPrintf(("dirtywebapips4: Invalid read buffer size %d\n", iValue));
            return(-1);
        }
        else if ((uint32_t)iValue > pRef->uReadBufferSize)
        {
            char *pTmp = (char*)DirtyMemAlloc(iValue, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            if (pTmp == NULL)
            {
                NetPrintf(("dirtywebapips4: Unable to grow read buffer [%p] to size %d\n", pRef, iValue));
                return(-1);
            }
            DirtyMemFree(pRef->aReadBuffer, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            pRef->aReadBuffer = pTmp;
        }

        pRef->uReadBufferSize = (uint32_t)iValue;
        return(iValue);
    }

    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtyWebApiStatus

    \Description
        Different status selectors do different functions

    \Input *pRef    - reference pointer
    \Input iStatus - status selector
    \Input iValue   - status value
    \Input *pBuf    - buffer to pass outputs
    \Input iBufSize - [in/out] buffer size value

    \Output
        int32_t    - negative=error, else success

        \verbatim
            qcal: return true if queuing is enabled
        \endverbatim

    \Version 07/21/2015 (tcho)
*/
/*************************************************************************************************F*/
int32_t DirtyWebApiStatus(DirtyWebApiRefT *pRef, int32_t iStatus, int32_t iValue, void* pBuf, int32_t iBufSize)
{
    // return true if queuing is enabled
    if (iStatus == 'qcal')
    {
        return (pRef->bQueueCallback);
    }

    return (-1);
}
