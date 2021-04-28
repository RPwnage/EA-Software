/*H*************************************************************************************************

    netconnxboxone.cpp

    Description:
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    Copyright (c) 2001-2012 Electronic Arts Inc.

    \Version 12/11/2012 (jbrookes)

*************************************************************************************************H*/

/*** Include files ********************************************************************************/

#define WIN32_LEAN_AND_MEAN 1               // avoid extra stuff
#include <windows.h>
#include <winsock2.h>
#include <stdlib.h>                         // for strtol()
#include <wrl.h>                            // for ComPtr
#include <robuffer.h>                       // for IBufferByteAccess

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/xboxone/dirtyauthxboxone.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtylang.h"             // for locality macros and definitions
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/util/jsonparse.h"
#include "netconncommon.h"

#ifdef DIRTYSDK_IEAUSER_ENABLED
#include "IEAUser/IEAUser.h"
#endif
#if DIRTYCODE_LOGGING
#include "DirtySDK/xml/xmlparse.h"          // for XmlPrintFmt()
#endif

#using "Microsoft.Xbox.Services.winmd"
using namespace Windows::Xbox::Networking;
using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Xbox::ApplicationModel::Core;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Xbox::System;
using namespace Windows::Xbox::UI;
using namespace Microsoft::Xbox::Services;


/*** Defines **************************************************************************************/

// LOCALE_SNAME: defined in WinNls.h (WINVER >= 0x0600). This block is just there for the case where the define is absent.
#ifndef LOCALE_SNAME
#define LOCALE_SNAME                    0x0000005c   // locale name (ie: en-us)
#endif

// Maximum number of times to retry polling the network status
#define NETCONN_MAX_NETWORK_STATUS_RETRIES 3

// Time between network status polls
#define NETCONN_NETWORK_STATUS_RETRY_TIME_MS 5000

// Time in ms to wait for internet connection
#define NETCONN_MAX_WAIT_TIME_FOR_NETWORK 30000

// duration of wait period for XBL calls to be allowed (5 sec)
#define NETCONNXBOXONE_XBL_CALLS_WAIT_TIMEOUT (5000)

// GetGeoInfo returns XX for unknown country
#define NETCONNXBOXONE_COUNTRY_UNKNOWN_STR  ("XX")

// token validity period
#define NETCONNXBOXONE_TOKEN_TIMEOUT        (60 * 1000)   // 1 minute

// environment determination timeout 
#define NETCONNXBOXONE_ENV_TIMEOUT (10 * 1000) //10 seconds

// initial size of external cleanup list (in number of entries)
#define NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY (12)

// Global EA TMS SCID setup for us by MS, and the propped ea_environment.json file contain environment info
#define NETCONNXBOXONE_GLOBAL_EA_TMS_SCID   "c1e40100-ffb3-44f2-916b-be7650f08966"
#define NETCONNXBOXONE_GLOBAL_EA_ENV_FILE   "ea_environment.json"
#define NETCONNXBOXONE_RETAIL_SANDBOX       "RETAIL"
#define NETCONNXBOXONE_DEFAULT_SANDBOX      NETCONNXBOXONE_RETAIL_SANDBOX
#define NETCONNXBOXONE_CERT_SANDBOX         "CERT"
#define NETCONNXBOXONE_TEST_SANDBOX         "EARW.1"
#define NETCONNXBOXONE_STR_SANDBOX_LENGTH   32

//Country Code String Length
#define NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH 3

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

typedef enum NetConnTokenStateE
{
    ST_TOKEN_INVALID = 0,    // token has not yet been requested
    ST_TOKEN_FAILED,         // token acquisition failed
    ST_TOKEN_INPROG,         // token acquisition in progress
    ST_TOKEN_VALID           // token valid
} NetConnTokenStateE;

typedef struct NetConnTokenT
{
    NetConnTokenStateE  eTokenStatus;       //!< token acquisition status
    int32_t             iTokenAcquTick;     //!< token acquisition time
    int32_t             iTokenBufSize;      //!< token buffer size
    char *              pTokenBuf;          //!< pointer to token buffer
    uint8_t             bFailureReported;   //!< whether token acquisition failure was at least reported once to the user
} NetConnTokenT;

typedef struct NetConnUserT
{
    User                   ^refXboxUser;
    uint64_t                Xuid;
    NetConnTokenT           token;
    NetCritT                asyncOpCrit;        // used to protect against concurrent calls to 'tick' from different threads
    NetCritT                userCrit;           // used to protect against 'xusr' being invoked from voip thread
    char                    strGamertag[16];
    uint8_t                 bCritInitialized;
    uint8_t                 _pad[3];
} NetConnUserT;

typedef int32_t (*NetConnExternalCleanupCallbackT)(void *pNetConnExternalCleanupData);

typedef struct NetConnExternalCleanupEntryT
{
    void *pCleanupData;                         //!< pointer to data to be passed to the external cleanup callback
    NetConnExternalCleanupCallbackT  pCleanupCb;//!< external cleanup callback
} NetConnExternalCleanupEntryT;

//! private module state
typedef struct NetConnRefT
{
    NetConnCommonRefT     Common;       //!< cross-platform netconn data (must come first!)

    NetConnExternalCleanupEntryT
        *pExternalCleanupList;          //!< pointer to an array of entries pending external cleanup completion

    DirtyAuthRefT *pDirtyAuth;          //!< dirtyauth ref

    uint32_t uPlatEnv;                  //!< one of the NETCONN_PLATENV_* values
    uint8_t  bPlatAsyncOpStarted;       //!< flag to determine if the async operation is in progress
    IAsyncOperation<TitleStorage::TitleStorageBlobResult^> ^PlatEnvAsyncOp;

    uint8_t bActPickerAsynOpStarted;
    IAsyncOperation<AccountPickerResult^> ^ActPickerAsynOp;

    int32_t iExternalCleanupListMax;    //!< maximum number of entries in the array (in number of entries)
    int32_t iExternalCleanupListCnt;    //!< number of valid entries in the array

    enum
    {
        ST_EXT_CLEANUP,                 //!< cleaning up external instances from previous NetConn connection, can't proceed before all cleaned up
        ST_INIT,                        //!< initializing
        ST_START_LOG,                   //!< show sign in dialog only when no login user found and bSlient is true
        ST_END_LOG,                     //!< end sign in dialog state
        ST_CONN,                        //!< connecting
        ST_IDLE,                        //!< idle
    } eState;

    uint32_t    uConnStatus;            //!< connection status (surfaced to user)
    uint32_t    uIspTimer;              //!< timing for connecting to the network
    uint32_t    uEnvTimer;              //!< environment determination retry timer

    NetConnUserT aUsers[NETCONN_MAXLOCALUSERS];

#ifdef DIRTYSDK_IEAUSER_ENABLED
    const EA::User::IEAUser *aIEAUsers[NETCONN_MAXLOCALUSERS]; //!< IEAUsers known by NetConn - populated when pIEAUserEventList is processed
#else
    Collections::IVectorView<User ^> ^refXboxUsers; //!< collection of users obtained from the system
    Windows::Foundation::EventRegistrationToken UserAddedEventToken;
    Windows::Foundation::EventRegistrationToken UserRemovedEventToken;
#endif

    Windows::Foundation::EventRegistrationToken CurrentUserChangedEventToken;
    Windows::Foundation::EventRegistrationToken NetworkStatusChangedEventToken;
    Windows::Foundation::EventRegistrationToken ResumingEventToken;
    Windows::Foundation::EventRegistrationToken SuspendingEventToken;
    Windows::Networking::Connectivity::NetworkConnectivityLevel eConnectivityLevel;

    char strAuthUrn[128];       //!< URN used with DirtyAuth
    char strSandboxId[NETCONNXBOXONE_STR_SANDBOX_LENGTH];      //!< sandbox id defined in the title storage


    uint32_t uNetworkStatusPollTimer;
    uint32_t uNetworkStatusPollCount;
    uint8_t  bRetryNetworkStatusPoll;
    uint8_t  bRetryNetworkStatusNow;
    uint8_t _pad0[2];

    uint8_t bResetUserList;
    uint8_t bRefreshUserList;
    uint8_t bRetrySignIn;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    uint8_t _pad1[1];
#else
    uint8_t bAutoResetUserList;;
#endif

    uint8_t bDelayedConn;
    uint8_t bProcessingCleanupList;
    uint8_t bNoLogin;
    uint8_t bSilent;

    /*  mclouatre - aug 7 2014
    MS confirmed a platform bug:  after a game is started or resumed, there is a small window during which the connectivity level can't be queried reliably.
    To work around the issue, MS suggested that we should not query the connectivity level before the NetworkStatusChanged event is notified by the system.
    If this notification does not come within a 5 sec window, the connectivity level ConnectivityLevel::None  should be assumed.
    The following two fields (uXblCallsTimeout, bXblCallsAllowed) are used to implement this work around.
    */
    uint32_t uXblCallsWaitStart;    //!< tick captured when starting to wait for XBL calls to be allowed 
    uint8_t bConnLevelQueryAllowed; //!< TRUE if conn level queries are permitted, FALSE otherwise
    uint8_t bXblCallsAllowed;       //!< TRUE if live calls are permitted, FALSE othewise
    uint8_t bSuspended;
    uint8_t _pad2[1];
} NetConnRefT;

/*** Function Prototypes **************************************************************************/
static void _UpdateNetworkStatus(NetConnRefT *pRef);
static void _NetConnShowAccountPicker(NetConnRefT *pRef);
static void _TransitionToConnState(NetConnRefT *pRef);

/*** Variables ************************************************************************************/

/*** Private Functions ****************************************************************************/

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

    NetPrintf(("netconnxboxone: netconn status changed to %s\n", strConnStatus));
}

