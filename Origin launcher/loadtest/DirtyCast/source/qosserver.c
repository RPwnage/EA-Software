/*H********************************************************************************/
/*!
    \File qosserver.c

    \Description
        This contains the implementation for the QOS mode for the voip server

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/24/2015 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/base64.h"

#include "qosserver.h"
#include "qosservice.h"
#include "tokenapi.h"
#include "voipserver.h"
#include "voipservercomm.h"
#include "udpmetrics.h"
#include "voipserverconfig.h"


/*** Defines **********************************************************************/
#define QOS_SERVER_DRAIN_TIME                               (60 * 1000)     //wait a minute to drain in progress users
#define QOS_SERVER_MAIN_STALL_THRESHOLD                     (1500)          //how long to pass between updates of the main thread before we complain
#define QOS_SERVER_RECV_STALL_THRESHOLD                     (500)           //how long to pass between updates of the recv thread before we complain
#define QOS_SERVER_THROTTLED_PRINT_COUNT_MAX                (100)           //the maximum number of throttled prints over QOS_SERVER_THROTTLED_PRINT_DURATION
#define QOS_SERVER_THROTTLED_PRINT_DURATION                 (1000 * 60 * 60)//the amount of time the throttled print is evaluated for before reseting
#define QOS_SERVER_MEMID                                    ('qoss')

#define QOS_SERVER_PROBE_VALIDATION_SUCCESS (0)
#define QOS_SERVER_PROBE_VALIDATION_FAILED  (-1)
#define QOS_SERVER_PROBE_VALIDATION_LIMITED (-2)

/*** Type Definitions *************************************************************/

//! state for the connection QOS mode
typedef struct QosServerRefT
{
    VoipServerRefT *pBase;                      //!< Used to access shared data
    QosServiceRefT *pQosService;                //!< used to communicate with the QosCoordinator
    int32_t iSpamValue;                         //!< set the debug level

    //! metrics
    QosServerMetricsT Metrics;
    QosServerMetricsT PreviousMetrics;

    //! load statistics (how busy are we) used to populate some metrics
    uint32_t aCurrentSecondLoad[PROBE_TYPE_COUNT];  //!< the number of new requests created during this second (to be used in aPastSecondLoad metric)
    uint32_t uTimeLastLoad;                         //!< the tick the last time we stored aCurrentSecondLoad

    //! shutdown
    uint32_t uShutdownStartTimer;               //!< for draining, we don't really track users so we don't wait for all users to complete, we just assume they will be done after some time

    //! allocation configuration
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    //! listening socket
    SocketT *pQosSocket;                            //!< socket pointer
    uint8_t aRecvBuff[QOS_COMMON_MAX_PACKET_SIZE];  //!< buffer we will be receiving into
    int32_t iRecvLen;                               //!< amount of data we last received
} QosServerRefT;

/*** Function Prototypes **********************************************************/
static int32_t _QosServerRecvCB(SocketT *pSocket, int32_t iFlags, void *pData);
static void _QosServerPrintProbeData(uint32_t uIndex, const uint8_t *pInBuff, uint32_t uBuffLen);

/*** Variables ********************************************************************/

/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _QosServerCheckMainStall

    \Description
        Log metrics if the main thread hasn't been firing consistently

    \Input *pQosServer      - pointer to module state

    \Version 09/07/2018 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerCheckMainStall(QosServerRefT *pQosServer)
{
    static uint32_t _uPrevTime = 0;
    uint32_t uTimeDiff;
    uint32_t uCurrentTime = NetTick();
    if (_uPrevTime == 0)
    {
        _uPrevTime = uCurrentTime;
    }
    uTimeDiff = NetTickDiff(uCurrentTime, _uPrevTime);
    if (uTimeDiff > QOS_SERVER_MAIN_STALL_THRESHOLD)
    {
        pQosServer->Metrics.uMainTimeStalled += uTimeDiff;
    }
    _uPrevTime = uCurrentTime;
}

