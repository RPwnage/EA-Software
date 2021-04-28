/*H********************************************************************************/
/*!
    \File qosstress.cpp

    \Description
        This module implements the main logic for the QosStress bot.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 06/15/2017 (cvienneau) 2.0
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <EABase/eabase.h>

#ifndef _WIN32
#include <unistd.h>
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
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "qosstress.h"
#include "qosstressmetrics.h"


/*** Defines **********************************************************************/
#if !defined(DIRTYCODE_PC) && !defined(DIRTYCODE_XBOXONE)
#define strtok_s strtok_r
#endif

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function QosStressC constructor
 
    \Description
        Construct a new QosStress instance.
  
    \Input None.

    \Output None.
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
QosStressC::QosStressC()
    : m_pConfig(NULL),
      m_pQosClient(NULL),
      m_QosStressMetrics(NULL),
      m_bDirtySDKActive(FALSE),
      m_eStressState(QOS_STRESS_STATE_INITIALIZE_BOT),
      m_uRunCount(0),
      m_uLastQosStartTime(0),
      m_uProfileCount(0),
      m_uAcitveProfile(0)
{
    ds_memclr(&m_Config, sizeof(m_Config));
}

/*F********************************************************************************/
/*!
    \Function QosStressC destructor
 
    \Description
        Destroy a QosStress instance.

    \Input None.

    \Output None.
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
QosStressC::~QosStressC()
{
}

/*F*************************************************************************************/
/*!
    \Function QosStressC::QosStressPrintf

    \Description
        Printf for the qosstress client. Not disabled in release build. 

    \Input *pApp    - app ref
    \Input *pFormat - failure format string
    \Input ...      - failure format args

    \Output
        None.

    \Version 07/09/2009 (mclouatre) initial version - same pattern as QosStressPrintf func created by JBrookes for VoipTunnel qosstress client
*/
/**************************************************************************************F*/
void QosStressC::QosStressPrintf(const char *pFormat, ...)
{
    char strText[1024];
    va_list Args;

    va_start(Args, pFormat);
    ds_vsnzprintf(strText, sizeof(strText), pFormat, Args);
    va_end(Args);

    fprintf(stdout, "%s", strText);
    fflush(stdout);
#if defined(DIRTYCODE_PC)
    OutputDebugString(strText);
#endif
}

