/*H********************************************************************************/
/*!
    \File qosapi.h

    \Description
        Main include for the Quality of Service module.

    \Notes
        None.

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 1.0 04/07/2008 (cadam) first version
*/
/********************************************************************************H*/

#ifndef _qosapi_h
#define _qosapi_h

/*** Include files ****************************************************************/

#include "dirtyaddr.h"

/*** Defines **********************************************************************/


//!< qosapi maximum response size
#define QOSAPI_RESPONSE_MAXSIZE     (256)

//! QoS Listen flags
#define QOSAPI_LISTENFL_ENABLE      (0x0001)    //!< enable QoS responses
#define QOSAPI_LISTENFL_DISABLE     (0x0002)    //!< disable QoS responses
#define QOSAPI_LISTENFL_SET_DATA    (0x0004)    //!< set title data
#define QOSAPI_LISTENFL_SET_BPS     (0x0008)    //!< set bits per second

//! QoS request flags
#define QOSAPI_REQUESTFL_BANDWIDTH  (0x0000)    //!< bandwidth requests
#define QOSAPI_REQUESTFL_LATENCY    (0x0001)    //!< latency only requests

//! QoS Status flags
#define QOSAPI_STATFL_COMPLETE      (0x0001)    //!< operation complete
#define QOSAPI_STATFL_PART_COMPLETE (0x0002)    //!< half responses received
#define QOSAPI_STATFL_DATA_RECEIVED (0x0004)    //!< title data received
#define QOSAPI_STATFL_DISABLED      (0x0008)    //!< "go away" received
#define QOSAPI_STATFL_CONTACTED     (0x0010)    //!< target contacted
#define QOSAPI_STATFL_TIMEDOUT      (0x0020)    //!< request timed out
#define QOSAPI_STATFL_FAILED        (0x0040)    //!< request failed

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! known NAT types
typedef enum
{
    QOSAPI_NAT_UNKNOWN,
    QOSAPI_NAT_OPEN,
    QOSAPI_NAT_MODERATE, 
    QOSAPI_NAT_STRICT_SEQUENTIAL,
    QOSAPI_NAT_STRICT,
	QOSAPI_NAT_PENDING,         //!< This value does not exist in the server enum, the server should never have this value
	
    QOSAPI_NUMNATTYPES          //!< max number of nat types
} QosApiNatTypeE;

//! callback types
typedef enum
{
    QOSAPI_CBTYPE_NAT,
    QOSAPI_CBTYPE_STATUS,

    QOSAPI_NUMCBTYPES           //!< max number of callback types
} QosApiCBTypeE;

//! Qos Listen status information
typedef struct {
    uint32_t uStatusSize;
    uint32_t uDataRequestsRecv;
    uint32_t uProbeRequestsRecv;
    uint32_t uRequestsDiscarded;
    uint32_t uDataRepliesSent;
    uint32_t uProbeRepliesSent;
} QosListenStatusT;

//! QoS probe result information
typedef struct {
    uint32_t uRequestId;
    uint32_t uWhenRequested;
    uint32_t uAddr;
    int32_t iProbesSent;
    int32_t iProbesRecv;
    char aData[QOSAPI_RESPONSE_MAXSIZE];
    int32_t iDataLen;
    int32_t iMinRTT;
    int32_t iMedRTT;
    uint32_t uUpBitsPerSec;
    uint32_t uDwnBitsPerSec;
    uint32_t uFlags;
    struct sockaddr ExternalAddr;
} QosInfoT;

//! Qos callback information
typedef struct {
    const QosInfoT *pQosInfo;
    QosApiNatTypeE eQosApiNatType;
    uint32_t uOldStatus;
    uint32_t uNewStatus;
} QosApiCBInfoT;

//! opaque module ref
typedef struct QosApiRefT QosApiRefT;

//! event callback function prototype
typedef void (QosApiCallbackT)(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
QosApiRefT *QosApiCreate(QosApiCallbackT *pCallback, void *pUserData, int32_t iServicePort);

// start listening for QoS probes/messages
int32_t QosApiListen(QosApiRefT *pQosApi, uint8_t *pResponse, uint32_t uResponseSize, uint32_t uBitsPerSecond, uint32_t uFlags);

// lookup QoS information for a specified number of hosts
uint32_t QosApiRequest(QosApiRefT *pQosApi, DirtyAddrT *pAddr, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags);

// lookup QoS information for the QoS service provider
uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, char *pServerAddress, uint16_t uProbePort, uint32_t uServiceId, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags);

// cancel the current lookup request
int32_t QosApiCancelRequest(QosApiRefT *pQosApi, uint32_t uRequestId);

// change the current listen parameters
int32_t QosApiControl(QosApiRefT *pQosApi, int32_t iControl, int32_t iValue, void *pValue);

// get the current lookup request’s status
int32_t QosApiStatus(QosApiRefT *pQosApi, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// get the current NAT type of the client
QosApiNatTypeE QosApiGetNatType(QosApiRefT *pQosApi);

// destroy the module state
void QosApiDestroy(QosApiRefT *pQosApi);

#ifdef __cplusplus
}
#endif

#endif // _filename_h