/*F********************************************************************************/
/*!
    \Function _QosServerCheckRecvStall

    \Description
        Log metrics if the receive thread hasn't been firing consistently

    \Input *pQosServer      - pointer to module state

    \Version 09/07/2018 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerCheckRecvStall(QosServerRefT *pQosServer)
{
    static uint32_t _uPrevTime = 0;
    uint32_t uTimeDiff;
    uint32_t uCurrentTime = NetTick();
    if (_uPrevTime == 0)
    {
        _uPrevTime = uCurrentTime;
    }
    uTimeDiff = NetTickDiff(uCurrentTime, _uPrevTime);
    if (uTimeDiff > QOS_SERVER_RECV_STALL_THRESHOLD)
    {
        pQosServer->Metrics.uRecvTimeStalled += uTimeDiff;
    }
    _uPrevTime = uCurrentTime;
}

/*F********************************************************************************/
/*!
    \Function _QosServerThrottledPrintfVerbose

    \Description
        Log printing for DirtyCast servers, with verbose level check; only prints
        if iVerbosityLevel is > iCheckLevel and only up to 100 times per hour.

    \Input iVerbosityLevel  - current verbosity level
    \Input iCheckLevel      - level to check against
    \Input *pFormat         - printf format string
    \Input ...              - variable argument list

    \Output
        uint8_t     -  TRUE if we printed, false if it was suppressed.

    \Version 09/05/2018 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _QosServerThrottledPrintfVerbose(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...)
{
    static uint32_t _uPrintCount = 0;
    static uint32_t _uFirstPrintTime = 0;

    if (iVerbosityLevel > iCheckLevel)
    {
        //check to see if its time to reset our print count and timing
        if ((_uFirstPrintTime == 0) || ((uint32_t)NetTickDiff(NetTick(), _uFirstPrintTime) > QOS_SERVER_THROTTLED_PRINT_DURATION))
        {
            //if we are enabling, were we disabled?
            if (_uPrintCount > QOS_SERVER_THROTTLED_PRINT_COUNT_MAX)
            {
                DirtyCastLogPrintf("qosserver: _QosServerThrottledPrintfVerbose, re-enabling prints %u prints were suppressed.\n", _uPrintCount - QOS_SERVER_THROTTLED_PRINT_COUNT_MAX);
            }
            _uFirstPrintTime = NetTick();
            _uPrintCount = 0;
        }
        _uPrintCount++;

        //print
        if (_uPrintCount < QOS_SERVER_THROTTLED_PRINT_COUNT_MAX)
        {
            va_list Args;
            char strText[2048];

            va_start(Args, pFormat);
            ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
            va_end(Args);
            DirtyCastLogPrintf(strText);
            return(TRUE);
        }
        //warn once about suppression
        else if (_uPrintCount == QOS_SERVER_THROTTLED_PRINT_COUNT_MAX)
        {
            //how long are we going to suppress the prints?
            uint32_t uSuppressTime = QOS_SERVER_THROTTLED_PRINT_DURATION - (NetTickDiff(NetTick(), _uFirstPrintTime));
            DirtyCastLogPrintf("qosserver: _QosServerThrottledPrintfVerbose, error, prints are being suppressed for the next %ums.\n", uSuppressTime);
        }
        // else just suppress the print
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosServerValidateProbe

    \Description
        Ensure the probe is correct and can be trusted.

    \Input *pQosServer      - pointer to module state
    \Input *pProbe          - received probe 
    \Input *pFrom           - address the probe was received from

    \Output
        int32_t     -  QOS_SERVER_PROBE_VALIDATION_*; 0 success, -1 failed, -2 validated but could not agree on source address

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosServerValidateProbe(QosServerRefT *pQosServer, QosCommonProbePacketT *pProbe, struct sockaddr *pFrom)
{
    int32_t iRet = QOS_SERVER_PROBE_VALIDATION_SUCCESS;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pQosServer->pBase);

    // make sure the probe claims to be of the same protocol
    if (pProbe->uProtocol != QOS_COMMON_PROBE_PROTOCOL_ID)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, probe protocol=%C is not compatible with local protocol=%C.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProtocol, QOS_COMMON_PROBE_PROTOCOL_ID);
        pQosServer->Metrics.ProbeValidationFailures.uProtocol++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // make sure the probe claims to be of a compatible version
    if (!QosCommonIsCompatibleProbeVersion(pProbe->uVersion))
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, probe version=%u is not compatible with local version=%u.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uVersion, QOS_COMMON_PROBE_VERSION);
        pQosServer->Metrics.ProbeValidationFailures.uVersion++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // make sure the probe size matches the number of bytes we received
    if (pProbe->uProbeSizeUp != pQosServer->iRecvLen)
    {
        uint32_t uNumBundledProbes, uBundledProbeIndex;
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeSizeUp=%u, does not match the number of bytes received=%d\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeSizeUp, pQosServer->iRecvLen);
        pQosServer->Metrics.ProbeValidationFailures.uRecvUnexpectedNumBytes++;
        
        //try to print info of the probes, if there are multiple probes bundled together (we've seen some packets multiples the size of a typical probe)
        uNumBundledProbes = pQosServer->iRecvLen / pProbe->uProbeSizeUp;
        
        //i expect all the probes in the bundle are the same size but lets check
        if ((uNumBundledProbes * pProbe->uProbeSizeUp) > (uint32_t)pQosServer->iRecvLen)
        {
            uNumBundledProbes = 1; //our guess was wrong, lets just go with 1 then
        }

        for (uBundledProbeIndex = 0; uBundledProbeIndex < uNumBundledProbes; uBundledProbeIndex++)
        {
            _QosServerPrintProbeData(uBundledProbeIndex, pQosServer->aRecvBuff + (uBundledProbeIndex * pProbe->uProbeSizeUp), pProbe->uProbeSizeUp);
        }
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // make sure the probe isn't smaller than could be possible
    if (pProbe->uProbeSizeUp < QOS_COMMON_MIN_PACKET_SIZE)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeSizeUp=%u, is smaller than QOS_COMMON_MIN_PACKET_SIZE=%u\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeSizeUp, QOS_COMMON_MIN_PACKET_SIZE);
        pQosServer->Metrics.ProbeValidationFailures.uProbeSizeUpTooSmall++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uProbeSizeDown < QOS_COMMON_MIN_PACKET_SIZE)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeSizeDown=%u, is smaller than QOS_COMMON_MIN_PACKET_SIZE=%u\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeSizeDown, QOS_COMMON_MIN_PACKET_SIZE);
        pQosServer->Metrics.ProbeValidationFailures.uProbeSizeDownTooSmall++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // make sure the probe isn't larger than could be possible
    if (pProbe->uProbeSizeUp > QOS_COMMON_MAX_PACKET_SIZE)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeSizeUp=%u, is larger than QOS_COMMON_MAX_PACKET_SIZE=%u\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeSizeUp, QOS_COMMON_MAX_PACKET_SIZE);
        pQosServer->Metrics.ProbeValidationFailures.uProbeSizeUpTooLarge++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uProbeSizeDown > QOS_COMMON_MAX_PACKET_SIZE)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeSizeDown=%u, is larger than QOS_COMMON_MAX_PACKET_SIZE=%u\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeSizeDown, QOS_COMMON_MAX_PACKET_SIZE);
        pQosServer->Metrics.ProbeValidationFailures.uProbeSizeDownTooLarge++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uServerReceiveTime != 0)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uServerReceiveTime=%u, expected to be 0, NetTick=%u.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uServerReceiveTime, NetTick());
        pQosServer->Metrics.ProbeValidationFailures.uUnexpectedServerReceiveTime++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uServerSendDelta != 0)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uServerSendDelta=%u, expected to be 0.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uServerSendDelta);
        pQosServer->Metrics.ProbeValidationFailures.uUnexpectedServerSendDelta++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // make sure the probe counts are within expected limits
    if (pProbe->uProbeCountDown > QOS_COMMON_MAX_PROBE_COUNT)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeCountDown=%u, higher than expected.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeCountDown);
        pQosServer->Metrics.ProbeValidationFailures.uProbeCountDownTooHigh++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uProbeCountUp > QOS_COMMON_MAX_PROBE_COUNT)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uProbeCountUp=%u, higher than expected.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uProbeCountUp);
        pQosServer->Metrics.ProbeValidationFailures.uProbeCountUpTooHigh++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    if (pProbe->uExpectedProbeCountUp > QOS_COMMON_MAX_PROBE_COUNT)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uExpectedProbeCountUp=%u, higher than expected.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uExpectedProbeCountUp);
        pQosServer->Metrics.ProbeValidationFailures.uExpectedProbeCountUpTooHigh++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    //there isn't really a way to validate that the pProbe->uClientRequestId is valid, so no validation is done

    // check to see if the service request id is in expected range
    uint32_t uMinServiceRequestId = QosServiceStatus(pQosServer->pQosService, 'msid', 0, NULL, 0);
    if (pProbe->uServiceRequestId <= uMinServiceRequestId)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, uServiceRequestId=%u, is lower than expected, expected higher than %u.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, pProbe->uServiceRequestId, uMinServiceRequestId);
        pQosServer->Metrics.ProbeValidationFailures.uServiceRequestIdTooOld++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // check to see if the from address has a valid port to send to "0" is not valid
    if (SockaddrInGetPort(pFrom) == 0)
    {
        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, port is 0.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom);
        pQosServer->Metrics.ProbeValidationFailures.uFromPort++;
        return(QOS_SERVER_PROBE_VALIDATION_FAILED);
    }

    // if we are using subspace we can't validate the ip address
    if (pConfig->uSubspaceLocalPort != 0)
    {
        iRet = QOS_SERVER_PROBE_VALIDATION_LIMITED;
    }
    // make sure the incoming probe is coming from where we expect
    else if (QosCommonIsRemappedAddrEqual(&pProbe->clientAddressFromService, pFrom) == FALSE)
    {
        char strAddrBuff[128];
        int32_t iLogLevel = 1;
        // we don't typically want to print this for every probe, it could be spammy
        if (pProbe->uProbeCountUp != 0)
        {
            iLogLevel = 2;
        }

        DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, iLogLevel, "qosserver: [%u:%u]%A, warning, clientAddressFromService=%s, expected to match pFrom.\n", pProbe->uServiceRequestId, pProbe->uClientRequestId, pFrom, QosCommonAddrToString(&pProbe->clientAddressFromService, strAddrBuff, sizeof(strAddrBuff)));
        pQosServer->Metrics.ProbeValidationFailures.uUnexpectedExternalAddress++;
        iRet = QOS_SERVER_PROBE_VALIDATION_LIMITED;
    }

    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function _QosServerGetProbeType

    \Description
        Although every probe has the same fields, attempt to classify what sort of test
        is likely being performed based on the settings used in the probe.

    \Input *pQosServer      - pointer to module state
    \Input *pInProbe        - received probe

    \Output
        QosServerProbeTypeE - enum identifying test type.

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static QosServerProbeTypeE _QosServerGetProbeType(QosServerRefT *pQosServer, QosCommonProbePacketT *pInProbe)
{
    QosServerProbeTypeE eProbeType;

    /*  to calculate metrics, here we make some assumptions about typical usage to make logical counts of how much each feature gets used
        if the probe is large its a bandwidth test, lets call "large" anything bigger than the minimum size. */
    if (pInProbe->uProbeSizeUp > QOS_COMMON_MIN_PACKET_SIZE)
    {
        eProbeType = PROBE_TYPE_BW_UP;
    }
    else if (pInProbe->uProbeSizeDown > QOS_COMMON_MIN_PACKET_SIZE)
    {
        eProbeType = PROBE_TYPE_BW_DOWN;
    }
    else //if its not bandwidth up/down then its a latency probe
    {
        eProbeType = PROBE_TYPE_LATENCY;
    }
    return(eProbeType);
}

