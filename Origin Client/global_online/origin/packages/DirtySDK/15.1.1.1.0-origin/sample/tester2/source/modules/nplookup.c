/*H*************************************************************************************/
/*!
    \File    nplookup.c

    \Description
        Reference application for 'nplookup' functions.

    \Copyright
        Copyright (c) Electronic Arts 2007.    ALL RIGHTS RESERVED.

    \Version    1.0        08/01/2007 (danielcarter) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/buddy/hlbudapi.h"

#include "zlib.h"

#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

#if 0
typedef struct HLBuddyAppT
{
    HLBApiRefT      *pBuddyApi;
    int32_t         bZCallback;
} HLBuddyAppT;

static HLBuddyAppT _HLBuddy_App = { NULL, FALSE };
#endif

typedef struct NpLookupRefT
{
        int32_t transCtxId;
        SceNpId npId;
        SceNpUserInfo userInfo;
        SceNpAboutMe aboutMe;
        SceNpMyLanguages languages;
        SceNpCountryCode countryCode;
        SceNpAvatarImage avatarImage;
} NpLookupRefT;

typedef struct NpLookupAppT
{
        NpLookupRefT *pNpLookupApi;
        int32_t       bZCallback;
} NpLookupAppT;

static NpLookupAppT _NpLookup_App = {NULL, FALSE};

        int32_t transCtxId;
        SceNpId npId;
        SceNpUserInfo userInfo;
        SceNpAboutMe aboutMe;
        SceNpMyLanguages languages;
        SceNpCountryCode countryCode;
        SceNpAvatarImage avatarImage;

/*** Function Prototypes ***************************************************************/