/*F********************************************************************************/
/*!
    \Function QosStressC::Initialize
     
    \Description
        Initialize the QosStress.  Read the config file, setup the network,
        and setup data structures.  Once this method completes successfully...

    \Input iArgCt           - number of command-line arguments
    \Input *pArgs[]         - command-line argument list
    \Input *pConfigTagsFile - config file name
 
    \Output
        bool                - true on success; false otherwise.
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
bool QosStressC::Initialize(int32_t iArgCt, const char *pArgs[], const char *pConfigTagsFile)
{
    const char *pConfigTags, *pCommandTags;
    char strCmdTagBuf[4096];
    uint32_t uRunRateMetricsRateOffset = 0;
    char *pSave = NULL;
    char *pTokenTemp = NULL;

    // first thing we do is start up the logger
    if (DirtyCastLoggerCreate(iArgCt, pArgs) < 0)
    {
        printf("exit: %d\n", DIRTYCAST_EXIT_ERROR);
        exit(DIRTYCAST_EXIT_ERROR);
    }

    // parse command-line parameters into tagfields
    pCommandTags = DirtyCastCommandLineProcess(strCmdTagBuf, sizeof(strCmdTagBuf), 2, iArgCt, pArgs, "");

    // log startup and build options
    DirtyCastLogPrintf("qosstress: INFO, started debug=%d profile=%d strCmdTagBuf=%s\n", DIRTYCODE_DEBUG, DIRTYCODE_PROFILE, pCommandTags);

    // create config file instance
    if ((m_pConfig = ConfigFileCreate()) == NULL)
    {
        DirtyCastLogPrintf("qosstress: ERROR, creating config file instance\n");
        return(false);
    }

    // load the config file
    DirtyCastLogPrintf("qosstress: INFO, loading config file %s\n", pConfigTagsFile);
    ConfigFileLoad(m_pConfig, pConfigTagsFile, "");
    ConfigFileWait(m_pConfig);

    // make sure we got a config file
    pConfigTags = ConfigFileData(m_pConfig, "");
    if ((pConfigTags == NULL) || (*pConfigTags == '\0'))
    {
        DirtyCastLogPrintf("qosstress: ERROR, no config file found\n");
        return(false);
    }

    // setup the logger
    DirtyCastLoggerConfig(pConfigTags);

    // get the configuration values
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "qoscaddr", m_Config.strQosServerAddress, sizeof(m_Config.strQosServerAddress), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "qosprofile", m_Config.strQosProfile, sizeof(m_Config.strQosProfile), "not-a-profile");
    m_Config.uQosSendPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "qoscport", 10010);
    m_Config.uQosListenPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "qosprobeport", 10000); //the local port to use
    m_Config.bUseSSL = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "usessl", 0);
    m_Config.bIgnoreSSLErrors = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "ignoresslerrors", 0);
    m_Config.iHttpSoLinger = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "httpsolinger", -1);
    m_Config.uDebugLevel = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "debuglevel", 0);
    m_Config.uMaxRunCount = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "maxruncount", 0);
    m_Config.uMaxRunRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "maxrunrate", 0);
    m_Config.bShutdownDsAfterEachTest = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "shutdowndsaftereachtest", 0);
    m_Config.bDestroyQosClientAfterEachTest = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "destroyqosclientaftereachtest", 0);

    // separate the profile list into an array (note that this is destructive to m_Config.strQosProfile)
    pTokenTemp = strtok_s(m_Config.strQosProfile, ",", &pSave);
    while (pTokenTemp != NULL)
    {
        ds_strnzcpy(m_aProfiles[m_uProfileCount], pTokenTemp, sizeof(m_aProfiles[m_uProfileCount]));
        pTokenTemp = strtok_s(NULL, ",", &pSave);
        m_uProfileCount++;
        if (m_uProfileCount >= QOSSTRESS_MAX_PROFILES)
        {
            DirtyCastLogPrintf("qosstress: ERROR, too many qos profiles provided, max %d.\n", QOSSTRESS_MAX_PROFILES);
            break;
        }
    }
    // rebuild the m_Config.strQosProfile for use in OI metrics, with '_' instead of ',' which is problematic for OI anyways
    ds_strnzcpy(m_Config.strQosProfile, m_aProfiles[0], sizeof(m_Config.strQosProfile));
    for (uint32_t i = 1; i < m_uProfileCount; i++)
    {
        ds_strnzcat(m_Config.strQosProfile, "_", sizeof(m_Config.strQosProfile));
        ds_strnzcat(m_Config.strQosProfile, m_aProfiles[i], sizeof(m_Config.strQosProfile));
    }

    // get test override values
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "overridesitename", m_Config.strOverrideSiteName, sizeof(m_Config.strOverrideSiteName), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "overridesiteaddr", m_Config.strOverrideSiteAddr, sizeof(m_Config.strOverrideSiteAddr), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "overridesitekey", m_Config.strOverrideSiteKey, sizeof(m_Config.strOverrideSiteKey), "");
    m_Config.uOverrideSitePort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "overridesiteport", 0);
    m_Config.uOverrideSiteVersion = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "overridesiteversion", 512);

    //setup what our host name is
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "hostname", m_Config.strLocalHostName, sizeof(m_Config.strLocalHostName), "");

    //metrics related
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "metricsaggregator.addr", m_Config.strMetricsAggregatorHost, sizeof(m_Config.strMetricsAggregatorHost), "localhost");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "pool", m_Config.strPool, sizeof(m_Config.strPool), "global");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "deployinfo", m_Config.strDeployInfo, sizeof(m_Config.strDeployInfo), "servulator");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "deployinfo2", m_Config.strDeployInfo2, sizeof(m_Config.strDeployInfo2), "servulator");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "pingsite", m_Config.strPingSite, sizeof(m_Config.strPingSite), "UNKNOWN_SITE");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "environment", m_Config.strEnvironment, sizeof(m_Config.strEnvironment), "UNKNOWN_ENV");
    m_Config.uMetricsAggregatorPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.port", 0);
    m_Config.eMetricsFormat = (ServerMetricsFormatE)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.format", SERVERMETRICS_FORMAT_DATADOG);
    m_Config.uMetricsPushingRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "metricsaggregator.rate", 30000);

    //you can't shutdown dirtysock without destroying the QOS client
    if (m_Config.bShutdownDsAfterEachTest)
    {
        m_Config.bDestroyQosClientAfterEachTest = TRUE;
    }

    //we try to be sure that the metrics reporting and the actual run don't happen at ~ the same time
    //or we can get metrics artifacts when 0 metrics are available, then 2, instead of 1 then 1
    if (m_Config.uMaxRunRate == m_Config.uMetricsPushingRate)
    {
        uRunRateMetricsRateOffset = m_Config.uMaxRunRate - 2000;
    }
    //setup timer for our first run
    m_uLastQosStartTime = NetTick() - uRunRateMetricsRateOffset;
    
    // done
    DirtyCastLogPrintf("qosstress: INFO, client initialized\n");

    m_eStressState = QOS_STRESS_STATE_START_DIRTY_SDK;
    return(true);
}

void QosStressC::PopulateHostName()
{
    // use hostname specified in config, otherwise query from system
    if (m_Config.strLocalHostName[0] != '\0')
    {
        DirtyCastLogPrintf("qosstress: hostname (%s) inherited from config\n", m_Config.strLocalHostName);
    }
    else
    {
        // do the reverse lookup
        int32_t iResult;
        struct sockaddr Addr;
        uint32_t uFrontAddr = SocketGetLocalAddr();
        SockaddrInit(&Addr, AF_INET);
        SockaddrInSetAddr(&Addr, uFrontAddr);
        SockaddrInSetPort(&Addr, m_Config.uQosListenPort);

        // TODO this code is borrowed from _VoipServerInitialize look into refactoring that code so we can use it directly here
        if ((iResult = DirtyCastGetHostNameByAddr(&Addr, sizeof(Addr), m_Config.strLocalHostName, sizeof(m_Config.strLocalHostName), NULL, 0)) == 0)
        {
            DirtyCastLogPrintf("voipserver: resolved hostname to %s by addr %a\n", m_Config.strLocalHostName, uFrontAddr);
        }
        else if ((iResult = DirtyCastGetHostName(m_Config.strLocalHostName, sizeof(m_Config.strLocalHostName))) == 0)
        {
            char *pEnd;
            if ((pEnd = strchr(m_Config.strLocalHostName, '.')) != NULL)
            {
                *pEnd = '\0';
            }

            DirtyCastLogPrintf("voipserver: reverse lookup failed falling back to appending .ea.com to %s\n", m_Config.strLocalHostName);
            ds_strnzcat(m_Config.strLocalHostName, ".ea.com", sizeof(m_Config.strLocalHostName));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function QosStressC::Shutdown
 
    \Description
        Shutdown the QosStress.
  
    \Output None.
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void QosStressC::Shutdown(void)
{
    DirtyCastLogPrintf("qosstress: INFO, shutting down\n");

    if (m_pQosClient != NULL)
    {
        QosClientDestroy(m_pQosClient);
    }

    if (m_bDirtySDKActive)
    {
        NetConnShutdown(0);
    }

    if (m_QosStressMetrics)
    {
        QosStressMetricsDestroy(m_QosStressMetrics);
    }

    if (m_pConfig != NULL)
    {
        ConfigFileDestroy(m_pConfig);
        m_pConfig = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function QosStressC::QosClientCallback
 
    \Description
        Gets updated with information on qos.
  
    \Output None.
 
    \Version 03/31/2011 cvienneau
*/
/********************************************************************************F*/
void QosStressC::QosStressCallback(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo, void *pUserData)
{
    QosStressC *pQosStress = static_cast<QosStressC*>(pUserData);

    // use the final results to output a report
    pQosStress->Report(pCBInfo);
    pQosStress->Metrics(pQosClient, pCBInfo);

    // do next steps in state machine 
    pQosStress->CompleteQosTest();
}