/*F********************************************************************************/
/*!
    \Function _QosServerPrepareOutgoingProbe

    \Description
        Although every probe has the same fields, attempt to classify what sort of test
        is likely being performed based on the settings used in the probe.

    \Input *pQosServer      - pointer to module state
    \Input *pInProbe        - received probe
    \Input *pOutProbe       - probe ready to be sent as response
    \Input *pFrom           - address the probe was received from
    \Input *pSecureKey      - secure key to use to sign this probe
    \Input uSendCounter     - probe index being sent
    \Input uTime            - accurate time the server received the incoming probe

    \Output
        uint32_t     - number of probes we expect the associated test to still send


    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosServerPrepareOutgoingProbe(QosServerRefT *pQosServer, QosCommonProbePacketT *pInProbe, QosCommonProbePacketT *pOutProbe, struct sockaddr *pFrom, uint8_t *pSecureKey, uint32_t uSendCounter, uint32_t uTime)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pQosServer->pBase);
    int32_t iNumMoreExpectedProbes = 0;

    //the outgoing probe is mostly a copy of the incoming probe
    ds_memcpy(pOutProbe, pInProbe, sizeof(QosCommonProbePacketT));

    //we add some additional bits of info
    pOutProbe->uServerReceiveTime = uTime;

    // if we are using subspace the qos server will not see the users real ip (so skip below code to stick with the coordinators view)
    // note the coordinators view is still missing the port used on the client, so this address is not valid.
    if (pConfig->uSubspaceLocalPort == 0)
    {
        //the qos servers view of the clients address takes precedence (over what the coordinator provided the client)
        QosCommonConvertAddr(&pOutProbe->clientAddressFromService, pFrom);
    }
    pOutProbe->uVersion = QOS_COMMON_PROBE_VERSION;
    pOutProbe->uServerSendDelta = NetTickDiff(NetTick(), pOutProbe->uServerReceiveTime);

    if (pInProbe->uProbeCountDown == 1)
    {
        // the probe will have the same uProbeCountDown stamped in it as the incoming uProbeCountUp (1 to 1 mapping)
        pOutProbe->uProbeCountDown = pInProbe->uProbeCountUp;
        iNumMoreExpectedProbes = (pOutProbe->uExpectedProbeCountUp - 1) - pInProbe->uProbeCountUp;
    }
    else
    {
        /*  the server will send a burst of probes each stamped with a unique uProbeCountDown (1 to many)
            at first uProbeCountDown will contain the total number of probes this packet will respond with, so if uSendCounter matches it we've reached the last one . */
        iNumMoreExpectedProbes = (pOutProbe->uProbeCountDown - 1) - uSendCounter;
        // instead of echoing back the number of probes expected to come down they each get their own index
        pOutProbe->uProbeCountDown = uSendCounter;
    }
    return iNumMoreExpectedProbes;
}

