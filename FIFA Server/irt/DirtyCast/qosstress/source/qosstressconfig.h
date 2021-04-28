#ifndef _qosstressconfig_h
#define _qosstressconfig_h

#include "servermetrics.h"
#include "DirtySDK/misc/qoscommon.h"

#define QOSSTRESS_ENV_SIZE                  (16)
#define QOSSTRESS_DEPLOYINFO_SIZE           (128)
#define QOSSTRESS_DEPLOYINFO2_SIZE          (128)

typedef struct QosStressConfigC
{
    char strQosServerAddress[QOS_COMMON_MAX_RPC_STRING];
    char strMetricsAggregatorHost[QOS_COMMON_MAX_RPC_STRING];
    char strLocalHostName[QOS_COMMON_MAX_RPC_STRING];
    char strQosProfile[QOS_COMMON_MAX_RPC_STRING];
    char strPool[QOS_COMMON_MAX_RPC_STRING];
    char strDeployInfo[QOSSTRESS_DEPLOYINFO_SIZE];
    char strDeployInfo2[QOSSTRESS_DEPLOYINFO2_SIZE];
    char strEnvironment[QOSSTRESS_ENV_SIZE];
    char strPingSite[QOS_COMMON_MAX_RPC_STRING_SHORT];
    uint32_t uDebugLevel;
    uint32_t uMaxRunCount;
    uint32_t uMaxRunRate;
    uint32_t uMetricsPushingRate;
    ServerMetricsFormatE eMetricsFormat;
    int32_t iHttpSoLinger;
    uint16_t uQosSendPort;
    uint16_t uQosListenPort;
    uint16_t uMetricsAggregatorPort;
    uint8_t bUseSSL;
    uint8_t bIgnoreSSLErrors;
    uint8_t bShutdownDsAfterEachTest;
    uint8_t bDestroyQosClientAfterEachTest;

    // test overrides
    char strOverrideSiteAddr[QOS_COMMON_MAX_URL_LENGTH];        //!< the url for the override qos site
    char strOverrideSiteKey[QOS_COMMON_SECURE_KEY_LENGTH];      //!< the secure key for the override qos site
    char strOverrideSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];  //!< the name of the qos server that we will override (such as "bio-sjc")
    uint16_t uOverrideSitePort;                                 //!< the probe port for the override qos site
    uint16_t uOverrideSiteVersion;                              //!< the version for the override qos site

} QosStressConfigC;

#endif
