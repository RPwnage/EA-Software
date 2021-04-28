/*H********************************************************************************/
/*!
    \File T2Driver.cpp

    \Description
        Sample that allows for running command on the T2Client, by exchanging
        messages both ways

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/22/2007 (jrainy) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "dirtysock.h"
#include "dirtylib.h"
#include "lobbytagfield.h"
#include "zmem.h"
#include "zmemtrack.h"
#include "testermodules.h"
#include "testerprofile.h"
#include "testerregistry.h"
#include "testerclientcore.h"
#include "testercomm.h"
#include "T2ClientResource.h"

typedef void (*T2DriverCallback)(char* message, int32_t length);

#define BEGIN_TAG "<T2CODE>"

typedef struct T2ClientT
{
    char strCurrentProfile[TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT];  // active profile
    TesterClientCoreT *pClientCore;             //!< pointer to client core structure
    int32_t iTimerID;                               //!< timer used to call NetIdle();
    char strConnectParams[512];                 //!< connect parameters
    T2DriverCallback callback;

} T2ClientT;
static T2ClientT T2Client = {"", NULL, 0, "", NULL};

VOID CALLBACK _T2ClientTimerCallback(HWND hDlg, UINT uMsg, UINT_PTR pIDEvent, DWORD dTime);

extern "C" {
    int32_t TesterCommCanAttachXenon(void);
}
static void _T2ClientDisplayOutput(const char *pBuf, int32_t iLen, int32_t iRefcon, void *pRefptr)
{
    char buffer[2048];
    const char* scan;
    int32_t length;
    printf("%s\n", pBuf);

    scan = strstr(pBuf,BEGIN_TAG);

    if (scan != NULL)
    {
        scan += strlen(BEGIN_TAG);
        length = TagFieldGetBinary(TagFieldFind(scan, "DATA"), buffer, sizeof(buffer));
        T2Client.callback(buffer, length);
    }
}

void T2DriverStartup()
{
    ZMemtrackStartup();

    // create and connect the core module
    T2Client.pClientCore = TesterClientCoreCreate("");

    // set the display function
    TesterClientCoreDisplayFunc(T2Client.pClientCore, _T2ClientDisplayOutput, 0, 0);

    #if !defined(TESTER_NO_XBDM)
    TagFieldSetNumber(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "PROFILENUM", -1);
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "PROFILENAME", "xenon_jrainy_1");
    TagFieldSetNumber(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "PLATFORM", TESTER_PLATFORM_XBDM);
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "INPUTFILE", "clientinput.txt");
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "OUTPUTFILE", "clientoutput.txt");
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "CONTROLDIR", "");
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "HOSTNAME", "xenon_jrainy_1");
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "SCRIPT", "");
    TagFieldSetString(T2Client.strConnectParams, sizeof(T2Client.strConnectParams), "LOGFILE", "clientlog.txt");
    #endif

    // connect the client core
    TesterClientCoreConnect(T2Client.pClientCore, T2Client.strConnectParams);
}

void T2DriverRun()
{
    TesterClientCoreIdle(T2Client.pClientCore);
}

void T2DriverShutdown()
{
    // disconnect and destroy the client core module
    TesterClientCoreDestroy(T2Client.pClientCore);

    ZMemtrackShutdown();
}

void T2DriverSendMessage(char* message, int32_t length)
{
    char buffer[2048];
    char sendBuffer[2048];

    TagFieldClear(buffer, sizeof(buffer));
    TagFieldSetBinary(buffer, sizeof(buffer), "DATA", message, length);

    sprintf(sendBuffer, "driver command %s", buffer);

    TesterClientCoreSendCommand(T2Client.pClientCore, sendBuffer);
}

void T2DriverSetRecvCallback(T2DriverCallback callback)
{
    T2Client.callback = callback;
}




void RecvMessage(char* message, int32_t length)
{
    message[length] = 0;
    printf("T2Driver received: %s\n", message);
}

int main()
{
    T2DriverStartup();
    T2DriverSetRecvCallback(&RecvMessage);

    T2DriverSendMessage("HELLO WORLD!", 12);
    while(true)
    {
        T2DriverRun();
    }
    T2DriverShutdown();


}

