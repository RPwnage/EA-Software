/*H********************************************************************************/
/*!
    \File gameservermetrics.c

    \Description
        This module implements the metrics datagram preparation for the voiserver.
        Specific to gameserver operating mode (not connection concierge operating mode)

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "gameservermetrics.h"
#include "commonmetrics.h"
#include "voipgameserver.h"
#include "voipserver.h"
#include "voipserverconfig.h"


/*** Defines **********************************************************************/

//! format of the dimensions string, if this value changes be sure to update the size calculation where it is used
#define GAMESERVER_DIMENSIONS_FORMAT ("%s,prototunnelver:%u.%u")

//! format of the dimensions string, if this value changes to sure to update the size calculation and values where it is used
#define GAMESERVER_COMMON_DIMENSIONS_FORMAT ("host:%s,port:%d,mode:gameserver,product:%s,deployinfo:%s,deployinfo2:%s,site:%s,env:%s,subspace:%d")

/*** Type Definitions *************************************************************/

//! metric handler type
typedef int32_t(GameServerMetricHandlerT)(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

typedef struct GameServerRemovalReasonEntryT
{
    uint32_t uCount;
    GameServerClientRemovalReasonT *removalReasonEntry;
} GameServerRemovalReasonEntryT;

//! per-metric handler
typedef struct GameServerMetricHandlerEntryT
{
    const char strMetricName[64];
    GameServerMetricHandlerT *const pMetricHandler;
    const uint32_t uInterval;
    uint32_t uWhen;
} GameServerMetricHandlerEntryT;


/*** Function Prototypes ***************************************************************/

static int32_t _GameServerMetricsAutoscalePct(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsClientsConnected(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsClientsSupported(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesAvailable(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesStuck(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesOffline(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesDisconnected(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesSupported(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsClientsRemoved(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsClientRemoveReason(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsGamesEnded(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsRlmtChangeCount(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsInbLateMin(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsInbLateMax(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsInbLateAvg(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsOtbLateMin(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsOtbLateMax(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsOtbLateAvg(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsRpackLost(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _GameServerMetricsLpackLost(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

/*** Variables ********************************************************************/

static GameServerMetricHandlerEntryT _GameServerMetrics_MetricHandlers[] =
{
    { "invalid",                                    NULL, 0, 0 },
    { "voipmetrics.autoscale.pct",                  _GameServerMetricsAutoscalePct, 0, 0 },
    { "voipmetrics.clients.active",                 _GameServerMetricsClientsConnected, 0, 0 },
    { "voipmetrics.clients.maximum",                _GameServerMetricsClientsSupported, 0, 0 },
    { "voipmetrics.games.available",                _GameServerMetricsGamesAvailable, 0, 0 },
    { "voipmetrics.games.stuck",                    _GameServerMetricsGamesStuck, 0, 0 },
    { "voipmetrics.games.offline",                  _GameServerMetricsGamesOffline, 0, 0 },
    { "voipmetrics.games.disconnected",             _GameServerMetricsGamesDisconnected, 0, 0 },
    { "voipmetrics.games.maximum",                  _GameServerMetricsGamesSupported, 0, 0 },
    { "voipmetrics.gamestats.clients.removed",      _GameServerMetricsClientsRemoved, 0, 0 },
    { "voipmetrics.gamestats.clients.removereason", _GameServerMetricsClientRemoveReason, 0, 0 },
    { "voipmetrics.gamestats.games.ended",          _GameServerMetricsGamesEnded, 0, 0 },
    { "voipmetrics.rlmt.count",                     _GameServerMetricsRlmtChangeCount, 0, 0 },
    { "voipmetrics.inblate.min",                    _GameServerMetricsInbLateMin, 0, 0 },
    { "voipmetrics.inblate.max",                    _GameServerMetricsInbLateMax, 0, 0 },
    { "voipmetrics.inblate.avg",                    _GameServerMetricsInbLateAvg, 0, 0 },
    { "voipmetrics.otblate.min",                    _GameServerMetricsOtbLateMin, 0, 0 },
    { "voipmetrics.otblate.max",                    _GameServerMetricsOtbLateMax, 0, 0 },
    { "voipmetrics.otblate.avg",                    _GameServerMetricsOtbLateAvg, 0, 0 },
    { "voipmetrics.rpacklost",                      _GameServerMetricsRpackLost,  0, 0 },
    { "voipmetrics.lpacklost",                      _GameServerMetricsLpackLost,  0, 0 }
};

static const char *_GameServerMetrics_GameStatesStr[] =
{
    "pregame",      // GAMESERVER_GAMESTATE_PREGAME
    "ingame",       // GAMESERVER_GAMESTATE_INGAME
    "postgame",     // GAMESERVER_GAMESTATE_POSTGAME
    "migrating",    // GAMESERVER_GAMESTATE_MIGRATING
    "other"         // GAMESERVER_GAMESTATE_OTHER
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsClientsConnected

    \Description
        Adds the "voipmetrics.client.active" metric to the specified buffer.
        Metric description: current number of clients that are serviced by the voipserver

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsClientsConnected(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strProtoTunnelVersions[128];
    char strProtoTunnelVersion[8];
    int32_t iProtoTunnelVersionIndex = 0;
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iDimensionsStringLen;
    char *pScratchPad = NULL;

    // query the versions supported by prototunnel
    ProtoTunnelStatus(VoipServerGetProtoTunnel(VoipGameServerGetBase(pVoipGameServer)), 'vset', 0, strProtoTunnelVersions, sizeof(strProtoTunnelVersions));

    // get an appropriately sized scratch pad
    iDimensionsStringLen = (int32_t)strnlen(pCommonDimensions, iBufSize) +                            // length of pCommonDimensions
        21 +                                                                                          // max length of uProtoTunnelVersMaj
        21 +                                                                                          // max length of uProtoTunnelVersMin
        (int32_t)strlen(GAMESERVER_DIMENSIONS_FORMAT);                                                // length of the remainder of the format string
    pScratchPad = ServerMetricsGetScratchPad(VoipServerGetServerMetrics(VoipGameServerGetBase(pVoipGameServer)), iDimensionsStringLen);

    // loop through all the versions found
    while ((iErrorCode == 0) && TagFieldGetDelim(strProtoTunnelVersions, strProtoTunnelVersion, sizeof(strProtoTunnelVersion), "", iProtoTunnelVersionIndex, ',') != 0)
    {
        uint32_t uProtoTunnelVersMaj, uProtoTunnelVersMin;
        uint16_t uProtoTunnelVersion;
        uint64_t uActiveClients;
        int32_t iResult;

        // convert string version to numerical version
        sscanf(strProtoTunnelVersion, "%u.%u", &uProtoTunnelVersMaj, &uProtoTunnelVersMin);
        uProtoTunnelVersion = (uint8_t)(uProtoTunnelVersMaj) << 8;
        uProtoTunnelVersion |= (uint8_t)(uProtoTunnelVersMin);

        // query number of active clients for the version
        uActiveClients = (uint64_t)ProtoTunnelStatus(VoipServerGetProtoTunnel(VoipGameServerGetBase(pVoipGameServer)), 'actv', uProtoTunnelVersion, NULL, 0);

        if (pScratchPad != NULL)
        {
            //  add the prototunnel version to the dimensions
            ds_memclr(pScratchPad, iDimensionsStringLen);
            ds_snzprintf(pScratchPad, iDimensionsStringLen, GAMESERVER_DIMENSIONS_FORMAT, pCommonDimensions, uProtoTunnelVersMaj, uProtoTunnelVersMin);

            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, uActiveClients, SERVERMETRICS_TYPE_GAUGE, 100, pScratchPad)) >= 0)
            {
                // count of active clients for specified prototunnel version successfully added
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
            iErrorCode = SERVERMETRICS_ERR_OUT_OF_MEMORY;
        }

        iProtoTunnelVersionIndex++;
    }

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
    \Function _GameServerMetricsClientsSupported

    \Description
        Adds the "voipmetrics.clients.maximum" metric to the specified buffer.
        Metric description: maximum number of clients that can be serviced by the voipserver

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsClientsSupported(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(VoipGameServerGetBase(pVoipGameServer));

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pConfig->iMaxClients, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsGamesAvailable

    \Description
        Adds the "voipmetrics.games.available" metric to the specified buffer.
        Metric description: maximum number of "games" that can be serviced by the voipserver

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsGamesAvailable(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipGameServerStats->iGamesAvailable, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsAutoscalePct

    \Description
        Adds the "voipmetrics.autoscale.pct" metric to the specified buffer.
        Metric description: the percentage of games being used by the VS, 0.0->1.0f

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/24/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _GameServerMetricsAutoscalePct(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(VoipGameServerGetBase(pVoipGameServer));
    int32_t uGamesActive = VoipTunnelStatus(VoipServerGetVoipTunnel(VoipGameServerGetBase(pVoipGameServer)), 'ngam', 0, NULL, 0);
    float fGamesPct = (float)uGamesActive / (float)pConfig->iMaxGameServers;

    return(ServerMetricsFormatFloat(pBuffer, iBufSize, pMetricName, eFormat, fGamesPct, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsGamesStuck

    \Description
        Adds the "voipmetrics.games.stuck" metric to the specified buffer.
        Metric description: current number of stuck games

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsGamesStuck(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipGameServerStats->iGamesStuck, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsGamesOffline

    \Description
        Adds the "voipmetrics.games.offline" metric to the specified buffer.
        Metric description: current number of offline games

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsGamesOffline(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipGameServerStats->iGamesOffline, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsGamesDisconnected

    \Description
        Adds the "voipmetrics.games.disconnected" metric to the specified buffer.
        Metric description: number of disconnected games over the sampling interval

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsGamesDisconnected(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipGameServerStats->uGameServDiscCount, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsGamesSupported

    \Description
        Adds the "voipmetrics.games.maximum" metric to the specified buffer.
        Metric description: maximum supported amount of games supported by the voipserver

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/15/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _GameServerMetricsGamesSupported(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uGameMax = 0;
    if (VoipServerStatus(VoipGameServerGetBase(pVoipGameServer), 'psta', 0, NULL, 0) == VOIPSERVER_PROCESS_STATE_RUNNING)
    {
        const VoipServerConfigT *pConfig = VoipServerGetConfig(VoipGameServerGetBase(pVoipGameServer));
        uGameMax = pConfig->iMaxGameServers;
    }
    else
    {
        // while draining and exiting the maximum number of game servers is less than the configured max
        uGameMax = pVoipGameServerStats->iGamesAvailable + pVoipGameServerStats->iGamesActive;
    }
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uGameMax, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
\Function _GameServerMetricsClientRemoveReason

    \Description
        Adds the "voipmetrics.gamestats.clients.removereason.<reamovalreason>" metrics to the specified buffer.
        Metric description: tracks the number of clients per removal reason

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 11/28/2017 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerMetricsClientRemoveReason(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint32_t uIndex, uLastIndex;
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    char strMetricName[64];

    // count the total amounts for each removal reason
    for (uIndex = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
    {
        GameServerClientRemovalInfoT *pRemovalInfo = NULL;
        GameServerClientRemovalInfoT *pLastRemoveInfo = NULL;
        int32_t iGameState;
        int32_t iResult;
        uint32_t uDelta = 0;

        pRemovalInfo = &pVoipGameServerStats->aClientRemovalInfo[uIndex];

        // if removal reson is 0 skip
        if (pRemovalInfo->RemovalReason.uRemovalReason == 0)
        {
            continue;
        }

        // find a matching last removal info
        for (uLastIndex = 0; uLastIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uLastIndex += 1)
        {
            if (pRemovalInfo->RemovalReason.uRemovalReason == pVoipGameServerStats->aLastClientRemovalInfo[uIndex].RemovalReason.uRemovalReason)
            {
                pLastRemoveInfo = &pVoipGameServerStats->aLastClientRemovalInfo[uIndex];
            }
        }

        if (pLastRemoveInfo == NULL)
        {
            // if last removal info is null delta is the current count
            for (iGameState = 0; iGameState < GAMESERVER_NUMGAMESTATES; iGameState += 1)
            {
                uDelta += pRemovalInfo->aRemovalStateCt[iGameState];
            }
        }
        else
        {
            // if last removal is not null delta is the difference between current and last removal info
            for (iGameState = 0; iGameState < GAMESERVER_NUMGAMESTATES; iGameState += 1)
            {
                uDelta += (pRemovalInfo->aRemovalStateCt[iGameState] - pLastRemoveInfo->aRemovalStateCt[iGameState]);
            }
        }

        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, pRemovalInfo->RemovalReason.strRemovalReason);

        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }

        if (iErrorCode)
        {
            iOffset = iErrorCode;
            ds_memclr(pBuffer, iBufSize);
            break;
        }
    }
    
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsClientsRemoved

    \Description
        Adds the "voipmetrics.clients.removed.<state>" metrics to the specified buffer.
        Metric description: number of clients removed since start while in specified game state

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsClientsRemoved(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    uint32_t uGameState = 0;

    // loop through all the versions found
    while ((iErrorCode == 0) && uGameState != GAMESERVER_NUMGAMESTATES)
    {
        GameServerClientRemovalInfoT *pRemovalInfo;
        uint32_t uIndex;
        uint64_t uClientsRemoved = 0, uLastClientsRemoved = 0, uClientsRemovedDelta;
        int32_t iResult;
        char strMetricName[64];

        // generate summary statistic
        for (uIndex = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
        {
            pRemovalInfo = &pVoipGameServerStats->aClientRemovalInfo[uIndex];
            if (pRemovalInfo->RemovalReason.strRemovalReason[0] != '\0')
            {
                uClientsRemoved += pRemovalInfo->aRemovalStateCt[uGameState];
            }

            pRemovalInfo = &pVoipGameServerStats->aLastClientRemovalInfo[uIndex];
            if (pRemovalInfo->RemovalReason.strRemovalReason[0] != '\0')
            {
                uLastClientsRemoved += pRemovalInfo->aRemovalStateCt[uGameState];
            }
        }
        uClientsRemovedDelta = uClientsRemoved - uLastClientsRemoved;

        // add game state suffix to metric name
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, _GameServerMetrics_GameStatesStr[uGameState]);
        
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uClientsRemovedDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions)) >= 0)
        {
            // count of removed clients for specified game state added
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }

        uGameState++;
    }

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
    \Function _GameServerMetricsGamesEnded

    \Description
        Adds the "voipmetrics.games.ended.<state>" metrics to the specified buffer.
        Metric description: number of games ended since start while in specified game state

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsGamesEnded(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    uint32_t uGameState = 0;

    // loop through all the versions found
    while ((iErrorCode == 0) && uGameState != GAMESERVER_NUMGAMESTATES)
    {
        char strMetricName[64];
        int32_t iResult;
        const uint64_t uGameStateCntDelta = pVoipGameServerStats->aGameEndStateCt[uGameState] - pVoipGameServerStats->aLastGameEndStateCt[uGameState];

        // add game state suffix to metric name
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, _GameServerMetrics_GameStatesStr[uGameState]);

        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uGameStateCntDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions)) >= 0)
        {
            // count of ended games for specified game state added
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }

        uGameState++;
    }

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
    \Function _GameServerMetricsRlmtChangeCount

    \Description
        Adds the "voipmetrics.rlmt.count" metric to the specified buffer.
        Metric description: number of times the rlmt feature was exercised over the sampling interval

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsRlmtChangeCount(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipGameServerStats->uAccRlmtChangeCount, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsInbLateMin

    \Description
        Adds the "voipmetrics.inblate.min" metric to the specified buffer.
        Metric description: minimum inbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsInbLateMin(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.InbLate.uMinLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsInbLateMax

    \Description
        Adds the "voipmetrics.inblate.max" metric to the specified buffer.
        Metric description: maximum inbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsInbLateMax(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.InbLate.uMaxLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsInbLateAvg

    \Description
        Adds the "voipmetrics.inblate.avg" metric to the specified buffer.
        Metric description: average inbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsInbLateAvg(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uInbLateAvg;

    // calculate metric value
    uInbLateAvg = (uint64_t)((pVoipServerStats->StatInfo.InbLate.uNumStat != 0) ? pVoipServerStats->StatInfo.InbLate.uSumLate / pVoipServerStats->StatInfo.InbLate.uNumStat : 0);
    
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uInbLateAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsOtbLateMin

    \Description
        Adds the "voipmetrics.otblate.min" metric to the specified buffer.
        Metric description: minimum outbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsOtbLateMin(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.OtbLate.uMinLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsOtbLateMax

    \Description
        Adds the "voipmetrics.otblate.max" metric to the specified buffer.
        Metric description: maximum outbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsOtbLateMax(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.OtbLate.uMaxLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsOtbLateAvg

    \Description
        Adds the "voipmetrics.otblate.avg" metric to the specified buffer.
        Metric description: average outbound latency

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
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
static int32_t _GameServerMetricsOtbLateAvg(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uOtbLateAvg;

    // calculate metric value
    uOtbLateAvg = (uint64_t)((pVoipServerStats->StatInfo.OtbLate.uNumStat != 0) ? pVoipServerStats->StatInfo.OtbLate.uSumLate / pVoipServerStats->StatInfo.OtbLate.uNumStat : 0);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uOtbLateAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsRpackLost

    \Description
        Adds the "voipmetrics.rpacklost" metric to the specified buffer.
        Metric description: remote packet lost percentage

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 08/19/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerMetricsRpackLost(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    float fRpackLost;

    // calculate metric value
    fRpackLost = (pVoipGameServerStats->DeltaPackLostSent.uLpackSent != 0) ? ((float)pVoipGameServerStats->DeltaPackLostSent.uRpackLost * 100.0f / (float)pVoipGameServerStats->DeltaPackLostSent.uLpackSent) : 0;

    return(ServerMetricsFormatFloat(pBuffer, iBufSize, pMetricName, eFormat, fRpackLost, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _GameServerMetricsLpackLost

    \Description
        Adds the "voipmetrics.lpacklost" metric to the specified buffer.
        Metric description: local packet lost percentage

    \Input *pVoipGameServer     - module state
    \Input *pVoipServerStats    - voipserver stats
    \Input *pVoipGameServerStats- gameserver stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 08/19/2019 (tcho)
*/
/********************************************************************************F*/
static int32_t _GameServerMetricsLpackLost(VoipGameServerRefT *pVoipGameServer, VoipServerStatT *pVoipServerStats, VoipGameServerStatT *pVoipGameServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    float fLpackLost;

    // calculate metric value
    fLpackLost = (pVoipGameServerStats->DeltaPackLostSent.uRpackSent != 0) ? ((float)pVoipGameServerStats->DeltaPackLostSent.uLpackLost * 100.0f / (float)pVoipGameServerStats->DeltaPackLostSent.uRpackSent) : 0;

    return(ServerMetricsFormatFloat(pBuffer, iBufSize, pMetricName, eFormat, fLpackLost, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function GameServertMetricsCallback

    \Description
        Callback to format voipserver stats into a datagram to be sent to an
        external metrics aggregator

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amout of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN] VoipServer module state

    \Output
        int32_t             - 0: complete;  >0: value to specify in iMetricIndex for next cain;  -1:error

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
int32_t GameServerMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    VoipGameServerRefT *pVoipGameServer = (VoipGameServerRefT *)pUserData;
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strHostName[128];
    char strProductId[GAMESERVER_MAX_PRODUCTID_LENGTH];
    char strCommonDimensions[sizeof(GAMESERVER_COMMON_DIMENSIONS_FORMAT) +  // size of the format string
                             sizeof(strHostName) +                          // size of the hostname
                             5 +                                            // size of the port
                             GAMESERVER_MAX_PRODUCTID_LENGTH +              // size of the product id value
                             VOIPSERVERCONFIG_DEPLOYINFO_SIZE +             // size of deployinfo value
                             VOIPSERVERCONFIG_DEPLOYINFO2_SIZE +            // size of deployinfo2 value
                             VOIPSERVERCONFIG_SITE_SIZE +                   // size of site value
                             VOIPSERVERCONFIG_ENV_SIZE +                    // size of env value
                                                                            // note subspace value '0' or '1', is less than the size of the format %d (no additional space is needed)
                             1];                                            // NULL

    //describe the order the different metrics sources are added to our list
    const int32_t iTrafficMetricsStart = 0;
    const int32_t iSystemMetricsStart = CommonTrafficMetricsGetCount();
    const int32_t iGameserverMetricsStart = CommonTrafficMetricsGetCount() + CommonSystemMetricsGetCount();

    VoipServerStatus(pVoipServer, 'host', 0, strHostName, sizeof(strHostName));

    // query voipserver product id
    VoipGameServerStatus(pVoipGameServer, 'prod', 0, strProductId, sizeof(strProductId));

    // early out since we are not ready to send metrics
    if (ds_stricmp(strProductId, "uninitialized") == 0)
    {
        *pSize = 0; // we wrote no bytes
        
        // reset the metrics, to throw them out
        VoipServerControl(pVoipServer, 'stat', TRUE, 0, NULL);
        VoipGameServerControl(pVoipGameServer, 'stat', TRUE, 0, NULL);
        return(0);  // we are done with reporting metrics on this reporting cycle
    }

    // prepare dimensions to be associated with all metrics
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), GAMESERVER_COMMON_DIMENSIONS_FORMAT, 
                                                                   strHostName, 
                                                                   pConfig->uDiagnosticPort,
                                                                   strProductId, 
                                                                   pConfig->strDeployInfo,
                                                                   pConfig->strDeployInfo2,
                                                                   pConfig->strPingSite,
                                                                   pConfig->strEnvironment,
                                                                   (pConfig->uSubspaceSidekickPort != 0) ? 1 : 0);


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
    else if ((iMetricIndex >= iSystemMetricsStart) && (iMetricIndex < iGameserverMetricsStart))
    {
        if (iMetricIndex == iSystemMetricsStart)   //the first index of a set is invalid
        {
            iMetricIndex++;
        }
        if ((iRetCode = CommonSystemMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex - iSystemMetricsStart, strCommonDimensions)) == 0)
        {
            // we are done with common system metrics, make sure we now take care of gameserver-specific metrics
            iMetricIndex = iRetCode = iGameserverMetricsStart;
            iOffset = *pSize;
        }
        else
        {
            iRetCode = iMetricIndex = iRetCode + iSystemMetricsStart;
        }
    }

    // is iMetricIndex pointing to a "gameserver-specific" metric?
    else if (iMetricIndex >= iGameserverMetricsStart)
    {
        // *** gameserver-specific metrics ***

        VoipGameServerStatT VoipGameServerStats;
        VoipServerStatT VoipServerStats;

        // get voipserver stats
        VoipGameServerStatus(pVoipGameServer, 'stat', 0, &VoipGameServerStats, sizeof(VoipGameServerStats));
        // get stats from voipserver
        VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));

        // rescale the metric index
        iMetricIndex -= iGameserverMetricsStart;
        if (iMetricIndex == 0)
        {
            iMetricIndex++;
        }

        // loop through
        while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_GameServerMetrics_MetricHandlers))
        {
            int32_t iResult;
            GameServerMetricHandlerEntryT *pEntry = &_GameServerMetrics_MetricHandlers[iMetricIndex];
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

            if ((iResult = pEntry->pMetricHandler(pVoipGameServer, &VoipServerStats, &VoipGameServerStats, pEntry->strMetricName, pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "gameservermetrics: adding gs metric %s (#%d) to report succeeded\n", _GameServerMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
                iOffset += iResult;
                pEntry->uWhen = pEntry->uInterval + uCurTick;
            }
            else if (iResult == 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "gameservermetrics: adding gs metric %s (#%d) to report skipped because metric unavailable\n", _GameServerMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
            }
            else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
            {
                // return "scaled" metric index
                iRetCode = iMetricIndex + iGameserverMetricsStart;
                break;
            }
            else
            {
                DirtyCastLogPrintf("gameservermetrics: adding gs metric %s (#%d) to report failed with error %d - metric skipped!\n", _GameServerMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
                iMetricIndex++;
            }
        }

        // let the caller know how many bytes were written to the buffer
        *pSize = iOffset;

        // if all metrics have been dealt with, then let the call know
        if (iMetricIndex == (signed)DIRTYCAST_CalculateArraySize(_GameServerMetrics_MetricHandlers))
        {
            iRetCode = 0;
        }
    }

    if (iRetCode == 0)
    {
        // tell the server to reset stats
        VoipServerControl(pVoipServer, 'stat', TRUE, 0, NULL);
        VoipGameServerControl(pVoipGameServer, 'stat', TRUE, 0, NULL);
    }

    return(iRetCode);
}
