/*H********************************************************************************/
/*!
    \File testercomm_msdmhost.c

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

#include <xtl.h>
#include <xbdm.h>
#include <stdio.h>
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

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TesterCommXenonT
{
    uint32_t uPad;                      //!< no host-specific xenon stuff to save
} TesterCommXenonT;

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


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
    char strLineOut[TESTERCOMM_COMMANDSIZE_DEFAULT];
    const char strPrefix[] = "CMD!";
    int32_t iLineOutSize, iLineLength;
    char *pLineOut;
    char cDivider;
    TesterCommDataT LineData;
    uint32_t iResult;

    // check error conditions
    if (pState == NULL)
        return(-1);

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
        iResult = DmSendNotificationString(strLineOut);
        if (iResult != XBDM_NOERR)
        {
            ZPrintf("testercomm: failed to send notification to client with result [0x%X]\n", iResult);
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
    \Function _TesterCommCmdProcessor

    \Description
        Handle incoming commands from the MS Debug Manager

    \Input strCommand   - incoming command, including prefix
    \Input pResponse    - (unused) location to push a response (if non-null)
    \Input uResponseLen - (unused) length of string for response (if non-zero)
    \Input pdmcc        - (unused) pointer to the command handler function

    \Output int32_t - 0=success, error code otherwise

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t __stdcall _TesterCommCmdProcessor( const CHAR* strCommand,
                                              CHAR* pResponse, uint32_t uResponseLen,
                                              PDM_CMDCONT pdmcc )
{
    TesterCommT *pState;
    TesterCommDataT LineData;
    char *pCommand;

    // get the comm module if possible
    if ((pState = (TesterCommT *)TesterRegistryGetPointer("COMM")) == NULL)
    {
        ZPrintf("testercomm: could not get comm module to push commands to.\n");
        return(-1);
    }

    // step past the prefix
    pCommand = strchr(strCommand, '!');
    if ((pCommand == NULL) || (*pCommand == 0) || (*(pCommand+1) == 0))
    {
        ZPrintf("testercomm: bad message from client [%s]\n", strCommand);
        return(-1);
    }
    pCommand++;

    // create an entry and add it to the list
    ZMemSet(&LineData, 0, sizeof(LineData));
    LineData.iType = TagFieldGetNumber(TagFieldFind(pCommand, "TYPE"), 0);
    TagFieldGetString(TagFieldFind(pCommand, "TEXT"), LineData.strBuffer, sizeof(LineData.strBuffer)-1, "");
    ZListPushBack(pState->pInputData, &LineData);
    ZPrintf("testercomm: command[%s]\n", LineData.strBuffer);
    pState->bGotInput = TRUE;

    return(0);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState  - pointer to host client comm module
    \Input *pParams - startup parameters
    \Input bIsHost  - TRUE=host, FALSE=client

    \Output int32_t - 0 for success, error code otherwise

    \Version 04/22/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    const char strCommandHandle[] = "CMD";
    // check for error conditions
    if ((pState == NULL) || (pParams == NULL))
    {
        ZPrintf("testercomm: connect got invalid NULL pointer - pState [0x%X] pParams [0x%X]\n", pState, pParams);
        return(-1);
    }

    // set the tick time so we don't automatically get a timeout
    pState->uLastSendTime = ZTick();

    // register the command processor
    DmRegisterCommandProcessor(strCommandHandle, (PDM_CMDPROC )_TesterCommCmdProcessor);

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

    volatile int32_t iHack=0;

    if (pState == NULL)
        return(-1);

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

    if(iHack)
    {
        DmSendNotificationString("CMD!Hello world.\n");
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

    // else return no error
    return(0);
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

}