/*F********************************************************************************/
/*!
    \Function QosStressC::CompleteQosTest

    \Description
        Perform necessary state machine actions (based on settings) once 
        a qos test finishes. 

    \Output None.

    \Version 03/31/2016 cvienneau
*/
/********************************************************************************F*/
void QosStressC::CompleteQosTest()
{
    if ((m_Config.uMaxRunCount != 0) && (m_uRunCount >= m_Config.uMaxRunCount))
    {
        DirtyCastLogPrintf("qosstress: INFO, uMaxRunCount(%d) reached, exiting.\n", m_uRunCount);
        m_eStressState = QOS_STRESS_STATE_SHUTDOWN_BOT;
    }
    else if (m_Config.bDestroyQosClientAfterEachTest)
    {
        m_eStressState = QOS_STRESS_STATE_DESTROY_QOS_CLIENT;
    }
    else
    {
        m_eStressState = QOS_STRESS_STATE_WAIT_FOR_NEXT_START_TIME;
    }
}

/*F********************************************************************************/
/*!
    \Function QosStressC::Report

    \Description
        Report stress test progress after each test completes.

    \Output None.

    \Version 03/31/2016 cvienneau
*/
/********************************************************************************F*/
void QosStressC::Report(QosCommonProcessedResultsT *pCBInfo)
{
    uint32_t iterationTimeDiff;
    uint32_t processTimeDiff;
    uint32_t averageIterationTimeDiff;
    static uint32_t uProcessStartTick = NetTick();
    static uint32_t uCurrentTick = 0; 
    static uint32_t uLastTick = 0;

    uLastTick = uCurrentTick;
    uCurrentTick = NetTick();
    processTimeDiff = NetTickDiff(uCurrentTick, uProcessStartTick);

    if (processTimeDiff == 0)
    {
        processTimeDiff = 1;
        iterationTimeDiff = 1;
    }
    else
    {
        iterationTimeDiff = NetTickDiff(uCurrentTick, uLastTick);
    }

    averageIterationTimeDiff = processTimeDiff / m_uRunCount;

    if (averageIterationTimeDiff == 0)
        averageIterationTimeDiff = 1;
    
    DirtyCastLogPrintf("qosstress: TIMING(count:%d, ms:%d, per/minute:%d)\n",
            m_uRunCount,
            iterationTimeDiff,
            60000 / averageIterationTimeDiff
        );
}

