/*H*************************************************************************************/
/*!
    \File    hlbuddy.c

    \Description
        Reference application for 'hlbuddy' functions.

    \Copyright
        Copyright (c) Electronic Arts 2006.    ALL RIGHTS RESERVED.

    \Version    1.0        08/01/2006 (tdills) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "hlbudapi.h"
#include "friendapi.h"

#include "zlib.h"

#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct HLBuddyAppT
{
    HLBApiRefT      *pBuddyApi;
    int32_t         bZCallback;
} HLBuddyAppT;

static HLBuddyAppT _HLBuddy_App = { NULL, FALSE };

/*** Function Prototypes ***************************************************************/

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

/*** Variables *************************************************************************/

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

/*** Private Functions *****************************************************************/


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

/*F*************************************************************************************/
/*!
    \Function _HLBuddyChat
    
    \Description
        hlbuddy subcommand - displays the attachment receive dialog box
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyAnswer(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   View messages with attachments that have been received to this context\n");
        ZPrintf("   usage: %s answer\n", argv[0]);
        return;
    }

    #if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
    HLBReceiveMsgAttachment(pApp->pBuddyApi, NULL);
    #else
    HLBListAnswerGameInvite(pApp->pBuddyApi, "", 0, NULL, NULL);
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _HLBuddyChat
    
    \Description
        hlbuddy subcommand - send chat message to buddy
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
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
/*!
    \Function _HLBuddyMsgAttachmentCallback
    
    \Description
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/02/2007 (danielcarter) First Version
*/
/**************************************************************************************F*/
#if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
static int32_t _HLBuddyMsgAttachmentCallback(HLBApiRefT *pRef,  HLBBudT  *pBuddy, int32_t eAction, void *pContext)
{
    // Go ahead and "answer" the message attachment
    HLBReceiveMsgAttachment(pRef, NULL);
    return(0);
}
#endif

/*F*************************************************************************************/
/*!
    \Function _HLBuddyGameInviteCallback
    
    \Description
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 04/02/2007 (danielcarter) First Version
*/
/**************************************************************************************F*/
#if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
static int32_t _HLBuddyGameInviteCallback(HLBApiRefT *pRef,  HLBBudT  *pBuddy, int32_t eAction, void *pContext)
{
    // Go ahead and "answer" the message attachment
    HLBListAnswerGameInvite(pRef, NULL, /*eInviteAction*/ HLB_INVITEANSWER_ACCEPT, NULL, NULL);
    return(0);
}
#endif

static void _BuddyListChangedCallback(HLBApiRefT *pRef, int32_t op, int32_t opStatus, void *pContext)
{
    ZPrintf("_BuddyListChangedCallback\n");
    ZPrintf("  Buddy count is now %d\n", HLBListGetBuddyCount(pRef));
}

static void _FriendApiNotificationCallback(FriendApiRefT *pFriendApiRef, FriendApiEventsE eventType, FriendEventDataT *pEventData, FriendDataT *pFriendData, void *pNotifyCbUserData)
{
    ZPrintf("_FriendApiNotificationCallback\n");

#if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
    if (pEventData)
    {
        ZPrintf("  PS3 event is %d\n", *(int32_t*)pEventData->pRawData1);
        ZPrintf("    message: %s\n", pEventData->strMsgBuf);
    }
    if (pFriendData)
    {
        ZPrintf("    friend's NP Online ID: %s\n", ((SceNpUserInfo *)pFriendData->pRawData1)->userId.handle.data);
    }
#endif
}


/*F*************************************************************************************/
/*!
    \Function _HLBuddyCreate
    
    \Description
        hlbuddy subcommand - create the hlbuddy api reference
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _HLBuddyCreate(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;
    FriendApiRefT *pFriendApiRef;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create\n", argv[0]);

    }

    if (pApp->pBuddyApi)
    {
        ZPrintf("CmdHLBuddy: HLBApi already created\n");
        return;
    }

    pApp->pBuddyApi = HLBApiCreate2(DirtyMemAlloc, DirtyMemFree, "tester2", "version", 0, NULL);

    if (pApp->pBuddyApi == NULL)
    {
        ZPrintf("CmdHLBuddy: create failed\n");
    }
    else
    {
        // register the message attachment callback
        #if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
        HLBApiRegisterMsgAttachmentCallback (pApp->pBuddyApi, (HLBBudActionCallbackT *)&_HLBuddyMsgAttachmentCallback, pApp);

        // register the game invite callback
        HLBApiRegisterGameInviteCallback (pApp->pBuddyApi, (HLBBudActionCallbackT *)&_HLBuddyGameInviteCallback, pApp);
        #endif

        HLBApiSetDebugFunction(pApp->pBuddyApi, NULL, _HLBuddyDebugPrintf);
        ZPrintf("CmdHLBuddy: create succeeded\n");

        // register hlbudapi pointer
        TesterRegistrySetPointer("HLBUDDY", pApp->pBuddyApi);
    }

    HLBApiRegisterBuddyChangeCallback(pApp->pBuddyApi, _BuddyListChangedCallback, pApp);

    // register notification callback with friend api module
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));
    FriendApiAddCallback(pFriendApiRef, _FriendApiNotificationCallback, (void *)pApp);
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
static void _HLBuddyDestroy(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    HLBuddyAppT *pApp = &_HLBuddy_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    HLBApiDestroy(pApp->pBuddyApi);
    pApp->pBuddyApi = NULL;
}

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
        #if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
        HLBListGameInviteBuddy(pApp->pBuddyApi, HLBBudGetName(pBuddy), "game invite state?", NULL, NULL);
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

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdHLBuddy
    
    \Description
        HLBuddy command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 08/01/2006 (tdills) First Version
*/
/**************************************************************************************F*/
int32_t CmdHLBuddy(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    HLBuddyAppT *pApp = &_HLBuddy_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_HLBuddy_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: execute high-level buddy commands\n", argv[0]);
        T2SubCmdUsage(argv[0], _HLBuddy_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _HLBuddyCreate) && (pApp->pBuddyApi == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _HLBuddyCreate(pApp, 1, &pCreate, bHelp);

        // if _HLBuddyCreate did not succeed
        if (pApp->pBuddyApi == NULL)
        {
            ZPrintf("   %s: _HLBuddyCreate did not succeed\n", argv[0]);
            return(0);
        }
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdHLBuddyCb, 16));
    }
    else
    {
        return(0);
    }
}