/*F********************************************************************************/
/*!
    \Function _QosServerAddProbeSendErrorCodeCount

    \Description
        In the case of a send error, save metrics about frequency of each error code.

    \Input *pQosServer  - pointer to module state
    \Input iCode        - error code encountered
    
    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerAddProbeSendErrorCodeCount(QosServerRefT *pQosServer, int32_t iCode)
{
    int32_t iIndex;
    for (iIndex = 0; iIndex < QOS_SERVER_MAX_SEND_ERROR_CODES_MONITORED; iIndex++)
    {
        // find the first empty slot or if we see our error code in the list
        if ((pQosServer->Metrics.aProbeSendErrorCodeCounts[iIndex].uCount == 0) || (pQosServer->Metrics.aProbeSendErrorCodeCounts[iIndex].iErrorCode == iCode))
        {
            pQosServer->Metrics.aProbeSendErrorCodeCounts[iIndex].iErrorCode = iCode;
            pQosServer->Metrics.aProbeSendErrorCodeCounts[iIndex].uCount++;
            return;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _QosServerProcessProbe

    \Description
        A probe has been received, react to it; validate it and send back probe to the client.

    \Input *pQosServer  - pointer to module state
    \Input *pInProbe    - received probe
    \Input *pSecureKey  - key used to authenticate the probe
    \Input *pFrom       - address the probe was received from
    \Input iAddrLen     - size of address field

    \Output
        int32_t - probe send result

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosServerProcessProbe(QosServerRefT *pQosServer, QosCommonProbePacketT *pInProbe, uint8_t *pSecureKey, struct sockaddr *pFrom, int32_t iAddrLen)
{
    uint32_t uSendCounter;
    QosCommonProbePacketT outProbe;
    int32_t iResult = 0;
    int32_t iProbeValidationResult;
    uint32_t uTime;
    uint8_t aSendBuff[QOS_COMMON_MAX_PACKET_SIZE];
    QosServerProbeTypeE eProbeType;

    // accurate measurement of when the probe came in
    uTime = SockaddrInGetMisc(pFrom);

    iProbeValidationResult = _QosServerValidateProbe(pQosServer, pInProbe, pFrom);

    if (iProbeValidationResult == QOS_SERVER_PROBE_VALIDATION_FAILED)
    {
        // printing will be done by _QosServerValidateProbe
        pQosServer->Metrics.aReceivedCount[PROBE_TYPE_VALIDATION_FAILED]++;
        pQosServer->Metrics.aReceivedBytes[PROBE_TYPE_VALIDATION_FAILED] += pQosServer->iRecvLen;
        pQosServer->aCurrentSecondLoad[PROBE_TYPE_VALIDATION_FAILED]++;
        return(-1);
    }
    else
    {
        eProbeType = _QosServerGetProbeType(pQosServer, pInProbe);
        pQosServer->Metrics.aReceivedCount[eProbeType]++;
        pQosServer->Metrics.aReceivedBytes[eProbeType] += pQosServer->iRecvLen;
    }

    if (iProbeValidationResult == QOS_SERVER_PROBE_VALIDATION_LIMITED)
    {
        /*
        in this case we haven't validated that the address in the probe and the from address are the same, this might be the result of ip spoofing 
        we will disallow sending of many probes or large probes (likely impacting bandwidth tests), this will however give the client an opportunity to update 
        the address they are providing in their packets, with the one from the probe we do send back, and they can then kick off the test without being limited anymore
        */
        pInProbe->uProbeCountDown = 1;
        pInProbe->uProbeSizeDown = QOS_COMMON_MIN_PACKET_SIZE;
    }

    ds_memclr(aSendBuff, sizeof(aSendBuff));

    // send response probes
    for (uSendCounter = 0; uSendCounter < pInProbe->uProbeCountDown; uSendCounter++)
    {
        int32_t iRemainingExpectedProbes = _QosServerPrepareOutgoingProbe(pQosServer, pInProbe, &outProbe, pFrom, pSecureKey, uSendCounter, uTime);
        
        if (QosCommonSerializeProbePacket(aSendBuff, sizeof(aSendBuff), &outProbe, pSecureKey) != QOS_COMMON_MIN_PACKET_SIZE)
        {
            DirtyCastLogPrintf("qosserver: error, serialized packet is not the expected size.\n");
            return(-2);
        }
        iResult = SocketSendto(pQosServer->pQosSocket, (char *)aSendBuff, outProbe.uProbeSizeDown, 0, pFrom, iAddrLen);
        
        if (iResult == outProbe.uProbeSizeDown) //success, log metrics
        {
            uint8_t uPrinted = FALSE;
            pQosServer->Metrics.aSendPacketCountDistribution[eProbeType][outProbe.uProbeCountDown]++;
        
            if (outProbe.uProbeCountDown == 0)  //first probe
            {
                uPrinted = TRUE;
                pQosServer->aCurrentSecondLoad[eProbeType]++;
                //we print the first packet every time
                DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, SF, r=%d.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, iResult);
            }
            if (iRemainingExpectedProbes == 0) // last probe
            {
                uPrinted = TRUE;
                DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, SL=%u, r=%d.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, outProbe.uProbeCountDown, iResult);
            }
            if (iRemainingExpectedProbes < 0) // more probes then we expected, there was probably packet loss and these are retries
            {
                uPrinted = TRUE;
                pQosServer->Metrics.aExtraProbes[eProbeType]++;
                DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, SX=%u, r=%d.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, outProbe.uProbeCountDown, iResult);
            }                
            if (!uPrinted) //we print the middle probes only on high logging levels
            {
                DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 2, "qosserver: [%u:%u]%A, SM=%u, r=%d.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, outProbe.uProbeCountDown, iResult);
            }

            pQosServer->Metrics.aSendCountSuccess[eProbeType]++;
            pQosServer->Metrics.aSentBytes[eProbeType] += iResult;
            
            if (outProbe.uServerSendDelta >= 2)
            {
                //if we've created more than 2ms of artificial latency complain
                pQosServer->Metrics.aArtificialLatency[eProbeType] += outProbe.uServerSendDelta;
                pQosServer->Metrics.aArtificialLatencyCount[eProbeType]++;
                DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 1, "qosserver: [%u:%u]%A, warning, probe %u had %dms of artificial latency.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, outProbe.uProbeCountDown, outProbe.uServerSendDelta);
            }
            
        }
        else  //fail
        {
            //some error, complain a bit more loudly
            uint32_t uLastError = DirtyCastGetLastError();
            DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, failed sending probe %u, result=%d, errno=%d, expected result %d.\n", pInProbe->uServiceRequestId, pInProbe->uClientRequestId, pFrom, outProbe.uProbeCountDown, iResult, uLastError, outProbe.uProbeSizeDown);
            pQosServer->Metrics.aSendCountFailed[eProbeType]++;

            //add metrics about this error
            _QosServerAddProbeSendErrorCodeCount(pQosServer, uLastError);

            //break if we were trying to send multiple probes to the same target, they were probably going to error too.
            break; 
        }    
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _QosServerPrintProbeData

    \Description
        Convert the bytes to a probe and print out its fields using DirtyCastLogPrintf

    \Input uIndex       - value to tag the print with
    \Input *pInBuff     - received data
    \Input uBuffLen     - amount of received data

    \Version 09/28/2018 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerPrintProbeData(uint32_t uIndex, const uint8_t *pInBuff, uint32_t uBuffLen)
{
    QosCommonProbePacketT probe;
    uint8_t aProbeBuff[QOS_COMMON_MIN_PACKET_SIZE];
    char strProbeData[128];
    char strHmac[128];

    ds_memclr(aProbeBuff, QOS_COMMON_MIN_PACKET_SIZE); //use our own buffer so we know that the end is clear, often we'll be printing an incomplete probe
    ds_memcpy_s(aProbeBuff, QOS_COMMON_MIN_PACKET_SIZE, pInBuff, uBuffLen);
    QosCommonDeserializeProbePacketInsecure(&probe, aProbeBuff);
    
    Base64Encode2((const char *)pInBuff, QOS_COMMON_SIZEOF_PROBE_DATA, strProbeData, sizeof(strProbeData));
    Base64Encode2((const char *)pInBuff + QOS_COMMON_SIZEOF_PROBE_DATA, QOS_COMMON_HMAC_SIZE, strHmac, sizeof(strHmac));

    DirtyCastLogPrintf("qosserver: probe(%u) dump: data=%s, hmac=%s\n", uIndex, strProbeData, strHmac);
    DirtyCastLogPrintf("qosserver: probe(%u) values, %dbytes: pro=%C, ver=%u, sid=%u, cid=%u, srt=%u, ssd=%u, sup=%u, sdo=%u, cup=%u, cdo=%u, eup=%u, fam=%u, prt=%u, add=%a\n", 
        uIndex,
        uBuffLen,
        probe.uProtocol, 
        probe.uVersion, 
        probe.uServiceRequestId, 
        probe.uClientRequestId, 
        probe.uServerReceiveTime, 
        probe.uServerSendDelta, 
        probe.uProbeSizeUp, 
        probe.uProbeSizeDown, 
        probe.uProbeCountUp, 
        probe.uProbeCountDown, 
        probe.uExpectedProbeCountUp, 
        probe.clientAddressFromService.uFamily, 
        probe.clientAddressFromService.uPort, 
        probe.clientAddressFromService.addr.v4);
}


