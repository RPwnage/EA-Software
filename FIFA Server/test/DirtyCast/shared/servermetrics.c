/*H********************************************************************************/
/*!
    \File servermetrics.c

    \Description
        Module reporting VS metrics to external metrics aggregator.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/01/2016 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>     // for memset()

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "dirtycast.h"
#include "servermetrics.h"

/*** Defines **********************************************************************/

//! module memid  -  vsst --> VoipServer STats
#define SERVERMETRICS_MEMID         ('vsst')

//! datagram size
#define SERVERMETRICS_DATAGRAM_SIZE (1024)

//! initial back buffer size
#define SERVERMETRICS_INIT_BACK_BUFFER_SIZE (1024)

//! default retry rate for metrics push
#define SERVERMETRICS_RETRY_RATE    (60*1000)


/*** Type Definitions *************************************************************/

// module state
struct ServerMetricsT
{
    //! module memory goup
    int32_t iMemGroup;
    void *pMemGroupUserData;

    uint32_t        uNextReportTime;                //!< tick of next time we report
    uint32_t        uLastConnectTime;               //!< tick of last time we attempted connection
    int32_t         iMetricsPushingRate;            //!< rate at which we push metrics to external metrics aggregator
    char            strMetricsAggregatorHost[256];  //!< address for the external metrics aggregator
    struct sockaddr MetricsAggregatorAddr;          //!< socket address for the external metrics aggregator
    ServerMetricsFormatE eFormat;                   //!< format to be used when building the metrics report

    ServerMetricsCallbackT *pCallback;
    void *pUserData;

    SocketT         *pMetricsAggregatorSock;        //!< socket to external metrics aggregator host
    HostentT        *pMetricsAggregatorHostent;     //!< hostname entry for external metrics aggregator

    int32_t         iBufLen;
    uint16_t        uMetricsAggregatorPort;         //!< port the external metrics aggregator is listening on
    uint8_t         bFailed;                        //!< TRUE if module entered failed state, FALSE otherwise
    int8_t          iVerbose;                       //!< debug output verbosity
    char           *pScratchPad;                    //!< temporary buffer used to generate substring 
    int32_t         iScratchPadLen;                 //!< length of the scratchpad

    uint32_t        uDatagramCount;                 //!< number of data sent (per back buffer)
    int32_t         iBackBufferLen;                 //!< current back buffer size
    int32_t         iBackBufferSendOffset;          //!< current back buffer send offset
    int32_t         iMaxBackBufferLen;              //!< current max back buffer size
    char            *pBackbuffer;                   //!< back buffer
};

/*** Variables ********************************************************************/


