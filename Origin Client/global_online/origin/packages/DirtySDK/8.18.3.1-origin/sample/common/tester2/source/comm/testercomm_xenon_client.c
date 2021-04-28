/*H********************************************************************************/
/*!
    \File testercomm_xenon_client.c

    \Description
        This module provides a communication layer between the host and the client.  
        Typical operations are SendLine() and GetLine(), which send and receive 
        lines of text, commands, debug output, etc.  Each platform will implement 
        its own way of communicating – through files, debugger API calls, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/26/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtylib.h"
#include "lobbytagfield.h"
#include "zmem.h"
#include "zlib.h"
#include "zlist.h"
#include "testerprofile.h"
#include "testerregistry.h"
#include "testercomm.h"

#if !defined(TESTER_NO_XBDM)
// We never define TESTER_NO_XBDM for building, it's here only for CodeStripper to work
#include "t2xbdmxenon.h"

/*** Defines **********************************************************************/

#define TESTERCOMM_CONNECT_RETRYTIME      (5000)              //!< retry to connect this often, in milliseconds

/*** Type Definitions *************************************************************/

typedef int32_t XBDMStartupFunc(void);
typedef int32_t XBDMCloseConnectionFunc(T2XBDMXenonConnectionT Connection);
typedef int32_t XBDMCloseNotificationSessionFunc(T2XBDMXenonSessionT Session);
typedef int32_t XBDMOpenConnectionFunc(T2XBDMXenonConnectionT *pConnection);
typedef int32_t XBDMNotifyFunc(T2XBDMXenonSessionT Session, uint32_t uNotification, T2XBDMXenonNotifyFunctionT Handler);
typedef int32_t XBDMOpenNotificationSessionFunc(uint32_t uFlags, T2XBDMXenonSessionT *pSession);
typedef int32_t XBDMRegisterNotificationProcessorFunc(T2XBDMXenonSessionT Session, char *pType, T2XBDMXenonExtNofityFunctionT Handler);
typedef int32_t XBDMSendCommandFunc(T2XBDMXenonConnectionT Connection, char *pCommand, char *pResponse, uint32_t *pResponseSize);
typedef int32_t XBDMSetXboxNameNoRegisterFunc(char *pName);
typedef int32_t XBDMTranslateErrorFunc(int32_t hr, char *pBuffer, int32_t iBufferMax);
typedef int32_t XBDMRebootFunc(uint32_t uFlags);
typedef int32_t XBDMShutdownFunc(void);

typedef struct TesterXBDMLibT
{
    XBDMStartupFunc                         *XBDMStartup;
    XBDMCloseConnectionFunc                 *XBDMCloseConnection;
    XBDMCloseNotificationSessionFunc        *XBDMCloseNotificationSession;
    XBDMOpenConnectionFunc                  *XBDMOpenConnection;
    XBDMNotifyFunc                          *XBDMNotify;
    XBDMOpenNotificationSessionFunc         *XBDMOpenNotificationSession;
    XBDMRegisterNotificationProcessorFunc   *XBDMRegisterNotificationProcessor;
    XBDMSendCommandFunc                     *XBDMSendCommand;
    XBDMSetXboxNameNoRegisterFunc           *XBDMSetXboxNameNoRegister;
    XBDMTranslateErrorFunc                  *XBDMTranslateError;
    XBDMRebootFunc                          *XBDMReboot;
    XBDMShutdownFunc                        *XBDMShutdown;
} TesterXBDMLibT;

typedef struct TesterCommXenonT
{
    // xbdm debugger interface stuff
    TesterXBDMLibT         XBDMLib;
    T2XBDMXenonConnectionT pDMConnection;                               //!< Debug Monitor Connection
    T2XBDMXenonSessionT    pDMSession;                                  //!< Debug Monitor Session
    unsigned char uECPConnected;                                        //!< indicates if the command processor is connected
    unsigned char uLibraryLoaded;                                       //!< indicates if we've loaded the library yet
    unsigned char uPad1[2];                                             //!< pad data
    char strXenonName[32];                                              //!< xenon name
} TesterCommXenonT;

/*** Variables ********************************************************************/

