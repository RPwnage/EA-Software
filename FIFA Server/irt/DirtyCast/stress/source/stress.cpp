/*H********************************************************************************/
/*!
    \File stress.cpp

    \Description
        This module implements the main logic for the Stress server.
        
        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <EABase/eabase.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/voip/voip.h"

#include "configfile.h"
#include "dirtycast.h"
#include "stressblaze.h"
#include "stressconfig.h"
#include "stressgameplay.h"
#include "stressgameplaymetrics.h"
#include "stress.h"

/*** Defines **********************************************************************/

#define STRESS_MEMID ('strs')
#define GAME_STRESS_INDEX_PORT_OFFSET   (10000)

/*** Type Definitions *************************************************************/

struct StressRefT
{
    ConfigFileT *pConfig;       //!< config file reader instance
    char strPidFileName[256];   //!< pid filename
    StressConfigC Config;       //!< configurations
    bool bMustLogin;            //!< do we need to login to the blaze server
    uint8_t _pad[3];
    uint32_t uPollRate;         //!< how long to wait for input before running the update functions
    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserData;    //!< memgroup userdata
    StressBlazeC *pStressBlaze; //!< object in charge of dealing with Blaze
    StressGamePlayC *pStressGamePlay; //!< object in charge of dealing with Game Play 
    StressGameplayMetricsRefT *pStressGameplayMetrics; //!< object in charge of dealing with Metrics
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _StressGetCurrentTime
 
    \Description
        Get current time.

    \Input *uYear   - [out] storage for year
    \Input *uMonth  - [out] storage for month
    \Input *uDay    - [out] storage for day
    \Input *uHour   - [out] storage for hour
    \Input *uMin    - [out] storage for minute
    \Input *uSec    - [out] storage for second
    \Input *uMillis - [out] storage for milliseconds

