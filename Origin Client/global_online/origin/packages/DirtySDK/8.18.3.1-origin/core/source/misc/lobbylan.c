/*H********************************************************************************/
/*!
    \File lobbylan.c

    \Description
        LAN-based lobby matchmaking module.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 09/15/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

// system includes
#include <string.h>
#include <stdio.h>

// dirtysock platform include
#include "platform.h"

// dirtysock includes
#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "netgameutil.h"
#include "netgamelink.h"
#include "netgamepkt.h"

// self include
#include "lobbylan.h"

/*** Defines **********************************************************************/

//! max buffer size for buffer passed to NetGameUtilQuery()
#define LOBBYLAN_MAXQUERYSIZE       (2048)

//! game connection timeout (five seconds)
#define LOBBYLAN_GAMECONN_TIMEOUT   (5 * 1000)

//! verbose lobbylan debugging
#define LOBBYLAN_VERBOSE            (DIRTYCODE_DEBUG && FALSE)

/*** Macros ***********************************************************************/

//! memory size of variable-length game list
#define LOBBYLAN_GameListSize(_uMaxGames) (sizeof(LobbyLanGameListT) + (((_uMaxGames) - 1)*sizeof(LobbyLanGameInfoT)))

//! memory size of variable-length player list
#define LOBBYLAN_PlyrListSize(_uMaxPlayers) (sizeof(LobbyLanPlyrListT) + (((_uMaxPlayers) - 1)*sizeof(LobbyLanPlyrInfoT)))

/*** Type Definitions *************************************************************/

//! game message types - these messages are sent over the peer-peer NetGameLink connection
typedef enum LobbyLanLinkMsgE
{
    LOBBYLAN_MSGTYPE_UNUSED,        //!< reserved

    // client->host messages
    LOBBYLAN_CLNTMESG_JOIN,         //!< client->host: join request
    LOBBYLAN_CLNTMESG_QUIT,         //!< client->host: leave notification
    LOBBYLAN_CLNTMESG_INFO,         //!< client->host: updated note info

    // host->client messages
    LOBBYLAN_HOSTMESG_JOIN,         //!< host->client: game has been joined
    LOBBYLAN_HOSTMESG_FULL,         //!< host->client: game is full
    LOBBYLAN_HOSTMESG_PASS,         //!< host->client: password invalid
    LOBBYLAN_HOSTMESG_REFU,         //!< host->client: join request refused
    LOBBYLAN_HOSTMESG_ROST,         //!< host->client: a player roster update
    LOBBYLAN_HOSTMESG_STRT,         //!< host->client: game start

    // user data
    LOBBYLAN_USERMESG_DATA,         //!< user data, not read by lobbylan
    
    LOBBYLAN_NUMMSGS                //!< total number ofmessages
} LobbyLanLinkMsgE;

typedef struct LobbyLanConnT
{
    enum
    {
        ST_INIT,                    //!< init
        ST_QUERY,                   //!< (host only) query host to see if user should be allowed to join
        ST_OPEN,                    //!< connection is open
        ST_CLOSE                    //!< connection is being closed
    } eState;                       //!< current module state

    NetGameUtilRefT *pUtilRef;      //!< pointer to util ref for this connection
    NetGameLinkRefT *pLinkRef;      //!< pointer to link ref for this connection
    uint16_t bPlyrListDirty;        //!< if player list is dirty, need to resend
    uint16_t iPlyrListDirtyIdx;     //!< index of last updated client in list
    struct LobbyLanConnT *pNext;    //!< pointer to next connection struct in kill list
} LobbyLanConnT;

//! game information
typedef struct LobbyLanGameT
{
    uint32_t            uFlags;                         //!< game flags
    LobbyLanGameInfoT   GameInfo;                       //!< public game info
    char                strPass[LOBBYLAN_PASSWORD_LEN]; //!< game password (can be a null string to indicate no password)
    LobbyLanConnT       *pConnList;                     //!< player connection list
    LobbyLanPlyrInfoT   *pPlyrList;                     //!< player info list
} LobbyLanGameT;

//! module state
struct LobbyLanRefT
{
    // module memory group
    int32_t             iMemGroup;          //!< module mem group id
    void                *pMemGroupUserData; //!< user data associated with mem group

    // create info
    int32_t             uMaxGames;      //!< max number of games allowed in lobby list
    char                strKind[32];    //!< game identifer, used for advertising

    LobbyLanPlyrInfoT   Self;           //!< player info about ourselves

    // current state
    enum
    {
        ST_IDLE,                        //!< idle
        ST_HOSTING,                     //!< hosting
        ST_STARTING,                    //!< starting a game (host only)
        ST_JOINING,                     //!< joining a game
        ST_JOINED,                      //!< a member of a game (non-host)
        ST_STARTED                      //!< game has been started
    } eState;                           //!< current module state
    
    // callback info
    LobbyLanCallbackT   *pCallback;     //!< user callback
    void                *pUserData;     //!< user callback data
    
    // game info
    LobbyLanGameT       Game;           //!< game info (hosting/joined info)
    
    // game list
    LobbyLanGameListT   *pGameList;     //!< list of all games
    
    // comm construct function
    CommAllConstructT   *pCommConstruct;//< comm construct function

    // module refs
    NetGameUtilRefT     *pGameUtilRef;  //!< netgame util ref

    // control parameters
    int32_t             iAdvtFreq;      //!< advertising frequency, in seconds
    int32_t             iMaxPacket;     //!< max packet size
    int32_t             iMaxInp;        //!< max input queue size
    int32_t             iMaxOut;        //!< max output queue size
    int32_t             iTimeout;       //!< game connection timeout
    int32_t             iGamePort;      //!< port to use for game communications
    int32_t             bKeepConn;      //!< keep connections after game start
    int32_t             iHosting;       //!< 0=client, 1=host, -1=not yet determined
    int32_t             iLinkBufSize;   //!< size in bytes of link buffer
    
    // kill list
    LobbyLanConnT       *pKillConnRef;  //!< netgame kill list
    
    //! advertising query buffer
    char                strQueryBuf[LOBBYLAN_MAXQUERYSIZE];

    #if DIRTYCODE_LOGGING
    int32_t             iDisplayLate;   //!< display latency info 
    #endif

    int32_t             iLocalGames;    //!< whether to enumerate local games
};

/*** Variables ********************************************************************/

