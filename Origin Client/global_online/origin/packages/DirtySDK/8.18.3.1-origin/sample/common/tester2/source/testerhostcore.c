/*H********************************************************************************/
/*!
    \File testerhostcore.c

    \Description
        Main control module for the Tester2 host application.

    \Notes
        This is the main host module for the tester2 host application.
        It contains mostly global-variable type objects and operations, similar
        to LobbyAPI. TesterHostCore is responsible for starting up all the
        necessary child modules like TesterConsole, TesterComm, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtydefs.h"
#include "dirtysock.h"
#include "dirtylib.h"
#include "lobbytagfield.h"
#include "netconn.h"

#include "zmem.h"
#include "testercomm.h"
#include "testerconsole.h"
#include "testermodules.h"
#include "testerregistry.h"
#include "testerhistory.h"
#include "testerhostcore.h"

/*** Defines **********************************************************************/

#define TESTERHOSTCORE_CONSOLESIZE_DEFAULT  (16384)     //!< default size of display console

/*** Type Definitions *************************************************************/

struct TesterHostCoreT
{
    TesterCommT             *pComm;         //!< host/client communication module
    TesterConsoleT          *pConsole;      //!< console for managing output
    TesterConsoleDisplayCbT *pDisplayProc;  //!< procedure for displaying output
    TesterModulesT          *pModules;      //!< tester modules/dispatcher
    TesterHistoryT          *pHistory;      //!< host history (only supported on some platforms)
    int32_t                 iCurHistory;    //!< current history index
    int16_t                 iShutdown;      //!< internal - set to 1 to shut stuff down
    int16_t                 iLocalEcho;     //!< internal - set to 1 to locally echo output
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Private Variables ************************************************************/

//! used to format strings for sending
static char _TesterHostCore_strTempText[4096] = "";

/*** External Functions ***********************************************************/

/*F********************************************************************************/
/*!
    \Function TesterHostCoreUpdate

    \Description
        Update function to pump the host/client buffers
        and do other idle processing tasks.

    \Input *pData - user specified data (modules state TesterHostCoreT *)
    \Input uTick  - NetIdle ticks

    \Output None

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHostCoreUpdate(void *pData, uint32_t uTick)
{
    TesterHostCoreT *pState = pData;

    // check for errors
    if (pState == NULL)
        return;

    // update the host client comm pipe
    if (pState->pComm)
    {
        TesterCommUpdate(pState->pComm);
    }

    // service the output
    if ((pState->pConsole) && (pState->pDisplayProc))
        TesterConsoleFlush(pState->pConsole, pState->pDisplayProc);

    // see if we want to shut all running commands down
    if(pState->iShutdown)
    {
        // shut down all running commands
        pState->iShutdown = 0;
        ZShutdown();
        // now disconnect dirtysock
        NetConnDisconnect();
    }
}




/*F********************************************************************************/
/*!
    \Function _TesterHostCoreMsgControl

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - User-supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterHostCoreMsgControl(TesterCommT *pState, const char *pMsg, void *pParam)
{
    char strBuf[TESTERCOMM_COMMANDSIZE_DEFAULT];
    const char strNetStart[] = "NETSTART";
    const char strNetStop[] = "NETSTOP";
    TesterHostCoreT *pCore = pParam;
    char *pNetParams;

    // print locally
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONTROL, pMsg);
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONTROL, "\n");

    // respond to the client
    ZMemSet(strBuf, 0, sizeof(strBuf));

    // ---- CONNECT ---- :
    if (strcmp(pMsg, "CONNECT") == 0)
    {
        // print locally
        TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONTROL, "CONNECTED\n");
        // send it along
        TesterCommMessage(pState, TESTER_MSGTYPE_CONTROL, "CONNECTED\n");
    }
    // ---- NETSTART ---- : bring up the network
    else if (strncmp(pMsg, strNetStart, strlen(strNetStart)) == 0)
    {
        // extract the net params
        pNetParams = (char *)(&pMsg[strlen(strNetStart)+1]);

        // connect the module
        TesterHostCoreConnect(pCore, pNetParams);

        // resolve local address
        ZPrintf("Local address: %a\n", NetConnStatus('addr', 0, NULL, 0));

        // get MAC address
        ZPrintf("Local MAC address: %s\n", NetConnMAC());

    }
    // ---- NETSTOP ---- : shut down the network
    else if (strncmp(pMsg, strNetStop, strlen(strNetStop)) == 0)
    {
        // disconnect stuff
        TesterHostCoreDisconnect(pCore);
    }
    else
    {
        ZPrintf("Unknown CONTROL call: {%s}\n", pMsg);
    }
}


/*F********************************************************************************/
/*!
    \Function _TesterHostCoreMsgStatus

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - User-supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterHostCoreMsgStatus(TesterCommT *pState, const char *pMsg, void *pParam)
{
    TesterHostCoreT *pCore = pParam;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_STATUS, pMsg);
}


/*F********************************************************************************/
/*!
    \Function _TesterHostCoreMsgCommand

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - User-supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterHostCoreMsgCommand(TesterCommT *pState, const char *pMsg, void *pParam)
{
    char strBuf[TESTERCOMM_COMMANDSIZE_DEFAULT];
    char strOutput[TESTERCOMM_COMMANDSIZE_DEFAULT];
    TesterHostCoreT *pCore = (TesterHostCoreT *)pParam;
    int32_t iResult;
    char cDivider;

    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, "\n> ");
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, pMsg);
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_COMMAND, "\n");

    // set the tagfield divider up
    cDivider = TagFieldDivider('\t');

    // signal that we've received the message
    TagFieldClear(strBuf, sizeof(strBuf));
    TagFieldSetString(strBuf, sizeof(strBuf), "COMMAND", pMsg);
    TagFieldSetToken(strBuf, sizeof(strBuf), "STATUS", 'rcvd');
    TesterCommMessage(pState, TESTER_MSGTYPE_STATUS, strBuf);

    // signal that we're executing the message
    TagFieldClear(strBuf, sizeof(strBuf));
    TagFieldSetString(strBuf, sizeof(strBuf), "COMMAND", pMsg);
    TagFieldSetToken(strBuf, sizeof(strBuf), "STATUS", 'exec');
    TesterCommMessage(pState, TESTER_MSGTYPE_STATUS, strBuf);

    // dispatch the command
    iResult = TesterModulesDispatch(pCore->pModules, pMsg);

    // display the results locally
    ZMemSet(strOutput, 0, sizeof(strOutput));
    if (iResult == 0)
    {
        sprintf(strOutput, "done {%s} error {none}\n", pMsg);
    }
    else
    {
        sprintf(strOutput, " {%s} error {%d}\n", pMsg, iResult);
    }
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONSOLE, strOutput);
    ZPrintf(strOutput); //send it back too?

    // send results back to the client
    TagFieldClear(strBuf, sizeof(strBuf));
    TagFieldSetString(strBuf, sizeof(strBuf), "COMMAND", pMsg);
    TagFieldSetToken(strBuf, sizeof(strBuf), "STATUS", 'done');
    // include an error if present
    if (iResult)
    {
        TagFieldSetNumber(strBuf, sizeof(strBuf), "ERROR", iResult);
    }
    TesterCommMessage(pState, TESTER_MSGTYPE_STATUS, strBuf);

    // don't forget to set the divider back
    TagFieldDivider(cDivider);
}


/*F********************************************************************************/
/*!
    \Function _TesterHostCoreMsgConsole

    \Description
        Handle incoming messages from TesterComm

    \Input *pState - Module state
    \Input *pMsg   - Message text
    \Input *pParam - User-supplied data

    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterHostCoreMsgConsole(TesterCommT *pState, const char *pMsg, void *pParam)
{
    TesterHostCoreT *pCore = pParam;
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONSOLE, pMsg);
}


/*F********************************************************************************/
/*!
    \Function _TesterHostCorePrintf

    \Description
        Tester Printf function.

    \Input *pParm  - Module state
    \Input *pText  - Message text

    \Output 0=success

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterHostCorePrintf(void *pParm, const char *pText)
{
    TesterHostCoreT *pCore = pParm;

    // this is where all NetPrintf calls wind up
    // and remember that ZPrintf calls NetPrintf
    // so, on the host, we have to do two things:
    // 1. print locally
    // 2. send back to the client

    // add locally
    TesterConsoleOutput(pCore->pConsole, TESTER_MSGTYPE_CONSOLE, pText);

    // send to the client, but only if we've gotten input from the client
    if (TesterCommStatus(pCore->pComm, 'inpt', 0))
    {
        // append to buffer
        strnzcat(_TesterHostCore_strTempText, pText, sizeof(_TesterHostCore_strTempText));

        // do we have a linefeed?
        if (strrchr(_TesterHostCore_strTempText, '\n') != NULL)
        {
            TesterCommMessage(pCore->pComm, TESTER_MSGTYPE_CONSOLE, _TesterHostCore_strTempText);
            _TesterHostCore_strTempText[0] = '\0';
        }
    }

    // returning a 1 here echoes locally
    return(pCore->iLocalEcho);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterHostCoreCreate

    \Description
        Create a TesterHostCoreT module and return the pointer to it

    \Input *pParams - startup parameters

    \Output TesterHostCoreT * - newly allocated and created TesterHostCoreT

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
TesterHostCoreT *TesterHostCoreCreate(const char *pParams)
{
    TesterHostCoreT *pState;
    int32_t iPlatform;

    // create and wipe clean the module
    pState = (TesterHostCoreT *)ZMemAlloc(sizeof(TesterHostCoreT));
    ZMemSet((void *)pState, 0, sizeof(TesterHostCoreT));

    // create a registry
    TesterRegistryCreate(-1);
    TesterRegistrySetPointer("CORE", pState);

    // now create the children modules
    pState->pComm       = TesterCommCreate();
    pState->pConsole    = TesterConsoleCreate(TESTERHOSTCORE_CONSOLESIZE_DEFAULT, TRUE);
    pState->pModules    = TesterModulesCreate();

    #if DIRTYCODE_LOGGING
    // in debug mode, hook into debug output
    NetPrintfHook(_TesterHostCorePrintf, pState);
    #endif

    // hook into tester output
    ZPrintfHook(_TesterHostCorePrintf, pState);

    // determine if we should locally echo stuff
    pState->iLocalEcho = (int16_t)TagFieldGetNumber(TagFieldFind(pParams, "LOCALECHO"), 1);

    // register host commands
    TesterModulesRegisterHostCommands(pState->pModules);

    // register platform-specific commands
    TesterModulesRegisterPlatformCommands(pState->pModules);

    // attach an interface method
    iPlatform = TagFieldGetNumber(TagFieldFind(pParams, "PLATFORM"), 0);

    switch(iPlatform)
    {
        #if !defined(DIRTYCODE_XENON)
            case TESTER_PLATFORM_NORMAL:
            {
                char strControlDir[2]; // check to see if a control dir was specified
                TagFieldGetString(TagFieldFind(pParams, "CONTROLDIR"), strControlDir, sizeof(strControlDir), "");
                if (strControlDir[0] != '\0')
                {
                    TesterCommAttachFile(pState->pComm);
                }
                else
                {
                    TesterCommAttachSocket(pState->pComm);
                }
                break;
            }
        #else // XENON
            case TESTER_PLATFORM_XBDM:
                TesterCommAttachXenon(pState->pComm);
                break;
        #endif
    }

    // pass the incoming startup parameters through to the comm modules
    TesterCommConnect(pState->pComm, pParams, TRUE);

    // register callbacks for all the types of messages we'll see
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_CONTROL, _TesterHostCoreMsgControl, (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_COMMAND, _TesterHostCoreMsgCommand, (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_STATUS,  _TesterHostCoreMsgStatus,  (void *)pState);
    TesterCommRegister(pState->pComm, TESTER_MSGTYPE_CONSOLE, _TesterHostCoreMsgConsole, (void *)pState);

    // create history
    pState->pHistory = TesterHistoryCreate(-1);

    // done
    return(pState);
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreConnect

    \Description
        Connect the core module and all its children

    \Input *pState              - TesterHostCoreT module to connect
    \Input *pNetParams          - Parameters to pass to NetConnStartup()

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHostCoreConnect(TesterHostCoreT *pState, const char *pNetParams)
{
    // check the state
    if (pState == NULL)
        return(TESTERHOSTCORE_ERROR_NULLPOINTER);

    // start dirtysock
    ZPrintf("HOSTCORE: Starting Dirtysock with NetParams {%s}\n",
        (pNetParams==NULL) ? "" : pNetParams );
    NetConnStartup(pNetParams);

    // done
    return(TESTERHOSTCORE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreIdle

    \Description
        Idle function - pump this to make networking and other modules happy.

    \Input *pState - TesterHostCoreT module state

    \Output None

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHostCoreIdle(TesterHostCoreT *pState)
{
    // give time to the network
    NetConnIdle();
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreDisplayFunc

    \Description
        Register a display function to show stuff on the screen.

    \Input *pState      - TesterHostCoreT module state
    \Input *pProc       - function pointer to display procedure
    \Input iRefcon      - int32_t value to pass to the display function when called
    \Input *pDisplayRef - ref value to pass to the display function when called (hDlg)

    \Output None

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHostCoreDisplayFunc(TesterHostCoreT *pState,  TesterConsoleDisplayCbT *pProc, int32_t iRefcon, void *pRefptr)
{
    pState->pDisplayProc = pProc;
    TesterConsoleConnect(pState->pConsole, iRefcon, pRefptr);
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreDispatch

    \Description
        Dispatch a function (if possible) based on the incoming command line.

    \Input *pState       - pointer to host core module
    \Input *pCommandLine - standard command line (command arg1 arg2 arg3 ...)

    \Output 0=success, error code otherwise

    \Notes
        This function is intended for use by the Tester2 Host application to
        dispatch commands locally.

    \Version 10/31/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t TesterHostCoreDispatch(TesterHostCoreT *pState, const char *pCommandLine)
{
    const char *pSemi;

    // check for multiple commands separated by a semi-colon
    while ((pSemi = strchr(pCommandLine, ';')) != NULL)
    {
        char strTmpCmd[256];
        strsubzcpy(strTmpCmd, sizeof(strTmpCmd), pCommandLine, pSemi - pCommandLine);

        // dispatch the command
        TesterHistoryAdd(pState->pHistory, strTmpCmd);
        pState->iCurHistory = -1;
        TesterModulesDispatch(pState->pModules, strTmpCmd);

        // skip past command
        pCommandLine = pSemi + 1;
    }

    // dispatch the command
    TesterHistoryAdd(pState->pHistory, pCommandLine);
    pState->iCurHistory = -1;
    return(TesterModulesDispatch(pState->pModules, pCommandLine));
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreGetHistory

    \Description
        Get prev/next history entry.

    \Input *pState      - pointer to host core module
    \Input iPrevNext    - amount to index prev (negative) or next (positive) in buffer
    \Input *pBuf        - [out] storage for returned history entry
    \Input iSize        - size of output buffer

    \Output
        None.

    \Version 10/31/2005 (jbrookes)
*/
/********************************************************************************F*/
void TesterHostCoreGetHistory(TesterHostCoreT *pState, int32_t iPrevNext, char *pBuf, int32_t iSize)
{
    int iHeadCount, iTailCount;

    // get current min/max
    TesterHistoryHeadTailCount(pState->pHistory, &iHeadCount, &iTailCount);

    // if we have a current history index, modify from there
    if (pState->iCurHistory != -1)
    {
        // update with clamping
        pState->iCurHistory += iPrevNext;
        if (pState->iCurHistory > iTailCount)
        {
            // iTailCount+1 to return an empty entry at the tail (current commandline)
            pState->iCurHistory = iTailCount+1;
        }
        if (pState->iCurHistory < iHeadCount)
        {
            pState->iCurHistory = iHeadCount;
        }
    }
    else
    {
        pState->iCurHistory = (iPrevNext < 0) ? iTailCount : iHeadCount;
    }

    // get history entry
    if (pState->iCurHistory <= iTailCount)
    {
        TesterHistoryGet(pState->pHistory, pState->iCurHistory, pBuf, iSize);
    }
    else
    {
        *pBuf = '\0';
    }
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreSendStatus

    \Description
        Send a command to the host - should comes from the client GUI command line.

    \Input *pState - TesterHostCoreT module
    \Input *pData  - command to send

    \Output 0=success, error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHostCoreSendStatus(TesterHostCoreT *pState, const char *pData)
{
    // echo it locally
    _TesterHostCoreMsgStatus(pState->pComm, pData, (void *)pState);

    // send it along
    return(TesterCommMessage(pState->pComm, TESTER_MSGTYPE_STATUS, pData));
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreRegister

    \Description
        Register a command

    \Input *pState       - TesterClientCoreT module state
    \Input *pCommand     - command name to register with
    \Input *pFunctionPtr - the ZCommand module to call

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHostCoreRegister(TesterHostCoreT *pState, const char *pCommand, ZCommand *pFunctionPtr)
{
    if (pState == NULL)
        return(TESTERHOSTCORE_ERROR_NULLPOINTER);

    return(TesterModulesRegister(pState->pModules, pCommand, pFunctionPtr));
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreDisconnect

    \Description
        Disconnect the core module and all its children

    \Input *pState - TesterHostCoreT module to disconnect

    \Output int32_t - 0=success, error code otherwise

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHostCoreDisconnect(TesterHostCoreT *pState)
{
    // check the state
    if (pState == NULL)
        return(TESTERHOSTCORE_ERROR_NULLPOINTER);
    // check the children
    if (pState->pComm == NULL)
        return(TESTERHOSTCORE_ERROR_NULLPOINTER);

    // kill running commands first
    ZShutdown();

    return(TESTERHOSTCORE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreShutdown

    \Description
        Signal for a shutdown of all running tester modules

    \Input *pState - TesterHostCoreT module to shutdown

    \Output None

    \Version 04/13/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHostCoreShutdown(TesterHostCoreT *pState)
{
    if(pState)
    {
        pState->iShutdown = 1;
    }
}


/*F********************************************************************************/
/*!
    \Function TesterHostCoreDestroy

    \Description
        Destroy a TesterHostCoreT module and all its children

    \Input *pState - TesterHostCoreT module to destroy

    \Output None

    \Version 03/21/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHostCoreDestroy(TesterHostCoreT *pState)
{
    // check the state
    if (pState == NULL)
        return;

    // destroy history
    TesterHistoryDestroy(pState->pHistory);

    // disconnect the children modules
    TesterCommDisconnect(pState->pComm);

    // destoy all the children modules
    TesterCommDestroy(pState->pComm);
    TesterConsoleDestroy(pState->pConsole);
    TesterModulesDestroy(pState->pModules);

    // unhook debug output
    #if DIRTYCODE_LOGGING
    NetPrintfHook(NULL, NULL);
    #endif

    // unhook tester output
    ZPrintfHook(NULL, NULL);

    // dump the registry
    TesterRegistryDestroy();

    // now destroy this module
    ZMemFree((void *)pState);
}




