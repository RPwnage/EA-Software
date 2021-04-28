/*H********************************************************************************/
/*!
    \File porttester.c

    \Description
        Tests the PortTester module.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/10/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "dirtynet.h"
#include "protoname.h"
#include "porttestapi.h"
#include "lobbytagfield.h"

#include "zlib.h"

#include "testerregistry.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static PortTesterT *pPortTester;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _PortTesterGetIPAddressFromString

    \Description
        Get a numeric IP address from a string.

    \Input *pIPAddress - dotted string ip address to translate
    
    \Output int32_t        - numeric ip address
    
    \Version 03/10/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _PortTesterGetIPAddressFromString(const char *pIPAddress)
{
    return(ProtoNameSync(pIPAddress, 10*10000));
}

/*F********************************************************************************/
/*!
    \Function _PortTesterStatusCallback

    \Description
        Callback from PortTester used to signify progress

    \Input Standard argz, argc, argv
    
    \Output Standard return codes
    
    \Version 03/10/2005 (jfrank)
*/
/********************************************************************************F*/
static void _PortTesterStatusCallback(PortTesterT *pRef, int32_t iStatus, void *pUserData)
{
    int32_t iReceived, iTotal, iSequence;
    char strStatusBuf[16];

    switch(iStatus)
    {
        case PORTTESTER_STATUS_IDLE:    
            strnzcpy(strStatusBuf, "IDLE", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_SENDING:    
            strnzcpy(strStatusBuf, "SENDING", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_SENT:    
            strnzcpy(strStatusBuf, "SENT", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_RECEIVING:    
            strnzcpy(strStatusBuf, "RECEIVING", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_SUCCESS:    
            strnzcpy(strStatusBuf, "SUCCESS", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_PARTIALSUCCESS:    
            strnzcpy(strStatusBuf, "PARTIALSUCCESS", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_TIMEOUT:    
            strnzcpy(strStatusBuf, "TIMEOUT", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_ERROR:    
            strnzcpy(strStatusBuf, "ERROR", sizeof(strStatusBuf));    
            break;
        case PORTTESTER_STATUS_UNKNOWN:
        default:
            strnzcpy(strStatusBuf, "UNKNOWN", sizeof(strStatusBuf));    
    }

    iTotal = PortTesterStatus(pRef, 'spkt', NULL, 0);
    iReceived = PortTesterStatus(pRef, 'recv', NULL, 0);
    iSequence = PortTesterStatus(pRef, 'seqn', NULL, 0);

    ZPrintf("PT Callback: SEQ[%3d] RCVD[%3d] TOTAL[%3d] STATUS[%-16s]\n",
        iSequence, iReceived, iTotal, strStatusBuf);
}

/*F********************************************************************************/
/*!
    \Function _CmdPortTesterCB

    \Description
        Callback from tester to allow us to pump the lobby

    \Input Standard argz, argc, argv
    
    \Output Standard return codes
    
    \Version 03/10/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _CmdPortTesterCB(ZContext *argz, int32_t argc, char *argv[])
{
    // handle the shutdown case
    if (argc == 0)
    {
        // check to make sure we haven't already quit
        if (pPortTester) PortTesterDestroy(pPortTester);
        pPortTester = 0;
        return(0);
    }

    // re-call me in 50ms, just like the ticker sample
    return(ZCallback(_CmdPortTesterCB,50));
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdPortTester

    \Description
        Test the PortTester module.

    \Input Standard zlib stuff
    
    \Output Standard result code

    \Version 03/10/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdPortTester(ZContext *argz, int32_t argc, char *argv[])
{
    // check usage
    if (argc < 2)
    {
        ZPrintf("   test the porttester module\n"); 
        ZPrintf("   usage: %s [create|connect|testport|disconnect|destroy]\n", argv[0]);
        ZPrintf("      create <ip> <port> <time> - create porttester\n");
        ZPrintf("      connect <server> <port> - connect to server\n");
        ZPrintf("      testport <port> - request a porttester test\n");
        ZPrintf("      disconnect - disconnect from the lobby\n");
        ZPrintf("      destroy    - destroy porttester\n");
        return(0);
    }
    // now use it
    if (argc >= 2)
    {
        char *strCmd = argv[1];
        if (strcmp(strCmd,"create")==0)
        {
            int32_t iServerIP;
            int32_t iServerPort;
            int32_t iTimeout;

            iServerIP = SocketInTextGetAddr("159.153.229.211"); // porttest01.ea.com; for dev use plum.online.ea.com
            iServerPort = 9660;
            iTimeout = 5000;
            
            // see if we should override them
            // check for dot separated IP address
            if (argc >= 3)
            {
                iServerIP = _PortTesterGetIPAddressFromString(argv[2]);
            }
            // check for port number
            if (argc >= 4)
            {
                iServerPort = atoi(argv[3]);
            }
            // check for timeout value
            if (argc >= 5)
            {
                iTimeout = atoi(argv[4]);
            }

            // create the port tester
            pPortTester = PortTesterCreate(iServerIP, iServerPort, iTimeout);
            ZPrintf("porttester: create addr=%a port=%d time=%d\n", iServerIP, iServerPort, iTimeout);

            // start the callback
            ZCallback(_CmdPortTesterCB, 50);
        }
        else if (strcmp(strCmd,"connect")==0)
        {
            PortTesterConnect(pPortTester, NULL); //CHANGEME ?
            ZPrintf("porttester: PortTesterConnect\n");
        }
        else if (strcmp(strCmd,"disconnect")==0)
        {
            if (pPortTester)         PortTesterDisconnect(pPortTester);
            ZPrintf("porttester: PortTesterDisconnect\n");
        }
        else if (strcmp(strCmd,"destroy")==0)
        {
            // destroy the port tester
            if (pPortTester)             
                PortTesterDestroy(pPortTester);
            pPortTester=NULL;
            
            ZPrintf("porttester: PortTesterDestroy\n");
        }
        else if ((strcmp(strCmd,"tp")==0) ||
                 (strcmp(strCmd,"testport")==0))
        {
            int32_t iPort = 3658;

            if (argc >= 3)
            {
                iPort = strtol(argv[2], NULL, 10);
            }

            PortTesterTestPort(pPortTester, iPort, 8, &_PortTesterStatusCallback, NULL);
            ZPrintf("porttester: testing port %d\n", iPort);

            // start the callback
            ZCallback(_CmdPortTesterCB,50);
        }
        else if (strcmp(strCmd, "open")==0)
        {
            int32_t iPort = 3658;
            if (argc >= 3)
            {
                iPort = strtol(argv[2], NULL, 10);
            }
            PortTesterOpenPort(pPortTester, iPort);
            ZPrintf("porttester: opening port %d\n", iPort);
        }
        else
        {
            ZPrintf("porttester: unknown command [%s].\n", strCmd);
        }
    }

    return(0);
}

