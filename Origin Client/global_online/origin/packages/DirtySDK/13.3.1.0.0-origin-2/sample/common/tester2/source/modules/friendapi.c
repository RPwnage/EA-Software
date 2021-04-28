/*H*************************************************************************************/
/*!
    \File    friendapi.c

    \Description
        Reference application for the Friend Api module.

    \Copyright
        Copyright (c) Electronic Arts 2011.    ALL RIGHTS RESERVED.

    \Version 03/07/2011 (mclouatre)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/
#include <string.h>
#include <stdio.h>   // for sscanf()

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_PS3)
#include <np.h>
#elif defined(DIRTYCODE_XENON)
#include <xtl.h>
#endif

#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "DirtySDK/friend/friendapi.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct FriendApiAppT
{
    int32_t iUserIndex;

    uint8_t bStarted;

    uint8_t bIsBlockedInProgress;
    uint8_t bBlockInProgress;
    uint8_t bUnblockInProgress;

#if defined(DIRTYCODE_XENON)
    XUID xuid;
#elif defined(DIRTYCODE_PS3)
    SceNpId npId;
    SceNpOnlineId onlineId;

    int32_t npLookupTitleCtx;
    int32_t npLookupTransactionCtx;
#endif
} FriendApiAppT;

/*** Function Prototypes ***************************************************************/

