/*H********************************************************************************/
/*!
    \File gameserveroimetrics.c

    \Description
        This module implements OIv2 metrics for the gameserver

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"

#include "dirtycast.h"
#include "servermetrics.h"
#include "gameserveroimetrics.h"
#include "gameserverconfig.h"
#include "gameserver.h"
#include "gameserverblaze.h"
#include "gameserverdist.h"

/*** Defines **********************************************************************/

//! format of the dimensions string, if this value changes to sure to update the size calculation and values where it is used
#define GAMESERVER_OI_METRICS_COMMON_DIMENSIONS_FORMAT ("deployinfo2:%s,host:%s,site:%s,product:%s,env:%s,subspace:%d")

//! allocation identifier
#define GAMESERVER_OI_METRIC_MEMID        ('gsom')

//! max number of metric values that can be accumulated before flushing
#define GAMESERVER_OI_ACCUMULATOR_MAX_SIZE  (300)    // 1024-byte datagram / 3-byte value entry 

/*** Type Definitions *************************************************************/

//! all the metrics that are to be sent by the GS
typedef struct GameServerOiMetricDataT
{
    uint8_t bIddpsGracePeriodDone[GAMESERVER_MAX_CLIENTS];  //!< TRUE when iddps samples can be considered valid (low idpps at beginning of game is tolerated but not used as contributing samples)
    uint32_t aInputDropCount[GAMESERVER_MAX_CLIENTS];       //!< input drop count per client
    uint8_t aInputDropCountInitialized[GAMESERVER_MAX_CLIENTS];
    uint32_t aInputDistPacketCount[GAMESERVER_MAX_CLIENTS]; //!< input dist packet count per client
    uint8_t aInputDistPacketCountInitialized[GAMESERVER_MAX_CLIENTS];
    uint32_t uOutputMultiDistPacketCount;                   //!< odmpps (per GS)
    uint8_t bOutputMultiDistPacketCountInitialized;
    uint8_t _pad[3];
} GameServerOiMetricDataT;

//! generic accumulator used to store metric values and send them all in a single packet
typedef struct GameServerOiAccumulatorT
{
    uint64_t aValues[GAMESERVER_OI_ACCUMULATOR_MAX_SIZE];
    uint32_t uCount;                          //!< number of valid entries in the accumulator
    uint8_t bPendingBackBufferExpansion;      //!< flag use to avoid accumulating the same value twice when back buffer expansion is needed
    uint8_t _pad[3];
} GameServerOiAccumulatorT;

//! accumulators used to store metric values and send them all in a single packet
typedef struct GameServerOiAccumulatorsT
{
    GameServerOiAccumulatorT InputDrop;
    GameServerOiAccumulatorT Idpps;
    GameServerOiAccumulatorT Odmpps;
    int32_t iAccumulationCyclesCount;
} GameServerOiAccumulatorsT;

//! manage the metrics system
typedef struct GameServerOiMetricsRefT
{
    GameServerOiMetricDataT MetricsData;      //!< metrics data to send
    GameServerOiAccumulatorsT Accumulators;   //!< accumulators used to pack multiple metric values into a single metric line
    GameServerRefT* pGameServer;              //!< gameserver ref
    GameServerConfigT *pConfig;               //!< pointer to the game server config
    ServerMetricsT* pServerMetrics;           //!< server metrics module
    void *pMemGroupUserData;                  //!< mem group user data
    int32_t iMemGroup;                        //!< mem group
    int32_t iMaxAccumulationCycles;           //!< configured maximum number of back-to-back reporting cycles that can push values in accumulators (0 means 'feature disabled')
    uint32_t uLastIntervalTick;               //!< time tick associated with last execution of metric reporting interval
    uint32_t bLastIntervalTickValid;          //!< boolean identifying if uLastIntervalTick is valid
} GameServerOiMetricsRefT;

