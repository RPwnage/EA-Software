/*H*************************************************************************************/
/*!
    \File    digobj.c

    \Description
        Reference application for the DigObjApi module.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2006.    ALL RIGHTS RESERVED.

    \Version    1.0        06/01/2006 (tchen) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "lobbyhasher.h"
#include "lobbydisplist.h"
#include "digobjapi.h"

#include "zlib.h"
#include "zfile.h"
#include "zmemcard.h"

#include "testersubcmd.h"
#include "testerregistry.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DigObjAppT
{
    DigObjApiRefT *pDigObjApi;
    
    char strPersona[16];
    char strLKey[128];
} DigObjAppT;

/*** Function Prototypes ***************************************************************/

static void _DigObjCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _DigObjDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _DigObjRdm(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _DigObjFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/


// Private variables

static const char *_strDigObjServerError[] = 
{
    "DIGOBJAPI_ERROR_NONE",           //!< no error
    "DIGOBJAPI_ERROR_SERVER",         //!< an unknown server error occurred
    "DIGOBJAPI_ERROR_LKEYINVALID",    //!< invalid lkey
    "DIGOBJAPI_ERROR_CODEINVALID",    //!< invalid regcode
    "DIGOBJAPI_ERROR_CODEUSED",      //!< already used regcode
    "DIGOBJAPI_ERROR_PERSINVALID",    //!< invalid user
    "DIGOBJAPI_ERROR_CMDINVALID",    //!< invalid command

};

static const char *_strDigObjClientError[] =
{
    "DIGOBJAPI_ERROR_CLIENT",
    "DIGOBJAPI_ERROR_TIMEOUT"
};

static T2SubCmdT _DigObj_Commands[] =
{
    { "create",     _DigObjCreate      },
    { "destroy",    _DigObjDestroy     },
    { "redeem",     _DigObjRdm         },
    { "fetch",      _DigObjFetch       },
    { "",           NULL               }
};

static DigObjAppT _DigObj_App = { NULL };

// Public variables


/*** Private Functions *****************************************************************/


/*

DigObj file support functions

*/


/*F*************************************************************************************/
/*!
    \Function _DigObjPrintDigObj
    
    \Description
        Display files in current digobj.
    
    \Input *pApp    - pointer to digobj app
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen)) First Version
*/
/**************************************************************************************F*/
static void _DigObjPrintDigObj(DigObjAppT *pApp)
{
    int32_t i = 0;
    const DigObjApiIdT *pId;

    ZPrintf("digobj: current list of object IDs\n\n");
    for(i=0; i < DigObjApiGetObjectCount(pApp->pDigObjApi); i++)
    {
        pId = DigObjApiGetObjectAtIndex(pApp->pDigObjApi,i);
        ZPrintf("  domain=%s ID=%s\n",pId->strDomain,pId->strID);
    }
}



/*F*************************************************************************************/
/*!
    \Function _DigObjCallback
    
    \Description
        DigObjApi user callback.
    
    \Input *pDigObjApi  - pointer to digobjapi
    \Input eEvent       - callback event type
    \Input *pFile       - pointer to current file, or NULL
    \Input *pUserData   - user callback data
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen) First Version
*/
/**************************************************************************************F*/
static void _DigObjCallback(DigObjApiRefT *pDigObjApi, DigObjApiEventE eEvent, void *pUserData)
{
    DigObjAppT *pApp = (DigObjAppT *)pUserData;

    switch(eEvent)
    {
        case DIGOBJAPI_EVENT_RDM:
        {
            _DigObjPrintDigObj(pApp);
        }
        break;

        case DIGOBJAPI_EVENT_FET:
        {
            _DigObjPrintDigObj(pApp);
        }
        break;

        case DIGOBJAPI_EVENT_ERROR:
        {
            DigObjApiErrorE eError;

            // print error
            eError = DigObjApiGetLastError(pApp->pDigObjApi);
            if (eError < DIGOBJAPI_ERROR_CLIENT)
            {
                ZPrintf("digobj: got error %s\n", _strDigObjServerError[eError]);
            }
            else
            {
                ZPrintf("digobj: got error %s\n", _strDigObjClientError[eError-DIGOBJAPI_ERROR_CLIENT]);
            }
        }
        break;

        default:
            ZPrintf("digobj: unhandled event %d\n", eEvent);
    }
}


/* 
    Command functions
*/


/*F*************************************************************************************/
/*!
    \Function _DigObjCreate
    
    \Description
        DigObj subcommand - create digobj
    
    \Input *pApp    - pointer to digobj app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen) First Version
*/
/**************************************************************************************F*/
static void _DigObjCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    DigObjAppT *pApp = (DigObjAppT *)_pApp;
    char *pDigObjServer = "";

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create <servername>\n", argv[0]);
        return;
    }

    // make sure we don't already have a ref
    if (pApp->pDigObjApi != NULL)
    {
        ZPrintf("   %s: ref already created\n", argv[0]);
        return;
    }
    
    // get stuff we need from lobby
    strnzcpy(pApp->strLKey, "CHANGEME", sizeof(pApp->strLKey));
    strnzcpy(pApp->strPersona, "CHANGEME", sizeof(pApp->strPersona));

    // override default server?
    if (argc == 3)
    {
        ZPrintf("   %s: using digobj server '%s'\n", argv[0], argv[2]);
        pDigObjServer = argv[2];
    }

    // create api
    pApp->pDigObjApi = DigObjApiCreate(pDigObjServer, _DigObjCallback, pApp);

    // set parms
    DigObjApiSetLoginInfo(pApp->pDigObjApi, pApp->strLKey, pApp->strPersona);
}