static const char strDllName[] = "t2xbdmxenon.dll";

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterCommXenonGetXBDMProc

    \Description
        Load the xbdm functions for the xbox debugging session.

    \Input hXBDM        - the module reference for the dll
    \Input *ppOutPtr    - the pointer to the proc that we loaded from the DLL
    \Input *strFuncName - the name of the function to load
    
    \Output int32_t - 0 = success, -1 = failure

    \Version 11/09/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _TesterCommXenonGetXBDMProc(HMODULE hXBDM, FARPROC *ppOutPtr, char *strFuncName)
{
    *ppOutPtr = GetProcAddress(hXBDM, strFuncName);
    if (*ppOutPtr == NULL)
    {
        ZPrintf("testercomm: could not get proc address for %s\n", strFuncName);
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommXenonLoadXBDM

    \Description
        Load the xbdm dll for the xenon debugging session.

    \Input *pState  - pointer to host client comm module
    
    \Output int32_t - 0 = success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommXenonLoadXBDM(TesterCommT *pState)
{
    HMODULE hXBDM;

    // load the library
    hXBDM = LoadLibrary(strDllName);
    if (!hXBDM)
    {
        ZPrintf("testercomm: Couldn't load [%s]\n", strDllName);
        return(-1);
    }
    else
    {
        TesterCommXenonT* pModule = pState->pInterface->pData;
        int32_t iResult = 0;

        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMStartup, "T2XBDMXenonStartup");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMCloseConnection, "T2XBDMXenonCloseConnection");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMCloseNotificationSession, "T2XBDMXenonCloseNotificationSession");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMOpenConnection, "T2XBDMXenonOpenConnection");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMNotify, "T2XBDMXenonNotify");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMOpenNotificationSession, "T2XBDMXenonOpenNotificationSession");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMRegisterNotificationProcessor, "T2XBDMXenonRegisterNotificationProcessor");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMSendCommand, "T2XBDMXenonSendCommand");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMSetXboxNameNoRegister, "T2XBDMXenonSetXboxNameNoRegister");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMTranslateError, "T2XBDMXenonTranslateError");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMReboot, "T2XBDMXenonReboot");
        iResult += _TesterCommXenonGetXBDMProc(hXBDM, (FARPROC*)&pModule->XBDMLib.XBDMShutdown, "T2XBDMXenonShutdown");

        if (iResult < 0)
        {
            ZPrintf("testercomm: one or more of the XBDM functions could not be loaded\n");
            return(-1);
        }

        pModule->XBDMLib.XBDMStartup();
        ZPrintf("testercomm: loaded and started [%s]\n", strDllName);
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommCmdProcessor

    \Description
        Process incoming xenon commands.

    \Input *pIncomingCommand  - incoming command to process
    
    \Output int32_t - 0 = success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