/*F********************************************************************************/
/*!
    \Function _NetConnShowAccountPicker

    \Description
        Shows MS Account Picker Dialog

    \Input *pRef    - state ref

    \Version 27/02/2014 (tcho)
*/
/********************************************************************************F*/
static void _NetConnShowAccountPicker(NetConnRefT *pRef)
{
    int32_t iIndex = NetConnQuery(NULL, NULL, 0);

    if (iIndex < 0)
    {
        pRef->bActPickerAsynOpStarted = TRUE;
        pRef->ActPickerAsynOp = SystemUI::ShowAccountPickerAsync(nullptr, Windows::Xbox::UI::AccountPickerOptions::AllowGuests);
        pRef->ActPickerAsynOp->Completed = ref new AsyncOperationCompletedHandler<AccountPickerResult^>
        ([pRef](IAsyncOperation<AccountPickerResult^>^ refAsyncOp, Windows::Foundation::AsyncStatus status)
        {
            switch(status)
            {
                case Windows::Foundation::AsyncStatus::Error:
                {
                    NetPrintf(("netconnxboxone: [%p] _NetConnShowAccountPicker() An error occured in call to ShowAccountPickerAsync(), ErrorCode=0x%08x\n", pRef, refAsyncOp->ErrorCode));
                    pRef->eState =  NetConnRefT::ST_END_LOG;
                    break;
                }
                default:
                {
                    pRef->eState =  NetConnRefT::ST_END_LOG;
                    break;
                }
            }

            pRef->bActPickerAsynOpStarted = FALSE;
        });
    }
    else
    {
        //found at least one person logged in
        pRef->eState = NetConnRefT::ST_END_LOG;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnXblUserMask

    \Description
        return the bitmask of logged-in users.

    \Input *pRef    - state ref

    \Output
        uint32_t    - mask of users bits 0-15

    \Version 12/09/2013 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnXblUserMask(NetConnRefT *pRef)
{
    uint32_t iIndex;
    uint32_t uUserMask = 0;

    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if (!(pRef->aUsers[iIndex].refXboxUser == nullptr || pRef->aUsers[iIndex].Xuid == 0))
        {
            if (pRef->aUsers[iIndex].refXboxUser->IsSignedIn)
            {
                uUserMask |= (1 << iIndex);
            }
        }
    }
    return(uUserMask);
}

/*F********************************************************************************/
/*!
    \Function _NetConnRequestAuthToken

    \Description
        Initiate async op for acquisition of Security Token from MS STS.

    \Input *pRef            - state ref
    \Input iUserIndex       - user index

    \Version 12/01/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnRequestAuthToken(NetConnRefT *pRef, int32_t iUserIndex)
{
    int32_t iResult;
    NetConnTokenT *pTokenSpace = &pRef->aUsers[iUserIndex].token;

    NetPrintf(("netconnxboxone: acquiring XSTS token for user %d\n", iUserIndex));

    // make token request
    if ((iResult = DirtyAuthGetToken(iUserIndex, pRef->strAuthUrn, TRUE)) == DIRTYAUTH_PENDING)
    {
        pTokenSpace->eTokenStatus = ST_TOKEN_INPROG;
    }
    else if (iResult == DIRTYAUTH_SUCCESS)
    {
        NetPrintf(("netconnxboxone: token acquisition for user %d completed (was cached)\n", iUserIndex));

        pTokenSpace->eTokenStatus = ST_TOKEN_VALID;
        pTokenSpace->iTokenAcquTick = NetTick();
    }
    else
    {
        NetPrintf(("netconnxboxone: failed to initiate token acquisition for user %d\n", iUserIndex));

        pTokenSpace->eTokenStatus = ST_TOKEN_FAILED;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnAddToExternalCleanupList

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
        if ((pNewList = (NetConnExternalCleanupEntryT *) DirtyMemAlloc(2 * pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData)) == NULL)
        {
            NetPrintf(("netconnxboxone: unable to allocate memory for the external cleanup list\n"));
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
    \Function _NetConnCheckAuthToken

    \Description
        Check if any pending async op for acquisition of Security Token from MS STS
        is completed.

    \Input *pRef            - state ref

    \Version 02/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnCheckAuthToken(NetConnRefT *pRef)
{
    int32_t iUserIndex, iResult;
    NetConnTokenT *pTokenSpace;

    for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        pTokenSpace = &pRef->aUsers[iUserIndex].token;

        if (pTokenSpace->eTokenStatus == ST_TOKEN_INPROG)
        {
            int32_t iTokenLen;

            // make token request
            if ((iResult = DirtyAuthCheckToken(iUserIndex, pRef->strAuthUrn, &iTokenLen, NULL)) == DIRTYAUTH_SUCCESS)
            {
                NetPrintf(("netconnxboxone: token acquisition for user %d completed - token length = %d\n", iUserIndex, iTokenLen));

                // save token acquisition tick
                pTokenSpace->iTokenAcquTick = NetTick();

                pTokenSpace->eTokenStatus = ST_TOKEN_VALID;
            }
            else if (iResult != DIRTYAUTH_PENDING)
            {
                NetPrintf(("netconnxboxone: token acquisition failed for user %d\n", iUserIndex));

                pTokenSpace->eTokenStatus = ST_TOKEN_FAILED;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnProcessExternalCleanupList

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

            NetPrintf(("netconnxboxone: successfully destroyed external module (cleanup data ptr = %p), external cleanup list count decremented to %d\n",
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
    \Function _NetConnUserIndexToSystemIndex

    \Description
        Translates the user level player index to the User::Users's index

    \Input *pRef        - NetConnRefT reference
    \Input iUserIndex   - player index

    \Output
        int32_t         - The index of the user in the User::Users array, or -1 if not found

    \Version 07/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnUserIndexToSystemIndex(NetConnRefT *pRef, int32_t iUserIndex)
{
    int32_t iReturn = -1;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    iReturn = iUserIndex;
#else
    if (iUserIndex >= 0 && iUserIndex < NETCONN_MAXLOCALUSERS && pRef->aUsers[iUserIndex].refXboxUser != nullptr)
    {
        // Locate the matching player id
        for (uint32_t iSystemIndex = 0; iSystemIndex < pRef->refXboxUsers->Size; ++iSystemIndex)
        {
            if (pRef->aUsers[iUserIndex].refXboxUser->Id == (int32_t)pRef->refXboxUsers->GetAt(iSystemIndex)->Id)
            {
                iReturn = iSystemIndex;
                break;
            }
        }
    }
#endif

    return(iReturn);
}

/*F********************************************************************************/
/*!
    \Function _NetConnSystemIndexToUserIndex

    \Description
        Translates the User::Users' index to the user level player index

    \Input *pRef         - NetConnRefT reference
    \Input iSystemIndex  - User::Users's index

    \Output
        int32_t          - The local player user index, or -1 if not found

    \Version 07/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static int32_t _NetConnSystemIndexToUserIndex(NetConnRefT *pRef, uint32_t iSystemIndex)
{
    int32_t iReturn = -1;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    iReturn = iSystemIndex;
#else
    if (iSystemIndex < pRef->refXboxUsers->Size)
    {
        // Locate the matching player id
        for (int32_t iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; ++iUserIndex)
        {
            if (pRef->aUsers[iUserIndex].refXboxUser != nullptr && pRef->aUsers[iUserIndex].refXboxUser->Id == pRef->refXboxUsers->GetAt(iSystemIndex)->Id)
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
    \Function _NetConnClearUser

    \Description
        Clear the user found at iUserIndex from NetConnUserT array

    \Input *pRef        - NetConnRefT reference
    \Input *iUserIndex  - iUserIndex in NetConnUserT array

    \Version 11/07/2013 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnClearUser(NetConnRefT *pRef, int32_t iUserIndex)
{
    //free the memory if its allocated
    if (pRef->aUsers[iUserIndex].token.pTokenBuf != NULL)
    {
        DirtyMemFree(pRef->aUsers[iUserIndex].token.pTokenBuf, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);
    }

    //clear all the other fields
    //we don't memset because we want to be explicit about eTokenStatus value.
    //memset(&pRef->aUsers[iUserIndex].token, 0, sizeof(pRef->aUsers[iUserIndex].token));
    pRef->aUsers[iUserIndex].token.bFailureReported = FALSE;
    pRef->aUsers[iUserIndex].token.eTokenStatus = ST_TOKEN_INVALID;
    pRef->aUsers[iUserIndex].token.iTokenAcquTick = 0;
    pRef->aUsers[iUserIndex].token.iTokenBufSize = 0;
    pRef->aUsers[iUserIndex].token.pTokenBuf = NULL;

    //we don't memset because we're going to keep the critical section
    pRef->aUsers[iUserIndex].strGamertag[0] = '\0';

    NetCritEnter(&pRef->aUsers[iUserIndex].userCrit);
    pRef->aUsers[iUserIndex].refXboxUser = nullptr;
    NetCritLeave(&pRef->aUsers[iUserIndex].userCrit);

    pRef->aUsers[iUserIndex].Xuid = 0;
}

#ifdef DIRTYSDK_IEAUSER_ENABLED
/*F********************************************************************************/
/*!
    \Function _NetConnInitUser

    \Description
        Init the user found at iUserIndex from NetConnUserT array

    \Input *pRef            - NetConnRefT reference
    \Input *iLocalUserIndex - iUserIndex in NetConnUserT array
    \Input uLocalUserId     - user identifier

    \Output
        int32_t             - 0 for success; negative for failure

    \Version 05/23/2014 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnInitUser(NetConnRefT *pRef, int32_t iLocalUserIndex, uint32_t uLocalUserId)
{
    User ^refXboxUser = nullptr;
    int32_t iXboxUserIndex;
    int32_t iResult = 0; // default to success

    // retrieve the local user
    Collections::IVectorView<User ^> ^refXboxUsers = User::Users;
    for (iXboxUserIndex = 0; iXboxUserIndex < (signed)refXboxUsers->Size; iXboxUserIndex++)
    {
        refXboxUser = refXboxUsers->GetAt(iXboxUserIndex);
        if (uLocalUserId == refXboxUser->Id)
        {
            break;
        }
    }

    if (refXboxUser != nullptr)
    {
        if (pRef->aUsers[iLocalUserIndex].refXboxUser == nullptr)
        {
            // fill our internal table at the specified location

            NetCritEnter(&pRef->aUsers[iLocalUserIndex].userCrit);
            pRef->aUsers[iLocalUserIndex].refXboxUser = refXboxUser;
            NetCritLeave(&pRef->aUsers[iLocalUserIndex].userCrit);
            wcstombs(pRef->aUsers[iLocalUserIndex].strGamertag, refXboxUser->DisplayInfo->Gamertag->Data(), sizeof(pRef->aUsers[iLocalUserIndex].strGamertag));
            pRef->aUsers[iLocalUserIndex].Xuid = _wtoi64(refXboxUser->XboxUserId->Data());
        }
        else
        {
            // this user is supposed to be unchanged
            if (pRef->aUsers[iLocalUserIndex].refXboxUser->XboxUserId != refXboxUser->XboxUserId)
            {
                NetPrintf(("netconnxboxone: _NetConnUpdateUserList() detected a critical misalignment when adding new local user 0x%08x\n", uLocalUserId));
            }
        }
    }
    else
    {
        iResult = -1;
    }

    return (iResult);
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
        if (_NetConnInitUser(pRef, iLocalUserIndex, (uint32_t)pIEAUser->GetUserID()) == 0)
        {
            pRef->aIEAUsers[iLocalUserIndex] = pIEAUser;
            pRef->aIEAUsers[iLocalUserIndex]->AddRef();

            pRef->bRefreshUserList = TRUE;

            NetPrintf(("netconnxboxone: netconn user added at local user index %d - xuid=0x%llx, gtag=%s, localUserId=0x%08x\n",
                iLocalUserIndex, pRef->aUsers[iLocalUserIndex].Xuid, pRef->aUsers[iLocalUserIndex].strGamertag, pRef->aUsers[iLocalUserIndex].refXboxUser->Id));
        }
        else
        {
            NetPrintf(("netconnxboxone: failed to add local user at index %d with local user id 0x%08x\n", iLocalUserIndex, (uint32_t)pIEAUser->GetUserID()));
        }
    }
    else
    {
        NetPrintf(("netconnxboxone: failed to add IEAUser at index %d with local user id 0x%08x because entry already used with local user id 0x%08x\n",
            iLocalUserIndex, (uint32_t)pIEAUser->GetUserID(), (uint32_t)pRef->aIEAUsers[iLocalUserIndex]->GetUserID()));
    }
}

/*F********************************************************************************/
/*!
    \Function   _NetConnQueryIEAUser

    \Description
        Finds the local user index for the passed in IEAUser UserId

    \Input *pRef    - module reference
    \Input id       - IEAUser UserId that we will use to match the index

    \Output
        int32_t - index of the user, negative=not found
*/
/********************************************************************************F*/
static int32_t _NetConnQueryIEAUser(NetConnRefT *pRef, uint32_t id)
{
    int32_t iResult = -1;
    int32_t i = 0;

    for (; i < NETCONN_MAXLOCALUSERS; i++)
    {
        if (pRef->aIEAUsers[i] != NULL &&
            (uint32_t)pRef->aIEAUsers[i]->GetUserID() == id)
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
            if (pRef->aUsers[iLocalUserIndex].refXboxUser != nullptr)
            {
                NetPrintf(("netconnxboxone: netconn user removed at local user index %d - xuid=0x%llx, gtag=%s, localUserId=0x%08x\n",
                    iLocalUserIndex, pRef->aUsers[iLocalUserIndex].Xuid, pRef->aUsers[iLocalUserIndex].strGamertag, pRef->aUsers[iLocalUserIndex].refXboxUser->Id));
                _NetConnClearUser(pRef, iLocalUserIndex);

                pRef->aIEAUsers[iLocalUserIndex]->Release();
                pRef->aIEAUsers[iLocalUserIndex] = NULL;

                pRef->bRefreshUserList = TRUE;
            }
        }
        else
        {
            NetPrintf(("netconnxboxone: failed to remove local user at index %d - local user ids do not match (0x%08x vs 0x%08x)\n",
                iLocalUserIndex, (uint32_t)pIEAUser->GetUserID(), (uint32_t)pRef->aIEAUsers[iLocalUserIndex]->GetUserID()));
        }
    }
    else
    {
        NetPrintf(("netconnxboxone: failed to remove IEAUSER at index %d - no local user at that spot\n", iLocalUserIndex));
    }
}
#else
/*F********************************************************************************/
/*!
    \Function _NetConnUpdateUserList

    \Description
        Update the NetConnUserT array

    \Input *pRef       - NetConnRefT reference

    \Version 07/20/2013 (mcorcoran)
*/
/********************************************************************************F*/
static void _NetConnUpdateUserList(NetConnRefT *pRef)
{

    int32_t iIndex;

    /*
     Build the local user index table
     It is necessary to maintain this level of indirection because
     Xbox's local index from User::Users will
     collapse in such a way that later indexes always become vacant
     first. For example, if the controllers at index 1 and 2 are
     logged into the console, and controller 1 is logger out, controller
     2 will become te new index 1.
     From a DirtySDK point of view this is undesireable, since we want
     a player at index 2 to STAY at index 2 if player 1 logs out.
    */

    uint8_t bFound = FALSE;

    pRef->refXboxUsers = User::Users;

    // First remove userId's that are no longer in the User::Users list.
    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        bFound = FALSE;

        // $todo: re-evaluate why we need the second part of the this condition. checking for XBoxUser==nullptr should be enough here
        if (pRef->aUsers[iIndex].refXboxUser == nullptr || pRef->aUsers[iIndex].Xuid == 0)
        {
            continue;
        }

        for (uint32_t iSubIndex = 0; iSubIndex < pRef->refXboxUsers->Size; ++iSubIndex)
        {
            if (pRef->refXboxUsers->GetAt(iSubIndex)->Id == pRef->aUsers[iIndex].refXboxUser->Id)
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            _NetConnClearUser(pRef, iIndex);
            NetPrintf(("netconnxboxone: user removed at index %d\n", iIndex));
        }
    }

    // then fill in new users in User::Users's list from the front of the local index
    for (uint32_t iIndex = 0; iIndex < pRef->refXboxUsers->Size; ++iIndex)
    {
        bFound = FALSE;
        User ^user = pRef->refXboxUsers->GetAt(iIndex);

        for (uint32_t iSubIndex = 0; iSubIndex < NETCONN_MAXLOCALUSERS; ++iSubIndex)
        {
            if (pRef->aUsers[iSubIndex].refXboxUser != nullptr && pRef->aUsers[iSubIndex].refXboxUser->Id == user->Id)
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
                // $todo: re-evaluate why we need the second part of the this condition. checking for XBoxUser==nullptr should be enough here, and
                // this OR statement seems to be opening the door for the entry to be overriden under some conditions
                if (pRef->aUsers[iWriteIndex].refXboxUser == nullptr || pRef->aUsers[iWriteIndex].Xuid == 0)
                {
                    NetCritEnter(&pRef->aUsers[iWriteIndex].userCrit);
                    pRef->aUsers[iWriteIndex].refXboxUser = user;
                    NetCritLeave(&pRef->aUsers[iWriteIndex].userCrit);
                    wcstombs(pRef->aUsers[iWriteIndex].strGamertag, user->DisplayInfo->Gamertag->Data(), sizeof(pRef->aUsers[iWriteIndex].strGamertag));
                    pRef->aUsers[iWriteIndex].Xuid = _wtoi64(user->XboxUserId->Data());

                    NetPrintf(("netconnxboxone: user added at index %d - xuid=0x%llx, gtag=%s, id=%d\n",
                        iWriteIndex, pRef->aUsers[iWriteIndex].Xuid, pRef->aUsers[iWriteIndex].strGamertag, pRef->aUsers[iWriteIndex].refXboxUser->Id));
                    break;
                }
            }
        }
    }
}
#endif   // DIRTYSDK_IEAUSER_ENABLED


