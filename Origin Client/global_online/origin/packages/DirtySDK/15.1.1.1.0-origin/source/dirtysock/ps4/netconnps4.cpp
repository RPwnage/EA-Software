/*H********************************************************************************/
/*!
    \File netconnps4.cpp

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 10/24/2012 (jbrookes) First version; a vanilla port from the Unix version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libnetctl.h>
#include <np.h>
#include <np/np_common.h>
#include <user_service.h>
#include <kernel.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/ps4/netconnps4.h"
#include "DirtySDK/dirtysock/ps4/dirtyeventmanagerps4.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoupnp.h"
#include "netconncommon.h"

#ifdef DIRTYSDK_IEAUSER_ENABLED
#include "IEAUser/IEAUser.h"
#endif

/*** Defines **********************************************************************/

#define NETCONNPS4_SAMPLE_TITLE_ID         "NPXX51027_00"
#define NETCONNPS4_CLIENT_ID               "33f3a6af-0a39-4c98-9870-bee66f2cbc57"  //Harcoded Client Id from Sony
#define NETCONNPS4_SCOPE_FOR_AUTH_CODE     "psn:s2s"
#define NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY (12) //! initial size of external cleanup list (in number of entries)

//! UPNP port
#define NETCONN_DEFAULT_UPNP_PORT          (3659)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
typedef int32_t (*NetConnExternalCleanupCallbackT)(void *pNetConnExternalCleanupData);

typedef struct NetConnExternalCleanupEntryT
{
    void *pCleanupData;                         //!< pointer to data to be passed to the external cleanup callback
    NetConnExternalCleanupCallbackT  pCleanupCb;//!< external cleanup callback
} NetConnExternalCleanupEntryT;

typedef enum NetConnNpRequestStateE
{
    NETCONN_NP_REQUEST_STATE_INVALID = 0,    // auth code has not yet been requested
    NETCONN_NP_REQUEST_STATE_FAILED,         // auth code acquisition failed
    NETCONN_NP_REQUEST_STATE_INPROG,         // auth code acquisition in progress
    NETCONN_NP_REQUEST_STATE_VALID,          // auth code valid
    NETCONN_NP_REQUEST_STATE_UNDERAGE        // special case error code
} NetConnNpRequestStateE;

typedef struct NetConnUserAuthCodeT
{
    NetConnNpRequestStateE          eAuthCodeState;     // current progress of getting an auth code
    int32_t                         iAuthCodeRequestId; // request id into sce libs for auth code
    int32_t                         iIssuerId;          // auth code issuer id
    SceNpAuthorizationCode          authCode;           // completed auth code
} NetConnUserAuthCodeT;

typedef struct NetConnRefT NetConnRefT;
typedef struct NetConnAsyncRequestDataT NetConnAsyncRequestDataT;
typedef int32_t (*NetConnAsyncRequestT)(NetConnRefT *pRef, int32_t iUserIndex, SceNpOnlineId onlineId, NetConnAsyncRequestDataT *pRequestData);
typedef int32_t (*NetConnAsyncResponseT)(NetConnRefT *pRef, int32_t iUserIndex, void *pBuf, int32_t iBufSize);

struct NetConnAsyncRequestDataT
{
    NetConnNpRequestStateE          eRequestState;      // current progress of the async request
    int32_t                         iRequestId;         // request id for the async request
};

typedef struct NetConnParentalControlsT
{
    NetConnAsyncRequestDataT        data;               // async request info
    int8_t                          iAge;               // the age of the user
    SceNpParentalControlInfo        parentalControls;   // completed parental control settings structure
} NetConnParentalControlsT;

typedef struct NetConnPsPlusDataT
{
    NetConnAsyncRequestDataT        data;               // async request info
    SceNpCheckPlusParameter         Param;              // PS+ features to check
    SceNpCheckPlusResult            Result;             // PS+ check result
} NetConnPsPlusDataT;

typedef struct NetConnNpAvailabilityT
{
    int32_t iRequestId;
    int32_t iResult;
    uint8_t bDone;
} NetConnNpAvailabilityT;

// note: All the arrays in this struct store data in the order that Sony sees the users.
//       To get the data for the user index the caller would expect use
//       _NetConnSceUserServiceIndexToUserIndex and _NetConnUserIndexToSceUserServiceIndex as appropriate.
typedef struct NetConnUserT
{
    SceNpId                         aSceNpid[NETCONN_MAXLOCALUSERS];          //!< associated NP id
    SceNpState                      aSceNpState[NETCONN_MAXLOCALUSERS];       //!< tracks the login status of players to the Playstation Network
    NetConnUserAuthCodeT            aAuthCode[NETCONN_MAXLOCALUSERS];         //!< tracks the progress of getting an auth code
    NetConnParentalControlsT        aParentalControls[NETCONN_MAXLOCALUSERS]; //!< tracks the progress of getting the parental controls
    NetConnPsPlusDataT              aPsPlus[NETCONN_MAXLOCALUSERS];           //!< tracks the progress of getting the PS+ membership
    NetConnNpAvailabilityT          aNpAvailability[NETCONN_MAXLOCALUSERS];   //!< tracks the progress of getting the sceNpCheckNpAvailability
#ifdef DIRTYSDK_IEAUSER_ENABLED
    SceUserServiceUserId            aSceUserServiceUserId[NETCONN_MAXLOCALUSERS];  //!< associated sceUserService id
#else
    SceUserServiceLoginUserIdList   aSceUsers;                                //<! tracks the signin status of players to the ps4 console
#endif
} NetConnUserT;

typedef struct NetConnSceNpOnlineIdQueryT
{
    SceNpOnlineId   onlineId;
    int32_t         result;
    uint8_t         invalid;
} NetConnSceNpOnlineIdQueryT;

typedef struct NetConnNpStateCallbackDataT
{
    SceUserServiceUserId userId;
    SceNpState           state;
    NetConnNpStateCallbackDataT *pNext;
} NetConnNpStateCallbackDataT;

//! private module state
struct NetConnRefT
{
   NetConnCommonRefT     Common;                                  //!< cross-platform netconn data (must come first!)

    enum
    {
        ST_INIT,                                                  //!< initialization
        ST_CONN,                                                  //!< bringing up network interface
        ST_IDLE,                                                  //!< active
        ST_EXT_CLEANUP                                            //!< cleaning up external instances from previous NetConn connection, can't proceed before all cleaned up
    } eState;                                                     //!< internal connection state

    uint32_t             uConnStatus;                             //!< connection status (surfaced to user)
    uint32_t             uLastConnStatus;                         //!< connection status before being reset by NetConnDisconnect()
    uint32_t             uPlatEnv;                                //!< Platform Environment
    uint32_t             uConnUserMode;                           //!< user mode, 0 = all user must be eligible to go +onl, 1 = only one user must be eligible to go +onl

    ProtoUpnpRefT        *pProtoUpnp;                             //!< protoupnp module state
    int32_t              iPeerPort;                               //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port
    int32_t              iNumProcCores;                           //!< number of processor cores on the system

    NetConnExternalCleanupEntryT *pExternalCleanupList;           //!< pointer to an array of entries pending external cleanup completion
    int32_t              iExternalCleanupListMax;                 //!< maximum number of entries in the array (in number of entries)
    int32_t              iExternalCleanupListCnt;                 //!< number of valid entries in the array

    char                 strTitleId[SCE_NP_TITLE_ID_LEN + 1];     //!< sce title id
    SceKernelCpumask     CpuAffinity;                             //!< CPU affinity for threads used by sony
    SceNpTitleSecret     NpTitleSecret;                           //!< sce title secret
    SceNpClientId        NpClientId;                              //!< SceNpClientId used for getting auth code
    int32_t              iUserThreadPrio;                         //!< the priority of the user service thread
    int32_t              iLastNpError;                            //!< holds the last error returned by a call to an sceNp library function

    NetConnUserT         NpUsers;                                 //!< NetConUsers

#ifdef DIRTYSDK_IEAUSER_ENABLED
    const EA::User::IEAUser *aIEAUsers[NETCONN_MAXLOCALUSERS];    //!< IEAUsers known by NetConn - populated when pIEAUserEventList is processed
#else
    SceUserServiceUserId aUserIndex[NETCONN_MAXLOCALUSERS];       //!< DirtySDK's user index table
#endif

    NetConnSceNpOnlineIdQueryT      aSceNpOnlineIdCache[NETCONN_MAXLOCALUSERS];

    NetConnNpStateCallbackT        *pNpStateCallback;
    void                           *pNpStateCallbackUserData;
    NetConnNpStateCallbackDataT    *pNpStateCallbackDataQueue;

    uint8_t              bNpToolKit;                              //!< TRUE if user do not want DirtySDK to use NP Tool Kit
    uint8_t              bNpInit;
    uint8_t              bSecretSet;                              //!< TRUE if secret has been set
    uint8_t              bProcessingCleanupList;
    uint8_t              bConnectDelayed;                         //!< if TRUE, NetConnConnect is being delayed until external cleanup phase completes
    uint8_t              bEnviRequest;                            //!< TRUE if the platform environment is requested.
    uint8_t              bUserSessionPriorInitialization;         //!< TRUE if user session was already initialized
    uint8_t              _pad[1];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

//TO DO: replace with the real secret once this is created
// Note that we do not know the use of this secret yet.
// It could be it is not a good idea to have it hardcoded
// like this for security reasons. If so, simply have
// NetConnConnect() return -1 if the secret is not set.
static uint8_t aDirtySdkSampleSecret[] = {0x2a, 0x00, 0x27, 0xfa, 0x77, 0x7c, 0xdc, 0x31, 0xfa, 0x65, 0xc2, 0x55, 0x6b, 0xa3, 0x31, 0xef, 0x10, 0x39, 0x83, 0x7a, 0xa0, 0xfe, 0xf8, 0xa7, 0x70, 0xbf, 0x68, 0x05, 0x9b, 0x1a, 0xb1, 0x9b, 0x67, 0xfc, 0x8a, 0x1f, 0xfe, 0x43, 0x7b, 0xd8, 0x82, 0xf7, 0xbf, 0xad, 0x52, 0x12, 0xa8, 0x19, 0x03, 0x94, 0x17, 0x18, 0xb4, 0x92, 0x44, 0xb4, 0x47, 0x3a, 0xdd, 0x29, 0x2c, 0x29, 0x87, 0x2a, 0xf5, 0x83, 0x90, 0xdd, 0xec, 0x9d, 0x81, 0x87, 0xff, 0xbc, 0x29, 0x25, 0xd7, 0xf3, 0x48, 0x3b, 0xb4, 0xae, 0xa8, 0x14, 0xf8, 0xa7, 0xa9, 0x10, 0x6a, 0x9f, 0xe0, 0xcf, 0xd3, 0x65, 0xec, 0xe7, 0x99, 0x08, 0x4d, 0x0f, 0xb9, 0x22, 0x4b, 0x4f, 0x57, 0x0e, 0x79, 0x58, 0x7d, 0x82, 0x26, 0x41, 0x90, 0x8e, 0x06, 0xc6, 0x44, 0x72, 0x57, 0x3c, 0x7a, 0x7b, 0xf7, 0x1b, 0x49, 0x9a, 0x8f, 0xa6};

#if DIRTYCODE_LOGGING
static const char *_NpState[] =
{
    "SCE_NP_STATE_UNKNOWN",            // 0
    "SCE_NP_STATE_SIGNED_OUT",         // 1
    "SCE_NP_STATE_SIGNED_IN"           // 2
};
#endif

/*** Private Functions ************************************************************/

static int32_t _NetConnSaveErr(NetConnRefT *pRef, int32_t iError)
{
    if (iError < 0)
    {
        pRef->iLastNpError = iError;
        DirtyErrAppReport(iError);
    }
    return iError;
}

static void _NetConnInvalidateSceNpOnlineIdCache(NetConnRefT *pRef)
{
    int32_t i;

    // mark all online ID queries as invalid
    NetCritEnter(&pRef->Common.crit);
    for (i = 0; i < NETCONN_MAXLOCALUSERS; i++)
    {
        pRef->aSceNpOnlineIdCache[i].invalid = TRUE;
    }
    NetCritLeave(&pRef->Common.crit);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateConnStatus

    \Description
        Update the Connection Status and provide logging of status changes

    \Input *pRef             - pointer to net NetConn module ref
    \Input *uNewConnStatus   - the new conn status

    \Version 01/19/2015 (tcho)
*/
/********************************************************************************F*/
static void _NetConnUpdateConnStatus(NetConnRefT *pRef, uint32_t uNewConnStatus)
{
    int32_t iIndex;
    char strConnStatus[5];

    pRef->uConnStatus = uNewConnStatus;

    for (iIndex = 0; iIndex < 4; ++iIndex)
    {
        strConnStatus[iIndex] = ((char *) &uNewConnStatus)[3 - iIndex];
    }

    strConnStatus[4] = 0;

    NetPrintf(("netconnps4: netconn status changed to %s\n", strConnStatus));
}

/*F********************************************************************************/
/*!
    \Function   _NetConnAddToExternalCleanupList

    \Description
        Add an entry to the list of external module pending successful cleanup.

    \Input *pRef            - netconn module state
    \Input *pCleanupCb      - cleanup callback
    \Input *pCleanupData    - data to be passed to cleanup callback

    \Output
        uint32_t            -  0 for success; -1 for failure.

    \Version 12/07/2009 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnAddToExternalCleanupList(NetConnRefT *pRef, NetConnExternalCleanupCallbackT pCleanupCb, void *pCleanupData)
{
    // if list if full, double its size.
    if (pRef->iExternalCleanupListCnt == pRef->iExternalCleanupListMax)
    {
        NetConnExternalCleanupEntryT *pNewList;
        NetConnExternalCleanupEntryT *pOldList = pRef->pExternalCleanupList;

        // allocate new external cleanup list
        if ((pNewList = (NetConnExternalCleanupEntryT *)DirtyMemAlloc(2 * pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData)) == NULL)
        {
            NetPrintf(("netconnps4: unable to allocate memory for the external cleanup list\n"));
            return(-1);
        }
        memset(pNewList, 0, 2 * pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // copy contents of old list in contents of new list
        ds_memcpy(pNewList, pOldList, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // replace old list with new list
        pRef->pExternalCleanupList = pNewList;
        pRef->iExternalCleanupListMax = pRef->iExternalCleanupListMax * 2;

        // free old list
        DirtyMemFree(pOldList, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);
    }

    // add new entry to the list
    pRef->pExternalCleanupList[pRef->iExternalCleanupListCnt].pCleanupCb = pCleanupCb;
    pRef->pExternalCleanupList[pRef->iExternalCleanupListCnt].pCleanupData = pCleanupData;
    pRef->iExternalCleanupListCnt += 1;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnProcessExternalCleanupList

    \Description
        Walk external cleanup list and try to destroy each individual entry.

    \Input *pRef     - netconn module state

    \Version 12/07/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessExternalCleanupList(NetConnRefT *pRef)
{
    int32_t iEntryIndex;
    int32_t iEntryIndex2;

    // make sure we don't re-enter this code; some modules may call NetConnStatus() during their cleanup
    if (pRef->bProcessingCleanupList)
    {
        return;
    }
    pRef->bProcessingCleanupList = TRUE;

    for (iEntryIndex = 0; iEntryIndex < pRef->iExternalCleanupListMax; iEntryIndex++)
    {
        if (pRef->pExternalCleanupList[iEntryIndex].pCleanupCb == NULL)
        {
            // no more entry in list
            break;
        }

        if(pRef->pExternalCleanupList[iEntryIndex].pCleanupCb(pRef->pExternalCleanupList[iEntryIndex].pCleanupData) == 0)
        {
            pRef->iExternalCleanupListCnt -= 1;

            NetPrintf(("netconnps4: successfully destroyed external module (cleanup data ptr = %p), external cleanup list count decremented to %d\n",
                pRef->pExternalCleanupList[iEntryIndex].pCleanupData, pRef->iExternalCleanupListCnt));

            // move all following entries one cell backward in the array
            for(iEntryIndex2 = iEntryIndex; iEntryIndex2 < pRef->iExternalCleanupListMax; iEntryIndex2++)
            {
                if (iEntryIndex2 == pRef->iExternalCleanupListMax-1)
                {
                    // last entry, reset to NULL
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = NULL;
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupData = NULL;
                }
                else
                {
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = pRef->pExternalCleanupList[iEntryIndex2+1].pCleanupCb;
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupData = pRef->pExternalCleanupList[iEntryIndex2+1].pCleanupData;
                }
            }
        }
    }

    // clear re-enter block
    pRef->bProcessingCleanupList = FALSE;
}

/*F********************************************************************************/
/*!
    \Function   _NetConnRequestAuthCode

    \Description
        Initiate async op for acquisition of auth code from sce servers

    \Input  *pRef            - state ref
    \Input  iUserIndex       - user index

    \Output
        int32_t             - <0=error, 0=retry (a request is already pending), >0=request issued

    \Version 04/09/2013 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _NetConnRequestAuthCode(NetConnRefT *pRef, int32_t iUserIndex)
{
    SceNpOnlineId onlineId;
    int32_t iRet = NetConnStatus('soid', iUserIndex, &onlineId, sizeof(onlineId));

    if (iRet >= 0)
    {
        if (pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState != NETCONN_NP_REQUEST_STATE_INPROG)
        {
            SceNpAuthCreateAsyncRequestParameter asyncParam;
            memset(&asyncParam, 0, sizeof(asyncParam));
            asyncParam.size = sizeof(asyncParam);
            asyncParam.threadPriority = pRef->iUserThreadPrio;
            asyncParam.cpuAffinityMask = pRef->CpuAffinity;

            iRet = pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId = _NetConnSaveErr(pRef, sceNpAuthCreateAsyncRequest(&asyncParam));

            if (iRet > 0)
            {
                SceNpAuthGetAuthorizationCodeParameter authParam;

                memset(&authParam, 0, sizeof(authParam));
                memset(&pRef->NpUsers.aAuthCode[iUserIndex].authCode, 0, sizeof(pRef->NpUsers.aAuthCode[iUserIndex].authCode));

                authParam.size = sizeof(authParam);
                authParam.pOnlineId = &onlineId;
                authParam.pClientId = &pRef->NpClientId;
                authParam.pScope = NETCONNPS4_SCOPE_FOR_AUTH_CODE;

                iRet = _NetConnSaveErr(pRef, sceNpAuthGetAuthorizationCode(pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId, &authParam, &pRef->NpUsers.aAuthCode[iUserIndex].authCode, &pRef->NpUsers.aAuthCode[iUserIndex].iIssuerId ));
                if (iRet == 0)
                {
                    pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_INPROG;
                }
                else
                {
                    NetPrintf(("netconnps4: _NetConnRequestAuthCode sceNpAuthGetAuthorizationCode() failed, err = %s \n", DirtyErrGetName(iRet)));
                    pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_FAILED;
                }
            }
            else if (iRet == 0)
            {
                NetPrintf(("netconnps4: _NetConnRequestAuthCode sceNpAuthCreateAsyncRequest() failed with result 0, most likely the required PRX is not loaded \n"));
                pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_FAILED; 
            }
            else
            {
                NetPrintf(("netconnps4: _NetConnRequestAuthCode sceNpAuthCreateAsyncRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
                pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_FAILED;
            }
        }
        else
        {
            //in progress
            iRet = 0;
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnRequestAuthCode invalid user, err = %d \n", iRet));
    }

    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCleanupAuthCodeRequest

    \Description
        Abort async op if in progress, and delete request.

    \Input *pRef            - state ref
    \Input iUserIndex       - user index

    \Version 04/09/2013 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnCleanupAuthCodeRequest(NetConnRefT *pRef, int32_t iUserIndex)
{
    //be sure the user index makes sense
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS)
    {
        int32_t iRet;
        //if there is an operation in progress we should abort the request first
        if (pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            iRet = sceNpAuthAbortRequest(pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId);
            if (iRet != 0)
            {
                NetPrintf(("netconnps4: _NetConnCleanupAuthCodeRequest, sceNpAuthAbortRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
            }
            pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_INVALID;
        }
        //delete the request
        if (pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId != 0)
        {
            iRet = sceNpAuthDeleteRequest(pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId);
            if (iRet != 0)
            {
                NetPrintf(("netconnps4: _NetConnCleanupAuthCodeRequest, sceNpAuthDeleteRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
            }
            pRef->NpUsers.aAuthCode[iUserIndex].iAuthCodeRequestId = 0;
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnCleanupAuthCodeRequest, invalid user id %d\n", iUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateAuthCodeRequests

    \Description
        Poll for completion of ticket request

    \Input *pRef            - state ref

    \Version 04/09/2013 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnUpdateAuthCodeRequests(NetConnRefT *pRef)
{
    //loop over the players, see if any of them have an outstanding request
    for (int32_t i = 0; i < NETCONN_MAXLOCALUSERS; i++)
    {
        if (pRef->NpUsers.aAuthCode[i].eAuthCodeState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            int32_t iResult = 0;
            int32_t iRet = sceNpAuthPollAsync(pRef->NpUsers.aAuthCode[i].iAuthCodeRequestId, &iResult);
            if (iResult >= 0)
            {
                if (iRet == SCE_NP_LOOKUP_POLL_ASYNC_RET_RUNNING)
                {
                    //continue waiting
                }
                else if (iRet == SCE_NP_LOOKUP_POLL_ASYNC_RET_FINISHED)
                {
                    pRef->NpUsers.aAuthCode[i].eAuthCodeState = NETCONN_NP_REQUEST_STATE_VALID;

                    if (pRef->bEnviRequest)
                    {
                        pRef->uPlatEnv = NETCONN_PLATENV_PROD;

                        /*  IssuerID == 1    --> SPINT Users
                            IssuerID == 8    --> PROD_QA Users
                            IssuerID == 256  --> NP Users */
                        if (pRef->NpUsers.aAuthCode[i].iIssuerId == 1)
                        {
                            pRef->uPlatEnv = NETCONN_PLATENV_TEST;
                            NetPrintf(("netconnps4: Platform Environment set to TEST.\n "));
                        }
                        else if (pRef->NpUsers.aAuthCode[i].iIssuerId == 8)
                        {
                            pRef->uPlatEnv = NETCONN_PLATENV_CERT;
                            NetPrintf(("netconnps4: Platform Environment set to CERT.\n"));
                        }
                        else if (pRef->NpUsers.aAuthCode[i].iIssuerId == 256)
                        {
                            NetPrintf(("netconnps4: Platform Environment set to PROD.\n"));
                        }
                        else
                        {
                            NetPrintf(("netconnps4: _NetConnUpdateAuthCodeRequests() Unknow Issuer Id. Environment defaulted to PROD.\n"));
                        }
                    }

