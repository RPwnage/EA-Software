/*H*************************************************************************************/
/*!

    \File    ticker.cpp

    \Description
        This application demonstrates the winrt Internet access capability using the
        Dirtysock stack.

    \Copyright
        Copyright (c) Electronic Arts 2012.  ALL RIGHTS RESERVED.

    \Version    1.0      10/28/2012 (mclouatre) Initial version

*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Objbase.h>  // for GUID

// dirtysock includes
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp.h"

#include "DirtySDK/misc/weblog.h"


/*** Defines ***************************************************************************/

#define TICKER_WEBLOG_ENABLED (DIRTYCODE_DEBUG && TRUE)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/


/*** Variables *************************************************************************/

// Private variables


// Public variables



/*** Public Functions ******************************************************************/


int main(Platform::Array<Platform::String^>^ args)
{
    int32_t iAddr, iLen, iStatus, iCount;
    uint8_t bProtoHttpInProgress;
    uint32_t uTimeout;
    ProtoHttpRefT *pHttp;
    char strBuffer[1024];
    #if TICKER_WEBLOG_ENABLED
    WebLogRefT *pWebLog;
    int32_t iVerbose = 1;
    #endif

    NetPrintf(("ticker: starting...\n"));

    #if TICKER_WEBLOG_ENABLED
    // create weblog module
    pWebLog = WebLogCreate(8192);

    // set weblog protohttp debugging to a high level
    WebLogControl(pWebLog, 'spam', iVerbose, 0, NULL);
    // hook in to netprintf debug output
    NetPrintfHook(WebLogDebugHook, pWebLog);
    // start logging
    WebLogStart(pWebLog);
    #endif

    // start network
    NetConnStartup("-servicename=ticker");

    // bring up the interface
    NetConnConnect(NULL, NULL, 0);

    // wait for network interface activation
    for (uTimeout = NetTick()+15*1000 ; ; )
    {
        // update network
        NetConnIdle();

        // get current status
        iStatus = NetConnStatus('conn', 0, NULL, 0);
        if ((iStatus == '+onl') || ((iStatus >> 24) == '-'))
        {
            break;
        }

        // check for timeout
        if (uTimeout < (signed)NetTick())
        {
            NetPrintf(("ticker: timeout waiting for interface activation\n"));
            return(-1);
        }

        // give time to other threads
        NetConnSleep(500);
    }

    // check result code
    if ((iStatus = NetConnStatus('conn', 0, NULL, 0)) == '+onl')
    {
        NetPrintf(("ticker: interface active\n"));
    }
    else if ((iStatus >> 14) == '-')
    {
        NetPrintf(("ticker: error bringing up interface\n"));
        return(-11);
    }

    // broadband check (should return TRUE if broadband, else FALSE)
    NetPrintf(("iftype=%d\n", NetConnStatus('type', 0, NULL, 0)));
    NetPrintf(("broadband=%s\n", NetConnStatus('bbnd', 0, NULL, 0) ? "TRUE" : "FALSE"));

    // acquire and display address
    iAddr = NetConnStatus('addr', 0, NULL, 0);
    NetPrintf(("addr=%a\n", iAddr));

    // display unique hardware identifier address
    NetPrintf(("(mac address is not available on winrt)\n"));
    NetPrintf(("dump of unique hw id (GUID):\n"));
    GUID guid;
    if (NetConnStatus('hwid', 0, &guid, sizeof(GUID)) == 0)
    {
        NetPrintMem(&guid, sizeof(GUID), "unique-hwid");
    }

    // setup http module
    pHttp = ProtoHttpCreate(4096);

    // just keep working
    bProtoHttpInProgress = FALSE;
    for ( uTimeout = NetTick()-1, iLen=-1, iCount = 0; iCount < 8; )
    {
        // see if its time to query
        if ((uTimeout != 0) && (NetTick() > uTimeout))
        {
            ProtoHttpGet(pHttp, "http://quote.yahoo.com/d/quotes.csv?s=^DJI,^SPC,^IXIC,ERTS,SNE,AOL,YHOO,^AORD,^N225,^FTSE&f=sl1c1&e=.csv", FALSE);
            uTimeout = NetTick()+20*1000;
            bProtoHttpInProgress = TRUE;
            iCount += 1; // count the attempt
        }

        // update protohttp
        ProtoHttpUpdate(pHttp);

        // read incoming data into buffer
        if (bProtoHttpInProgress == TRUE)
        {
            if ((iLen = ProtoHttpRecvAll(pHttp, strBuffer, sizeof(strBuffer)-1)) > 0)
            {
                // null-terminate response
                strBuffer[iLen] = '\0';

                // print response
                NetPrintf(("ticker: received response\n"));
                NetPrintf(("%s", strBuffer));

                bProtoHttpInProgress = FALSE;
            }
            else if (iLen != PROTOHTTP_RECVWAIT)
            {
                NetPrintf(("ticker: GET failed\n"));
                bProtoHttpInProgress = FALSE;
            }
        }

        NetConnIdle();
    }

    NetPrintf(("ticker: done\n"));

    // shut down HTTP
    ProtoHttpDestroy(pHttp);

    // disconnect from the network
    NetConnDisconnect();

    // shutdown the network connections && destroy the dirtysock code
    NetConnShutdown(FALSE);
    return(0);
}


void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return(malloc((unsigned)iSize));
}

void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}


