/*H*************************************************************************************/
/*!
    \File    ps3sysutils.c

    \Description
        tester2 module for displaying ps3 sysutils dialogs.

    \Copyright
        Copyright (c) Electronic Arts 2006.    ALL RIGHTS RESERVED.

    \Version    1.0        09/15/2006 (tdills) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netex/libnetctl.h>
#include <sysutil/sysutil_common.h>
#include <sysutil/sysutil_webbrowser.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "zlib.h"
#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct PS3SysUtilsAppT
{
    int32_t         bZCallback;
    enum 
    {
        ST_IDLE,

        ST_NETSTART_LOADING,
        ST_NETSTART_RUNNING,
        ST_NETSTART_FINISHED,
        ST_NETSTART_UNLOADING,
        ST_NETSTART_CLEANUP,

        ST_FRIENDLIST_RUNNING,

        ST_WEBBROWSER_INITIALIZING,
        ST_WEBBROWSER_INITIALIZED,
        ST_WEBBROWSER_RUNNING,
        ST_WEBBROWSER_UNLOADED,

    } eState;

    char            strURL[256];

    struct CellNetCtlNetStartDialogParam  NetStartDlgParms;
    struct CellNetCtlNetStartDialogResult NetStartDlgResult;

} PS3SysUtilsAppT;

static PS3SysUtilsAppT _PS3SysUtils_App = { FALSE, 0 };

/*** Function Prototypes ***************************************************************/