                    pRef->bEnviRequest = FALSE;
                }
                else
                {
                    NetPrintf(("netconnps4: _NetConnUpdateAuthCodeRequests, sceNpAuthPollAsync() failed, err = %s \n", DirtyErrGetName(iRet)));
                    pRef->NpUsers.aAuthCode[i].eAuthCodeState = NETCONN_NP_REQUEST_STATE_FAILED;

                    //Need to reset the state in case of a failure when requesting the environment
                    if (pRef->bEnviRequest == TRUE)
                    {
                        pRef->bEnviRequest = FALSE;
                        _NetConnUpdateConnStatus(pRef, '-err');
                        pRef->eState = NetConnRefT::ST_INIT;
                    }
                }
            }
            else
            {
                NetPrintf(("netconnps4: _NetConnUpdateAuthCodeRequests, sceNpAuthPollAsync() failed iResult, err = %s \n", DirtyErrGetName(iResult)));
                pRef->NpUsers.aAuthCode[i].eAuthCodeState = NETCONN_NP_REQUEST_STATE_FAILED;

                //Need to reset the state in case of a failure when requesting the environment
                if (pRef->bEnviRequest == TRUE)
                {
                    pRef->bEnviRequest = FALSE;
                    _NetConnUpdateConnStatus(pRef, '-err');
                    pRef->eState = NetConnRefT::ST_INIT;
                }
            }

            if (pRef->NpUsers.aAuthCode[i].eAuthCodeState != NETCONN_NP_REQUEST_STATE_INPROG)
            {
                _NetConnCleanupAuthCodeRequest(pRef, i);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnReadAuthCode

    \Description
        Fills pBuf with auth code for target user, if available.

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index
    \Input pBuf         - buffer to write auth code
    \Input iBufSize     - size of supplied buffer, at this time sizeof(SceNpAuthorizationCode) == 136

    \Output
        int32_t         -  0:"try again"; <0:error; >0:number of bytes copied into pBuf

    \Version 04/10/2013 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _NetConnReadAuthCode(NetConnRefT *pRef, int32_t iUserIndex, void *pBuf, int32_t iBufSize)
{
    int32_t iRet = 0;
    //be sure the user index makes sense
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS)
    {
        if (pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState == NETCONN_NP_REQUEST_STATE_VALID)
        {
            //be sure we have enough buffer space
            if(pBuf == NULL)
            {
                //Returns the size of the ticket (to be consistent with other platforms)
                iRet = sizeof(SceNpAuthorizationCode);
            }
            else if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(SceNpAuthorizationCode)))
            {
                memset(pBuf, 0, iBufSize);
                ds_memcpy(pBuf, &pRef->NpUsers.aAuthCode[iUserIndex].authCode, sizeof(SceNpAuthorizationCode));
                iRet = sizeof(SceNpAuthorizationCode);
            }
            else
            {
                NetPrintf(("netconnps4: NetConnStatus 'tick' called with invalid buffer space.\n"));
                iRet = -1;
            }
        }
        else if (pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            iRet = 0;
        }
        else if (pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState == NETCONN_NP_REQUEST_STATE_INVALID)
        {
            //the user should have used the 'tick' control but we can kick off the first time for them
            iRet = _NetConnRequestAuthCode(pRef, iUserIndex);
        }
        else
        {
            //its either in a failed state or a state that is unknown, there should already have been a print about this transition, lets not spam
            iRet = -1;
        }
    }
    else
    {
        NetPrintf(("netconnps4: NetConnStatus 'tick' called with invalid user index %d", iUserIndex));
        iRet = -1;
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnNpAsyncRequest

    \Description
        Initiate async request against NP services

    \Input *pRef            - state ref
    \Input iUserIndex       - user index
    \Input pRequestData     - pointer to the NetConnAsyncRequestDataT struct to track this request
    \Input pRequestCallback - pointer to a NetConnAsyncRequestT callback which is called to initiate the actual SceNp request

    \Output
        int32_t             - <0=error, 0=retry (a request is already pending), >0=request issued

    \Version 06/20/2013 (mcorcoran) - used code from _NetConnRequestAuthCode
*/
/********************************************************************************F*/
static int32_t _NetConnNpAsyncRequest(NetConnRefT *pRef, int32_t iUserIndex, NetConnAsyncRequestDataT *pRequestData, NetConnAsyncRequestT pRequestCallback)
{
    SceNpOnlineId onlineId;
    int32_t iRet = NetConnStatus('soid', iUserIndex, &onlineId, sizeof(onlineId));
    if (iRet >= 0)
    {
        if (pRequestData->eRequestState != NETCONN_NP_REQUEST_STATE_INPROG)
        {
            SceNpCreateAsyncRequestParameter asyncParam;
            memset(&asyncParam, 0, sizeof(asyncParam));
            asyncParam.size = sizeof(asyncParam);
            asyncParam.threadPriority = pRef->iUserThreadPrio;
            asyncParam.cpuAffinityMask = pRef->CpuAffinity;

            iRet = pRequestData->iRequestId = _NetConnSaveErr(pRef, sceNpCreateAsyncRequest(&asyncParam));

            if (iRet > 0)
            {
                iRet = _NetConnSaveErr(pRef, pRequestCallback(pRef, iUserIndex, onlineId, pRequestData));
                if (iRet == 0)
                {
                    pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_INPROG;
                }
                else
                {
                    NetPrintf(("netconnps4: _NetConnNpAsyncRequest call to pRequestCallback failed, err = %s \n", DirtyErrGetName(iRet)));
                    pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_FAILED;
                }
            }
            else
            {
                NetPrintf(("netconnps4: _NetConnNpAsyncRequest sceNpCreateAsyncRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
                pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_FAILED;
            }
        }
        else
        {
            //in progress
            iRet = 0;
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnNpAsyncRequest invalid user, err = %d \n", iRet));
    }

    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCleanupNpRequest

    \Description
        Abort async op if in progress, and delete request.

    \Input *pRef            - state ref
    \Input iUserIndex       - user index
    \Input pRequestData     - associated NetConnAsyncRequestDataT struct for the current request

    \Version 06/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static void _NetConnCleanupNpRequest(NetConnRefT *pRef, int32_t iUserIndex, NetConnAsyncRequestDataT *pRequestData)
{
    //be sure the user index makes sense
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS)
    {
        int32_t iRet;
        //if there is an operation in progress we should abort the request first
        if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            iRet = sceNpAbortRequest(pRequestData->iRequestId);
            if (iRet != 0)
            {
                NetPrintf(("netconnps4: _NetConnCleanupNpRequest, sceNpAbortRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
            }
            pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;
        }
        else if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_FAILED)
        {
            pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;
        }

        //delete the request
        if (pRequestData->iRequestId != 0)
        {
            iRet = sceNpDeleteRequest(pRequestData->iRequestId);
            if (iRet != 0)
            {
                NetPrintf(("netconnps4: _NetConnCleanupNpRequest, sceNpDeleteRequest() failed, err = %s \n", DirtyErrGetName(iRet)));
            }
            pRequestData->iRequestId = 0;
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnCleanupNpRequest, invalid user id %d\n", iUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateNpRequests

    \Description
        Poll for completion of ticket request

    \Input *pRef                 - state ref
    \Input *pRequestDataArray    - pointer to the begining of the per-user array of requests
    \Input  iSizeOfArrayElement  - the size of the element in the array pointed to by pRequestDataArray

    \Version 06/20/2013 (mcorcoran) - used code from _NetConnUpdateAuthCodeRequest
*/
/********************************************************************************F*/
static void _NetConnUpdateNpRequests(NetConnRefT *pRef, void *pRequestDataArray, int32_t iSizeOfArrayElement)
{
    //loop over the players, see if any of them have an outstanding request
    for (int32_t i = 0; i < NETCONN_MAXLOCALUSERS; i++)
    {
        // find the NetConnAsyncRequestDataT at index (i), taking into account proper byte alginment
        NetConnAsyncRequestDataT *pRequestData = (NetConnAsyncRequestDataT*)((char*)pRequestDataArray + (iSizeOfArrayElement * i));
        if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            int32_t iResult = 0;
            int32_t iRet = sceNpPollAsync(pRequestData->iRequestId, &iResult);
            if (iResult >= 0)
            {
                if (iRet == SCE_NP_POLL_ASYNC_RET_RUNNING)
                {
                    //continue waiting
                }
                else if (iRet == SCE_NP_POLL_ASYNC_RET_FINISHED)
                {
                    pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_VALID;
                }
                else
                {
                    NetPrintf(("netconnps4: _NetConnUpdateNpRequests, sceNpPollAsync() failed, err = %s \n", DirtyErrGetName(iRet)));
                    pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_FAILED;
                }
            }
            else if (iResult == SCE_NP_ERROR_AGE_RESTRICTION)
            {
                // age restricted
                NetPrintf(("netconnps4: _NetConnUpdateNpRequests, sceNpPollAsync() underage user detected, err = SCE_NP_ERROR_AGE_RESTRICTION \n"));
                pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_UNDERAGE;
            }
            else
            {
                NetPrintf(("netconnps4: _NetConnUpdateNpRequests, sceNpPollAsync() failed iResult, err = %s \n", DirtyErrGetName(iResult)));
                pRequestData->eRequestState = NETCONN_NP_REQUEST_STATE_FAILED;
            }

            if (pRequestData->eRequestState != NETCONN_NP_REQUEST_STATE_INPROG)
            {
                _NetConnCleanupNpRequest(pRef, i, pRequestData);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnReadAsyncResponse

    \Description
        Fills pBuf with the data appropriate to the request type

    \Input *pRef                - NetConnRefT reference
    \Input iUserIndex           - player index
    \Input pBuf                 - buffer to copy the parental control settings to (should be a SceNpParentalControlInfo*)
    \Input iBufSize             - size of supplied buffer
    \Input pRequestData         - associated NetConnAsyncRequestDataT struct for the current request
    \Input pRequestCallback     - the callback to initiate the SceNp async request
    \Input pResponseCallback    - the callback to read the SceNp async response

    \Output
        int32_t                 -  0:"try again"; <0:error; >0:number of bytes copied into pBuf

    \Version 06/20/2013 (mcorcoran) - used code from _NetConnReadAuthCode
*/
/********************************************************************************F*/
static int32_t _NetConnReadAsyncResponse(NetConnRefT *pRef, int32_t iUserIndex, void *pBuf, int32_t iBufSize, NetConnAsyncRequestDataT *pRequestData, NetConnAsyncRequestT pRequestCallback, NetConnAsyncResponseT pResponseCallback)
{
    int32_t iRet = 0;
    //be sure the user index makes sense
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS)
    {
        if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_VALID)
        {
            iRet = pResponseCallback(pRef, iUserIndex, pBuf, iBufSize);
        }
        else if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_INPROG)
        {
            iRet = 0;
        }
        else if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_INVALID)
        {
            if (pRequestCallback != NULL)
            {
                //the user should have used the related NetConnControl() selector, but we can kick off the first time for them
                iRet = _NetConnNpAsyncRequest(pRef, iUserIndex, pRequestData, pRequestCallback);
            }
            else
            {
                NetPrintf(("netconnps4: _NetConnReadAsyncResponse called without first initiating the request with NetConnControl(), user index %d\n", iUserIndex));
                iRet = -2;
            }
        }
        else if (pRequestData->eRequestState == NETCONN_NP_REQUEST_STATE_UNDERAGE)
        {
            iRet = SCE_NP_ERROR_AGE_RESTRICTION;
        }
        else
        {
            //its either in a failed state or a state that is unknown, there should already have been a print about this transition, lets not spam
            iRet = -1;
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnReadAsyncResponse called with invalid user index %d\n", iUserIndex));
        iRet = -1;
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnParentalControlsRequest

    \Description
        Callback for generic async request.  Initiates a request to get the parental control info.

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index
    \Input onlineId     - the SceNpOnlineId of the user making the request
    \Input pRequestData - the generic async request data

    \Output
        int32_t         - SCE error code

    \Version 06/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnParentalControlsRequest(NetConnRefT *pRef, int32_t iUserIndex, SceNpOnlineId onlineId, NetConnAsyncRequestDataT *pRequestData)
{
    return sceNpGetParentalControlInfo(pRequestData->iRequestId, &onlineId, &pRef->NpUsers.aParentalControls[iUserIndex].iAge, &pRef->NpUsers.aParentalControls[iUserIndex].parentalControls);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnParentalControlsResponse

    \Description
        Callback for reading the async response for the parental control info check.

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index
    \Input pBuf         - pointer to a SceNpParentalControlInfo struct
    \Input iBufSize     - size of the buffer pointed to by pBuf

    \Output
        int32_t         - number of bytes copied into pBuf, negative=error

    \Version 06/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnParentalControlsResponse(NetConnRefT *pRef, int32_t iUserIndex, void *pBuf, int32_t iBufSize)
{
    //be sure we have enough buffer space
    if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(SceNpParentalControlInfo)))
    {
        memset(pBuf, 0, iBufSize);
        ds_memcpy(pBuf, &pRef->NpUsers.aParentalControls[iUserIndex].parentalControls, sizeof(SceNpParentalControlInfo));
        return(sizeof(SceNpParentalControlInfo));
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnParentalControlsResponse called with invalid buffer space.\n"));
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnPsPlusRequest

    \Description
        Callback for generic async request.  Initiates a request to get the PS+ membership.

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index
    \Input onlineId     - the SceNpOnlineId of the user making the request
    \Input pRequestData - the generic async request data

    \Output
        int32_t         - SCE error code

    \Version 06/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnPsPlusRequest(NetConnRefT *pRef, int32_t iUserIndex, SceNpOnlineId onlineId, NetConnAsyncRequestDataT *pRequestData)
{
    return sceNpCheckPlus(pRequestData->iRequestId, &pRef->NpUsers.aPsPlus[iUserIndex].Param, &pRef->NpUsers.aPsPlus[iUserIndex].Result);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnPsPlusResponse

    \Description
        Callback for reading the async response for the PS+ membership check.

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index
    \Input pBuf         - pointer to a SceNpCheckPlusResult struct
    \Input iBufSize     - size of the buffer pointed to by pBuf

    \Output
        int32_t         - number of bytes copied into pBuf, negative=error

    \Version 06/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnPsPlusResponse(NetConnRefT *pRef, int32_t iUserIndex, void *pBuf, int32_t iBufSize)
{
    //be sure we have enough buffer space
    if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(SceNpCheckPlusResult)))
    {
        memset(pBuf, 0, iBufSize);
        ds_memcpy(pBuf, &pRef->NpUsers.aPsPlus[iUserIndex].Result, sizeof(SceNpCheckPlusResult));
        return(sizeof(SceNpCheckPlusResult));
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnPsPlusResponse called with invalid buffer space.\n"));
        return(-1);
    }
}

#ifdef DIRTYSDK_IEAUSER_ENABLED
/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateLocalUserNpSate

    \Description
        Update NP state of specified local user.

    \Input *pRef           - NetConnRefT reference
    \Input iLocalUserIndex - local user index

    \Version 18/01/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateLocalUserNpSate(NetConnRefT *pRef, int32_t iLocalUserIndex)
{
    int32_t iResult;

    if (pRef->bNpInit)
    {
        SceNpState sceOldNpState = pRef->NpUsers.aSceNpState[iLocalUserIndex];
        if ((iResult = _NetConnSaveErr(pRef, sceNpGetState(pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], &pRef->NpUsers.aSceNpState[iLocalUserIndex]))) == SCE_OK)
        {
            if (sceOldNpState != pRef->NpUsers.aSceNpState[iLocalUserIndex])
            {
                NetPrintf(("netconnps4: NP state of local user 0x%08x at index %d changed from %s to %s\n",
                    pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], iLocalUserIndex, _NpState[sceOldNpState], _NpState[pRef->NpUsers.aSceNpState[iLocalUserIndex]]));
            }

            if (pRef->NpUsers.aSceNpState[iLocalUserIndex] == SCE_NP_STATE_SIGNED_IN)
            {
                if ((iResult = _NetConnSaveErr(pRef, sceNpGetNpId(pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], &pRef->NpUsers.aSceNpid[iLocalUserIndex]))) != SCE_OK)
                {
                    NetPrintf(("netconnps4: sceNpGetNpId(0x%08x) failed (idx=%d); err=%s\n", pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], iLocalUserIndex, DirtyErrGetName(iResult)));
                }

                // refresh the parental control settings
                if (pRef->NpUsers.aParentalControls[iLocalUserIndex].data.eRequestState == NETCONN_NP_REQUEST_STATE_INVALID)
                {
                    if ((iResult = _NetConnNpAsyncRequest(pRef, iLocalUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iLocalUserIndex], _NetConnParentalControlsRequest)) < 0)
                    {
                        NetPrintf(("netconnps4: unable to refresh parental control settings for user (0x%08x, %d) failed; err=%s\n", pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], iLocalUserIndex, DirtyErrGetName(iResult)));
                    }
                }
            }
        }
        else
        {
            NetPrintf(("netconnps4: sceNpGetState(0x%08x) failed (idx=%d); err=%s\n", pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex], iLocalUserIndex, DirtyErrGetName(iResult)));
        }
    }
    else
    {
        NetPrintf(("netconnps4: _NetConnUpdateLocalUserNpSate() ignored because NP not yet initialized\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnAddLocalUser

    \Description
        Add a local user

    \Input *pCommonRef      - common module reference
    \Input iLocalUserIndex  - local user index
    \Input *pIEAUser        - pointer to IEAUser object

    \Version 05/16/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnAddLocalUser(NetConnCommonRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser)
{
    NetConnRefT *pRef = (NetConnRefT *)pCommonRef;

    if (pRef->aIEAUsers[iLocalUserIndex] == NULL)
    {
        pRef->aIEAUsers[iLocalUserIndex] = pIEAUser;
        pRef->aIEAUsers[iLocalUserIndex]->AddRef();

        if (pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex] == SCE_USER_SERVICE_USER_ID_INVALID)
        {
            // obtain the local user identifier from the IEAUser
            pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex] = (SceUserServiceUserId)pIEAUser->GetUserID();

            NetPrintf(("netconnps4: netconn user added at local user index %d --> local user id: 0x%08x\n",
                iLocalUserIndex, pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex]));

            // update NP state of this user
            _NetConnUpdateLocalUserNpSate(pRef, iLocalUserIndex);
        }
        else
        {
            NetPrintf(("netconnps4: failed to add IEAUser at index %d with local user id 0x%08x because aSceUserServiceUserId is out of sync.\n",
                iLocalUserIndex, (SceUserServiceUserId)pIEAUser->GetUserID()));
        }
    }
    else
    {
        NetPrintf(("netconnps4: failed to add IEAUser at index %d with local user id 0x%08x because entry already used with local user id 0x%08x\n",
            iLocalUserIndex, (SceUserServiceUserId)pIEAUser->GetUserID(), (SceUserServiceUserId)pRef->aIEAUsers[iLocalUserIndex]->GetUserID()));
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnQueryIEAUser

    \Description
        Finds the local user index for the passed in IEAUser UserId

    \Input *pRef    - module reference
    \Input id       - the userid used for the lookup

    \Output
        int32_t - index of user, negative=not found
*/
/********************************************************************************F*/
static int32_t _NetConnQueryIEAUser(NetConnRefT *pRef, SceUserServiceUserId id)
{
    int32_t iResult = -1;
    int32_t i = 0;

    for (; i < NETCONN_MAXLOCALUSERS; i++)
    {
        if (pRef->aIEAUsers[i] != NULL &&
            (SceUserServiceUserId)pRef->aIEAUsers[i]->GetUserID() == id)
        {
            iResult = i;
            break;
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnRemoveLocalUser

    \Description
        Remove a local user

    \Input *pCommonRef      - common module reference
    \Input iLocalUserIndex  - local user index
    \Input *pIEAUser        - pointer to IEAUser object

    \Version 05/16/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnRemoveLocalUser(NetConnCommonRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser)
{
    NetConnRefT *pRef = (NetConnRefT *)pCommonRef;

    if (pRef->aIEAUsers[iLocalUserIndex] != NULL)
    {
        if (pIEAUser->GetUserID() == pRef->aIEAUsers[iLocalUserIndex]->GetUserID())
        {
            if (pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex] != SCE_USER_SERVICE_USER_ID_INVALID)
            {
                NetPrintf(("netconnps4: netconn user removed at local user index %d --> local user id: 0x%08x\n",
                    iLocalUserIndex, pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex]));

                pRef->NpUsers.aSceUserServiceUserId[iLocalUserIndex] = SCE_USER_SERVICE_USER_ID_INVALID;

                memset(&pRef->NpUsers.aSceNpid[iLocalUserIndex], 0, sizeof(pRef->NpUsers.aSceNpid[iLocalUserIndex]));
                pRef->NpUsers.aSceNpState[iLocalUserIndex] = SCE_NP_STATE_UNKNOWN;

                _NetConnCleanupAuthCodeRequest(pRef, iLocalUserIndex);
                pRef->NpUsers.aAuthCode[iLocalUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_INVALID;

                _NetConnCleanupNpRequest(pRef, iLocalUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iLocalUserIndex]);
                pRef->NpUsers.aParentalControls[iLocalUserIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;

                _NetConnCleanupNpRequest(pRef, iLocalUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aPsPlus[iLocalUserIndex]);
                pRef->NpUsers.aPsPlus[iLocalUserIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;

                pRef->aSceNpOnlineIdCache[iLocalUserIndex].invalid = TRUE;

                pRef->aIEAUsers[iLocalUserIndex]->Release();
                pRef->aIEAUsers[iLocalUserIndex] = NULL;
            }
            else
            {
                NetPrintf(("netconnps4: failed to remove local user at index %d - invalid local user id in internal cache\n", iLocalUserIndex));
            }
        }
        else
        {
            NetPrintf(("netconnps4: failed to remove local user at index %d - local user ids do not match (0x%08x vs 0x%08x)\n",
                iLocalUserIndex, (SceUserServiceUserId)pIEAUser->GetUserID(), (SceUserServiceUserId)pRef->aIEAUsers[iLocalUserIndex]->GetUserID()));
        }
    }
    else
    {
        NetPrintf(("netconnps4: failed to remove IEAUSER at index %d - no local user at that spot\n", iLocalUserIndex));
    }
}
#endif   // DIRTYSDK_IEAUSER_ENABLED

/*F********************************************************************************/
/*!
    \Function   _NetConnUserIndexToSceUserServiceIndex

    \Description
        Translates the user level player index to the SceUserServiceLoginUserIdList's index

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index

    \Version 02/14/2013 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _NetConnUserIndexToSceUserServiceIndex(NetConnRefT *pRef, int32_t iUserIndex)
{
    int32_t iReturn = -1;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    iReturn = iUserIndex;
#else
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS && pRef->aUserIndex[iUserIndex] >= 0)
    {
        // Locate the matching player id
        for (int32_t iLocalIndex = 0; iLocalIndex < NETCONN_MAXLOCALUSERS; ++iLocalIndex)
        {
            if (pRef->aUserIndex[iUserIndex] == pRef->NpUsers.aSceUsers.userId[iLocalIndex])
            {
                iReturn = iLocalIndex;
                break;
            }
        }
    }
#endif

    return(iReturn);
}

/*F********************************************************************************/
/*!
    \Function    _NetConnSceUserServiceIndexToUserIndex

    \Description
        Translates the SceUserServiceLoginUserIdList's index to the user level player index

    \Input *pRef        - NetConnRefT reference
    \Input iLocalIndex  - SceUserServiceLoginUserIdList's index

    \Version 02/14/2013 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _NetConnSceUserServiceIndexToUserIndex(NetConnRefT *pRef, int32_t iLocalIndex)
{
    int32_t iReturn = -1;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    iReturn = iLocalIndex;
#else
    if (iLocalIndex >= 0 && iLocalIndex < NETCONN_MAXLOCALUSERS && pRef->NpUsers.aSceUsers.userId[iLocalIndex] >= 0)
    {
        // Locate the matching player id
        for (int32_t iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; ++iUserIndex)
        {
            if (pRef->aUserIndex[iUserIndex] == pRef->NpUsers.aSceUsers.userId[iLocalIndex])
            {
                iReturn = iUserIndex;
                break;
            }
        }
    }
#endif

    return(iReturn);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnCopyParam

    \Description
        Copy a command-line parameter. Copied from netconnps3.c

    \Input *pDst        - output buffer
    \Input iDstLen      - output buffer length
    \Input *pParamName  - name of parameter to check for
    \Input *pSrc        - input string to look for parameters in
    \Input *pDefault    - default string to use if paramname not found

    \Version 02/11/2013 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _NetConnCopyParam(char *pDst, int32_t iDstLen, const char *pParamName, const char *pSrc, const char *pDefault)
{
    int32_t iIndex;

    // find parameter
    if ((pSrc = strstr(pSrc, pParamName)) == NULL)
    {
        // copy in default
        ds_strnzcpy(pDst, pDefault, iDstLen);
        return(0);
    }

    // skip parameter name
    pSrc += strlen(pParamName);

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // copy the string
    for (iIndex = 0; (iIndex < iDstLen) && (pSrc[iIndex] != '\0') && (pSrc[iIndex] != ' '); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }

    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

#ifndef DIRTYSDK_IEAUSER_ENABLED
/*F********************************************************************************/
/*!
    \Function   _NetConnUserServicePriority

    \Description
        Copy a command-line parameter. Copied from netconnps3.c

    \Input *pRef        - NetConnRefT reference
    \Input *pParamName  - name of parameter to check for
    \Input *pSrc        - input string to look for parameters in
    \Input *iDefault    - default value to use if paramname not found

    \Version 02/12/2013 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _NetConnUserServicePriority(NetConnRefT *pRef, const char *pParamName, const char *pSrc, int32_t iDefault)
{
    int32_t iIndex;
    char strDst[100];

    // find parameter
    if ((pSrc = strstr(pSrc, pParamName)) == NULL)
    {
        // copy in default if not present
        pRef->iUserThreadPrio = iDefault;
        return(0);
    }

    // skip parameter name
    pSrc += strlen(pParamName);

    // copy the string
    for (iIndex = 0; (iIndex < (int32_t)sizeof(strDst)) && (pSrc[iIndex] != '\0') && (pSrc[iIndex] != ' '); iIndex++)
    {
        strDst[iIndex] = pSrc[iIndex];
    }

    // write null terminator and return number of bytes written
    strDst[iIndex] = '\0';

    //check to see if the names of the standard SCE_KERNEL_PRIO defines were passed, or a number;
    if (ds_stricmp(strDst, "SCE_KERNEL_PRIO_FIFO_DEFAULT") == 0)
    {
        pRef->iUserThreadPrio = SCE_KERNEL_PRIO_FIFO_DEFAULT;
    }
    else if (ds_stricmp(strDst, "SCE_KERNEL_PRIO_FIFO_HIGHEST") == 0)
    {
        pRef->iUserThreadPrio = SCE_KERNEL_PRIO_FIFO_HIGHEST;
    }
    else if (ds_stricmp(strDst, "SCE_KERNEL_PRIO_FIFO_LOWEST") == 0)
    {
        pRef->iUserThreadPrio = SCE_KERNEL_PRIO_FIFO_LOWEST;
    }
    else if ((iIndex = atoi(strDst)) != 0)
    {
        // Range check the number and apply the appropriate
        if (iIndex > SCE_KERNEL_PRIO_FIFO_LOWEST)
        {
            NetPrintf(("netconnps4: warning - invalid value parameter %s; priority too low, using SCE_KERNEL_PRIO_FIFO_LOWEST\n", pParamName));
            pRef->iUserThreadPrio = SCE_KERNEL_PRIO_FIFO_LOWEST;
        }
        else if (iIndex < SCE_KERNEL_PRIO_FIFO_HIGHEST)
        {
            NetPrintf(("netconnps4: warning - invalid value parameter %s; priority too high, using SCE_KERNEL_PRIO_FIFO_HIGHEST\n", pParamName));
            pRef->iUserThreadPrio = SCE_KERNEL_PRIO_FIFO_HIGHEST;
        }
        else
        {
            pRef->iUserThreadPrio = iIndex;
        }
    }
    else
    {
        // Nothing intelligible, use default
        NetPrintf(("netconnps4: warning - invalid startup parameter %s, using default value %d\n", pParamName, iDefault));
        pRef->iUserThreadPrio = iDefault;
    }

    return(pRef->iUserThreadPrio);
}
#endif


/*F********************************************************************************/
/*!
    \Function   _NetConnGetInterfaceType

    \Description
        Get interface type and return it to caller.

    \Input *pRef    - module state

    \Output
        uint32_t    - interface type bitfield (NETCONN_IFTYPE_*)

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetInterfaceType(NetConnRefT *pRef)
{
    uint32_t uIfType = NETCONN_IFTYPE_ETHER;
    union SceNetCtlInfo Info;
    int32_t iResult;

    // check for pppoe
    if (((iResult = sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_CONFIG, &Info)) == 0) && (Info.ip_config == SCE_NET_CTL_IP_PPPOE))
    {
        uIfType |= NETCONN_IFTYPE_PPPOE;
    }
    else
    {
        NetPrintf(("netconnps4: sceNetCtlGetInfo(IP_CONFIG) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    // check for wireless
    if ((iResult = sceNetCtlGetInfo(SCE_NET_CTL_INFO_DEVICE, &Info)) == 0)
    {
        switch (Info.device)
        {
            case SCE_NET_CTL_DEVICE_WIRELESS:
                uIfType |= NETCONN_IFTYPE_WIRELESS;
                NetPrintf(("netconnps4: sceNetCtlGetInfo() returned: wireless\n"));
                break;
            case SCE_NET_CTL_DEVICE_WIRED:
                NetPrintf(("netconnps4: sceNetCtlGetInfo() returned: wired\n"));
                break;
            default:
                NetPrintf(("netconnps4: warning - unsupported device type (%d) detected\n", Info.device));
                break;
        }
    }
    else
    {
        NetPrintf(("netconnps4: sceNetCtlGetInfo(DEVICE) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    return(uIfType);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnQueryNpUser

    \Description
        Returns the index of the first logged in user with Good Np Availability

    \Input *pRef    - module state

    \Output
        int32_t -    negative= if not found , else index of user

    \Version 07/10/2013 (tcho)
*/
/********************************************************************************F*/
static int32_t _NetConnQueryNpUser(NetConnRefT *pRef)
{
    int32_t iIndex;
    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if (pRef->NpUsers.aNpAvailability[iIndex].bDone == TRUE && pRef->NpUsers.aNpAvailability[iIndex].iResult == SCE_OK)
        {
            // return the first
            return(_NetConnSceUserServiceIndexToUserIndex(pRef, iIndex));
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnNpCheckNpAvailabilityError

    \Description
        Returns the index of the first logged in user with Good Np Availability

    \Input *pRef          - module state
    \Input  bErrorAtStart - if the error was detected at the start of the process


    \Version 06/17/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _NetConnNpCheckNpAvailabilityError(NetConnRefT *pRef, uint8_t bErrorAtStart)
{
    switch(pRef->NpUsers.aNpAvailability[0].iResult)
    {
        case SCE_NP_ERROR_OUT_OF_MEMORY:
        case SCE_NP_ERROR_ABORTED:
        case SCE_NP_ERROR_REQUEST_NOT_FOUND:
            NetPrintf(("netconnps4:%s() error: %s \n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-err');
            break;

        case SCE_NP_ERROR_SIGNED_OUT:
        case SCE_NP_ERROR_USER_NOT_FOUND:
        case SCE_NP_ERROR_NOT_SIGNED_UP:
        case SCE_NP_ERROR_LOGOUT:
            NetPrintf(("netconnps4:%s() account error: %s \n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-act');
            break;

        case SCE_NP_ERROR_AGE_RESTRICTION:
            NetPrintf(("netconnps4:%s() account error: %s \n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-uda');
            break;

        case SCE_NP_ERROR_LATEST_SYSTEM_SOFTWARE_EXIST:
        case SCE_NP_ERROR_LATEST_SYSTEM_SOFTWARE_EXIST_FOR_TITLE:
            NetPrintf(("netconnps4:%s() system update required: %s \n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-ups');
            break;

        case SCE_NP_ERROR_LATEST_PATCH_PKG_EXIST:
        case SCE_NP_ERROR_LATEST_PATCH_PKG_DOWNLOADED:
            NetPrintf(("netconnps4:%s() patch update required: %s \n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-upp');
            break;

        default:
            NetPrintf(("netconnps4:%s() unknown error: %s\n", bErrorAtStart == TRUE ? "_NetConnStartQueryNpUser" : "_NetConnQueryNpUserIdle", DirtyErrGetName(pRef->NpUsers.aNpAvailability[0].iResult)));
            _NetConnUpdateConnStatus(pRef, '-err');
            break;
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnStartQueryNpUser

    \Description
        Starts the query for Np Availability of each logged in user

    \Input *pRef    - module state

    \Output
        int32_t - negative= if unable to start , else 0

    \Version 04/22/2014 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _NetConnStartQueryNpUser(NetConnRefT *pRef)
{
    int32_t iResult = 0;
    bool bOneStarted = false;

    _NetConnUpdateConnStatus(pRef, '~cav');

    for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        SceNpCreateAsyncRequestParameter asyncParam;
        memset(&asyncParam, 0, sizeof(asyncParam));
        asyncParam.size = sizeof(asyncParam);
        asyncParam.threadPriority = pRef->iUserThreadPrio;
        asyncParam.cpuAffinityMask = pRef->CpuAffinity;
        int32_t iRequest = sceNpCreateAsyncRequest(&asyncParam);

        if (iRequest > 0)
        {
            SceNpOnlineId onlineId;

            memset(&onlineId, 0, sizeof(onlineId));
            if (!(NetConnStatus('soid', iIndex, &onlineId, sizeof(onlineId)) < 0))
            {
                if ((iResult = sceNpCheckNpAvailability(iRequest, &onlineId, NULL)) == SCE_OK)
                {
                    // Async processing started
                    NetPrintf(("netconnps4: async processing started index %d for sceNpCheckNpAvailability\n", iIndex));
                    pRef->NpUsers.aNpAvailability[iIndex].iRequestId = iRequest;
                    pRef->NpUsers.aNpAvailability[iIndex].iResult = 0;
                    pRef->NpUsers.aNpAvailability[iIndex].bDone = FALSE;
                    bOneStarted = true;
                }
                else
                {
                    pRef->NpUsers.aNpAvailability[iIndex].iRequestId = 0;
                    pRef->NpUsers.aNpAvailability[iIndex].iResult = iResult;
                    pRef->NpUsers.aNpAvailability[iIndex].bDone = TRUE;
                    sceNpDeleteRequest(iRequest);
                    NetPrintf(("netconnps4: user at index %d failed sceNpCheckNpAvailability with error %s\n", iIndex, DirtyErrGetName(iResult)));
                }
            }
            else
            {
                pRef->NpUsers.aNpAvailability[iIndex].iRequestId = 0;
                pRef->NpUsers.aNpAvailability[iIndex].iResult = SCE_NP_ERROR_SIGNED_OUT;
                pRef->NpUsers.aNpAvailability[iIndex].bDone = TRUE;
                sceNpDeleteRequest(iRequest);
                NetPrintf(("netconnps4: user at index %d failed to retrieve onlineId\n", iIndex));
            }

        }
        else
        {
            pRef->NpUsers.aNpAvailability[iIndex].iRequestId = 0;
            pRef->NpUsers.aNpAvailability[iIndex].iResult = iRequest;
            pRef->NpUsers.aNpAvailability[iIndex].bDone = TRUE;
            NetPrintf(("netconnps4: user at index %d failed to create a NpRequest\n", iIndex));
        }
    }

    // check to make sure at least one request started, otherwise fall into error state
    // if everything failed use the failure reason of the first user to set the state
    if (!bOneStarted)
    {
        _NetConnNpCheckNpAvailabilityError(pRef, TRUE);
        
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnQueryNpUserIdle

    \Description
        Checks the queries for Np Availability 

    \Input *pRef    - module state

    \Version 04/22/2014 (amakoukji)
*/
/********************************************************************************F*/
static void _NetConnQueryNpUserIdle(NetConnRefT *pRef)
{
    int32_t iResult = 0;
    int32_t iReturn = 0;
    bool bOneIncomplete = false;

    for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if(pRef->NpUsers.aNpAvailability[iIndex].bDone == TRUE)
        {
            continue;
        }

        iReturn = sceNpPollAsync(pRef->NpUsers.aNpAvailability[iIndex].iRequestId, &iResult);

        if (iReturn == SCE_NP_POLL_ASYNC_RET_RUNNING)
        {
            // still running
            bOneIncomplete = true;
        }
        else if (iReturn == SCE_NP_POLL_ASYNC_RET_FINISHED)
        {
            // request done, capture the result
            pRef->NpUsers.aNpAvailability[iIndex].bDone = TRUE;
            pRef->NpUsers.aNpAvailability[iIndex].iResult = iResult;
            sceNpDeleteRequest(pRef->NpUsers.aNpAvailability[iIndex].iRequestId);
        }
        else
        {
            // error fetching result
            NetPrintf(("netconnps4:_NetConnQueryNpUserIdle() error: %s\n", DirtyErrGetName(iReturn)));
            pRef->NpUsers.aNpAvailability[iIndex].bDone = TRUE;
            pRef->NpUsers.aNpAvailability[iIndex].iResult = iReturn;
            sceNpDeleteRequest(pRef->NpUsers.aNpAvailability[iIndex].iRequestId);
        }
    }

    if (!bOneIncomplete)
    {
        bool bAllFailed = true;
        // check if all the requests failed
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (pRef->NpUsers.aNpAvailability[iIndex].iResult >= 0)
            {
                bAllFailed = false;
            }
        }

        if(bAllFailed)
        {
            // if all the requests failed use the error code of the first one to determine the error state we will fall into
            _NetConnNpCheckNpAvailabilityError(pRef, FALSE);
        }
        else
        {
            // progress to the next state in the login flow
            _NetConnUpdateConnStatus(pRef, '~ens');
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnGetPlatformEnvironment

    \Description
        Uses auth code issuer to determine platform environment.

    \Input *pRef    - module state

    \Version 10/07/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetPlatformEnvironment(NetConnRefT *pRef)
{
    int32_t iRet = 0;
    int32_t iUserIndex;

    // set to invalid environment and pending state
    pRef->uPlatEnv = (unsigned)-1;
    
    pRef->bEnviRequest = TRUE;
    iUserIndex = _NetConnQueryNpUser(pRef);

    if (iUserIndex >= 0)
    {
        _NetConnUpdateConnStatus(pRef, '~enw');
        iRet = _NetConnRequestAuthCode(pRef, iUserIndex);
        if (iRet < 0)
        {
            NetPrintf(("netconnps4:_NetConnGetPlatformEnvironment() Failed to request Auth Code\n"));
            pRef->bEnviRequest = FALSE;
            _NetConnUpdateConnStatus(pRef, '-act');
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnInitParentalSettings

    \Description
        Loads parental settings for all users, called repeatedly to see if its completed.
        Transition pRef->uConnStatus to '~pse' and '-pse'

    \Input *pRef    - module state

    \Output
        uint32_t   - <0=error, 0=retry (a request is already pending), >0 parental settings are loaded

    \Version 08/15/2013 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnInitParentalSettings(NetConnRefT *pRef)
{
    int32_t iUserIndex;
    uint8_t bReady = TRUE;
    int32_t aPctlResult[NETCONN_MAXLOCALUSERS];

    _NetConnUpdateConnStatus(pRef, '~pse');
    memset(&aPctlResult, 0, sizeof(aPctlResult));

    // update any parental control requests, in progress
    _NetConnUpdateNpRequests(pRef, &pRef->NpUsers.aParentalControls, sizeof(pRef->NpUsers.aParentalControls[0]));

    //foreach logged in user verify we have parental info loaded
    for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        SceNpOnlineId onlineId;
        int32_t iRet = NetConnStatus('soid', iUserIndex, &onlineId, sizeof(onlineId));

        if (iRet >= 0)
        {
            SceNpParentalControlInfo parentalControlInfo;

            //0:"try again"; <0:error; >0:number of bytes copied into pBuf
            int32_t iRet = NetConnStatus('pctl', iUserIndex, &parentalControlInfo, sizeof(parentalControlInfo));
            aPctlResult[iUserIndex] = iRet;
            if (iRet == 0)
            {
                //this user isn't ready
                bReady = FALSE;
                continue;
            }
            else if (iRet == sizeof(parentalControlInfo))
            {
                //this user is ready, if all users are ready we will be ready
            }  

            if (pRef->uConnUserMode == 0)
            {
                // in this mode, if a single user's parental control indicate they are not allowed to online or is unretrieveable go into an error state
                if (iRet < 0)
                {
                    //an error has occured
                    NetPrintf(("netconnps4:_NetConnInitParentalSettings() 'pctl' failed.\n"));
                    _NetConnUpdateConnStatus(pRef, '-pse');
                    return(-1);
                }
                else if (iRet != sizeof(parentalControlInfo))
                {
                    //unexpected positive value, treat it as an error
                    NetPrintf(("netconnps4:_NetConnInitParentalSettings() 'pctl' unexpeceted return value.\n"));
                    _NetConnUpdateConnStatus(pRef, '-pse');
                    return(-2);            
                }
            }
        }
    }
    if (bReady == TRUE)
    {
        uint8_t bOnePassed = TRUE;

        if (pRef->uConnUserMode == 1)
        {
            // in this mode check that one user that had good np availability also has successfully retrieved parental control settings this test passes
            bOnePassed = FALSE;
            for (int32_t iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; ++iUserIndex)
            {
                if (pRef->NpUsers.aNpAvailability[iUserIndex].iResult >= 0 && aPctlResult[iUserIndex] == sizeof(SceNpParentalControlInfo))
                {
                    bOnePassed = TRUE;
                }
            }
        }

        if (bOnePassed)
        {
            return(1);
        }
        else
        {
            NetPrintf(("netconnps4:_NetConnInitParentalSettings() no user with NpAvailability was able to retrieve parental control data.\n"));
            _NetConnUpdateConnStatus(pRef, '-pse');
            return(-3); 
        }
    }
    return(0);
}


#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _NetConnPrintNpState

    \Description
        Util to print the np state of all players

    \Input *pRef    - module state

    \Version 12/13/2012 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnPrintNpState(NetConnRefT *pRef)
{
    int32_t iIndex;

    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        if (pRef->NpUsers.aSceNpState[iIndex] == SCE_NP_STATE_SIGNED_IN)
        {
            NetPrintf(("netconnps4: user #%d, %s\n", iIndex, pRef->NpUsers.aSceNpid[iIndex].handle.data));
        }
        else
        {
            NetPrintf(("netconnps4: user #%d, not signed in (%s)\n", iIndex, _NpState[pRef->NpUsers.aSceNpState[iIndex]]));
        }
    }
}
#endif

#ifndef DIRTYSDK_IEAUSER_ENABLED
/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateUserIndexes

    \Description
        Update the Dirtysock user indexes

    \Input *pRef    - NetConnRefT reference

    \Version 02/13/2013 (amakoukji)
*/
/********************************************************************************F*/
static void _NetConnUpdateUserIndexes(NetConnRefT *pRef)
{
    // Build the local user index table
    // It is necessary to maintain this level of indirection because
    // Sony's local index from sceUserServiceGetLoginUserIdList() will
    // collapse in such a way that later indexes always become vacant
    // first. For example, if the controllers at index 1 and 2 are
    // logged into the console, and controller 1 is logger out, controller
    // 2 will become tne new index 1.
    // From a DirtySDK point of view this is undesireable, since we want
    // a player at index 2 to STAY at index 2 if player 1 logs out.

    uint8_t bFound = FALSE;

    // first remove userId's that are no longer in Sony's user list.
    for (uint32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        bFound = FALSE;
        if (pRef->aUserIndex[iIndex] == -1)
        {
            continue;
        }
        for (uint32_t iSubIndex = 0; iSubIndex < NETCONN_MAXLOCALUSERS; ++iSubIndex)
        {
            if (pRef->NpUsers.aSceUsers.userId[iSubIndex] == pRef->aUserIndex[iIndex])
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            pRef->aUserIndex[iIndex] = -1;
        }
    }

    // then fill in new users in Sony's list from the front of the local index
    for (uint32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        bFound = FALSE;
        if (pRef->NpUsers.aSceUsers.userId[iIndex] == SCE_USER_SERVICE_USER_ID_INVALID)
        {
            break;
        }

        for (uint32_t iSubIndex = 0; iSubIndex < NETCONN_MAXLOCALUSERS; ++iSubIndex)
        {
            if (pRef->NpUsers.aSceUsers.userId[iIndex] == pRef->aUserIndex[iSubIndex])
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            // put the userId in the first available slot
            for (uint32_t iWriteIndex = 0; iWriteIndex < NETCONN_MAXLOCALUSERS; ++iWriteIndex)
            {
                if (pRef->aUserIndex[iWriteIndex] == -1)
                {
                    pRef->aUserIndex[iWriteIndex] = pRef->NpUsers.aSceUsers.userId[iIndex];
                    break;
                }
            }
        }
    }
    _NetConnInvalidateSceNpOnlineIdCache(pRef);
}


/*F********************************************************************************/
/*!
    \Function   _NetConnOnUserEvent

    \Description
        Handler for when a user logs in or out of the ps4 console

    \Input *pUserData  - NetConnRefT reference
    \Input *pUserEvent - the login event from the console

    \Version 02/13/2013 (amakoukji)
*/
/********************************************************************************F*/
static void _NetConnOnUserEvent(void *pUserData, const SceUserServiceEvent *pUserEvent)
{
    if (pUserEvent)
    {
        int32_t iResult;
        NetConnRefT *pRef = (NetConnRefT*)pUserData;

        // update the user id array
        if (sceUserServiceGetLoginUserIdList(&pRef->NpUsers.aSceUsers) < 0)
        {
            //Unable to retrieve login user id list shutting down netconn
            NetPrintf(("netconnps4: unable to retrieve LoginUserIdList. NetConn Shutting Down \n"));
            NetConnShutdown(0);
        }

        NetPrintf(("netconnps4: _NetConnOnUserEvent() for local user id 0x%08x\n", pUserEvent->userId));

        // Fetch the NpId and NpState
        // Sony notes sceNpRegisterStateCallback and sceNpGetState may be slow and blocking,
        // but we don't call it frequently (only when a controller is plugged or unplugged.
        // If it becomes a problem it should be moved to its own thread.
        for(int32_t iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
        {
            if (pRef->NpUsers.aSceUsers.userId[iUserIndex] < 0)
            {
                // There is no more user at this index, clear the  NpId and NpState
                memset(&pRef->NpUsers.aSceNpid[iUserIndex], 0, sizeof(pRef->NpUsers.aSceNpid[iUserIndex]));
                pRef->NpUsers.aSceNpState[iUserIndex] = SCE_NP_STATE_UNKNOWN;

                _NetConnCleanupAuthCodeRequest(pRef, iUserIndex);
                pRef->NpUsers.aAuthCode[iUserIndex].eAuthCodeState = NETCONN_NP_REQUEST_STATE_INVALID;

                _NetConnCleanupNpRequest(pRef, iUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iUserIndex]);
                pRef->NpUsers.aParentalControls[iUserIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;

                _NetConnCleanupNpRequest(pRef, iUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aPsPlus[iUserIndex]);
                pRef->NpUsers.aPsPlus[iUserIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;
            }
            else if ((iResult = _NetConnSaveErr(pRef, sceNpGetState(pRef->NpUsers.aSceUsers.userId[iUserIndex], &pRef->NpUsers.aSceNpState[iUserIndex]))) == SCE_OK)
            {
                if (pRef->NpUsers.aSceNpState[iUserIndex] == SCE_NP_STATE_SIGNED_IN)
                {
                    if ((iResult = _NetConnSaveErr(pRef, sceNpGetNpId(pRef->NpUsers.aSceUsers.userId[iUserIndex], &pRef->NpUsers.aSceNpid[iUserIndex]))) != SCE_OK)
                    {
                        NetPrintf(("netconnps4: sceNpGetNpId(%d) failed; err=%s\n", iUserIndex, DirtyErrGetName(iResult)));
                    }

                    // refresh the parental control settings
                    if (pRef->NpUsers.aParentalControls[iUserIndex].data.eRequestState == NETCONN_NP_REQUEST_STATE_INVALID)
                    {
                        if ((iResult = _NetConnNpAsyncRequest(pRef, iUserIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iUserIndex], _NetConnParentalControlsRequest)) < 0)
                        {
                            NetPrintf(("netconnps4: unable to refresh parental control settings for user (%d) failed; err=%s\n", iUserIndex, DirtyErrGetName(iResult)));
                        }
                    }
                }
            }
            else
            {
                NetPrintf(("netconnps4: sceNpGetState(%d) failed; err=%s\n", iUserIndex, DirtyErrGetName(iResult)));
            }
        }

        // Update the local indexes
        _NetConnUpdateUserIndexes(pRef);

        #if DIRTYCODE_LOGGING
        _NetConnPrintNpState(pRef);
        #endif
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function   _NetConnOnSystemEvent

    \Description
        Handler for when a systen event occurs on the ps4 console

    \Input *pUserData  - NetConnRefT reference
    \Input *pEvent      - the system event from the console

    \Version 02/13/2013 (amakoukji)
*/
/********************************************************************************F*/
static void _NetConnOnSystemEvent(void *pUserData, const SceSystemServiceEvent *pEvent)
{
    if (pEvent)
    {
        // we do nothing with this for now
        // we have this handler ready if the need arises
        // NOTE that SCE_SYSTEM_SERVICE_EVENT_ON_RESUME type
        // may require some processing, but if during the time
        // the application is suspended user events occur the
        // appropriate callback will be triggered after the app
        // is resumed
        // ALSO NOTE after tests on FIFA there was some odd
        // behavior when updating the user list from the ON_RESUME
        // notification. It is recommended that we wait until the
        // system hands out the UserService notifications before
        // updating. This can come several frames after the
        // ON_RESUME event is triggered.
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnSceNpStateCallback

    \Description
        Callback function provided to sceNpRegisterStateCallback on notification of
        NP state changes.

    \Input sceUserServiceUserId - the user id of the player whos state changed.
    \Input state                - the new state SCE_NP_STATE_UNKNOWN, SCE_NP_STATE_SIGNED_OUT, SCE_NP_STATE_SIGNED_IN
    \Input npId                 - the new npid, containing user name.
    \Input *pUserData           - module state

    \Version 12/13/2012 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnSceNpStateCallback(SceUserServiceUserId sceUserServiceUserId, SceNpState state, SceNpId *npId, void *pUserData)
{
    // this callback is hit when a user logs in or logs out from PSN,
    // but not if a controller is plugged or unplugged.

    // update the login state of the user with sceUserServiceUserId
    NetConnRefT *pRef    = (NetConnRefT*)pUserData;

    NetPrintf(("netconnps4: NP state event for 0x%08x\n", sceUserServiceUserId));

    if (pRef)
    {
        // find the index of the user
        uint32_t iIndex = 0;
        for (; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            #ifdef DIRTYSDK_IEAUSER_ENABLED
            if (pRef->NpUsers.aSceUserServiceUserId[iIndex] == sceUserServiceUserId)
            #else
            if (pRef->NpUsers.aSceUsers.userId[iIndex] == sceUserServiceUserId)
            #endif
            {
                break;
            }
        }

        // it is possible, in the event of a signout for the userId
        // to not be found if _NetConnOnUserEvent() already handled
        // the event
        // in that case, simply skip because the state is up-to-date
        if (iIndex < NETCONN_MAXLOCALUSERS)
        {
            #ifdef DIRTYSDK_IEAUSER_ENABLED
            _NetConnUpdateLocalUserNpSate(pRef, iIndex);
            #else
            pRef->NpUsers.aSceNpState[iIndex] = state;
            if (state == SCE_NP_STATE_SIGNED_IN)
            {
                int32_t iResult = 0;
                if ((iResult = _NetConnSaveErr(pRef, sceNpGetNpId(pRef->NpUsers.aSceUsers.userId[iIndex], &pRef->NpUsers.aSceNpid[iIndex]))) != SCE_OK)
                {
                    NetPrintf(("netconnps4: sceNpGetNpId(%d) failed; err=%s\n", iIndex, DirtyErrGetName(iResult)));
                }

                /* mclouatre jan 2013:  evaluate - why aren't we refreshing parental controls here */
            }
            #endif
        }

        _NetConnInvalidateSceNpOnlineIdCache(pRef);

        // queue the notification for callback at the next NetConnIdle()
        if (pRef->pNpStateCallback) // queue only if someone if listening
        {
            NetCritEnter(&pRef->Common.crit);
            NetConnNpStateCallbackDataT *pTmp = (NetConnNpStateCallbackDataT*)DirtyMemAlloc(sizeof(NetConnNpStateCallbackDataT), NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);
            if (pTmp)
            {
                memset(pTmp, 0, sizeof(NetConnNpStateCallbackDataT));
                pTmp->userId = sceUserServiceUserId;
                pTmp->state = state;
                NetConnNpStateCallbackDataT *pQueue = pRef->pNpStateCallbackDataQueue;

                // stick the data at the end of the queue
                if (pQueue == NULL)
                {
                    pRef->pNpStateCallbackDataQueue = pTmp;
                }
                else
                {
                    while (pQueue->pNext != NULL)
                    {
                        pQueue = pQueue->pNext;
                    }
                    pQueue->pNext = pTmp;
                }
            }
            NetCritLeave(&pRef->Common.crit);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function    _NetConnNpInit

    \Description
        Initialize Network Platform

    \Input *pRef    - netconn state

    \Output
        int32_t     - result of initialization

    \Version 12/12/2012 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _NetConnNpInit(NetConnRefT *pRef)
{
    int32_t iResult, iIndex;

    memset(pRef->NpUsers.aSceNpid, 0, sizeof(pRef->NpUsers.aSceNpid));
    memset(pRef->NpUsers.aSceNpState, 0, sizeof(pRef->NpUsers.aSceNpState));

    if (!pRef->bSecretSet)
    {
        // If the secret is not set, check to see if we are using the default title id
        // if we are using the default title id, use the default secret as well, otherwise
        // return an error. The game must set the appropriate title secret with NetConnControl('npsc', ...)
        if (ds_strnicmp(pRef->strTitleId, NETCONNPS4_SAMPLE_TITLE_ID, SCE_NP_TITLE_ID_LEN + 1) == 0)
        {
            // We are the default titleId, use the default title secret
            memset(&pRef->NpTitleSecret, 0, sizeof(pRef->NpTitleSecret));
            ds_memcpy(&pRef->NpTitleSecret.data, aDirtySdkSampleSecret, SCE_NP_TITLE_SECRET_SIZE);
        }
        else
        {
            NetPrintf(("netconnps4: NP Title Secret not set. Use NetConnControl('npsc', ...)\n"));
            return(-1);
        }
    }

    // Set the title id and secret
    SceNpTitleId NpTitleId;
    memset(&NpTitleId, 0, sizeof(NpTitleId));
    ds_strnzcpy(NpTitleId.id, pRef->strTitleId, sizeof(NpTitleId.id));

    // if the bNpToolKit is specified Setting Title Id will be the customers's responsibility
    // and state events must now be feed into DirtySDK through NetConnControl('stat') selector for proper operation
    if (!pRef->bNpToolKit)
    {
        iResult =  _NetConnSaveErr(pRef, sceNpSetNpTitleId(&NpTitleId, &pRef->NpTitleSecret));
        if (iResult != SCE_OK)
        {
            NetPrintf(("netconnps4: sceNpSetNpTitleId() failed; err=%s\n", DirtyErrGetName(iResult)));
            return(iResult);
        }

        iResult =  _NetConnSaveErr(pRef, sceNpRegisterStateCallback(_NetConnSceNpStateCallback, pRef));
        if (iResult != SCE_OK)
        {
            NetPrintf(("netconnps4: sceNpRegisterStateCallback() failed; err=%s\n", DirtyErrGetName(iResult)));
            return(iResult);
        }
    }

    pRef->bNpInit = TRUE;

    #ifdef DIRTYSDK_IEAUSER_ENABLED
    //get the current state, since registering the callback won't tell us where we are at
    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        if (pRef->NpUsers.aSceUserServiceUserId[iIndex] != SCE_USER_SERVICE_USER_ID_INVALID)
        {
            // update NP state of this user
            _NetConnUpdateLocalUserNpSate(pRef, iIndex);
        }
    }
    #else
    //get the current state, since registering the callback won't tell us where we are at
    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        if (pRef->NpUsers.aSceUsers.userId[iIndex] != SCE_USER_SERVICE_USER_ID_INVALID)
        {
            if ((iResult = _NetConnSaveErr(pRef, sceNpGetState(pRef->NpUsers.aSceUsers.userId[iIndex], &pRef->NpUsers.aSceNpState[iIndex]))) == SCE_OK)
            {
                if (pRef->NpUsers.aSceNpState[iIndex] == SCE_NP_STATE_SIGNED_IN)
                {
                    if ((iResult = _NetConnSaveErr(pRef, sceNpGetNpId(pRef->NpUsers.aSceUsers.userId[iIndex], &pRef->NpUsers.aSceNpid[iIndex]))) != SCE_OK)
                    {
                        NetPrintf(("netconnps4: sceNpGetNpId(%d) failed; err=%s\n", iIndex, DirtyErrGetName(iResult)));
                    }
                }

                /* mclouatre jan 2013:  evaluate - why aren't we refreshing parental controls here */
            }
            else
            {
                NetPrintf(("netconnps4: sceNpGetState(%d) failed; err=%s\n", iIndex, DirtyErrGetName(iResult)));
            }
        }
    }

    _NetConnUpdateUserIndexes(pRef);
    #endif

    #if DIRTYCODE_LOGGING
    NetPrintf(("netconnps4: Initial NP state:\n"));
    _NetConnPrintNpState(pRef);
    #endif

    return (iResult);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnNpTerm

    \Description
        Shut down Network Platform

    \Input *pRef    - netconn state

    \Version 12/12/2012 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnNpTerm(NetConnRefT *pRef)
{
    int32_t iResult;

    NetPrintf(("netconnps4: shutting down Network Platform\n"));

    if (!pRef->bNpToolKit)
    {
        // unregister npmanager callback
        // sony notes sceNpUnregisterStateCallback may be slow and blocking
        if ((iResult = sceNpUnregisterStateCallback()) != SCE_OK)
        {
            NetPrintf(("netconnps4: sceNpManagerUnregisterStateCallback() failed: err=%s\n", DirtyErrGetName(iResult)));
        }
    }

    pRef->bNpInit = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _NetConnConnect

    \Description
        Start connection process

    \Input *pRef     - netconn module state

    \Output
        int32_t     - 0 for success, negative for failure

    \Version 5/25/2013 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _NetConnConnect(NetConnRefT *pRef)
{
    int32_t iResult = 0;

    // start up network interface
    if ((iResult = SocketControl(NULL, 'conn', 0, NULL, NULL)) == 0)
    {
        // initialize Network Platform
        if ((iResult = _NetConnNpInit(pRef)) != 0)
        {
            NetPrintf(("netconnps4: failed to initialize network platform\n"));
            SocketControl(NULL, 'disc', 0, NULL, NULL);
            _NetConnUpdateConnStatus(pRef, '-sce');
            return(-1);
        }

        pRef->eState = NetConnRefT::ST_CONN;
        _NetConnUpdateConnStatus(pRef, '~con');
    }
    else
    {
        NetPrintf(("netconnps4: failed to initialize network stack\n"));
        _NetConnUpdateConnStatus(pRef, '-skt');
        return(-2);
    }
    return (iResult);
}

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uTick    - current tick counter

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uTick)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;
    int32_t iResult;

    // perform idle processing
    SocketControl(NULL, 'idle', uTick, NULL, NULL);

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

#ifdef DIRTYSDK_IEAUSER_ENABLED
    NetConnCommonUpdateLocalUsers((NetConnCommonRefT *)pRef);
#endif

    // waiting for external cleanup list to be empty
    if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
    {
        if (pRef->pExternalCleanupList[0].pCleanupCb == NULL)
        {
            pRef->eState = NetConnRefT::ST_INIT;
            if (pRef->bConnectDelayed)
            {
                pRef->bConnectDelayed = FALSE;

                // it is only when the external clean up list has been fully cleaned up that sceNpTerm() can be called
                _NetConnNpTerm(pRef);

                if (_NetConnConnect(pRef) < 0)
                {
                    // assumption: _NetConnConnect() has updated pRef->uConnStatus with appropriate error state
                    NetPrintf(("netconnps4: critical error - can't complete connection setup\n"));
                }
            }
        }
    }

    // wait for network active status
    if (pRef->eState == NetConnRefT::ST_CONN)
    {
        uint32_t uSocketStatus;
        uSocketStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        if (uSocketStatus == '+onl')
        {
            if (pRef->uConnStatus == '~con')
            {
                _NetConnStartQueryNpUser(pRef);
            }
            else if (pRef->uConnStatus == '~cav')
            {
                _NetConnQueryNpUserIdle(pRef);
            }
            else if (pRef->uConnStatus == '~ens')
            {
                _NetConnGetPlatformEnvironment(pRef);
            }
            else if (pRef->uConnStatus == '~enw')
            {
                //Update environment request which is part of an auth code request
                _NetConnUpdateAuthCodeRequests(pRef);

                if (pRef->uPlatEnv != (unsigned)(-1))
                {
                    _NetConnInitParentalSettings(pRef);
                }
            }
            else if (pRef->uConnStatus == '~pse')
            {
                if (_NetConnInitParentalSettings(pRef) > 0)
                {
                    _NetConnUpdateConnStatus(pRef, '+onl');
                }
            }

            if (pRef->uConnStatus == '+onl')
            {
                // discover upnp router information
                if (pRef->pProtoUpnp != NULL)
                {
                    if (pRef->iPeerPort != 0)
                    {
                        ProtoUpnpControl(pRef->pProtoUpnp, 'port', pRef->iPeerPort, 0, NULL);
                        ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'upnp', 0, NULL);
                    }
                    else
                    {
                        ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'dscg', 0, NULL);
                    }
                }

                pRef->eState = NetConnRefT::ST_IDLE;
            }
        }
        else if ((uSocketStatus >> 24) == '-')
        {
            _NetConnUpdateConnStatus(pRef, uSocketStatus);
        }
    }

    // update connection status while idle
    if (pRef->eState == NetConnRefT::ST_IDLE)
    {
        if (pRef->uConnStatus == '+onl')
        {
            // update any auth code requests
            _NetConnUpdateAuthCodeRequests(pRef);

            // update any parental control requests
            _NetConnUpdateNpRequests(pRef, &pRef->NpUsers.aParentalControls, sizeof(pRef->NpUsers.aParentalControls[0]));

            // update any PS+ requests
            _NetConnUpdateNpRequests(pRef, &pRef->NpUsers.aPsPlus, sizeof(pRef->NpUsers.aPsPlus[0]));
        }

        // update connection status if not already in an error state
        if ((pRef->uConnStatus >> 24) != '-')
        {
            uint32_t uSocketConnStat = SocketInfo(NULL, 'conn', 0, NULL, 0);
            
            if (pRef->uConnStatus != uSocketConnStat)
            {
                _NetConnUpdateConnStatus(pRef, uSocketConnStat);
            }
        }
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != NetConnRefT::ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = NetConnRefT::ST_IDLE;
    }

    //If the customer uses the NpToolKit sceNpCheckCallback will not be called
    if (pRef->bNpInit)
    {
        if (!pRef->bNpToolKit)
        {
            if ((iResult = sceNpCheckCallback()) != SCE_OK)
            {
                NetPrintf(("netconnps4: sceNpCheckCallback() failed; err=%s\n", DirtyErrGetName(iResult)));
            }
        }
    }

    //do we still have users logged in?
    iResult = NetConnQuery(NULL, NULL, 0);
    if (iResult < 0)
    {
        if (pRef->uConnStatus >> 24 == '+')
        {
            NetPrintf(("netconnps4: no user is signed in\n"));
            _NetConnUpdateConnStatus(pRef, '-act');
        }
    }

    // surface the NpStateCallbacks
    if (pRef->pNpStateCallback != NULL)
    {
        if (pRef->pNpStateCallbackDataQueue != NULL) // don't get the crit if we dont have to
        {
            NetCritEnter(&pRef->Common.crit);
            NetConnNpStateCallbackDataT *pTmp = pRef->pNpStateCallbackDataQueue;
            while (pTmp != NULL)
            {
                NetConnNpStateCallbackDataT *pNext = pTmp->pNext;

                // surface the callback
                pRef->pNpStateCallback(pTmp->userId, pTmp->state, pRef->pNpStateCallbackUserData);

                // free the memory
                DirtyMemFree(pTmp, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);

                // move to the next item
                pTmp = pNext;
            }

            //clear the list 
            pRef->pNpStateCallbackDataQueue = NULL;
            NetCritLeave(&pRef->Common.crit);
        }
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    NetConnStartup

    \Description
        Bring the network connection module to life. Creates connection with IOP
        resources and gets things ready to go. Puts all device drivers into "probe"
        mode so they look for appropriate hardware. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -noupnp                     - disable upnp support
            -nptoolkit                  - enable nptoolkit-compliant mode
            -titleid=titleidstr         - specify title id (default "NPXX51027_00")
            -userserviceprio=priostr    - specify user service priority (default "SCE_KERNEL_PRIO_FIFO_DEFAULT")
        \endverbatim

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef;
#ifdef DIRTYSDK_IEAUSER_ENABLED
    int32_t iIndex;
#else
    int32_t iResult = 0;
    SceUserServiceInitializeParams sceInitParams;
#endif

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // debug display of input params
    NetPrintf(("netconnps4: startup params='%s'\n", pParams));

    // initialize DirtyEventManager for UserService event processing;
    if (DirtyEventManagerInit() < 0)
    {
        NetPrintf(("netconnps4: SystemEventMessageDispatcher has not been initialized\n"));
        return(-1);
    }

    // common startup
#ifdef DIRTYSDK_IEAUSER_ENABLED
    if ((pRef = (NetConnRefT *)NetConnCommonStartup(sizeof(*pRef), _NetConnAddLocalUser, _NetConnRemoveLocalUser)) == NULL)
#else
    if ((pRef = (NetConnRefT *)NetConnCommonStartup(sizeof(*pRef))) == NULL)
#endif
    {
        return(-2);
    }

    pRef->eState = NetConnRefT::ST_INIT;
    pRef->iPeerPort = NETCONN_DEFAULT_UPNP_PORT;
    pRef->bEnviRequest = FALSE;
    pRef->bSecretSet = FALSE;
    pRef->CpuAffinity = SCE_KERNEL_CPUMASK_USER_ALL;
    pRef->uPlatEnv = (unsigned)(-1);
    pRef->bNpToolKit = FALSE;
    _NetConnInvalidateSceNpOnlineIdCache(pRef);

    // set hardcoded client id value
    ds_strnzcpy(pRef->NpClientId.id, NETCONNPS4_CLIENT_ID, sizeof(pRef->NpClientId.id));

    // allocate external cleanup list
    pRef->iExternalCleanupListMax = NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY;
    if ((pRef->pExternalCleanupList = (NetConnExternalCleanupEntryT *)DirtyMemAlloc(pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnps4: unable to allocate memory for initial external cleanup list\n"));
        NetConnShutdown(0);
        return(-3);
    }
    memset(pRef->pExternalCleanupList, 0, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

    // create network instance
    SocketCreate(10, 0, 0);

    // np toolkit
    if (strstr(pParams, "-nptoolkit"))
    {
        pRef->bNpToolKit = TRUE;
    }

    // create the upnp module
    if (!strstr(pParams, "-noupnp"))
    {
        pRef->pProtoUpnp = ProtoUpnpCreate();
    }

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-4);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnps4: unable to startup protossl\n"));
        return(-5);
    }

    _NetConnCopyParam(pRef->strTitleId, sizeof(pRef->strTitleId), "-titleid=", pParams, NETCONNPS4_SAMPLE_TITLE_ID);
    NetPrintf(("netconnps4: setting titleid=%s\n", pRef->strTitleId));

    DirtyEventManagerRegisterSystemService(&_NetConnOnSystemEvent, pRef);

#ifdef DIRTYSDK_IEAUSER_ENABLED
    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        pRef->NpUsers.aSceUserServiceUserId[iIndex] = SCE_USER_SERVICE_USER_ID_INVALID;
    }
#else
    DirtyEventManagerRegisterUserService(&_NetConnOnUserEvent, pRef);

    // start up the Sce User Service
    _NetConnUserServicePriority(pRef, "-userserviceprio=", pParams, SCE_KERNEL_PRIO_FIFO_DEFAULT);
    sceInitParams.priority = pRef->iUserThreadPrio;

    // initialize the user service if it has not already been
    // this should be redundant as in most cases an external library will have done this
    iResult = sceUserServiceInitialize(&sceInitParams);

    switch (iResult)
    {
        case SCE_OK:
            NetPrintf(("netconnps4: user service initialization complete with thread priority=%d\n", pRef->iUserThreadPrio));
            break;

        case SCE_USER_SERVICE_ERROR_ALREADY_INITIALIZED:
            pRef->bUserSessionPriorInitialization = TRUE;
            NetPrintf(("netconnps4: user service was already initialized\n"));
            break;

        default:
            NetConnShutdown(0);
            NetPrintf(("netconnps4: sceUserServiceInitialize failed; err=%s\n", DirtyErrGetName(iResult)));
            return(-6);
    }

    // Populate the list of logged in users
    memset(&pRef->NpUsers, 0, sizeof(pRef->NpUsers));
    if (sceUserServiceGetLoginUserIdList(&pRef->NpUsers.aSceUsers) < 0)
    {
        //Unable to retrieve login user id list shutting down netconn
        NetPrintf(("netconnps4: unable to retrieve LoginUserIdList. NetConn Shutting Down \n"));
        NetConnShutdown(0);
        return(-7);
    }

    // Populate the DirtySDK user index table
    memset(pRef->aUserIndex, -1, sizeof(pRef->aUserIndex));    // mclouatre - review - that sounds incorrect
    _NetConnUpdateUserIndexes(pRef);
#endif

    // add netconn task handle
    NetConnIdleAdd(_NetConnUpdate, pRef);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function   NetConnQuery

    \Description
        Returns the index of the given gamertag. If the gamertag is not
         present, find the index of the SceUserServiceUserId in the pList.
         Otherwise return the index of the first logged in user.

    \Input *pGamertag   - gamer tag
    \Input *pList       - SceUserServiceUserId to get index of
    \Input iListSize    - when pGamerTag

    \Output
        int32_t         - negative=gamertag not logged in, else index of user

    \Version 02/19/2013 (amakoukji)
*/
/********************************************************************************F*/
int32_t NetConnQuery(const char *pGamertag, NetConfigRecT *pList, int32_t iListSize)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    if (pGamertag != NULL)
    {
        // Find the matching gamertag
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (pRef->NpUsers.aSceNpState[iIndex] == SCE_NP_STATE_SIGNED_IN)
            {
                //special handling since SceNpOnlineId's .data field is not always terminated within its char array, but instead in the next .term character field
                if (ds_strnicmp(pRef->NpUsers.aSceNpid[iIndex].handle.data, pGamertag, SCE_NP_ONLINEID_MAX_LENGTH + 1) == 0)
                {
                    // We've found the right user, get their index
                    return(_NetConnSceUserServiceIndexToUserIndex(pRef, iIndex));
                }
            }
        }
    }
    else if (pList != NULL)
    {
        // find user based on SceUserServiceUserId
        int32_t iIndex = NetConnStatus('indx', *(SceUserServiceUserId*)pList, NULL, 0);
        if (NetConnStatus('npst', iIndex, NULL, 0) == SCE_NP_STATE_SIGNED_IN)
        {
            return(iIndex);
        }
    }
    else
    {
        // Return the index of the first online user
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (pRef->NpUsers.aSceNpState[iIndex] == SCE_NP_STATE_SIGNED_IN)
            {
                // We've found the right user, get their index
                return(_NetConnSceUserServiceIndexToUserIndex(pRef, iIndex));
            }
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses a
        configuration returned by NetConnQuery.

    \Input *pConfig - unused
    \Input *pOption - asciiz list of config parameters
                      "peerport=<port>" to specify peer port to be opened by upnp.
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    int32_t iResult = 0;
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    NetPrintf(("netconnps4: connecting...\n"));

#if DIRTYCODE_LOGGING
    {
        int32_t iInitialUserIndex = NetConnStatus('iind', 0, NULL, 0);
        if (iInitialUserIndex >= 0)
        {
            NetPrintf(("netconnps4: application was lauched by user at index %d\n", iInitialUserIndex));
        }
        else
        {
            NetPrintf(("netconnps4: no initial user detected - param.sfo was modified to allow the initial user to logout during your game\n"));
        }
    }
#endif

    pRef->uLastConnStatus = 0;

    // check connection options, if present
    if ((pRef->eState == NetConnRefT::ST_INIT) || (pRef->eState == NetConnRefT::ST_EXT_CLEANUP))
    {
        // check for connect options
        if (pOption != NULL)
        {
            const char *pOpt;

            // check for specification of peer port
            if ((pOpt = strstr(pOption, "peerport=")) != NULL)
            {
                pRef->iPeerPort = strtol(pOpt+9, NULL, 10);
            }
        }
        NetPrintf(("netconnps4: upnp peerport=%d %s\n",
            pRef->iPeerPort, (pRef->iPeerPort == NETCONN_DEFAULT_UPNP_PORT ? "(default)" : "(selected via netconnconnect param)")));

        // if needed, delay NetConnConnect until external cleanup phase completes
        if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
        {
            pRef->bConnectDelayed = TRUE;
            NetPrintf(("netconnps4: delaying completion of NetConnConnect() until external cleanup phase is completed\n"));
        }
        else
        {
            iResult = _NetConnConnect(pRef);
        }
    }
    else
    {
        NetPrintf(("netconnps4: NetConnConnect() ignored because already connected!\n"));
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    NetPrintf(("netconnps4: disconnecting...\n"));

    // early exit if NetConn is already in cleanup phase
    if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
    {
        NetPrintf(("netconnps4: external cleanup already in progress\n"));
        return(0);
    }

    if (pRef->eState == NetConnRefT::ST_INIT)
    {
        NetPrintf(("netconnps4: already disconnected\n"));
        return(0);
    }

    // bring down network interface
    SocketControl(NULL, 'disc', 0, NULL, NULL);

    // reset status
    pRef->eState = NetConnRefT::ST_INIT;
    pRef->uLastConnStatus = pRef->uConnStatus;  // save conn status
    pRef->uConnStatus = 0;

    // destroy any unfinished sceNpAsynrequests and clear control status
    for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        // psplus
        if (pRef->NpUsers.aPsPlus[iIndex].data.iRequestId != 0)
        {
            sceNpDeleteRequest(pRef->NpUsers.aPsPlus[iIndex].data.iRequestId);
        }
        pRef->NpUsers.aPsPlus[iIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;
        pRef->NpUsers.aPsPlus[iIndex].data.iRequestId = 0;

        // parental controls
        if (pRef->NpUsers.aParentalControls[iIndex].data.iRequestId != 0)
        {
            sceNpDeleteRequest(pRef->NpUsers.aParentalControls[iIndex].data.iRequestId);
        }
        pRef->NpUsers.aParentalControls[iIndex].data.eRequestState = NETCONN_NP_REQUEST_STATE_INVALID;
        pRef->NpUsers.aParentalControls[iIndex].data.iRequestId = 0;

        // npavailability
        if (pRef->NpUsers.aNpAvailability[iIndex].iRequestId != 0)
        {
            sceNpDeleteRequest(pRef->NpUsers.aNpAvailability[iIndex].iRequestId);
        }
        pRef->NpUsers.aNpAvailability[iIndex].iRequestId = 0;
        pRef->NpUsers.aNpAvailability[iIndex].iResult = 0;
        pRef->NpUsers.aNpAvailability[iIndex].bDone = FALSE;
    }

    // clear cached NP information
    //memset(&pRef->NpId, 0, sizeof(pRef->NpId));

    // delay call to _NetConnNpTerm() if need be
    if (pRef->iExternalCleanupListCnt != 0)
    {
        /*
        transit to ST_EXT_CLEANUP state
            Upon next call to NetConnConnect(), the NetConn module will be stuck in
            ST_EXT_CLEANUP state until all pending external module destructions
            are completed.
        */
        pRef->eState = NetConnRefT::ST_EXT_CLEANUP;
    }
    else
    {
        // shut down network platform
        _NetConnNpTerm(pRef);
    }

    // abort upnp operations
    if (pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpControl(pRef->pProtoUpnp, 'abrt', 0, 0, NULL);
    }
        
    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   NetConnControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input
    \Input *pValue2     - selector input

    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            cpaf: set cpu affinity for threads
            npsc: set up SceNpTitleSecret, pValue = data, iVlaue = sizeof data
            recu: stands for "register for external cleanup"; add a new entry to the external cleanup list
            tick: initiate a new auth code request for the user index specified in iValue; <0=error, 0=retry (a request is already pending), >0=request issued
            pctl: initiate a new parental control settings request for the user index specified in iValue; <0=error, 0=retry (a request is already pending), >0=request issued
            pspl: initiate a new PlayStation Plus membership check for user index iValue; iValue2=desired PS+ features to check; <0=error, 0=retry (a request is already pending), >0=request issued
            scid: set up SceNpClientId, pValue = data, iValue = sizeof data
            snam: set DirtyCert service name
            spam: set debug output verbosity (0...n)
            stat: feed user status events into DirtySock if NpToolKit is used by the customer
            ousr: one user mode, which determines whether all users must be old enough to go into an +onl state (iValue=0) or any one user must be old enough (iValue=1)
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 1.0 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnps4: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }

    // test for tick
    if (iControl == 'cpaf')
    {
        pRef->CpuAffinity = iValue;
        return 0;
    }
    if (iControl == 'npsc')
    {
        // iValue is supposed to contain the size of the buffer to which pValue points
        if ((iValue <= (int32_t)sizeof(pRef->NpTitleSecret.data)) && (pValue != NULL))
        {
            // fill buffer with NP Title Secret passed in pValue
            memset(&pRef->NpTitleSecret.data, 0, sizeof(pRef->NpTitleSecret.data));
            ds_memcpy(&pRef->NpTitleSecret.data, pValue, iValue);
            pRef->bSecretSet = TRUE;

            NetPrintMem(&pRef->NpTitleSecret, sizeof(pRef->NpTitleSecret), "NpTitleSecret");
        }
        else
        {
            NetPrintf(("netconnps4: 'npsc' selector ignored because input buffer is invalid.\n"));
            return(-1);
        }

        return(0);
    }
    // register for external cleanup
    if (iControl == 'recu')
    {
        return(_NetConnAddToExternalCleanupList(pRef, (NetConnExternalCleanupCallbackT)pValue, pValue2));
    }

    // feeds user status events into DirtySock if NpToolKit is used by the customer
    if (iControl == 'stat')
    {
        if (pValue != NULL && iValue == sizeof(NetConnNpUserStatusT))
        {
            if (pRef->bNpToolKit)
            {
                NetConnNpUserStatusT* userStatus = (NetConnNpUserStatusT *)pValue;
                _NetConnSceNpStateCallback(userStatus->iUserId, userStatus->state, &userStatus->npId, pRef);
                return(0);
            }
            else
            {
                NetPrintf(("netconnps4: NetConnStatus('stat') Ignored because bNpToolKit is not set to true.\n"));
                return(-2);
            }
        }
        else
        {
            NetPrintf(("netconnps4: NetConnStatus('stat') Invalid Arguments!\n"));
            return(-1);
        }
    }

    // request refresh of auth token
    if (iControl == 'tick')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iValue);
        return(_NetConnRequestAuthCode(pRef, iSceUserServiceIndex));
    }

    // set the "one user" mode
    if (iControl == 'ousr')
    {
        pRef->uConnUserMode = (uint32_t)iValue;
        return(0);
    }

    // request refresh of parental control settings
    if (iControl == 'pctl')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iValue);
        return(_NetConnNpAsyncRequest(pRef, iSceUserServiceIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iSceUserServiceIndex], _NetConnParentalControlsRequest));
    }
    
    // request refresh of PS+ membership
    if (iControl == 'pspl')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iValue);
        pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Param.size = sizeof(pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Param);

        #ifdef DIRTYSDK_IEAUSER_ENABLED
        pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Param.userId = pRef->NpUsers.aSceUserServiceUserId[iSceUserServiceIndex];
        #else
        pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Param.userId = pRef->aUserIndex[iSceUserServiceIndex];
        #endif

        pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Param.features = iValue2;
        return(_NetConnNpAsyncRequest(pRef, iSceUserServiceIndex, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aPsPlus[iSceUserServiceIndex], _NetConnPsPlusRequest));
    }
    if (iControl == 'scid')
    {
        // iValue is supposed to contain the size of the buffer to which pValue points
        if ((iValue <= (int32_t)sizeof(pRef->NpClientId.id)) && (pValue != NULL))
        {
            // fill buffer with NP Title Secret passed in pValue
            ds_strnzcpy(pRef->NpClientId.id, (char *)pValue, sizeof(pRef->NpClientId.id));
            NetPrintf(("netconnps4: NpClientId set to %s\n", pRef->NpClientId.id));
        }
        else
        {
            NetPrintf(("netconnps4: 'scid' selector ignored because input buffer is invalid.\n"));
            return(-1);
        }

        return(0);
    }

    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }

    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("netconnps4: changing debug level from %d to %d\n", pRef->Common.iDebugLevel, iValue));
        pRef->Common.iDebugLevel = iValue;
        return(0);
    }
    #endif

    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function    NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - status selector ('open', 'conn', 'onln')
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            acct: returns the account id (pBuf) given the user index (iData)
            addr: ip address of client
            bbnd: TRUE if broadband, else FALSE
            chat: TRUE if local user cannot chat, else FALSE
            chav: S_OK (0) for available user, -1 for invalid parameters, -2 if check is in progress, SCE_NP_ERROR code otherwise
            conn: connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            envi: EA back-end environment type in use (-1=not available, NETCONN_PLATENV_TEST, NETCONN_PLATENV_CERT, NETCONN_PLATENV_PROD)
            cuis: convert a user index (iData) to a system index, returns the index or a negative value for errors
            gust: unsupported
            csiu: convert a system index (iData) to a user index, returns the index or a negative value for errors
            gtag: zero=username of user at index [0-3] in iData returned in pBuf, negative=erro
            iind: returns the index of the user that launched the game
            indx: returns the index of the user with the specified Sony User ID
            iuid: return the SceUserServiceUserId for the user that launched the application
            lcon: (stands for Last CONNection status) connection status saved upon NetConnDisconnect()
            macx: MAC address of client (returned in pBuf)
            mask: returns the mask of users that are ONLINE, similar to the Xbox equivalent
            mgrp: returns memory group and user data pointer
            npid: Return the n'th users's SceNpId via pBuf, negative return=error
            npst: Return the n'th users's SceNpState, negative return=error
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            soid: return the SceNpOnlineId via pBuf, for the user at index in iData [0-3], negative means no user
            suid: return the SceUserServiceUserId for the user at index in iData [0-3], negative means no user
            tick: fills pBuf with authcode for user index iData, if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf. If pBuf is null it will return the ticket size
            titl: fills pBuf with the title id, pBuf must be at least 13 characters large
            pctl: fills pBuf with a SceNpParentalControlInfo structure containing the parental control settings for user index iData, if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf
            pspl: fills pBuf with a bool containing the result of the PlayStation Plus membership check for user index iData, if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf
            type: connection type: NETCONN_IFTYPE_* bitfield
            upnp: return protoupnp external port info, if available
            vers: return DirtySDK version
            lerr: return the last error that occurred as a result of an sceNp library call
            iusr: return the index to the IEAUser::GetUserId() passed via iData. Should be called from the thread that calls NetConnIdle(). negative=error
        \endverbatim

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // initialize output buffer
    if (pBuf != NULL)
    {
        memset(pBuf, 0, iBufSize);
    }

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }

    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYSDK_VERSION);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnps4: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    //Returns the Account Id
    if (iKind == 'acct')
    {
        SceNpOnlineId onlineId;
        int32_t iRet = 0;
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);

        if (iSceUserServiceIndex < 0 || iSceUserServiceIndex >= NETCONN_MAXLOCALUSERS || pBuf == NULL || iBufSize != sizeof(SceNpAccountId))
        {
            NetPrintf(("netconnps4: NetConnStatus('acct') Invalid arguments.\n"));
            return(-1);
        }

        memset(&onlineId, 0, sizeof(onlineId));
        memset(pBuf, 0, iBufSize);

        iRet = NetConnStatus('soid', iData, &onlineId, sizeof(onlineId));
        if (iRet != SCE_OK)
        {
            NetPrintf(("netconnps4: NetConnStatus('acct') Get OnlineId for User Index: %i failed.\n", iData));
            return(-2);
        }

        iRet = sceNpGetAccountId(&onlineId, (SceNpAccountId *)pBuf);
        if (iRet < 0)
        {
            NetPrintf(("netconnps4: NetConnStatus('acct') Get AccountId for Online Id: %s failed.\n", onlineId.data));
            return(-3);
        }

        return(0);
    }

    // return broadband (TRUE/FALSE)
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }

    // return ability of local user to chat
    if (iKind == 'chat')
    {
        int32_t iRet;
        SceNpParentalControlInfo parentalControlInfo;
        iRet = NetConnStatus('pctl', iData, &parentalControlInfo, sizeof(parentalControlInfo));
        if (iRet == sizeof(parentalControlInfo))
        {
            return(parentalControlInfo.chatRestriction ? TRUE : FALSE);
        }
        else if (iRet == 0)
        {
            // retrieval of parental control values is in progress
            return(-1);
        }
        else
        {
            // an error occured retrieving parental control values
            return(-2);
        }
    }

    // return ability of local user to share user generated content
    if (iKind == 'ugcn')
    {
        int32_t iRet;
        SceNpParentalControlInfo parentalControlInfo;
        iRet = NetConnStatus('pctl', iData, &parentalControlInfo, sizeof(parentalControlInfo));
        if (iRet == sizeof(parentalControlInfo))
        {
            return(parentalControlInfo.ugcRestriction ? TRUE : FALSE);
        }
        else if (iRet == 0)
        {
            // retrieval of parental control values is in progress
            return(-1);
        }
        else
        {
            // an error occured retrieving parental control values
            return(-2);
        }
    }

    // return ability of local user to perform online activities
    if (iKind == 'ctnt')
    {
        int32_t iRet;
        SceNpParentalControlInfo parentalControlInfo;
        iRet = NetConnStatus('pctl', iData, &parentalControlInfo, sizeof(parentalControlInfo));
        if (iRet == sizeof(parentalControlInfo))
        {
            return(parentalControlInfo.contentRestriction ? TRUE : FALSE);
        }
        else if (iRet == 0)
        {
            // retrieval of parental control values is in progress
            return(-1);
        }
        else
        {
            // an error occured retrieving parental control values
            return(-2);
        }
    }

    // connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }

    // return netconn user index from system index
    if (iKind == 'csiu')
    {
        return(_NetConnSceUserServiceIndexToUserIndex(pRef, iData));
    }

    // return system index from netconn user index
    if (iKind == 'cuis')
    {
        return(_NetConnUserIndexToSceUserServiceIndex(pRef, iData));
    }

    // EA back-end environment type in use
    if (iKind == 'envi')
    {
        if (pRef->uConnStatus == '+onl')
        {
            return(pRef->uPlatEnv);
        }

        return(-1);
    }

    // (stands for Last CONNection status) connection status saved upon NetConnDisconnect()
    if (iKind == 'lcon')
    {
        return(pRef->uLastConnStatus);
    }

    //npid: Return the n'th users's SceNpId
    if ((iKind == 'npid') && (iBufSize >= (int32_t)sizeof(SceNpId)) && (iData < NETCONN_MAXLOCALUSERS))
    {
        // Convert the user index to Sony's equivalent
        int32_t iLocalIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);

        if (iLocalIndex >= 0)
        {
            ds_memcpy(pBuf, &pRef->NpUsers.aSceNpid[iLocalIndex], sizeof(SceNpId));
            return(0);
        }

        return(-1);
    }

    // mgrp: return memory group and user data pointer
    if (iKind == 'mgrp')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(void *)))
        {
            *((void **)pBuf) = pRef->Common.pMemGroupUserData;
        }
        return(pRef->Common.iMemGroup);
    }

    //npst: Return the n'th users's SceNpState
    if ((iKind == 'npst') && (iData < NETCONN_MAXLOCALUSERS))
    {
        // Convert the user index to Sony's equivalent
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        if (iSceUserServiceIndex >= 0)
        {
            return(pRef->NpUsers.aSceNpState[iSceUserServiceIndex]);
        }

        return(-1);
    }

    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->uConnStatus == '+onl');
    }

    // return the SceNpOnlineId for the user at index in iData [0-3], negative means error
    if (iKind == 'soid')
    {
        if ((iData >= 0) && (iData < NETCONN_MAXLOCALUSERS) && (pBuf != NULL) && (iBufSize == sizeof(SceNpOnlineId)))
        {
            int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
            SceUserServiceUserId sceUserServiceUserId;
            #ifdef DIRTYSDK_IEAUSER_ENABLED
            sceUserServiceUserId = pRef->NpUsers.aSceUserServiceUserId[iSceUserServiceIndex];
            #else
            sceUserServiceUserId = pRef->aUserIndex[iSceUserServiceIndex];
            #endif

            // ignore if user is not SIGNEDIN with PSN
            if (pRef->NpUsers.aSceNpState[iSceUserServiceIndex] == SCE_NP_STATE_SIGNED_IN)
            {
                // check if the cache is dirty
                NetCritEnter(&pRef->Common.crit);
                if( pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].invalid == TRUE )
                {
                    //get a fresh value for onlineid
                    pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].result = _NetConnSaveErr(pRef, sceNpGetOnlineId(sceUserServiceUserId, &pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].onlineId));
                    pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].invalid = FALSE;
                }

                if(pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].result == SCE_OK)
                {
                    ds_memcpy(pBuf, &pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].onlineId, iBufSize);
                }
                else
                {
                    NetPrintf(("netconnps4: sceNpGetOnlineId(0x%08x) (idx=%d) failed with %s\n", sceUserServiceUserId, iData, DirtyErrGetName(pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].result)));
                }
                NetCritLeave(&pRef->Common.crit);

                return(pRef->aSceNpOnlineIdCache[iSceUserServiceIndex].result);
            }
        }
        else
        {
            NetPrintf(("netconnps4: NetConnStatus('soid') Invalid Arguments!\n"));
        }

        return(-1);
    }

    // return the SceUserServiceUserId for the user at index in iData [0-3], negative means no user
    if (iKind == 'suid')
    {
        if (iData >= 0 && iData < NETCONN_MAXLOCALUSERS)
        {
            #ifdef DIRTYSDK_IEAUSER_ENABLED
            return(pRef->NpUsers.aSceUserServiceUserId[iData]);
            #else
            return(pRef->aUserIndex[iData]);
            #endif
        }
        else
        {
            return(-1);
        }
    }

    // 'tick' - fills pBuf with authcode for user index iData, if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf
    if (iKind == 'tick')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        return(_NetConnReadAuthCode(pRef, iSceUserServiceIndex, pBuf, iBufSize));
    }

    if (iKind == 'pctl')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        if (iSceUserServiceIndex < 0 || iSceUserServiceIndex >= NETCONN_MAXLOCALUSERS || pBuf == NULL || iBufSize != sizeof(SceNpParentalControlInfo))
        {
            NetPrintf(("netconnps4: NetConnStatus('pctl') Invalid arguments.\n"));
            return(-1);
        }
        else
        {
            return(_NetConnReadAsyncResponse(pRef, iSceUserServiceIndex, pBuf, iBufSize, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[iSceUserServiceIndex], _NetConnParentalControlsRequest, _NetConnParentalControlsResponse));
        }
    }

    if (iKind == 'pspl')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        if (iSceUserServiceIndex < 0 || iSceUserServiceIndex >= NETCONN_MAXLOCALUSERS || pBuf == NULL || iBufSize != sizeof(pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Result.authorized))
        {
            NetPrintf(("netconnps4: NetConnStatus('pspl') Invalid arguments.\n"));
            return(-1);
        }
        else
        {
            SceNpCheckPlusResult plusResult;
            int32_t iRet = _NetConnReadAsyncResponse(pRef, iSceUserServiceIndex, &plusResult, sizeof(plusResult), (NetConnAsyncRequestDataT*)&pRef->NpUsers.aPsPlus[iSceUserServiceIndex], NULL, _NetConnPsPlusResponse);
            if (iRet > 0)
            {
                NetPrintf(("netconnps4: NetConnStatus('pspl') PS+ membership request for user (%d) returned (%s).\n", iSceUserServiceIndex, pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Result.authorized ? "true" : "false"));
                ds_memcpy(pBuf, &pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Result.authorized, sizeof(pRef->NpUsers.aPsPlus[iSceUserServiceIndex].Result.authorized));
            }
            else if (iRet == -2)
            {
                NetPrintf(("netconnps4: NetConnStatus('pspl') PS+ membership request for user (%d) must be restarted manually. Use NetConnControl('pspl').\n", iSceUserServiceIndex));
            }
            return(iRet);
        }
    }

    // return the SceUserServiceUserId for the user that launched the application
    if (iKind == 'iuid')
    {
        /*
        note: will always return -1 when game's param.sfo is modified for "allow the initial user to logout during your game"
              because sceUserServiceGetInitialUser() is known to be unusable/failing in that context.
        */

        SceUserServiceUserId userId = 0;
        if (sceUserServiceGetInitialUser(&userId) == SCE_OK)
        {
            return userId;
        }
        return(-1);
    }

    // return the index for the user that launched the application
    if (iKind == 'iind')
    {
        /*
        note: will always return -1 when game's param.sfo is modified for "allow the initial user to logout during your game"
              because sceUserServiceGetInitialUser() is known to be unusable/failing in that context.
        */

        SceUserServiceUserId userId = 0;
        if (sceUserServiceGetInitialUser(&userId) == SCE_OK)
        {
            for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
            {
                #ifdef DIRTYSDK_IEAUSER_ENABLED
                if (pRef->NpUsers.aSceUserServiceUserId[iIndex] == userId)
                #else
                if (pRef->aUserIndex[iIndex] == userId)
                #endif
                {
                    return(iIndex);
                }
            }
        }
        return(-1);
    }

    // returns the index of the user with the specified Sony User ID
    if (iKind == 'indx')
    {
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            #ifdef DIRTYSDK_IEAUSER_ENABLED
            if (pRef->NpUsers.aSceUserServiceUserId[iIndex] == iData)
            #else
            if (pRef->aUserIndex[iIndex] == iData)
            #endif
            {
                return(iIndex);
            }
        }

        return(-1);
    }

    // returns the mask of users that are ONLINE, similar to the Xbox equivalent
    if (iKind == 'mask')
    {
        int32_t iIndex;
        int32_t iSceUserServiceIndex = 0;
        int32_t iUserMask = 0x0000;

        for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iIndex);
            if (iSceUserServiceIndex >= 0 && pRef->NpUsers.aSceNpState[iSceUserServiceIndex] == SCE_NP_STATE_SIGNED_IN)
            {
                iUserMask |= (1 << iIndex);
            }
        }
        return(iUserMask);
    }

    // return interface type (more verbose)
    if (iKind == 'type')
    {
        return(_NetConnGetInterfaceType(pRef));
    }

    // unsupported
    if (iKind == 'gust')
    {
         /*on ps4, a guest user and a regular user are exact same except for one thing: guests have their local data deleted when they logout
         both the guest user and the regular user can be associated or not with a PSN account (signed-up or not signed-up)
         both the guest user and the regular user can be signed-in with PSN or signed-out from PSN
         there exist no api to determine if the user was created with “Create a User” or “Play as a Guest” in the system gui.
         */
        return(-1);
    }

    // zero=username of user at index [0-3] in iData returned in pBuf, negative=erro
    if (iKind == 'gtag')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        if (iSceUserServiceIndex < 0 || iSceUserServiceIndex >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            return(-1);
        }

        #ifdef DIRTYSDK_IEAUSER_ENABLED
        if (pRef->NpUsers.aSceUserServiceUserId[iData] == SCE_USER_SERVICE_USER_ID_INVALID)
        #else
        if (pRef->aUserIndex[iData] < 0)
        #endif
        {
            // No user at the specified index
            return(-1);
        }

        //special handling since SceNpOnlineId's .data field is not always terminated within its char array, but instead in the next .term character field
        if (iBufSize < (SCE_NP_ONLINEID_MAX_LENGTH + 1))
        {
            // Buffer too small
            return(-2);
        }
        if (pRef->NpUsers.aSceNpState[iSceUserServiceIndex] == SCE_NP_STATE_SIGNED_IN)
        {
            ds_strnzcpy((char*)pBuf, pRef->NpUsers.aSceNpid[iSceUserServiceIndex].handle.data, iBufSize);
            return(0);
        }

        // User Offline
        return(-3);
    }

    // return upnp addportmap info, if available
    if (iKind == 'upnp')
    {
        // if protoupnp is available, and we've added a port map, return the external port for the port mapping
        if ((pRef->pProtoUpnp != NULL) && (ProtoUpnpStatus(pRef->pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_ADDPORTMAP))
        {
            return(ProtoUpnpStatus(pRef->pProtoUpnp, 'extp', NULL, 0));
        }
    }

    if (iKind == 'titl')
    {
        if (iBufSize < SCE_NP_TITLE_ID_LEN + 1)
        {
            NetPrintf(("netconnps4: buffer too small to contain title id\n"));
            return(-1);
        }
        ds_strnzcpy((char*)pBuf, pRef->strTitleId, iBufSize);
        return(0);
    }

    if (iKind == 'lerr')
    {
        return(pRef->iLastNpError);
    }

    if (iKind == 'chav')
    {
        int32_t iSceUserServiceIndex = _NetConnUserIndexToSceUserServiceIndex(pRef, iData);
        if (iSceUserServiceIndex < 0 || iSceUserServiceIndex >= NETCONN_MAXLOCALUSERS)
        {
            NetPrintf(("netconnps4: NetConnStatus('chav') Invalid arguments.\n"));
            return(-1);
        }
        else
        {
            if (pRef->NpUsers.aNpAvailability[iSceUserServiceIndex].bDone == TRUE)
            {
                return(pRef->NpUsers.aNpAvailability[iSceUserServiceIndex].iResult);
            }
            else
            {
                NetPrintf(("netconnps4: NetConnStatus('chav') query in progress for user at index %d\n", iData));
                return(-2);
            }
        }
    }

    #ifdef DIRTYSDK_IEAUSER_ENABLED 
    if (iKind == 'iusr')
    {
        return(_NetConnQueryIEAUser(pRef, iData));
    }
    #endif

    // pass unrecognized options to SocketInfo
    return(SocketInfo(NULL, iKind, iData, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function    NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input  uShutdownFlags  - shutdown configuration flags

    \Output
        int32_t             - negative=error, else zero

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // make sure we've been started
    if (pRef == NULL)
    {
        return(-1);
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

    // make sure external cleanup list is empty before proceeding
    if (pRef->iExternalCleanupListCnt != 0)
    {
        // signal "try again"
        NetPrintf(("netconnps4: deferring shutdown while external cleanup list is not empty (%d items)\n", pRef->iExternalCleanupListCnt));
        return(NETCONN_ERROR_ISACTIVE);
    }

    //cleanup any auth code requests that might be in progress
    for (int32_t i = 0; i < NETCONN_MAXLOCALUSERS; i++)
    {
        _NetConnCleanupAuthCodeRequest(pRef, i);
        _NetConnCleanupNpRequest(pRef, i, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aParentalControls[i]);
        _NetConnCleanupNpRequest(pRef, i, (NetConnAsyncRequestDataT*)&pRef->NpUsers.aPsPlus[i]);
    }

    // destroy the upnp ref
    if (pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(pRef->pProtoUpnp);
        pRef->pProtoUpnp = NULL;
    }

    // shut down protossl
    ProtoSSLShutdown();

    // destroy the dirtycert module
    DirtyCertDestroy();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, pRef);

    // shut down Idle handler
    NetConnIdleShutdown();

    // check if a call to NetConnDisconnect() triggered the external cleanup mechanism before we reached this point
    if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
    {
        // note: it is only when the external clean up list has been fully cleaned up that sceNpTerm() can be called
        _NetConnNpTerm(pRef);
    }
    else
    {
        // disconnect the interfaces
        NetConnDisconnect();
    }

    // free the pending NpStateCallbacks
    pRef->pNpStateCallback = NULL;
    if (pRef->pNpStateCallbackDataQueue != NULL) 
    {
        NetCritEnter(&pRef->Common.crit);
        NetConnNpStateCallbackDataT *pTmp = pRef->pNpStateCallbackDataQueue;
        while (pTmp != NULL)
        {
            NetConnNpStateCallbackDataT *pNext = pTmp->pNext;

            // free the memory
            DirtyMemFree(pTmp, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);

            // move to the next item
            pTmp = pNext;
        }

        //clear the list 
        pRef->pNpStateCallbackDataQueue = NULL;
        NetCritLeave(&pRef->Common.crit);
    }

    // shutdown the network code
    SocketDestroy(0);

    // shutdown DirtyEventManager
    #ifndef DIRTYSDK_IEAUSER_ENABLED
    DirtyEventManagerUnregisterUserService(&_NetConnOnUserEvent);
    #endif
    DirtyEventManagerUnregisterSystemService(&_NetConnOnSystemEvent);
    DirtyEventManagerShutdown();

    // shutdown the ps4 user service if it was not initialized externally
    if (!pRef->bUserSessionPriorInitialization)
    {
        sceUserServiceTerminate();
    }

    // free external cleanup list
    if (pRef->pExternalCleanupList != NULL)
    {
        DirtyMemFree(pRef->pExternalCleanupList, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);
        pRef->pExternalCleanupList = NULL;
    }

    // common shutdown (must come last as this frees the memory)
    NetConnCommonShutdown(&pRef->Common);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    usleep(iMilliSecs*1000);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnRegisterNpStateCallback

    \Description
        Use this function to tell netconn about a local user that no longer exists

    \Input pCallback  - pointer to the function to call
    \Input pUserData  - user data to pass to the callback
    
    \Version 05/11/2015 (amakoukji)
*/
/*************************************************************************************************F*/
void NetConnRegisterNpStateCallback(NetConnNpStateCallbackT *pCallback, void *pUserData)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();
    if (pRef == NULL)
    {
        return;
    }
 
    pRef->pNpStateCallback = pCallback;
    pRef->pNpStateCallbackUserData = pUserData;
}

#ifdef DIRTYSDK_IEAUSER_ENABLED
/*F*************************************************************************************************/
/*!
    \Function    NetConnAddLocalUser

    \Description
        Use this function to tell netconn about a newly detected local user on the local console.

    \Input iLocalUserIndex  - index at which DirtySDK needs to insert this user it its internal user array
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnAddLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    return(NetConnCommonAddLocalUser(iLocalUserIndex, pLocalUser));
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnRemoveLocalUser

    \Description
        Use this function to tell netconn about a local user that no longer exists

    \Input iLocalUserIndex  - index in the internal DirtySDK user array at which the user needs to be cleared
    \Input pLocalUser       - pointer to associated IEAUser

    \Output
        int32_t             - 0 for success; negative for error

    \Version 01/16/2014 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnRemoveLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser)
{
    return(NetConnCommonRemoveLocalUser(iLocalUserIndex, pLocalUser));
}

#endif


