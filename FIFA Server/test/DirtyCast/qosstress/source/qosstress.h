/*H********************************************************************************/
/*!
    \File qosstress.h

    \Description
        This module implements the main logic for the QosStress client.
        
        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _qosstress_h
#define _qosstress_h

/*** Include files ****************************************************************/

#include "dirtycast.h"
#include "configfile.h"
#include "DirtySDK/misc/qosclient.h"
#include "serverdiagnostic.h"
#include "qosstressconfig.h"
#include "qosstressmetrics.h"


/*** Defines **********************************************************************/

// shutdown flags
#define QOSSTRESS_SHUTDOWN_IMMEDIATE   (1)
#define QOSSTRESS_SHUTDOWN_IFEMPTY     (2)

// the bot can use more than one qos profile
#define QOSSTRESS_MAX_PROFILES         (8)
/*** Type Definitions *************************************************************/

typedef enum QosStressStateE
{
    QOS_STRESS_STATE_INITIALIZE_BOT = 0,                    
    QOS_STRESS_STATE_START_DIRTY_SDK,             
    QOS_STRESS_STATE_CREATE_QOS_CLIENT,           
    QOS_STRESS_STATE_START_QOS_TEST,  
    QOS_STRESS_STATE_WAIT_FOR_QOS_COMPLETE,
    QOS_STRESS_STATE_WAIT_FOR_NEXT_START_TIME,
    QOS_STRESS_STATE_DESTROY_QOS_CLIENT,          
    QOS_STRESS_STATE_SHUTDOWN_DIRTY_SDK,           
    QOS_STRESS_STATE_SHUTDOWN_BOT        
} QosStressStateE;

//! QosStress module
class QosStressC
{
public:
    QosStressC();
    ~QosStressC();


    //! main process loop
    bool Process(uint32_t uShutdownFlags);

    //! Functions mapped to each value of QosStressStateE
    bool Initialize(int argc, const char *argv[], const char *pConfigFile);
    void StartDirtySDK();
    void CreateQosClient();
    void StartQosTest();
    void CompleteQosTest();
    void DestroyQosClient();
    void ShutdownDirtySDK();
    void Shutdown();
    QosStressConfigC GetConfig();

private:
    //util
    static void QosStressPrintf(const char *pFormat, ...);
    static void QosStressCallback(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo, void *pUserData);
    void Report(QosCommonProcessedResultsT *pCBInfo);
    void Metrics(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo);
    void PopulateHostName();

    char m_aProfiles[QOSSTRESS_MAX_PROFILES][QOS_COMMON_MAX_RPC_STRING];            //!< divide up the list of profiles into individual profiles
    QosStressConfigC m_Config;
    ConfigFileT *m_pConfig;     //! config file reader instance
    QosClientRefT* m_pQosClient;
    QosStressMetricsRefT* m_QosStressMetrics;
    uint8_t m_bDirtySDKActive;

    QosStressStateE m_eStressState;
    uint32_t m_uRunCount;
    uint32_t m_uLastQosStartTime;
    uint32_t m_uProfileCount;
    uint32_t m_uAcitveProfile;
};

#endif // _qosstress_h

