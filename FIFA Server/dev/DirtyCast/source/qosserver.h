/*H********************************************************************************/
/*!
    \File qosserver.h

    \Description
        This contains the declarations of the QOS server mode for the voip server
        
    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 09/24/2015 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _qos_server_h
#define _qos_server_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/misc/qoscommon.h"

/*** Type Definitions *************************************************************/

#define QOS_SERVER_MAX_SEND_ERROR_CODES_MONITORED (64)

//! opaque module ref
typedef struct QosServerRefT QosServerRefT;

//! forward declaration
typedef struct VoipServerRefT VoipServerRefT;

//! accumulate statistics on why probes are failing validation
typedef struct QosServerProbeValidationFailCountsT
{
    uint32_t uProtocol;                     //!< probe and server have a QOS_COMMON_PROBE_VERSION_MAJOR mismatch
    uint32_t uVersion;                      //!< packet doesn't start with 'qos2'
    uint32_t uRecvTooFewBytes;              //!< the number of bytes received isn't enough to make a probe
    uint32_t uRecvUnexpectedNumBytes;       //!< the number of bytes received does not match the stated size of the uProbeSizeUp
    uint32_t uProbeSizeUpTooSmall;          //!< the probe is impossibly small because the minimum fields cant be represented in what was received
    uint32_t uProbeSizeDownTooSmall;        //!< the probe requests the server to send probes smaller than is possible
    uint32_t uProbeSizeUpTooLarge;          //!< the probe is larger than QOS_COMMON_MAX_PACKET_SIZE
    uint32_t uProbeSizeDownTooLarge;        //!< the probe requests the server to send packets larger than QOS_COMMON_MAX_PACKET_SIZE
    uint32_t uUnexpectedExternalAddress;    //!< the external address stamped into the packet body (client obtained it from the coordinator), doesn't match the address the qos server received it from
    uint32_t uUnexpectedServerReceiveTime;  //!< the probe has a non-zero receive time in its body, the client should not have set the receive time
    uint32_t uUnexpectedServerSendDelta;    //!< the probe has a non-zero artificial latency time in its body, the client should not have set the send delta
    uint32_t uProbeCountDownTooHigh;        //!< the probe has requested more response probes than QOS_COMMON_MAX_PROBE_COUNT
    uint32_t uProbeCountUpTooHigh;          //!< the probe claims its index is higher than QOS_COMMON_MAX_PROBE_COUNT
    uint32_t uExpectedProbeCountUpTooHigh;  //!< the probe claims that the max probe index of this test is beyond QOS_COMMON_MAX_PROBE_COUNT
    uint32_t uFirstHmacFailed;              //!< the first hmac has failed, this may be due to the secure key changed
    uint32_t uSecondHmacFailed;             //!< the second hmac has failed, this probe is rejected
    uint32_t uServiceRequestIdTooOld;       //!< the service request id is older than we expected, we have serviced beyond this id over a minute ago
    uint32_t uFromPort;                     //!< the address we received the probe from has a 0 port to send back to
} QosServerProbeValidationFailCountsT;

//! accumulate statistics on how often send error codes are encountered
typedef struct QosServerProbeSendErrorCodeCountT
{
    int32_t iErrorCode;
    uint32_t uCount;
} QosServerProbeSendErrorCodeCountT;

//! attempt to classify the traffic into common use cases
typedef enum QosServerProbeTypeE
{
    PROBE_TYPE_LATENCY,
    PROBE_TYPE_BW_UP,
    PROBE_TYPE_BW_DOWN,
    PROBE_TYPE_VALIDATION_FAILED,
    PROBE_TYPE_COUNT
} QosServerProbeTypeE;

//! gather metrics about server traffic
typedef struct QosServerMetricsT
{
    QosServerProbeValidationFailCountsT ProbeValidationFailures;
    uint64_t aSendPacketCountDistribution[PROBE_TYPE_COUNT][QOS_COMMON_MAX_PROBE_COUNT];
    uint64_t aSendCountSuccess[PROBE_TYPE_COUNT];
    uint64_t aSendCountFailed[PROBE_TYPE_COUNT];
    uint64_t aSentBytes[PROBE_TYPE_COUNT];
    uint64_t aReceivedCount[PROBE_TYPE_COUNT];
    uint64_t aReceivedBytes[PROBE_TYPE_COUNT];
    uint64_t aArtificialLatency[PROBE_TYPE_COUNT];
    uint64_t aArtificialLatencyCount[PROBE_TYPE_COUNT]; 
    uint64_t aPastSecondLoad[PROBE_TYPE_COUNT];
    uint64_t aExtraProbes[PROBE_TYPE_COUNT];
    QosServerProbeSendErrorCodeCountT aProbeSendErrorCodeCounts[QOS_SERVER_MAX_SEND_ERROR_CODES_MONITORED];
    uint32_t uCapacityMax;
    uint32_t uMainTimeStalled;
    uint32_t uRecvTimeStalled;
    uint8_t bRegisterdWithCoordinator;
} QosServerMetricsT;

/*** Functions ********************************************************************/

// create the module
QosServerRefT *QosServerInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags);

// update the module
uint8_t QosServerProcess(QosServerRefT *pQosServer, uint32_t *pSignalFlags, uint32_t uCurTick);

// get the base to access the shared functionality
VoipServerRefT *QosServerGetBase(QosServerRefT *pQosServer);

// status function
int32_t QosServerStatus(QosServerRefT *pQosServer, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t QosServerControl(QosServerRefT *pQosServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// drain the module
uint32_t QosServerDrain(QosServerRefT *pRef);

// destroy the module
void QosServerShutdown(QosServerRefT *pQosServer);

#endif // _qos_server_h