    \Version 06/20/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _StressGetCurrentTime(uint32_t *uYear, uint8_t *uMonth, uint8_t *uDay, uint8_t *uHour, uint8_t *uMin, uint8_t *uSec, uint32_t *uMillis)
{
    #ifdef _WIN32
    SYSTEMTIME SystemTime;

    GetLocalTime(&SystemTime);
    *uYear = SystemTime.wYear;
    *uMonth = SystemTime.wMonth;
    *uDay = SystemTime.wDay;
    *uHour = SystemTime.wHour;
    *uMin = SystemTime.wMinute;
    *uSec = SystemTime.wSecond;
    *uMillis = SystemTime.wMilliseconds;
    #else
    struct timeval tv;
    struct tm *pTime;

    gettimeofday(&tv, NULL);
    pTime = gmtime((time_t *)&tv.tv_sec);
    *uYear = 1900 + pTime->tm_year;
    *uMonth = pTime->tm_mon + 1;
    *uDay = pTime->tm_mday;
    *uHour = pTime->tm_hour;
    *uMin = pTime->tm_min;
    *uSec = pTime->tm_sec;
    *uMillis = tv.tv_usec / 1000;
    #endif
}

/*F********************************************************************************/
/*!
    \Function _StressPrintfHook
    
    \Description
        Print output to stdout so it is visible when run from the commandline as well
        as in the debugger output.
    
    \Input *pParm   - unused
    \Input *pText   - text to print
    
    \Output
        int32_t     - if zero, suppress OutputDebugString printing by NetPrintf()

    \Notes
        If the debuglevel is greater than two, we suppress OutputDebugString printing,
        as there is so much output, printing debug output impedes the functioning of
        the server.

    \Version 04/04/2007 (jbrookes)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
static int32_t _StressPrintfHook(void *pParm, const char *pText)
{
    uint8_t  uMonth, uDay, uHour, uMin, uSec;
    uint32_t uYear, uMillis;

    _StressGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis);
    fprintf(stdout, "%4u/%02u/%02u %02u:%02u:%02u.%03u %s", uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, pText);
    fflush(stdout);

    // if PC, we want lower layer to issue debug output
    #if defined(DIRTYCODE_PC)
    return(1);
    #else
    return(0);
    #endif

}
#endif

/*F********************************************************************************/
/*!
    \Function _StressPopulateHostName
    
    \Description
        Figures out the hostname that can be used for this stress client instance

    \Input *pStress - module state
    
    \Version 08/20/2019 (cvienneau)
*/
/********************************************************************************F*/
static void _StressPopulateHostName(StressRefT *pStress)
{
    // use hostname specified in config, otherwise query from system
    if (pStress->Config.strLocalHostName[0] != '\0')
    {
        StressPrintf("stress: hostname (%s) inherited from config\n", pStress->Config.strLocalHostName);
    }
    else
    {
        // do the reverse lookup
        int32_t iResult;
        struct sockaddr Addr;
        uint32_t uFrontAddr = SocketGetLocalAddr();
        SockaddrInit(&Addr, AF_INET);
        SockaddrInSetAddr(&Addr, uFrontAddr);
        SockaddrInSetPort(&Addr, pStress->Config.uGamePort);

        // TODO this code is borrowed from _VoipServerInitialize look into refactoring that code so we can use it directly here
        if ((iResult = DirtyCastGetHostNameByAddr(&Addr, sizeof(Addr), pStress->Config.strLocalHostName, sizeof(pStress->Config.strLocalHostName), NULL, 0)) == 0)
        {
            StressPrintf("stress: resolved hostname to %s by addr %a\n", pStress->Config.strLocalHostName, uFrontAddr);
        }
        else if ((iResult = DirtyCastGetHostName(pStress->Config.strLocalHostName, sizeof(pStress->Config.strLocalHostName))) == 0)
        {
            char *pEnd;
            if ((pEnd = strchr(pStress->Config.strLocalHostName, '.')) != NULL)
            {
                *pEnd = '\0';
            }

            StressPrintf("stress: reverse lookup failed falling back to appending .ea.com to %s\n", pStress->Config.strLocalHostName);
            ds_strnzcat(pStress->Config.strLocalHostName, ".ea.com", sizeof(pStress->Config.strLocalHostName));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _StressEventCallback

    \Description
        Callback handler for blaze events.

    \Input *pStress - Stress ref
    \Input iEvent       - type of event
    \Input *StressGameT - associated game object, when applicable
    
    \Output
        0- failure; 1- success

    \Version 08/26/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _StressEventCallback(void *pUserData, int32_t iEvent, StressGameT *pGameObject)
{
    StressRefT *pStress = (StressRefT *)pUserData;    

    switch (iEvent)
    {
        case STRESS_EVENT_CREATE:
            NetPrintf(("stress: event notification from StressBlaze -> STRESS_EVENT_CREATE\n"));
            break;

        case STRESS_EVENT_GAME:
            NetPrintf(("stress: event notification from StressBlaze -> STRESS_EVENT_GAME\n"));

            if (pStress->Config.uGameMode != 2)
            {
                if (pStress->pStressGamePlay == NULL)
                {
                    pStress->pStressGamePlay = new StressGamePlayC(pStress->pStressBlaze->GetConnApiAdapter(), pStress->pStressGameplayMetrics);

                    if (pStress->pStressGamePlay == NULL)
                    {
                        StressPrintf("stress: could not allocat game play object\n");
                        return(0);
                    }
                }

                pStress->pStressGamePlay->SetVerbosity(pStress->Config.uDebugLevel);
                pStress->pStressGamePlay->SetLateRate(pStress->Config.uLateRate);
                pStress->pStressGamePlay->SetDebugPoll(pStress->Config.bDebugPoll);
                pStress->pStressGamePlay->SetPacketSize(pStress->Config.uPacketSize);
                pStress->pStressGamePlay->SetPacketsPerSec(pStress->Config.uPacketsPerSec);
                pStress->pStressGamePlay->SetNetGameDistStreamId(pStress->Config.uGameStream);
                pStress->pStressGamePlay->SetGame(pStress->pStressBlaze->GetGame());
                pStress->pStressGamePlay->SetPersona(pStress->Config.strPersona);
                pStress->pStressGamePlay->SetWriteCsv(pStress->Config.bWriteCsv);
                pStress->pStressGamePlay->SetStreamMessageSize(pStress->Config.uStreamMessageSize);
                pStress->pStressGamePlay->SetPeer2Peer((pStress->Config.uGameMode == 3) || (pStress->Config.uGameMode == 4));

                if (pStress->Config.bUseDist)
                {
                    pStress->pStressGamePlay->SetDist();
                    NetPrintf(("stress: using NetGameDist\n"));
                }
                else
                {
                    NetPrintf(("stress: using NetGameLink\n"));
                }
            }
            else
            {
                NetPrintf(("stress: StressGamePlayC not instantiated because game mode is 2\n"));
            }

            break;

        case STRESS_EVENT_PLAY:
            NetPrintf(("stress: event notification from StressBlaze -> STRESS_EVENT_PLAY\n"));

            NetPrintf(("stress: game started with %d players\n", pStress->pStressBlaze->GetPlayerCount()));
            pStress->pStressGamePlay->SetNumberOfPlayers(pStress->pStressBlaze->GetPlayerCount());
            pStress->pStressGamePlay->SetGameStarted();

            break;

        case STRESS_EVENT_END:
            NetPrintf(("stress: event notification from StressBlaze -> STRESS_EVENT_END\n"));

            if (pStress->Config.uGameMode != 2)
            {
                if (pStress->pStressGamePlay != NULL)
                {
                   pStress->pStressGamePlay->SetGameEnded();
                }
            }

            break;

        case STRESS_EVENT_DELETE:

            NetPrintf(("stress: event notification from StressBlaze -> STRESS_EVENT_DELETE\n"));

            if (pStress->pStressGamePlay != NULL)
            {
                pStress->pStressGamePlay->SetGame(NULL);

                delete pStress->pStressGamePlay;
                pStress->pStressGamePlay = NULL;
            }
            break;

        default:
            NetPrintf(("stress: event notification from StressBlaze -> unknown event!\n"));
            break;
    }

   return(1);
}

/*** Public functions *************************************************************/


/*F*************************************************************************************/
/*!
    \Function StressPrintf

    \Description
        Printf for the stress client. Not disabled in release build. 

    \Input *pApp    - app ref
    \Input *pFormat - failure format string
    \Input ...      - failure format args

    \Version 07/09/2009 (mclouatre) initial version - same pattern as StressPrintf func created by JBrookes for VoipTunnel stress client
*/
/**************************************************************************************F*/
void StressPrintf(const char *pFormat, ...)
{
    char strText[1024];
    uint8_t  uMonth, uDay, uHour, uMin, uSec;
    uint32_t uYear, uMillis;
    va_list Args;

    va_start(Args, pFormat);
    ds_vsnzprintf(strText, sizeof(strText), pFormat, Args);
    va_end(Args);

    _StressGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis);
    fprintf(stdout, "%4u/%02u/%02u %02u:%02u:%02u.%03u %s", uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, strText);
    fflush(stdout);

    #if defined(DIRTYCODE_PC)
    OutputDebugString(strText);
    #endif
}

/*F********************************************************************************/
/*!
    \Function StressInitialize
     
    \Description
        Initialize the Stress.  Read the config file, setup the network,
        and setup data structures.  Once this method completes successfully...

    \Input iArgCt           - number of command-line arguments
    \Input *pArgs[]         - command-line argument list
    \Input *pConfigTagsFile - config file name
 
    \Output
        StressRefT *        - module ref on success, NULL on failure  
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
StressRefT *StressInitialize(int32_t iArgCt, const char *pArgs[], const char *pConfigTagsFile)
{
    StressRefT *pStress;
    const char *pAppName, *pBotIndex;
    const char *pConfigTags, *pCommandTags;
    char strCmdTagBuf[2048];
    uint16_t uBotIndex;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // set up NetPrintf() debug hook
    #if DIRTYCODE_LOGGING
    NetTimeStampEnable(FALSE);
    NetPrintfHook(_StressPrintfHook, NULL);
    #endif

    // log startup and build options
    StressPrintf("stress: started debug=%d profile=%d\n", DIRTYCODE_DEBUG, DIRTYCODE_PROFILE);

    // check usage
    if (iArgCt < 3)
    {
        StressPrintf("usage: %s <configfile> <botindex>\n", pArgs[0]);
        return(NULL);
    }

    // locate args
    pAppName = pArgs[0];
    pBotIndex = pArgs[2];
    uBotIndex = (uint16_t)atoi(pBotIndex);

    // query memgroup info
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate module state
    if ((pStress = (StressRefT *)DirtyMemAlloc(sizeof(*pStress), STRESS_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        StressPrintf("stress: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pStress, sizeof(*pStress));
    pStress->iMemGroup = iMemGroup;
    pStress->pMemGroupUserData = pMemGroupUserData;

    // parse command line parameters from tagfields
    pCommandTags = DirtyCastCommandLineProcess(strCmdTagBuf, sizeof(strCmdTagBuf), 3, iArgCt, pArgs, "");

    // create pid file using diagnostic port in name
    #if defined(DIRTYCODE_LINUX)
    if (DirtyCastPidFileCreate(pStress->strPidFileName, sizeof(pStress->strPidFileName), pAppName, uBotIndex) < 0)
    {
        StressPrintf("stress: error creating pid file -- exiting\n");
        DirtyMemFree(pStress, STRESS_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    #endif

    // create config file instance
    if ((pStress->pConfig = ConfigFileCreate()) == NULL)
    {
        StressPrintf("stress: error creating config file instance\n");
        StressShutdown(pStress);
        return(NULL);
    }

    // load the config file
    StressPrintf("stress: loading config file %s\n", pConfigTagsFile);
    ConfigFileLoad(pStress->pConfig, pConfigTagsFile, "");
    ConfigFileWait(pStress->pConfig);

    // make sure we got a config file
    pConfigTags = ConfigFileData(pStress->pConfig, "");
    if ((pConfigTags == NULL) || (*pConfigTags == '\0'))
    {
        StressPrintf("stress: error no config file found\n");
        StressShutdown(pStress);
        return(NULL);
    }

    // get the configuration values
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "USERNAME", pStress->Config.strUsername, sizeof(pStress->Config.strUsername), "gsstressbot");
    ds_snzprintf(pStress->Config.strUsername, sizeof(pStress->Config.strUsername), "%s%s@ea.com", pStress->Config.strUsername, pBotIndex);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "PASSWORD", pStress->Config.strPassword, sizeof(pStress->Config.strPassword), "gsstressbot");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "PERSONA", pStress->Config.strPersona, sizeof(pStress->Config.strPersona), "gsstressbot");
    ds_snzprintf(pStress->Config.strPersona, sizeof(pStress->Config.strPersona), "%s%s", pStress->Config.strPersona, pBotIndex);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "ENVIRONMENT", pStress->Config.strEnvironment, sizeof(pStress->Config.strEnvironment), "dev");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "RDIRNAME", pStress->Config.strRdirName, sizeof(pStress->Config.strRdirName), "");
    pStress->Config.uStressIndex = uBotIndex;
    pStress->Config.uGamePort = GAME_STRESS_INDEX_PORT_OFFSET + uBotIndex;
    pStress->Config.uPacketsPerSec = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "PACKETS_PER_SEC", 30);
    pStress->Config.uPacketSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "PACKET_SIZE", 20);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "SERVICE", pStress->Config.strService, sizeof(pStress->Config.strService), "OTG-2010-ps3");
    pStress->Config.iWaitForPlayers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "WAIT_FOR_PLAYERS", 3) * 1000;
    pStress->Config.iGameLength = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "GAME_LENGTH", 100) * 1000;
    pStress->Config.iDummyVoipSubPacketSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "VOIP_SUBPACKET_SIZE", 50);
    pStress->Config.iDummyVoipSendPeriod = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "VOIP_SUBPACKET_RATE", 250);
    pStress->Config.iWaitBetweenGames = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "WAIT_BETWEEN_GAMES", 10) * 1000;
    pStress->Config.iMaxGameCount = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "MAX_GAME_COUNT", 0);
    pStress->Config.eMaxGameCountAction = (StressGameCountActionE)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "MAX_GAME_COUNT_ACTION", kDisableMetrics);
    pStress->Config.uMinPlayers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "MIN_PLAYERS", 3);
    pStress->Config.uMaxPlayers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "MAX_PLAYERS", 4);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "GAME_STREAM", pStress->Config.strGameStream, sizeof(pStress->Config.strGameStream), "");
    pStress->Config.uGameStream  = pStress->Config.strGameStream[0] << 24;
    pStress->Config.uGameStream |= pStress->Config.strGameStream[1] << 16;
    pStress->Config.uGameStream |= pStress->Config.strGameStream[2] << 8;
    pStress->Config.uGameStream |= pStress->Config.strGameStream[3] << 0;
    pStress->Config.uStreamMessageSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "STREAM_MESSAGE_SIZE", 0);
    pStress->Config.uDebugLevel = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "DEBUG_LEVEL", 0);
    pStress->Config.bUseDist = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "USE_DIST", 0) ? true : false;
    pStress->Config.uGameMode = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "GAME_MODE", 0);
    pStress->Config.uLateRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "LATE_RATE", 60);
    pStress->Config.bDebugPoll = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "DEBUG_POLL", 0) ? true : false;
    pStress->Config.bUseSecureBlazeConn = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "USE_BLAZE_SECURE_CONN", 1) ? true : false;
    pStress->Config.bUseStressLogin = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "USE_STRESS_LOGIN", 0) ? true : false;
    pStress->Config.bWriteCsv = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "WRITE_CSV", 0) ? true : false;
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "GPVS", pStress->Config.strGameProtocolVersion, sizeof(pStress->Config.strGameProtocolVersion), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "MM_SCENARIO", pStress->Config.strMatchmakingScenario, sizeof(pStress->Config.strMatchmakingScenario), "dirtyCastStress");

    // metrics related
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "metricsaggregator.addr", pStress->Config.strMetricsAggregatorHost, sizeof(pStress->Config.strMetricsAggregatorHost), "localhost");
    pStress->Config.uMetricsEnabled = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.enabled", 1) ? true : false;
    pStress->Config.uMetricsAggregatorPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.port", 0);
    pStress->Config.eMetricsFormat = (ServerMetricsFormatE)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.format", SERVERMETRICS_FORMAT_DATADOG);
    pStress->Config.uMetricsPushingRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.rate", 30000);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "hostname", pStress->Config.strLocalHostName, sizeof(pStress->Config.strLocalHostName), "");
    _StressPopulateHostName(pStress);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "deployinfo", pStress->Config.strDeployInfo, sizeof(pStress->Config.strDeployInfo), "servulator");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "deployinfo2", pStress->Config.strDeployInfo2, sizeof(pStress->Config.strDeployInfo2), "servulator");

    StressPrintf("stress: config is %s\n", pConfigTagsFile);

    // create stress lobby module and initialize state
    pStress->pStressBlaze = StressBlazeC::Get();
    pStress->pStressBlaze->SetConfig(&pStress->Config);
    pStress->pStressBlaze->SetCallback(&_StressEventCallback, pStress);

    // create metrics system
    pStress->pStressGameplayMetrics = StressGameplayMetricsCreate(&pStress->Config);

    pStress->bMustLogin = true;

    if (pStress->Config.uPacketsPerSec == 0)
    {
        pStress->uPollRate = 1000;
    }
    else
    {
        pStress->uPollRate = 1000 / pStress->Config.uPacketsPerSec;
    }

    // done
    StressPrintf("stress: client initialized\n");
    return(pStress);
}

/*F********************************************************************************/
/*!
    \Function StressShutdown
 
    \Description
        Shutdown the Stress.

    \Input *pStress  - stress module state
  
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void StressShutdown(StressRefT *pStress)
{
    StressPrintf("stress: shutting down\n");

    if (pStress->pStressGameplayMetrics != NULL)
    {
        StressGameplayMetricsDestroy(pStress->pStressGameplayMetrics);
        pStress->pStressGameplayMetrics = NULL;
    }

    if (pStress->pConfig != NULL)
    {
        ConfigFileDestroy(pStress->pConfig);
        pStress->pConfig = NULL;
    }
    // remove pid file
    DirtyCastPidFileRemove(pStress->strPidFileName);
    // delete module memory
    DirtyMemFree(pStress, STRESS_MEMID, pStress->iMemGroup, pStress->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function StressProcess
 
    \Description
        Main server process loop.

    \Input *pStress         - module state
    \Input *pSignalFlags    - flags passed to the app

    \Output
        bool                - true to continue processing, false to exit
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
bool StressProcess(StressRefT *pStress, uint32_t *pSignalFlags)
{
    uint32_t uCurTick;

    // get current tick count
    uCurTick = NetTick();

    if (pStress->bMustLogin)
    {
        pStress->pStressBlaze->LoginToBlaze();
        pStress->bMustLogin = false;
    }
    else
    { 
        // If game play object exists, call its update method
        if (pStress->pStressGamePlay != NULL) 
        {
            // call gameplay update
            if (pStress->pStressGamePlay->Update(uCurTick) == 0)
            {
                StressPrintf("stress: game play failure ...\n");
                pStress->pStressGamePlay->SetGame(NULL);

                delete pStress->pStressGamePlay;
                pStress->pStressGamePlay = NULL;

                pStress->pStressBlaze->SetFailedState();
            }
        }
        else
        {
            #if defined(DIRTYCODE_LINUX)
            /* if voip sub-system is initialized (meaning we are running the single-threaded voip stress test client),
               then pump its update function in a manner that mimicks the _VoipThread, i.e. every 20 ms  */
            if (VoipGetRef())
            {
                uint32_t uTime0, uTime1;
                int32_t iSleep;

                uTime0 = NetTick();
                VoipControl(VoipGetRef(), 'updt', 0, NULL);
                uTime1 = NetTick();

                iSleep = 20 - NetTickDiff(uTime1, uTime0);
                if (iSleep < 0)
                {
                    iSleep = 0;
                }

                NetConnControl('poll', iSleep, 0, NULL, NULL);
            }
            else
            #endif
            {
                NetConnControl('poll', 33, 0, NULL, NULL);
            }
        }
    }

