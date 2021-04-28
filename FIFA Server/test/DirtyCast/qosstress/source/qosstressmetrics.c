/*H********************************************************************************/
/*!
    \File qosstressmetrics.c

    \Description
        This module implements OIv2 metrics for the qos bot

    \Copyright
        Copyright (c) 2018 Electronic Arts Inc.

    \Version 04/12/2018 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <stdio.h>
#include <string.h>
#include "DirtySDK/platform.h"
#include "dirtycast.h"
#include "servermetrics.h"
#include "qosstressmetrics.h"
#include "qosstressconfig.h"
#include "DirtySDK/dirtysock/dirtymem.h"

/*** Macros ***********************************************************************/

#define QOSSTRESS_METRICS_MAX_HRESULTS   (4)
#define QOSSTRESS_METRICS_MAX_SITES      (100)

// format of the dimensions string, if this value changes to sure to update the size calculation and values where it is used
#define QOSSTRESS_METRICS_COMMON_DIMENSIONS_FORMAT ("mode:qosbot,qosbotid:%s_%d,pool:%s,deployinfo:%s,deployinfo2:%s,site:%s,env:%s,profile:%s")

//! allocation identifier
#define QOS_STRESS_MEMID        ('qstr')

/*** Type Definitions *************************************************************/

typedef struct KeyValuePairT
{
    uint32_t uKey;
    uint32_t uValue;
} KeyValuePairT;

typedef struct HResultCountT
{
    KeyValuePairT aHResults[QOSSTRESS_METRICS_MAX_HRESULTS];
    uint32_t uHResultStoreError;
} HResultCountT;

typedef struct StatsT
{
    uint64_t uSum;
    uint64_t uCount;
    uint32_t uMin;
    uint32_t uMax;
} StatsT;

typedef struct SiteMetricsT
{
    char strSiteName[QOS_COMMON_MAX_RPC_STRING_SHORT];   //!< alias of target QOS server
    StatsT Rtt;
    StatsT BwUp;
    StatsT BwDown;
    HResultCountT SiteHResults;
} SiteMetricsT;

//! gather metrics about client results
typedef struct QosStressMetricsDataT
{
    HResultCountT ModuleHResults;
    SiteMetricsT aSiteMetrics[QOSSTRESS_METRICS_MAX_SITES];
    uint32_t aFirewallTypes[QOS_COMMON_FIREWALL_NUMNATTYPES];
    uint32_t uTimeTillRetryCount;
} QosStressMetricsDataT;

//! manage the metrics system
typedef struct QosStressMetricsRefT
{
    QosStressMetricsDataT MetricsData;
    QosStressConfigC* pConfig;
    ServerMetricsT* pServerMetrics;
    void *pMemGroupUserData;
    int32_t iMemGroup;
} QosStressMetricsRefT;