static void _FriendApiBlockUser(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiGetBlockList(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiGetBlockListVersion(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiGetFriendsList(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiGetFriendsListVersion(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiIsUserBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiStart(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiStop(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiUnblockUser(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _FriendApiNotificationCb(FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData, void *pNotifyCbUserData);


/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _FriendApi_Commands[] =
{
    { "block",               _FriendApiBlockUser             },
    { "getblocklist",        _FriendApiGetBlockList          },
    { "getblocklistver",     _FriendApiGetBlockListVersion   },
    { "getfrndslist",        _FriendApiGetFriendsList        },
    { "getfrndslistver",     _FriendApiGetFriendsListVersion },
    { "isblocked",           _FriendApiIsUserBlocked         },
    { "start",               _FriendApiStart                 },
    { "stop",                _FriendApiStop                  },
    { "unblock",             _FriendApiUnblockUser           },
    { "",                    NULL                            }
};

static FriendApiAppT _FriendApi_App;

static BlockedUserDataT _BlockList[FRIENDAPI_MAX_BLOCKED_USERS];
static FriendDataT _FriendsList[FRIENDAPI_MAX_FRIENDS];
#if defined(DIRTYCODE_XENON)
static XONLINE_FRIEND _XenonFriendsData[FRIENDAPI_MAX_FRIENDS];
#endif

// Public variables

/*** Private Functions *****************************************************************/

#if defined(DIRTYCODE_PS3)
/*F*************************************************************************************/
/*!
    \Function _InitNpLookupUtility

    \Description
        Init NP Lookup utility

    \Input *pApp    - pointer to FriendApi app

    \Output
        int32_t     - success=0; failure=-1

    \Version 03/10/2011 (mclouatre)
*/
/**************************************************************************************F*/
static int32_t _InitNpLookupUtility(FriendApiAppT *pApp)
{
    int32_t iResult;
    SceNpCommunicationId npCommId;
    SceNpId selfNpId;

    iResult = sceNpLookupInit();
    if (iResult != 0)
    {
        ZPrintf("   sceNpLookupInit() failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }
    iResult = NetConnStatus('npcm', 0, &npCommId, sizeof(npCommId));
    if (iResult != 0)
    {
        ZPrintf("   NetConnStatus('npcm') failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }
    iResult = NetConnStatus('npid', 0, &selfNpId, sizeof(selfNpId));
    if (iResult != 0)
    {
        ZPrintf("   NetConnStatus('npid') failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }
    iResult = sceNpLookupCreateTitleCtx(&npCommId, &selfNpId);
    if (iResult >= 0)
    {
        pApp->npLookupTitleCtx = iResult;
    }
    else
    {
        ZPrintf("   sceNpLookupCreateTitleCtx() failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _ShutdownNpLookup

    \Description
        Shutdown NP Lookup utility

    \Input *pApp    - pointer to FriendApi app

    \Version 03/10/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _ShutdownNpLookupUtility(FriendApiAppT *pApp)
{
    int32_t iResult;

    if (pApp->npLookupTitleCtx)
    {
        iResult = sceNpLookupDestroyTitleCtx(pApp->npLookupTitleCtx);
        if (iResult != 0)
        {
            ZPrintf("   sceNpLookupDestroyTitleCtx() failed with %s\n", DirtyErrGetName(iResult));
        }
    }
    iResult = sceNpLookupTerm();
    if (iResult != 0)
    {
        ZPrintf("   sceNpLookupTerm() failed with %s\n", DirtyErrGetName(iResult));
    }
}

/*F*************************************************************************************/
/*!
    \Function _StartNpIdLookup

    \Description
        Start an async req to get NP id from Online id.

    \Input *pApp        - pointer to FriendApi app

    \Output
        int32_t         - success=0; failure=-1

    \Version 03/10/2011 (mclouatre)
*/
/**************************************************************************************F*/
static int32_t _StartNpIdLookup(FriendApiAppT *pApp)
{
    int32_t iResult;

    iResult = sceNpLookupCreateTransactionCtx(pApp->npLookupTitleCtx);
    if (iResult >= 0)
    {
        pApp->npLookupTransactionCtx = iResult;
    }
    else
    {
        ZPrintf("   sceNpLookupCreateTransactionCtx() failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }

    iResult = sceNpLookupNpIdAsync(pApp->npLookupTransactionCtx, &pApp->onlineId, &pApp->npId, 0, NULL);
    if (iResult != 0)
    {
        ZPrintf("   sceNpLookupNpIdAsync() failed with %s\n", DirtyErrGetName(iResult));
        return(-1);
    }

    return(0);
}


/*F*************************************************************************************/
/*!
    \Function _FinishNpIdLookup

    \Description
        If async op completed, destroy transaction context

    \Input *pApp    - pointer to FriendApi app

    \Version 03/10/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FinishNpIdLookup(FriendApiAppT *pApp)
{
    int32_t iResult;

    iResult = sceNpLookupDestroyTransactionCtx(pApp->npLookupTransactionCtx);
    if (iResult != 0)
    {
        ZPrintf("   sceNpLookupDestroyTransactionCtx() failed with %s\n", DirtyErrGetName(iResult));
    }
}
#endif

/*F*************************************************************************************/
/*!
    \Function _IsCmdInProgress()

    \Description
        Checks whether a command is already in progress.

    \Input *pApp    - pointer to FriendApi app

    \Output
        int32_t     - TRUE or FALSE

    \Version 03/10/2011 (mclouatre)
*/
/**************************************************************************************F*/
static int32_t _IsCmdInProgress(FriendApiAppT *pApp)
{
    return(pApp->bIsBlockedInProgress || pApp->bBlockInProgress || pApp->bUnblockInProgress);
}

/*F*************************************************************************************/
/*!
    \Function _CmdFriendApiCb

    \Description
        FriendApi idle callback.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t     - zero

    \Version 11/03/2010 (mclouatre)
*/
/**************************************************************************************F*/
static int32_t _CmdFriendApiCb(ZContext *argz, int32_t argc, char **argv)
{
    FriendApiAppT *pApp = &_FriendApi_App;
    FriendApiRefT *pFriendApiRef;
    void *pUserId = NULL;

#if defined(DIRTYCODE_PS3)
    int32_t iAsyncResult = 0;
    int32_t iResult;
    uint8_t bFail = FALSE;

    if (_IsCmdInProgress(pApp))
    {
        iResult = sceNpLookupPollAsync(pApp->npLookupTransactionCtx, &iAsyncResult);
        if (iResult == 1)
        {
            // keep waiting
            return(ZCallback(_CmdFriendApiCb, 1000));
        }
        else if (iResult < 0)
        {
            ZPrintf("   sceNpLookupPollAsync() failed with %s\n", DirtyErrGetName(iResult));
            bFail = TRUE;
        }
        else
        {
            if (iAsyncResult < 0)
            {
                ZPrintf("   sceNpLookupPollAsync() failed asynchronously with %s\n", DirtyErrGetName(iAsyncResult));
                bFail = TRUE;
            }
        }

        _FinishNpIdLookup(pApp);

        if (bFail)
        {
            pApp->bIsBlockedInProgress = pApp->bBlockInProgress = pApp->bUnblockInProgress = FALSE;
            return(0);
        }

        pUserId = (SceNpId *)&pApp->npId;
    }
#elif defined(DIRTYCODE_XENON)
    pUserId = (XUID *)&pApp->xuid;
#endif

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    if (pApp->bIsBlockedInProgress)
    {
        // query FriendApi
        if (FriendApiIsUserBlocked(pFriendApiRef, pApp->iUserIndex, pUserId))
        {
            ZPrintf("   user is blocked\n");
        }
        else
        {
            ZPrintf("   user is not blocked\n");
        }

        pApp->bIsBlockedInProgress = FALSE;
    }

    if (pApp->bBlockInProgress)
    {
        // block user
        if (FriendApiBlockUser(pFriendApiRef, pApp->iUserIndex, pUserId) == 0)
        {
            ZPrintf("   successfully initiated blocking of user %s\n", pUserId);
        }
        else
        {
            ZPrintf("   failed to initiate blocking of user %s\n", pUserId);
            pApp->bBlockInProgress = FALSE;
        }
    }

    if (pApp->bUnblockInProgress)
    {
        // unblock user
        if (FriendApiUnblockUser(pFriendApiRef, pApp->iUserIndex, pUserId) == 0)
        {
            ZPrintf("   successfully unblocked user %s\n", pUserId);
        }
        else
        {
            ZPrintf("   failed to unblock user %s\n", pUserId);
        }

        pApp->bUnblockInProgress = FALSE;
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiGetBlockListVersion

    \Description
        FriendApi subcommand - query blocklist version

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 06/01/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiGetBlockListVersion(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;
    uint32_t uVersion;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getblocklistversion\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // query blocklist version
    if(FriendApiGetBlockListVersion(pFriendApiRef, &uVersion) < 0)
    {
        ZPrintf("   FriendApi failed to retrieve blocklist version\n");
        return;
    }

    ZPrintf("   Blocklist version is %d\n", uVersion);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiGetBlockList

    \Description
        FriendApi subcommand - query blocklist

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 06/01/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiGetBlockList(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;
    int32_t iBlockedUsersCount;
    int32_t i;
    int32_t iUserIndex = 0;

#if defined(DIRTYCODE_XENON)
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getblocklist <user index [0,3]>\n", argv[0]);
        return;
    }
#else
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getblocklist\n", argv[0]);
        return;
    }
#endif

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

#if defined(DIRTYCODE_XENON)
    sscanf(argv[2], "%d", &iUserIndex);
#endif

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // query blocklist entry count
    FriendApiGetBlockList(pFriendApiRef, iUserIndex, NULL, &iBlockedUsersCount);

    // query blocklist
    FriendApiGetBlockList(pFriendApiRef, iUserIndex, _BlockList, &iBlockedUsersCount);

    if (!iBlockedUsersCount)
    {
        ZPrintf("   Blocklist is empty\n");
    }
    else
    {
        for (i=0; i<iBlockedUsersCount; i++)
        {
            ZPrintf("   Blocklist entry #%d: %s\n", i, _BlockList[i].strBlockedUserId);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiGetFriendsListVersion

    \Description
        FriendApi subcommand - query friendslist version

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 06/01/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiGetFriendsListVersion(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;
    uint32_t uVersion;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getfrndslistversion\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // query blocklist version
    if(FriendApiGetFriendsListVersion(pFriendApiRef, &uVersion) < 0)
    {
        ZPrintf("   FriendApi failed to retrieve friends list version\n");
        return;
    }

    ZPrintf("   Friendslist version is %d\n", uVersion);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiGetFriendsList

    \Description
        FriendApi subcommand - query friendslist

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 06/01/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiGetFriendsList(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;
    int32_t iFriendsCount;
    int32_t i;
    int32_t iUserIndex = 0;

#if defined(DIRTYCODE_XENON)
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getfrndslist <user index [0,3]>\n", argv[0]);
        return;
    }
#else
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getfrndslist\n", argv[0]);
        return;
    }
#endif

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

#if defined(DIRTYCODE_XENON)
    sscanf(argv[2], "%d", &iUserIndex);
#endif

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // query friendslist entry count
    FriendApiGetFriendsList(pFriendApiRef, iUserIndex, NULL, NULL, &iFriendsCount);

    // query friendslist
#if defined(DIRTYCODE_XENON)
    FriendApiGetFriendsList(pFriendApiRef, iUserIndex, _FriendsList, _XenonFriendsData, &iFriendsCount);
#else
    FriendApiGetFriendsList(pFriendApiRef, iUserIndex, _FriendsList, NULL, &iFriendsCount);
#endif

    if (!iFriendsCount)
    {
        ZPrintf("   Friendslist is empty\n");
    }
    else
    {
        for (i=0; i<iFriendsCount; i++)
        {
            ZPrintf("   Friendslist entry #%d: %s\n", i, _FriendsList[i].strFriendName);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiIsUserBlocked

    \Description
        FriendApi subcommand - query whether given user is blocked or not

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiIsUserBlocked(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
#if defined(DIRTYCODE_PS3)
    FriendApiRefT *pFriendApiRef;
#endif

#if defined(DIRTYCODE_XENON)
    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s isblocked <user index [0,3]> <base-10 XUID string>\n", argv[0]);
        return;
    }
#else
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s isblocked <online id string>\n", argv[0]);
        return;
    }
#endif

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    if (_IsCmdInProgress(pApp))
    {
        ZPrintf("   NP ID lookup in progress... please try again later\n");
        return;
    }

#if defined(DIRTYCODE_XENON)
    sscanf(argv[2], "%d", &pApp->iUserIndex);
    sscanf(argv[3], "%I64d", &pApp->xuid);
#elif defined(DIRTYCODE_PS3)
    pApp->iUserIndex = 0;

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // wait for PS3 system to signal end of initial presence
    if (FriendApiStatus(pFriendApiRef, 'eoip', NULL, 0) == FALSE)
    {
        ZPrintf("   another friendapi command already in progress... please try again later\n");
        return;
    }

    memset(&pApp->onlineId, 0, sizeof(pApp->onlineId));
    ds_strnzcpy(pApp->onlineId.data, argv[2], sizeof(pApp->onlineId.data) + 1);

    if (_StartNpIdLookup(pApp) < 0)
    {
        return;
    }
#endif

    ZPrintf("   initiated block status query for user %s ... command in progress\n", argv[2]);

    pApp->bIsBlockedInProgress = TRUE;

    // install callback
    ZCallback(_CmdFriendApiCb, 1000);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiBlockUser

    \Description
        FriendApi subcommand - block user

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiBlockUser(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;

#if defined(DIRTYCODE_XENON)
    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s block <user index [0,3]> <base-10 XUID string>\n", argv[0]);
        return;
    }
#elif defined(DIRTYCODE_PS3)
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s block <online id string>\n", argv[0]);
        return;
    }
#endif

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    if (_IsCmdInProgress(pApp))
    {
        ZPrintf("   another friendapi command already in progress... please try again later\n");
        return;
    }

#if defined(DIRTYCODE_XENON)
    sscanf(argv[2], "%d", &pApp->iUserIndex);
    sscanf(argv[3], "%I64d", &pApp->xuid);
#elif defined(DIRTYCODE_PS3)
    pApp->iUserIndex = 0;
    memset(&pApp->onlineId, 0, sizeof(pApp->onlineId));
    ds_strnzcpy(pApp->onlineId.data, argv[2], sizeof(pApp->onlineId.data) + 1);

    if (_StartNpIdLookup(pApp) < 0)
    {
        return;
    }
#endif

    ZPrintf("   initiated blocking of user %s ... command in progress\n", argv[2]);

    pApp->bBlockInProgress = TRUE;

    // install callback
    ZCallback(_CmdFriendApiCb, 1000);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiUnblockUser

    \Description
        FriendApi subcommand - unblock user

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiUnblockUser(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;

#if defined(DIRTYCODE_XENON)
    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s unblock <user index [0,3]> <base-10 XUID string>\n", argv[0]);
        return;
    }
#else
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s unblock <online id string>\n", argv[0]);
        return;
    }
#endif

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    if (_IsCmdInProgress(pApp))
    {
        ZPrintf("   another friendapi command already in progress... please try again later\n");
        return;
    }

#if defined(DIRTYCODE_XENON)
    sscanf(argv[2], "%d", &pApp->iUserIndex);
    sscanf(argv[3], "%I64d", &pApp->xuid);
#elif defined(DIRTYCODE_PS3)
    pApp->iUserIndex = 0;
    memset(&pApp->onlineId, 0, sizeof(pApp->onlineId));
    ds_strnzcpy(pApp->onlineId.data, argv[2], sizeof(pApp->onlineId.data) + 1);

    if (_StartNpIdLookup(pApp) < 0)
    {
        return;
    }
#endif

    ZPrintf("   initiated unblocking of user %s ... command in progress\n", argv[2]);

    pApp->bUnblockInProgress = TRUE;

    // install callback
    ZCallback(_CmdFriendApiCb, 1000);
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiStart

    \Description
        FriendApi subcommand - register notification callback with FriendApi and start the module

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiStart(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s start\n", argv[0]);
        return;
    }

    if (pApp->bStarted)
    {
        ZPrintf("   FriendApi already started\n");
        return;
    }

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // register notification callback
    FriendApiAddCallback(pFriendApiRef, _FriendApiNotificationCb, (void *)pApp);

#if defined(DIRTYCODE_PS3)
    if (_InitNpLookupUtility(pApp) < 0)
    {
        return;
    }
#endif

    pApp->bStarted = TRUE;

    return;
}

/*F*************************************************************************************/
/*!
    \Function _FriendApiStop

    \Description
        FriendApi subcommand - unregister notification callback with FriendApi and stop the module

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _FriendApiStop(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    FriendApiAppT *pApp = (FriendApiAppT *)_pApp;
    FriendApiRefT *pFriendApiRef;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s stop\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("   FriendApi not yet started\n");
        return;
    }

    if (_IsCmdInProgress(pApp))
    {
        ZPrintf("   another friendapi command already in progress... please try again later\n");
        return;
    }

#if defined(DIRTYCODE_PS3)
    _ShutdownNpLookupUtility(pApp);
#endif

    // retrieve FriendApi ref from NetConn
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));

    // unregister notification callback
    FriendApiRemoveCallback(pFriendApiRef, _FriendApiNotificationCb, (void *)pApp);

    pApp->bStarted = FALSE;
}

static void _FriendApiNotificationCb(FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData, void *pNotifyCbUserData)
{
    FriendApiAppT *pApp = (FriendApiAppT *)pNotifyCbUserData;

    ZPrintf("FriendApi notification!\n");

#if defined(DIRTYCODE_PS3)
    if(pEventData->pRawData1)
    {
        ZPrintf("   raw PS3 event id = %d\n", *(uint32_t *)pEventData->pRawData1);
    }
#endif

    switch (pEventData->eventType)
    {
        case FRIENDAPI_EVENT_UNKNOWN:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_UNKNOWN\n");
            break;

        case FRIENDAPI_EVENT_FRIEND_UPDATED:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_FRIEND_UPDATED\n");
            ZPrintf("      friend id = %s\n", pEventData->eventTypeData.FriendUpdated.pFriendData->strFriendId);
            ZPrintf("      friend name = %s\n", pEventData->eventTypeData.FriendUpdated.pFriendData->strFriendName);
            ZPrintf("      friend presence = %s\n", (pEventData->eventTypeData.FriendUpdated.bPresence?"ONLINE":"OFFLINE"));
            break;

        case FRIENDAPI_EVENT_FRIEND_REMOVED:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_FRIEND_REMOVED\n");
            ZPrintf("      friend id = %s\n", pEventData->eventTypeData.FriendRemoved.pFriendData->strFriendId);
            ZPrintf("      friend name = %s\n", pEventData->eventTypeData.FriendRemoved.pFriendData->strFriendName);
            break;

        case FRIENDAPI_EVENT_BLOCKLIST_UPDATED:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_BLOCKLIST_UPDATED\n");
            break;

        case FRIENDAPI_EVENT_BLOCK_USER_RESULT:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_BLOCK_USER_RESULT\n");
            ZPrintf("      block user operation result = %s\n", (pEventData->eventTypeData.BlockUserResult.bSuccess?"SUCCESS":"FAILURE"));
#if defined(DIRTYCODE_PS3)
            ZPrintf("      ps3 error code = %s\n", DirtyErrGetName(*(int32_t *)pEventData->pRawData2));
#endif
            pApp->bBlockInProgress = FALSE;
            break;

        case FRIENDAPI_EVENT_MSG_RCVED:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_MSG_RCVED\n");
            ZPrintf("      friend id = %s\n", pEventData->eventTypeData.MsgRcved.pFriendData->strFriendId);
            ZPrintf("      friend name = %s\n", pEventData->eventTypeData.MsgRcved.pFriendData->strFriendName);
            ZPrintf("      msg= %s\n", pEventData->eventTypeData.MsgRcved.pRawData2);
            break;

        case FRIENDAPI_EVENT_INIT_COMPLETE:
            ZPrintf("   friendapi event type = FRIENDAPI_EVENT_INIT_COMPLETE\n");
            ZPrintf("      user index = %d\n", pEventData->uUserIndex);
            break;

        default:
            ZPrintf("   unsupported event\n");
            break;
    }
}


/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdFriendApi

    \Description
        FriendApi command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 03/07/2011 (mclouatre)
*/
/**************************************************************************************F*/
int32_t CmdFriendApi(ZContext *argz, int32_t argc, char **argv)
{
    T2SubCmdT *pCmd;
    FriendApiAppT *pApp = &_FriendApi_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_FriendApi_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   1st-party friends service abstraction\n");
        T2SubCmdUsage(argv[0], _FriendApi_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