#if 0
static void _HLBuddyAnswer(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyChat(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyCreate(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyDestroy(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyInvite(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyList(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyRead(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddyResume(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddySet(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _HLBuddySuspend(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);

void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);
#endif

static void _NpLookupCreate(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _NpLookupQuery(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _NpLookupResponse(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/

#if 0
static T2SubCmdT _HLBuddy_Commands[] =
{
    { "answer",         _HLBuddyAnswer  },
    { "chat",           _HLBuddyChat    },
    { "create",         _HLBuddyCreate  },
    { "destroy",        _HLBuddyDestroy },
    { "invite",         _HLBuddyInvite  },
    { "list",           _HLBuddyList    },
    { "read",           _HLBuddyRead    },
    { "resume",         _HLBuddyResume  },
    { "set",            _HLBuddySet     },
    { "suspend",        _HLBuddySuspend },
    { "",               NULL            }
};
#endif

static T2SubCmdT _NpLookup_Commands[] =
{
    { "create",         _NpLookupCreate  },
//    { "destroy",        _NpLookupDestroy },
    { "query",          _NpLookupQuery   },
    { "response",       _NpLookupResponse},
    { "",               NULL             }
};
/*** Private Functions *****************************************************************/


#if 0
/*F*************************************************************************************/
/*!
    \Function _HLBuddyDebugPrintf
    
    \Description
        HLBuddy debug print callback.
    
    \Input *pArg    - unused
    \Input *pString - pointer to string to print
    
    \Output
        None.
            
    \Version 1.0 08/02/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyDebugPrintf(void *pArg, const char *pString)
{
    ZPrintf("%s", pString);
}
#endif
 
#if 0
static void _HLBuddyAnswer(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   View messages with attachments that have been received to this context\n");
        ZPrintf("   usage: %s answer\n", argv[0]);
        return;
    }

#if defined(DIRTYCODE_PS3)
    HLBReceiveMsgAttachment(pApp->pBuddyApi, &g_MemoryContainer);
#else
    HLBListAnswerGameInvite(pApp->pBuddyApi, "", 0, NULL, NULL);
#endif
}

/*F*************************************************************************************/
        hlbuddy subcommand - send chat message to buddy
/**************************************************************************************F*/
static void _HLBuddyChat(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    HLBBudT* pBuddy = NULL;
    int32_t iBuddyIndex, iArgs;
    char strMessage[512];

    if ((bHelp == TRUE) || (argc < 4))
    {
        ZPrintf("   Send a chat message to one of your buddies\n");
        ZPrintf("   usage: %s chat buddyNum message\n", argv[0]);
        return;
    }

    // extract the buddy index; user-list is 1-based but internal is 0-based so subtract 1.
    iBuddyIndex = TagFieldGetNumber(argv[2], 0) - 1;

    // concatenate the message to send
    memset(strMessage, 0, sizeof(strMessage));
    for (iArgs=3; iArgs<argc; iArgs++)
    {
        sprintf(strMessage, "%s %s", strMessage, argv[iArgs]);
    }

    // get the buddy record
    ZPrintf("CmdHLBuddy: getting buddy\n");
    pBuddy = HLBListGetBuddyByIndex(pApp->pBuddyApi, iBuddyIndex);

    if (pBuddy != NULL)
    {
        // send the message
        ZPrintf("CmdHLBuddy: sending message '%s' to %s\n", strMessage, HLBBudGetName(pBuddy));
        HLBListSendChatMsg(pApp->pBuddyApi, HLBBudGetName(pBuddy), strMessage, NULL, NULL);
    }
    else
    {
        ZPrintf("CmdHLBuddy: Buddy not found\n");
    }
}

/*F*************************************************************************************/
    \Function _HLBuddyMsgAttachmentCallback
/**************************************************************************************F*/
static int32_t _HLBuddyMsgAttachmentCallback(HLBApiRefT *pRef,  HLBBudT  *pBuddy, int32_t eAction, void *pContext)
{
    // Go ahead and "answer" the message attachment
#if defined(DIRTYCODE_PS3)
    HLBReceiveMsgAttachment(pRef, &g_MemoryContainer);
#endif
    return(0);
}

/*F*************************************************************************************/
    \Function _HLBuddyGameInviteCallback
/**************************************************************************************F*/
static int32_t _HLBuddyGameInviteCallback(HLBApiRefT *pRef,  HLBBudT  *pBuddy, int32_t eAction, void *pContext)
{
    // Go ahead and "answer" the message attachment
    HLBListAnswerGameInvite(pRef, NULL, /*eInviteAction*/ HLB_INVITEANSWER_ACCEPT, NULL, NULL);
    return(0);
}
#endif

static void
dump_userProfile(
    const SceNpId *_npId,
    const SceNpUserInfo *_userInfo,
    const SceNpAboutMe *_aboutMe,
    const SceNpMyLanguages *_languages,
    const SceNpCountryCode *_countryCode,
    const SceNpAvatarImage *_avatarImage
    )
{
    NetPrintf(("==== UserProfile ====\n"));
    NetPrintf(("onlineId = <%s>\n", _npId->handle.data));
    NetPrintf(("onlineName = <%s>\n", _userInfo->name.data));
    NetPrintf(("aboutMe = <%s>\n", _aboutMe->data));
    NetPrintf(("language1 = %d(CELL_SYSUTIL_LANG_XXX)\n", _languages->language1));
    if (_languages->language2 >= 0){
        NetPrintf(("language2 = %d(CELL_SYSUTIL_LANG_XXX)\n", _languages->language2));
    }
    if (_languages->language3 >= 0){
        NetPrintf(("language3 = %d(CELL_SYSUTIL_LANG_XXX)\n", _languages->language3));
    }
    NetPrintf(("avatar url = %s\n", _userInfo->icon.data));
    NetPrintf(("avatar size = %d\n", _avatarImage->size));
    NetPrintf(("country or region = %s\n", _countryCode->data));
    NetPrintf(("============================\n"));
    return;
}

/*F*************************************************************************************/
/*!
    \Function _NpLookupCreate
    
    \Description
        nplookup subcommand - create the nplookup api reference
    
    \Input *pCmdRef - pointer to nplookup cmd
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (danielcarter) First Version
*/
/**************************************************************************************F*/
static void _NpLookupCreate(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    NpLookupAppT *pApp = &_NpLookup_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        return;
    }

    /*
    pApp->pBuddyApi = HLBApiCreate2(DirtyMemAlloc, DirtyMemFree, "tester2", "version", 0);

    if (pApp->pBuddyApi == NULL)
    {
        ZPrintf("CmdHLBuddy: create failed\n");
    }
    else
    {
    */
    // Create the Api pointer, though not used
    pApp->pNpLookupApi = (NpLookupRefT*)malloc(sizeof(NpLookupRefT)); 

    ZPrintf("CmdNpLookup: create started\n");
        int32_t iStatus;
        SceNpId selfNpId;
        // SceNpOnlineName onlineName;
        // SceNpAvatarUrl avatarUrl;

        if ((iStatus = sceNpLookupInit()) < 0)
        {
            NetPrintf(("_NpLookupCreate: sceNpLookup Init failed (err=%s)\n", DirtyErrGetName(iStatus)));
            return;
        }

        // NP Manager Direct Access
        sceNpManagerGetNpId(&selfNpId);
        ZPrintf("sceNpManagerGetNpId = %s\n", &selfNpId.handle);

        // NP Lookup Access
        int32_t titleCtxId;
        // SceNpTitleId titleId;
        SceNpOnlineId onlineId;
        SceNpCommunicationId npCommunicationId;

#if 0
        // get registered np communications id
        /*
        NetConnStatus('npti', 0, &titleId, sizeof(titleId));
        titleId.term = '\0';
        titleId.num = 0;
        */
        ds_strnzcpy(npCommunicationId.data, "NPWR00014", SCE_NET_NP_ONLINENAME_MAX_LENGTH + 1);
        npCommunicationId.term = '\0';
        npCommunicationId.num = 0;
        NetPrintf(("_HLBuddyCreate: npCommunicationId = (%s).num = %d\n", &npCommunicationId.data, npCommunicationId.num));
#endif

        // Already obtained NpId above
        // Create Title Context 
        if ((titleCtxId = sceNpLookupCreateTitleCtx(&npCommunicationId, &selfNpId)) < 0)
        {
            NetPrintf(("_NpLookupCreate: sceNpLookupCreateTitleCtx failed (err=%s)\n", DirtyErrGetName(titleCtxId)));
            sceNpLookupTerm();
            return;
        }
        NetPrintf(("_NpLookupCreate: npCommunicationId = (%s).num = %d\n", &npCommunicationId.data, npCommunicationId.num));
        NetPrintf(("_NpLookupCreate: titleCtxId = 0x%x\n", titleCtxId));

        // Create Transaction Context 
        if ((transCtxId = sceNpLookupCreateTransactionCtx(titleCtxId)) < 0)
        {
            NetPrintf(("_NpLookupCreate: sceNpLookupCreateTransCtx failed (err=%s)\n", DirtyErrGetName(transCtxId)));
            sceNpLookupTerm();
            return;
        }
        NetPrintf(("_NpLookupCreate: transCtxId = 0x%x\n", transCtxId));

        // fake the info for obtaining the npId for another user
        //strncpy(onlineId.data, "cladam2_93" /*(char *)&onlineName */, 48 /*SCE_NET_NP_ONLINENAME_MAX_LENGTH*/ + 1);
        strcpy(onlineId.data, "cladam2_93");
        if ((iStatus = sceNpLookupNpIdAsync(transCtxId, &onlineId, &npId, 0, NULL)) < 0)
        {
            NetPrintf(("_NpLookupCreate: sceNpLookupCreateTransCtx failed (err=%s)\n", DirtyErrGetName(transCtxId)));
            sceNpLookupTerm();
            return;
        }
        NetPrintf(("_NpLookupCreate: npId = %s\n", npId.handle.data));
 
        ZPrintf("CmdNpLookup: create succeeded\n");
        
#if 0
        // register nplookup pointer
        TesterRegistrySetPointer("NPLOOKUP", pApp->pNpLookupApi);
#endif
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddyDestroy
    
    \Description
        hlbuddy subcommand - destroy the hlbuddy module
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
/*
static void _NpLookupDestroy(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    // HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    HLBApiDestroy(pApp->pBuddyApi);
    pApp->pBuddyApi = NULL;
}
*/

static void _NpLookupQuery(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
        int32_t iStatus;
        SceNpId selfNpId;
        // SceNpOnlineName onlineName;
        // SceNpAvatarUrl avatarUrl;

        // NP Lookup Access
        int32_t titleCtxId;
        // SceNpTitleId titleId;
        // SceNpOnlineId onlineId;
        SceNpCommunicationId npCommunicationId;

    ZPrintf("CmdNpLookup: query started\n");
    if (sceNpLookupPollAsync(transCtxId, &iStatus) == 0)
    {
        sceNpLookupDestroyTransactionCtx(transCtxId);

        // NP Manager Direct Access
        sceNpManagerGetNpId(&selfNpId);
        ZPrintf("sceNpManagerGetNpId = %s\n", &selfNpId.handle);

        // Create Title Context 
        if ((titleCtxId = sceNpLookupCreateTitleCtx(&npCommunicationId, &selfNpId)) < 0)
        {
            NetPrintf(("_NpLookupQuery: sceNpLookupCreateTitleCtx failed (err=%s)\n", DirtyErrGetName(titleCtxId)));
            sceNpLookupTerm();
            return;
        }
        NetPrintf(("_NpLookupQuery: titleCtxId = 0x%x\n", titleCtxId));

        // Create Transaction Context 
        if ((transCtxId = sceNpLookupCreateTransactionCtx(titleCtxId)) < 0)
        {
            NetPrintf(("_NpLookupQuery: sceNpLookupCreateTransCtx failed (err=%s)\n", DirtyErrGetName(transCtxId)));
            sceNpLookupTerm();
            return;
        }
        NetPrintf(("_NpLookupQuery: transCtxId = 0x%x\n", transCtxId));

        NetPrintf(("_NpLookupQuery: call sceNpLookupUserProfile\n"));
        // Get the Avatar Url in the userInfo
        if ((iStatus = sceNpLookupUserProfileAsync(transCtxId, 
                                                   &npId, 
                                                   &userInfo, 
                                                   &aboutMe, 
                                                   &languages, 
                                                   &countryCode, 
                                                   &avatarImage, 0, NULL)) < 0)
        {
            NetPrintf(("_NpLookupQuery: sceNpLookupUserProfileAsync failed (err=%s)\n", DirtyErrGetName(iStatus)));
        }
        NetPrintf(("_NpLookupQuery: sceNpLookupUserProfileAsync success (err=%s)\n", DirtyErrGetName(iStatus)));

        ZPrintf("CmdNpLookup: query succeeded\n");
    }
    else
    {
        NetPrintf(("_NpLookupQuery: sceNpLookupPollAsync failed (err=%s)\n", DirtyErrGetName(iStatus)));
        sceNpLookupTerm();
    }
}

static void _NpLookupResponse(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
        int32_t iStatus;
        // SceNpOnlineName onlineName;
        // SceNpAvatarUrl avatarUrl;

    ZPrintf("CmdNpLookup: response started\n");
    if (sceNpLookupPollAsync(transCtxId, &iStatus) == 0)
    {
        dump_userProfile(&npId,
                         &userInfo,
                         &aboutMe,
                         &languages,
                         &countryCode,
                         &avatarImage);

        sceNpLookupDestroyTransactionCtx(transCtxId);
        sceNpLookupTerm();
    }
}

#if 0
/*F*************************************************************************************/
/*!
    \Function _HLBuddyInvite
    
    \Description
        hlbuddy subcommand - send a game invite message to buddy
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/25/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyInvite(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    HLBBudT* pBuddy = NULL;
    int32_t iBuddyIndex;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   Send a game invite message to one of your buddies\n");
        ZPrintf("   usage: %s invite buddyNum\n", argv[0]);
        return;
    }

    // extract the buddy index; user-list is 1-based but internal is 0-based so subtract 1.
    iBuddyIndex = TagFieldGetNumber(argv[2], 0) - 1;

    // get the buddy record
    ZPrintf("CmdHLBuddy: getting buddy\n");
    pBuddy = HLBListGetBuddyByIndex(pApp->pBuddyApi, iBuddyIndex);

    if (pBuddy != NULL)
    {
        // send the message
        ZPrintf("CmdHLBuddy: sending game invite message to '%s'\n", HLBBudGetName(pBuddy));
#if defined(DIRTYCODE_PS3)
        HLBListGameInviteBuddy(pApp->pBuddyApi, HLBBudGetName(pBuddy), "game invite state?", NULL, &g_MemoryContainer);
#endif
    }
    else
    {
        ZPrintf("CmdHLBuddy: Buddy not found\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddyList
    
    \Description
        hlbuddy subcommand - list buddies
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyList(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    HLBBudT* pBuddy = NULL;
    int32_t iIndex = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   List your buddies.\n");
        ZPrintf("   usage: %s list\n", argv[0]);
        return;
    }

    ZPrintf("Sorting Buddies...\n");
    HLBListSort(pApp->pBuddyApi);

    ZPrintf("Listing Buddies:\n");
    for(pBuddy = HLBListGetBuddyByIndex(pApp->pBuddyApi, iIndex); 
        pBuddy != NULL;
        iIndex++, pBuddy = HLBListGetBuddyByIndex(pApp->pBuddyApi, iIndex))
    {
        int32_t iNumUnreadMsgs = HLBMsgListGetUnreadCount(pBuddy);
        char strPresence[64];

        ZPrintf("%d) %s - ", iIndex+1, HLBBudGetName(pBuddy));

        if (HLBBudGetState(pBuddy) == HLB_STAT_CHAT)
        {
            ZPrintf("Online");
            if (iNumUnreadMsgs > 0)
            {
                ZPrintf("*** %d Unread Messages ***", iNumUnreadMsgs);
            }
            ZPrintf("\n");
            HLBBudGetPresence(pBuddy, strPresence);
            ZPrintf("    presence: %s\n", strPresence);
        }
        else
        {
            if (HLBBudIsBlocked(pBuddy))
            {
                ZPrintf("Blocked\n");
            }
            else
            {
                ZPrintf("Offline");
                if (iNumUnreadMsgs > 0)
                {
                    ZPrintf("*** %d Unread Messages ***", iNumUnreadMsgs);
                }
                ZPrintf("\n");
            }
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddyRead
    
    \Description
        hlbuddy subcommand - read next unread message from a buddy
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/02/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyRead(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    HLBBudT* pBuddy = NULL;
    int32_t iBuddyIndex;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   Read first unread chat message from one of your buddies\n");
        ZPrintf("   usage: %s read buddyNum\n", argv[0]);
        return;
    }

    // extract the buddy index; user-list is 1-based but internal is 0-based so subtract 1.
    iBuddyIndex = TagFieldGetNumber(argv[2], 0) - 1;

    // get the buddy record
    pBuddy = HLBListGetBuddyByIndex(pApp->pBuddyApi, iBuddyIndex);

    if (pBuddy != NULL)
    {
        // get the first unread message
        BuddyApiMsgT* pMessage = HLBMsgListGetFirstUnreadMsg(pBuddy, 0, TRUE);

        if (pMessage != NULL)
        {
            char strMessage[512];
            // display the message text
            HLBMsgListGetMsgText(pApp->pBuddyApi, pMessage, strMessage, sizeof(strMessage));
            ZPrintf("message text: %s\n", strMessage);

            // delete all messages that have already been read
            HLBMsgListDeleteAll(pBuddy, TRUE);
        }
    }
    else
    {
        ZPrintf("CmdHLBuddy: read - buddy not found\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddyResume
    
    \Description
        hlbuddy subcommand - resume from suspend.
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/21/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyResume(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   Resume the API from the suspended state\n");
        ZPrintf("   usage: %s resume\n", argv[0]);
        return;
    }

    HLBApiResume(pApp->pBuddyApi);
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddySet
    
    \Description
        hlbuddy subcommand - set your presence string
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddySet(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    char strPresence[64];
    int32_t iArgs, iFlags;

    if ((bHelp == TRUE) || (argc < 4))
    {
        ZPrintf("   Set your presence data\n");
        ZPrintf("   usage: %s set flags [presence string]\n", argv[0]);
        return;
    }

    iFlags = TagFieldGetNumber(argv[2], 0);

    memset(strPresence, 0, sizeof(strPresence));
    for (iArgs=3; iArgs<argc; iArgs++)
    {
        sprintf(strPresence, "%s %s", strPresence, argv[iArgs]);
    }

    HLBApiPresenceSame(pApp->pBuddyApi, iFlags, strPresence);
    HLBApiPresenceSendSetPresence(pApp->pBuddyApi);
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddySuspend
    
    \Description
        hlbuddy subcommand - suspend the HLBuddy api.
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/21/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddySuspend(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   Suspend the Buddy API\n");
        ZPrintf("   usage: %s suspend\n", argv[0]);
        return;
    }

    HLBApiSuspend(pApp->pBuddyApi);
}

/*F*************************************************************************************/
/*!
    \Function _CmdHLBuddyCb
    
    \Description
        HLBuddy idle callback.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 04/27/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdHLBuddyCb(ZContext *argz, int32_t argc, char *argv[])
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (pApp->pBuddyApi == NULL)
    {
        // we've been shutdown - do nothing
        pApp->bZCallback = FALSE;
        return(0);
    }

    // shutdown?
    if (argc == 0)
    {
        ZPrintf("CmdHLBuddy: callback is destroying the module\n");
        _HLBuddyDestroy(NULL, 0, NULL, FALSE);
        pApp->bZCallback = FALSE;
        return(0);
    }

    if (pApp->pBuddyApi == NULL)
    {
        ZPrintf("CmdHLBuddy: _CmdHLBuddyCb called with NULL pBuddyApi ref\n");
        return(0);
    }

    // give the HLBuddy module some time
    HLBApiUpdate(pApp->pBuddyApi);

    // update at ~60hz
    return(ZCallback(_CmdHLBuddyCb, 16));
}
#endif

static int32_t _CmdNpLookupCb(ZContext *argz, int32_t argc, char *argv[])
{
    NpLookupAppT *pApp = &_NpLookup_App;

    if (pApp->pNpLookupApi == NULL)
    {
        // we've been shutdown - do nothing
        pApp->bZCallback = FALSE;
        return(0);
    }

    if (pApp->pNpLookupApi == NULL)
    {
        ZPrintf("CmdNpLookup: _CmdNpLookupCb called with NULL pNpLookupApi ref\n");
        return(0);
    }

    // update at ~60hz
    return(ZCallback(_CmdNpLookupCb, 16));
}
/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdNpLookup
    
    \Description
        NpLookup command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 08/01/2006 (danielcarter) First Version
*/
/**************************************************************************************F*/
int32_t CmdNpLookup(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    NpLookupAppT *pApp = &_NpLookup_App;
    unsigned char bHelp;

        ZPrintf("CMD NP Lookup entry\n");
    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_NpLookup_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: execute NP Lookup commands\n", argv[0]);
        T2SubCmdUsage(argv[0], _NpLookup_Commands);
        return(0);
    }
        ZPrintf("CMD NP Lookup handle basic help\n");

    // if no ref yet, make one
    if ((pCmd->pFunc != _NpLookupCreate) && (pApp->pNpLookupApi == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _NpLookupCreate(pApp, 1, &pCreate, bHelp);

        // if _NpLookupCreate did not succeed
        if (pApp->pNpLookupApi == NULL)
        {
            ZPrintf("   %s: _NpLookupCreate did not succeed\n", argv[0]);
            return(0);
        }
    }
        ZPrintf("CMD NP Lookup create ref \n");

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

        ZPrintf("CMD NP Lookup handoff to command \n");
    return(0);

    /*
    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdNpLookupCb, 16));
    }
    else
    {
        return(0);
    }
    */
}