/*F********************************************************************************/
/*!
    \Function _QosServerAuthenticateProbe

    \Description
        Take the received bytes validate the hmac and generate a struct representing the probe.

    \Input *pQosServer  - pointer to module state
    \Input *pInBuff     - received data
    \Input uBuffLen     - amount of received data
    \Input *pOutProbe   - struct to hold the probe data after deserialization
    \Input pOutSecureKey- output buffer which must be at least sizeof(QOS_COMMON_SECURE_KEY_LENGTH) containing the secure key that successful validate the data
    \Input *pFrom       - address the probe was received from (only used for printing on failure)

    \Output
        int32_t - 0 on success

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _QosServerAuthenticateProbe(QosServerRefT *pQosServer, uint8_t *pInBuff, uint32_t uBuffLen, QosCommonProbePacketT *pOutProbe, uint8_t *pOutSecureKey, struct sockaddr *pFrom)
{
    uint8_t uRet;

    if (uBuffLen >= QOS_COMMON_MIN_PACKET_SIZE)
    {

        uint8_t aSecureKey1[QOS_COMMON_SECURE_KEY_LENGTH], aSecureKey2[QOS_COMMON_SECURE_KEY_LENGTH];

        //get the two possible secure keys we could use
        QosServiceStatus(pQosServer->pQosService, 'secc', 0, aSecureKey1, QOS_COMMON_SECURE_KEY_LENGTH);
        QosServiceStatus(pQosServer->pQosService, 'secp', 0, aSecureKey2, QOS_COMMON_SECURE_KEY_LENGTH);

        //turn the received data into a probe and validate its authenticity
        uRet = QosCommonDeserializeProbePacket(pOutProbe, pInBuff, aSecureKey1, aSecureKey2);

        //first key worked, all is good
        if (uRet == 0)
        {
            ds_memcpy(pOutSecureKey, aSecureKey1, sizeof(aSecureKey1));
        }
        //second key worked, log some metrics but everything is good
        else if (uRet == 1)
        {
            ds_memcpy(pOutSecureKey, aSecureKey2, sizeof(aSecureKey2));
            pQosServer->Metrics.ProbeValidationFailures.uFirstHmacFailed++;
            DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 2, "qosserver: [%u:%u]%A, first hmac failed, r=%u.\n", pOutProbe->uServiceRequestId, pOutProbe->uClientRequestId, pFrom, uBuffLen);
        }
        //neither key worked, log metrics and fail out
        else if (uRet == 2)
        {
            uint16_t uClientRequestId = QosCommonDeserializeClientRequestId(pInBuff);
            uint32_t uServiceRequestId = QosCommonDeserializeServiceRequestId(pInBuff);
            if (_QosServerThrottledPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [%u:%u]%A, error, second hmac failed, r=%u.\n", uServiceRequestId, uClientRequestId, pFrom, uBuffLen))
            {
                _QosServerPrintProbeData(0, pInBuff, uBuffLen);
            }
            pQosServer->Metrics.ProbeValidationFailures.uFirstHmacFailed++;
            pQosServer->Metrics.ProbeValidationFailures.uSecondHmacFailed++;
            pQosServer->Metrics.aReceivedCount[PROBE_TYPE_VALIDATION_FAILED]++;
            pQosServer->Metrics.aReceivedBytes[PROBE_TYPE_VALIDATION_FAILED] += uBuffLen;
            return(1);
        }
        else
        {
            //unexpected result, should never actually happen since all return values of QosCommonDeserializeProbePacket are currently handled
            DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 1, "qosserver: [?:?]%A, warning, unknown QosCommonDeserializeProbePacket error (d%), r=%u.\n", pFrom, uRet, uBuffLen);
            return(2);
        }
    }
    else
    {
        if (_QosServerThrottledPrintfVerbose(pQosServer->iSpamValue, 0, "qosserver: [?:?]%A, error, discarding probe packet that is too small, r=%u.\n", pFrom, uBuffLen))
        {
            _QosServerPrintProbeData(0, pInBuff, uBuffLen);
        }
        pQosServer->Metrics.ProbeValidationFailures.uRecvTooFewBytes++;
        pQosServer->Metrics.aReceivedCount[PROBE_TYPE_VALIDATION_FAILED]++;
        pQosServer->Metrics.aReceivedBytes[PROBE_TYPE_VALIDATION_FAILED] += uBuffLen;
        return(3);
    }
    //the probe is good to be used
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosServerRecvCB

    \Description
        Receive a probe.

    \Input *pSocket - pointer to module socket
    \Input iFlags   - ignored
    \Input *pData   - pointer to module state

    \Output 
        int32_t     - zero

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosServerRecvCB(SocketT *pSocket, int32_t iFlags, void *pData)
{
    QosServerRefT *pQosServer = (QosServerRefT *)pData;
    //todo it might be cleaner to have; from, iAddrLen probe, aSecureKey as members of QosServerRefT so we wont need to pass them around
    struct sockaddr_storage from;
    int32_t iAddrLen;
    uint32_t uRecvCount = 0;

    SockaddrInit((struct sockaddr*)&from, AF_INET);

    _QosServerCheckRecvStall(pQosServer);

    while ((uRecvCount < 64) && //do a max of 64 receives in a row to be sure other systems get some time
        ((pQosServer->iRecvLen = SocketRecvfrom(pQosServer->pQosSocket, (char*)(pQosServer->aRecvBuff), sizeof(pQosServer->aRecvBuff), 0, (struct sockaddr*)&from, &iAddrLen)) > 0))
    {
        QosCommonProbePacketT probe;
        uint8_t aSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];

        if (_QosServerAuthenticateProbe(pQosServer, pQosServer->aRecvBuff, pQosServer->iRecvLen, &probe, aSecureKey, (struct sockaddr*)&from) == 0)
        {
            DirtyCastLogPrintfVerbose(pQosServer->iSpamValue, 2, "qosserver: [%u:%u]%A, received (iRecvLen=%d).\n", probe.uServiceRequestId, probe.uClientRequestId, from, pQosServer->iRecvLen);
            _QosServerProcessProbe(pQosServer, &probe, aSecureKey, (struct sockaddr*)&from, iAddrLen);
        }

        uRecvCount++;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosServerGetMetricsDelta

    \Description
        Provides a metrics delta since the last time QosServerControl('mset') was called. 

    \Input *pQosServer          - pointer to module state
    \Input *pOutMetricsDeleta   - pointer to output metrics delta

    \Version 03/28/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerGetMetricsDelta(QosServerRefT *pQosServer, QosServerMetricsT *pOutMetricsDeleta)
{
    uint32_t uProbeCount, uProbeTypeCount, iSendErrorIndex;

    //copy the gauges
    pOutMetricsDeleta->uCapacityMax = pQosServer->Metrics.uCapacityMax;
    pOutMetricsDeleta->bRegisterdWithCoordinator = pQosServer->Metrics.bRegisterdWithCoordinator;

    //get delta on misc counters
    pOutMetricsDeleta->uMainTimeStalled = pQosServer->Metrics.uMainTimeStalled - pQosServer->PreviousMetrics.uMainTimeStalled;
    pOutMetricsDeleta->uRecvTimeStalled = pQosServer->Metrics.uRecvTimeStalled - pQosServer->PreviousMetrics.uRecvTimeStalled;

    //get delta on validation failures
    pOutMetricsDeleta->ProbeValidationFailures.uProtocol = pQosServer->Metrics.ProbeValidationFailures.uProtocol - pQosServer->PreviousMetrics.ProbeValidationFailures.uProtocol;
    pOutMetricsDeleta->ProbeValidationFailures.uVersion = pQosServer->Metrics.ProbeValidationFailures.uVersion - pQosServer->PreviousMetrics.ProbeValidationFailures.uVersion;
    pOutMetricsDeleta->ProbeValidationFailures.uRecvTooFewBytes = pQosServer->Metrics.ProbeValidationFailures.uRecvTooFewBytes - pQosServer->PreviousMetrics.ProbeValidationFailures.uRecvTooFewBytes;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeSizeUpTooSmall = pQosServer->Metrics.ProbeValidationFailures.uProbeSizeUpTooSmall - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeSizeUpTooSmall;
    pOutMetricsDeleta->ProbeValidationFailures.uRecvUnexpectedNumBytes = pQosServer->Metrics.ProbeValidationFailures.uRecvUnexpectedNumBytes - pQosServer->PreviousMetrics.ProbeValidationFailures.uRecvUnexpectedNumBytes;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeSizeDownTooSmall = pQosServer->Metrics.ProbeValidationFailures.uProbeSizeDownTooSmall - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeSizeUpTooSmall;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeSizeUpTooLarge = pQosServer->Metrics.ProbeValidationFailures.uProbeSizeUpTooLarge - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeSizeUpTooLarge;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeSizeDownTooLarge = pQosServer->Metrics.ProbeValidationFailures.uProbeSizeDownTooLarge - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeSizeDownTooLarge;
    pOutMetricsDeleta->ProbeValidationFailures.uUnexpectedExternalAddress = pQosServer->Metrics.ProbeValidationFailures.uUnexpectedExternalAddress - pQosServer->PreviousMetrics.ProbeValidationFailures.uUnexpectedExternalAddress;
    pOutMetricsDeleta->ProbeValidationFailures.uUnexpectedServerReceiveTime = pQosServer->Metrics.ProbeValidationFailures.uUnexpectedServerReceiveTime - pQosServer->PreviousMetrics.ProbeValidationFailures.uUnexpectedServerReceiveTime;
    pOutMetricsDeleta->ProbeValidationFailures.uUnexpectedServerSendDelta = pQosServer->Metrics.ProbeValidationFailures.uUnexpectedServerSendDelta - pQosServer->PreviousMetrics.ProbeValidationFailures.uUnexpectedServerSendDelta;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeCountDownTooHigh = pQosServer->Metrics.ProbeValidationFailures.uProbeCountDownTooHigh - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeCountDownTooHigh;
    pOutMetricsDeleta->ProbeValidationFailures.uProbeCountUpTooHigh = pQosServer->Metrics.ProbeValidationFailures.uProbeCountUpTooHigh - pQosServer->PreviousMetrics.ProbeValidationFailures.uProbeCountUpTooHigh;
    pOutMetricsDeleta->ProbeValidationFailures.uFirstHmacFailed = pQosServer->Metrics.ProbeValidationFailures.uFirstHmacFailed - pQosServer->PreviousMetrics.ProbeValidationFailures.uFirstHmacFailed;
    pOutMetricsDeleta->ProbeValidationFailures.uSecondHmacFailed = pQosServer->Metrics.ProbeValidationFailures.uSecondHmacFailed - pQosServer->PreviousMetrics.ProbeValidationFailures.uSecondHmacFailed;
    pOutMetricsDeleta->ProbeValidationFailures.uServiceRequestIdTooOld = pQosServer->Metrics.ProbeValidationFailures.uServiceRequestIdTooOld - pQosServer->PreviousMetrics.ProbeValidationFailures.uServiceRequestIdTooOld;
    pOutMetricsDeleta->ProbeValidationFailures.uFromPort = pQosServer->Metrics.ProbeValidationFailures.uFromPort - pQosServer->PreviousMetrics.ProbeValidationFailures.uFromPort;
    pOutMetricsDeleta->ProbeValidationFailures.uExpectedProbeCountUpTooHigh = pQosServer->Metrics.ProbeValidationFailures.uExpectedProbeCountUpTooHigh - pQosServer->PreviousMetrics.ProbeValidationFailures.uExpectedProbeCountUpTooHigh;

    //get delta on probes sent distributions
    for (uProbeCount = 0; uProbeCount < QOS_COMMON_MAX_PROBE_COUNT; uProbeCount++)
    {
        for (uProbeTypeCount = 0; uProbeTypeCount < PROBE_TYPE_COUNT; uProbeTypeCount++)
        {
            pOutMetricsDeleta->aSendPacketCountDistribution[uProbeTypeCount][uProbeCount] = pQosServer->Metrics.aSendPacketCountDistribution[uProbeTypeCount][uProbeCount] - pQosServer->PreviousMetrics.aSendPacketCountDistribution[uProbeTypeCount][uProbeCount];
        }
    }

    //get delta on counts
    for (uProbeTypeCount = 0; uProbeTypeCount < PROBE_TYPE_COUNT; uProbeTypeCount++)
    {
        pOutMetricsDeleta->aSendCountSuccess[uProbeTypeCount] = pQosServer->Metrics.aSendCountSuccess[uProbeTypeCount] - pQosServer->PreviousMetrics.aSendCountSuccess[uProbeTypeCount];
        pOutMetricsDeleta->aSendCountFailed[uProbeTypeCount] = pQosServer->Metrics.aSendCountFailed[uProbeTypeCount] - pQosServer->PreviousMetrics.aSendCountFailed[uProbeTypeCount];
        pOutMetricsDeleta->aSentBytes[uProbeTypeCount] = pQosServer->Metrics.aSentBytes[uProbeTypeCount] - pQosServer->PreviousMetrics.aSentBytes[uProbeTypeCount];
        pOutMetricsDeleta->aReceivedCount[uProbeTypeCount] = pQosServer->Metrics.aReceivedCount[uProbeTypeCount] - pQosServer->PreviousMetrics.aReceivedCount[uProbeTypeCount];
        pOutMetricsDeleta->aReceivedBytes[uProbeTypeCount] = pQosServer->Metrics.aReceivedBytes[uProbeTypeCount] - pQosServer->PreviousMetrics.aReceivedBytes[uProbeTypeCount];
        pOutMetricsDeleta->aArtificialLatency[uProbeTypeCount] = pQosServer->Metrics.aArtificialLatency[uProbeTypeCount] - pQosServer->PreviousMetrics.aArtificialLatency[uProbeTypeCount];
        pOutMetricsDeleta->aArtificialLatencyCount[uProbeTypeCount] = pQosServer->Metrics.aArtificialLatencyCount[uProbeTypeCount] - pQosServer->PreviousMetrics.aArtificialLatencyCount[uProbeTypeCount];
        pOutMetricsDeleta->aExtraProbes[uProbeTypeCount] = pQosServer->Metrics.aExtraProbes[uProbeTypeCount] - pQosServer->PreviousMetrics.aExtraProbes[uProbeTypeCount];
        
        //note that load are gauges so we don't calculate deltas for them
        pOutMetricsDeleta->aPastSecondLoad[uProbeTypeCount] = pQosServer->Metrics.aPastSecondLoad[uProbeTypeCount];
    }

    //deltas for send error code counts
    for (iSendErrorIndex = 0; iSendErrorIndex < QOS_SERVER_MAX_SEND_ERROR_CODES_MONITORED; iSendErrorIndex++)
    {
        pOutMetricsDeleta->aProbeSendErrorCodeCounts[iSendErrorIndex].iErrorCode = pQosServer->Metrics.aProbeSendErrorCodeCounts[iSendErrorIndex].iErrorCode;
        pOutMetricsDeleta->aProbeSendErrorCodeCounts[iSendErrorIndex].uCount = pQosServer->Metrics.aProbeSendErrorCodeCounts[iSendErrorIndex].uCount - pQosServer->PreviousMetrics.aProbeSendErrorCodeCounts[iSendErrorIndex].uCount;
    }
}

/*F********************************************************************************/
/*!
    \Function _QosServerUpdateMonitor

    \Description
        Update the warnings/errors based on the configured thresholds

    \Input *pQosServer  - module state
    \Input  uCurTick        - current tick for this frame

    \Version 06/09/2017 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServerUpdateMonitor(QosServerRefT *pQosServer, uint32_t uCurTick)
{
    uint32_t uValue, uMonitorFlagWarnings, uMonitorFlagErrors;
    uint8_t bClearOnly;
    VoipServerStatT VoipServerStats;
    UdpMetricsT UdpMetrics;

    const VoipServerConfigT *pConfig = VoipServerGetConfig(pQosServer->pBase);
    const ServerMonitorConfigT *pWrn = &pConfig->MonitorWarnings;
    const ServerMonitorConfigT *pErr = &pConfig->MonitorErrors;
    const uint32_t uPrevFlagWarnings = (uint32_t)VoipServerStatus(pQosServer->pBase, 'mwrn', 0, NULL, 0);
    const uint32_t uPrevFlagErrors = (uint32_t)VoipServerStatus(pQosServer->pBase, 'merr', 0, NULL, 0);

    // get voip server stats
    VoipServerStatus(pQosServer->pBase, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));
    VoipServerStatus(pQosServer->pBase, 'udpm', 0, &UdpMetrics, sizeof(UdpMetrics));

    // check average latency, if system load is 1/2 of warning threshold or greater
    bClearOnly = (VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]) < (pWrn->uAvgSystemLoad/2)) ? TRUE : FALSE;
    uValue = (VoipServerStats.StatInfo.E2eLate.uNumStat != 0) ? VoipServerStats.StatInfo.E2eLate.uSumLate / VoipServerStats.StatInfo.E2eLate.uNumStat : 0;
    VoipServerUpdateMonitorFlag(pQosServer->pBase, uValue > pWrn->uAvgClientLate, uValue > pErr->uAvgClientLate, 1 << 2, bClearOnly);

    // check pct server load
    uValue = (uint32_t)VoipServerStats.ServerInfo.fCpu;
    VoipServerUpdateMonitorFlag(pQosServer->pBase, uValue > pWrn->uPctServerLoad, uValue > pErr->uPctServerLoad, 1 << 3, FALSE);

    // check pct system load
    uValue = VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]);
    VoipServerUpdateMonitorFlag(pQosServer->pBase, uValue > pWrn->uAvgSystemLoad, uValue > pErr->uAvgSystemLoad, 1 << 4, FALSE);

    // check the UDP stats
    VoipServerUpdateMonitorFlag(pQosServer->pBase, UdpMetrics.uRecvQueueLen > pWrn->uUdpRecvQueueLen, UdpMetrics.uRecvQueueLen > pErr->uUdpRecvQueueLen, VOIPSERVER_MONITORFLAG_UDPRECVQULEN, FALSE);
    VoipServerUpdateMonitorFlag(pQosServer->pBase, UdpMetrics.uSendQueueLen > pWrn->uUdpSendQueueLen, UdpMetrics.uSendQueueLen > pErr->uUdpSendQueueLen, VOIPSERVER_MONITORFLAG_UDPSENDQULEN, FALSE);

    // debug output if monitor state was changed
    uMonitorFlagWarnings = (uint32_t)VoipServerStatus(pQosServer->pBase, 'mwrn', 0, NULL, 0);
    uMonitorFlagErrors = (uint32_t)VoipServerStatus(pQosServer->pBase, 'merr', 0, NULL, 0);

    if (uMonitorFlagWarnings != uPrevFlagWarnings)
    {
        DirtyCastLogPrintf("qosserver: monitor warning state change: 0x%08x->0x%08x\n", uPrevFlagWarnings, uMonitorFlagWarnings);
    }
    if (uMonitorFlagErrors != uPrevFlagErrors)
    {
        DirtyCastLogPrintf("qosserver: monitor error state change: 0x%08x->0x%08x\n", uPrevFlagErrors, uMonitorFlagErrors);
    }
}



/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function QosServerInitialize

    \Description
        Allocates the state for the mode and does any additional setup or
        configuration needed

    \Input *pVoipServer     - pointer to the base module (shared data / functionality)
    \Input *pCommandTags    - configuration data (from command line)
    \Input *pConfigTags     - configuration data (from configuration file)

    \Output
        QosServerRefT *     - pointer to the mode state

    \Version 09/24/2016 (cvienneau)
*/
/********************************************************************************F*/
QosServerRefT *QosServerInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags)
{
    char strLocalHostname[256];
    uint32_t uLocalPort = 0;
    QosServerRefT *pQosServer;
    int32_t iMemGroup;
    void *pMemGroupUserdata;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    TokenApiRefT *pTokenApi = VoipServerGetTokenApi(pVoipServer);
    QosCommonServerToCoordinatorRequestT serverRegistartionInfo;

    // get allocation information
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // ensure that we have a valid ping site for registration
    if (pConfig->strPingSite[0] == '\0')
    {
        DirtyCastLogPrintf("qosserver: error, ping site is required to register with the coordinator. please add a ping site to the configuration\n");
        return(NULL);
    }

    // allocate module state
    if ((pQosServer = (QosServerRefT *)DirtyMemAlloc(sizeof(QosServerRefT), QOS_SERVER_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("qosserver: error, unable to allocate qos mode module\n");
        return(NULL);
    }
    ds_memclr(pQosServer, sizeof(QosServerRefT));
    pQosServer->pBase = pVoipServer;
    pQosServer->iMemGroup = iMemGroup;
    pQosServer->pMemGroupUserdata = pMemGroupUserdata;
    pQosServer->uTimeLastLoad = NetTick();

    // create the socket
    if ((pQosServer->pQosSocket = VoipServerSocketOpen(SOCK_DGRAM, 0, pConfig->uApiPort, FALSE, &_QosServerRecvCB, pQosServer)) == NULL)
    {
        DirtyCastLogPrintf(("qosserver: error, opening listening socket.\n"));
        QosServerShutdown(pQosServer);
        return(NULL);
    }

    // allocate the QOS service module, for communicating to the coordinator
    if ((pQosServer->pQosService = QosServiceCreate(pTokenApi, pConfig->strCertFilename, pConfig->strKeyFilename)) == NULL)
    {
        DirtyCastLogPrintf("qosserver: error, unable to create the qosservice module\n");
        QosServerShutdown(pQosServer);
        return(NULL);
    }

    // setup debug log levels
    QosServerControl(pQosServer, 'spam', pConfig->uDebugLevel, 0, NULL);
    
    // who is the coordinator
    QosServiceControl(pQosServer->pQosService, 'qcad', 0, 0, (void*)pConfig->strCoordinatorAddr);

    // use the override secure key, if one was provided
    if (pConfig->strOverrideSiteKey[0] != '\0')
    {
        uint8_t aOverrideSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];
        ds_memclr(aOverrideSecureKey, sizeof(aOverrideSecureKey));
        ds_snzprintf((char*)aOverrideSecureKey, sizeof(aOverrideSecureKey), "%s", pConfig->strOverrideSiteKey);
        QosServiceControl(pQosServer->pQosService, 'osky', 0, 0, aOverrideSecureKey);
    }

    // use hostname specified in config, otherwise query from system
    if (pConfig->strHostName[0] != '\0')
    {
        ds_strnzcpy(strLocalHostname, pConfig->strHostName, sizeof(strLocalHostname));
    }
    else
    {
        VoipServerStatus(pVoipServer, 'host', 0, strLocalHostname, sizeof(strLocalHostname));
    }

    // set the currently configured local port
    uLocalPort = pConfig->uApiPort;

    // if subspace has given us a different host name/port to use, override with that
    if (pConfig->uSubspaceLocalPort != 0)
    {
        DirtyCastLogPrintf("qosserver: overriding qos server address %s:%d with address from subspace %s:%d.\n", 
            strLocalHostname, uLocalPort, pConfig->strSubspaceLocalAddr, pConfig->uSubspaceLocalPort);
        uLocalPort = pConfig->uSubspaceLocalPort;
        ds_strnzcpy(strLocalHostname, pConfig->strSubspaceLocalAddr, sizeof(strLocalHostname));
    }

    // setup our initial registration request
    ds_memclr(&serverRegistartionInfo, sizeof(serverRegistartionInfo));
    ds_strnzcpy(serverRegistartionInfo.strSiteName, pConfig->strPingSite, sizeof(serverRegistartionInfo.strSiteName));
    ds_strnzcpy(serverRegistartionInfo.strPool, pConfig->strPool, sizeof(serverRegistartionInfo.strPool));
    ds_strnzcpy(serverRegistartionInfo.strAddr, strLocalHostname, sizeof(serverRegistartionInfo.strAddr));
    serverRegistartionInfo.uCapacityPerSec = pQosServer->Metrics.uCapacityMax = pConfig->iMaxClients;
    serverRegistartionInfo.uPort = uLocalPort;
    serverRegistartionInfo.uProbeVersion = QOS_COMMON_PROBE_VERSION;
    serverRegistartionInfo.bShuttingDown = FALSE;

    // register ourselves
    if (QosServiceRegister(pQosServer->pQosService, &serverRegistartionInfo) != 0)
    {
        DirtyCastLogPrintf("qosserver: error, unable to start registration with the QosCoordinator.\n");
        QosServerShutdown(pQosServer);
        return(NULL);
    }

    return(pQosServer);
}

/*F********************************************************************************/
/*!
    \Function QosServerProcess

    \Description
        Process loop for QOS mode of the voip server

    \Input *pQosServer      - mode state
    \Input *pSignalFlags    - VOIPSERVER_SIGNAL_* flags passing signal state to process.
    \Input uCurTick         - current tick for this frame

    \Output
        uint8_t             - TRUE to continue processing, FALSE to exit

    \Version 09/24/2016 (cvienneau)
*/
/********************************************************************************F*/
uint8_t QosServerProcess(QosServerRefT *pQosServer, uint32_t *pSignalFlags, uint32_t uCurTick)
{
    _QosServerCheckMainStall(pQosServer);

    // calculate how busy we've been
    if (NetTickDiff(NetTick(), pQosServer->uTimeLastLoad) >= 1000)
    {
        uint32_t uProbeTypeCount, uCurrentLoad;
        pQosServer->uTimeLastLoad = NetTick();
        for (uProbeTypeCount = 0; uProbeTypeCount < PROBE_TYPE_COUNT; uProbeTypeCount++)
        {
            pQosServer->Metrics.aPastSecondLoad[uProbeTypeCount] = pQosServer->aCurrentSecondLoad[uProbeTypeCount];
            pQosServer->aCurrentSecondLoad[uProbeTypeCount] = 0;
        }

        //report load on latency, bw_up and bw_down; though we gather a metric on load of unknown probes its kind of meaningless for the coordinator
        uCurrentLoad = pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_LATENCY] + pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_BW_UP] + pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_BW_DOWN];
        QosServiceControl(pQosServer->pQosService, 'load', uCurrentLoad, 0, NULL);
    }

    // check signal flags
    if (*pSignalFlags & VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY)
    {
        // set once when we get the signal
        if (pQosServer->uShutdownStartTimer == 0)
        {
            DirtyCastLogPrintf("qosserver: starting if empty shutdown.\n");
            pQosServer->uShutdownStartTimer = NetTick();
            // tell the coordinator we are shutting down (don't send us anymore clients)
            QosServiceControl(pQosServer->pQosService, 'shut', 0, 0, NULL);
        }

        if (NetTickDiff(NetTick(), pQosServer->uShutdownStartTimer) >= QOS_SERVER_DRAIN_TIME)
        {
            DirtyCastLogPrintf("qosserver: executing if empty shutdown.\n");
            return(FALSE);
        }
    }

    // heartbeat to the coordinator
    QosServiceUpdate(pQosServer->pQosService);

    // check to see if we have a connection to the coordinator
    pQosServer->Metrics.bRegisterdWithCoordinator = QosServiceStatus(pQosServer->pQosService, 'rcoo', 0, NULL, 0);

    // update monitor flags
    _QosServerUpdateMonitor(pQosServer, uCurTick);

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function QosServerStatus

    \Description
        Get module status

    \Input *pQosServer  - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'done'  - return if the module has finished its time critical operations
            'load'  - return the current load we are under
            'mtrc'  - retrieve a copy of the QosServerMetricsT struct
            'mdlt'  - retrieve a delta of metrics since the last time 'mdlt' was called
            'read'  - return TRUE if kubernetes readiness probe can succeed, FALSE otherwise
        \endverbatim

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosServerStatus(QosServerRefT *pQosServer, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iStatus == 'done')
    {
        uint8_t bDone;
        TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pQosServer->pBase);

        // we need to finish all nucleus operations or other RSA operations in a timely manner
        bDone  = TokenApiStatus(pTokenRef, 'done', NULL, 0);
        bDone &= QosServiceStatus(pQosServer->pQosService, 'done', 0, NULL, 0);

        return(bDone);
    }
    if (iStatus == 'load')
    {
        return (pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_LATENCY] +
            pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_BW_UP] +
            pQosServer->Metrics.aPastSecondLoad[PROBE_TYPE_BW_DOWN]);
    }
    if (iStatus == 'mtrc')
    {
        if (iBufSize >= (int32_t)sizeof(QosServerMetricsT))
        {
            ds_memcpy(pBuf, &(pQosServer->Metrics), sizeof(QosServerMetricsT));
            return(0);
        }
        else
        {
            return(-1);
        }
    }
    if (iStatus == 'mdlt')
    {
        if (iBufSize >= (int32_t)sizeof(QosServerMetricsT))
        {
            QosServerMetricsT MetricsDelta;
            _QosServerGetMetricsDelta(pQosServer, &MetricsDelta);
            ds_memcpy(pBuf, &MetricsDelta, sizeof(QosServerMetricsT));
            return(0);
        }
        else
        {
            return(-1);
        }
    }    
    if (iStatus == 'read')
    {
        return(TRUE);
    }
 
    // selector unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosServerControl

    \Description
        Module control function

    \Input *pQosServer     - pointer to module state
    \Input iControl            - control selector
    \Input iValue              - selector specific
    \Input iValue2             - selector specific
    \Input *pValue             - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'mset'  - set a point in time when the metrics delta will be based on
            'spam' - update debug level via iValue

        \endverbatim

    \Version 12/09/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosServerControl(QosServerRefT *pQosServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'mset')
    {
        //save the state of the current metrics as a snapshot in time 
        pQosServer->PreviousMetrics = pQosServer->Metrics;
        return(0);
    }
    if (iControl == 'spam')
    {
        pQosServer->iSpamValue = iValue;
        QosServiceControl(pQosServer->pQosService, iControl, iValue, iValue2, pValue);
        return(0);
    }

    // control unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosServerDrain

    \Description
        Cleans up the mode state

    \Input *pQosServer  - mode state

    \Output
        uint32_t        - TRUE if drain complete, FALSE otherwise

    \Version 01/07/2020 (mclouatre)