//! metric handler type
typedef int32_t(GameServerOiMetricsHandlerT)(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct GameServerOiMetricsHandlerEntryT
{
    char strMetricName[64];
    GameServerOiMetricsHandlerT *pMetricHandler;
} GameServerOiMetricsHandlerEntryT;


/*** Function Prototypes ***************************************************************/
static int32_t _GameServerOiMetricsIdpps(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerOiMetricsOdmpps(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerOiMetricsInputDrop(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

/*** Variables ********************************************************************/

static const GameServerOiMetricsHandlerEntryT _GameServerOiMetricsHandlers[] =
{
    { "invalid",              NULL },
    { "gameserver.idpps",     _GameServerOiMetricsIdpps },
    { "gameserver.inputdrop", _GameServerOiMetricsInputDrop },
    { "gameserver.odmpps",    _GameServerOiMetricsOdmpps }
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsRate

    \Description
        Perform rate calculation, skip first sample

    \Input *pMetricsRef   - module state
    \Input uCurrentCount  - current count
    \Input uPrevCount     - previous count
    \Input uClientNum     - if non zero rate will be average across the clients
    \Input *pRate         - [OUT] rate

    \Version 09/16/2019 (tcho)
*/
/********************************************************************************F*/
static void _GameServerOiMetricsRate(GameServerOiMetricsRefT* pMetricsRef, uint32_t uCurrentCount, uint32_t uPrevCount, uint32_t uClientNum, uint32_t *pRate)
{
    // detecting rollover
    if (uCurrentCount < uPrevCount)
    {
        *pRate = (0xFFFFFFFF - (uPrevCount - uCurrentCount) + 1) / (pMetricsRef->pConfig->uMetricsPushingRate / 1000);
    }
    else
    {
        // perform normal rate calculation
        *pRate = (uCurrentCount - uPrevCount) / (pMetricsRef->pConfig->uMetricsPushingRate / 1000);
    }

    // if client num is not zero divide rate by it
    if (uClientNum > 0)
    {
        *pRate /= uClientNum;
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsInputDropWithoutAccumulation

    \Description
        Adds one sample contributing to the metric into the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/07/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsInputDropWithoutAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iClient;
    int32_t iOffset = 0;

    for (iClient = 0; iClient < pMetricsRef->pConfig->uMaxClients; iClient++)
    {
        int32_t iResult;
        uint32_t uCurrentCount = 0;

        if ((iResult = GameServerDistStatus(pDist, 'drop', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
        {
            uint32_t uRate = 0;

            // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
            if ((iResult == 0) && (pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] == TRUE))
            {
                // perform rate calculation
                _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.aInputDropCount[iClient], 0, &uRate);

                iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, uRate, SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);

                if (iResult < 0)
                {
                    // set offset to the error and break
                    iOffset = iResult;
                    break;
                }
                else
                {
                    iOffset += iResult;
                }
            }

            if (iResult == 1)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid inputdrop sample for client %d\n", iClient);
            }

            // save down previous value
            pMetricsRef->MetricsData.aInputDropCount[iClient] = uCurrentCount;
            pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] = TRUE;
        }
        else
        {
            // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
            pMetricsRef->MetricsData.aInputDropCount[iClient] = 0;
            pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] = FALSE;
        }
    }

    if (iOffset < 0)
    {
        // if an error occured clear everything that was written to the buffer 
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsInputDropWithAccumulation

    \Description
        Adds multiple samples contributing to the same metric into the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsInputDropWithAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iClient;
    int32_t iOffset = 0;

    // avoid accumulating twice if we are being re-executed after back buffer expansion
    if (pMetricsRef->Accumulators.InputDrop.bPendingBackBufferExpansion == FALSE)
    {
        for (iClient = 0; (iClient < pMetricsRef->pConfig->uMaxClients) && (iOffset == 0); iClient++)
        {
            int32_t iResult;
            uint32_t uCurrentCount = 0;

            if ((iResult = GameServerDistStatus(pDist, 'drop', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
            {
                uint32_t uRate = 0;

                // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
                if ((iResult == 0) && (pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] == TRUE))
                {
                    // perform rate calculation
                    _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.aInputDropCount[iClient], 0, &uRate);

                    if (pMetricsRef->Accumulators.InputDrop.uCount < GAMESERVER_OI_ACCUMULATOR_MAX_SIZE)
                    {
                        pMetricsRef->Accumulators.InputDrop.aValues[pMetricsRef->Accumulators.InputDrop.uCount++] = uRate;
                    }
                    else
                    {
                        iOffset = SERVERMETRICS_ERR_ACCUMULATION_OVERFLOW;
                    }
                }

                if (iResult == 1)
                {
                    DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid inputdrop sample for client %d\n", iClient);
                }

                // save down previous value
                pMetricsRef->MetricsData.aInputDropCount[iClient] = uCurrentCount;
                pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] = TRUE;
            }
            else
            {
                // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
                pMetricsRef->MetricsData.aInputDropCount[iClient] = 0;
                pMetricsRef->MetricsData.aInputDropCountInitialized[iClient] = FALSE;
            }
        }
    }

    // if not yet in error state
    if (iOffset == 0)
    {
        // is it time to flush out the accumulated values?
        if ((pMetricsRef->Accumulators.iAccumulationCyclesCount == (pMetricsRef->iMaxAccumulationCycles - 1)) &&
            (pMetricsRef->Accumulators.InputDrop.uCount != 0))
        {
            int32_t iResult = 0;

            iResult = ServerMetricsFormatEx(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat,
                &pMetricsRef->Accumulators.InputDrop.aValues[0], pMetricsRef->Accumulators.InputDrop.uCount,
                SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);

            if (iResult < 0)
            {
                if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
                {
                    // set flag used to detect that we are being re-executed after back buffer expansion
                    pMetricsRef->Accumulators.InputDrop.bPendingBackBufferExpansion = TRUE;
                }

                // set offset to the error and break
                iOffset = iResult;
            }
            else
            {
                iOffset += iResult;
            }
        }
    }

    // if an error occured clear everything that was written to the buffer 
    if (iOffset < 0)
    {
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsInputDrop

    \Description
        Adds the metric to the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsInputDrop(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    if (pMetricsRef->iMaxAccumulationCycles == 0)
    {
        return(_GameServerOiMetricsInputDropWithoutAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
    else
    {
        return(_GameServerOiMetricsInputDropWithAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsIdppsWithoutAccumulation

    \Description
        Adds one sample contributing to the metric into the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsIdppsWithoutAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iClient;
    int32_t iOffset = 0;

    for (iClient = 0; iClient < pMetricsRef->pConfig->uMaxClients; iClient++)
    {
        int32_t iResult;
        uint32_t uCurrentCount = 0;

        #if DIRTYVERS >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 0)
        // trama code line and beyond
        if ((iResult = GameServerDistStatus(pDist, 'icnt', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
        #else
        // latest releases of shrike code line
        if ((iResult = GameServerDistStatus(pDist, 'idps', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
        #endif
        {
            uint32_t uRate = 0;

            // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
            if ((iResult == 0) && (pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] == TRUE))
            {
                // perform rate calculation
                _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.aInputDistPacketCount[iClient], 0, &uRate);

                /* Special condition added to help with games that do not exercise dist flow control at beginning of game play.
                   To deal with scenarios where idpps is low early in the game even if flow control is enabled (i.e. 'ocnt' selector can't invalidate
                   those samples because pDistServ->bFlowEnabled=TRUE), we drop all early samples until we at least see one above the 27 pps mark. */
                if ((pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] == TRUE) || (uRate >= 27))
                {
                    if (pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] == FALSE)
                    {
                        DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: end of idpps grace period for client %d\n", iClient);
                    }
                    pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] = TRUE;

                    iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, uRate, SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);

                    if (iResult < 0)
                    {
                        // set offset to the error and break
                        iOffset = iResult;
                        break;
                    }
                    else
                    {
                        iOffset += iResult;
                    }
                }
                else
                {
                    // mark sampling interval as invalid
                    iResult = -1;
                }
            }

            if (iResult == 1)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid idpps sample for client %d\n", iClient);
            }

            // save down previous value
            pMetricsRef->MetricsData.aInputDistPacketCount[iClient] = uCurrentCount;
            pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] = TRUE;
        }
        else
        {
            // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
            pMetricsRef->MetricsData.aInputDistPacketCount[iClient] = 0;
            pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] = FALSE;
        }
    }

    if (iOffset < 0)
    {
        // if an error occured clear everything that was written to the buffer 
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsIdppsWithAccumulation

    \Description
        Adds multiple samples contributing to the same metric into the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsIdppsWithAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iClient;
    int32_t iOffset = 0;

    // avoid accumulating twice if we are being re-executed after back buffer expansion
    if (pMetricsRef->Accumulators.Idpps.bPendingBackBufferExpansion == FALSE)
    {
        for (iClient = 0; (iClient < pMetricsRef->pConfig->uMaxClients) && (iOffset == 0); iClient++)
        {
            int32_t iResult;
            uint32_t uCurrentCount = 0;

            #if DIRTYVERS >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 0)
            // trama code line and beyond
            if ((iResult = GameServerDistStatus(pDist, 'icnt', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
            #else
            // latest releases of shrike code line
            if ((iResult = GameServerDistStatus(pDist, 'idps', iClient, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
            #endif
            {
                uint32_t uRate = 0;

                // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
                if ((iResult == 0) && (pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] == TRUE))
                {
                    // perform rate calculation
                    _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.aInputDistPacketCount[iClient], 0, &uRate);

                    /* Special condition added to help with games that do not exercise dist flow control at beginning of game play.
                       To deal with scenarios where idpps is low early in the game even if flow control is enabled (i.e. 'ocnt' selector can't invalidate
                       those samples because pDistServ->bFlowEnabled=TRUE), we drop all early samples until we at least see one above the 27 pps mark. */
                    if ((pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] == TRUE) || (uRate >= 27))
                    {
                        if (pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] == FALSE)
                        {
                            DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: end of idpps grace period for client %d\n", iClient);
                        }
                        pMetricsRef->MetricsData.bIddpsGracePeriodDone[iClient] = TRUE;

                        if (pMetricsRef->Accumulators.Idpps.uCount < GAMESERVER_OI_ACCUMULATOR_MAX_SIZE)
                        {
                            pMetricsRef->Accumulators.Idpps.aValues[pMetricsRef->Accumulators.Idpps.uCount++] = uRate;
                        }
                        else
                        {
                            iOffset = SERVERMETRICS_ERR_ACCUMULATION_OVERFLOW;
                        }
                    }
                    else
                    {
                        // mark sampling interval as invalid
                        iResult = -1;
                    }
                }

                if (iResult == 1)
                {
                    DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid idpps sample for client %d\n", iClient);
                }

                // save down previous value
                pMetricsRef->MetricsData.aInputDistPacketCount[iClient] = uCurrentCount;
                pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] = TRUE;
            }
            else
            {
                // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
                pMetricsRef->MetricsData.aInputDistPacketCount[iClient] = 0;
                pMetricsRef->MetricsData.aInputDistPacketCountInitialized[iClient] = FALSE;
            }
        }
    }

    // if not yet in error state
    if (iOffset == 0)
    {
        // is it time to flush out the accumulated values?
        if ((pMetricsRef->Accumulators.iAccumulationCyclesCount == (pMetricsRef->iMaxAccumulationCycles - 1)) &&
            (pMetricsRef->Accumulators.Idpps.uCount != 0))
        {
            int32_t iResult = 0;

            iResult = ServerMetricsFormatEx(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat,
                &(pMetricsRef->Accumulators.Idpps.aValues[0]), pMetricsRef->Accumulators.Idpps.uCount,
                SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);

            if (iResult < 0)
            {
                if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
                {
                    // set flag used to detect that we are being re-executed after back buffer expansion
                    pMetricsRef->Accumulators.Idpps.bPendingBackBufferExpansion = TRUE;
                }

                // set offset to the error and break
                iOffset = iResult;
            }
            else
            {
                iOffset += iResult;
            }
        }
    }

    // if an error occured clear everything that was written to the buffer 
    if (iOffset < 0)
    {
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsIdpps

    \Description
        Adds the metric to the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/17/2019 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsIdpps(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    if (pMetricsRef->iMaxAccumulationCycles == 0)
    {
        return(_GameServerOiMetricsIdppsWithoutAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
    else
    {
        return(_GameServerOiMetricsIdppsWithAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsOdmppsWithoutAccumulation

    \Description
        Adds one sample contributing to the metric into the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsOdmppsWithoutAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint32_t uCurrentCount;
    uint32_t uRate = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;

    #if DIRTYVERS >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 0)
    // trama code line and beyond
    if ((iResult = GameServerDistStatus(pDist, 'ocnt', 0, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
    #else
    // latest releases of shrike code line
    if ((iResult = GameServerDistStatus(pDist, 'odmp', 0, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
    #endif
    {
        // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
        if ((iResult == 0) && (pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized == TRUE))
        {
            // perform rate calculation
            _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.uOutputMultiDistPacketCount, GameServerDistStatus(pDist, 'clnu', 0, NULL, 0), &uRate);

            iOffset = ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uRate, SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);
        }

        if (iResult == 1)
        {
            DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid odmpps sample\n");
        }

        // save down previous value
        pMetricsRef->MetricsData.uOutputMultiDistPacketCount = uCurrentCount;
        pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized = TRUE;
    }
    else
    {
        // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
        pMetricsRef->MetricsData.uOutputMultiDistPacketCount = 0;
        pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized = FALSE;
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsOdmppsWithAccumulation

    \Description
        Adds the metric to the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsOdmppsWithAccumulation(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint32_t uCurrentCount=0;
    uint32_t uRate = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;

    #if DIRTYVERS >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 0)
    // trama code line and beyond
    if ((iResult = GameServerDistStatus(pDist, 'ocnt', 0, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
    #else
    // latest releases of shrike code line
    if ((iResult = GameServerDistStatus(pDist, 'odmp', 0, &uCurrentCount, sizeof(uCurrentCount))) >= 0)
    #endif
    {
        // calculate rate only if return code flags sampling interval as valid and if previous value has been initialized already
        if ((iResult == 0) && (pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized == TRUE))
        {
            // perform rate calculation
            _GameServerOiMetricsRate(pMetricsRef, uCurrentCount, pMetricsRef->MetricsData.uOutputMultiDistPacketCount, GameServerDistStatus(pDist, 'clnu', 0, NULL, 0), &uRate);

            if (pMetricsRef->Accumulators.Odmpps.uCount < GAMESERVER_OI_ACCUMULATOR_MAX_SIZE)
            {
                // avoid accumulating twice if we are being re-executed after back buffer expansion
                if (pMetricsRef->Accumulators.Odmpps.bPendingBackBufferExpansion == FALSE)
                {
                    pMetricsRef->Accumulators.Odmpps.aValues[pMetricsRef->Accumulators.Odmpps.uCount++] = uRate;
                }

                // is it time to flush out the accumulated values?
                if (pMetricsRef->Accumulators.iAccumulationCyclesCount == (pMetricsRef->iMaxAccumulationCycles - 1))
                {
                    iOffset = ServerMetricsFormatEx(pBuffer, iBufSize, pMetricName, eFormat,
                        &pMetricsRef->Accumulators.Odmpps.aValues[0], pMetricsRef->Accumulators.Odmpps.uCount,
                        SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions);

                    if (iOffset == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
                    {
                        // set flag used to detect that we are being re-executed after back buffer expansion
                        pMetricsRef->Accumulators.Odmpps.bPendingBackBufferExpansion = TRUE;
                    }
                }
            }
            else
            {
                iOffset = SERVERMETRICS_ERR_ACCUMULATION_OVERFLOW;
            }
        }

        if (iResult == 1)
        {
            DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 1, "gameserveroimetrics: skipped invalid odmpps sample\n");
        }

        // save down previous value
        pMetricsRef->MetricsData.uOutputMultiDistPacketCount = uCurrentCount;
        pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized = TRUE;
    }
    else
    {
        // mark count as uninitialized to avoid next cycle diffing readings that are not back-to-back
        pMetricsRef->MetricsData.uOutputMultiDistPacketCount = 0;
        pMetricsRef->MetricsData.bOutputMultiDistPacketCountInitialized = FALSE;
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsOdmpps

    \Description
        Adds the metric to the specified buffer.

    \Input *pMetricsRef         - module state
    \Input *pDist               - dist module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsOdmpps(GameServerOiMetricsRefT *pMetricsRef, GameServerDistT* pDist, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    if (pMetricsRef->iMaxAccumulationCycles == 0)
    {
        return(_GameServerOiMetricsOdmppsWithoutAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
    else
    {
        return(_GameServerOiMetricsOdmppsWithAccumulation(pMetricsRef, pDist, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions));
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerOiMetricsCallback

    \Description
        Callback to format game server stats into a datagram to be sent to an
        external metrics aggregation

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN] GameServerOiMetricsRefT module state

    \Output
        int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerOiMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    GameServerOiMetricsRefT* pMetricsRef = (GameServerOiMetricsRefT *)pUserData;
    GameServerDistT* pDist = GameServerBlazeGetGameServerDist();
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strCommonDimensions[512];
    uint32_t uCurrentTick = NetTick();
    int32_t iExpectedTickDelta = pMetricsRef->pConfig->uMetricsPushingRate + GameServerStatus(pMetricsRef->pGameServer, 'rate', 0, NULL, 0) + 10;

    if (pMetricsRef->bLastIntervalTickValid == TRUE)
    {
        int32_t iTickDelta = NetTickDiff(uCurrentTick, pMetricsRef->uLastIntervalTick);

        if (iTickDelta > iExpectedTickDelta)
        {
            DirtyCastLogPrintf("gameserveroimetrics: last metrics reporting interval was larger (%d ms) than expected (%d ms) - expect high inputdrop rate and (maybe) an idpps down/up glitch over 2 intervals\n", 
                iTickDelta, iExpectedTickDelta);
        }
    }
    pMetricsRef->uLastIntervalTick = uCurrentTick;
    pMetricsRef->bLastIntervalTickValid = TRUE;

    // prepare dimensions to be associated with all metrics
    ds_memclr(strCommonDimensions, sizeof(strCommonDimensions));
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), GAMESERVER_OI_METRICS_COMMON_DIMENSIONS_FORMAT,
        pMetricsRef->pConfig->strDeployInfo2,
        pMetricsRef->pConfig->strHostName,
        pMetricsRef->pConfig->strPingSite,
        pMetricsRef->pConfig->strServer,
        pMetricsRef->pConfig->strEnvironment,
        GameServerBlazeStatus('ssst', 0, NULL, 0));

    // do not send metrics if pDist is null or game is not started
    if ((pDist != NULL) && GameServerStatus(pMetricsRef->pGameServer, 'gsta', 0, NULL, 0))
    {
        // for now gameserveroimetrics doesn't make use of any common metrics, so we just loop through our own
        while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_GameServerOiMetricsHandlers))
        {
            int32_t iResult;
            const GameServerOiMetricsHandlerEntryT *pEntry = &_GameServerOiMetricsHandlers[iMetricIndex];

            if ((iResult = pEntry->pMetricHandler(pMetricsRef, pDist, pEntry->strMetricName, pMetricsRef->pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 3, "gameserveroimetrics: adding metric %s (#%d) to report succeeded\n", _GameServerOiMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
                iOffset += iResult;
            }
            else if (iResult == 0)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 3, "gameserveroimetrics: adding metric %s (#%d) to report skipped because metric unavailable\n", _GameServerOiMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
            }
            else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 3, "gameserveroimetrics: adding metric %s (#%d) to report failed because it did not fit - back buffer expansion required\n", _GameServerOiMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);

                // return "scaled" metric index
                iRetCode = iMetricIndex;
                break;
            }
            else
            {
                DirtyCastLogPrintf("gameserveroimetrics: adding metric %s (#%d) to report failed with error %d - metric skipped!\n", _GameServerOiMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
                iMetricIndex++;
            }
        }
    }

    // let the caller know how many bytes were written to the buffer
    *pSize = iOffset;

    // if all metrics have been dealt with, then let the caller know
    if (iMetricIndex == DIRTYCAST_CalculateArraySize(_GameServerOiMetricsHandlers))
    {
        // reset stat when we are done
        GameServerDistControl(pDist, 'rsta', 0, NULL, 0);

        // if multi-value line feature is used, then count the accummulation cycles properly
        if (pMetricsRef->iMaxAccumulationCycles != 0)
        {
            pMetricsRef->Accumulators.iAccumulationCyclesCount++;

            // is it time to reset accumulators
            if (pMetricsRef->Accumulators.iAccumulationCyclesCount == pMetricsRef->iMaxAccumulationCycles)
            {
                DirtyCastLogPrintfVerbose(pMetricsRef->pConfig->uDebugLevel, 3, "gameserveroimetrics: resetting metrics accumulators\n");
                ds_memclr(&pMetricsRef->Accumulators, sizeof(pMetricsRef->Accumulators));
            }
        }

        iRetCode = 0;
    }

    return(iRetCode);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function GameServerOiMetricsRefT

    \Description
        Create the GameServerOiMetricsRefT module.

    \Input *pGameServer   - gameserver ref

    \Output
        GameServerOiMetricsRefT * - state pointer on success, NULL on failure

    \Version 07/31/2019 (tcho)
 */
/********************************************************************************F*/
GameServerOiMetricsRefT *GameServerOiMetricsCreate(GameServerRefT *pGameServer)
{
    GameServerOiMetricsRefT* pMetricsRef;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemGroupEnter(iMemGroup, pMemGroupUserData);

    // allocate and init module state
    if ((pMetricsRef = (GameServerOiMetricsRefT *)DirtyMemAlloc(sizeof(*pMetricsRef), GAMESERVER_OI_METRIC_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemGroupLeave();
        DirtyCastLogPrintf(("gameserveroimetrics: could not allocate module state.\n"));
        return(NULL);
    }
    ds_memclr(pMetricsRef, sizeof(*pMetricsRef));

    // retrieve config from gameserver
    GameServerStatus(pGameServer, 'conf', 0, &pMetricsRef->pConfig, sizeof(pMetricsRef->pConfig));

    // create module to report metrics to an external metrics aggregator
    if ((pMetricsRef->pServerMetrics = ServerMetricsCreate(pMetricsRef->pConfig->strMetricsAggregatorHost, pMetricsRef->pConfig->uMetricsAggregatorPort, pMetricsRef->pConfig->uMetricsPushingRate, pMetricsRef->pConfig->eMetricsFormat, &_GameServerOiMetricsCallback, pMetricsRef)) != NULL)
    {
        DirtyCastLogPrintf("gameserveroimetrics: created module for reporting metrics to external aggregator (report rate = %d ms)\n", pMetricsRef->pConfig->uMetricsPushingRate);
        ServerMetricsControl(pMetricsRef->pServerMetrics, 'spam', pMetricsRef->pConfig->uDebugLevel, 0, NULL);
    }
    else
    {
        GameServerOiMetricsDestroy(pMetricsRef);
        DirtyCastLogPrintf("gameserveroimetrics: unable to create module for reporting metrics to external aggregator\n");
        return(NULL);
    }

    //init
    pMetricsRef->pMemGroupUserData = pMemGroupUserData;
    pMetricsRef->iMemGroup = iMemGroup;
    pMetricsRef->iMaxAccumulationCycles = 0; // default to 'multi-value lines not used in metrics payload'
    pMetricsRef->pGameServer = pGameServer;

    return(pMetricsRef);
}

/*F********************************************************************************/
/*!
    \Function GameServerOiMetricsDestroy

    \Description
        Destroy the GameServerOiMetricsRefT module.

    \Input *pMetricsRef - module state

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
void GameServerOiMetricsDestroy(GameServerOiMetricsRefT *pMetricsRef)
{
    // release module memory
    if (pMetricsRef->pServerMetrics != NULL)
    {
        ServerMetricsDestroy(pMetricsRef->pServerMetrics);
    }

    DirtyMemFree(pMetricsRef, GAMESERVER_OI_METRIC_MEMID, pMetricsRef->iMemGroup, pMetricsRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function GameServerOiMetricsUpdate

    \Description
        Service communication with the metrics agent

    \Input *pMetrics  - module state

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
void GameServerOiMetricsUpdate(GameServerOiMetricsRefT* pMetricsRef)
{
    ServerMetricsUpdate(pMetricsRef->pServerMetrics);
}

/*F********************************************************************************/
/*!
    \Function GameServerOiMetricsControl


    \Description
        Set module behavior based on input selector.


    \Input *pMetrics  - module state
    \Input iControl   - input selector
    \Input iData      - selector input
    \Input *pBuf      - selector input
    \Input iBufSize   - selector input

    \Output
        int32_t       - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            accl: set the limit count of accumulated metrics
            gend: clean up internal data structures upon game end
            spam: set the spam level
        \endverbatim

    \Version 10/29/2019 (tcho)
*/
/********************************************************************************F*/
int32_t GameServerOiMetricsControl(GameServerOiMetricsRefT *pMetricsRef, int32_t iControl, int32_t iData, void *pBuf, int32_t iBufSize)
{
    // set the limit count of accumulated metrics
    if (iControl == 'accl')
    {
        if (iData > GAMESERVER_OI_ACCUMULATOR_MAX_SIZE)
        {
            iData = GAMESERVER_OI_ACCUMULATOR_MAX_SIZE;
        }
        DirtyCastLogPrintf("gameserveroimetrics: maximum number of metrics accumulation cycles changed from %d to %d\n", pMetricsRef->iMaxAccumulationCycles, iData);
        pMetricsRef->iMaxAccumulationCycles = iData;
        return(0);
    }

    // clean up internal data structures upon game end
    if (iControl == 'gend')
    {
        ds_memclr(&pMetricsRef->MetricsData, sizeof(pMetricsRef->MetricsData));

        /* Drop the last samples that remained in the accumulators and did not make it into a 
           packet to the metrics agent. This is done to avoid these metrics being added later in time
           to the first packet sent to the metrics agent at the beginning of the next game. They
           would still be valid samples but they would be chronologically incorrect. 
           Dropping them sounds like a better solution. Missing few samples from very end of game
           does not sound like a big deal as it does not compromise the integrity of 
           the metric to which they contribute. */
        ds_memclr(&pMetricsRef->Accumulators, sizeof(pMetricsRef->Accumulators));

        return(0);
    }

    // set spam level
    if (iControl == 'spam')
    {
        if (pMetricsRef->pServerMetrics != NULL)
        {
            ServerMetricsControl(pMetricsRef->pServerMetrics, 'spam', iData, 0, NULL);
            return(0);
        }
    }

    return(-1);
}