/*F********************************************************************************/
/*!
    \Function _NetConnResetUserList

    \Description
        Clear then re-populate the NetConnUserT array

    \Input *pRef       - NetConnRefT reference

    \Version 11/07/2013 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnResetUserList(NetConnRefT *pRef)
{
    uint32_t iIndex;

    //blow away the current user list so we reconstruct a new one from scratch
    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        #if DIRTYCODE_LOGGING
        if (pRef->aUsers[iIndex].refXboxUser != nullptr || pRef->aUsers[iIndex].Xuid != 0)
        {
            NetPrintf(("netconnxboxone: purged user at %d\n", iIndex));
        }
        #endif
        _NetConnClearUser(pRef, iIndex);
    }

#ifdef DIRTYSDK_IEAUSER_ENABLED
    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if (pRef->aIEAUsers[iIndex] != NULL)
        {
            _NetConnInitUser(pRef, iIndex, (uint32_t)pRef->aIEAUsers[iIndex]->GetUserID());
            NetPrintf(("netconnxboxone: initialized user at %d\n", iIndex));
        }
    }
#else
    //add the users back in
    _NetConnUpdateUserList(pRef);
#endif
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetEnvironment

    \Description
        Determine environment using title storage

    \Input *pRef        - netconn module state

    \Version 06/24/2013 (mcorcoran)
*/
/********************************************************************************F*/
static void _NetConnGetEnvironment(NetConnRefT *pRef)
{
    XboxLiveContext^ context = nullptr;
    int32_t iIndex;

    // set to invalid environment and pending state
    pRef->uPlatEnv = (unsigned)-1;
    _NetConnUpdateConnStatus(pRef, '~enw');

    // Query for the SandboxId using the XboxLiveConfiguration
    // If the environment is an official Microsoft sandbox that maps directly to an environment then return it
    // Otherwise this is a custom EA sandbox so you need to query to see which environment we are on using for this title
    wcstombs(pRef->strSandboxId, Windows::Xbox::Services::XboxLiveConfiguration::SandboxId->Data(), sizeof(pRef->strSandboxId));
    if (ds_stricmp(pRef->strSandboxId, NETCONNXBOXONE_RETAIL_SANDBOX) == 0)
    {
        pRef->uPlatEnv = NETCONN_PLATENV_PROD;
        NetPrintf(("netconnxboxone: [%p] Direct Sandbox to Environment mapping found : SandboxId='%s' Environment='PROD'\n", pRef, pRef->strSandboxId));
    }
    else if (ds_stricmp(pRef->strSandboxId, NETCONNXBOXONE_CERT_SANDBOX) == 0)
    {
        pRef->uPlatEnv = NETCONN_PLATENV_CERT;
        NetPrintf(("netconnxboxone: [%p] Direct Sandbox to Environment mapping found : SandboxId='%s' Environment='CERT'\n", pRef, pRef->strSandboxId));
    }
    else if (ds_stricmp(pRef->strSandboxId, NETCONNXBOXONE_TEST_SANDBOX) == 0)
    {
        pRef->uPlatEnv = NETCONN_PLATENV_TEST;
        NetPrintf(("netconnxboxone: [%p] Direct Sandbox to Environment mapping found : SandboxId='%s' Environment='TEST'\n", pRef, pRef->strSandboxId));
    }

    // Have our environment mapping don't bother doing the TMS lookup
    if (pRef->uPlatEnv != (unsigned)-1)
    {
        return;
    }

    // get any signed in user to make the TMS call below
    iIndex = NetConnQuery(NULL, NULL, 0);
    if (iIndex < 0)
    {
        // This should really never happen.  The user would have to call
        // NetConnConnect() with a user signed in (otherwise we wouldn't have
        // gotten here), and then within one frame/idle(), signed out.
        NetPrintf(("netconnxboxone: no user is signed in\n"));
         _NetConnUpdateConnStatus(pRef, '-act');
        return;
    }

    try
    {
        context = ref new XboxLiveContext(pRef->aUsers[iIndex].refXboxUser);
    }
    catch (Exception ^ e)
    {
        NetPrintf(("netconnxboxone: (Error) unable to instantiate XboxLiveContext (%S/0x%08x)\n", e->Message->Data(), e->HResult));
        NetPrintf((
            "netconnxboxone: WARNING!!! Microsoft.Xbox.Services.dll is likely not loaded. Please add "
            "a reference to 'XBox Services API' in your top-level project file to make this feature useable.\n"));
        pRef->uPlatEnv = NETCONN_PLATENV_PROD; // default to prod
        return;
    }

    TimeSpan ts;
    ts.Duration = 60000000; // 6 seconds
    context->Settings->HttpTimeoutWindow = ts; // The total window for the request(s) to execute
    ts.Duration = 30000000; // 3 seconds
    context->Settings->HttpTimeout = ts; // The timeout for a single request
    ts.Duration = 0; // 0 seconds
    context->Settings->HttpRetryDelay = ts; // the delay, after a request fails before the next request.

#if DIRTYCODE_LOGGING
    context->Settings->DiagnosticsTraceLevel = XboxServicesDiagnosticsTraceLevel::Verbose;
    context->Settings->EnableServiceCallRoutedEvents = true;

    context->Settings->ServiceCallRouted += ref new EventHandler<XboxServiceCallRoutedEventArgs^>(
        [=](Platform::Object^, XboxServiceCallRoutedEventArgs^ args)
        {
            // Display HTTP errors to screen for easy debugging
            NetPrintf(("netconnxboxone: [XboxLiveServices] %S %S\n", args->HttpMethod->Data(), args->Url->AbsoluteUri->Data()));
            NetPrintf(("netconnxboxone: [XboxLiveServices] %S\n", args->RequestHeaders->Data()));
            NetPrintf(("netconnxboxone: [XboxLiveServices] \n"));
            NetPrintf(("netconnxboxone: [XboxLiveServices] %S\n", args->RequestBody->Data()));

            NetPrintf(("netconnxboxone: [XboxLiveServices] Response Status: %S\n", args->HttpStatus.ToString()->Data()));
            NetPrintf(("netconnxboxone: [XboxLiveServices] %S\n", args->ResponseHeaders->Data()));
            NetPrintf(("netconnxboxone: [XboxLiveServices] \n"));
            NetPrintf(("netconnxboxone: [XboxLiveServices] %S\n", args->ResponseBody->Data()));
        });
#endif

    TitleStorage::TitleStorageBlobMetadata^ metadata = ref new TitleStorage::TitleStorageBlobMetadata(
        NETCONNXBOXONE_GLOBAL_EA_TMS_SCID,
        TitleStorage::TitleStorageType::GlobalStorage,
        NETCONNXBOXONE_GLOBAL_EA_ENV_FILE,
        TitleStorage::TitleStorageBlobType::Json,
        context->User->XboxUserId);

    Windows::Storage::Streams::Buffer^ buffer = ref new Windows::Storage::Streams::Buffer(1024);

    try
    {
        pRef->PlatEnvAsyncOp = context->TitleStorageService->DownloadBlobAsync(metadata, buffer, TitleStorage::TitleStorageETagMatchCondition::NotUsed, "");
    }
    catch (Exception ^ e)
    {
        NetPrintf(("netconnxboxone: (Error) exception thrown when calling DownloadBlobAsync() (%S/0x%08x)\n", e->Message->Data(), e->HResult));
        pRef->uPlatEnv = NETCONN_PLATENV_PROD; // default to prod
        return;
    }
    pRef->bPlatAsyncOpStarted = TRUE;

    pRef->PlatEnvAsyncOp->Completed = ref new AsyncOperationCompletedHandler<TitleStorage::TitleStorageBlobResult^>
        ([pRef](IAsyncOperation<TitleStorage::TitleStorageBlobResult^>^ refAsyncOp, Windows::Foundation::AsyncStatus status)
        {
            switch (status)
            {
                case Windows::Foundation::AsyncStatus::Completed:
                {
                    uint32_t uPlatEnv = NETCONN_PLATENV_PROD; // default to prod

                    TitleStorage::TitleStorageBlobResult^ result = nullptr;
                    try
                    {
                        result = refAsyncOp->GetResults();
                    }
                    catch (Platform::Exception ^e)
                    {
                        NetPrintf(("netconnxboxone: _NetConnGetEnvironment() Exception thrown (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                    }

                    if (result != nullptr)
                    {
                        Microsoft::WRL::ComPtr<IUnknown> pBuffer((IUnknown*)result->BlobBuffer);
                        Microsoft::WRL::ComPtr<IBufferByteAccess> pBufferByteAccess;
                        pBuffer.As(&pBufferByteAccess);

                        // get pointer to data
                        byte* pData = nullptr;
                        if (SUCCEEDED(pBufferByteAccess->Buffer(&pData)))
                        {
                            uint16_t aJsonList[2048];
                            if (JsonParse(aJsonList, sizeof(aJsonList), (char*)pData, result->BlobBuffer->Length) >= 0)
                            {
                                char strEnvironment[32] = "";
                                const char *pEnvironment = JsonFind(aJsonList, "environmentInfo.environment");
                                if (pEnvironment)
                                {
                                    JsonGetString(pEnvironment, strEnvironment, sizeof(strEnvironment), "");

                                    if (ds_stricmp(strEnvironment, "SDEV") == 0)
                                    {
                                        uPlatEnv = NETCONN_PLATENV_DEV;
                                    }
                                    else if (ds_stricmp(strEnvironment, "STEST") == 0)
                                    {
                                        uPlatEnv = NETCONN_PLATENV_TEST;
                                    }
                                    else if (ds_stricmp(strEnvironment, "SCERT") == 0)
                                    {
                                        uPlatEnv = NETCONN_PLATENV_CERT;
                                    }
                                    else if (ds_stricmp(strEnvironment, "PROD") == 0)
                                    {
                                        uPlatEnv = NETCONN_PLATENV_PROD;
                                    }
                                }

                                NetPrintf(("netconnxboxone: [%p] " NETCONNXBOXONE_GLOBAL_EA_ENV_FILE " retrieved from TMS reports: SandboxId='%s', Environment='%s'\n", pRef, pRef->strSandboxId, strEnvironment));
                            }
                            else
                            {
                                NetPrintf(("netconnxboxone: [%p] _NetConnGetEnvironment() failed to parse json response\n", pRef));
                            }
                        }
                        else
                        {
                            // this should pretty much never happen
                            NetPrintf(("netconnxboxone: [%p] _NetConnGetEnvironment() failed to obtain pointer to internal data for BlobBuffer\n", pRef));
                        }
                    }

                    // assign platform environment
                    pRef->uPlatEnv = uPlatEnv;
                    break;
                }
                case Windows::Foundation::AsyncStatus::Error:
                {
                    NetPrintf(("netconnxboxone: _NetConnGetEnvironment() An error occured in call to DownloadBlobAsync(), ErrorCode=0x%08x.\n", refAsyncOp->ErrorCode));

                    if ((pRef->uEnvTimer == 0) || (NetTickDiff(NetTick(), pRef->uEnvTimer) < NETCONNXBOXONE_ENV_TIMEOUT))
                    {
                        if ((pRef->uEnvTimer == 0))
                        {
                            pRef->uEnvTimer = NetTick();
                        }
                        
                        if (pRef->eState == NetConnRefT::ST_CONN && pRef->uConnStatus >> 24 == '~')
                        {
                            NetConnSleep(1000);                      //Retry every 1 second if still in a connecting state
                            _NetConnUpdateConnStatus(pRef, '~env');  //Retry the DownblobAsync Call
                            NetPrintf(("netconnxboxone: retrying environment determination.\n"));
                        }
                        break;
                    }
                    else
                    {            
                        pRef->uEnvTimer = 0; //Reseting the env timeout 
                        if (pRef->eConnectivityLevel < Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess)
                        {
                            NetPrintf(("netconnxboxone: lost XboxLive connection while determining environment.\n"));
                            _NetConnUpdateConnStatus(pRef, '-net');
                        }
                        else
                        {
                            pRef->uPlatEnv = NETCONN_PLATENV_PROD; // if an error occurs, just default to prod
                            NetPrintf(("netconnxboxone: environment determination failed. Retry timeout exceeded, defaulting to PROD.\n"));
                        }
                        break;
                    }
                }
                case Windows::Foundation::AsyncStatus::Canceled:
                {
                    // The user may have called NetConnDisconnect() before +onl, in which case, don't modify the value of pRef->uPlatEnv or pRef->strSandboxId
                    NetPrintf(("netconnxboxone: _NetConnGetEnvironment() Canceled.\n"));
                    break;
                }
            }

            pRef->bPlatAsyncOpStarted = FALSE;
            refAsyncOp->Close();
        });
}

/*F********************************************************************************/
/*!
    \Function _TransitionToConnState

    \Description
        Transition ST_CONN and appropriate substate.

    \Input *pRef    - netconn state

    \Version 09/05/2013 (abaldeva)
*/
/********************************************************************************F*/
static void _TransitionToConnState(NetConnRefT *pRef)
{
    pRef->eState = NetConnRefT::ST_CONN;
    if(NetConnStatus('xcon', 0, NULL, 0))
    {
        NetPrintf(("netconnxboxone: Xbox Live access detected for game. Transition to ~con state.\n"));
        _NetConnUpdateConnStatus(pRef, '~con');
    }
    else
    {
        NetPrintf(("netconnxboxone: No Xbox Live access detected for game. Transition to ~net state.\n"));
        _NetConnUpdateConnStatus(pRef, '~net');
    }

    pRef->uIspTimer = NetTick();
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateCONN

    \Description
        ST_CONN state processing

    \Input *pRef    - netconn state

    \Version 17/12/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateCONN(NetConnRefT *pRef)
{
    if (pRef->bNoLogin)
    {
        NetPrintf(("netconnxboxone: skipping token acquisition and user profile query because -nologin was used\n"));
        _NetConnUpdateConnStatus(pRef, '+onl');
        pRef->eState = NetConnRefT::ST_IDLE;
    }
    else
    {
        // is any user signed in
        if (NetConnQuery(NULL, NULL, 0) >= 0)
        {
            if ((pRef->pDirtyAuth == NULL) && ((pRef->pDirtyAuth = DirtyAuthCreate(TRUE)) == NULL))
            {
                NetPrintf(("netconnxboxone: error creating DirtyAuth\n"));
                _NetConnUpdateConnStatus(pRef, '-xht');
            }
            else
            {
                // signal _NetConnUpdate() to call _NetConnGetEnvironment() on the next idle
                _NetConnUpdateConnStatus(pRef, '~env');
            }
        }
        else
        {
            NetPrintf(("netconnxboxone: no user signed in\n"));
            _NetConnUpdateConnStatus(pRef, '-act');
        }
    }

    // check for completin of pending token acquisition operation
    _NetConnCheckAuthToken(pRef);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uTick    - current tick count

    \Version 12/13/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uTick)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    if (pRef->bXblCallsAllowed == FALSE)
    {
        if (NetTickDiff(NetTick(), pRef->uXblCallsWaitStart) > NETCONNXBOXONE_XBL_CALLS_WAIT_TIMEOUT)
        {
            NetPrintf(("netconnxboxone: failed to reach state where XBL calls are allowed  tick=%d, forcing network status query.\n", NetTick()));
            _NetConnUpdateConnStatus(pRef, '-xbl');
            pRef->eConnectivityLevel = Windows::Networking::Connectivity::NetworkConnectivityLevel::None;
            pRef->uXblCallsWaitStart = NetTick();   // reset start tick

            // after the timeout force a network check
            // amakoukji: as per GOSOPS-151751 it seems like if a user logged out during a suspend 
            // then resumes we will never receive the appropriate NetworkStatusChanged event
            pRef->bRetryNetworkStatusNow = TRUE;
            pRef->bConnLevelQueryAllowed = TRUE;
        }
    }

    // waiting for external cleanup list to be empty
    if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
    {
        if (pRef->pExternalCleanupList[0].pCleanupCb == NULL)
        {
            pRef->eState = NetConnRefT::ST_INIT;
            if (pRef->bDelayedConn)
            {
                pRef->eState = NetConnRefT::ST_START_LOG;
                pRef->bDelayedConn = FALSE;
            }
        }
    }

    // retry fetching the network status if MS threw an exception or notified us of a network change
    if (pRef->bRetryNetworkStatusNow)
    {
        pRef->bRetryNetworkStatusNow = FALSE;
        _UpdateNetworkStatus(pRef);
    }
    else if (pRef->bRetryNetworkStatusPoll)
    {
        uint32_t uCurrentTime = NetTick();
        if (pRef->uNetworkStatusPollCount >= NETCONN_MAX_NETWORK_STATUS_RETRIES)
        {
            char cStatus = ((pRef->uConnStatus >> 24) & 0xFF);
            if (cStatus == '+' || cStatus == '~')
            {
                _NetConnUpdateConnStatus(pRef, '-net');
            }

            pRef->bRetryNetworkStatusPoll = FALSE;
            pRef->uNetworkStatusPollTimer = 0;
            pRef->uNetworkStatusPollCount = 0;
        }
        else if ((NetTickDiff(uCurrentTime, pRef->uNetworkStatusPollTimer) > NETCONN_NETWORK_STATUS_RETRY_TIME_MS))
        {
            pRef->uNetworkStatusPollTimer = uCurrentTime;
            pRef->bRetryNetworkStatusPoll = FALSE;
            ++(pRef->uNetworkStatusPollCount);
            _UpdateNetworkStatus(pRef);
        }
    }

    //Show Sign in dialog when appropriate
    if (pRef->eState == NetConnRefT::ST_START_LOG)
    {
        if ((pRef->bSilent != TRUE) && (pRef->bActPickerAsynOpStarted != TRUE))
        {
            _NetConnShowAccountPicker(pRef);
        }
        else if (pRef->bSilent == TRUE)
        {
            //Silent Connect Proceed to CONN
            _TransitionToConnState(pRef);
        }
    }

    //End Sign in dialog state
    if (pRef->eState == NetConnRefT::ST_END_LOG)
    {
        //Check to see if the user actually signed in anyone with the dialog
        int32_t iIndex = NetConnQuery(NULL, NULL, 0);

        if (iIndex >= 0)
        {
            //Valid Signed in User found Connecting
            _TransitionToConnState(pRef);
        }
        else
        {
            //No Valid Signed in User (following Xenon behavior)
            NetPrintf(("netconnxboxone: no signed-in users - returning to idle state \n"));
            _NetConnUpdateConnStatus(pRef, '-svc');
            pRef->eState = NetConnRefT::ST_IDLE;
        }
    }

#ifdef DIRTYSDK_IEAUSER_ENABLED
    NetConnCommonUpdateLocalUsers((NetConnCommonRefT *)pRef);
#endif

    // if connecting, check for connection success
    if (pRef->eState == NetConnRefT::ST_CONN)
    {
        if(pRef->uConnStatus == '~net')
        {
            //If we previously attempted to connect but were not able to do so due to lack of internet connection, try again.
            if (NetConnStatus('xcon', 0, NULL, 0))
            {
                _NetConnUpdateConnStatus(pRef, '~con');
                pRef->uIspTimer = 0;
            }
            else
            {
                // after a set amount of time set the state to '-svc' if we have network connectivity
                uint32_t uCurrentTime = NetTick();
                if ((NetTickDiff(uCurrentTime, pRef->uIspTimer) > NETCONN_MAX_WAIT_TIME_FOR_NETWORK))
                {
                    NetPrintf(("netconnxboxone: timout waiting for network connectivity. Moving to -svc state.\n"));
                    pRef->uConnStatus = '-svc';
                    pRef->uIspTimer = 0;
                }
            }
        }
        else if (pRef->uConnStatus == '~con')
        {
            _NetConnUpdateCONN(pRef);
        }
        else if (pRef->uConnStatus == '~env') // Time to issue the request to get the environment
        {
            try
            {
                _NetConnGetEnvironment(pRef);
            }
            catch (Exception ^ e)
            {
                // amakoukji: DICE reported that other exceptions are possible above the ones that are caught inside of _NetConnGetEnvironment.
                // As I have not been able to reproduce these I put a catch all here for unknown exceptions that will clean up appropriately.
                NetPrintf(("netconnxboxone: (Error) unexpected exception thrown when fetching environment (%S/0x%08x).\n", e->Message->Data(), e->HResult));
                pRef->uPlatEnv = NETCONN_PLATENV_PROD; // default to prod
                ds_strnzcpy(pRef->strSandboxId, NETCONNXBOXONE_DEFAULT_SANDBOX, sizeof(pRef->strSandboxId));
                if (pRef->PlatEnvAsyncOp)
                {
                    pRef->PlatEnvAsyncOp->Cancel();
                    pRef->bPlatAsyncOpStarted = FALSE;
                }
            }
        }
        else if (pRef->uConnStatus == '~enw') // Working on getting the environment information
        {
            // check for completion to transition +onl
            if (pRef->uPlatEnv != (unsigned)-1)
            {
                _NetConnUpdateConnStatus(pRef, '+onl');
                pRef->eState = NetConnRefT::ST_IDLE;
            }
        }
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != NetConnRefT::ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = NetConnRefT::ST_IDLE;
    }

    // idle processing
    if (pRef->eState == NetConnRefT::ST_IDLE)
    {
        if (pRef->uConnStatus == '+onl')
        {
            _NetConnCheckAuthToken(pRef);
        }
    }

    if (pRef->bResetUserList)
    {
        _NetConnResetUserList(pRef);
        pRef->bResetUserList = FALSE;
    }

    if (pRef->bRefreshUserList)
    {
        // if we weren't online and someone has just signed in try to connect
        if (pRef->uConnStatus >> 24 == '-' && pRef->bRetrySignIn)
        {
            pRef->eState = NetConnRefT::ST_START_LOG;
        }

#ifndef DIRTYSDK_IEAUSER_ENABLED
        _NetConnUpdateUserList(pRef);
#endif

        if (pRef->uConnStatus == '+onl')
        {
            // check to see if all users are signed out
            if (NetConnQuery(NULL, NULL, 0) < 0)
            {
                // detect that all users have logged out of live
                _NetConnUpdateConnStatus(pRef, '-act');
            }
        }

        pRef->bRefreshUserList = FALSE;
        pRef->bRetrySignIn = FALSE;
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);
}

/*F********************************************************************************/
/*!
    \Function _UpdateNetworkStatus

    \Description
        Update status of Network connectivity.

    \Input *pRef    - netconn state

    \Version 09/05/2013 (abaldeva)
*/
/********************************************************************************F*/
static void _UpdateNetworkStatus(NetConnRefT *pRef)
{
    #if DIRTYCODE_LOGGING
    const char *_strConnectivityLevel[] =
    {
        "None",                         // Windows::Networking::Connectivity::NetworkConnectivityLevel::None                        == 0
        "LocalAccess",                  // Windows::Networking::Connectivity::NetworkConnectivityLevel::LocalAccess                 == 1
        "ConstrainedInternetAccess",    // Windows::Networking::Connectivity::NetworkConnectivityLevel::ConstrainedInternetAccess   == 2
        "InternetAccess",               // Windows::Networking::Connectivity::NetworkConnectivityLevel::InternetAccess              == 3
        "XboxLiveAccess"                // Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess              == 4
    };
    #endif

    NetPrintf(("netconnxboxone: attempting to update connectivity level\n"));

    if (pRef->bConnLevelQueryAllowed == FALSE)
    {
        NetPrintf(("netconnxboxone: connectivity level update skipped because XBL calls not yet allowed\n"));
        return;
    }

    ConnectionProfile^ refConnectionProfile;
    Windows::Networking::Connectivity::NetworkConnectivityLevel ePrevConnectivityLevel = pRef->eConnectivityLevel;

    try
    {
        refConnectionProfile = Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile();
    }
    catch (Exception ^ e)
    {
        NetPrintf(("netconnxboxone: exception caught while trying to get GetInternetConnectionProfile (%S/0x%08x)\n", e->Message->Data(), e->HResult));
        pRef->bRetryNetworkStatusPoll = TRUE;
        pRef->uNetworkStatusPollTimer = NetTick();
        return;
    }
    // getInternetConnectionProfile() can actually return a nullptr. if it does, assume no internet connection (as per sample code)
    if (refConnectionProfile != nullptr)
    {
        try
        {
            pRef->eConnectivityLevel = refConnectionProfile->GetNetworkConnectivityLevel();
        }
        catch (Exception ^ e)
        {
            NetPrintf(("netconnxboxone: exception caught while trying to get NetworkConnectivityLevel (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            pRef->bRetryNetworkStatusPoll = TRUE;
            pRef->uNetworkStatusPollTimer = NetTick();
            return;
        }
    }
    else
    {
        NetPrintf(("netconnxboxone: null internet connection profile\n"));
        pRef->eConnectivityLevel = Windows::Networking::Connectivity::NetworkConnectivityLevel::None;
    }

    // are we waiting for XBL calls to be allowed
    if (pRef->bXblCallsAllowed == FALSE)
    {
        if (pRef->eConnectivityLevel == Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess)
        {
            // signal the end of the wait period for XBL calls to be allowed
            pRef->uXblCallsWaitStart = 0;
            pRef->bXblCallsAllowed = TRUE;
        }
        else
        {
            NetPrintf(("netconnxboxone: connectivity level update temporarily ignored until XboxLiveAccess is recovered (%d) \n", pRef->eConnectivityLevel));
            pRef->eConnectivityLevel = ePrevConnectivityLevel;
            return;
        }
    }

    if ((ePrevConnectivityLevel >= Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess)  &&
        (pRef->eConnectivityLevel < Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess))
    {
        NetPrintf(("netconnxboxone: just disconnected - move to -net state.\n"));
        _NetConnUpdateConnStatus(pRef, '-net');
    }

    // refresh address
    NetConnStatus('addr', 1, NULL, 0);
    // display address
    NetPrintf(("netconnxboxone: connectivity level %s (%d) addr=%a\n", _strConnectivityLevel[(int)pRef->eConnectivityLevel], pRef->eConnectivityLevel, NetConnStatus('addr', 0, NULL, 0)));

    pRef->bRetryNetworkStatusPoll = FALSE;
    pRef->uNetworkStatusPollTimer = 0;
    pRef->uNetworkStatusPollCount = 0;
}

/*** Public functions *****************************************************************************/

/*F********************************************************************************/
/*!
    \Function NetConnStartup

    \Description
        Bring the network connection module to life. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -noupnp  : disables UPNP
            -nologin : disables user athentication during connection flow (temporary - will eventually be removed)
            -servicename=<game-year-platform> : set servicename required for SSL use
        \endverbatim

    \Version 12/01/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef;
    int32_t iUserIndex;

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // common startup
#ifdef DIRTYSDK_IEAUSER_ENABLED
    if ((pRef = (NetConnRefT *)NetConnCommonStartup(sizeof(*pRef), _NetConnAddLocalUser, _NetConnRemoveLocalUser)) == NULL)
    {
        return(-1);
    }
#else
    if ((pRef = (NetConnRefT *)NetConnCommonStartup(sizeof(*pRef))) == NULL)
    {
        return(-1);
    }
    
    pRef->bAutoResetUserList = TRUE;
#endif

    pRef->eState = NetConnRefT::ST_INIT;
    pRef->bDelayedConn = FALSE;
    pRef->bXblCallsAllowed = TRUE;
    pRef->bConnLevelQueryAllowed = TRUE;
    pRef->bSuspended = FALSE;
    
    // default to PROD environment, will eventually be overwritten with value obtained from ea_environment.json file in TMS
    pRef->uPlatEnv = NETCONN_PLATENV_PROD;
    ds_strnzcpy(pRef->strSandboxId, NETCONNXBOXONE_DEFAULT_SANDBOX, sizeof(pRef->strSandboxId));

    // initialize user-specific data
    for(iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        NetCritInit(&pRef->aUsers[iUserIndex].asyncOpCrit, "netconn-userasyncopcrit");
        NetCritInit(&pRef->aUsers[iUserIndex].userCrit, "netconn-usercrit");
        _NetConnClearUser(pRef, iUserIndex);
        pRef->aUsers[iUserIndex].bCritInitialized = TRUE;
    }

    // start up dirtysock
    if (SocketCreate(THREAD_PRIORITY_HIGHEST, 0, 0) != 0)
    {
        NetPrintf(("netconnxboxone: unable to start up dirtysock\n"));
        NetConnShutdown(0);
        return(-3);
    }

    if (strstr(pParams, "-nologin") != NULL)
    {
        pRef->bNoLogin = TRUE;
    }

    // add netconn task handle
    if (NetConnIdleAdd(_NetConnUpdate, pRef) < 0)
    {
        NetPrintf(("netconnxboxone: unable to add netconn task handler\n"));
        NetConnShutdown(0);
        return(-9);
    }

    // allocate external cleanup listr
    pRef->iExternalCleanupListMax = NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY;
    if ((pRef->pExternalCleanupList = (NetConnExternalCleanupEntryT *) DirtyMemAlloc(pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnxboxone: unable to allocate memory for initial external cleanup list\n"));
        NetConnShutdown(0);
        return(-5);
    }
    memset(pRef->pExternalCleanupList, 0, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-5);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnxboxone: unable to startup protossl\n"));
        return(-3);
    }

// mclouatre 05-17-2013
// this call keeps throwing accessdenied exception on xboxoneadk. I have not succeeded to explain this part yet.
#if !defined(DIRTYCODE_XBOXONEADK_ONLY)
    // startup microsoft secure sockets, so socket calls don't silently fail
    // this will load all the port configurations (SecureDeviceAssociationTemplate) from networkmanifest.xml
    try
    {
        Windows::Xbox::Networking::SecureDeviceAssociationTemplate::Templates;
    }
    catch (Exception ^ e)
    {
        NetPrintf(("netconnxboxone: failed loading SecureDeviceAssociationTemplate (err=%s)\n", DirtyErrGetName(e->HResult)));
    }
#endif


#ifndef DIRTYSDK_IEAUSER_ENABLED
    // do the initial setup of the local user array
    _NetConnUpdateUserList(pRef);

    if(!pRef->UserAddedEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding user added event handler \n"));
        pRef->UserAddedEventToken = (User::UserAdded += ref new EventHandler<UserAddedEventArgs^>(
            [=](Platform::Object ^refObj, UserAddedEventArgs ^refArgs)
        {
            if (refArgs->User)
            {
                NetPrintf(("netconnxboxone: user added event (gtag=%S xuid=%S, id=%d, signin=%d)\n",
                    refArgs->User->DisplayInfo->Gamertag->Data(), refArgs->User->XboxUserId->Data(), refArgs->User->Id, refArgs->User->IsSignedIn));
            }
            else
            {
                NetPrintf(("netconnxboxone: user added event (signin=0)\n"));
            }
            pRef->bRefreshUserList = TRUE;

            if (refArgs->User->IsSignedIn)
            {
                pRef->bRetrySignIn = TRUE;
            }
        }));
    }

    if(!pRef->UserRemovedEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding user removed event handler \n"));
        pRef->UserRemovedEventToken = (User::UserRemoved += ref new EventHandler<UserRemovedEventArgs^>(
                [=](Platform::Object ^refObj, UserRemovedEventArgs ^refArgs)
            {
                NetPrintf(("netconnxboxone: user removed event (gtag=%S xuid=%S, id=%d, signin=%d)\n",
                    refArgs->User->DisplayInfo->Gamertag->Data(), refArgs->User->XboxUserId->Data(), refArgs->User->Id, refArgs->User->IsSignedIn));
                pRef->bRefreshUserList = TRUE;
        }));
    }
#endif // DIRTYSDK_IEAUSER_ENABLED

    // setup event to update pRef->refXboxUsers when something changes
    if(!pRef->CurrentUserChangedEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding current user changed event handler \n"));
        pRef->CurrentUserChangedEventToken = (CoreApplicationContext::CurrentUserChanged += ref new EventHandler<Platform::Object ^>(
            [=](Platform::Object ^refObj, Object ^refArgs)
        {
            if (CoreApplicationContext::CurrentUser)
            {
                NetPrintf(("netconnxboxone: current user changed event (gtag=%S xuid=%S, id=%d, signin=%d)\n",
                    CoreApplicationContext::CurrentUser->DisplayInfo->Gamertag->Data(), CoreApplicationContext::CurrentUser->XboxUserId->Data(),
                    CoreApplicationContext::CurrentUser->Id, CoreApplicationContext::CurrentUser->IsSignedIn));

                if (CoreApplicationContext::CurrentUser->IsSignedIn)
                {
                    pRef->bRetrySignIn = TRUE;
                }
            }
            else
            {
                NetPrintf(("netconnxboxone: current user changed event (signin=0)\n"));
            }
            pRef->bRefreshUserList = TRUE;
        }));
    }

    if(!pRef->NetworkStatusChangedEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding network status changed event handler \n"));
        pRef->NetworkStatusChangedEventToken = (NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler(
            [=](Platform::Object ^refObj)
        {
            NetPrintf(("netconnxboxone: NetworkStatusChangedEvent triggered. tick=%d\n", NetTick()));
            ConnectionProfile^ refConnectionProfile;
            Windows::Networking::Connectivity::NetworkConnectivityLevel ePrevConnectivityLevel = pRef->eConnectivityLevel;
            Windows::Networking::Connectivity::NetworkConnectivityLevel eNewConnectivityLevel = Windows::Networking::Connectivity::NetworkConnectivityLevel::None;

            try
            {
                refConnectionProfile = Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile();
            }
            catch (Exception ^ e)
            {
                NetPrintf(("netconnxboxone: (In Event Handler) exception caught while trying to get GetInternetConnectionProfile (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                return;
            }

            if (refConnectionProfile != nullptr)
            {
                try
                {
                    eNewConnectivityLevel = refConnectionProfile->GetNetworkConnectivityLevel();
                }
                catch (Exception ^ e)
                {
                    NetPrintf(("netconnxboxone: (In Event Handler) exception caught while trying to get NetworkConnectivityLevel (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                    return;
                }
            }

            if (ePrevConnectivityLevel != eNewConnectivityLevel)
            {
                NetPrintf(("netconnxboxone: change in connectivity level detected\n"));
                pRef->bRetryNetworkStatusNow = TRUE;
                pRef->bRetryNetworkStatusPoll = FALSE;

                // signal the end of the wait period for connectivity level queries to be allowed
                pRef->bConnLevelQueryAllowed = TRUE;
            }
        }));
    }


    // when you resume users are logged in fresh
    if (!pRef->SuspendingEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding supsending event handler \n"));
        
        pRef->SuspendingEventToken = (CoreApplication::Suspending += ref new EventHandler<Windows::ApplicationModel::SuspendingEventArgs ^>(
            [=](Platform::Object ^refObj, Platform::Object ^refObj2)
        {
            NetPrintf(("netconnxboxone: SuspendingEvent triggered   tick=%d\n", NetTick()));
            pRef->bSuspended = TRUE;
        }));
    }

    // when you resume users are logged in fresh
    if (!pRef->ResumingEventToken.Value)
    {
        NetPrintf(("netconnxboxone: adding resuming event handler \n"));

        pRef->ResumingEventToken = (CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(
            [=](Platform::Object ^refObj, Platform::Object ^refObj2)
        {
            NetPrintf(("netconnxboxone: ResumingEvent triggered. tick=%d\n", NetTick()));
           
#ifndef DIRTYSDK_IEAUSER_ENABLED 
            if (pRef->bAutoResetUserList)
            {
                NetPrintf(("netconnxboxone: automatically resetting user list.\n"));
                pRef->bResetUserList = TRUE;
            }
#endif

            // signal the beginning of the wait period for XBL calls to be allowed
            pRef->uXblCallsWaitStart = NetTick();
            pRef->bConnLevelQueryAllowed = FALSE;
            pRef->bXblCallsAllowed = FALSE;
            pRef->eConnectivityLevel = Windows::Networking::Connectivity::NetworkConnectivityLevel::None;
            pRef->bSuspended = FALSE;
        }));
    }

    // We check for internet connectivity at start up rather than setting it to None. Otherwise, if a user called NetConnShutdown and then called
    // NetConnStartup again, we'd end up with a wrong state. The NetworkStatusChanged event is not fired by OS when NetConnShutdown is followed by NetConnInit.
    // This also avoids a potential race condition when the Network Status has changed before we had a chance to add the handler.
    _UpdateNetworkStatus(pRef);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnQuery

    \Description
        Returns the index of the given gamertag. If the gamertag is not
        present, treat pList as a pointer to an XUID, and find the user by
        XUID. Otherwise return the index of the first logged in user.

    \Input *pGamertag   - gamer tag
    \Input *pList       - XUID to get index of
    \Input iListSize    - unused

    \Output
        int32_t         - negative=gamertag not logged in, else index of user

    \Version  07/20/2012 (mcorcoran)
*/
/*************************************************************************************************F*/
int32_t NetConnQuery(const char *pGamertag, NetConfigRecT *pList, int32_t iListSize)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    if (pGamertag != NULL)
    {
        // Find the matching gamertag
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (ds_strnicmp(pRef->aUsers[iIndex].strGamertag, pGamertag, sizeof(pRef->aUsers[iIndex].strGamertag)) == 0)
            {
                // We've found the right user, get their index
                return(iIndex);
            }
        }
    }
    else if (pList != NULL)
    {
        // Find the matching Xuid
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (memcmp(&pRef->aUsers[iIndex].Xuid, pList, sizeof(pRef->aUsers[iIndex].Xuid)) == 0)
            {
                // We've found the right user, get their index
                return(iIndex);
            }
        }
    }
    else
    {
        // Return the index of the first online user
        for (int32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
        {
            if (pRef->aUsers[iIndex].refXboxUser != nullptr && pRef->aUsers[iIndex].refXboxUser->IsSignedIn)
            {
                // We've found an online user, get their index
                return(iIndex);
            }
        }
    }

    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses the
        configuration returned by NetConnQuery.

    \Input *pConfig - the configuration entry from NetConnQuery
    \Input *pOption - asciiz list of config parameters (tbd)
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 12/01/2012 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    NetPrintf(("netconnxboxone: connecting...\n"));

    pRef->bSilent = TRUE;

    // make sure we aren't already connected
    if (pRef->eState == NetConnRefT::ST_INIT)
    {
        //Process Connect Options
        if (pOption != NULL)
        {
            //find out if we're doing a silent login or if we're going to bring up the signin UI if needed
            if (strstr(pOption, "silent=false") != NULL)
            {
                pRef->bSilent = FALSE;
            }
        }

        pRef->eState = NetConnRefT::ST_START_LOG;
    }
    else if (pRef->eState == NetConnRefT::ST_EXT_CLEANUP)
    {
        pRef->bDelayedConn = TRUE;
        NetPrintf(("netconnxboxone: delaying completion of NetConnConnect() until external cleanup phase is completed\n"));
    }

    else
    {
        NetPrintf(("netconnxboxone: NetConnConnect() ignored because already connected!\n"));
    }

    _UpdateNetworkStatus(pRef);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 12/01/2012 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    NetPrintf(("netconnxboxone: disconnecting...\n"));

    pRef->bDelayedConn = FALSE;
    pRef->uEnvTimer = 0;

    // shut down xhttp/xauth if started
    if (pRef->pDirtyAuth != NULL)
    {
        DirtyAuthDestroy(pRef->pDirtyAuth);
        pRef->pDirtyAuth = NULL;
    }

    if (pRef->PlatEnvAsyncOp != nullptr)
    {
        pRef->PlatEnvAsyncOp->Cancel();
        pRef->PlatEnvAsyncOp = nullptr;
    }

    // $todo: deprecate ST_EXT_CLEANUP state (but not the external cleanup mechanism)

    pRef->eState = NetConnRefT::ST_INIT;
    NetPrintf(("netconnxboxone: NetConnDisconnect transitioning to ST_INIT\n"));

    // reset status
    pRef->uConnStatus = 0;
    pRef->bRetryNetworkStatusPoll = FALSE;
    pRef->uNetworkStatusPollTimer = 0;
    pRef->uNetworkStatusPollCount = 0;
    pRef->eConnectivityLevel = Windows::Networking::Connectivity::NetworkConnectivityLevel::None;
    pRef->bConnLevelQueryAllowed = TRUE;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnControl

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
            'arul' - automatically reset the user list, on resume event (defaults to true) iValue as bool (only available when not using IEAUSER)
            'aurn' - stands for "Authenticaiton URN"; sets the URN to be used when acquiring an XSTS token
            'recu' - stands for "register for external cleanup"; add a new entry to the external cleanup list
            'ruli' - reset the user list (only available when not using IEAUSER)
            'snam' - set DirtyCert service name
            'spam' - set debug output verbosity (0...n)
            'tick' - initiate a new XSTS token acquisition for the user index specified in iValue; <0=error, 0=retry (a request is already pending), >0=request issued
            'xblu' - stands for "xbl update"; force an immediate update of the different users' signin state
            'xtag' - retrieve gamertag from xuid. pValue: XUID, pValue2: user-provided pData
            'xtcb' - set pointer(pValue) to callback for retrieving gamertag from XUID
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 12/01/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnxboxone: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
     }

#ifndef DIRTYSDK_IEAUSER_ENABLED 
    // 'arul' - automatically reset the user list, on resume event (defaults to true) iValue as bool
    if (iControl == 'arul')
    {

        pRef->bAutoResetUserList = iValue;
        return(0);
    }
#endif

    // sets the URN
    if (iControl == 'aurn')
    {
        ds_strnzcpy(pRef->strAuthUrn, (const char *)pValue, sizeof(pRef->strAuthUrn));
        NetPrintf(("netconnxboxone: authentication URN set to %s\n", pRef->strAuthUrn));
        return(0);
    }

    // register for external cleanup
    if (iControl == 'recu')
    {
        return(_NetConnAddToExternalCleanupList(pRef, (NetConnExternalCleanupCallbackT)pValue, pValue2));
    }

#ifndef DIRTYSDK_IEAUSER_ENABLED 
    //'ruli' - reset the user list
    if (iControl == 'ruli')
    {
        pRef->bResetUserList = TRUE;
        return(0);
    }
#endif

    // request refresh of XSTS token
    if (iControl == 'tick')
    {
        int32_t iUserIndex = iValue;
        int32_t iRetCode = 0;  // default to "try again"

        if (iUserIndex < NETCONN_MAXLOCALUSERS)
        {
            NetPrintf(("netconnxboxone: NetConnControl 'tick' for user index %d  (URN = %s)\n", iUserIndex, pRef->strAuthUrn));

            switch (pRef->aUsers[iUserIndex].token.eTokenStatus)
            {
                // token acquisition is currently in progress
                case ST_TOKEN_INPROG:
                    break;

                default:
                    _NetConnRequestAuthToken(pRef, iUserIndex);
                    iRetCode = 1;  // signal success
                    break;
            }
        }
        else
        {
            NetPrintf(("netconnxboxone: NetConnControl 'tick' for invalid user index %d\n", iUserIndex));
            iRetCode = -1;  // signal failure
        }

        return(iRetCode);
    }

    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }

    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("netconnxboxone: changing debug level from %d to %d\n", pRef->Common.iDebugLevel, iValue));
        pRef->Common.iDebugLevel = iValue;
        return(0);
    }
    #endif

    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F*************************************************************************************************/