#if LOBBYLAN_VERBOSE
static const char *_LobbyLan_strMsgTypes[] =
{
    "LOBBYLAN_MSGTYPE_UNUSED",        //!< reserved
    "LOBBYLAN_CLNTMESG_JOIN",         //!< client->host: join request
    "LOBBYLAN_CLNTMESG_QUIT",         //!< client->host: leave notification
    "LOBBYLAN_CLNTMESG_INFO",         //!< client->host: updated note info
    "LOBBYLAN_HOSTMESG_JOIN",         //!< host->client: game has been joined
    "LOBBYLAN_HOSTMESG_FULL",         //!< host->client: game is full
    "LOBBYLAN_HOSTMESG_PASS",         //!< host->client: password invalid
    "LOBBYLAN_HOSTMESG_REFU",         //!< host->client: join request refused
    "LOBBYLAN_HOSTMESG_ROST",         //!< host->client: a player roster update
    "LOBBYLAN_HOSTMESG_STRT",         //!< host->client: game start
    "LOBBYLAN_USERMESG_DATA",         //!< user data, not read by lobbylan
};
#endif

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _LobbyLanDefaultCallback

    \Description
        Default (empty) callback.

    \Input *pRef        - pointer to module state
    \Input eType        - callback type
    \Input *pCbData     - callback data
    \Input *pUserData   - pointer to callback user data

    \Output
        int32_t         - zero

    \Version  09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanDefaultCallback(LobbyLanRefT *pRef, LobbyLanCBTypeE eType, void *pCbData, void *pUserData)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanAlloc

    \Description
        An implementation of calloc.

    \Input *pRef        - module state
    \Input iListSize    - size of list

    \Output
        void *          - memptr on success, NULL on failure

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static void *_LobbyLanAlloc(LobbyLanRefT *pRef, int32_t iListSize)
{
    void *pList;
    if ((pList = DirtyMemAlloc(iListSize, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
    {
        memset(pList, 0, iListSize);
    }
    return(pList);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanPackPlyrInfo

    \Description
        Concatenate strings in PlyrInfo structure and return packed size.

    \Input *pBuffer     - output buffer for packed PlyrInfo
    \Input *pPlyrInfo   - PlyrInfo source structure

    \Output
        int32_t         - size of packed info

    \Version 12/05/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanPackPlyrInfo(char *pBuffer, const LobbyLanPlyrInfoT *pPlyrInfo)
{
    char *pStart = pBuffer;
    strcpy(pBuffer, pPlyrInfo->strName);
    pBuffer += strlen(pPlyrInfo->strName)+1;
    strcpy(pBuffer, pPlyrInfo->strNote);
    pBuffer += strlen(pPlyrInfo->strNote)+1;
    strcpy(pBuffer, pPlyrInfo->Addr.strMachineAddr);
    pBuffer += strlen(pPlyrInfo->Addr.strMachineAddr)+1;
    return(pBuffer - pStart);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUnpackPlyrInfo

    \Description
        Unpack concatenated PlyrInfo data into a PlyrInfo structure.

    \Input *pPlyrInfo   - PlyrInfo output buffer
    \Input *pBuffer     - source packed PlyrInfo data

    \Output
        int32_t         - size of packed info

    \Version 12/05/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanUnpackPlyrInfo(LobbyLanPlyrInfoT *pPlyrInfo, const char *pBuffer)
{
    const char *pStart = pBuffer;
    strnzcpy(pPlyrInfo->strName, pBuffer, sizeof(pPlyrInfo->strName));
    pBuffer += strlen(pPlyrInfo->strName)+1;
    strnzcpy(pPlyrInfo->strNote, pBuffer, sizeof(pPlyrInfo->strNote));
    pBuffer += strlen(pPlyrInfo->strNote)+1;
    strnzcpy(pPlyrInfo->Addr.strMachineAddr, pBuffer, sizeof(pPlyrInfo->Addr.strMachineAddr));
    pBuffer += strlen(pPlyrInfo->Addr.strMachineAddr)+1;
    return(pBuffer - pStart);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanCreateGameLists

    \Description
        Allocate players list and connection list.

    \Input *pRef        - pointer to module state
    \Input uMaxPlayers  - number of elements for lists

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanCreateGameLists(LobbyLanRefT *pRef, uint32_t uMaxPlayers)
{
    // create and clear player list
    if ((pRef->Game.pPlyrList = _LobbyLanAlloc(pRef, uMaxPlayers * sizeof(pRef->Game.pPlyrList[0]))) == NULL)
    {
        NetPrintf(("lobbylan: unable to allocate player list\n"));
        return(-1);
    }

    // create and clear game link ref list
    if ((pRef->Game.pConnList = _LobbyLanAlloc(pRef, uMaxPlayers * sizeof(pRef->Game.pConnList[0]))) == NULL)
    {
        NetPrintf(("lobbylan: unable to allocate gamelink ref list\n"));
        return(-2);
    }

    // return success to caller    
    return(0);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _LobbyLanShowLinkStat

    \Description
        Print link stats to debug output at a metered rate.

    \Input *pStats  - pointer to stat structure

    \Version 03/09/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanShowLinkStat(LobbyLanRefT *pRef, const NetGameLinkStatT *pStats)
{
    static int32_t _iLast = 0;
    
    if (pRef->iDisplayLate != 0)
    {
        int32_t iTick = NetTick();
        if ((iTick - _iLast) > (pRef->iDisplayLate * 1000))
        {
            NetPrintf(("lobbylan: ping=%3d late=%3d\n", pStats->ping, pStats->late));
            _iLast = NetTick();
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LobbyLanIntEncode

    \Description
        Encode an int32_t as a string-encoded decimal number.

    \Input *pOut        - pointer to output buffer
    \Input uValue       - integer to encode
    \Input iNumDigits   - number of digits to encode

    \Output
        char *          - pointer to output buffer past encoded integer

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static char *_LobbyLanIntEncode(char *pOut, uint32_t uValue, int32_t iNumDigits)
{
    int32_t iDigit;
    for (iDigit = 0; iDigit < iNumDigits; iDigit++)
    {
        pOut[iNumDigits-iDigit-1] = (uValue % 10) + '0';
        uValue /= 10;
    }
    return(pOut + iNumDigits);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanIntDecode

    \Description
        Decode an int32_t from a string-encoded decimal number.

    \Input **pInp       - pointer to input buffer (updated to point after decoded integer)
    \Input iNumDigits   - number of digits to decode

    \Output
        uint32_t        - decoded number

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LobbyLanIntDecode(char **pInp, int32_t iNumDigits)
{
    uint32_t uValue, uDigitVal, uMult;
    int32_t iDigit;

    for (uValue = 0, iDigit = 0, uMult = 1; iDigit < iNumDigits; iDigit++)
    {
        uDigitVal = (uint32_t)((*pInp)[iNumDigits-iDigit-1] - '0');
        uDigitVal *= uMult;
        uValue += uDigitVal;
        uMult *= 10;
    }

    *pInp += iNumDigits;
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanParseAdvertField

    \Description
        Parse a field from an advertisement.

    \Input *pOutBuf     - output buffer for field
    \Input iOutSize     - size of output buffer
    \Input *pInpBuf     - pointer to start of field in advertisement buffer
    \Input cTerminator  - termination character

    \Output
        char *          - pointer to next field

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static char *_LobbyLanParseAdvertField(char *pOutBuf, int32_t iOutSize, char *pInpBuf, char cTerminator)
{
    char *pEndPtr;

    pEndPtr = strchr(pInpBuf, cTerminator);
    *pEndPtr = '\0';

    strnzcpy(pOutBuf, pInpBuf, iOutSize);
    return(pEndPtr+1);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanGameLinkRecv

    \Description
        Receive data on a connection.

    \Input *pRef        - pointer to module state
    \Input *pLinkRef    - pointer to link ref to receive on
    \Input bGameData    - TRUE if game data, else FALSE
    \Input *pPacket     - [out] - output packet buffer
    \Input *pPacketType - [out] - type of packet received

    \Output
        int32_t         - return result from NetGameLinkRecv()

    \Version 04/04/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanGameLinkRecv(LobbyLanRefT *pRef, NetGameLinkRefT *pLinkRef, uint32_t bGameData, NetGamePacketT *pPacket, uint8_t *pPacketType)
{
    NetGamePacketT *pPeekPacket;
    int32_t iResult;
    uint8_t uPacketType;

    // guard against trying to receive on NULL link refs
    if (pLinkRef == NULL)
    {
        return(0);
    }

    // see if there is any data waiting
    if (NetGameLinkPeek(pLinkRef, &pPeekPacket) <= 0)
    {
        return(0);
    }
    
    // make sure the packet is not user data
    if ((pPeekPacket->body.data[pPeekPacket->head.len-1] == LOBBYLAN_USERMESG_DATA) != (signed)bGameData)
    {
        return(0);
    }

    // read the data
    if ((iResult = NetGameLinkRecv(pLinkRef, pPacket, 1, FALSE)) > 0)
    {
        // remove lobbylan type byte
        uPacketType = pPacket->body.data[pPacket->head.len-1];
        pPacket->head.len -= 1;
        
        #if LOBBYLAN_VERBOSE
        NetPrintf(("lobbylan: recv %d byte %s message\n", pPacket->head.len, _LobbyLan_strMsgTypes[uPacketType]));
        #endif
        if ((uPacketType < LOBBYLAN_CLNTMESG_JOIN) || (uPacketType > LOBBYLAN_USERMESG_DATA))
        {
            NetPrintf(("lobbylan: warning -- recv unrecognized packet type %d\n", uPacketType));
        }
        *pPacketType = uPacketType;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanGameLinkSend

    \Description
        Send data on a connection.

    \Input *pRef        - pointer to module state
    \Input *pLinkRef    - pointer to link ref to send on
    \Input *pPacket     - input packet buffer
    \Input uPacketType  - lobbylan packet type

    \Output
        int32_t         - return result from NetGameLinkSend()

    \Version 04/04/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanGameLinkSend(LobbyLanRefT *pRef, NetGameLinkRefT *pLinkRef, NetGamePacketT *pPacket, uint32_t uPacketType)
{
    int32_t iResult;
    
    #if LOBBYLAN_VERBOSE
    NetPrintf(("lobbylan: send %d byte %s message\n", pPacket->head.len, _LobbyLan_strMsgTypes[uPacketType]));
    #endif
    
    // validate packet type
    if ((uPacketType < LOBBYLAN_CLNTMESG_JOIN) || (uPacketType > LOBBYLAN_USERMESG_DATA))
    {
        NetPrintf(("lobbylan: warning -- send unrecognized packet type %d\n", pPacket->body.data[0]));
        return(-1);
    }

    // append type byte
    pPacket->body.data[pPacket->head.len] = uPacketType;
    pPacket->head.len += 1;

    // send the packet
    if ((iResult = NetGameLinkSend(pLinkRef, pPacket, 1)) <= 0)
    {
        NetPrintf(("lobbylan: error sending packet of type %d (error=%d)\n", uPacketType, iResult));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanAddPlayer

    \Description
        Add a player to the local game.

    \Input *pRef        - pointer to module state 
    \Input *pPlyrInfo   - player to add to game

    \Output
        int32_t         - negative=error, zero=success

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanAddPlayer(LobbyLanRefT *pRef, LobbyLanPlyrInfoT *pPlyrInfo)
{
    // make sure list isn't full
    if (pRef->Game.GameInfo.uNumPlayers >= pRef->Game.GameInfo.uMaxPlayers)
    {
        NetPrintf(("lobbylan: unable to add player '%s' to game as game is full\n", pPlyrInfo->strName));
        return(-1);
    }

    // add player to list and return success
    memcpy(&pRef->Game.pPlyrList[pRef->Game.GameInfo.uNumPlayers++], pPlyrInfo, sizeof(pRef->Game.pPlyrList[0]));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanDelPlayer

    \Description
        Delete a player from the local game.

    \Input *pRef    - pointer to module state 
    \Input iPlyrId  - index of player to remove from list

    \Version 05/05/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanDelPlayer(LobbyLanRefT *pRef, int32_t iPlyrId)
{
    int32_t iLastPlyr = (signed)pRef->Game.GameInfo.uNumPlayers - 1;
    
    // if it's the last player, just remove them
    if (iPlyrId == iLastPlyr)
    {
        memset(&pRef->Game.pPlyrList[iPlyrId], 0, sizeof(pRef->Game.pPlyrList[iPlyrId]));
    }
    else if (iPlyrId < iLastPlyr)
    {
        int32_t iNumMovePlyrs = iLastPlyr - iPlyrId;
        memmove(&pRef->Game.pConnList[iPlyrId], &pRef->Game.pConnList[iPlyrId+1], sizeof(pRef->Game.pConnList[iPlyrId])*iNumMovePlyrs);
        memmove(&pRef->Game.pPlyrList[iPlyrId], &pRef->Game.pPlyrList[iPlyrId+1], sizeof(pRef->Game.pPlyrList[iPlyrId])*iNumMovePlyrs);
    }
    else
    {
        NetPrintf(("lobbylan: unable to del player %d from game\n", iPlyrId));
        return;
    }

    // decrement player count
    pRef->Game.GameInfo.uNumPlayers -= 1;
}


/*F********************************************************************************/
/*!
    \Function _LobbyLanGetGame

    \Description
        Get game info based on game name.

    \Input *pRef            - pointer to module state
    \Input *pName           - pointer to game name

    \Output
        LobbyLanGameInfoT * - pointer to game info, or NULL

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static LobbyLanGameInfoT *_LobbyLanGetGame(LobbyLanRefT *pRef, const char *pName)
{
    LobbyLanGameInfoT *pGameInfo;
    uint32_t uGame;

    // search game list for game
    for (uGame = 0; uGame < pRef->pGameList->uNumGames; uGame++)
    {
        // ref game
        pGameInfo = &pRef->pGameList->Games[uGame];

        // is this the one we are looking for?
        if (!strcmp(pGameInfo->strName, pName))
        {
            return(pGameInfo);
        }
    }

    // could not find game
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanJoinGame

    \Description
        Join a game.

    \Input *pRef        - module state ref
    \Input *pGameInfo   - game info
    \Input *pPassword   - game password

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanJoinGame(LobbyLanRefT *pRef, LobbyLanGameInfoT *pGameInfo, const char *pPassword)
{
    LobbyLanConnT *pConn;
    char strConnect[64];

    // make sure we're currently idle
    if (pRef->eState != ST_IDLE)
    {
        NetPrintf(("lobbylan: cannot join a game when not idle\n"));
        return(-1);
    }

    // save game info
    memcpy(&pRef->Game.GameInfo, pGameInfo, sizeof(pRef->Game.GameInfo));
    strnzcpy(pRef->Game.strPass, pPassword, sizeof(pRef->Game.strPass));

    // create game lists
    if (_LobbyLanCreateGameLists(pRef, pGameInfo->uMaxPlayers) < 0)
    {
        return(-3);
    }

    // create connect string with host address
    snzprintf(strConnect, sizeof(strConnect), "%a:%d:%d", pGameInfo->uHostAddr, pRef->iGamePort, pRef->iGamePort);

    // move game util ref to host slot
    pConn = &pRef->Game.pConnList[0];
    pConn->pUtilRef = pRef->pGameUtilRef;
    pRef->pGameUtilRef = NULL;

    // initiate connect to host and advance state
    NetGameUtilConnect(pConn->pUtilRef, NETGAME_CONN_CONNECT, strConnect, pRef->pCommConstruct);
    pRef->eState = ST_JOINING;
    pRef->iHosting = 0;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUtilCreate

    \Description
        Create NetGameUtil module used for advertising

    \Input *pRef        - pointer to module state

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanUtilCreate(LobbyLanRefT *pRef)
{
    // don't create it if it already exists
    if (pRef->pGameUtilRef != NULL)
    {
        return(0);
    }

    // create the util ref and set the advertising frequency
    DirtyMemGroupEnter(pRef->iMemGroup, pRef->pMemGroupUserData);
    pRef->pGameUtilRef = NetGameUtilCreate();
    DirtyMemGroupLeave();
    if (pRef->pGameUtilRef == NULL)
    {
        NetPrintf(("lobbylan: unable to create NetGameUtil module\n"));
        return(-1);
    }

    // set advertising frequency and packet queue size/counts
    NetGameUtilControl(pRef->pGameUtilRef, 'advf', pRef->iAdvtFreq);
    NetGameUtilControl(pRef->pGameUtilRef, 'mwid', pRef->iMaxPacket);
    if (pRef->iMaxInp != 0)
    {
        NetGameUtilControl(pRef->pGameUtilRef, 'minp', pRef->iMaxInp);
    }
    if (pRef->iMaxOut != 0)
    {
        NetGameUtilControl(pRef->pGameUtilRef, 'mout', pRef->iMaxOut);
    }

    NetGameUtilControl(pRef->pGameUtilRef, 'locl', pRef->iLocalGames);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUtilDestroy

    \Description
        Destroy NetGameUtil module used for advertising

    \Input *pRef        - pointer to module state

    \Version 05/19/2008 (ihamadi)
*/
/********************************************************************************F*/
static void _LobbyLanUtilDestroy(LobbyLanRefT *pRef)
{
    // destroy ref if it exists
    if (pRef->pGameUtilRef != NULL)
    {
        // Cancel any adverts
        if (pRef->iHosting == 1)
        {
            NetGameUtilWithdraw(pRef->pGameUtilRef, pRef->strKind, pRef->Game.GameInfo.strName);
        }

        NetGameUtilDestroy(pRef->pGameUtilRef);
        pRef->pGameUtilRef = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdateAdvert

    \Description
        Update current game advertisement.

    \Input *pRef        - pointer to module state

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdateAdvert(LobbyLanRefT *pRef)
{
    if (pRef->pGameUtilRef)
    {
        char strNote[192], *pEncodePtr = strNote;

        NetPrintf(("lobbylan: updating advertisement\n"));

        // update the note field with current data
        memset(strNote, 0, sizeof(strNote));
        pEncodePtr = _LobbyLanIntEncode(pEncodePtr, pRef->Game.GameInfo.uMaxPlayers, 2);
        pEncodePtr = _LobbyLanIntEncode(pEncodePtr, pRef->Game.GameInfo.uNumPlayers, 2);
        pEncodePtr = _LobbyLanIntEncode(pEncodePtr, pRef->Game.GameInfo.bPassword, 1);

        // concatenate game note
        strcat(strNote, pRef->Game.GameInfo.strNote);

        // refresh advertisement
        NetGameUtilAdvert(pRef->pGameUtilRef, pRef->strKind, pRef->Game.GameInfo.strName, strNote);
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanAdvert

    \Description
        Create and issue game advertisement.

    \Input *pRef        - pointer to module state
    \Input bListen      - if TRUE, start listening

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanAdvert(LobbyLanRefT *pRef, unsigned char bListen)
{
    // create game util ref if we haven't already
    if (_LobbyLanUtilCreate(pRef) < 0)
    {
        return;
    }

    // start listening
    if (bListen == TRUE)
    {
        char strPort[16];
        snzprintf(strPort, sizeof(strPort), ":%d", pRef->iGamePort);
        NetGameUtilConnect(pRef->pGameUtilRef, NETGAME_CONN_LISTEN, strPort, pRef->pCommConstruct);
    }

    // update the advertisement
    _LobbyLanUpdateAdvert(pRef);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdateAdverts

    \Description
        Parse advertisement list.

    \Input *pRef        - pointer to module state
    \Input *pQueryBuf   - pointer to advertisement buffer
    \Input uNumAdverts  - number of advertisements in buffer

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdateAdverts(LobbyLanRefT *pRef, char *pQueryBuf, uint32_t uNumAdverts)
{
    LobbyLanGameInfoT *pGameInfo;
    char strAddr[32], strNote[192];
    char *pQueryPtr, *pTempPtr;
    uint32_t uAdvert;

    // save advertisement
    strnzcpy(pRef->strQueryBuf, pQueryBuf, sizeof(pRef->strQueryBuf));

    // clamp number of advertisements to fit max
    if (uNumAdverts > pRef->pGameList->uMaxGames)
    {
        NetPrintf(("lobbylan: clamping %d advertisements to max of %d\n", uNumAdverts, pRef->pGameList->uMaxGames));
        uNumAdverts = pRef->pGameList->uMaxGames;
    }

    // parse advertisements
    for (uAdvert = 0, pQueryPtr = pQueryBuf; uAdvert < uNumAdverts; uAdvert++)
    {
        // ref game info
        pGameInfo = &pRef->pGameList->Games[uAdvert];

        // parse info
        pQueryPtr = _LobbyLanParseAdvertField(pGameInfo->strName, sizeof(pGameInfo->strName), pQueryPtr, '\t');
        pQueryPtr = _LobbyLanParseAdvertField(strNote, sizeof(strNote), pQueryPtr, '\t');
        pQueryPtr = _LobbyLanParseAdvertField(strAddr, sizeof(strAddr), pQueryPtr + 4, '\n');

        // extract address from addr field
        pTempPtr = strchr(strAddr, ':');
        *pTempPtr = '\0';
        pGameInfo->uHostAddr = SocketInTextGetAddr(strAddr);

        // extract game info from note field
        pTempPtr = strNote;
        pGameInfo->uMaxPlayers = _LobbyLanIntDecode(&pTempPtr, 2);
        pGameInfo->uNumPlayers = _LobbyLanIntDecode(&pTempPtr, 2);
        pGameInfo->bPassword = (unsigned char)_LobbyLanIntDecode(&pTempPtr, 1);
        
        // copy game note into note field
        strnzcpy(pGameInfo->strNote, pTempPtr, sizeof(pGameInfo->strNote));

        // skip duplicate advertisement
        pQueryPtr = strchr(pQueryPtr, '\n') + 1;
    }
    
    // save number of games
    pRef->pGameList->uNumGames = uNumAdverts;
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanDestroyGameRefs

    \Description
        Perform "safe destroy" of GameUtil and GameLink refs.
        
        A "safe destroy does two things - it destroys the module only if the
        module pointer is not NULL, and it sets the module pointer to NULL after
        the destroy.

    \Input **ppUtilRef  - handle to connection's util ref
    \Input **ppLinkRef  - handle to connection's link ref

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanDestroyGameRefs(NetGameUtilRefT **ppUtilRef, NetGameLinkRefT **ppLinkRef)
{
    if (*ppLinkRef != NULL)
    {
        NetGameLinkDestroy(*ppLinkRef);
        *ppLinkRef = NULL;
    }
    if (*ppUtilRef != NULL)
    {
        NetGameUtilDestroy(*ppUtilRef);
        *ppUtilRef = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanKillConn

    \Description
        Halt sending on the connection and add it to a kill list so it can be
        terminated once all pending traffic has been delivered.

    \Input *pRef        - pointer to module state
    \Input **ppUtilRef  - handle to connection's util ref
    \Input **ppLinkRef  - handle to connection's link ref

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanKillConn(LobbyLanRefT *pRef, NetGameUtilRefT **ppUtilRef, NetGameLinkRefT **ppLinkRef)
{
    LobbyLanConnT *pConn;

    if ((pConn = DirtyMemAlloc(sizeof(*pConn), LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) != NULL)
    {
        NetPrintf(("lobbylan: adding connection ref 0x%08x to kill list\n", (intptr_t)pConn));

        // stop sending
        NetGameLinkControl(*ppLinkRef, 'send', FALSE, NULL);

        // add ref to kill list for termination
        pConn->pLinkRef = *ppLinkRef;
        pConn->pUtilRef = *ppUtilRef;
        pConn->pNext = pRef->pKillConnRef;
        pRef->pKillConnRef = pConn;
        
        // null refs
        *ppLinkRef = NULL;
        *ppUtilRef = NULL;
    }
    else
    {
        NetPrintf(("lobbylan: unable to allocate memory for connection kill ref, terminating immediately\n"));
        _LobbyLanDestroyGameRefs(ppUtilRef, ppLinkRef);
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdateKillList

    \Description
        Process kill list and kill connections that have been flushed.

    \Input *pRef        - pointer to module state

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdateKillList(LobbyLanRefT *pRef)
{
    LobbyLanConnT **ppConn, *pConn;

    // scan kill list
    for (ppConn = &pRef->pKillConnRef; *ppConn != NULL; )
    {
        pConn = *ppConn;

        // send queue empty?
        if ((pConn->pLinkRef == NULL) || (NetGameLinkControl(pConn->pLinkRef, 'sque', 0, NULL) != 0))
        {
            uint32_t uClientAddr = NetGameUtilStatus(pConn->pUtilRef, 'peip', NULL, 0);
            
            // kill ref
            NetPrintf(("lobbylan: killing connection ref 0x%08x addr=%a\n", (intptr_t)pConn, uClientAddr));
            _LobbyLanDestroyGameRefs(&pConn->pUtilRef, &pConn->pLinkRef);

            // remove it from list
            *ppConn = (*ppConn)->pNext;

            // tell application the connection is gone
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_PLYRKILL, &uClientAddr, pRef->pUserData);

            // release memory
            DirtyMemFree(pConn, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        else
        {
            // index to next link
            ppConn = &((*ppConn)->pNext);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanCloseConn

    \Description
        Close a connection.

    \Input *pRef            - pointer to module state
    \Input *pConn           - connection to close
    \Input bImmediate       - if TRUE, kill immediately
    \Input bUpdateAdvert    - if TRUE, update the advertisement

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanCloseConn(LobbyLanRefT *pRef, LobbyLanConnT *pConn, uint32_t bImmediate, uint32_t bUpdateAdvert)
{
    if (bImmediate == FALSE)
    {
        // add to kill list 
        _LobbyLanKillConn(pRef, &pConn->pUtilRef, &pConn->pLinkRef);
    }
    else
    {
        uint32_t uClientAddr = (pConn->pUtilRef != NULL) ? NetGameUtilStatus(pConn->pUtilRef, 'peip', NULL, 0) : -1;
        
        // kill immediately instead of adding to kill list
        _LobbyLanDestroyGameRefs(&pConn->pUtilRef, &pConn->pLinkRef);
        
        // tell application the connection is gone
        pRef->pCallback(pRef, LOBBYLAN_CBTYPE_PLYRKILL, &uClientAddr, pRef->pUserData);
    }

    // reset conn
    memset(pConn, 0, sizeof(*pConn));
    
    // refresh advertisement to indicate new player count
    if (bUpdateAdvert == TRUE)
    {
        _LobbyLanAdvert(pRef, FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdatePlyrList

    \Description
        If a connection's player list is dirty, send it and clear the dirty flag.

    \Input *pRef            - pointer to module state
    \Input *pConn           - connection to update
    \Input uConn            - index of connection

    \Version 04/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdatePlyrList(LobbyLanRefT *pRef, LobbyLanConnT *pConn, uint32_t uConn)
{
    LobbyLanPlyrInfoT *pPlyrInfo;
    NetGamePacketT Packet;

    // if not dirty, nothing to do
    if (pConn->bPlyrListDirty == FALSE)
    {
        return;
    }

    if (pConn->iPlyrListDirtyIdx >= pRef->Game.GameInfo.uNumPlayers)
    {
        NetPrintf(("lobbylan: roster send complete for client %d\n", uConn));
        pConn->bPlyrListDirty = FALSE;
        pConn->iPlyrListDirtyIdx = 0;
    }

    // format base packet
    memset(&Packet, 0, sizeof(Packet));
    Packet.head.kind = GAME_PACKET_USER;
    Packet.body.data[0] = LOBBYLAN_HOSTMESG_ROST;

    // send current roster info
    pPlyrInfo = &pRef->Game.pPlyrList[pConn->iPlyrListDirtyIdx];

    Packet.body.data[0] = (uint8_t)pConn->iPlyrListDirtyIdx;
    Packet.head.len = _LobbyLanPackPlyrInfo((char *)&Packet.body.data[1], pPlyrInfo) + 1;

    NetPrintf(("lobbylan: sending player %d info to client %d (%d bytes) at %d\n", pConn->iPlyrListDirtyIdx, uConn, Packet.head.len, NetTick()));
    _LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_HOSTMESG_ROST);

    pConn->iPlyrListDirtyIdx += 1;
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanAddConnHost

    \Description
        Add a new host->client connection.

    \Input *pRef        - pointer to module state
    \Input *pCommRef    - pointer to newly established commref

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanAddConnHost(LobbyLanRefT *pRef, void *pCommRef)
{
    NetGameLinkRefT *pLinkRef;
    LobbyLanConnT *pConn;

    DirtyMemGroupEnter(pRef->iMemGroup, pRef->pMemGroupUserData);
    pLinkRef = NetGameLinkCreate(pCommRef, FALSE, pRef->iLinkBufSize);
    DirtyMemGroupLeave();

    NetPrintf(("lobbylan: connection to client #%d established\n", pRef->Game.GameInfo.uNumPlayers));

    // do we have room to add them?
    if (pRef->Game.GameInfo.uNumPlayers < pRef->Game.GameInfo.uMaxPlayers)
    {
        // add them to the connection list
        NetPrintf(("lobbylan: adding client to connection list\n"));
        pConn = &pRef->Game.pConnList[pRef->Game.GameInfo.uNumPlayers++];
        pConn->pUtilRef = pRef->pGameUtilRef;
        pConn->pLinkRef = pLinkRef;
        pConn->eState = ST_INIT;
        pConn->bPlyrListDirty = TRUE;
        pConn->iPlyrListDirtyIdx = 0;

        // NULL game ref so it will create a new one
        pRef->pGameUtilRef = NULL;
    }
    else // no room... inform them and then add connection to kill list
    {
        // set up packet
        NetGamePacketT Packet;
        memset(&Packet, 0, sizeof(Packet));
        Packet.head.kind = GAME_PACKET_USER;
        Packet.head.len = 0;

        // send the message
        NetPrintf(("lobbylan: sending full message to client\n"));
        _LobbyLanGameLinkSend(pRef, pLinkRef, &Packet, LOBBYLAN_HOSTMESG_FULL);
        _LobbyLanKillConn(pRef, &pRef->pGameUtilRef, &pLinkRef);
        pRef->pGameUtilRef = NULL;
    }

    // create new util ref and restart advertisement
    _LobbyLanAdvert(pRef, TRUE);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdateConnHost

    \Description
        Update a host->client connection.

    \Input *pRef    - pointer to module state
    \Input uConn    - connection to update

    \Output
        int32_t     - one if this client was removed, else zero

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LobbyLanUpdateConnHost(LobbyLanRefT *pRef, uint32_t uConn)
{
    LobbyLanPlyrInfoT *pPlyrInfo;
    NetGamePacketT Packet;
    LobbyLanConnT *pConn;
    uint32_t bClose=FALSE, bIsOpen, uPlyr;
    int32_t iTimeout;
    uint8_t uPacketType;

    pConn = &pRef->Game.pConnList[uConn];
    pPlyrInfo = &pRef->Game.pPlyrList[uConn];

    if (pConn->eState == ST_INIT)
    {
        // process incoming data
        if ((_LobbyLanGameLinkRecv(pRef, pConn->pLinkRef, FALSE, &Packet, &uPacketType)) > 0)
        {
            // is it a join request?
            if (uPacketType == LOBBYLAN_CLNTMESG_JOIN)
            {
                char strPass[LOBBYLAN_PASSWORD_LEN];
                LobbyLanPlyrInfoT PlyrInfo;
                int32_t iPassOffset;
                
                // extract player info and find offset to password (if any)
                iPassOffset = _LobbyLanUnpackPlyrInfo(&PlyrInfo, (char *)Packet.body.data);

                // if this game is passworded, extract the password
                if (pRef->Game.GameInfo.bPassword == TRUE)
                {
                    strnzcpy(strPass, (char *)&Packet.body.data[iPassOffset], sizeof(strPass));
                    if (strcmp(pRef->Game.strPass, strPass))
                    {
                        // password invalid... inform them and then add connection to kill list
                        NetPrintf(("lobbylan: sending pass message to client\n"));
                        memset(&Packet, 0, sizeof(Packet));
                        Packet.head.kind = GAME_PACKET_USER;
                        Packet.head.len = 0;
                        _LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_HOSTMESG_PASS);
                        _LobbyLanCloseConn(pRef, pConn, FALSE, TRUE);
                        return(0);
                    }
                }
                
                // save player info
                memcpy(pPlyrInfo, &PlyrInfo, sizeof(*pPlyrInfo));

                // move to query state
                pConn->eState = ST_QUERY;
            }
        }
    }

    if (pConn->eState == ST_QUERY)
    {
        int32_t iResult;
        
        // call user callback to see if we should add them
        if ((iResult = pRef->pCallback(pRef, LOBBYLAN_CBTYPE_JOINREQU, pPlyrInfo, pRef->pUserData)) > 0)
        {
            // let them know they're added
            NetPrintf(("lobbylan: sending join message to client\n"));
            memset(&Packet, 0, sizeof(Packet));
            Packet.head.kind = GAME_PACKET_USER;
            Packet.head.len = 0;
            _LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_HOSTMESG_JOIN);

            // mark all connected client's rosters as dirty
            for (uPlyr = 1; uPlyr < pRef->Game.GameInfo.uNumPlayers; uPlyr++)
            {
                pRef->Game.pConnList[uPlyr].bPlyrListDirty = TRUE;
            }

            // transition to open state
            pConn->eState = ST_OPEN;
        }
        else if (iResult < 0)
        {
            // connection was refused by host
            NetPrintf(("lobbylan: sending refused message to client\n"));
            Packet.head.kind = GAME_PACKET_USER;
            Packet.head.len = 0;
            _LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_HOSTMESG_REFU);
            _LobbyLanCloseConn(pRef, pConn, FALSE, TRUE);
            pConn->eState = ST_CLOSE;
            bClose = TRUE;
        }
    }

    if (pConn->eState == ST_OPEN)
    {
        // receive data from client
        while (_LobbyLanGameLinkRecv(pRef, pConn->pLinkRef, FALSE, &Packet, &uPacketType) > 0)
        {
            if (uPacketType == LOBBYLAN_CLNTMESG_INFO)
            {
                NetPrintf(("lobbylan: got updated note info from client %d\n", uConn));
                strnzcpy(pPlyrInfo->strNote, (char *)&Packet.body.data[0], sizeof(pPlyrInfo->strNote));
                
                // mark rosters as dirty
                for (uPlyr = 1; uPlyr < pRef->Game.GameInfo.uNumPlayers; uPlyr++)
                {
                    pRef->Game.pConnList[uPlyr].bPlyrListDirty = TRUE;
                }
                
                // trigger local roster update callback
                pRef->pCallback(pRef, LOBBYLAN_CBTYPE_PLYRLIST, NULL, pRef->pUserData);
            }
            if (uPacketType == LOBBYLAN_CLNTMESG_QUIT)
            {
                NetPrintf(("lobbylan: got quit message from client %d\n", uConn));
                pConn->eState = ST_CLOSE;
                bClose = TRUE;
            }
        }
        
        // send current roster, if it has changed
        _LobbyLanUpdatePlyrList(pRef, pConn, uConn);
    }

    // make sure we're not closed
    if (pConn->pLinkRef != NULL)
    {
        NetGameLinkStatT Stats;
        NetGameLinkStatus(pConn->pLinkRef, 'stat', 0, &Stats, sizeof(NetGameLinkStatT));
        
        #if DIRTYCODE_LOGGING
        _LobbyLanShowLinkStat(pRef, &Stats);
        #endif
        
        bIsOpen = Stats.isopen;
        iTimeout = NetTickDiff(Stats.tick, Stats.rcvdlast);
    }
    else
    {
        bIsOpen = FALSE;
        iTimeout = 0;
    }
    
    // check for disconnect/timeout
    if ((bClose == TRUE) || (bIsOpen == FALSE) || (iTimeout > pRef->iTimeout))
    {
        // close the connection
        NetPrintf(("lobbylan: client %d has disconnected (close=%s isopen=%s timeout=%d)\n", uConn,
            (bClose == TRUE) ? "true" : "false",
            (bIsOpen == TRUE) ? "true" : "false",
            iTimeout));
        _LobbyLanCloseConn(pRef, pConn, TRUE, FALSE);
        
        // remove player and connection from player/connection list
        _LobbyLanDelPlayer(pRef, uConn);

        // refresh the advertisement
        _LobbyLanUpdateAdvert(pRef);

        // set up packet for removal notice
        memset(&Packet, 0, sizeof(Packet));
        Packet.head.kind = GAME_PACKET_USER;
        Packet.head.len = 1;
        Packet.body.data[0] = uConn | 0x80;

        // Send notice to ourself as well.
        pRef->pCallback(pRef, LOBBYLAN_CBTYPE_PLYRLIST, NULL, pRef->pUserData);

        // send removal notice to all clients
        for (uPlyr = 1; uPlyr < pRef->Game.GameInfo.uNumPlayers; uPlyr++)
        {
            NetPrintf(("lobbylan: sending (prev) player %d removal notice to (curr) player %d\n", uConn, uPlyr));
            _LobbyLanGameLinkSend(pRef, pRef->Game.pConnList[uPlyr].pLinkRef, &Packet, LOBBYLAN_HOSTMESG_ROST);
        }

        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanAddConnClient

    \Description
        Update a client who is joining a game.

    \Input *pRef    - pointer to module state

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanAddConnClient(LobbyLanRefT *pRef)
{
    LobbyLanConnT *pConn = &pRef->Game.pConnList[0];
    NetGamePacketT Packet;
    uint8_t uPacketType;
    void *pCommRef;

    // have we established a connection yet?
    if (pConn->pLinkRef == NULL)
    {
        // do we have a connection?
        if ((pCommRef = NetGameUtilComplete(pConn->pUtilRef)) != NULL)
        {
            DirtyMemGroupEnter(pRef->iMemGroup, pRef->pMemGroupUserData);
            pConn->pLinkRef = NetGameLinkCreate(pCommRef, FALSE, 8192);
            DirtyMemGroupLeave();

            // send join request to server
            memset(&Packet, 0, sizeof(Packet));
            Packet.head.kind = GAME_PACKET_USER;
            Packet.head.len = _LobbyLanPackPlyrInfo((char *)&Packet.body.data[0], &pRef->Self);
            strcpy((char *)&Packet.body.data[Packet.head.len], pRef->Game.strPass);
            Packet.head.len += (uint16_t)strlen(pRef->Game.strPass) + 1;

            NetPrintf(("lobbylan: sending join request to host (len=%d)\n", Packet.head.len));
            if (_LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_CLNTMESG_JOIN) <= 0)
            {
                NetPrintf(("lobbylan: error sending join message to host\n"));
            }
        }
    }
    else if (_LobbyLanGameLinkRecv(pRef, pConn->pLinkRef, FALSE, &Packet, &uPacketType) > 0) // wait for msg from host
    {
        // what kind of message did we get?
        if (uPacketType == LOBBYLAN_HOSTMESG_JOIN)
        {
            NetPrintf(("lobbylan: game has been successfully joined\n"));
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMEJOIN, NULL, pRef->pUserData);
            pRef->eState = ST_JOINED;
            pConn->eState = ST_OPEN;
        }
        else if (uPacketType == LOBBYLAN_HOSTMESG_FULL)
        {
            NetPrintf(("lobbylan: game is full\n"));
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMEFULL, NULL, pRef->pUserData);
            _LobbyLanCloseConn(pRef, pConn, FALSE, FALSE);
            pRef->eState = ST_IDLE;
        }
        else if (uPacketType == LOBBYLAN_HOSTMESG_PASS)
        {
            NetPrintf(("lobbylan: password refused\n"));
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMEPASS, NULL, pRef->pUserData);
            _LobbyLanCloseConn(pRef, pConn, FALSE, FALSE);
            pRef->eState = ST_IDLE;
        }
        else if (uPacketType == LOBBYLAN_HOSTMESG_REFU)
        {
            NetPrintf(("lobbylan: join request refused\n"));
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMEREFU, NULL, pRef->pUserData);
            _LobbyLanCloseConn(pRef, pConn, FALSE, FALSE);
            pRef->eState = ST_IDLE;
        }
        else
        {
            NetPrintf(("lobbylan: received unknown message 0x%02x from host\n", Packet.body.data[0]));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdateConnClient

    \Description
        Update a client who is joined to a game.

    \Input *pRef    - pointer to module state

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdateConnClient(LobbyLanRefT *pRef)
{
    LobbyLanConnT *pConn = &pRef->Game.pConnList[0];
    NetGameLinkStatT Stats;
    LobbyLanPlyrInfoT *pPlyrInfo;
    NetGamePacketT Packet;
    uint32_t uRostChanged = FALSE;
    uint32_t uPlyr;
    uint8_t uPacketType;

    // receive data from host
    while (_LobbyLanGameLinkRecv(pRef, pConn->pLinkRef, FALSE, &Packet, &uPacketType) > 0)
    {
        // parse incoming packet
        if (uPacketType == LOBBYLAN_HOSTMESG_ROST)
        {
            // received a roster update
            uPlyr = Packet.body.data[0];
            if (uPlyr < pRef->Game.GameInfo.uMaxPlayers)
            {
                pPlyrInfo = &pRef->Game.pPlyrList[uPlyr];
                _LobbyLanUnpackPlyrInfo(pPlyrInfo, (char *)&Packet.body.data[1]);
                if (pRef->Game.GameInfo.uNumPlayers < (uPlyr+1))
                {
                    pRef->Game.GameInfo.uNumPlayers = uPlyr+1;
                }
                NetPrintf(("lobbylan: received roster update for player #%d '%s'\n", uPlyr, pPlyrInfo->strName));
                uRostChanged = TRUE;
            }
            else if (uPlyr & 0x80)
            {
                // process removal notification
                uPlyr &= ~0x80;
                pPlyrInfo = &pRef->Game.pPlyrList[uPlyr];
                NetPrintf(("lobbylan: received roster delete of player #%d '%s'\n", uPlyr, pPlyrInfo->strName));
                _LobbyLanDelPlayer(pRef, uPlyr);
                uRostChanged = TRUE;
            }
            else
            {
                NetPrintf(("lobbylan: got a roster update at an index outside the player list\n"));
            }
        }

        if (uPacketType == LOBBYLAN_HOSTMESG_STRT)
        {
            // issue the game start callback
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMESTRT, NULL, pRef->pUserData);

            // leave game in preperation for shutdown
            if (!pRef->bKeepConn)
            {
                LobbyLanLeaveGame(pRef);
            }

            // transition to started state and bail
            pRef->eState = ST_STARTED;
            return;
        }
    }

    // if roster changed...
    if (uRostChanged == TRUE)
    {
        pRef->pCallback(pRef, LOBBYLAN_CBTYPE_PLYRLIST, NULL, pRef->pUserData);
    }

    // make sure we're not closed
    NetGameLinkStatus(pConn->pLinkRef, 'stat', 0, &Stats, sizeof(NetGameLinkStatT));

    // check for disconnect/timeout
    if ((Stats.isopen == FALSE) || (NetTickDiff(Stats.tick, Stats.rcvdlast) > pRef->iTimeout))
    {
        NetPrintf(("lobbylan: host has disconnected (isopen=%s timeout=%d)\n",
            (Stats.isopen == TRUE) ? "true" : "false",
            (Stats.tick - Stats.rcvdlast)));
        LobbyLanLeaveGame(pRef);
        pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMEKILL, NULL, pRef->pUserData);            
    }
    else
    {
        #if DIRTYCODE_LOGGING
        _LobbyLanShowLinkStat(pRef, &Stats);
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function _LobbyLanUpdate

    \Description
        Update the LobbyLan module.

    \Input *pData       - pointer to module state
    \Input uTick        - current tick, in milliseconds

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called and this module
        to work as intended

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LobbyLanUpdate(void *pData, uint32_t uTick)
{
    LobbyLanRefT *pRef = (LobbyLanRefT *)pData;
    char strQueryBuf[LOBBYLAN_MAXQUERYSIZE];
    uint32_t uConn;
    int32_t iNumAdverts;
    void *pCommRef;

    // update games list
    if ((pRef->eState != ST_STARTING) && (pRef->eState != ST_STARTED))
    {
        // create util ref if it doesn't already exist
        if (_LobbyLanUtilCreate(pRef) < 0)
        {
            return;
        }
        
        // query advertisements (note: NetGameUtilAdvert() generates two "advertisements" per actual advert)
        iNumAdverts = NetGameUtilQuery(pRef->pGameUtilRef, pRef->strKind, strQueryBuf, sizeof(strQueryBuf))/2;

        // any changes in game advertisements?
        if (strcmp(strQueryBuf, pRef->strQueryBuf))
        {
            // update adverts
            _LobbyLanUpdateAdverts(pRef, strQueryBuf, iNumAdverts);
        
            // notify application of update
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMELIST, NULL, pRef->pUserData);
        }
    }

    // update host during idle
    if (pRef->eState == ST_HOSTING)
    {
        // do we have a new connection?
        if ((pCommRef = NetGameUtilComplete(pRef->pGameUtilRef)) != NULL)
        {
            _LobbyLanAddConnHost(pRef, pCommRef);
        }

        // update connection list
        for (uConn = 1; uConn < pRef->Game.GameInfo.uNumPlayers; uConn++)
        {
            uConn -= _LobbyLanUpdateConnHost(pRef, uConn);
        }

        // autostart?
        if ((pRef->Game.uFlags & LOBBYLAN_GAMEFLAG_AUTOSTART) &&
            (pRef->Game.GameInfo.uNumPlayers == pRef->Game.GameInfo.uMaxPlayers))
        {
            LobbyLanStartGame(pRef);
            return;
        }
    }

    // update host starting a game
    if (pRef->eState == ST_STARTING)
    {
        // have we shut down all of our connections yet?
        if (pRef->Game.GameInfo.uNumPlayers == 1)
        {
            // yes, time to issue the game start callback
            pRef->pCallback(pRef, LOBBYLAN_CBTYPE_GAMESTRT, NULL, pRef->pUserData);

            // remove ourselves
            pRef->Game.GameInfo.uNumPlayers = 0;

            // transition to started state
            pRef->eState = ST_STARTED;
        }
    }

    // update client joining a game
    if (pRef->eState == ST_JOINING)
    {
        _LobbyLanAddConnClient(pRef);
    }

    // update client joined to a game
    if (pRef->eState == ST_JOINED)
    {
        _LobbyLanUpdateConnClient(pRef);
    }

    // update host/client in STARTED state
    if ((pRef->eState == ST_STARTED) && (pRef->bKeepConn == TRUE))
    {
        if (pRef->iHosting == 1)
        {
            // update connection list
            for (uConn = 1; uConn < pRef->Game.GameInfo.uNumPlayers; uConn++)
            {
                uConn -= _LobbyLanUpdateConnHost(pRef, uConn);
            }
        }
        else
        {
            _LobbyLanUpdateConnClient(pRef);
        }
    }

    // update kill list
    _LobbyLanUpdateKillList(pRef);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function LobbyLanCreate2

    \Description
        Create the LobbyLan module

    \Input *pSelfName   - pointer to local client's name
    \Input *pSelfAddr   - pointer to local client's address
    \Input *pKind       - unique title identifier
    \Input uMaxGames    - max number of games viewable
    \Input *pConstruct  - comm construct function

    \Output
        LobbyLanRefT *  - pointer to new module, or NULL if creation failed

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
LobbyLanRefT *LobbyLanCreate2(const char *pSelfName, const DirtyAddrT *pSelfAddr, const char *pKind, uint32_t uMaxGames, CommAllConstructT *pConstruct)
{
    LobbyLanRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), LOBBYLAN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("lobbylan: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // create and clear game list
    if ((pRef->pGameList = _LobbyLanAlloc(pRef, LOBBYLAN_GameListSize(uMaxGames))) == NULL)
    {
        NetPrintf(("lobbylan: unable to allocate game list\n"));
        DirtyMemFree(pRef, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        return(NULL);
    }
    pRef->pGameList->uMaxGames = uMaxGames;

    // save info
    pRef->uMaxGames = uMaxGames;
    strnzcpy(pRef->strKind, pKind, sizeof(pRef->strKind));
    strnzcpy(pRef->Self.strName, pSelfName, sizeof(pRef->Self.strName));
    strnzcpy(pRef->Self.Addr.strMachineAddr, pSelfAddr->strMachineAddr, sizeof(pRef->Self.Addr));
    pRef->iAdvtFreq = 30;    
    pRef->iMaxPacket = NETGAME_DATAPKT_DEFSIZE;
    pRef->iGamePort = 10000;
    pRef->iLinkBufSize = 8192;
    pRef->iHosting = -1;
    pRef->iTimeout = LOBBYLAN_GAMECONN_TIMEOUT;
    pRef->pCommConstruct = pConstruct;
    pRef->bKeepConn = TRUE;
    
    // set up default callback
    pRef->pCallback = _LobbyLanDefaultCallback;

    // add update function to netconn idle handler
    NetConnIdleAdd(_LobbyLanUpdate, pRef);

    // return ref to caller
    return(pRef);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanDestroy

    \Description
        Destroy the LobbyLan module

    \Input *pRef        - module state ref

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanDestroy(LobbyLanRefT *pRef)
{
    // remove idle handler
    NetConnIdleDel(_LobbyLanUpdate, pRef);

    // if not in idle state, leave game
    if (pRef->eState != ST_IDLE)
    {
        LobbyLanLeaveGame(pRef);
    }

    // free the game list
    DirtyMemFree(pRef->pGameList, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

    // destroy netgame module
    _LobbyLanUtilDestroy(pRef);

    // release module memory
    DirtyMemFree(pRef, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanSetCallback

    \Description
        Set LobbyLan callback function.

    \Input *pRef        - module state ref
    \Input *pCallback   - pointer to user callback
    \Input *pUserData   - pointer to user callback data

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanSetCallback(LobbyLanRefT *pRef, LobbyLanCallbackT *pCallback, void *pUserData)
{
    pRef->pCallback = pCallback;
    pRef->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function LobbyLanSetCallback

    \Description
        Reset module state.

    \Input *pRef        - module state ref

    \Version 05/15/2007 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanReset(LobbyLanRefT *pRef)
{
    pRef->pGameList->uNumGames = 0;
    pRef->strQueryBuf[0] = '\0';
    if (pRef->pGameUtilRef != NULL)
    {
        NetPrintf(("lobbylan: resetting util ref\n"));
        NetGameUtilReset(pRef->pGameUtilRef);
    }
}

/*F********************************************************************************/
/*!
    \Function LobbyLanCreateGame

    \Description
        Create a new game as host and join it.

    \Input *pRef        - module state ref
    \Input uGameFlags   - game flags (LOBBYLAN_GAMEFLAGS_*)
    \Input *pName       - pointer to game name
    \Input *pNote       - pointer to game-specific parameters
    \Input *pPassword   - pointer to game password (optional)
    \Input uMaxPlayers  - maximum number of players allowed in game

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanCreateGame(LobbyLanRefT *pRef, uint32_t uGameFlags, const char *pName, const char *pNote, const char *pPassword, uint32_t uMaxPlayers)
{
    // make sure we're idle
    if (pRef->eState != ST_IDLE)
    {
        NetPrintf(("lobbylan: cannot create a game when not in IDLE state\n"));
        return(-1);
    }

    // make sure game isn't already being advertised by someone else
    if (_LobbyLanGetGame(pRef, pName) != NULL)
    {
        NetPrintf(("lobbylan: game '%s' already exists\n", pName));
        return(-2);
    }

    // clear game ref
    memset(&pRef->Game, 0, sizeof(pRef->Game));

    // create game lists
    if (_LobbyLanCreateGameLists(pRef, uMaxPlayers) < 0)
    {
        return(-3);
    }

    // save game info
    pRef->Game.uFlags = uGameFlags;
    pRef->Game.GameInfo.uMaxPlayers = uMaxPlayers;
    pRef->Game.GameInfo.bPassword = ((pPassword == NULL) || (*pPassword == '\0')) ? FALSE : TRUE;
    strnzcpy(pRef->Game.GameInfo.strName, pName, sizeof(pRef->Game.GameInfo.strName));
    strnzcpy(pRef->Game.GameInfo.strNote, pNote, sizeof(pRef->Game.GameInfo.strNote));
    strnzcpy(pRef->Game.strPass, pPassword, sizeof(pRef->Game.strPass));

    // add ourselves to game
    _LobbyLanAddPlayer(pRef, &pRef->Self);

    // go to hosting state
    pRef->eState = ST_HOSTING;
    pRef->iHosting = 1;
    
    // start advertisement
    _LobbyLanAdvert(pRef, TRUE);

    // return success to caller
    return(0);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanSetNote

    \Description
        Set game note, if hosting a game.

    \Input *pRef        - module state ref
    \Input *pNote       - pointer to new note to use

    \Version 04/12/2005 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanSetNote(LobbyLanRefT *pRef, const char *pNote)
{
    // make sure we're hosting
    if (pRef->iHosting != 1)
    {
        NetPrintf(("lobbylan: cannot update note while not in hosting state\n"));
        return;
    }

    // make sure we have an advertisement to update
    if (pRef->pGameUtilRef == NULL)
    {
        NetPrintf(("lobbylan: no advertisement to update\n"));
        return;
    }

    // update the game note
    strnzcpy(pRef->Game.GameInfo.strNote, pNote, sizeof(pRef->Game.GameInfo.strNote));

    // refresh the advertisement    
    _LobbyLanUpdateAdvert(pRef);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanSetPlyrNote

    \Description
        Set our player note.

    \Input *pRef        - module state ref
    \Input *pNote       - pointer to new note to use

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanSetPlyrNote(LobbyLanRefT *pRef, const char *pNote)
{
    // save info
    strnzcpy(pRef->Self.strNote, pNote, sizeof(pRef->Self.strNote));

    // if we're hosting, update our info and mark rosters as dirty    
    if (pRef->iHosting == 1)
    {
        uint32_t uPlyr;

        // update info
        memcpy(&pRef->Game.pPlyrList[0], &pRef->Self, sizeof(pRef->Game.pPlyrList[0]));
        
        // mark all connected client's rosters as dirty
        for (uPlyr = 1; uPlyr < pRef->Game.GameInfo.uNumPlayers; uPlyr++)
        {
            pRef->Game.pConnList[uPlyr].bPlyrListDirty = TRUE;
        }
    }
    else if (pRef->iHosting == 0)
    {
        // send updated info to host
        NetGamePacketT Packet;
        
        memset(&Packet, 0, sizeof(Packet));
        Packet.head.kind = GAME_PACKET_USER;
        Packet.body.data[0] = LOBBYLAN_CLNTMESG_INFO;
        strnzcpy((char *)&Packet.body.data[0], pNote, LOBBYLAN_PLYRNOTE_LEN);
        Packet.head.len = (uint16_t)strlen((char *)&Packet.body.data[0]) + 1;
        NetPrintf(("lobbylan: sending updated note to host (len=%d)\n", Packet.head.len));
        _LobbyLanGameLinkSend(pRef, pRef->Game.pConnList[0].pLinkRef, &Packet, LOBBYLAN_CLNTMESG_INFO);
    }
}

/*F********************************************************************************/
/*!
    \Function LobbyLanStartGame

    \Description
        Start a game (game host only)

    \Input *pRef        - module state ref

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanStartGame(LobbyLanRefT *pRef)
{
    NetGamePacketT Packet;
    uint32_t uClient;

    // make sure we're currently hosting
    if (pRef->eState != ST_HOSTING)
    {
        NetPrintf(("lobbylan: cannot start a game without hosting a game\n"));
        return(-1);
    }

    // make sure there is more than one player
    if (pRef->Game.GameInfo.uNumPlayers < 2)
    {
        NetPrintf(("lobbylan: cannot start a game with out at least two players\n"));
        return(-2);
    }

    // set up packet
    memset(&Packet, 0, sizeof(Packet));
    Packet.head.kind = GAME_PACKET_USER;
    Packet.head.len = 0;

    // tell all of the clients the game is starting
    for (uClient = pRef->Game.GameInfo.uNumPlayers-1; uClient > 0; uClient--)
    {
        NetPrintf(("lobbylan: sending start message to client %d (0x%08x)\n", uClient, pRef->Game.pConnList[uClient].pLinkRef));
        _LobbyLanGameLinkSend(pRef, pRef->Game.pConnList[uClient].pLinkRef, &Packet, LOBBYLAN_HOSTMESG_STRT);

        // if not keeping the connections, blow them away        
        if (!pRef->bKeepConn)
        {
            // close the connection
            _LobbyLanCloseConn(pRef, &pRef->Game.pConnList[uClient], FALSE, FALSE);
            // remove player and connection from player/connection list
            _LobbyLanDelPlayer(pRef, uClient);
        }
    }

    // reset the advertisement/listen ref to kill advertisement
    NetGameUtilReset(pRef->pGameUtilRef);

    // transition to starting state
    pRef->eState = (!pRef->bKeepConn) ? ST_STARTING : ST_STARTED;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanJoinGame

    \Description
        Join a game.

    \Input *pRef        - module state ref
    \Input *pName       - name of game to join
    \Input *pPassword   - game password

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanJoinGame(LobbyLanRefT *pRef, const char *pName, const char *pPassword)
{
    LobbyLanGameInfoT *pGameInfo;

    // make sure we're currently idle
    if (pRef->eState != ST_IDLE)
    {
        NetPrintf(("lobbylan: cannot join a game when not idle\n"));
        return(-1);
    }
    
    // find game based on name
    if ((pGameInfo = _LobbyLanGetGame(pRef, pName)) == NULL)
    {
        NetPrintf(("lobbylan: game '%s' not in list and cannot be joined\n", pName));
        return(-2);
    }
    
    // join the game
    return(_LobbyLanJoinGame(pRef, pGameInfo, pPassword));
}
    
/*F********************************************************************************/
/*!
    \Function LobbyLanJoinGameByIndex

    \Description
        Join a game by index.

    \Input *pRef        - module state ref
    \Input iGameIndex   - index of game
    \Input *pPassword   - game password

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanJoinGameByIndex(LobbyLanRefT *pRef, int32_t iGameIndex, const char *pPassword)
{
    LobbyLanGameInfoT *pGameInfo;

    // make sure we're currently idle
    if (pRef->eState != ST_IDLE)
    {
        NetPrintf(("lobbylan: cannot join a game when not idle\n"));
        return(-1);
    }
    
    // find game based on index
    if ((iGameIndex < 0) || (iGameIndex >= (signed)pRef->pGameList->uNumGames))
    {
        NetPrintf(("lobbylan: invalid game index %d specified\n", iGameIndex));
        return(-2);
    }
    
    // ref the game
    pGameInfo = &pRef->pGameList->Games[iGameIndex];
    
    // join the game
    return(_LobbyLanJoinGame(pRef, pGameInfo, pPassword));
}

/*F********************************************************************************/
/*!
    \Function LobbyLanSendto

    \Description
        Send a packet to a client using link interface.

    \Input *pRef        - module state ref
    \Input *pPacket     - pointer to packet data to send
    \Input iClientId    - index of client to send to

    \Output
        int32_t         - positive=bytes sent, else failure

    \Notes
        The input packet buffer should include at least one byte of space over the
        size of the data, for LobbyLan to append its kind byte to.

    \Version 04/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanSendto(LobbyLanRefT *pRef, NetGamePacketT *pPacket, int32_t iClientId)
{
    LobbyLanConnT *pConn;
    int32_t iSentSize;

    // validate connection id
    if (iClientId >= (signed)pRef->Game.GameInfo.uNumPlayers)
    {
        NetPrintf(("lobbylan: sendto player %d failed - not a valid index\n", iClientId));
        return(-1);
    }

    // ref the connection
    pConn = &pRef->Game.pConnList[iClientId];

    // make sure the connection is established
    if ((pConn->eState != ST_OPEN) || (pConn->pLinkRef == NULL))
    {
        return(0);
    }

    // send the data
    iSentSize = _LobbyLanGameLinkSend(pRef, pConn->pLinkRef, pPacket, LOBBYLAN_USERMESG_DATA);
    return((iSentSize > 0) ? (iSentSize - 1) : iSentSize);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanRecvfrom

    \Description
        Receive a packet from a peer using link interface.

    \Input *pRef        - module state ref
    \Input *pPacket     - [out] pointer to storage for received packet data
    \Input iClientId    - index of client to receive from

    \Output
        int32_t         - positive=received data, else zero

    \Version 04/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanRecvfrom(LobbyLanRefT *pRef, NetGamePacketT *pPacket, int32_t iClientId)
{
    NetGamePacketT *pPeekPacket;
    LobbyLanConnT *pConn;
    uint8_t uPacketType;
    int32_t iRecvLen;

    // validate connection id
    if (iClientId >= (signed)pRef->Game.GameInfo.uNumPlayers)
    {
        NetPrintf(("lobbylan: recvfrom player %d failed - not a valid index\n", iClientId));
        return(-1);
    }
    
    // ref the connection
    pConn = &pRef->Game.pConnList[iClientId];

    // make sure the connection is established
    if ((pConn->eState != ST_OPEN) || (pConn->pLinkRef == NULL))
    {
        return(0);
    }

    // see if there is any data waiting
    if (NetGameLinkPeek(pConn->pLinkRef, &pPeekPacket) <= 0)
    {
        return(0);
    }

    // if there is data and it isn't ours, let LobbyLan take a crack at it and try again
    if (pPeekPacket->body.data[pPeekPacket->head.len-1] != LOBBYLAN_USERMESG_DATA)
    {
        NetPrintf(("lanplay: calling update routine\n"));
        _LobbyLanUpdate(pRef, 0);
        if (NetGameLinkPeek(pConn->pLinkRef, &pPeekPacket) <= 0)
        {
            return(0);
        }
        if (pPeekPacket->body.data[0] != LOBBYLAN_USERMESG_DATA)
        {
            return(0);
        }
    }
    
    // read the data
    if ((iRecvLen = _LobbyLanGameLinkRecv(pRef, pConn->pLinkRef, TRUE, pPacket, &uPacketType)) > 0)
    {
        // make sure we didn't get lobbylan data
        if (uPacketType != LOBBYLAN_USERMESG_DATA)
        {
            NetPrintf(("lobbylan: LobbyLanRecvfrom() received packet of type %d\n", uPacketType));
        }
    }
    
    // return recv true/false to caller
    return(iRecvLen);    
    
}

/*F********************************************************************************/
/*!
    \Function LobbyLanLeaveGame

    \Description
        Leave current game

    \Input *pRef    - module state ref

    \Output
        int32_t     - negative=error, else success

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanLeaveGame(LobbyLanRefT *pRef)
{
    LobbyLanConnT *pConn;
    uint32_t uClient;

    // make sure we're currently joining or joined to a game
    if (pRef->eState == ST_IDLE)
    {
        NetPrintf(("lobbylan: cannot leave a game when idle\n"));
        return(-1);
    }

    // if we're a client quitting a game, notify host to be nice
    if (pRef->eState == ST_JOINED)
    {
        pConn = &pRef->Game.pConnList[0];
        
        if (pConn->pLinkRef != NULL)
        {
            NetGamePacketT Packet;
            
            NetPrintf(("lobbylan: sending leave notification to host\n"));
            memset(&Packet, 0, sizeof(Packet));
            Packet.head.kind = GAME_PACKET_USER;
            Packet.head.len = 0;
            if (_LobbyLanGameLinkSend(pRef, pConn->pLinkRef, &Packet, LOBBYLAN_CLNTMESG_QUIT) <= 0)
            {
                NetPrintf(("lobbylan: error sending quit message to host\n"));
            }
        }
    }

    // close all client connections
    for (uClient = 0; uClient < pRef->Game.GameInfo.uNumPlayers; uClient++)
    {
        pConn = &pRef->Game.pConnList[uClient];
        _LobbyLanDestroyGameRefs(&pConn->pUtilRef, &pConn->pLinkRef);
        pConn->eState = ST_INIT;
    }

    // if hosting, destroy advertising module
    if (pRef->iHosting == 1)
    {
        _LobbyLanUtilDestroy(pRef);
    }

    // clear allocated lists
    DirtyMemFree(pRef->Game.pPlyrList, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    DirtyMemFree(pRef->Game.pConnList, LOBBYLAN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);

    // clear game
    memset(&pRef->Game, 0, sizeof(pRef->Game));

    // reset state
    pRef->eState = ST_IDLE;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanGetGameList

    \Description
        Get game list.

    \Input *pRef                    - module state ref

    \Output
        const LobbyLanGameListT *   - pointer to current game list

    \Version 09/16/2004 (jbrookes)
*/
/********************************************************************************F*/
const LobbyLanGameListT *LobbyLanGetGameList(LobbyLanRefT *pRef)
{
    return(pRef->pGameList);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanGetGameInfo

    \Description
        Get current game info.

    \Input *pRef                    - module state ref
    \Input *pName                   - name of game to get, or NULL for the local game

    \Output
        const LobbyLanGameInfoT *   - pointer to current game info

    \Version 09/17/2004 (jbrookes)
*/
/********************************************************************************F*/
const LobbyLanGameInfoT *LobbyLanGetGameInfo(LobbyLanRefT *pRef, const char *pName)
{
    LobbyLanGameInfoT *pGameInfo = NULL;

    pGameInfo = (pName != NULL) ? _LobbyLanGetGame(pRef, pName) : &pRef->Game.GameInfo;
    return(pGameInfo);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanGetPlyrList

    \Description
        Get player list for local (hosted/joined) game.

    \Input *pRef        - module state ref
    \Input *pPlyrList   - [out] pointer to buffer to fill in with player list info

    \Version 09/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void LobbyLanGetPlyrList(LobbyLanRefT *pRef, LobbyLanPlyrListT *pPlyrList)
{
    pPlyrList->uNumPlayers = pRef->Game.GameInfo.uNumPlayers;
    pPlyrList->uMaxPlayers = pRef->Game.GameInfo.uMaxPlayers;
    pPlyrList->pPlayers = pRef->Game.pPlyrList;
}

/*F********************************************************************************/
/*!
    \Function LobbyLanStatus

    \Description
        Get status information.

    \Input *pRef    - pointer to module state
    \Input iSelect  - status selector
    \Input iData    - selector-specific
    \Input *pBuf    - [out] selector-specific
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'host' - returns true if hosting
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'self' - copies LobbyLanPlyrInfoT for self into output
            'slen' - length of send queue - 
            'stat' - copies NetGameLinkStatT for client iData into output buffer
            'strt' - returns true if game has been started
        \endverbatim

    \Version 03/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanStatus(LobbyLanRefT *pRef, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'host')
    {
        return(pRef->iHosting == 1);
    }
    if (iSelect == 'ingm')
    {
        return((pRef->eState == ST_HOSTING) || (pRef->eState == ST_JOINED) || (pRef->eState == ST_STARTED));
    }
    if (iSelect == 'self')
    {
        if (iBufSize >= (signed)sizeof(pRef->Self))
        {
            memcpy(pBuf, &pRef->Self, sizeof(pRef->Self));
            return(0);
        }
    }
    if ((iSelect == 'slen') && (iData < (signed)pRef->Game.GameInfo.uNumPlayers) && (pRef->Game.pConnList[iData].pLinkRef != NULL))
    {
        return(NetGameLinkControl(pRef->Game.pConnList[iData].pLinkRef, 'slen', 0, NULL));        
    }
    if ((iSelect == 'stat') && (pBuf != NULL) && (iBufSize == sizeof(NetGameLinkStatT)) &&
        (pRef->Game.pConnList != NULL) && (pRef->Game.pConnList[iData].pLinkRef != NULL))
    {
        NetGameLinkStatus(pRef->Game.pConnList[iData].pLinkRef, 'stat', 0, pBuf, iBufSize);
        return(0);
    }
    if (iSelect == 'strt')
    {
        return(pRef->eState == ST_STARTED);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function LobbyLanControl

    \Description
        Control behavior of module.

    \Input *pRef        - module state ref
    \Input iControl     - control selector
    \Input iValue       - selector-specific
    \Input *pValue      - selector-specific
    
    \Output
        int32_t         - selector-specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'advf' - set advert freq - iValue=frequency in seconds
            'dlat' - display latency - iValue=frequency in seconds [DEBUG ONLY]
            'gprt' - set port for game communications - iValue=port
            'kcon' - set keep-connection status - iValue=TRUE or FALSE
            'mwid' - set maximum packet width - iValue=size in bytes
            'time' - set peer connection timeout - iValue=timeout in ms (default 5000)
        \endverbatim

        Unhandled selectors are passed through to NetGameUtilControl() if the
        ref is not NULL.

    \Version 02/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t LobbyLanControl(LobbyLanRefT *pRef, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'advf')
    {
        pRef->iAdvtFreq = iValue;
        // fall through to let NetGameUtilControl() finish up
    }
    #if DIRTYCODE_LOGGING
    if (iControl == 'dlat')
    {
        pRef->iDisplayLate = iValue;
        return(0);
    }
    #endif
    if (iControl == 'gprt')
    {
        NetPrintf(("lobbylan: using game port %d\n", iValue));
        pRef->iGamePort = iValue;
        return(0);
    }
    if (iControl == 'lbuf')
    {
        NetPrintf(("lobbylan: setting gamelink buffer size to %d bytes\n", iValue));
        pRef->iLinkBufSize = iValue;
        return(0);
    }
    if (iControl == 'locl')
    {
        NetPrintf(("lobbylan: saving 'locl' setting %d \n", iValue));
        pRef->iLocalGames = iValue;

        //fall-through to netgameutil
    }
    if (iControl == 'kcon')
    {
        NetPrintf(("lobbylan: setting keep-connection %s\n", iValue ? "enabled" : "disabled"));
        pRef->bKeepConn = iValue;
        return(0);
    }
    if (iControl == 'minp')
    {
        pRef->iMaxInp = iValue;
        // fall through to let NetGameUtilControl() finish up
    }
    if (iControl == 'mout')
    {
        pRef->iMaxOut = iValue;
        // fall through to let NetGameUtilControl() finish up
    }
    if (iControl == 'mwid')
    {
        pRef->iMaxPacket = iValue;
        // fall through to let NetGameUtilControl() finish up
    }
    if (iControl == 'time')
    {
        NetPrintf(("lobbylan: setting timeout to %dms\n"));
        pRef->iTimeout = iValue;
        return(0);
    }
    if (pRef->pGameUtilRef != NULL)
    {
        NetGameUtilControl(pRef->pGameUtilRef, iControl, iValue);
        return(0);
    }
    else
    {
        return(-1);
    }
}
