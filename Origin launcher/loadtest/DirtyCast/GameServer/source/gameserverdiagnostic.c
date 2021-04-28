/*H********************************************************************************/
/*!
    \File gameserverdiagnostic.c

    \Description
        This module implements diagnostic page formatting for the GameServer.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/27/2010 (jbrookes) Split from gameserver.cpp
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#endif

#include "DirtySDK/dirtysock/dirtynet.h"
#include "dirtycast.h"
#include "gameserverblaze.h"
#include "gameserverconfig.h"
#include "gameserver.h"
#include "serveraction.h"
#include "serverdiagnostic.h"
#include "gameserverdiagnostic.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static const char* _aStateText[] = {
    "DISCONNECTED",
    "CONNECTING",
    "CONNECTED",
    "CREATED GAME",
    "WAITING FOR PLAYERS",
    "GAME STARTED"
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GameServerDiagnosticStatus

    \Description
        Callback to format gameserver stats for diagnostic request.

    \Input *pGameServer         - game server
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - size of buffer to format data into
    \Input uCurTime             - current time

    \Output
        int32_t                 - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerDiagnosticStatus(GameServerRefT *pGameServer, char *pBuffer, int32_t iBufSize, time_t uCurTime)
{
    static const char *_strVoipStates[] = { "INIT", "CONNECTING", "CONNECTED", "ONLINE", "DISCONNECTED" };
    const GameServerConfigT *pConfig = GameServerGetConfig(pGameServer);
    BufferedSocketInfoT BSocInfo;
    int32_t iOffset = 0, iLoginHistoryCnt;
    char strAddrText[20];
    GameServerLoginHistoryT aLoginHistory[GAMESERVER_LOGINHISTORYCNT];
    uint32_t uValue;

    // server versioning info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildversion: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildtime: %s\n", _ServerBuildTime);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildlocation: %s\n", _ServerBuildLocation);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "debuglevel: %d\n", pConfig->uDebugLevel);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "blazeserver: %s status=%s\n", pConfig->strServer, _aStateText[GameServerBlazeStatus('stat', 0, NULL, 0)]);

    // show login history
    if ((iLoginHistoryCnt = GameServerStatus(pGameServer, 'logn', 0, aLoginHistory, sizeof(aLoginHistory))) > 0)
    {
        uint32_t uKind, uCode;
        int32_t iHistory;
        char strTime[32];

        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "login failures:\n");

        for (iHistory = 0; iHistory < iLoginHistoryCnt; iHistory++)
        {
            uKind = aLoginHistory[iHistory].uFailKind;
            uCode = aLoginHistory[iHistory].uFailCode;
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   [%d] %c%c%c%c/%08x (%s) %s (%d consecutive)\n", iHistory,
                (uKind>>24&0xff),(uKind>>16&0xff),(uKind>>8&0xff),(uKind&0xff), uCode,
                GameServerBlazeGetErrorCodeName(uCode),
                DirtyCastCtime(strTime, sizeof(strTime), aLoginHistory[iHistory].uFailTime),
                aLoginHistory[iHistory].uFailCount);
        }
    }

    // more status
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "siteinfo: env=%s pingsite=%s \n", pConfig->strEnvironment, pConfig->strPingSite);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "comm redundancy (rltm) config: limit=%d threshold=%d timeout=%d period=%d\n",
        pConfig->uRedundancyLimit, pConfig->uRedundancyThreshold, pConfig->iRedundancyTimeout, pConfig->iRedundancyPeriod);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "voipserver: %s:%d status=%s\n", pConfig->strVoipServerAddr, pConfig->uVoipServerPort,
        _strVoipStates[GameServerStatus(pGameServer, 'vsta', 0, NULL, 0)]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "gameinfo: addr=%s rate=%dms\n", SocketInAddrGetText(GameServerStatus(pGameServer, 'addr', 0, NULL, 0), strAddrText, sizeof(strAddrText)),
        pConfig->uFixedUpdateRate);

    // get buffered socket info
    if (GameServerStatus(pGameServer, 'bnfo', 0, &BSocInfo, sizeof(BSocInfo)) == 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsockinfo: read=%u recv=%u rbyte=%u writ=%u sent=%d sbyte=%u maxr=%u maxs=%u\n",
            BSocInfo.uNumRead, BSocInfo.uNumRecv, BSocInfo.uDatRecv, BSocInfo.uNumWrit, BSocInfo.uNumSent, BSocInfo.uDatSent,
            BSocInfo.uMaxRBuf, BSocInfo.uMaxSBuf);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsockinfo: socket recv=%u, max socket recv=%u, socket send=%u, max socket send=%u\n", BSocInfo.uRcvQueue, BSocInfo.uMaxRQueue,
            BSocInfo.uSndQueue, BSocInfo.uMaxSQueue);
        if ((BSocInfo.uNumWrit > 0) && ((uValue = BSocInfo.uWriteDelay / BSocInfo.uNumWrit) > 0))
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsock warning: flush latency %dms\n", uValue);
        }
    }

    if (GameServerBlazeGetGameServerLink() != NULL)
    {
        // show individual client info
        iOffset = iOffset + GameServerLinkDiagnosticStatus(GameServerBlazeGetGameServerLink(), pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDiagnosticServulator

    \Description
        Callback to format gameserver stats for servulator diagnostic request.

    \Input *pGameServer     - game server module state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *uCurTime        - current time

    \Output
    int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _GameServerDiagnosticServulator(GameServerRefT *pGameServer, char *pBuffer, int32_t iBufSize, time_t uCurTime)
{
    int32_t iOffset = 0;
    int32_t iClient = 0;
    int32_t iPlayer = 0;

    // start filling in the message
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<servulator>\n");

    // if we've gotten this far we're init'ed
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Init>true</Init>\n");

    // Name of the process
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Name>Game Server</Name>\n");

    // Game Version
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Version>%s</Version>\n", _SDKVersion);

    // Program State
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <State>%s</State>\n", _aStateText[GameServerBlazeStatus('stat', 0, NULL, 0)]);

    // Game Mode
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <GameMode>na</GameMode>\n");

    // Ranked
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Ranked>na</Ranked>\n");

    // Private Game
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Private>na</Private>\n");

    // Number of clients
    int32_t iPlayerCount = 0;
    GameServerGameT lastGame;
    char playerList[GAMESERVER_MAX_PLAYERS * GAMESERVER_MAX_CLIENTS * (GAMESERVER_MAX_PERSONA_LENGTH+4)];

    GameServerBlazeStatus('lgam', 0, &lastGame, sizeof(lastGame));
    playerList[0] = '\0';
    for (iClient = 0; iClient < GAMESERVER_MAX_CLIENTS; iClient++)
    {
        for (iPlayer = 0; iPlayer < GAMESERVER_MAX_PLAYERS; iPlayer++)
        {
            if (strcmp(lastGame.aClients[iClient].aPlayers[iPlayer].strPers, "") != 0)
            {
                char playerElement[(GAMESERVER_MAX_PERSONA_LENGTH+4)];
                ds_snzprintf(playerElement, (GAMESERVER_MAX_PERSONA_LENGTH+4), "%d-%s ", iClient, lastGame.aClients[iClient].aPlayers[iPlayer].strPers);
                strcat(playerList, playerElement);
                ++iPlayerCount;
            }
        }
    }

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Players>%d</Players>\n", iPlayerCount);

    //  Number of observers
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Observers>0</Observers>\n");

    // List of Players
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <PlayerList>%s</PlayerList>\n", playerList);

    // List of Observers
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <ObserverList>na</ObserverList>\n");

    // done filling in the message
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</servulator>\n");

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDiagnosticAction

    \Description
        Take an action based on specified command

    \Input *pGameServer     - gameserver module state
    \Input *pStrCommand     - command to parse
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 10/15/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerDiagnosticAction(GameServerRefT *pGameServer, const char *pStrCommand, char *pBuffer, int32_t iBufSize)
{
    ServerActionT ServerAction;
    int32_t iOffset = 0;

    // parse action and value
    if (ServerActionParse(pStrCommand, &ServerAction) < 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unable to parse action\n");
        return(iOffset);
    }

    if (!strcmp(ServerAction.strName, "debuglevel"))
    {
        int32_t iDebugLevel;
        iOffset += ServerActionParseInt(pBuffer, iBufSize, &ServerAction, 0, 10, &iDebugLevel);
        GameServerControl(pGameServer, 'spam', iDebugLevel, 0, NULL);
    }
    else if (!strcmp(ServerAction.strName, "shutdown"))
    {
        int32_t iSignalFlags;
        ServerActionEnumT ShutdownEnum[] = { { "immediate", GAMESERVER_SHUTDOWN_IMMEDIATE }, { "graceful", GAMESERVER_SHUTDOWN_IFEMPTY } };
        iOffset += ServerActionParseEnum(pBuffer, iBufSize, &ServerAction, ShutdownEnum, sizeof(ShutdownEnum)/sizeof(ShutdownEnum[0]), &iSignalFlags);
        GameServerControl(pGameServer, 'sgnl', iSignalFlags, 0, NULL);
    }
#if DIRTYCODE_DEBUG
    else if (strncmp(ServerAction.strName, "fake_crash", 10) == 0)
    {
        int32_t *pCrash = NULL;
        int32_t iCrash = 0;

        DirtyCastLogPrintf("serveraction: %s\n", ServerAction.strName);

        iCrash = *pCrash;

        // this point should never be reached as previous line should throw a "read access violation" exception
    }
#endif
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "action '%s' is not recognized\n", ServerAction.strName);
    }

    // return updated buffer size to caller
    return(iOffset);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerDiagnosticCallback

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module state
    \Input *pUrl            - request url
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipServer module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    GameServerRefT *pGameServer = (GameServerRefT *)pUserData;
    time_t uCurTime = time(NULL);
    int32_t iOffset;
    char strTime[32];

    // show general status and config info
    iOffset = ds_snzprintf(pBuffer, iBufSize, "server time: %s\n", DirtyCastCtime(strTime, sizeof(strTime), uCurTime));
    iOffset += ServerDiagnosticFormatUptime(pBuffer+iOffset, iBufSize-iOffset, uCurTime, GameServerStatus(pGameServer, 'star', 0, NULL, 0));

    // check for an action
    if (!strncmp(pUrl, "/action", 7))
    {
        iOffset += _GameServerDiagnosticAction(pGameServer, strstr(pUrl, "/action"), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (!strncmp(pUrl, "/servulator", 11))
    {
        iOffset += _GameServerDiagnosticServulator(pGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else
    {
        iOffset += _GameServerDiagnosticStatus(pGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }

    // return buffer offset
    return(iOffset);
}