/*** Private Functions ************************************************************/
/*F********************************************************************************/
/*!
    \Function _ServerMetricsBackBufferAlloc

    \Description
        Alloc (or re-alloc) back buffer

    \Input *pServerMetrics  - module state.
    \Input iSize            - size of buffer

    \Output
        int32_t             - negative=error, zero=success

    \Version 04/30/2018 (tcho)
*/
/********************************************************************************F*/
static int32_t _ServerMetricsBackBufferAlloc(ServerMetricsT *pServerMetrics, int32_t iSize)
{
    char* pNewBuffer;

    if (iSize < pServerMetrics->iMaxBackBufferLen)
    {
        DirtyCastLogPrintf("servermetrics: unable to reallocate back buffer with size %d which is smaller than the current size %d\n", iSize, pServerMetrics->iMaxBackBufferLen);
        return(-1);
    }

    if ((pNewBuffer = (char*)DirtyMemAlloc(iSize, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("servermetrics: unable to allocate or reallocate back buffer with size %d\n", iSize);
        return(-2);
    }
    
    ds_memclr(pNewBuffer, iSize);

    // check if we have stuff to copy
    if (pServerMetrics->pBackbuffer != NULL)
    {
        // only copy if there is actually stuff to copy
        if (pServerMetrics->iBackBufferLen > 0)
        {
            ds_memcpy_s(pNewBuffer, iSize, pServerMetrics->pBackbuffer, pServerMetrics->iBackBufferLen);
        }

        DirtyMemFree(pServerMetrics->pBackbuffer, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
    }

    DirtyCastLogPrintf("servermetrics: expanding back buffer from %d bytes to %d bytes (old buffer = %p, new buffer = %p)\n", pServerMetrics->iMaxBackBufferLen, iSize, pServerMetrics->pBackbuffer, pNewBuffer);

    // replace existing buffer
    pServerMetrics->pBackbuffer = pNewBuffer;
    pServerMetrics->iMaxBackBufferLen = iSize;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ServerMetricsSend

    \Description
        Send next datagram to the aggregator 
   
    \Input *pServerMetrics  - module state.

    \Version 04/30/2018 (tcho)
*/
/********************************************************************************F*/
static void _ServerMetricsSend(ServerMetricsT *pServerMetrics)
{
    int32_t iResult;
    int32_t iIndex = 0;
    int32_t iSize = 0;
    char* pBuffer = pServerMetrics->pBackbuffer + pServerMetrics->iBackBufferSendOffset;

    // skip if back buffer is empty
    if (pServerMetrics->iBackBufferLen == 0)
    {
        return;
    }

    // keep looking for metric entries until next entry does not fit in datagram, or end of back buffer is hit
    while ( (iIndex < SERVERMETRICS_DATAGRAM_SIZE) &&
            ((pBuffer + iIndex) < (pServerMetrics->pBackbuffer + pServerMetrics->iMaxBackBufferLen)) )
    {
        if (*(pBuffer + iIndex) == '\n')
        {
            iSize = iIndex + 1;
        }
        ++iIndex;
    }

    if (iSize == 0)
    {
        DirtyCastLogPrintf("servermetrics: ERROR - aborting sending of metrics report at datagram #%d because EOL can't be found  (back buffer --> max: %d  offset: %d)\n", 
            pServerMetrics->uDatagramCount, pServerMetrics->iMaxBackBufferLen, pServerMetrics->iBackBufferSendOffset);

        pServerMetrics->uDatagramCount = 0;
        pServerMetrics->iBackBufferSendOffset = 0;
        pServerMetrics->iBackBufferLen = 0;
        ds_memclr(pServerMetrics->pBackbuffer, pServerMetrics->iMaxBackBufferLen);

        return;
    }

    if (pServerMetrics->iVerbose > 2)
    {
        static char strDatagram[SERVERMETRICS_DATAGRAM_SIZE + 1];
        ds_memcpy(strDatagram, pBuffer, iSize);
        strDatagram[iSize] = '\0';
        DirtyCastLogPrintfVerbose(pServerMetrics->iVerbose, 2, "servermetrics: attempt to push metrics datagram #%d to metrics aggregator - content (size: %d bytes)\n%s", pServerMetrics->uDatagramCount, iSize, strDatagram);
    }

    // try to send datagram
    if ((iResult = SocketSendto(pServerMetrics->pMetricsAggregatorSock, pBuffer, iSize, 0, &pServerMetrics->MetricsAggregatorAddr, sizeof(pServerMetrics->MetricsAggregatorAddr))) < 0)
    {
        DirtyCastLogPrintf("servermetrics: sending metrics datagram #%d to the metrics aggregator failed (error=%d/%d)\n", pServerMetrics->uDatagramCount, iResult, DirtyCastGetLastError());
        pServerMetrics->bFailed = TRUE;
    }
    else
    {
        // update variables tracking send process
        pServerMetrics->uDatagramCount++;
        pServerMetrics->iBackBufferSendOffset += iSize;

        // if send complete, then reset variables accordingly
        if (pServerMetrics->iBackBufferSendOffset >= pServerMetrics->iBackBufferLen)
        {
            if (pServerMetrics->iBackBufferSendOffset > pServerMetrics->iBackBufferLen)
            {
                DirtyCastLogPrintf("servermetrics: WARNING - sending of metrics report at datagram #%d incorrectly ended with offset (%d) larger than length (%d)\n",
                    pServerMetrics->uDatagramCount, pServerMetrics->iBackBufferSendOffset, pServerMetrics->iBackBufferLen);
            }

            pServerMetrics->uDatagramCount = 0;
            pServerMetrics->iBackBufferSendOffset = 0;
            pServerMetrics->iBackBufferLen = 0;
            ds_memclr(pServerMetrics->pBackbuffer, pServerMetrics->iMaxBackBufferLen);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ServerMetricsReset

    \Description
        Go back to a state from which init stat can be re-entered

    \Input *pServerMetrics  - module state.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static void _ServerMetricsReset(ServerMetricsT *pServerMetrics)
{
    // destroy hostname lookup if we have one in progress
    if (pServerMetrics->pMetricsAggregatorHostent != NULL)
    {
        pServerMetrics->pMetricsAggregatorHostent->Free(pServerMetrics->pMetricsAggregatorHostent);
        pServerMetrics->pMetricsAggregatorHostent = NULL;
    }

    // destroy metrics aggregator socket
    if (pServerMetrics->pMetricsAggregatorSock != NULL)
    {
        SocketClose(pServerMetrics->pMetricsAggregatorSock);
        pServerMetrics->pMetricsAggregatorSock = NULL;
    }

    pServerMetrics->bFailed = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ServerMetricsInitUpdate

    \Description
        Perform init phase of servermetrics module

    \Input *pServerMetrics  - module state.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static void _ServerMetricsInitUpdate(ServerMetricsT *pServerMetrics)
{
    // is address lookup started?
    if (pServerMetrics->pMetricsAggregatorHostent == NULL)
    {
        pServerMetrics->uLastConnectTime = pServerMetrics->uNextReportTime = NetTick();
        if ((pServerMetrics->pMetricsAggregatorHostent = SocketLookup(pServerMetrics->strMetricsAggregatorHost, 30 * 1000)) == NULL)
        {
            DirtyCastLogPrintf("servermetrics: cannot start address lookup for metrics aggregator host (%s)\n", pServerMetrics->strMetricsAggregatorHost);

            pServerMetrics->bFailed = TRUE;
        }

        DirtyCastLogPrintf("servermetrics: metrics aggregator host (%s) address lookup started\n", pServerMetrics->strMetricsAggregatorHost);
    }
    else
    {
        // check if we are done resolving the hostname
        if (pServerMetrics->pMetricsAggregatorHostent->Done(pServerMetrics->pMetricsAggregatorHostent) == FALSE)
        {
            return;
        }

        // update the address from the lookup
        SockaddrInit(&pServerMetrics->MetricsAggregatorAddr, AF_INET);
        SockaddrInSetAddr(&pServerMetrics->MetricsAggregatorAddr, pServerMetrics->pMetricsAggregatorHostent->addr);
        SockaddrInSetPort(&pServerMetrics->MetricsAggregatorAddr, pServerMetrics->uMetricsAggregatorPort);

        // free the lookup info before try to connect
        pServerMetrics->pMetricsAggregatorHostent->Free(pServerMetrics->pMetricsAggregatorHostent);
        pServerMetrics->pMetricsAggregatorHostent = NULL;

        // check if we received a valid address back
        if (SockaddrInGetAddr(&pServerMetrics->MetricsAggregatorAddr) != 0)
        {
            DirtyCastLogPrintf("servermetrics: metrics aggregator host (%s) address lookup succeeded: %a\n",
                pServerMetrics->strMetricsAggregatorHost, SockaddrInGetAddr(&pServerMetrics->MetricsAggregatorAddr));
        }
        else
        {
            DirtyCastLogPrintf("servermetrics: lookup of metrics aggregator address failed\n");
            pServerMetrics->bFailed = TRUE;
        }

        if ((pServerMetrics->pMetricsAggregatorSock = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == NULL)
        {
            DirtyCastLogPrintf("servermetrics: cannot open socket for pushing metrics to metrics aggregator\n");
            pServerMetrics->bFailed = TRUE;
        }
        // disable the async receive thread, we will not be receiving any data
        SocketControl(pServerMetrics->pMetricsAggregatorSock, 'arcv', FALSE, NULL, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ServerMetricsReport

    \Description
        Fills the back buffer with metrics

    \Input *pServerMetrics  - module state.

    \Version 07/07/2017 (eesponda)
*/
/********************************************************************************F*/
static void _ServerMetricsReport(ServerMetricsT *pServerMetrics)
{
    uint32_t uMetricIndex = 1;
    int32_t iRemainSize = pServerMetrics->iMaxBackBufferLen;
    int32_t iSize = pServerMetrics->iMaxBackBufferLen;

    // if the back buffer is not empty skip this reporting cycle
    // expect reporting interval to be far greater than the time it takes to service the back buffer
    if (pServerMetrics->iBackBufferLen != 0)
    {
        return;
    }

    // make sure we have a valid socket
    if (pServerMetrics->pMetricsAggregatorSock == NULL)
    {
        DirtyCastLogPrintf("servermetrics: attempting to report using an invalid socket\n");
        return;
    }

    // keep looping until the callback returns "nothing left"
    while (uMetricIndex != 0)
    {
        // call user callback to format stats data
        uMetricIndex = pServerMetrics->pCallback(pServerMetrics, pServerMetrics->pBackbuffer + pServerMetrics->iBackBufferLen, &(iSize), uMetricIndex, pServerMetrics->pUserData);

        // update back buffer length
        pServerMetrics->iBackBufferLen += iSize;

        // note: iSize in is the remaining size of the back buffer
        //       iSize out is the number of bytes written to the back buffer
        // if no bytes are written but we still have metrics to send expand the back buffer by doubling the size
        if ((iSize == 0) && (uMetricIndex != 0))
        {
            if (_ServerMetricsBackBufferAlloc(pServerMetrics, 2 * pServerMetrics->iMaxBackBufferLen) < 0)
            {
                DirtyCastLogPrintf("servermetrics: cannot expand back buffer! metrics report will be truncated\n");
                break;
            }

            // update back buffer and remaining size
            iRemainSize += (pServerMetrics->iMaxBackBufferLen/2);
        }

        // update remining size
        iRemainSize -= iSize;
        iSize = iRemainSize;
    }
}

/*F********************************************************************************/
/*!
    \Function _ServerMetricsReportUpdate

    \Description
        Perform reporting phase of servermetrics module

    \Input *pServerMetrics  - module state.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static void _ServerMetricsReportUpdate(ServerMetricsT *pServerMetrics)
{
    // make sure all datagrams from previous report are sent
    if (pServerMetrics->uDatagramCount == 0)
    {
        // check if it is time to report to the metrics aggregator
        if (NetTickDiff(NetTick(), pServerMetrics->uNextReportTime) > 0)
        {
            _ServerMetricsReport(pServerMetrics);
            pServerMetrics->uNextReportTime = NetTick() + pServerMetrics->iMetricsPushingRate;
        }
    }

    _ServerMetricsSend(pServerMetrics);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsFormatDatadog

    \Description
        Add metric to the specified buffer using datadog formatting.

    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer
    \Input *pMetricName     - pointer to metric name
    \Input *pMetricValue    - metric value
    \Input eMetricType      - metric type
    \Input uSampleRate      - percentage of the metric samples that are being sent to the aggregator  (ignored for now because not neeeded)
    \Input *pDimensionsStr  - pointer to string containing a sequence of dimensions to be associated with the metric (expected format "dim1:val1,dim2,dim3:val3"). Can be NULL.

    \Output
        int32_t             - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ServerMetricsFormatDatadog(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, const char *pMetricValue, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr)
{
    int32_t iOffset = 0;

    // bzero user buffer content
    ds_memclr(pBuffer, iBufSize);

    // add metric name 
    if ((int32_t)strlen(pMetricName) < (iBufSize - iOffset))
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%s", pMetricName);
    }
    else
    {
        // bzero buffer content and return "not enough space" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_INSUFFICIENT_SPACE);
    }

    // add metric value
    if ((int32_t)strlen(pMetricValue) < (iBufSize - iOffset))
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%s", pMetricValue);
    }
    else
    {
        // bzero user buffer content and return "not enough space" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_INSUFFICIENT_SPACE);
    }

    // add metric type
    if ((int32_t)strlen("|ms") < (iBufSize - iOffset))
    {
        switch (eMetricType)
        {
        case SERVERMETRICS_TYPE_GAUGE:
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|g");
            break;
        case SERVERMETRICS_TYPE_COUNTER:
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|c");
            break;
        case SERVERMETRICS_TYPE_HISTOGRAM:
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|h");
            break;
        case SERVERMETRICS_TYPE_SET:
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|s");
            break;
        case SERVERMETRICS_TYPE_TIMER:
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|ms");
            break;
        default:
            // bzero user buffer content and return "unsupported metric type" ret code
            ds_memclr(pBuffer, iBufSize);
            return(SERVERMETRICS_ERR_UNSUPPORTED_METRICTYPE);
        }
    }
    else
    {
        // bzero user buffer content and return "not enough space" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_INSUFFICIENT_SPACE);
    }

    // add sample rate  -- not yet implemented because metric never downsampled at the moment
    if (uSampleRate != 100)
    {
        // bzero user buffer content and return "unsupported sample rate" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_UNSUPPORTED_SAMPLERATE);
    }

    // add tags
    if (((int32_t)strlen("|#") + (int32_t)strlen(pDimensionsStr)) < (iBufSize - iOffset))
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "|#%s", pDimensionsStr);
    }
    else
    {
        // bzero user buffer content and return "not enough space" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_INSUFFICIENT_SPACE);
    }

    // add line feed
    if ((int32_t)strlen("\n") < (iBufSize - iOffset))
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "\n");
    }
    else
    {
        // bzero user buffer content and return "not enough space" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_INSUFFICIENT_SPACE);
    }

    // catch scenarios where a single line does not even fit in a metric datagram
    if (iOffset > SERVERMETRICS_DATAGRAM_SIZE)
    {
        // bzero user buffer content and return "entry too large" ret code
        ds_memclr(pBuffer, iBufSize);
        return(SERVERMETRICS_ERR_ENTRY_TOO_LARGE);
    }

    return(iOffset);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ServerMetricsCreate

    \Description
        Create ServerMetrics module.

    \Input *pAggregatorHost         - aggregator host name
    \Input uMericsAggregatorPort    - port that the external metrics aggregator is listening on
    \Input uMetricsPushingRate      - delay between each push to metrics aggregatro (in ms)
    \Input eFormat                  - format type to be used when reporting metrics
    \Input *pCallback               - callback to format stats info
    \Input *pUserData               - user data ref for callback

    \Output
        ServerMetricsT *            - module state, or NULL if create failed

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
ServerMetricsT *ServerMetricsCreate(const char *pAggregatorHost, uint32_t uMetricsAggregatorPort, uint32_t uMetricsPushingRate, ServerMetricsFormatE eFormat, ServerMetricsCallbackT *pCallback, void *pUserData)
{
    ServerMetricsT *pServerMetrics;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create module state
    if ((pServerMetrics = (ServerMetricsT *)DirtyMemAlloc(sizeof(*pServerMetrics), SERVERMETRICS_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("servermetrics: could not allocate module state\n");
        return(NULL);
    }

    ds_memclr(pServerMetrics, sizeof(*pServerMetrics));

    // create the back buffer 
    if (_ServerMetricsBackBufferAlloc(pServerMetrics, SERVERMETRICS_INIT_BACK_BUFFER_SIZE) < 0)
    {
        DirtyCastLogPrintf("servermetrics: could not allocate memory for the back buffer\n");
        DirtyMemFree(pServerMetrics, SERVERMETRICS_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    pServerMetrics->iMemGroup = iMemGroup;
    pServerMetrics->pMemGroupUserData = pMemGroupUserData;
    pServerMetrics->pCallback = pCallback;
    pServerMetrics->pUserData = pUserData;
    pServerMetrics->iBufLen = SERVERMETRICS_DATAGRAM_SIZE;
    pServerMetrics->iMetricsPushingRate = (int32_t)uMetricsPushingRate;
    pServerMetrics->uMetricsAggregatorPort = uMetricsAggregatorPort;
    pServerMetrics->eFormat = eFormat;
    ds_strnzcpy(pServerMetrics->strMetricsAggregatorHost, pAggregatorHost, sizeof(pServerMetrics->strMetricsAggregatorHost));

    DirtyCastLogPrintf("servermetrics: reporting to %s:%d metrics aggregator\n", pServerMetrics->strMetricsAggregatorHost, pServerMetrics->uMetricsAggregatorPort);

    // return module state to caller
    return(pServerMetrics);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsUpdate

    \Description
        Update ServerMetrics module

    \Input *pServerMetrics  - module state.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
void ServerMetricsUpdate(ServerMetricsT *pServerMetrics)
{
    // early exit if module is in failed state
    if (pServerMetrics->bFailed)
    {
        // reset module state if it is time to re-attempt reporting to the external aggregator
        if (NetTickDiff(NetTick(), pServerMetrics->uLastConnectTime) > SERVERMETRICS_RETRY_RATE)
        {
            _ServerMetricsReset(pServerMetrics);
        }
    }
    else
    {
        // resolve metrics aggregator hostname if not yet done
        if (pServerMetrics->pMetricsAggregatorSock == NULL)
        {
            _ServerMetricsInitUpdate(pServerMetrics);
        }
        else if (pServerMetrics->iMetricsPushingRate != 0)
        {
            _ServerMetricsReportUpdate(pServerMetrics);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsDestroy

    \Description
        Destroy ServerMetrics module.

    \Input *pServerMetrics  - module state.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
void ServerMetricsDestroy(ServerMetricsT *pServerMetrics)
{
    // destroy hostname lookup if we have one in progress
    if (pServerMetrics->pMetricsAggregatorHostent != NULL)
    {
        pServerMetrics->pMetricsAggregatorHostent->Free(pServerMetrics->pMetricsAggregatorHostent);
        pServerMetrics->pMetricsAggregatorHostent = NULL;
    }

    // destroy socket used to push metrics to external metrics aggregator
    if (pServerMetrics->pMetricsAggregatorSock != NULL)
    {
        SocketClose(pServerMetrics->pMetricsAggregatorSock);
        pServerMetrics->pMetricsAggregatorSock = NULL;
    }

    // destroy the scratchpad
    if (pServerMetrics->pScratchPad != NULL)
    {
        DirtyMemFree(pServerMetrics->pScratchPad, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
    }

    // destroy back buffer
    if (pServerMetrics->pBackbuffer != NULL)
    {
        DirtyMemFree(pServerMetrics->pBackbuffer, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
    }

    DirtyMemFree(pServerMetrics, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsControl

    \Description
        Format uptime into output buffer.

    \Input *pServerMetrics  - module state
    \Input iControl         - status selector
    \Input iValue           - control value
    \Input iValue2          - control value
    \Input *pValue          - control value

    \Output
        int32_t             - size of returned data or error code (negative value)

    \Notes
        iInfo can be one of the following:

        \verbatim
            'push' - force an immediate metrics report
            'rate' - change metrics push rate
            'spam' - debug level for debug output
        \endverbatim

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
int32_t ServerMetricsControl(ServerMetricsT *pServerMetrics, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'push')
    {
        pServerMetrics->uNextReportTime = NetTick();
        return(0);
    }
    if (iControl == 'rate')
    {
        DirtyCastLogPrintfVerbose(pServerMetrics->iVerbose, 0, "servermetrics: [%p] metrics push rate changed from %d ms to %d ms\n", pServerMetrics, pServerMetrics->iMetricsPushingRate, iValue);
        pServerMetrics->iMetricsPushingRate = iValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        DirtyCastLogPrintf("servermetrics: [%p] debug verbosity level changed from %d to %d\n", pServerMetrics, pServerMetrics->iVerbose, iValue);
        pServerMetrics->iVerbose = iValue;
        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsFormatEx

    \Description
        Format multiple metric values for a given metric in the specified buffer.

        \Input *pBuffer             - pointer to buffer to format data into
        \Input iBufSize             - size of buffer
        \Input *pMetricName         - pointer to metric name
        \Input eFormat              - expected output format
        \Input *pMetricValues       - pointer to first array of metric values
        \Input uMetricValueCount    - number of items in array that pMetriValues is pointing to
        \Input eMetricType          - metric type
        \Input uSampleRate          - percentage of the metric samples that are being sent to the aggregator
        \Input *pDimensionsStr      - pointer to string containing a sequence of dimensions to be associated with the metric (expected format "dim1:val1,dim2,dim3:val3"). Can be NULL.

    \Output
        int32_t                     - >=0: success (number of bytes written);  -1: not enough space;  <-1: other errors

    \Version 10/01/2019 (mclouatre)
*/
/********************************************************************************F*/
int32_t ServerMetricsFormatEx(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, uint64_t *pMetricValues, uint32_t uMetricValueCount, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr)
{
    int32_t iResult;

    switch (eFormat)
    {
        case SERVERMETRICS_FORMAT_DATADOG:
        {
            char strTemp[SERVERMETRICS_DATAGRAM_SIZE];    // we can't pack on a single line more than what a single datagram can take
            char *pWrite = &strTemp[0];
            char *pEnd = &strTemp[SERVERMETRICS_DATAGRAM_SIZE];
            uint32_t uMetricValueIndex;
            ds_memclr(strTemp, sizeof(strTemp));

            // add column symbol at the beginning
            *pWrite++ = ':';

            // concatenate all metric values into a single string
            for (uMetricValueIndex = 0; (uMetricValueIndex < uMetricValueCount) && (pWrite < pEnd); uMetricValueIndex++, pMetricValues++)
            {
                pWrite += ds_snzprintf(pWrite, pEnd - pWrite, "%llu,", *pMetricValues);
            }

            if (pWrite < pEnd)
            {
                // remove comma after last added value
                pWrite = pWrite - 1;
                *pWrite = 0;

                iResult = _ServerMetricsFormatDatadog(pBuffer, iBufSize, pMetricName, eFormat, strTemp, eMetricType, uSampleRate, pDimensionsStr);
            }
            else
            {
                iResult = SERVERMETRICS_ERR_ENTRY_TOO_LARGE;
            }
            break;
        }
        default:
            // unsupported format
            iResult = SERVERMETRICS_ERR_UNSUPPORTED_FORMAT;
            break;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsFormat

    \Description
        Format metric in the specified buffer.

    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer
    \Input *pMetricName     - pointer to metric name
    \Input eFormat          - expected output format
    \Input uMetricValue     - metric value
    \Input eMetricType      - metric type
    \Input uSampleRate      - percentage of the metric samples that are being sent to the aggregator
    \Input *pDimensionsStr  - pointer to string containing a sequence of dimensions to be associated with the metric (expected format "dim1:val1,dim2,dim3:val3"). Can be NULL.

    \Output
        int32_t                 - >=0: success (number of bytes written);  -1: not enough space;  <-1: other errors

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
int32_t ServerMetricsFormat(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, uint64_t uMetricValue, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr)
{
    return(ServerMetricsFormatEx(pBuffer, iBufSize, pMetricName, eFormat, &uMetricValue, 1, eMetricType, uSampleRate, pDimensionsStr));
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsFormatFloat

    \Description
        Format metric in the specified buffer.

    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer
    \Input *pMetricName     - pointer to metric name
    \Input eFormat          - expected output format
    \Input fMetricValue     - metric value
    \Input eMetricType      - metric type
    \Input uSampleRate      - percentage of the metric samples that are being sent to the aggregator
    \Input *pDimensionsStr  - pointer to string containing a sequence of dimensions to be associated with the metric (expected format "dim1:val1,dim2,dim3:val3"). Can be NULL.

    \Output
        int32_t             - >=0: success (number of bytes written);  -1: not enough space;  <-1: other errors

    \Version 05/02/2018 (cvienneau)
*/
/********************************************************************************F*/
int32_t ServerMetricsFormatFloat(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, float fMetricValue, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr)
{
    int32_t iResult;

    switch (eFormat)
    {
    case SERVERMETRICS_FORMAT_DATADOG:
    {
        char strTemp[32];
        ds_memclr(strTemp, sizeof(strTemp));
        ds_snzprintf(strTemp, sizeof(strTemp), ":%f", fMetricValue);
        iResult = _ServerMetricsFormatDatadog(pBuffer, iBufSize, pMetricName, eFormat, strTemp, eMetricType, uSampleRate, pDimensionsStr);
        break;
    }
    default:
        // unsupported format
        iResult = SERVERMETRICS_ERR_UNSUPPORTED_FORMAT;
        break;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ServerMetricsGetScratchPad

    \Description
        Get and appropriately size the scratch.

    \Input *pServerMetrics  - module state
    \Input iMinSize         - minimum required length

    \Output
        char*               - pointer to the scratchpad

    \Version 02/15/2018 (amakoukji)
*/
/********************************************************************************F*/
char* ServerMetricsGetScratchPad(ServerMetricsT *pServerMetrics, int32_t iMinSize)
{
    // check size, recreate if necessary
    if ((pServerMetrics->iScratchPadLen < iMinSize) || (pServerMetrics->pScratchPad == NULL))
    {
        // destroy the old scratchpad
        if (pServerMetrics->pScratchPad != NULL)
        {
            DirtyMemFree(pServerMetrics->pScratchPad, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
        }

        pServerMetrics->pScratchPad = DirtyMemAlloc(iMinSize, SERVERMETRICS_MEMID, pServerMetrics->iMemGroup, pServerMetrics->pMemGroupUserData);
        if (pServerMetrics->pScratchPad == NULL)
        {
            DirtyCastLogPrintf("servermetrics: [%p] could not allocate %d bytes for scratchpad\n", pServerMetrics, iMinSize);
            pServerMetrics->iScratchPadLen = 0;
        }
        else
        {
            pServerMetrics->iScratchPadLen = iMinSize;
        }
    }
    
    return(pServerMetrics->pScratchPad);
}