static uint32_t __stdcall _TesterCommCmdProcessor(const char* pIncomingCommand)
{
    TesterCommT *pState;
    TesterCommDataT LineData;
    char *pCommand;

    // get the comm module if possible
    if ((pState = (TesterCommT *)TesterRegistryGetPointer("COMM")) == NULL)
    {
        ZPrintf("testercomm: could not get comm module to push commands to.\n");
        return((uint32_t)-1);
    }

    // step past the prefix
    pCommand = strchr(pIncomingCommand, '!');
    if ((pCommand == NULL) || (*pCommand == 0) || (*(pCommand+1) == 0))
    {
        ZPrintf("testercomm: bad message from client [%s]\n", pIncomingCommand);
        return((uint32_t)-1);
    }
    pCommand++;

    // create an entry and add it to the list
    ZMemSet(&LineData, 0, sizeof(LineData));
    LineData.iType = TagFieldGetNumber(TagFieldFind(pCommand, "TYPE"), 0);
    TagFieldGetString(TagFieldFind(pCommand, "TEXT"), LineData.strBuffer, sizeof(LineData.strBuffer)-1, "");
    ZListPushBack(pState->pInputData, &LineData);

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommXenonConnect

    \Description
        Connect a debugging session to a xenon.  Pass in a tagfield with "HOSTNAME"
        to specify a xenon other than the default to connect to.

    \Input *pState  - pointer to host client comm module
    \Input *pParams - special params for the connect to use
    
    \Output int32_t - 0 = success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommXenonConnect(TesterCommT *pState, const char *pParams)
{
    TesterCommXenonT *pData;
    char strResponse[256];
    const char *pXenonName;
    uint32_t uTick;
    HRESULT hRes;

    // check error conditions
    if ((pState == NULL) || (pState->pInterface->pData == NULL))
    {
        return(-1);
    }
    pData = pState->pInterface->pData;

    // already connected?
    if (pData->uECPConnected == 1)
    {
        return(0);
    }

    // throttle our attempts
    uTick = ZTick();
    if (uTick - pState->uLastConnectTime < TESTERCOMM_CONNECT_RETRYTIME)
    {
        return(0);
    }
    pState->uLastConnectTime = uTick;

    // Set the machine name to connect to, if specified
    pXenonName = TagFieldFind(pParams, "HOSTNAME");
    if (pXenonName)
    {
        ZMemSet(pData->strXenonName, 0, sizeof(pData->strXenonName));
        TagFieldGetString(pXenonName, pData->strXenonName, sizeof(pData->strXenonName), "");
    }

    // Set the machine to connect to if one was provided
    if (pData->strXenonName[0] != 0)
    {
        hRes = pData->XBDMLib.XBDMSetXboxNameNoRegister(pData->strXenonName);
        if (FAILED(hRes))
        {
            pData->XBDMLib.XBDMTranslateError(hRes, strResponse, sizeof(strResponse)-1);
            ZPrintf("testercomm: failed to set xenon name [%s] with result [0x%X, %s]\n", 
                pXenonName, hRes, strResponse);
            return(-1);
        }
    }

    // Open our connection
    hRes = pData->XBDMLib.XBDMOpenConnection( &(pData->pDMConnection) );
    if (FAILED(hRes))
    {
        pData->XBDMLib.XBDMTranslateError(hRes, strResponse, sizeof(strResponse)-1);
        ZPrintf("testercomm: failed to open connection to Xenon with result [0x%X, %s]\n", hRes, strResponse);
        return(-1);
    }

    // Make sure we'll be able to receive notifications
    hRes  = pData->XBDMLib.XBDMOpenNotificationSession( 0, &(pData->pDMSession) );
    if (FAILED(hRes))
    {
        pData->XBDMLib.XBDMTranslateError(hRes, strResponse, sizeof(strResponse)-1);
        ZPrintf("testercomm: failed to open notification session to xenon with result [0x%X, %s]\n", hRes, strResponse);
        pData->XBDMLib.XBDMCloseConnection(pData->pDMConnection);
        return(-1);
    }

    hRes = pData->XBDMLib.XBDMRegisterNotificationProcessor(pData->pDMSession, "CMD", _TesterCommCmdProcessor);
    if (FAILED(hRes))
    {
        ZPrintf("testercomm: failed to register a notification processor with result [0x%X, %s]\n", hRes, strResponse);
        return(-1);
    }

    ZPrintf("testercomm: connection established to [%s]\n", pData->strXenonName);
    pData->uECPConnected = 1;
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommXenonDisconnect

    \Description
        Disconnect from a xenon debugging session

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0 = success, error code otherwise

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommXenonDisconnect(TesterCommT *pState)
{
    TesterCommXenonT *pData;
    if ((pState) && (pState->pInterface->pData))
    {
        pData = pState->pInterface->pData;
        if(pData->uECPConnected)
        {
            ZPrintf("testercomm: disconnecting from xenon\n");
            pData->XBDMLib.XBDMNotify(pData->pDMSession, 0, NULL);
            pData->XBDMLib.XBDMCloseNotificationSession(pData->pDMSession);
            pData->XBDMLib.XBDMCloseConnection(pData->pDMConnection);
            pData->uECPConnected = 0;
        }
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommCheckInput

    \Description
        Check for data coming from the other side (host or client) and pull
        any data into our internal buffer, if possible.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0 = no data, >0 = data, error code otherwise

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckInput(TesterCommT *pState)
{
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommCheckOutput

    \Description
        Check and send data from the output buffer, if possible.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckOutput(TesterCommT *pState)
{
    TesterCommXenonT *pData;
    char strLineOut[TESTERCOMM_COMMANDSIZE_DEFAULT];
    char strResponse[TESTERCOMM_COMMANDSIZE_DEFAULT];
    uint32_t uResponseSize = sizeof(strResponse);
    const char strPrefix[] = "CMD!";
    int32_t iLineOutSize, iLineLength;
    char *pLineOut;
    char cDivider;
    TesterCommDataT LineData;
    int32_t iResult;

    // check error conditions
    if ((pState == NULL) || (pState->pInterface->pData == NULL))
    {
        return(-1);
    }
    pData = pState->pInterface->pData;

    //// if we're not connected, try to connect
    //if (pState->uECPConnected == 0)
    //{
    //    // if we fail to connect, do nothing
    //    _TesterCommXenonConnect(pState, NULL);
    //    if (pState->uECPConnected == 0)
    //    {
    //        return(-1);
    //    }
    //}

    // see if there's any data
    if(ZListPeekFront(pState->pOutputData) == NULL)
        return(0);

    // push output queue stuff by sending commands
    cDivider = TagFieldDivider('\t');
    while(ZListPeekFront(pState->pOutputData))
    {
        // snag the data into the local buffer
        ZListPopFront(pState->pOutputData, &LineData);

        // create the stuff to write
        pLineOut = strLineOut;
        iLineOutSize = sizeof(strLineOut);
        ZMemSet(strLineOut, 0, sizeof(strLineOut));
        pLineOut = strcat(pLineOut, strPrefix);
        iLineOutSize -= strlen(strLineOut);
        iLineOutSize -= 1; // for NULL
        TagFieldSetNumber(strLineOut, iLineOutSize, "TYPE", LineData.iType);
        TagFieldSetString(strLineOut, iLineOutSize, "TEXT", LineData.strBuffer);
        iLineLength = strlen(strLineOut);
        strLineOut[iLineLength] = '\0';

        // send it as a command
        iResult = pData->XBDMLib.XBDMSendCommand(pData->pDMConnection, strLineOut, strResponse, &uResponseSize);
        if (FAILED(iResult))
        {
            char strError[100];
            ZMemSet(strError, 0, sizeof(strError));

            // close the connection
            //_TesterCommXenonDisconnect(pState);

            // try to translate the error, but if we fail, just quit
            if (FAILED(pData->XBDMLib.XBDMTranslateError(iResult, strError, sizeof(strError))))
            {
                ZPrintf("testercomm: failed to send command to xenon with result [0x%X]\n", iResult);
                return(-1);
            }

            // print what we got
            if (iResult == 0x82DA0000)
            {
                strnzcpy(strError, strResponse, sizeof(strError));
            }
            ZPrintf("testercomm: failed to send command to xenon with result [0x%X, %s]\n", iResult, strError);
            return(-1);
        }
    }

    // close out the send operations
    TagFieldDivider(cDivider);

    // mark the last send time
    pState->uLastSendTime = ZTick();
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState              - pointer to host client comm module
    \Input *pParams             - startup parameters
    \Input bIsHost              - TRUE=host, FALSE=client
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 04/22/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    // check for error conditions
    if ((pState == NULL) || (pParams == NULL))
    {
        ZPrintf("testercomm: connect got invalid NULL pointer - pState [0x%X] pParams [0x%X]\n", pState, pParams);
        return(-1);
    }

    // connect to the xenon
    _TesterCommXenonConnect(pState, pParams);

    // set the tick time so we don't automatically get a timeout
    pState->uLastSendTime = ZTick();

    // done for now
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommUpdate

    \Description
        Give the host/client interface module some processor time.  Call this
        once in a while to pump the input and output pipes.

    \Input *pState - module state

    \Output    int32_t - 0 for success, error code otherwise

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommUpdate(TesterCommT *pState)
{
    TesterCommDataT Entry;
    int32_t iResult;

    if (pState == NULL)
    {
        return(-1);
    }

    // quit if we are suspended (don't do any more commands)
    if (pState->uSuspended)
        return(0);

    // check for outgoing and incoming data
    _TesterCommCheckOutput(pState);
    _TesterCommCheckInput(pState);

    // now call the callbacks for incoming messages
    iResult = ZListPopFront(pState->pInputData, &Entry);
    while(iResult > 0)
    {
        // try to access the message map
        if (pState->MessageMap[Entry.iType] != NULL)
        {
            // protect against recursion by suspending commands until this one completes
            TesterCommSuspend(pState);
            (pState->MessageMap[Entry.iType])(pState, Entry.strBuffer, pState->pMessageMapUserData[Entry.iType]);
            TesterCommWake(pState);
        }

        // try to get the next chunk
        iResult = ZListPopFront(pState->pInputData, &Entry);
    }
    
    // done
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect

    \Description
        Disconnect the host client communication module.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/23/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect(TesterCommT *pState)
{
    // check for error conditions
    if (pState == NULL)
        return(-1);

    // disconnect from the xenon
    _TesterCommXenonDisconnect(pState);

    // else return no error
    return(0);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TesterCommCanAttachXenon

    \Description
        Is the Xenon XBDM module present in the bin directory?

    \Input None
    
    \Output int32_t - BOOL

    \Version 11/09/2006 (tdills)
*/
/********************************************************************************F*/
int32_t TesterCommCanAttachXenon(void)
{
    HANDLE hXBDM = LoadLibrary(strDllName);
    if (!hXBDM)
    {
        ZPrintf("testercomm: Couldn't load [%s]\n", strDllName);
        return(FALSE);
    }
    FreeLibrary(hXBDM);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function TesterCommAttachXenon

    \Description
        Attach file module function pointers to a tester comm module.

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterCommAttachXenon(TesterCommT *pState)
{
    if(pState == NULL)
        return;

    ZPrintf("testercomm: attaching XENON MSDM interface methods\n");

    // setup the interface and interface data
    pState->pInterface->CommConnectFunc    = &_TesterCommConnect;
    pState->pInterface->CommUpdateFunc     = &_TesterCommUpdate;
    pState->pInterface->CommDisconnectFunc = &_TesterCommDisconnect;
    pState->pInterface->pData = (TesterCommXenonT *)ZMemAlloc(sizeof(TesterCommXenonT));
    ZMemSet(pState->pInterface->pData, 0, sizeof(TesterCommXenonT));
    
    // load the DLL
    _TesterCommXenonLoadXBDM(pState);
}
#else
int32_t TesterCommCanAttachXenon(void)
{
    return FALSE;
}
void TesterCommAttachXenon(TesterCommT *pState)
{
}
#endif // TESTER_NO_XBDM