static void _PS3SysUtilsNetStartDlg(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _PS3SysUtilsFriendlist(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _PS3SysUtilsWebBrowser(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/

static T2SubCmdT _PS3SysUtils_Commands[] =
{
    { "netstart",       _PS3SysUtilsNetStartDlg  },
    { "friendlist",     _PS3SysUtilsFriendlist   },
    { "web",            _PS3SysUtilsWebBrowser   },
    { "",               NULL                     }
};


/*** Private Functions *****************************************************************/



/*F*************************************************************************************/
/*!
    \Function _PS3SysUtilsNetStartDlg
    
    \Description
        ps3sysutils subcommand - displays PS3 network startup dialog
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/15/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _PS3SysUtilsNetStartDlg(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;
    int32_t iResult;

    if (argc < 3)
    {
        bHelp = TRUE;
    }
    else if (ds_stricmp(argv[2],"np") == 0)
    {
        pApp->NetStartDlgParms.type = CELL_NET_CTL_NETSTART_TYPE_NP;
    }
    else if (ds_stricmp(argv[2],"nw") == 0)
    {
        pApp->NetStartDlgParms.type = CELL_NET_CTL_NETSTART_TYPE_NET;
    }
    else
    {
        bHelp = TRUE;
    }

    if (bHelp == TRUE)
    {
        ZPrintf("   Display the PS3 Network Startup Dialog\n");
        ZPrintf("   usage: %s netstart <np | nw>\n", argv[0]);
        return;
    }

    if (pApp->eState != ST_IDLE)
    {
        ZPrintf("PS3SysUtils needs to be idle to run this command\n");
        return;
    }

    pApp->NetStartDlgParms.size = sizeof(pApp->NetStartDlgParms);
    pApp->NetStartDlgParms.cid = 0;

    if ((iResult = cellNetCtlNetStartDialogLoadAsync(&pApp->NetStartDlgParms)) != CELL_OK)
    {
        ZPrintf("PS3SysUtils: cellNetCtlNetStartDialogLoadAsync() failed (err=%s)\n", DirtyErrGetName(iResult));
    }
    else
    {
        pApp->eState = ST_NETSTART_LOADING;
    }
}

/*F*************************************************************************************/
/*!
    \Function _PS3SysUtilsWebBrowserCb
    
    \Description
        Callback function to receive web browser state changes
    
    \Input iEvent - Event type
    \INput *pUserData - ignored
    
    \Output
        None.
            
    \Version 1.0 09/15/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _PS3SysUtilsWebBrowserCb(int32_t iEvent, void *pUserData)
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;

    ZPrintf("PS3SysUtils: received web browser event %d\n", iEvent);

    switch (iEvent)
    {
    case CELL_SYSUTIL_WEBBROWSER_INITIALIZING_FINISHED:
        {
            pApp->eState = ST_WEBBROWSER_INITIALIZED;
        }
        break;

    case CELL_SYSUTIL_WEBBROWSER_UNLOADING_FINISHED:
        {
            pApp->eState = ST_WEBBROWSER_UNLOADED;
        }
        break;

    case CELL_SYSUTIL_WEBBROWSER_RELEASED:
        {
            pApp->eState = ST_IDLE;
        }
        break;

    default:
        break;
    }

}

/*F*************************************************************************************/
/*!
    \Function _PS3SysUtilsFriendlist
    
    \Description
        ps3sysutils subcommand - displays friendlist utility
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/15/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static int32_t _PS3SysUtilsFriendlistHandler(int32_t iArg, void *pArg)
{
    PS3SysUtilsAppT *pApp = (PS3SysUtilsAppT *)pArg;
    pApp->eState = ST_IDLE;
    ZPrintf("PS3SysUtils: FrienlistHandler=%d\n", iArg);
    return(0);
}

static void _PS3SysUtilsFriendlist(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;
    int32_t iResult;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   Execute the PS3 Friendlist utility\n");
        ZPrintf("   usage: %s friendlist\n", argv[0]);
        return;
    }
    
    if (pApp->eState != ST_IDLE)
    {
        ZPrintf("PS3SysUtils needs to be idle to run this command\n");
        return;
    }

    // start the friendlist utility
    if ((iResult = sceNpFriendlist(_PS3SysUtilsFriendlistHandler, pApp, SYS_MEMORY_CONTAINER_ID_INVALID)) < 0)
    {
        ZPrintf("PS3SysUtils: sceNpFriendlist() failed (err=%s)\n", DirtyErrGetName(iResult));
        return;
    }
    
    pApp->eState = ST_FRIENDLIST_RUNNING;
}

/*F*************************************************************************************/
/*!
    \Function _PS3SysUtilsWebBrowser
    
    \Description
        ps3sysutils subcommand - displays PS3 web browser
    
    \Input *pApp    - pointer to hlbuddy app
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 09/15/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static void _PS3SysUtilsWebBrowser(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;
    static CellWebBrowserConfig config_full;
    sys_memory_container_t MemHandle;
    int32_t mc_size;
    int32_t iResult;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   Execute the PS3 Friendlist utility\n");
        ZPrintf("   usage: %s web <url>\n", argv[0]);
        return;
    }

    if (pApp->eState != ST_IDLE)
    {
        ZPrintf("PS3SysUtils needs to be idle to run this command\n");
        return;
    }

    // hold onto the URL for later
    memset(pApp->strURL, 0, sizeof(pApp->strURL));
    strcpy(pApp->strURL, argv[2]);

    // initialize the  web browser config struct
    memset((void*)&config_full, 0, sizeof(config_full));
	cellWebBrowserConfigSetHeapSize(&config_full, 48*1024*1024);
	config_full.functions = CELL_WEBBROWSER_FUNCTION_ALL;

    // determine the size of the memory container that will be needed
    mc_size = cellWebBrowserEstimate(&config_full);
    ZPrintf("PS3SysUtils: web browswer required memory container size: %d\n", mc_size);

    if ((iResult = sys_memory_container_create(&MemHandle, 128*1024*1024)) != CELL_OK)
    {
        ZPrintf("PS3SysUtils: sys_memory_container_create() failed (err=%s)\n", DirtyErrGetName(iResult));
        return;
    }

    cellWebBrowserConfig(&config_full);

    if ((iResult = cellWebBrowserInitialize(_PS3SysUtilsWebBrowserCb, MemHandle)) != CELL_OK)
    {
        ZPrintf("PS3SysUtils: cellWebBrowserInitialize() failed (err=%s)\n", DirtyErrGetName(iResult));
        return;
    }

    pApp->eState = ST_WEBBROWSER_INITIALIZING;
}

/*F********************************************************************************/
/*!
    \Function   _PS3SysUtilsSysutilCallback
    
    \Description
        Callback function for receiving message box input.
    
    \Input uStatus   - status of the onscreen dialog
    \Input uParam    - not used
    \Input pUserData - not used
    
    \Output void
            
    \Version 09/15/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _PS3SysUtilsSysutilCallback(uint64_t uStatus, uint64_t uParam, void* pUserData)
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;

    switch (uStatus)
    {
    case CELL_SYSUTIL_NET_CTL_NETSTART_LOADED:
        pApp->eState = ST_NETSTART_RUNNING;
        break;
    case CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED:
        pApp->eState = ST_NETSTART_FINISHED;
        break;
    case CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED:
        ZPrintf("PS3SysUtils: Network Start Dialog returned %d\n", pApp->NetStartDlgResult.result);
        pApp->eState = ST_NETSTART_CLEANUP;
        break;
    }
}

/*F*************************************************************************************/
/*!
    \Function _CmdPS3SysUtilsUpdate
    
    \Description
        ps3sysutils idle callback.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 04/27/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdPS3SysUtilsUpdate(ZContext *argz, int32_t argc, char *argv[])
{
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;

    switch (pApp->eState)
    {
    case ST_NETSTART_FINISHED:
        {
            memset((void*)&pApp->NetStartDlgResult, 0, sizeof(pApp->NetStartDlgResult));
            pApp->NetStartDlgResult.size = sizeof(pApp->NetStartDlgResult);

            cellNetCtlNetStartDialogUnloadAsync(&pApp->NetStartDlgResult);
            pApp->eState = ST_NETSTART_UNLOADING;
        }
        break;

    case ST_NETSTART_CLEANUP:
        {
            pApp->eState = ST_IDLE;
        }
        break;

    case ST_WEBBROWSER_INITIALIZED:
        {
            int32_t iResult;

            if ((iResult = cellWebBrowserCreate(pApp->strURL)) != CELL_OK)
            {
                ZPrintf("PS3SysUtils: cellWebBrowserCreate() failed (err=%s)\n", DirtyErrGetName(iResult));
                pApp->eState = ST_IDLE;
            }
            else
            {
                pApp->eState = ST_WEBBROWSER_RUNNING;
            }
        }
        break;

    case ST_WEBBROWSER_UNLOADED:
        {
            cellWebBrowserShutdown();
        }
        break;

    default:
        break;
    }

    // update at ~60hz
    return(ZCallback(_CmdPS3SysUtilsUpdate, 16));
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdPS3SysUtils
    
    \Description
        PS3SysUtils command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 09/15/2006 (tdills) First Version
*/
/**************************************************************************************F*/
int32_t CmdPS3SysUtils(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    PS3SysUtilsAppT *pApp = &_PS3SysUtils_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_PS3SysUtils_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: execute PS3 sysutils commands\n", argv[0]);
        T2SubCmdUsage(argv[0], _PS3SysUtils_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;

    	if (cellSysutilRegisterCallback(0, _PS3SysUtilsSysutilCallback, NULL) != 0)
        {
            ZPrintf("cellSysutilRegisterCallback failed\n");
        }

        return(ZCallback(_CmdPS3SysUtilsUpdate, 16));
    }
    else
    {
        return(0);
    }
}