*/
/********************************************************************************F*/
uint32_t QosServerDrain(QosServerRefT *pQosServer)
{
    if (pQosServer->uShutdownStartTimer == 0)
    {
        DirtyCastLogPrintf("qosserver: starting drain.\n");
        pQosServer->uShutdownStartTimer = NetTick();
        // tell the coordinator we are shutting down (don't send us anymore clients)
        QosServiceControl(pQosServer->pQosService, 'shut', 0, 0, NULL);
    }

    if (NetTickDiff(NetTick(), pQosServer->uShutdownStartTimer) >= QOS_SERVER_DRAIN_TIME)
    {
        DirtyCastLogPrintf("qosserver: drain completed.\n");
        return(TRUE);
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function QosServerShutdown

    \Description
        Cleans up the mode state

    \Input *pQosServer   - mode state

    \Version 09/24/2016 (cvienneau)
*/
/********************************************************************************F*/
void QosServerShutdown(QosServerRefT *pQosServer)
{
    // destroy qosservice module
    if (pQosServer->pQosService != NULL)
    {
        QosServiceDestroy(pQosServer->pQosService);
        pQosServer->pQosService = NULL;
    }

    // cleanup module state
    DirtyMemFree(pQosServer, QOS_SERVER_MEMID, pQosServer->iMemGroup, pQosServer->pMemGroupUserdata);
}

/*F********************************************************************************/
/*!
    \Function QosServerGetBase

    \Description
        Gets the base module for getting shared functionality

    \Input *pQosServer   - module state

    \Output
        VoipServerRefT * - module state to our shared functionality between modes

    \Version 12/02/2016 (cvienneau)
*/
/********************************************************************************F*/
VoipServerRefT *QosServerGetBase(QosServerRefT *pQosServer)
{
    return(pQosServer->pBase);
}