/*F********************************************************************************/
/*!
    \Function QosStressC::Metrics

    \Description
        Adds data to be sent to OIv2 metrics system.

    \Output None.

    \Version 04/10/2018 cvienneau
*/
/********************************************************************************F*/
void QosStressC::Metrics(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo)
{
    if (m_QosStressMetrics)
    {
        QosStressMetricsAccumulate(m_QosStressMetrics, pCBInfo);
    }
}

void QosStressC::StartDirtySDK()
{
    // disable time stamp for qos bot (it will double stamp if not)
    NetTimeStampEnable(FALSE);

    // start DirtySock with no UPnP and in single-threaded mode
    NetConnStartup("-noupnp -singlethreaded -servicename=qosstress");
    // test some DirtySock stuff
    NetPrintf(("main: addr=%a\n", NetConnStatus('addr', 0, NULL, 0)));
    NetPrintf(("main: macx=%s\n", NetConnMAC()));
    m_bDirtySDKActive = TRUE;
    m_eStressState = QOS_STRESS_STATE_CREATE_QOS_CLIENT;
    PopulateHostName();
    
    if(m_Config.uMetricsAggregatorPort != 0)
    {
        m_QosStressMetrics = QosStressMetricsCreate(&m_Config);
    }
}

void QosStressC::ShutdownDirtySDK()
{
    if (m_QosStressMetrics)
    {
        QosStressMetricsDestroy(m_QosStressMetrics);
    }    
    NetConnShutdown(0);
    m_bDirtySDKActive = FALSE;
    m_eStressState = QOS_STRESS_STATE_START_DIRTY_SDK;

}

void QosStressC::CreateQosClient()
{
    if(m_pQosClient == NULL)
    {
        m_pQosClient = QosClientCreate(QosStressCallback, this, m_Config.uQosListenPort);
        QosClientControl(m_pQosClient, 'ussl', m_Config.bUseSSL, NULL);
        QosClientControl(m_pQosClient, 'ncrt', m_Config.bIgnoreSSLErrors, NULL);
        QosClientControl(m_pQosClient, 'soli', m_Config.iHttpSoLinger, NULL);
        QosClientControl(m_pQosClient, 'spam', m_Config.uDebugLevel, NULL);

        // apply test overrides if they have been provided
        if (m_Config.strOverrideSiteName[0] != '\0')
        {
            uint8_t aOverrideSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];
            QosClientControl(m_pQosClient, 'orts', 0, m_Config.strOverrideSiteName);
            QosClientControl(m_pQosClient, 'orta', 0, m_Config.strOverrideSiteAddr);
            ds_memclr(aOverrideSecureKey, sizeof(aOverrideSecureKey));
            ds_snzprintf((char*)aOverrideSecureKey, sizeof(aOverrideSecureKey), "%s", m_Config.strOverrideSiteKey);
            QosClientControl(m_pQosClient, 'ortk', 0, aOverrideSecureKey);
            QosClientControl(m_pQosClient, 'ortp', m_Config.uOverrideSitePort, NULL);
            QosClientControl(m_pQosClient, 'ortv', m_Config.uOverrideSiteVersion, NULL);
        }

    }
    else
    {
        DirtyCastLogPrintf("qosstress: ERROR, QosStressC::StartQos m_pQosClient != NULL");
    }
    m_eStressState = QOS_STRESS_STATE_WAIT_FOR_NEXT_START_TIME;
}