//! metric handler type
typedef int32_t(QosMetricHandlerT)(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct QosMetricHandlerEntryT
{
    char strMetricName[64];
    QosMetricHandlerT *pMetricHandler;
} QosMetricHandlerEntryT;


/*** Function Prototypes ***************************************************************/

static int32_t _QosStressMetricHResult(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricFirewall(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricRetry(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

// 100 RTT results
static int32_t _QosStressMetricSiteRTT0(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT1(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT2(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT3(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT4(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT5(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT6(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT7(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT8(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT9(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT10(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT11(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT12(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT13(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT14(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT15(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT16(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT17(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT18(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT19(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT20(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT21(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT22(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT23(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT24(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT25(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT26(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT27(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT28(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT29(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT30(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT31(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT32(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT33(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT34(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT35(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT36(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT37(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT38(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT39(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT40(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT41(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT42(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT43(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT44(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT45(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT46(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT47(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT48(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT49(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT50(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT51(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT52(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT53(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT54(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT55(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT56(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT57(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT58(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT59(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT60(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT61(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT62(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT63(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT64(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT65(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT66(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT67(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT68(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT69(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT70(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT71(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT72(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT73(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT74(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT75(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT76(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT77(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT78(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT79(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT80(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT81(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT82(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT83(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT84(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT85(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT86(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT87(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT88(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT89(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT90(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT91(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT92(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT93(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT94(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT95(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT96(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT97(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT98(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteRTT99(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

// 100 BWUP results
static int32_t _QosStressMetricSiteBwUp0(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp1(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp2(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp3(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp4(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp5(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp6(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp7(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp8(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp9(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp10(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp11(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp12(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp13(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp14(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp15(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp16(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp17(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp18(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp19(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp20(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp21(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp22(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp23(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp24(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp25(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp26(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp27(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp28(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp29(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp30(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp31(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp32(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp33(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp34(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp35(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp36(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp37(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp38(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp39(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp40(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp41(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp42(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp43(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp44(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp45(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp46(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp47(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp48(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp49(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp50(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp51(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp52(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp53(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp54(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp55(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp56(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp57(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp58(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp59(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp60(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp61(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp62(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp63(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp64(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp65(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp66(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp67(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp68(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp69(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp70(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp71(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp72(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp73(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp74(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp75(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp76(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp77(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp78(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp79(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp80(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp81(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp82(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp83(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp84(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp85(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp86(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp87(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp88(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp89(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp90(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp91(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp92(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp93(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp94(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp95(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp96(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp97(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp98(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwUp99(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

// 100 BWDOWN results
static int32_t _QosStressMetricSiteBwDown0(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown1(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown2(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown3(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown4(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown5(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown6(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown7(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown8(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown9(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown10(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown11(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown12(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown13(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown14(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown15(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown16(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown17(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown18(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown19(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown20(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown21(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown22(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown23(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown24(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown25(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown26(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown27(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown28(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown29(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown30(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown31(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown32(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown33(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown34(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown35(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown36(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown37(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown38(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown39(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown40(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown41(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown42(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown43(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown44(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown45(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown46(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown47(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown48(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown49(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown50(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown51(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown52(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown53(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown54(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown55(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown56(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown57(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown58(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown59(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown60(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown61(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown62(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown63(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown64(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown65(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown66(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown67(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown68(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown69(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown70(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown71(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown72(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown73(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown74(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown75(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown76(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown77(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown78(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown79(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown80(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown81(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown82(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown83(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown84(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown85(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown86(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown87(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown88(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown89(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown90(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown91(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown92(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown93(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown94(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown95(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown96(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown97(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown98(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteBwDown99(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

// 100 HResults
static int32_t _QosStressMetricSiteHResult0(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult1(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult2(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult3(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult4(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult5(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult6(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult7(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult8(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult9(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult10(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult11(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult12(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult13(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult14(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult15(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult16(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult17(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult18(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult19(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult20(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult21(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult22(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult23(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult24(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult25(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult26(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult27(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult28(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult29(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult30(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult31(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult32(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult33(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult34(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult35(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult36(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult37(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult38(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult39(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult40(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult41(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult42(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult43(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult44(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult45(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult46(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult47(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult48(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult49(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult50(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult51(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult52(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult53(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult54(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult55(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult56(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult57(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult58(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult59(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult60(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult61(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult62(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult63(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult64(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult65(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult66(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult67(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult68(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult69(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult70(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult71(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult72(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult73(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult74(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult75(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult76(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult77(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult78(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult79(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult80(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult81(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult82(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult83(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult84(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult85(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult86(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult87(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult88(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult89(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult90(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult91(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult92(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult93(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult94(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult95(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult96(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult97(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult98(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosStressMetricSiteHResult99(QosStressMetricsRefT *pQosServer, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);


/*** Variables ********************************************************************/

static QosMetricHandlerEntryT _QosMetrics_MetricHandlers[] =
{
    { "invalid",                            NULL },
    { "qosbot.module.hresult",              _QosStressMetricHResult },
    { "qosbot.module.firewall",             _QosStressMetricFirewall },
    { "qosbot.module.retry",                _QosStressMetricRetry },

    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT0 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT1 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT2 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT3 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT4 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT5 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT6 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT7 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT8 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT9 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT10 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT11 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT12 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT13 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT14 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT15 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT16 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT17 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT18 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT19 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT20 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT21 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT22 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT23 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT24 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT25 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT26 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT27 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT28 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT29 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT30 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT31 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT32 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT33 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT34 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT35 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT36 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT37 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT38 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT39 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT40 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT41 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT42 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT43 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT44 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT45 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT46 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT47 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT48 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT49 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT50 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT51 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT52 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT53 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT54 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT55 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT56 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT57 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT58 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT59 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT60 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT61 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT62 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT63 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT64 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT65 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT66 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT67 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT68 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT69 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT70 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT71 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT72 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT73 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT74 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT75 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT76 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT77 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT78 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT79 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT80 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT81 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT82 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT83 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT84 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT85 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT86 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT87 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT88 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT89 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT90 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT91 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT92 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT93 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT94 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT95 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT96 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT97 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT98 },
    { "qosbot.test.rtt",                    _QosStressMetricSiteRTT99 },

    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp0 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp1 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp2 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp3 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp4 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp5 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp6 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp7 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp8 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp9 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp10 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp11 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp12 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp13 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp14 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp15 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp16 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp17 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp18 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp19 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp20 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp21 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp22 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp23 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp24 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp25 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp26 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp27 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp28 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp29 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp30 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp31 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp32 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp33 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp34 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp35 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp36 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp37 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp38 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp39 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp40 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp41 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp42 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp43 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp44 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp45 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp46 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp47 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp48 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp49 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp50 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp51 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp52 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp53 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp54 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp55 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp56 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp57 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp58 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp59 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp60 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp61 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp62 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp63 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp64 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp65 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp66 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp67 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp68 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp69 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp70 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp71 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp72 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp73 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp74 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp75 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp76 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp77 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp78 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp79 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp80 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp81 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp82 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp83 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp84 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp85 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp86 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp87 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp88 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp89 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp90 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp91 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp92 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp93 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp94 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp95 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp96 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp97 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp98 },
    { "qosbot.test.bwup",                   _QosStressMetricSiteBwUp99 },

    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown0 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown1 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown2 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown3 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown4 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown5 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown6 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown7 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown8 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown9 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown10 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown11 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown12 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown13 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown14 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown15 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown16 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown17 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown18 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown19 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown20 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown21 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown22 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown23 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown24 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown25 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown26 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown27 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown28 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown29 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown30 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown31 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown32 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown33 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown34 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown35 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown36 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown37 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown38 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown39 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown40 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown41 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown42 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown43 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown44 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown45 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown46 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown47 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown48 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown49 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown50 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown51 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown52 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown53 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown54 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown55 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown56 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown57 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown58 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown59 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown60 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown61 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown62 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown63 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown64 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown65 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown66 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown67 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown68 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown69 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown70 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown71 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown72 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown73 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown74 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown75 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown76 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown77 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown78 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown79 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown80 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown81 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown82 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown83 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown84 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown85 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown86 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown87 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown88 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown89 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown90 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown91 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown92 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown93 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown94 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown95 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown96 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown97 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown98 },
    { "qosbot.test.bwdown",                 _QosStressMetricSiteBwDown99 },

    { "qosbot.test.hresult",                _QosStressMetricSiteHResult0 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult1 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult2 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult3 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult4 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult5 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult6 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult7 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult8 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult9 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult10 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult11 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult12 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult13 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult14 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult15 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult16 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult17 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult18 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult19 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult20 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult21 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult22 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult23 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult24 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult25 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult26 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult27 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult28 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult29 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult30 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult31 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult32 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult33 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult34 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult35 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult36 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult37 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult38 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult39 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult40 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult41 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult42 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult43 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult44 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult45 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult46 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult47 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult48 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult49 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult50 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult51 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult52 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult53 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult54 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult55 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult56 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult57 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult58 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult59 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult60 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult61 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult62 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult63 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult64 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult65 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult66 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult67 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult68 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult69 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult70 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult71 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult72 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult73 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult74 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult75 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult76 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult77 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult78 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult79 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult80 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult81 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult82 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult83 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult84 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult85 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult86 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult87 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult88 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult89 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult90 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult91 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult92 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult93 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult94 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult95 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult96 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult97 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult98 },
    { "qosbot.test.hresult",                _QosStressMetricSiteHResult99 }
};

static const char *strQosCommonFirewallType[] =              //!< keep in sync with QosCommonFirewallTypeE
{
    "FIREWALL_UNKNOWN  ",
    "FIREWALL_OPEN ",
    "FIREWALL_MODERATE ",
    "FIREWALL_STRICT ",
    "FIREWALL_NUMNATTYPES"
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _QosStressMetricHResult

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/10/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricHResult(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t i;

    for (i = 0; i < QOSSTRESS_METRICS_MAX_HRESULTS; i++)
    {
        if (pQosStressMetrics->MetricsData.ModuleHResults.aHResults[i].uKey != 0)
        {
            ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,modulehresult:0x%08x", pCommonDimensions, pQosStressMetrics->MetricsData.ModuleHResults.aHResults[i].uKey);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosStressMetrics->MetricsData.ModuleHResults.aHResults[i].uValue, SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                iOffset += iResult;
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
                break;
            }
        }
    }

    //add the metric for hresults that have gone past what we store
    if (iErrorCode == 0)
    {
        if (pQosStressMetrics->MetricsData.ModuleHResults.uHResultStoreError != 0)
        {
            ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,modulehresult:ADDITIONAL_HRESULTS", pCommonDimensions);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosStressMetrics->MetricsData.ModuleHResults.uHResultStoreError, SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                iOffset += iResult;
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
            }
        }
    }

    //there was an error clear everything this function attempted to add
    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _QosStressMetricFirewall

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricFirewall(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t i;

    for (i = 0; i < QOS_COMMON_FIREWALL_NUMNATTYPES; i++)
    {
        if (pQosStressMetrics->MetricsData.aFirewallTypes[i] != 0)
        {
            ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,firewall:%s", pCommonDimensions, strQosCommonFirewallType[i]);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosStressMetrics->MetricsData.aFirewallTypes[i], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                iOffset += iResult;
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
                break;
            }
        }
    }

    //there was an error clear everything this function attempted to add
    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _QosStressMetricRetry

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricRetry(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosStressMetrics->MetricsData.uTimeTillRetryCount, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosStressMetricSiteRTTIndex

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input uSiteIndex           - index of site to report

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricSiteRTTIndex(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, uint32_t uSiteIndex)
{
    int32_t iOffset = 0;
    if (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName[0] != '\0')
    {
        char strCustomDimensions[512];
        char strCustomMetricName[128];
        int32_t iErrorCode = 0;
        int32_t iResult = 0;

        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,targetsite:%s", pCommonDimensions, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName);
        ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.min", pMetricName);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].Rtt.uMin, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
            ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.max", pMetricName);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].Rtt.uMax, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                float fAverage = pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].Rtt.uSum / (float)pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].Rtt.uCount;
                iOffset += iResult;
                ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.average", pMetricName);
                if ((iResult = ServerMetricsFormatFloat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, fAverage, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
                {
                    // move forward in the buffer
                    iOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                }
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
            }
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }

        //there was an error clear everything this function attempted to add
        if (iErrorCode != 0)
        {
            iOffset = iErrorCode;

            // reset user buffer
            ds_memclr(pBuffer, iBufSize);
        }
    }
    return(iOffset);
}


/*F********************************************************************************/
/*!
    \Function _QosStressMetricSiteBwUpIndex

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input uSiteIndex           - index of site to report

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricSiteBwUpIndex(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, uint32_t uSiteIndex)
{
    int32_t iOffset = 0;
    if ((pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName[0] != '\0') && (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp.uSum != 0))
    {
        char strCustomDimensions[512];
        char strCustomMetricName[128];
        int32_t iErrorCode = 0;
        int32_t iResult = 0;

        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,targetsite:%s", pCommonDimensions, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName);
        ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.min", pMetricName);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp.uMin, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
            ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.max", pMetricName);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp.uMax, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                uint64_t uAverage = pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp.uSum / pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp.uCount;
                iOffset += iResult;
                ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.average", pMetricName);
                if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, uAverage, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
                {
                    // move forward in the buffer
                    iOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                }
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
            }
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }

        //there was an error clear everything this function attempted to add
        if (iErrorCode != 0)
        {
            iOffset = iErrorCode;

            // reset user buffer
            ds_memclr(pBuffer, iBufSize);
        }
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _QosStressMetricSiteBwUpIndex

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input uSiteIndex           - index of site to report

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricSiteBwDownIndex(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, uint32_t uSiteIndex)
{
    int32_t iOffset = 0;
    if ((pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName[0] != '\0') && (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown.uSum != 0))
    {
        char strCustomDimensions[512];
        char strCustomMetricName[128];
        int32_t iErrorCode = 0;
        int32_t iResult = 0;

        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,targetsite:%s", pCommonDimensions, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName);
        ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.min", pMetricName);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown.uMin, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
            ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.max", pMetricName);
            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown.uMax, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
            {
                // move forward in the buffer
                uint64_t uAverage = pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown.uSum / pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown.uCount;
                iOffset += iResult;
                ds_snzprintf(strCustomMetricName, sizeof(strCustomMetricName), "%s.average", pMetricName);
                if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strCustomMetricName, eFormat, uAverage, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
                {
                    // move forward in the buffer
                    iOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                }
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
            }
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }

        //there was an error clear everything this function attempted to add
        if (iErrorCode != 0)
        {
            iOffset = iErrorCode;

            // reset user buffer
            ds_memclr(pBuffer, iBufSize);
        }
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _QosStressMetricSiteHResultIndex

    \Description
        Adds the metric to the specified buffer.

    \Input *pQosStressMetrics   - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input uSiteIndex           - index of site to report


    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/10/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricSiteHResultIndex(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, uint32_t uSiteIndex)
{
    int32_t iOffset = 0;

    if (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName[0] != '\0')
    {
        char strCustomDimensions[512];
        int32_t iErrorCode = 0;
        int32_t iResult = 0;
        int32_t i;

        for (i = 0; i < QOSSTRESS_METRICS_MAX_HRESULTS; i++)
        {
            if (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults.aHResults[i].uKey != 0)
            {
                ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,sitehresult:0x%08x,targetsite:%s", pCommonDimensions, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults.aHResults[i].uKey, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName);
                if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults.aHResults[i].uValue, SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
                {
                    // move forward in the buffer
                    iOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                    break;
                }
            }
        }

        //add the metric for hresults that have gone past what we store
        if (iErrorCode == 0)
        {
            if (pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults.uHResultStoreError != 0)
            {
                ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,sitehresult:ADDITIONAL_HRESULTS,targetsite:%s", pCommonDimensions, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName);
                if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosStressMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults.uHResultStoreError, SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
                {
                    // move forward in the buffer
                    iOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                }
            }
        }

        //there was an error clear everything this function attempted to add
        if (iErrorCode != 0)
        {
            iOffset = iErrorCode;

            // reset user buffer
            ds_memclr(pBuffer, iBufSize);
        }
    }
    return(iOffset);
}

//rtt index handlers
static int32_t _QosStressMetricSiteRTT0(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0);}
static int32_t _QosStressMetricSiteRTT1(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 1);}
static int32_t _QosStressMetricSiteRTT2(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 2);}
static int32_t _QosStressMetricSiteRTT3(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 3);}
static int32_t _QosStressMetricSiteRTT4(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 4);}
static int32_t _QosStressMetricSiteRTT5(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 5);}
static int32_t _QosStressMetricSiteRTT6(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 6);}
static int32_t _QosStressMetricSiteRTT7(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 7);}
static int32_t _QosStressMetricSiteRTT8(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 8);}
static int32_t _QosStressMetricSiteRTT9(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 9);}
static int32_t _QosStressMetricSiteRTT10(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 10);}
static int32_t _QosStressMetricSiteRTT11(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 11);}
static int32_t _QosStressMetricSiteRTT12(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 12);}
static int32_t _QosStressMetricSiteRTT13(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 13);}
static int32_t _QosStressMetricSiteRTT14(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 14);}
static int32_t _QosStressMetricSiteRTT15(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 15);}
static int32_t _QosStressMetricSiteRTT16(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 16);}
static int32_t _QosStressMetricSiteRTT17(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 17);}
static int32_t _QosStressMetricSiteRTT18(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 18);}
static int32_t _QosStressMetricSiteRTT19(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 19);}
static int32_t _QosStressMetricSiteRTT20(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 20);}
static int32_t _QosStressMetricSiteRTT21(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 21);}
static int32_t _QosStressMetricSiteRTT22(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 22);}
static int32_t _QosStressMetricSiteRTT23(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 23);}
static int32_t _QosStressMetricSiteRTT24(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 24);}
static int32_t _QosStressMetricSiteRTT25(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 25);}
static int32_t _QosStressMetricSiteRTT26(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 26);}
static int32_t _QosStressMetricSiteRTT27(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 27);}
static int32_t _QosStressMetricSiteRTT28(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 28);}
static int32_t _QosStressMetricSiteRTT29(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 29);}
static int32_t _QosStressMetricSiteRTT30(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 30);}
static int32_t _QosStressMetricSiteRTT31(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 31);}
static int32_t _QosStressMetricSiteRTT32(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 32);}
static int32_t _QosStressMetricSiteRTT33(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 33);}
static int32_t _QosStressMetricSiteRTT34(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 34);}
static int32_t _QosStressMetricSiteRTT35(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 35);}
static int32_t _QosStressMetricSiteRTT36(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 36);}
static int32_t _QosStressMetricSiteRTT37(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 37);}
static int32_t _QosStressMetricSiteRTT38(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 38);}
static int32_t _QosStressMetricSiteRTT39(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 39);}
static int32_t _QosStressMetricSiteRTT40(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 40);}
static int32_t _QosStressMetricSiteRTT41(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 41);}
static int32_t _QosStressMetricSiteRTT42(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 42);}
static int32_t _QosStressMetricSiteRTT43(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 43);}
static int32_t _QosStressMetricSiteRTT44(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 44);}
static int32_t _QosStressMetricSiteRTT45(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 45);}
static int32_t _QosStressMetricSiteRTT46(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 46);}
static int32_t _QosStressMetricSiteRTT47(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 47);}
static int32_t _QosStressMetricSiteRTT48(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 48);}
static int32_t _QosStressMetricSiteRTT49(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 49);}
static int32_t _QosStressMetricSiteRTT50(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 50);}
static int32_t _QosStressMetricSiteRTT51(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 51);}
static int32_t _QosStressMetricSiteRTT52(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 52);}
static int32_t _QosStressMetricSiteRTT53(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 53);}
static int32_t _QosStressMetricSiteRTT54(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 54);}
static int32_t _QosStressMetricSiteRTT55(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 55);}
static int32_t _QosStressMetricSiteRTT56(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 56);}
static int32_t _QosStressMetricSiteRTT57(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 57);}
static int32_t _QosStressMetricSiteRTT58(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 58);}
static int32_t _QosStressMetricSiteRTT59(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 59);}
static int32_t _QosStressMetricSiteRTT60(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 60);}
static int32_t _QosStressMetricSiteRTT61(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 61);}
static int32_t _QosStressMetricSiteRTT62(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 62);}
static int32_t _QosStressMetricSiteRTT63(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 63);}
static int32_t _QosStressMetricSiteRTT64(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 64);}
static int32_t _QosStressMetricSiteRTT65(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 65);}
static int32_t _QosStressMetricSiteRTT66(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 66);}
static int32_t _QosStressMetricSiteRTT67(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 67);}
static int32_t _QosStressMetricSiteRTT68(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 68);}
static int32_t _QosStressMetricSiteRTT69(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 69);}
static int32_t _QosStressMetricSiteRTT70(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 70);}
static int32_t _QosStressMetricSiteRTT71(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 71);}
static int32_t _QosStressMetricSiteRTT72(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 72);}
static int32_t _QosStressMetricSiteRTT73(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 73);}
static int32_t _QosStressMetricSiteRTT74(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 74);}
static int32_t _QosStressMetricSiteRTT75(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 75);}
static int32_t _QosStressMetricSiteRTT76(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 76);}
static int32_t _QosStressMetricSiteRTT77(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 77);}
static int32_t _QosStressMetricSiteRTT78(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 78);}
static int32_t _QosStressMetricSiteRTT79(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 79);}
static int32_t _QosStressMetricSiteRTT80(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 80);}
static int32_t _QosStressMetricSiteRTT81(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 81);}
static int32_t _QosStressMetricSiteRTT82(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 82);}
static int32_t _QosStressMetricSiteRTT83(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 83);}
static int32_t _QosStressMetricSiteRTT84(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 84);}
static int32_t _QosStressMetricSiteRTT85(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 85);}
static int32_t _QosStressMetricSiteRTT86(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 86);}
static int32_t _QosStressMetricSiteRTT87(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 87);}
static int32_t _QosStressMetricSiteRTT88(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 88);}
static int32_t _QosStressMetricSiteRTT89(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 89);}
static int32_t _QosStressMetricSiteRTT90(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 90);}
static int32_t _QosStressMetricSiteRTT91(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 91);}
static int32_t _QosStressMetricSiteRTT92(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 92);}
static int32_t _QosStressMetricSiteRTT93(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 93);}
static int32_t _QosStressMetricSiteRTT94(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 94);}
static int32_t _QosStressMetricSiteRTT95(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 95);}
static int32_t _QosStressMetricSiteRTT96(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 96);}
static int32_t _QosStressMetricSiteRTT97(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 97);}
static int32_t _QosStressMetricSiteRTT98(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 98);}
static int32_t _QosStressMetricSiteRTT99(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteRTTIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 99);}

//bwup index handlers
static int32_t _QosStressMetricSiteBwUp0(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0);}
static int32_t _QosStressMetricSiteBwUp1(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 1);}
static int32_t _QosStressMetricSiteBwUp2(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 2);}
static int32_t _QosStressMetricSiteBwUp3(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 3);}
static int32_t _QosStressMetricSiteBwUp4(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 4);}
static int32_t _QosStressMetricSiteBwUp5(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 5);}
static int32_t _QosStressMetricSiteBwUp6(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 6);}
static int32_t _QosStressMetricSiteBwUp7(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 7);}
static int32_t _QosStressMetricSiteBwUp8(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 8);}
static int32_t _QosStressMetricSiteBwUp9(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 9);}
static int32_t _QosStressMetricSiteBwUp10(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 10);}
static int32_t _QosStressMetricSiteBwUp11(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 11);}
static int32_t _QosStressMetricSiteBwUp12(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 12);}
static int32_t _QosStressMetricSiteBwUp13(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 13);}
static int32_t _QosStressMetricSiteBwUp14(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 14);}
static int32_t _QosStressMetricSiteBwUp15(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 15);}
static int32_t _QosStressMetricSiteBwUp16(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 16);}
static int32_t _QosStressMetricSiteBwUp17(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 17);}
static int32_t _QosStressMetricSiteBwUp18(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 18);}
static int32_t _QosStressMetricSiteBwUp19(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 19);}
static int32_t _QosStressMetricSiteBwUp20(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 20);}
static int32_t _QosStressMetricSiteBwUp21(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 21);}
static int32_t _QosStressMetricSiteBwUp22(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 22);}
static int32_t _QosStressMetricSiteBwUp23(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 23);}
static int32_t _QosStressMetricSiteBwUp24(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 24);}
static int32_t _QosStressMetricSiteBwUp25(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 25);}
static int32_t _QosStressMetricSiteBwUp26(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 26);}
static int32_t _QosStressMetricSiteBwUp27(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 27);}
static int32_t _QosStressMetricSiteBwUp28(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 28);}
static int32_t _QosStressMetricSiteBwUp29(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 29);}
static int32_t _QosStressMetricSiteBwUp30(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 30);}
static int32_t _QosStressMetricSiteBwUp31(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 31);}
static int32_t _QosStressMetricSiteBwUp32(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 32);}
static int32_t _QosStressMetricSiteBwUp33(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 33);}
static int32_t _QosStressMetricSiteBwUp34(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 34);}
static int32_t _QosStressMetricSiteBwUp35(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 35);}
static int32_t _QosStressMetricSiteBwUp36(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 36);}
static int32_t _QosStressMetricSiteBwUp37(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 37);}
static int32_t _QosStressMetricSiteBwUp38(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 38);}
static int32_t _QosStressMetricSiteBwUp39(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 39);}
static int32_t _QosStressMetricSiteBwUp40(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 40);}
static int32_t _QosStressMetricSiteBwUp41(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 41);}
static int32_t _QosStressMetricSiteBwUp42(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 42);}
static int32_t _QosStressMetricSiteBwUp43(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 43);}
static int32_t _QosStressMetricSiteBwUp44(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 44);}
static int32_t _QosStressMetricSiteBwUp45(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 45);}
static int32_t _QosStressMetricSiteBwUp46(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 46);}
static int32_t _QosStressMetricSiteBwUp47(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 47);}
static int32_t _QosStressMetricSiteBwUp48(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 48);}
static int32_t _QosStressMetricSiteBwUp49(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 49);}
static int32_t _QosStressMetricSiteBwUp50(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 50);}
static int32_t _QosStressMetricSiteBwUp51(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 51);}
static int32_t _QosStressMetricSiteBwUp52(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 52);}
static int32_t _QosStressMetricSiteBwUp53(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 53);}
static int32_t _QosStressMetricSiteBwUp54(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 54);}
static int32_t _QosStressMetricSiteBwUp55(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 55);}
static int32_t _QosStressMetricSiteBwUp56(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 56);}
static int32_t _QosStressMetricSiteBwUp57(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 57);}
static int32_t _QosStressMetricSiteBwUp58(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 58);}
static int32_t _QosStressMetricSiteBwUp59(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 59);}
static int32_t _QosStressMetricSiteBwUp60(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 60);}
static int32_t _QosStressMetricSiteBwUp61(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 61);}
static int32_t _QosStressMetricSiteBwUp62(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 62);}
static int32_t _QosStressMetricSiteBwUp63(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 63);}
static int32_t _QosStressMetricSiteBwUp64(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 64);}
static int32_t _QosStressMetricSiteBwUp65(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 65);}
static int32_t _QosStressMetricSiteBwUp66(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 66);}
static int32_t _QosStressMetricSiteBwUp67(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 67);}
static int32_t _QosStressMetricSiteBwUp68(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 68);}
static int32_t _QosStressMetricSiteBwUp69(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 69);}
static int32_t _QosStressMetricSiteBwUp70(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 70);}
static int32_t _QosStressMetricSiteBwUp71(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 71);}
static int32_t _QosStressMetricSiteBwUp72(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 72);}
static int32_t _QosStressMetricSiteBwUp73(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 73);}
static int32_t _QosStressMetricSiteBwUp74(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 74);}
static int32_t _QosStressMetricSiteBwUp75(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 75);}
static int32_t _QosStressMetricSiteBwUp76(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 76);}
static int32_t _QosStressMetricSiteBwUp77(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 77);}
static int32_t _QosStressMetricSiteBwUp78(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 78);}
static int32_t _QosStressMetricSiteBwUp79(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 79);}
static int32_t _QosStressMetricSiteBwUp80(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 80);}
static int32_t _QosStressMetricSiteBwUp81(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 81);}
static int32_t _QosStressMetricSiteBwUp82(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 82);}
static int32_t _QosStressMetricSiteBwUp83(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 83);}
static int32_t _QosStressMetricSiteBwUp84(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 84);}
static int32_t _QosStressMetricSiteBwUp85(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 85);}
static int32_t _QosStressMetricSiteBwUp86(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 86);}
static int32_t _QosStressMetricSiteBwUp87(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 87);}
static int32_t _QosStressMetricSiteBwUp88(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 88);}
static int32_t _QosStressMetricSiteBwUp89(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 89);}
static int32_t _QosStressMetricSiteBwUp90(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 90);}
static int32_t _QosStressMetricSiteBwUp91(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 91);}
static int32_t _QosStressMetricSiteBwUp92(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 92);}
static int32_t _QosStressMetricSiteBwUp93(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 93);}
static int32_t _QosStressMetricSiteBwUp94(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 94);}
static int32_t _QosStressMetricSiteBwUp95(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 95);}
static int32_t _QosStressMetricSiteBwUp96(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 96);}
static int32_t _QosStressMetricSiteBwUp97(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 97);}
static int32_t _QosStressMetricSiteBwUp98(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 98);}
static int32_t _QosStressMetricSiteBwUp99(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwUpIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 99);}


//bwDown index handlers
static int32_t _QosStressMetricSiteBwDown0(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0);}
static int32_t _QosStressMetricSiteBwDown1(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 1);}
static int32_t _QosStressMetricSiteBwDown2(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 2);}
static int32_t _QosStressMetricSiteBwDown3(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 3);}
static int32_t _QosStressMetricSiteBwDown4(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 4);}
static int32_t _QosStressMetricSiteBwDown5(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 5);}
static int32_t _QosStressMetricSiteBwDown6(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 6);}
static int32_t _QosStressMetricSiteBwDown7(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 7);}
static int32_t _QosStressMetricSiteBwDown8(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 8);}
static int32_t _QosStressMetricSiteBwDown9(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 9);}
static int32_t _QosStressMetricSiteBwDown10(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 10);}
static int32_t _QosStressMetricSiteBwDown11(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 11);}
static int32_t _QosStressMetricSiteBwDown12(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 12);}
static int32_t _QosStressMetricSiteBwDown13(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 13);}
static int32_t _QosStressMetricSiteBwDown14(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 14);}
static int32_t _QosStressMetricSiteBwDown15(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 15);}
static int32_t _QosStressMetricSiteBwDown16(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 16);}
static int32_t _QosStressMetricSiteBwDown17(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 17);}
static int32_t _QosStressMetricSiteBwDown18(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 18);}
static int32_t _QosStressMetricSiteBwDown19(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 19);}
static int32_t _QosStressMetricSiteBwDown20(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 20);}
static int32_t _QosStressMetricSiteBwDown21(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 21);}
static int32_t _QosStressMetricSiteBwDown22(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 22);}
static int32_t _QosStressMetricSiteBwDown23(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 23);}
static int32_t _QosStressMetricSiteBwDown24(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 24);}
static int32_t _QosStressMetricSiteBwDown25(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 25);}
static int32_t _QosStressMetricSiteBwDown26(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 26);}
static int32_t _QosStressMetricSiteBwDown27(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 27);}
static int32_t _QosStressMetricSiteBwDown28(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 28);}
static int32_t _QosStressMetricSiteBwDown29(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 29);}
static int32_t _QosStressMetricSiteBwDown30(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 30);}
static int32_t _QosStressMetricSiteBwDown31(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 31);}
static int32_t _QosStressMetricSiteBwDown32(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 32);}
static int32_t _QosStressMetricSiteBwDown33(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 33);}
static int32_t _QosStressMetricSiteBwDown34(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 34);}
static int32_t _QosStressMetricSiteBwDown35(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 35);}
static int32_t _QosStressMetricSiteBwDown36(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 36);}
static int32_t _QosStressMetricSiteBwDown37(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 37);}
static int32_t _QosStressMetricSiteBwDown38(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 38);}
static int32_t _QosStressMetricSiteBwDown39(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 39);}
static int32_t _QosStressMetricSiteBwDown40(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 40);}
static int32_t _QosStressMetricSiteBwDown41(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 41);}
static int32_t _QosStressMetricSiteBwDown42(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 42);}
static int32_t _QosStressMetricSiteBwDown43(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 43);}
static int32_t _QosStressMetricSiteBwDown44(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 44);}
static int32_t _QosStressMetricSiteBwDown45(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 45);}
static int32_t _QosStressMetricSiteBwDown46(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 46);}
static int32_t _QosStressMetricSiteBwDown47(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 47);}
static int32_t _QosStressMetricSiteBwDown48(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 48);}
static int32_t _QosStressMetricSiteBwDown49(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 49);}
static int32_t _QosStressMetricSiteBwDown50(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 50);}
static int32_t _QosStressMetricSiteBwDown51(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 51);}
static int32_t _QosStressMetricSiteBwDown52(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 52);}
static int32_t _QosStressMetricSiteBwDown53(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 53);}
static int32_t _QosStressMetricSiteBwDown54(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 54);}
static int32_t _QosStressMetricSiteBwDown55(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 55);}
static int32_t _QosStressMetricSiteBwDown56(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 56);}
static int32_t _QosStressMetricSiteBwDown57(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 57);}
static int32_t _QosStressMetricSiteBwDown58(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 58);}
static int32_t _QosStressMetricSiteBwDown59(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 59);}
static int32_t _QosStressMetricSiteBwDown60(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 60);}
static int32_t _QosStressMetricSiteBwDown61(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 61);}
static int32_t _QosStressMetricSiteBwDown62(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 62);}
static int32_t _QosStressMetricSiteBwDown63(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 63);}
static int32_t _QosStressMetricSiteBwDown64(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 64);}
static int32_t _QosStressMetricSiteBwDown65(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 65);}
static int32_t _QosStressMetricSiteBwDown66(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 66);}
static int32_t _QosStressMetricSiteBwDown67(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 67);}
static int32_t _QosStressMetricSiteBwDown68(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 68);}
static int32_t _QosStressMetricSiteBwDown69(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 69);}
static int32_t _QosStressMetricSiteBwDown70(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 70);}
static int32_t _QosStressMetricSiteBwDown71(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 71);}
static int32_t _QosStressMetricSiteBwDown72(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 72);}
static int32_t _QosStressMetricSiteBwDown73(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 73);}
static int32_t _QosStressMetricSiteBwDown74(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 74);}
static int32_t _QosStressMetricSiteBwDown75(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 75);}
static int32_t _QosStressMetricSiteBwDown76(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 76);}
static int32_t _QosStressMetricSiteBwDown77(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 77);}
static int32_t _QosStressMetricSiteBwDown78(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 78);}
static int32_t _QosStressMetricSiteBwDown79(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 79);}
static int32_t _QosStressMetricSiteBwDown80(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 80);}
static int32_t _QosStressMetricSiteBwDown81(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 81);}
static int32_t _QosStressMetricSiteBwDown82(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 82);}
static int32_t _QosStressMetricSiteBwDown83(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 83);}
static int32_t _QosStressMetricSiteBwDown84(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 84);}
static int32_t _QosStressMetricSiteBwDown85(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 85);}
static int32_t _QosStressMetricSiteBwDown86(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 86);}
static int32_t _QosStressMetricSiteBwDown87(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 87);}
static int32_t _QosStressMetricSiteBwDown88(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 88);}
static int32_t _QosStressMetricSiteBwDown89(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 89);}
static int32_t _QosStressMetricSiteBwDown90(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 90);}
static int32_t _QosStressMetricSiteBwDown91(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 91);}
static int32_t _QosStressMetricSiteBwDown92(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 92);}
static int32_t _QosStressMetricSiteBwDown93(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 93);}
static int32_t _QosStressMetricSiteBwDown94(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 94);}
static int32_t _QosStressMetricSiteBwDown95(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 95);}
static int32_t _QosStressMetricSiteBwDown96(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 96);}
static int32_t _QosStressMetricSiteBwDown97(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 97);}
static int32_t _QosStressMetricSiteBwDown98(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 98);}
static int32_t _QosStressMetricSiteBwDown99(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteBwDownIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 99);}

//hresult index handlers
static int32_t _QosStressMetricSiteHResult0(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0);}
static int32_t _QosStressMetricSiteHResult1(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 1);}
static int32_t _QosStressMetricSiteHResult2(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 2);}
static int32_t _QosStressMetricSiteHResult3(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 3);}
static int32_t _QosStressMetricSiteHResult4(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 4);}
static int32_t _QosStressMetricSiteHResult5(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 5);}
static int32_t _QosStressMetricSiteHResult6(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 6);}
static int32_t _QosStressMetricSiteHResult7(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 7);}
static int32_t _QosStressMetricSiteHResult8(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 8);}
static int32_t _QosStressMetricSiteHResult9(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 9);}
static int32_t _QosStressMetricSiteHResult10(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 10);}
static int32_t _QosStressMetricSiteHResult11(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 11);}
static int32_t _QosStressMetricSiteHResult12(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 12);}
static int32_t _QosStressMetricSiteHResult13(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 13);}
static int32_t _QosStressMetricSiteHResult14(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 14);}
static int32_t _QosStressMetricSiteHResult15(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 15);}
static int32_t _QosStressMetricSiteHResult16(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 16);}
static int32_t _QosStressMetricSiteHResult17(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 17);}
static int32_t _QosStressMetricSiteHResult18(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 18);}
static int32_t _QosStressMetricSiteHResult19(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 19);}
static int32_t _QosStressMetricSiteHResult20(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 20);}
static int32_t _QosStressMetricSiteHResult21(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 21);}
static int32_t _QosStressMetricSiteHResult22(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 22);}
static int32_t _QosStressMetricSiteHResult23(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 23);}
static int32_t _QosStressMetricSiteHResult24(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 24);}
static int32_t _QosStressMetricSiteHResult25(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 25);}
static int32_t _QosStressMetricSiteHResult26(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 26);}
static int32_t _QosStressMetricSiteHResult27(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 27);}
static int32_t _QosStressMetricSiteHResult28(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 28);}
static int32_t _QosStressMetricSiteHResult29(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 29);}
static int32_t _QosStressMetricSiteHResult30(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 30);}
static int32_t _QosStressMetricSiteHResult31(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 31);}
static int32_t _QosStressMetricSiteHResult32(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 32);}
static int32_t _QosStressMetricSiteHResult33(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 33);}
static int32_t _QosStressMetricSiteHResult34(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 34);}
static int32_t _QosStressMetricSiteHResult35(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 35);}
static int32_t _QosStressMetricSiteHResult36(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 36);}
static int32_t _QosStressMetricSiteHResult37(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 37);}
static int32_t _QosStressMetricSiteHResult38(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 38);}
static int32_t _QosStressMetricSiteHResult39(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 39);}
static int32_t _QosStressMetricSiteHResult40(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 40);}
static int32_t _QosStressMetricSiteHResult41(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 41);}
static int32_t _QosStressMetricSiteHResult42(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 42);}
static int32_t _QosStressMetricSiteHResult43(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 43);}
static int32_t _QosStressMetricSiteHResult44(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 44);}
static int32_t _QosStressMetricSiteHResult45(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 45);}
static int32_t _QosStressMetricSiteHResult46(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 46);}
static int32_t _QosStressMetricSiteHResult47(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 47);}
static int32_t _QosStressMetricSiteHResult48(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 48);}
static int32_t _QosStressMetricSiteHResult49(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 49);}
static int32_t _QosStressMetricSiteHResult50(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 50);}
static int32_t _QosStressMetricSiteHResult51(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 51);}
static int32_t _QosStressMetricSiteHResult52(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 52);}
static int32_t _QosStressMetricSiteHResult53(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 53);}
static int32_t _QosStressMetricSiteHResult54(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 54);}
static int32_t _QosStressMetricSiteHResult55(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 55);}
static int32_t _QosStressMetricSiteHResult56(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 56);}
static int32_t _QosStressMetricSiteHResult57(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 57);}
static int32_t _QosStressMetricSiteHResult58(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 58);}
static int32_t _QosStressMetricSiteHResult59(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 59);}
static int32_t _QosStressMetricSiteHResult60(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 60);}
static int32_t _QosStressMetricSiteHResult61(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 61);}
static int32_t _QosStressMetricSiteHResult62(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 62);}
static int32_t _QosStressMetricSiteHResult63(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 63);}
static int32_t _QosStressMetricSiteHResult64(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 64);}
static int32_t _QosStressMetricSiteHResult65(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 65);}
static int32_t _QosStressMetricSiteHResult66(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 66);}
static int32_t _QosStressMetricSiteHResult67(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 67);}
static int32_t _QosStressMetricSiteHResult68(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 68);}
static int32_t _QosStressMetricSiteHResult69(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 69);}
static int32_t _QosStressMetricSiteHResult70(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 70);}
static int32_t _QosStressMetricSiteHResult71(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 71);}
static int32_t _QosStressMetricSiteHResult72(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 72);}
static int32_t _QosStressMetricSiteHResult73(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 73);}
static int32_t _QosStressMetricSiteHResult74(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 74);}
static int32_t _QosStressMetricSiteHResult75(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 75);}
static int32_t _QosStressMetricSiteHResult76(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 76);}
static int32_t _QosStressMetricSiteHResult77(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 77);}
static int32_t _QosStressMetricSiteHResult78(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 78);}
static int32_t _QosStressMetricSiteHResult79(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 79);}
static int32_t _QosStressMetricSiteHResult80(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 80);}
static int32_t _QosStressMetricSiteHResult81(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 81);}
static int32_t _QosStressMetricSiteHResult82(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 82);}
static int32_t _QosStressMetricSiteHResult83(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 83);}
static int32_t _QosStressMetricSiteHResult84(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 84);}
static int32_t _QosStressMetricSiteHResult85(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 85);}
static int32_t _QosStressMetricSiteHResult86(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 86);}
static int32_t _QosStressMetricSiteHResult87(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 87);}
static int32_t _QosStressMetricSiteHResult88(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 88);}
static int32_t _QosStressMetricSiteHResult89(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 89);}
static int32_t _QosStressMetricSiteHResult90(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 90);}
static int32_t _QosStressMetricSiteHResult91(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 91);}
static int32_t _QosStressMetricSiteHResult92(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 92);}
static int32_t _QosStressMetricSiteHResult93(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 93);}
static int32_t _QosStressMetricSiteHResult94(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 94);}
static int32_t _QosStressMetricSiteHResult95(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 95);}
static int32_t _QosStressMetricSiteHResult96(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 96);}
static int32_t _QosStressMetricSiteHResult97(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 97);}
static int32_t _QosStressMetricSiteHResult98(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 98);}
static int32_t _QosStressMetricSiteHResult99(QosStressMetricsRefT *pQosStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{return _QosStressMetricSiteHResultIndex(pQosStressMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 99);}


static void _AddHresult(HResultCountT* pHresultStorage, uint32_t hResult)
{
    int32_t i;
    
    //look for an empty slot or matching key
    for (i = 0; i < QOSSTRESS_METRICS_MAX_HRESULTS; i++)
    {
        if ((pHresultStorage->aHResults[i].uKey == 0) || (pHresultStorage->aHResults[i].uKey == hResult))
        {
            pHresultStorage->aHResults[i].uKey = hResult;
            pHresultStorage->aHResults[i].uValue++;
            return;
        }
    }
    pHresultStorage->uHResultStoreError++;
}

static void _AddStats(StatsT* pStatStorage, uint32_t uValue)
{
    pStatStorage->uCount++;
    pStatStorage->uSum += uValue;
    if (uValue > pStatStorage->uMax)
    {
        pStatStorage->uMax = uValue;
    }
    if (uValue < pStatStorage->uMin)
    {
        pStatStorage->uMin = uValue;
    }
}

static void _AddTestResults(QosStressMetricsRefT* pMetrics, QosCommonProcessedResultsT* pResults)
{
    uint32_t uResultIndex, uSiteIndex;

    _AddHresult(&pMetrics->MetricsData.ModuleHResults, pResults->hResult);

    pMetrics->MetricsData.aFirewallTypes[pResults->eFirewallType]++;

    if (pResults->uTimeTillRetry != 0)
    {
        pMetrics->MetricsData.uTimeTillRetryCount++;
    }  
    
    for (uResultIndex = 0; uResultIndex < pResults->uNumResults; uResultIndex++)
    {
        if (pResults->aTestResults[uResultIndex].hResult != 0)
        {
            for (uSiteIndex = 0; uSiteIndex < QOSSTRESS_METRICS_MAX_SITES; uSiteIndex++)
            {
                //find the site or an empty slot
                if ((pMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName[0] == '\0') ||
                    (strncmp(pResults->aTestResults[uResultIndex].strSiteName, pMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName, QOS_COMMON_MAX_RPC_STRING_SHORT) == 0))
                {
                    //store stats about the results
                    strcpy(pMetrics->MetricsData.aSiteMetrics[uSiteIndex].strSiteName, pResults->aTestResults[uResultIndex].strSiteName);
                    _AddHresult(&pMetrics->MetricsData.aSiteMetrics[uSiteIndex].SiteHResults, pResults->aTestResults[uResultIndex].hResult);
                    _AddStats(&pMetrics->MetricsData.aSiteMetrics[uSiteIndex].Rtt, pResults->aTestResults[uResultIndex].uMinRTT);
                    _AddStats(&pMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwDown, pResults->aTestResults[uResultIndex].uDownbps);
                    _AddStats(&pMetrics->MetricsData.aSiteMetrics[uSiteIndex].BwUp, pResults->aTestResults[uResultIndex].uUpbps);
                    break;
                }
            }
        }
    }
}

static void _ResetMetrics(QosStressMetricsDataT* pQosStressMetricsData)
{
    uint32_t uSiteIndex;

    //clear everything
    memset(pQosStressMetricsData, 0, sizeof(QosStressMetricsDataT));

    //set all min values to something high
    for (uSiteIndex = 0; uSiteIndex < QOSSTRESS_METRICS_MAX_SITES; uSiteIndex++)
    {
        pQosStressMetricsData->aSiteMetrics[uSiteIndex].BwDown.uMin = UINT32_MAX;
        pQosStressMetricsData->aSiteMetrics[uSiteIndex].BwUp.uMin = UINT32_MAX;
        pQosStressMetricsData->aSiteMetrics[uSiteIndex].Rtt.uMin = UINT32_MAX;
    }
}

/*F********************************************************************************/
/*!
    \Function QosStressMetricsCallback

    \Description
        Callback to format voip server stats into a datagram to be sent to an
        external metrics aggregation

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN]VoipServer module state

    \Output
        int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosStressMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    QosStressMetricsRefT* pQosStressMetrics = (QosStressMetricsRefT *)pUserData;
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strHostName[128];
    //QosServerMetricsT QosServerMetrics;
    char strCommonDimensions[sizeof(QOSSTRESS_METRICS_COMMON_DIMENSIONS_FORMAT) +  // size of the format string
        sizeof(strHostName) + 5 +              // size of the dirtycastid value
        QOS_COMMON_MAX_RPC_STRING +            // size of the pool value
        QOSSTRESS_DEPLOYINFO_SIZE +            // size of deployinfo value
        QOSSTRESS_DEPLOYINFO2_SIZE +           // size of deployinfo2 value
        QOS_COMMON_MAX_RPC_STRING_SHORT +      // size of site value
        QOSSTRESS_ENV_SIZE +                   // size of env value
        QOS_COMMON_MAX_RPC_STRING +            // size of profile value
        1];                                    // NULL

                                               // prepare dimensions to be associated with all metrics
    ds_memclr(strCommonDimensions, sizeof(strCommonDimensions));
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), QOSSTRESS_METRICS_COMMON_DIMENSIONS_FORMAT,
        pQosStressMetrics->pConfig->strLocalHostName,
        pQosStressMetrics->pConfig->uQosListenPort,
        pQosStressMetrics->pConfig->strPool,
        pQosStressMetrics->pConfig->strDeployInfo,
        pQosStressMetrics->pConfig->strDeployInfo2,
        pQosStressMetrics->pConfig->strPingSite,
        pQosStressMetrics->pConfig->strEnvironment,
        pQosStressMetrics->pConfig->strQosProfile);

    //get the delta stats from QOS module
    //QosServerStatus(pQosServer, 'mdlt', 0, &QosServerMetrics, sizeof(QosServerMetrics));

    // for now QosStressMetrics doesn't make use of any common metrics, so we just loop through our own
    while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_QosMetrics_MetricHandlers))
    {
        int32_t iResult;
        QosMetricHandlerEntryT *pEntry = &_QosMetrics_MetricHandlers[iMetricIndex];

        if ((iResult = pEntry->pMetricHandler(pQosStressMetrics, pEntry->strMetricName, pQosStressMetrics->pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
        {
            //DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "qosmetrics: adding QOS metric %s (#%d) to report succeeded\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
            iOffset += iResult;
        }
        else if (iResult == 0)
        {
            //DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "qosmetrics: adding QOS metric %s (#%d) to report skipped because metric unavailable\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
        }
        else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
        {
            //DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "qosmetrics: adding QOS metric %s (#%d) to report failed because it did not fit - back buffer expansion required\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);

            // return "scaled" metric index
            iRetCode = iMetricIndex;
            break;
        }
        else
        {
            //DirtyCastLogPrintf("qosmetrics: adding QOS metric %s (#%d) to report failed with error %d - metric skipped!\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
            iMetricIndex++;
        }
    }

    // let the caller know how many bytes were written to the buffer
    *pSize = iOffset;

    // if all metrics have been dealt with, then let the call know
    if (iMetricIndex == (sizeof(_QosMetrics_MetricHandlers) / sizeof(_QosMetrics_MetricHandlers[0])))
    {
        iRetCode = 0;
        _ResetMetrics(&pQosStressMetrics->MetricsData);
    }

    return(iRetCode);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function QosStressMetricsCreate

    \Description
        Create the QosStressMetricsRefT module.

    \Input *mpConfig   - the config used the the qos bot

    \Output
    QosStressMetricsRefT * - state pointer on success, NULL on failure

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
QosStressMetricsRefT *QosStressMetricsCreate(QosStressConfigC* mpConfig)
{
    QosStressMetricsRefT* pQosStressMetrics;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemGroupEnter(iMemGroup, pMemGroupUserData);

    // allocate and init module state
    if ((pQosStressMetrics = (QosStressMetricsRefT*)DirtyMemAlloc(sizeof(*pQosStressMetrics), QOS_STRESS_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemGroupLeave();
        DirtyCastLogPrintf(("qosstressmetrics: could not allocate module state.\n"));
        return(NULL);
    }

    // create module to report metrics to an external metrics aggregator
    if ((pQosStressMetrics->pServerMetrics = ServerMetricsCreate(mpConfig->strMetricsAggregatorHost, mpConfig->uMetricsAggregatorPort, mpConfig->uMetricsPushingRate, mpConfig->eMetricsFormat, &_QosStressMetricsCallback, pQosStressMetrics)) != NULL)
    {
        DirtyCastLogPrintf("voipserver: created module for reporting metrics to external aggregator (report rate = %d ms)\n", mpConfig->uMetricsPushingRate);
        ServerMetricsControl(pQosStressMetrics->pServerMetrics, 'spam', (int32_t)mpConfig->uDebugLevel, 0, NULL);
    }
    else
    {
        QosStressMetricsDestroy(pQosStressMetrics);
        DirtyCastLogPrintf("voipserver: unable to create module for reporting metrics to external aggregator\n");
        return(NULL);
    }

    //init
    pQosStressMetrics->pMemGroupUserData = pMemGroupUserData;
    pQosStressMetrics->iMemGroup = iMemGroup;
    pQosStressMetrics->pConfig = mpConfig;
    _ResetMetrics(&pQosStressMetrics->MetricsData);

    return (pQosStressMetrics);
}

/*F********************************************************************************/
/*!
    \Function QosStressMetricsDestroy

    \Description
    Destroy the QosStressMetricsRefT module.

    \Input *pQosStressMetrics - module state

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
void QosStressMetricsDestroy(QosStressMetricsRefT *pQosStressMetrics)
{
    // release module memory
    if (pQosStressMetrics)
    {
        if (pQosStressMetrics->pServerMetrics)
        {
            ServerMetricsDestroy(pQosStressMetrics->pServerMetrics);
        }
        DirtyMemFree(pQosStressMetrics, QOS_STRESS_MEMID, pQosStressMetrics->iMemGroup, pQosStressMetrics->pMemGroupUserData);
    }

}

/*F********************************************************************************/
/*!
    \Function QosStressMetricsUpdate

    \Description
    Service communication with the metrics agent

    \Input *pMetrics  - module state

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
void QosStressMetricsUpdate(QosStressMetricsRefT* pMetrics)
{
    ServerMetricsUpdate(pMetrics->pServerMetrics);
}

void QosStressMetricsAccumulate(QosStressMetricsRefT* pMetrics, QosCommonProcessedResultsT* pResults)
{
    _AddTestResults(pMetrics, pResults);
}