/*!
    \Function NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - status selector
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            addr: ip address of client
            bbnd: broadband true or false
            chat: TRUE if local user cannot chat, else FALSE
            conn: true/false indication of whether connection in progress
            cuis: convert a user index (iData) to a system index, returns the index or a negative value for errors
            csiu: convert a system index (iData) to a user index, returns the index or a negative value for errors
            envi: EA back-end environment type in use (-1=not available, NETCONN_PLATENV_DEV, NETCONN_PLATENV_TEST, NETCONN_PLATENV_CERT, NETCONN_PLATENV_PROD), and sandbox name via pBuf if provided (32 bytes)
            ethr: mac address of primary adapter (returned in pBuf), 0=success, negative=error
            gtag: gamertag returned in pBuf, 0=success, negative=error
            gust: returns 1 if the given user is a guest, returns 0 for a normal player, returns a negative value is the operation failed.
            locl: return locality (Windows locale) for local system, ex. 'enUS'
                  For unrecognized locale, 'zzZZ' will be returned.
            locn: return location On Xbox One this is the same as locl
            macx: mac address of primary adapter (returned in pBuf), 0=success, negative=error
            mask: returns the mask of user that are logged in.
            mgrp: return memory group and user data pointer
            ncon: true/false indication of whether network connectivity is at least Internet.
            nste: enum indicating Windows::Networking::Connectivity::NetworkConnectivityLevel.
            natt: returns the NAT Type as an int32_t (returned in pBuf). If pBuf is not specified the NAT type will be returned in the return value
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            tick: fills pBuf with XSTS Token if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf
            tria: returns value indicating if the running title has a trial license. return value: -1:"failed to fetch trial info"; 0:not trial; 1:trial
            type: NETCONN_IFTYPE_*
            vers: return DirtySDK version
            xadd: >0:number of bytes (local SecureDeviceAddress blob) copied in pBuf, negative=error
            xcon: true/false indicating if we have xbox live access
            xlan: retrieve the language code of the active user as a char * (returned in pBuf). pBuf must be at least be NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH in length.
            xnfo: (deprecated) return status of user account info (gamertag and xuid) for a given user index (iData)
            xtry: retrieve the country code of the active user as a char* (returned in pBuf). pBuf must be at least be NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH in length.
            xuid: xuid of client returned in pBuf, 0=success, negative=error
            xusr: retrieves a reference to the User object at user index iData, pBuf is a pointer to the (User^). 0=success, negative=error
            iusr: return the index of the IEAUser::GetUserId() passed via iData. Should be called from the thread that calls NetConnIdle(). negative=error
        \endverbatim

    \Version 12/01/2012 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }
    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYVERS);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnxboxone: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return memory group and user data pointer
    if (iKind == 'mgrp')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(void *)))
        {
            *((void **)pBuf) = pRef->Common.pMemGroupUserData;
        }
        return(pRef->Common.iMemGroup);
    }

    // return whether we are broadband or not
    if (iKind == 'bbnd')
    {
        // assume broadband
        return(TRUE);
    }

    // return ability of local user to chat
    if (iKind == 'chat')
    {
        return(FALSE);  // chat is not disabled
    }

    // return connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }

    // return netconn user index from system index
    if (iKind == 'csiu')
    {
        return(_NetConnSystemIndexToUserIndex(pRef, iData));
    }

    // return system index from netconn user index
    if (iKind == 'cuis')
    {
        return(_NetConnUserIndexToSystemIndex(pRef, iData));
    }

    // EA back-end environment type in use
    if (iKind == 'envi')
    {
        if (pRef->uConnStatus == '+onl')
        {
            if (pBuf != NULL)
            {
                if (iBufSize >= sizeof(pRef->strSandboxId))
                {
                    ds_strnzcpy((char*)pBuf, pRef->strSandboxId, iBufSize);
                }
                else
                {
                    NetPrintf(("netconnxboxone: insufficient buffer space provided to 'envi' %d required\n", sizeof(pRef->strSandboxId)));
                }
            }

            return(pRef->uPlatEnv);
        }
        return(-1);
    }

    // return host mac address
    if ((iKind == 'ethr') || (iKind == 'macx'))
    {
        NetPrintf(("netconnxboxone: mac address query is currently unsupported on xboxone\n"));
        return(-1);
    }

    // return email address
    if (iKind == 'mail')
    {
        NetPrintf(("netconnxboxone: NetConnStatus('mail') is unsupported on xboxone\n"));
        return(-1);
    }

    //mask: returns the mask of user that are logged in.
    if (iKind == 'mask')
    {
        return(_NetConnXblUserMask(pRef));
    }

    // return logged in gamertag
    if (iKind == 'gtag')
    {
        if (iData < 0 || iData >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            return(-1);
        }

        if ((pBuf == NULL) || (iBufSize < sizeof(pRef->aUsers[iData].strGamertag)))
        {
            // invalid buffer or buffer too small
            return(-2);
        }

        if (pRef->aUsers[iData].strGamertag[0] == '\0')
        {
            // no valid gamertag at the specified user index
            return(-3);
        }

        // fill user-provided buffer with gamertag
        ds_memcpy(pBuf, pRef->aUsers[iData].strGamertag, sizeof(pRef->aUsers[iData].strGamertag));

        return(0);
    }

    // reutrn 1 if the user is a guest user , 0 if user is a normal user
    if (iKind == 'gust')
    {
        if ((iData < 0) || (iData >= NETCONN_MAXLOCALUSERS))
        {
            // index out of range
            NetPrintf(("netconnxboxone: netconnstatus('gust') user index out of range\n"));
            return(-1);
        }

        if (pRef->aUsers[iData].refXboxUser != nullptr)
        {
            if (pRef->aUsers[iData].refXboxUser->IsGuest)
            {
                return(1);
            }
            else
            {
                return(0);
            }
        }
        
        NetPrintf(("netconnxboxone: netconnstatus('gust') failed for user at index %i\n", iData));
        return(-2);
    }

    // return Windows locale information
    if (iKind == 'locl')
    {
        char country[NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH];
        char language[NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH];
        int32_t iIndex = 0;
        int32_t iLocale = 0;

        if ((NetConnStatus('xtry', 0 , country, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH) >= 0) && (NetConnStatus('xlan', 0, language, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH) >= 0))
        {
            for(iIndex = 0; iIndex < NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH - 1; ++iIndex)
            {
                iLocale += language[iIndex] << (24 - (iIndex * 8));
            }

            for(iIndex = 0; iIndex < NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH - 1; ++iIndex)
            {
                iLocale += country[iIndex] << (8 - (iIndex * 8));
            }

            return(iLocale);
        }
        else
        {
            NetPrintf(("netconnxboxone: NetConnStatus('locl') Unable to retrieve language/country code.\n"));
            return(LOBBYAPI_LOCALITY_UNKNOWN);
        }
    }

    // return Windows location information
    if (iKind == 'locn')
    {
        //On the Xbox One locn is the same as locl
        return(NetConnStatus('locl', 0, NULL, 0));
    }

    // return NAT Type info
    if (iKind == 'natt')
    {
        int32_t iResult = 0;

        try
        {
            SecureDeviceAddress ^refSecureDeviceAddr = SecureDeviceAddress::GetLocal();
            iResult = (int32_t) refSecureDeviceAddr->NetworkAccessType;

        }
        catch (Exception ^ e)
        {
            NetPrintf(("netconnxboxone:  NetConnStatus('natt') exception caught (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            iResult = -2;
        }

        if (pBuf != NULL  && iResult >= 0)
        {
            if(iBufSize == sizeof(int32_t))
            {
                ds_memcpy(pBuf, &iResult, iBufSize);
                iResult = 0;
            }
            else
            {
                NetPrintf(("netconnxboxone:  NetConnStatus('natt') Invalid arguments.\n"));
                iResult = -1;
            }
        }

        return(iResult);
    }

    // check network connectivity
    if (iKind == 'ncon')
    {
        return (int32_t)(pRef->eConnectivityLevel >= Windows::Networking::Connectivity::NetworkConnectivityLevel::InternetAccess);
    }

    //return network status enum provided by the OS. This allows the user to get finer details if needed (say make a distinction between internet vs xbox live connection).
    if(iKind == 'nste')
    {
        return (int32_t)(pRef->eConnectivityLevel);
    }

    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->uConnStatus == '+onl');
    }

    // return ticket info (if available)
    if (iKind == 'tick')
    {
        uint8_t bNeedRefresh = FALSE;   // default to "no need to refresh ticket"
        uint8_t bCacheValid = FALSE;    // default to "ticket cache content is not valid"
        int32_t iResult = 0;            // default to "try again"
        int32_t iUserIndex = iData;
        int32_t iTokenLen = 0;

        if (iUserIndex >= NETCONN_MAXLOCALUSERS || iUserIndex < 0)
        {
            NetPrintf(("netconnxboxone: NetConnStatus 'tick' for invalid user index %d\n", iUserIndex));
            return(-1);
        }

        NetPrintf(("netconnxboxone: NetConnStatus 'tick' for user index %d  (URN = %s)\n", iUserIndex, pRef->strAuthUrn));

        NetCritEnter(&pRef->aUsers[iUserIndex].asyncOpCrit);

        switch (pRef->aUsers[iUserIndex].token.eTokenStatus)
        {
            // token has not been requested yet
            case ST_TOKEN_INVALID:
                bNeedRefresh = TRUE;
                break;

            // token aquisition failed
            case ST_TOKEN_FAILED:
                if (pRef->aUsers[iUserIndex].token.bFailureReported == TRUE)
                {
                    // attempt a new token request
                    pRef->aUsers[iUserIndex].token.bFailureReported = FALSE;
                    pRef->aUsers[iUserIndex].token.eTokenStatus = ST_TOKEN_INVALID;
                    bNeedRefresh = TRUE;
                }
                else
                {
                    // report the error to the xsts token server
                    pRef->aUsers[iUserIndex].token.bFailureReported = TRUE;
                    iResult = -1;
                }
                break;

            // token acquisition is currently in progress
            case ST_TOKEN_INPROG:
                break;

            // token is valid
            case ST_TOKEN_VALID:
                if (NetTickDiff(NetTick(), pRef->aUsers[iUserIndex].token.iTokenAcquTick) > NETCONNXBOXONE_TOKEN_TIMEOUT)
                {
                    NetPrintf(("netconnxboxone: ticket cache for user %d expired, refreshing now...\n", iUserIndex));
                    bNeedRefresh = TRUE;
                }
                else if (DirtyAuthCheckToken(iUserIndex, pRef->strAuthUrn, &iTokenLen, NULL) != DIRTYAUTH_SUCCESS)
                {
                    bNeedRefresh = TRUE;
                }
                else
                {
                    bCacheValid = TRUE;
                }
                break;

            default:
                NetPrintf(("netconnxboxone: critical error - invalid token status \n"));
                iResult = -1;
                break;
        }

        // return cached ticket to caller if possible
        if (bCacheValid)
        {
            // add one to the token to guarantee null termination
            iResult = iTokenLen + 1;

            if (pBuf)
            {
                char *pTokenBuf = (char *)pBuf;

                if (iBufSize >= (iTokenLen + 1))
                {
                    // retrieve cached token
                    DirtyAuthCheckToken(iUserIndex, pRef->strAuthUrn, NULL, (char *)pTokenBuf);

                    // null terminate
                    pTokenBuf[iTokenLen] = '\0';

                    // log token
                    NetPrintWrap((const char *)pTokenBuf, 132);
                }
                else
                {
                    NetPrintf(("netconnxboxone: user-provided buffer is too small (%d/%d)\n", iBufSize, (iTokenLen + 1)));
                    iResult = -1;
                }
            }
        }

        NetCritLeave(&pRef->aUsers[iUserIndex].asyncOpCrit);

        // initiate a ticket cache refresh if needed
        if (bNeedRefresh)
        {
            _NetConnRequestAuthToken(pRef, iUserIndex);
        }

        return(iResult);
    }

    if (iKind == 'tria')
    {
        int32_t iResult = -1;
        try
        {
            if (Windows::ApplicationModel::Store::CurrentApp::LicenseInformation->IsTrial)
            {
                iResult = 1;
            }
            else
                iResult = 0;
        }
        catch(...)
        {
            NetPrintf(("Windows::ApplicationModel::Store::CurrentApp::LicenseInformation->IsTrial failed\n"));
        }

        return(iResult);
    }

    // return what type of interface we are connected with
    if (iKind == 'type')
    {
        // assume broadband
        return(NETCONN_IFTYPE_ETHER);
    }

    // return local SecureDeviceAddress blob (use pBuf=NULL to only query blob size)
    if (iKind == 'xadd')
    {
        int32_t iResult = 0;

        try
        {
            SecureDeviceAddress ^refSecureDeviceAddr = SecureDeviceAddress::GetLocal();
            IBuffer ^refSecureDeviceAddressBuffer = refSecureDeviceAddr->GetBuffer();

            if (pBuf != NULL)
            {
                if ((unsigned)iBufSize >= refSecureDeviceAddressBuffer->Length)
                {
                    DataReader ^refDataReader = DataReader::FromBuffer(refSecureDeviceAddressBuffer);
                    refDataReader->ReadBytes(ArrayReference<uint8_t>((uint8_t *)pBuf, refSecureDeviceAddressBuffer->Length));
                    iResult = refSecureDeviceAddressBuffer->Length;

                    NetPrintMem(pBuf, refSecureDeviceAddressBuffer->Length, "local-secure-addr-blob");
                }
                else
                {
                    NetPrintf(("netconnxboxone: local SecureDeviceAddress blob (%d bytes) does not fit in user's buffer (%d bytes)\n",
                        refSecureDeviceAddressBuffer->Length, iBufSize));
                    iResult = -1;
                }
            }
        }
        catch (Exception ^ e)
        {
            NetPrintf(("netconnxboxone: exception caught while trying to get local SecureDeviceAddress blob (%S/0x%08x)\n",
                e->Message->Data(), e->HResult));
            iResult = -2;
        }

        return(iResult);
    }

    //Check for Xbox Live Connectivity
    if (iKind == 'xcon')
    {
        return (int32_t)(pRef->eConnectivityLevel >= Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess);
    }

    //return xbox one language code
    if (iKind == 'xlan')
    {
        wchar_t langCode[NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH];

        if (pBuf == NULL)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xlan') null buffer passed in.\n"));
            return(-1);
        }
        else if (iBufSize < NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xlan') buffer size is too small.\n"));
            return(-2);
        }
        
        if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SISO639LANGNAME, langCode, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH) <= 0)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xlan') cannot retrieve the language code.\n"));
            return(-3);
        }

        wcstombs((char *)pBuf, langCode, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH);
        
        return(0);
    }

    // (deprecated) return status of user account info (gamertag and xuid)
    if (iKind == 'xnfo')
    {
        if (iData < 0 || iData >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            return(-1);
        }

        if (pRef->aUsers[iData].strGamertag[0] == '\0')
        {
            // no valid gamertag at the specified user index
            return(-2);
        }

        return(0);
    }

    //return xbox one country code
    if (iKind == 'xtry')
    {
        wchar_t countryCode[NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH];

        if (pBuf == NULL)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xtry') null buffer passed in.\n"));
            return(-1);
        }
        else if (iBufSize < NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xtry') buffer size is too small.\n"));
            return(-2);
        }

        if (GetGeoInfoW(GetUserGeoID(GEOCLASS_NATION), GEO_ISO2, countryCode, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH, 0) <= 0)
        {
            NetPrintf(("netconnxboxone: NetConnStatus('xtry') cannot retrieve the country code.\n"));
            return(-3);
        }

        wcstombs((char *)pBuf, countryCode, NETCONNXBOXONE_STR_COUNTRY_LANG_CODE_LENGTH);

        return(0);
    }

    // return xuid
    if (iKind == 'xuid')
    {
        if (iData < 0 || iData >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            return(-1);
        }

        if ((pBuf == NULL) || (iBufSize < sizeof(pRef->aUsers[iData].Xuid)))
        {
            // invalid buffer or buffer too small
            return(-2);
        }

        if (pRef->aUsers[iData].Xuid == 0)
        {
            // no valid XUID at the specified user index
            return(-3);
        }

        ds_memcpy(pBuf, &pRef->aUsers[iData].Xuid, sizeof(pRef->aUsers[iData].Xuid));
        return(0);
    }

    if (iKind == 'xusr')
    {
        int32_t iRetValue = 0;

        NetCritEnter(&pRef->aUsers[iData].userCrit);

        if (iData < 0 || iData >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            iRetValue = -1;
        }

        if ((pBuf == NULL) || (iBufSize != sizeof(pRef->aUsers[iData].refXboxUser)))
        {
            // invalid buffer or buffer too small
            iRetValue = -2;
        }

        if (pRef->aUsers[iData].refXboxUser == nullptr)
        {
            // no user at specified index
            iRetValue = -3;
        }

        if (iRetValue == 0)
        {
            *((User^*)pBuf) = pRef->aUsers[iData].refXboxUser;
        }

        NetCritLeave(&pRef->aUsers[iData].userCrit);

        return(iRetValue);
    }

    #ifdef DIRTYSDK_IEAUSER_ENABLED 
    if (iKind == 'iusr')
    {
        return(_NetConnQueryIEAUser(pRef, (uint32_t)iData));
    }
    #endif

    // pass unrecognized options to SocketInfo
    return(SocketInfo(NULL, iKind, iData, pBuf, iBufSize));
}

/*F*************************************************************************************************/
/*!
    \Function     NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input bStopEE  - ignored in xbox one implementation

    \Output
        int32_t     - negative=error, zero=success

    \Version 12/01/2012 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetConnShutdown(uint32_t bStopEE)
{
    NetConnRefT *pRef = (NetConnRefT *)NetConnCommonGetRef();

    // error see if already stopped
    if (pRef == NULL)
    {
        return(NETCONN_ERROR_NOTACTIVE);
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

    // make sure external cleanup list is empty before proceeding
    if (pRef->iExternalCleanupListCnt != 0 || pRef->bPlatAsyncOpStarted != 0)
    {
        #if DIRTYCODE_LOGGING
        static uint32_t uLastTime = 0;
        if (!uLastTime || NetTickDiff(NetTick(), uLastTime) > 1000)
        {
            NetPrintf(("netconnxboxone: deferring shutdown while external cleanup list is not empty (%d items)\n", pRef->iExternalCleanupListCnt));
            uLastTime = NetTick();
        }
        #endif

        // signal "try again"
        NetPrintf(("netconnxboxone: deferring shutdown while external cleanup list is not empty (%d items)\n", pRef->iExternalCleanupListCnt));
        return(NETCONN_ERROR_ISACTIVE);
    }

    // now free the cleanup list memory
    if (pRef->pExternalCleanupList != NULL)
    {
        DirtyMemFree(pRef->pExternalCleanupList, NETCONN_MEMID, pRef->Common.iMemGroup, pRef->Common.pMemGroupUserData);
    }

    if(pRef->NetworkStatusChangedEventToken.Value)
    {
        NetworkInformation::NetworkStatusChanged -= pRef->NetworkStatusChangedEventToken;
        pRef->NetworkStatusChangedEventToken.Value = 0;
    }

    if(pRef->SuspendingEventToken.Value)
    {
        CoreApplication::Suspending -= pRef->SuspendingEventToken;
        pRef->SuspendingEventToken.Value = 0;
    }

    if(pRef->ResumingEventToken.Value)
    {
        CoreApplication::Resuming -= pRef->ResumingEventToken;
        pRef->ResumingEventToken.Value = 0;
    }

    if(pRef->CurrentUserChangedEventToken.Value)
    {
        CoreApplicationContext::CurrentUserChanged -= pRef->CurrentUserChangedEventToken;
        pRef->CurrentUserChangedEventToken.Value = 0;
    }

#ifndef DIRTYSDK_IEAUSER_ENABLED
    if(pRef->UserAddedEventToken.Value)
    {
        User::UserAdded -= pRef->UserAddedEventToken;
        pRef->UserAddedEventToken.Value = 0;
    }
    if(pRef->UserRemovedEventToken.Value)
    {
        User::UserRemoved -= pRef->UserRemovedEventToken;
        pRef->UserAddedEventToken.Value = 0;
    }
    // release the array of Xbox Users
    if (pRef->refXboxUsers != nullptr)
    {
        pRef->refXboxUsers = nullptr;
    }
#endif

    // shut down protossl
    ProtoSSLShutdown();

    // shut down dirtycert
    DirtyCertDestroy();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, pRef);

    // shut down Idle handler
    NetConnIdleShutdown();

    // disconnect the interfaces
    NetConnDisconnect();

    // shut down dirtysock
    SocketDestroy(0);

    for (int32_t iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        _NetConnClearUser(pRef, iUserIndex);
        // release critical section
        if (pRef->aUsers[iUserIndex].bCritInitialized)
        {
            NetCritKill(&pRef->aUsers[iUserIndex].asyncOpCrit);
            NetCritKill(&pRef->aUsers[iUserIndex].userCrit);
            pRef->aUsers[iUserIndex].bCritInitialized = FALSE;
        }
    }

    // common shutdown (must come last as this frees the memory)
    NetConnCommonShutdown(&pRef->Common);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application (yield thread) for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 12/01/2012 (jbrookes)
*/
/*************************************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    Sleep((DWORD)iMilliSecs);
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