/*F*************************************************************************************/
/*!
    \Function _DigObjDestroy
    
    \Description
        DigObj subcommand - destroy digobj module
    
    \Input *pApp    - pointer to digobj app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen) First Version
*/
/**************************************************************************************F*/
static void _DigObjDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    DigObjAppT *pApp = (DigObjAppT *)_pApp;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    // destroy digobjapi
    if (pApp->pDigObjApi)
    {
        DigObjApiDestroy(pApp->pDigObjApi);
        pApp->pDigObjApi = NULL;
    }
}

/*F*************************************************************************************/
/*!
    \Function _DigObjRdm
    
    \Description
        DigObj subcommand - display current digobj
    
    \Input *pApp    - pointer to digobj app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen) First Version
*/
/**************************************************************************************F*/
static void _DigObjRdm(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    DigObjAppT *pApp = (DigObjAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s redeem code\n", argv[0]);
        return;
    }

    ZPrintf("%s: redeeming reg code '%s'\n", argv[0], argv[2]);
    DigObjApiRedeemCode(pApp->pDigObjApi, argv[2]);
}

/*F*************************************************************************************/
/*!
    \Function _DigObjFetch
    
    \Description
        DigObj subcommand - get current directory info
    
    \Input *pApp    - pointer to digobj app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 06/01/2006 (tchen) First Version
*/
/**************************************************************************************F*/
static void _DigObjFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    DigObjAppT *pApp = (DigObjAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s fetch pers\n", argv[0]);
        return;
    }

    ZPrintf("%s: fetching object list for user '%s'\n", argv[0], argv[2]);
    DigObjApiFetchObjectListForUser(pApp->pDigObjApi, argv[2]);
}

/*F*************************************************************************************/
/*!
    \Function _CmdDownloadCb
    
    \Description
        Locker idle callback.  This callback function exists only to clean up after
        locker when tester exits.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 12/14/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdDigObjCb(ZContext *argz, int32_t argc, char **argv)
{
    if (argc == 0)
    {
        // shut down locker
        _DigObjDestroy(&_DigObj_App, 0, NULL, FALSE);
        return(0);
    }

    return(ZCallback(_CmdDigObjCb, 1000));
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdDigObj
    
    \Description
        DigObj command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 11/24/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
int32_t CmdDigObj(ZContext *argz, int32_t argc, char **argv)
{
    T2SubCmdT *pCmd;
    DigObjAppT *pApp = &_DigObj_App;
    unsigned char bHelp, bCreate = FALSE;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_DigObj_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   get, update, and otherwise manage an ea digobj\n");
        T2SubCmdUsage(argv[0], _DigObj_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pApp->pDigObjApi == NULL) && strcmp(pCmd->strName, "create"))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _DigObjCreate(pApp, 1, &pCreate, bHelp);
        bCreate = TRUE;
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // if we executed create, remember
    if (pCmd->pFunc == _DigObjCreate)
    {
        bCreate = TRUE;
    }

    // if we executed create, install periodic callback
    return((bCreate == TRUE) ? ZCallback(_CmdDigObjCb, 1000) : 0);
}
