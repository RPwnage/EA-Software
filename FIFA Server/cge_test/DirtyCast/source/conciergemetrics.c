/*H********************************************************************************/
/*!
    \File conciergemetrics.c

    \Description
        This module implements metrics datagram formatting for the VoipServer (Concierge)

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <stdio.h>
#include <string.h>

#include "conciergemetrics.h"

#include "DirtySDK/dirtysock/dirtylib.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "commonmetrics.h"
#include "voipconcierge.h"
#include "voipserverconfig.h"
#include "voipserver.h"

/*** Macros ***********************************************************************/

//! format of the dimensions string, if this value changes be sure to update the size calculation where it is used
#define CONCIERGE_DIMENSIONS_FORMAT ("%s,product:%s,prototunnelver:%u.%u")
#define CONCIERGE_COMMON_DIMENSIONS_FORMAT ("host:%s,port:%d,mode:concierge,pool:%s,deployinfo:%s,deployinfo2:%s,site:%s,env:%s")

/*** Type Definitions *************************************************************/

//! metric handler type
typedef int32_t(ConciergeMetricHandlerT)(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct ConciergeMetricHandlerEntryT
{
    const char strMetricName[64];
    ConciergeMetricHandlerT *const pMetricHandler;
    const uint32_t uInterval;
    uint32_t uWhen;
} ConciergeMetricHandlerEntryT;


/*** Function Prototypes ***************************************************************/

static int32_t _ConciergeMetricsAutoscalePct(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _ConciergeMetricsClientsConnected(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _ConciergeMetricsClientsSupported(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _ConciergeMetricsGamesAvailable(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

/*** Variables ********************************************************************/

static ConciergeMetricHandlerEntryT _ConciergeMetrics_MetricHandlers[] =
{
    { "invalid",                                NULL, 0, 0 },
    { "voipmetrics.autoscale.pct",              _ConciergeMetricsAutoscalePct, 0, 0 },
    { "voipmetrics.clients.active",             _ConciergeMetricsClientsConnected, 0, 0 },
    { "voipmetrics.clients.maximum",            _ConciergeMetricsClientsSupported, 0, 0 },   // uInterval=0 to allow quick announcement of max change when entering "draining" state
    { "voipmetrics.games.available",            _ConciergeMetricsGamesAvailable, 0, 0 }      // uInterval=0 to allow quick announcement of max change when entering "draining" state
};

//! global variable used to remember info in between calls to _ConciergeMetricsClientsConnected()
//! $todo - find a graceful way to make this dynamically growable without introducing the possibility of a mem leak
static int32_t _ConciergeMetrics_iPreviousProductMapSize = 0;
static ConciergeProductMapEntryT _ConciergeMetrics_aPreviousProductsMap[64] =
    { {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0} };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsAutoscalePct

    \Description
        Adds the "voipmetrics.autoscale.pct" metric to the specified buffer.
        Metric description: ratio representing how much of our allowed CPU budget is consumed

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/29/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsAutoscalePct(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    VoipServerRefT *pVoipServer = VoipConciergeGetBase(pVoipConcierge);
    float fConsumedCpuBudget;

    VoipServerStatus(pVoipServer, 'epct', 0, &fConsumedCpuBudget, sizeof(fConsumedCpuBudget));

    return(ServerMetricsFormatFloat(pBuffer, iBufSize, pMetricName, eFormat, fConsumedCpuBudget, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}


/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsClientsConnectedForActiveProducts

    \Description
        For all products that have active clients, this funciton adds the
        "voipmetrics.client.active" metric to the specified buffer.
        Metric description: current number of clients that are serviced by the voipserver 

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input *pProtoTunnelVersion - prototunnel version string
    \Input *pProductMap         - pointer to most recent product map queried from voiptunnel
    \Input iProductMapSize      - size of most recent product map queried from voiptunnel
    \Input *pOffset             - [IN,OUT] write offset in pBuffer

    \Output
        int32_t                 - 0: success;  <0: SERVERMETRICS_ERR_XXX

    \Version 10/24/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsClientsConnectedForActiveProducts(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, 
                                                                  char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, const char *pProtoTunnelVersion,
                                                                  ConciergeProductMapEntryT *pProductMap, int32_t iProductMapSize, int32_t *pOffset)
{
    int32_t iDimensionsStringLen;
    int32_t iErrorCode = 0;
    int32_t iProductIndex;
    int32_t iResult;
    char *pScratchPad = NULL;

    for (iProductIndex = 0; iProductIndex < iProductMapSize; iProductIndex++)
    {
        // make sure product entry is valid
        if (pProductMap[iProductIndex].strProductId[0] != '\0')
        {
            // get an appropriately sized scratch pad
            iDimensionsStringLen = (int32_t)strnlen(pCommonDimensions, iBufSize) +                            // length of pCommonDimensions
                (int32_t)strnlen(pProductMap[iProductIndex].strProductId, VOIPCONCIERGE_CCS_PRODUCTID_SIZE) + // length of strProductId
                21 +                                                                                          // max length of uProtoTunnelVersMaj
                21 +                                                                                          // max length of uProtoTunnelVersMin
                (int32_t)strlen(CONCIERGE_DIMENSIONS_FORMAT);                                                 // length of the remainder of the format string
            pScratchPad = ServerMetricsGetScratchPad(VoipServerGetServerMetrics(VoipConciergeGetBase(pVoipConcierge)), iDimensionsStringLen);

            if (pScratchPad != NULL)
            {
                uint64_t uActiveClients;
                uint32_t uProtoTunnelVersMaj, uProtoTunnelVersMin;
                uint16_t uProtoTunnelVersion;

                // convert prototunnel version string to numerical prototunnel version
                sscanf(pProtoTunnelVersion, "%u.%u", &uProtoTunnelVersMaj, &uProtoTunnelVersMin);

                // convert prototunnel version string to numerical prototunnel version
                uProtoTunnelVersion = (uint8_t)(uProtoTunnelVersMaj) << 8;
                uProtoTunnelVersion |= (uint8_t)(uProtoTunnelVersMin);

                // query number of active clients for the specified product and tunnel version
                uActiveClients = (uint64_t)VoipConciergeStatus(pVoipConcierge, 'actv', uProtoTunnelVersion, pProductMap[iProductIndex].strProductId, 0);

                //  add the product and the prototunnel version to the dimensions
                ds_memclr(pScratchPad, iDimensionsStringLen);
                ds_snzprintf(pScratchPad, iDimensionsStringLen, CONCIERGE_DIMENSIONS_FORMAT, pCommonDimensions, pProductMap[iProductIndex].strProductId, uProtoTunnelVersMaj, uProtoTunnelVersMin);

                if ((iResult = ServerMetricsFormat(pBuffer + *pOffset, iBufSize - *pOffset, pMetricName, eFormat, uActiveClients, SERVERMETRICS_TYPE_GAUGE, 100, pScratchPad)) >= 0)
                {
                    // count of active clients for specified prototunnel version successfully added
                    *pOffset += iResult;
                }
                else
                {
                    // error - abort!
                    iErrorCode = iResult;
                    break;
                }
            }
            else
            {
                iErrorCode = SERVERMETRICS_ERR_OUT_OF_MEMORY;
                break;
            }
        }
    }

    return(iErrorCode);
}


/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsClientsConnectedForInactiveProducts

    \Description
        For all products that no longer have active clients, this function adds the
        "voipmetrics.client.active" metric to the specified buffer with a 0 value.
        Metric description: current number of clients that are serviced by the voipserver 

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics
    \Input *pProtoTunnelVersion - prototunnel version string
    \Input *pProductMap         - pointer to most recent product map queried from voiptunnel
    \Input iProductMapSize      - size of most recent product map queried from voiptunnel
    \Input *pOffset             - [IN,OUT] write offset in pBuffer

    \Output
        int32_t                 - 0: success; <0: SERVERMETRICS_ERR_XXX

    \Notes
        By updating with 0 the metric for products that no longer have active clients, we avoid 
        the last announced value to stick around for multiple aggregator flush intervals and
        we eliminate ephemeral capacity errors in our dashboards.

    \Version 10/24/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsClientsConnectedForInactiveProducts(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, 
                                                                    char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, const char *pProtoTunnelVersion,
                                                                    ConciergeProductMapEntryT *pProductMap, int32_t iProductMapSize, int32_t *pOffset)
{
    int32_t iDimensionsStringLen;
    int32_t iErrorCode = 0;
    int32_t iProductIndex, iPreviousProductIndex;
    int32_t iResult;
    char *pScratchPad = NULL;

    for (iPreviousProductIndex = 0; iPreviousProductIndex < _ConciergeMetrics_iPreviousProductMapSize; iPreviousProductIndex++)
    {
        // make sure previous product entry is valid
        if (_ConciergeMetrics_aPreviousProductsMap[iPreviousProductIndex].strProductId[0] != '\0')
        {
            // get an appropriately sized scratch pad
            iDimensionsStringLen = (int32_t)strnlen(pCommonDimensions, iBufSize) +                            // length of pCommonDimensions
                (int32_t)strnlen(_ConciergeMetrics_aPreviousProductsMap[iPreviousProductIndex].strProductId, VOIPCONCIERGE_CCS_PRODUCTID_SIZE) + // length of strProductId
                21 +                                                                                          // max length of uProtoTunnelVersMaj
                21 +                                                                                          // max length of uProtoTunnelVersMin
                (int32_t)strlen(CONCIERGE_DIMENSIONS_FORMAT);                                                 // length of the remainder of the format string
            pScratchPad = ServerMetricsGetScratchPad(VoipServerGetServerMetrics(VoipConciergeGetBase(pVoipConcierge)), iDimensionsStringLen);

            if (pScratchPad != NULL)
            {
                uint32_t bFound = FALSE;

                // check if that product still exists
                for (iProductIndex = 0; iProductIndex < iProductMapSize; iProductIndex++)
                {
                    // make sure product entry is valid
                    if (pProductMap[iProductIndex].strProductId[0] != '\0')
                    {
                        if (!strcmp(_ConciergeMetrics_aPreviousProductsMap[iPreviousProductIndex].strProductId, pProductMap[iProductIndex].strProductId))
                        {
                            bFound = TRUE;
                            break;
                        }
                    }
                } // end of FOR (all current products) loop

                if (bFound == FALSE)
                {
                    uint32_t uProtoTunnelVersMaj, uProtoTunnelVersMin;
                    uint16_t uProtoTunnelVersion;

                    // convert prototunnel version string to numerical prototunnel version
                    sscanf(pProtoTunnelVersion, "%u.%u", &uProtoTunnelVersMaj, &uProtoTunnelVersMin);

                    // convert prototunnel version string to numerical prototunnel version
                    uProtoTunnelVersion = (uint8_t)(uProtoTunnelVersMaj) << 8;
                    uProtoTunnelVersion |= (uint8_t)(uProtoTunnelVersMin);

                    // add the product and the prototunnel version to the dimensions
                    ds_memclr(pScratchPad, iDimensionsStringLen);
                    ds_snzprintf(pScratchPad, iDimensionsStringLen, CONCIERGE_DIMENSIONS_FORMAT, pCommonDimensions, _ConciergeMetrics_aPreviousProductsMap[iPreviousProductIndex].strProductId, uProtoTunnelVersMaj, uProtoTunnelVersMin);

                    if ((iResult = ServerMetricsFormat(pBuffer + *pOffset, iBufSize - *pOffset, pMetricName, eFormat, 0, SERVERMETRICS_TYPE_GAUGE, 100, pScratchPad)) >= 0)
                    {
                        // count of active clients for specified prototunnel version successfully added
                        *pOffset += iResult;
                    }
                    else
                    {
                        // error - abort!
                        iErrorCode = iResult;
                        break;
                    }
                }
            }
            else
            {
                iErrorCode = SERVERMETRICS_ERR_OUT_OF_MEMORY;
                break;
            }
        }
    }

    return(iErrorCode);
}


/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsSaveProductMap

    \Description
        Saves product map for comparison (old vs new) during next reporting cycle.

    \Input *pVoipConcierge      - module state
    \Input *pProductMap         - pointer to most recent product map queried from voiptunnel
    \Input iProductMapSize      - size of most recent product map queried from voiptunnel

    \Notes
        In a scenario where _ConciergeMetrics_aPreviousProductsMap is not large enough
        to save the entire product map, then we would end up with sub-obtimal reporting
        of voipmetrics.client.active for these products. They would continue to report
        some active clients when in fact they no longer have any, and that would skew our
        dashboards until the aggregator stops reporting the stalled value because it
        aged out.

    \Version 10/24/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _ConciergeMetricsSaveProductMap(VoipConciergeRefT *pVoipConcierge, ConciergeProductMapEntryT *pProductMap, int32_t iProductMapSize)
{
    static uint32_t bOverflowLogged = FALSE;
    int32_t iCopySize;

    // save product map for next reporting cycle
    if (iProductMapSize <= (signed)DIRTYCAST_CalculateArraySize(_ConciergeMetrics_aPreviousProductsMap))
    {
        iCopySize = iProductMapSize;

        if (bOverflowLogged == TRUE)
        {
            bOverflowLogged = FALSE;
            DirtyCastLogPrintf("conciergemetrics: RECOVER product map size (%d) back to a level we support (%d)\n",
                iProductMapSize, DIRTYCAST_CalculateArraySize(_ConciergeMetrics_aPreviousProductsMap));
        }
    }
    else
    {
        iCopySize = DIRTYCAST_CalculateArraySize(_ConciergeMetrics_aPreviousProductsMap);

        if (bOverflowLogged == FALSE)
        {
            DirtyCastLogPrintf("conciergemetrics: ERROR product map size (%d) does not fit in our cache (%d) for usage in next reporting cycle!\n",
                iProductMapSize, DIRTYCAST_CalculateArraySize(_ConciergeMetrics_aPreviousProductsMap));
            bOverflowLogged = TRUE;
        }
    }
    ds_memcpy_s(&_ConciergeMetrics_aPreviousProductsMap[0], sizeof(_ConciergeMetrics_aPreviousProductsMap), pProductMap, (iCopySize * sizeof(ConciergeProductMapEntryT)));
    _ConciergeMetrics_iPreviousProductMapSize = iCopySize;
}


/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsClientsConnected

    \Description
        Adds the "voipmetrics.client.active" metric to the specified buffer.
        Metric description: current number of clients that are serviced by the voipserver

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 02/20/2017 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsClientsConnected(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strProtoTunnelVersions[128];
    char strProtoTunnelVersion[8];
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iProtoTunnelVersionIndex = 0;
    ConciergeProductMapEntryT *pProductMap;
    int32_t iProductMapSize;

    // query the versions supported by prototunnel
    ProtoTunnelStatus(VoipServerGetProtoTunnel(VoipConciergeGetBase(pVoipConcierge)), 'vset', 0, strProtoTunnelVersions, sizeof(strProtoTunnelVersions));

    // query the products known by voipconcierge
    iProductMapSize = VoipConciergeStatus(pVoipConcierge, 'prod', 0, &pProductMap, 0);

    // loop through all the prototunnel versions found
    while ((iErrorCode == 0) && TagFieldGetDelim(strProtoTunnelVersions, strProtoTunnelVersion, sizeof(strProtoTunnelVersion), "", iProtoTunnelVersionIndex, ',') != 0)
    {
        // first, deal with all products existing in the newly-queried product map 
        iErrorCode = _ConciergeMetricsClientsConnectedForActiveProducts(pVoipConcierge, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions,
                                                                        strProtoTunnelVersion, pProductMap, iProductMapSize, &iOffset);

        if (iErrorCode == 0)
        {
             // second, deal with all products for which there was a valid count at the last report cycle, but there no longer is for this new cycle.
             iErrorCode = _ConciergeMetricsClientsConnectedForInactiveProducts(pVoipConcierge, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions,
                                                                               strProtoTunnelVersion, pProductMap, iProductMapSize, &iOffset);
        }

        iProtoTunnelVersionIndex++;
    }

    // save product map for comparison during next reporting cycle
    _ConciergeMetricsSaveProductMap(pVoipConcierge, pProductMap, iProductMapSize);

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
    \Function _ConciergeMetricsClientsSupported

    \Description
        Adds the "voipmetrics.clients.maximum" metric to the specified buffer.
        Metric description: maximum number of clients that can be serviced by the voipserver

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsClientsSupported(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uMaximumClients;

    if (VoipServerStatus(VoipConciergeGetBase(pVoipConcierge), 'psta', 0, NULL, 0) == VOIPSERVER_PROCESS_STATE_RUNNING)
    {
        // query max clients supported
        uMaximumClients = VoipTunnelStatus(VoipServerGetVoipTunnel(VoipConciergeGetBase(pVoipConcierge)), 'musr', 0, NULL, 0);
    }
    else
    {
        // when draining, report that the max is limited to the set of still active clients (because the extra capacity is no longer usable)
        uMaximumClients = (uint64_t)VoipTunnelStatus(VoipServerGetVoipTunnel(VoipConciergeGetBase(pVoipConcierge)), 'nusr', -1, NULL, 0);
    }

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uMaximumClients, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}


/*F********************************************************************************/
/*!
    \Function _ConciergeMetricsGamesAvailable

    \Description
        Adds the "voipmetrics.games.available" metric to the specified buffer.
        Metric description: maximum number of "games" that can be serviced by the voipserver

    \Input *pVoipConcierge      - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _ConciergeMetricsGamesAvailable(VoipConciergeRefT *pVoipConcierge, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uGamesAvailable;

    if (VoipServerStatus(VoipConciergeGetBase(pVoipConcierge), 'psta', 0, NULL, 0) == VOIPSERVER_PROCESS_STATE_RUNNING)

    {
        // query max clients supported
        uGamesAvailable = (uint64_t)VoipTunnelStatus(VoipServerGetVoipTunnel(VoipConciergeGetBase(pVoipConcierge)), 'mgam', 0, NULL, 0);
    }
    else
    {
        // when draining, report that the max is limited to the set of still active games (because the extra capacity is no longer usable)
        uGamesAvailable = (uint64_t)VoipTunnelStatus(VoipServerGetVoipTunnel(VoipConciergeGetBase(pVoipConcierge)), 'ngam', 0, NULL, 0);
    }

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uGamesAvailable, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ConciergeMetricsCallback

    \Description
        Callback to format voipserver stats into a datagram to be sent to an
        external metrics aggregator

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN]VoipServer module state

    \Output
        int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Notes
        Each single metric consumes a "line" in the text-formatted datagram. The number of dimensions that the metric is
        tagged with, and their respective length, can significantly impact the number of characters consumed by each
        line... and, consequently, the number of metrics that fit in a datagram buffer.

        This function loops through multiple handlers and call them in sequence until the datagram is full... in which
        case the function exits with SERVERMETRICS_ERR_INSUFFICIENT_SPACE and expects to be called again with a
        new buffer to fill in and the metrics index where it needs to resume the packing from.

        For this mechanism to work, it is CRITICAL that each single metric handler limits the number of lines
        it writes into the packet buffer (not more than 4). If a single handler writes too many lines, it may
        immediately exhaust the datagram capacity and that data will never fit into any datagram... resulting in
        the respective metrics to never be reported.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
int32_t ConciergeMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    VoipServerRefT *pVoipServer = VoipConciergeGetBase(pVoipConcierge);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strHostName[128];
    char strCommonDimensions[sizeof(CONCIERGE_COMMON_DIMENSIONS_FORMAT) +   // size of the format string
                             sizeof(strHostName) +                          // size of the hostname
                             5 +                                            // size of the port
                             VOIPSERVERCONFIG_POOL_SIZE +                   // size of the pool value
                             VOIPSERVERCONFIG_DEPLOYINFO_SIZE +             // size of deployinfo value
                             VOIPSERVERCONFIG_DEPLOYINFO2_SIZE +            // size of deployinfo2 value
                             VOIPSERVERCONFIG_SITE_SIZE +                   // size of site value
                             VOIPSERVERCONFIG_ENV_SIZE +                    // size of env value
                             1];                                            // NULL


    //describe the order the different metrics sources are added to our list
    const int32_t iTrafficMetricsStart = 0;
    const int32_t iSystemMetricsStart = CommonTrafficMetricsGetCount();
    const int32_t iCcMetricsStart = CommonTrafficMetricsGetCount() + CommonSystemMetricsGetCount();
    
    VoipServerStatus(pVoipServer, 'host', 0, strHostName, sizeof(strHostName));

    // prepare dimensions to be associated with all metrics
    ds_memclr(strCommonDimensions, sizeof(strCommonDimensions));
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), CONCIERGE_COMMON_DIMENSIONS_FORMAT,
                                                                   strHostName,
                                                                   pConfig->uDiagnosticPort,
                                                                   pConfig->strPool,
                                                                   pConfig->strDeployInfo,
                                                                   pConfig->strDeployInfo2,
                                                                   pConfig->strPingSite,
                                                                   pConfig->strEnvironment);

    // is iMetricIndex pointing to a "Common Traffic" metric?
    if ((iMetricIndex >= iTrafficMetricsStart) && (iMetricIndex < iSystemMetricsStart))
    {
        if (iMetricIndex == iTrafficMetricsStart)   //the first index of a set is invalid
        {
            iMetricIndex++;
        }
        if ((iRetCode = CommonTrafficMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex - iTrafficMetricsStart, strCommonDimensions)) == 0)
        {
            // we are done with common traffic metrics, make sure we now carry on with common system metrics
            iMetricIndex = iRetCode = iSystemMetricsStart;
            iOffset = *pSize;
        }
    }

    // is iMetricIndex pointing to a "Common System" metric?
    else if ((iMetricIndex >= iSystemMetricsStart) && (iMetricIndex < iCcMetricsStart))
    {
        if (iMetricIndex == iSystemMetricsStart)   //the first index of a set is invalid
        {
            iMetricIndex++;
        }
        if ((iRetCode = CommonSystemMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex - iSystemMetricsStart, strCommonDimensions)) == 0)
        {
            // we are done with common system metrics, make sure we now take care of qos-specific metrics
            iMetricIndex = iRetCode = iCcMetricsStart;
            iOffset = *pSize;
        }
        else
        {
            iRetCode = iMetricIndex = iRetCode + iSystemMetricsStart;
        }

    }

    // is iMetricIndex pointing to a "concierge-specific" metric?
    else if (iMetricIndex >= iCcMetricsStart)
    {
        // *** concierge-specific metrics ***

        // rescale the metric index
        iMetricIndex -= iCcMetricsStart;
        if (iMetricIndex == 0)
        {
            iMetricIndex++;
        }

        // loop through
        while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_ConciergeMetrics_MetricHandlers))
        {
            int32_t iResult;
            ConciergeMetricHandlerEntryT *pEntry = &_ConciergeMetrics_MetricHandlers[iMetricIndex];
            const uint32_t uCurTick = NetTick();

            // set when so we always send the first time
            if (pEntry->uWhen == 0)
            {
                pEntry->uWhen = uCurTick;
            }
            // if it isn't time yet then just move to the next metric
            if (NetTickDiff(uCurTick, pEntry->uWhen) < 0)
            {
                iMetricIndex++;
                continue;
            }

            if ((iResult = pEntry->pMetricHandler(pVoipConcierge, pEntry->strMetricName, pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "conciergemetrics: adding concierge metric %s (#%d) to report succeeded\n", _ConciergeMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
                iOffset += iResult;
                pEntry->uWhen = pEntry->uInterval + uCurTick;
            }
            else if (iResult == 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "conciergemetrics: adding concierge metric %s (#%d) to report skipped because metric unavailable\n", _ConciergeMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
            }
            else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
            {
                // return "scaled" metric index
                iRetCode = iMetricIndex + iCcMetricsStart;
                break;
            }
            else
            {
                DirtyCastLogPrintf("conciergemetrics: adding concierge metric %s (#%d) to report failed with error %d - metric skipped!\n", _ConciergeMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
                iMetricIndex++;
            }
        }

        // let the caller know how many bytes were written to the buffer
        *pSize = iOffset;

        // if all metrics have been dealt with, then let the call know
        if (iMetricIndex == (sizeof(_ConciergeMetrics_MetricHandlers) / sizeof(_ConciergeMetrics_MetricHandlers[0])))
        {
            iRetCode = 0;
        }
    }

    if (iRetCode == 0)
    {
        // tell the server to reset stats
        VoipServerControl(pVoipServer, 'stat', TRUE, 0, NULL);
    }

    return(iRetCode);
}