    NetConnIdle();

    if (!pStress->bMustLogin)
    {
        pStress->pStressBlaze->StressBlazeUpdate();
    }

    // give time to the metrics system to send to the aggregator
    if (pStress->Config.uMetricsEnabled == TRUE)
    {
        StressGameplayMetricsUpdate(pStress->pStressGameplayMetrics);
    }

    // update signal flags (may be altered in the app)
    if (pStress->Config.uSignalFlags != 0)
    {
        *pSignalFlags = pStress->Config.uSignalFlags;
    }
    // cache current signal flags
    pStress->Config.uSignalFlags = *pSignalFlags;

    // determine shutdown status
    if (*pSignalFlags & STRESS_SHUTDOWN_IMMEDIATE)
    {
        StressPrintf("stress: executing immediate shutdown (flags=0x%08x)\n", *pSignalFlags);
        return(false);
    }

    uint8_t uCanShutDown = FALSE;
    if (pStress->pStressBlaze->GetPlayerCount() == 0)
    {
        uCanShutDown = TRUE;
    }

    if ((*pSignalFlags & STRESS_SHUTDOWN_IFEMPTY) && (uCanShutDown))
    {
        StressPrintf("stress: executing ifempty shutdown\n");
        return(false);
    }
    // continue processing
    return(true);
}

/*F********************************************************************************/
/*!
    \Description
        Get module status

    \Input *pStress     - pointer to module state
    \Input iSelect      - status selector
    \Input iValue       - selector specific
    \Input *pBuf        - [out] - selector specific
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'spam' - return our debug level
        \endverbatim

    \Version 08/25/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t StressStatus(StressRefT *pStress, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufLen)
{
    if (iSelect == 'spam')
    {
        return(pStress->Config.uDebugLevel);
    }
    return(-1);
}

#ifndef EASTL_DEBUG_BREAK
void EASTL_DEBUG_BREAK()
{
}
#endif

