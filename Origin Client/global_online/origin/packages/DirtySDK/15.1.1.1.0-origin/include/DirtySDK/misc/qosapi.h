/*H********************************************************************************/
/*!
    \File qosapi.h

    \Description
        Main include for the Quality of Service module.

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 1.0 04/07/2008 (cadam) first version
*/
/********************************************************************************H*/

#ifndef _qosapi_h
#define _qosapi_h

/*!
    \Moduledef Misc Misc

    \Description
        Miscellaneous network APIs

        <b>Overview</b>

        Overview TBD
*/

/*!
\Moduledef QosApi QosApi
\Modulemember Misc
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"

/*** Defines **********************************************************************/


//!< qosapi maximum response size
#define QOSAPI_RESPONSE_MAXSIZE     (256)

//! QoS Listen flags
#define QOSAPI_LISTENFL_ENABLE      (0x0001)    //!< enable QoS responses
#define QOSAPI_LISTENFL_DISABLE     (0x0002)    //!< disable QoS responses
#define QOSAPI_LISTENFL_SET_DATA    (0x0004)    //!< set title data
#define QOSAPI_LISTENFL_SET_BPS     (0x0008)    //!< set bits per second

//! QoS request flags                           //todo consider exposing the enum eRequestType instead
#define QOSAPI_REQUESTFL_CLIENT     (0x0000)    //!< client latency request
#define QOSAPI_REQUESTFL_LATENCY    (0x0001)    //!< server latency request
#define QOSAPI_REQUESTFL_BANDWIDTH  (0x0002)    //!< server bandwidth request
#define QOSAPI_REQUESTFL_FIREWALL   (0x0003)    //!< server firewall request

//! QoS Status flags
#define QOSAPI_STATFL_NONE          (0x0000)    //!< no change in status
#define QOSAPI_STATFL_COMPLETE      (0x0001)    //!< operation complete
#define QOSAPI_STATFL_PART_COMPLETE (0x0002)    //!< half responses received
#define QOSAPI_STATFL_DATA_RECEIVED (0x0004)    //!< title data received
#define QOSAPI_STATFL_DISABLED      (0x0008)    //!< "go away" received
#define QOSAPI_STATFL_CONTACTED     (0x0010)    //!< target contacted
#define QOSAPI_STATFL_TIMEDOUT      (0x0020)    //!< request timed out
#define QOSAPI_STATFL_FAILED        (0x0040)    //!< request failed

//! Qos Status Code
#define QOSAPI_STATUS_SUCCESS                       (1)      //!< operation complete successfully
#define QOSAPI_STATUS_UNSET                         (0)      //!< un-initialized value
#define QOSAPI_STATUS_INIT                          (-1)     //!< initialized, not no qos action was taken
#define QOSAPI_STATUS_USER_INIT                     (-2)     //!< initialized, outside of qosapi, likely blaze
#define QOSAPI_STATUS_PENDING                       (-3)     //!< qos operations were in progress
#define QOSAPI_STATUS_HTTP_PARSE_RESPONSE_FAILED    (-4)     //!< received http response but failed to parse it
#define QOSAPI_STATUS_NAME_RESOLUTION_FAILED        (-5)     //!< failed resolving dns
#define QOSAPI_STATUS_OPERATION_TIMEOUT             (-6)     //!< operation timed out

#define QOSAPI_STATUS_XB_PROBE_FAIL                 (-128)  
#define QOSAPI_STATUS_XB_SESSION_LOOKUP_FAIL        (-129)  
#define QOSAPI_STATUS_XB_SESSION_UNEXPECTED_NUM     (-130)  
#define QOSAPI_STATUS_XB_UNKNOWN_HOST               (-131)  
#define QOSAPI_STATUS_XB_ADDR_MAP_FAIL              (-131)  
#define QOSAPI_STATUS_XB_DISABLED                   (-132)  

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! known NAT types
typedef enum _QosApiNatTypeE
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
typedef enum _QosApiCBTypeE
{
    QOSAPI_CBTYPE_LATENCY,
    QOSAPI_CBTYPE_BANDWIDTH,
    QOSAPI_CBTYPE_NAT,
    QOSAPI_CBTYPE_X360_STATUS,
    QOSAPI_NUMCBTYPES           //!< max number of callback types
} QosApiCBTypeE;

//! QoS probe result information
typedef struct _QosInfoT {
    uint32_t uRequestId;
    uint32_t uWhenRequested;
    uint32_t uAddr;
    int32_t iProbesSent;
    int32_t iProbesRecv;
    int32_t iMinRTT;
    int32_t iMedRTT;
    uint32_t uUpBitsPerSec;
    uint32_t uDwnBitsPerSec;
    uint32_t uFlags;
    uint32_t hResult;
    struct sockaddr ExternalAddr;
} QosInfoT;

//! Qos callback information
typedef struct _QosApiCBInfoT {
    const QosInfoT *pQosInfo;
    QosApiNatTypeE eQosApiNatType;
    uint32_t uOldStatus;
    uint32_t uNewStatus;
} QosApiCBInfoT;

//! opaque module ref
typedef struct QosApiRefT QosApiRefT;

//! event callback function prototype
DIRTYCODE_API typedef void (QosApiCallbackT)(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
DIRTYCODE_API QosApiRefT *QosApiCreate(QosApiCallbackT *pCallback, void *pUserData, int32_t iServicePort);

// lookup QoS information for the QoS service provider
DIRTYCODE_API uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, char *pServerAddress, uint16_t uProbePort, uint32_t uServiceId, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags);

// cancel the current lookup request
DIRTYCODE_API int32_t QosApiCancelRequest(QosApiRefT *pQosApi, uint32_t uRequestId);

// change the current listen parameters
DIRTYCODE_API int32_t QosApiControl(QosApiRefT *pQosApi, int32_t iControl, int32_t iValue, void *pValue);

// get the current lookup request’s status
DIRTYCODE_API int32_t QosApiStatus(QosApiRefT *pQosApi, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// get the current NAT type of the client
DIRTYCODE_API QosApiNatTypeE QosApiGetNatType(QosApiRefT *pQosApi);

// destroy the module state
DIRTYCODE_API void QosApiDestroy(QosApiRefT *pQosApi);

#ifdef __cplusplus
}
#endif

//@}

#endif // _filename_h