void QosStressC::StartQosTest()
{
    m_uAcitveProfile = m_uRunCount % m_uProfileCount;
    QosClientStart(m_pQosClient, m_Config.strQosServerAddress, m_Config.uQosSendPort, m_aProfiles[m_uAcitveProfile]);
    m_uRunCount++;
    m_eStressState = QOS_STRESS_STATE_WAIT_FOR_QOS_COMPLETE;
}

void QosStressC::DestroyQosClient()
{
    if(m_pQosClient != NULL)
    {
        QosClientDestroy(m_pQosClient);
        m_pQosClient = NULL;
    }
    else
    {
        DirtyCastLogPrintf("qosstress: ERROR, QosStressC::ShutdownQos m_pQosClient == NULL");
    }

    if (m_Config.bShutdownDsAfterEachTest)
    {
        m_eStressState = QOS_STRESS_STATE_SHUTDOWN_DIRTY_SDK;
    }
    else
    {
        m_eStressState = QOS_STRESS_STATE_CREATE_QOS_CLIENT;
    }
}


QosStressConfigC QosStressC::GetConfig()
{
    return m_Config;
}

/*F********************************************************************************/
/*!
    \Function QosStressC::Process
 
    \Description
        Main server process loop.

    \Input uShutdownFlags - QOSSTRESS_SHUTDOWN_* flags indicating shutdown state

    \Output
        bool              - true to continue processing, false to exit
 
    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
bool QosStressC::Process(uint32_t uShutdownFlags)
{
    // give some time to registered idle functions
    NetConnIdle();

    // call the update for the qos module
    if (m_pQosClient)
    {
        QosClientUpdate(m_pQosClient);
    }

    if (m_eStressState == QOS_STRESS_STATE_START_DIRTY_SDK)
    {
        StartDirtySDK();
    }
    else if (m_eStressState == QOS_STRESS_STATE_CREATE_QOS_CLIENT)
    {
        CreateQosClient();
    }
    else if (m_eStressState == QOS_STRESS_STATE_START_QOS_TEST)
    {
        StartQosTest();
    }
    else if (m_eStressState == QOS_STRESS_STATE_WAIT_FOR_QOS_COMPLETE)
    {
        // do nothing, wait for callback
        NetConnControl('poll', 4, 0, NULL, NULL);
    }
    else if (m_eStressState == QOS_STRESS_STATE_WAIT_FOR_NEXT_START_TIME)
    {
        // only push out metrics if we aren't currently doing qos tests
        if (m_QosStressMetrics)
        {
            QosStressMetricsUpdate(m_QosStressMetrics);
        }

        // we run all the profiles as fast as we can, but we wait at the start of each batch
        if ((m_uAcitveProfile != 0) || (NetTickDiff(NetTick(), m_uLastQosStartTime) >= (int32_t)m_Config.uMaxRunRate))
        {
            m_uLastQosStartTime = NetTick();
            m_eStressState = QOS_STRESS_STATE_START_QOS_TEST;
        }
        else
        {
            NetConnControl('poll', 4, 0, NULL, NULL);
        }
    }    
    else if (m_eStressState == QOS_STRESS_STATE_DESTROY_QOS_CLIENT)
    {
        DestroyQosClient();
    }
    else if (m_eStressState == QOS_STRESS_STATE_SHUTDOWN_DIRTY_SDK)
    {
        ShutdownDirtySDK();
    }
    else if (m_eStressState == QOS_STRESS_STATE_SHUTDOWN_BOT)
    {
        DirtyCastLogPrintf("qosstress: reached state QOS_STRESS_STATE_SHUTDOWN_BOT\n");
        DirtyCastLoggerDestroy();
        return(false);
    }

    
    // process shutdown flags
    if (uShutdownFlags & QOSSTRESS_SHUTDOWN_IMMEDIATE)
    {
        DirtyCastLogPrintf("qosstress: INFO, executing immediate shutdown (flags=0x%08x)\n", uShutdownFlags);
        DirtyCastLoggerDestroy();
        return(false);
    }
    return(true);
}

