/*H*************************************************************************************/
/*!
    \File    lanplay.c

    \Description
        Reference application for the LobbyLan module.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004.    ALL RIGHTS RESERVED.

    \Version    1.0        09/15/2004 (jbrookes) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/misc/lobbylan.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamelink.h"

#include "zlib.h"

#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

#define LANPLAY_SEND_RATE       (60)                        // send rate, in hz
#define LANPLAY_SEND_INTERVAL   (1000/LANPLAY_SEND_RATE)    // lanplay send interval, in ms

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct LanPlayRefT
{
    char strName[32];
    LobbyLanRefT *pLobbyLanRef;
    int32_t iMaxPacketSize;

    uint32_t bSendEnabled;

    int32_t iRecvSeqn;
    int32_t iSendSeqn;
    int32_t iLastSend;

    int32_t iRecvSize;
    int32_t iTotalSize;
    int32_t iStatTick;
} LanPlayRefT;

/*** Function Prototypes ***************************************************************/

static void _LanPlayCreate(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayDestroy(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayHost(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayStartGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayListGames(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayListPlayers(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayJoinGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayLeaveGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlaySetNote(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlaySetPlyrNote(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayXfer(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _LanPlayControl(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables *************************************************************************/


// Private variables

static T2SubCmdT _LanPlay_Commands[] =
{
    { "create", _LanPlayCreate      },
    { "destroy",_LanPlayDestroy     },
    { "host",   _LanPlayHost        },
    { "start",  _LanPlayStartGame   },
    { "games",  _LanPlayListGames   },
    { "players",_LanPlayListPlayers },
    { "join",   _LanPlayJoinGame    },
    { "leave",  _LanPlayLeaveGame   },
    { "leave",  _LanPlayLeaveGame   },
    { "setnote",_LanPlaySetNote     },
    { "setpnote",_LanPlaySetPlyrNote},
    { "xfer",   _LanPlayXfer        },
    { "control",_LanPlayControl    },
    { "",       NULL                }
};

static LanPlayRefT _LanPlay_Ref = { "LanPlay", NULL, 0, 0, 0, 0, 0, 0, 0, 0 };

// Public variables


/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _LanPlayCb

    \Description
        LobbyLan event callback handler.

    \Input *pRef        - pointer to lobbylan ref
    \Input eType        - event type
    \Input *pUserData   - pointer to lanplay app ref
    
    \Output
        int32_t             - zero, or join result code if LOBBYLAN_CBTYPE_JOINREQU

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LanPlayCb(LobbyLanRefT *pRef, LobbyLanCBTypeE eType, void *pCbData, void *pUserData)
{
    // the list of available games has changed
    if (eType == LOBBYLAN_CBTYPE_GAMELIST)
    {
        NetPrintf(("lobbylan: LOBBYLAN_CBTYPE_GAMELIST callback\n"));
    }

    // indicates gamejoin success/failure
    if (eType == LOBBYLAN_CBTYPE_GAMEJOIN)
    {
        NetPrintf(("lobbylan: LOBBYLAN_CBTYPE_GAMEJOIN callback\n"));
    }

    // the list of players in game room has changed
    if (eType == LOBBYLAN_CBTYPE_PLYRLIST)
    {
        NetPrintf(("lobbylan: LOBBYLAN_CBTYPE_PLYRLIST callback\n"));
    }

    // the game has been started
    if (eType == LOBBYLAN_CBTYPE_GAMESTRT)
    {
        NetPrintf(("lobbylan: LOBBYLAN_CBTYPE_GAMESTART callback\n"));
    }

    // the game has been withdrawn by host
    if (eType == LOBBYLAN_CBTYPE_GAMEKILL)
    {
        NetPrintf(("lobbylan: LOBBYLAN_CBTYPE_GAMEKILL callback\n"));
    }

    // host is checking to see if we should let a user join
    if (eType == LOBBYLAN_CBTYPE_JOINREQU)
    {
        static int32_t _iTest=0;

        NetPrintf(("lanplay: LOBBYLAN_CBTYPE_JOINREQU callback\n"));
        
        // simulate delay in checking for host join accept
        if (++_iTest < 60)
        {
            return(0);
        }
        // accept join request
        return(1);
    }

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _LanPlayStatsUpdate

    \Description
        Handle sending/receiving data.

    \Input *pData   - pointer to app ref
    \Input uTick    - ignored
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayStatsUpdate(LanPlayRefT *pRef, int32_t iPacketLen, uint32_t uPlyr)
{
    int32_t iTick = NetTick();
    
    // init stat tracker?
    if (pRef->iStatTick == 0)
    {
        pRef->iStatTick = iTick;
    }

    // update current recv size and total recv size
    pRef->iRecvSize += iPacketLen;
    pRef->iTotalSize += iPacketLen;

    // periodically output throughput
    if ((iTick - pRef->iStatTick) > 1000)
    {
        NetGameLinkStatT NetStats;
        
        if (LobbyLanStatus(pRef->pLobbyLanRef, 'stat', uPlyr, &NetStats, sizeof(NetStats)) == 0)
        {
            #if 1
            ZPrintf("lanplay: %2.2f k/s (%4dk) pps/rpps=%d/%d bps/rps/nps %d/%d/%d over=%d/%d pct=%d%%/%d%%\n",
                (float)pRef->iRecvSize/1024.0f, pRef->iTotalSize/1024,
                NetStats.pps, NetStats.rpps,
                NetStats.bps, NetStats.rps, NetStats.nps,
                NetStats.rps - NetStats.bps, NetStats.nps - NetStats.bps,
                NetStats.bps ? ((NetStats.rps - NetStats.bps) * 100) / NetStats.bps : 0,
                NetStats.bps ? ((NetStats.nps - NetStats.bps) * 100) / NetStats.bps : 0);
            #else // packet loss tracking
            static uint32_t _rpacksent = 0, _lpackrcvd = 0;
            uint32_t rpacksent, lpackrcvd;
            float packlostpct;
            
            rpacksent = NetStats.rpacksent - _rpacksent;
            lpackrcvd = NetStats.lpackrcvd - _lpackrcvd;
            packlostpct = (float)lpackrcvd*100.0f/rpacksent;
            ZPrintf("lanplay: lprcvd/rpsent/pct=%d/%d/%2.2f %s\n",
                lpackrcvd, rpacksent, packlostpct, packlostpct > 100.0f ? "!!!!!!!!!!!!" : "");
            _rpacksent = NetStats.rpacksent;
            _lpackrcvd = NetStats.lpackrcvd;
            #endif
            
            pRef->iRecvSize = 0;
            pRef->iStatTick = iTick;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayRecv

    \Description
        Try and receive data from player uPlyr.

    \Input *pRef    - pointer to app ref
    \Input uPlyr    - index of player to receive data from
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayRecv(LanPlayRefT *pRef, uint32_t uPlyr)
{
    NetGamePacketT Packet;
    int32_t iSeqn;
    
    if (pRef->iStatTick == 0)
    {
        pRef->iStatTick = NetTick();
    }

    // check for receive
    while (LobbyLanRecvfrom(pRef->pLobbyLanRef, &Packet, uPlyr) > 0)
    {
        if (sscanf((const char *)Packet.body.data, "%*s %*s %*s %d", &iSeqn) > 0)
        {
            pRef->iRecvSeqn = iSeqn;
        }
        else
        {
            NetPrintf(("failed to get sequence\n"));
        }
        _LanPlayStatsUpdate(pRef, Packet.head.len, uPlyr);
    }

    // update for when we don't receive
    _LanPlayStatsUpdate(pRef, 0, uPlyr);
}

/*F********************************************************************************/
/*!
    \Function _LanPlaySend

    \Description
        Try and send data to player uPlyr.

    \Input *pRef    - pointer to lobbylan ref
    \Input uPlyr    - index of player to send to
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LanPlaySend(LanPlayRefT *pRef, uint32_t uPlyr)
{
    NetGamePacketT Packet;

    // don't send unless enabled
    if (pRef->bSendEnabled == FALSE)
    {
        pRef->iLastSend = 0;
        return(FALSE);
    }
    
    // initialize send timer
    if (pRef->iLastSend == 0)
    {
        pRef->iLastSend = NetTick();
    }
    
    // wait to send?
    if (NetTickDiff(NetTick(), pRef->iLastSend) < LANPLAY_SEND_INTERVAL)
    {
        return(FALSE);
    }
    
    // if we have more than 16 packets queued in send buffer, don't send more
    if (LobbyLanStatus(pRef->pLobbyLanRef, 'slen', uPlyr, NULL, 0) > 16)
    {
        return(FALSE);
    }

    // set up a packet to send
    memset(&Packet, 0, sizeof(Packet));
    Packet.head.kind = GAME_PACKET_USER;
    sprintf((char *)Packet.body.data, "LanPlay Test Packet %d", pRef->iSendSeqn);
    
    // maxsize-1 because lobbylan consumes one byte
    Packet.head.len = pRef->iMaxPacketSize-1;

    // send packet
    if (LobbyLanSendto(pRef->pLobbyLanRef, &Packet, uPlyr) > 0)
    {
        // update send sequence and last send time
        pRef->iSendSeqn += 1;
        pRef->iLastSend += LANPLAY_SEND_INTERVAL;
        return(TRUE);
    }
    else
    {
        NetPrintf(("lanplay: unable to send\n"));
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayUpdate

    \Description
        Handle sending/receiving data.

    \Input *pData   - pointer to app ref
    \Input uTick    - ignored
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayUpdate(void *pData, uint32_t uTick)
{
    // get app ref
    LanPlayRefT *pRef = (LanPlayRefT *)pData;

    // if we are ingame
    if (LobbyLanStatus(pRef->pLobbyLanRef, 'ingm', 0, NULL, 0) == TRUE)
    {
        // get player list
        LobbyLanPlyrListT PlyrList;
        LobbyLanPlyrInfoT *pPlyrInfo, SelfInfo;
        uint32_t uPlyr;
    
        // get player list            
        LobbyLanGetPlyrList(pRef->pLobbyLanRef, &PlyrList);
    
        // get self info
        LobbyLanStatus(pRef->pLobbyLanRef, 'self', 0, &SelfInfo, sizeof(SelfInfo));

        // send/recv data to/from our peers           
        for (uPlyr = 0; uPlyr < PlyrList.uNumPlayers; uPlyr++)
        {
            pPlyrInfo = &PlyrList.pPlayers[uPlyr];
            if (!strcmp(pPlyrInfo->strName, SelfInfo.strName))
            {
                // don't send/recv to self
                continue;
            }
        
            // send packet(s) to peer
            while (_LanPlaySend(pRef, uPlyr) != 0)
                ;
        
            // try and receive data
            _LanPlayRecv(pRef, uPlyr);
        }
    }
}


/*

    Lanplay Commands
    
*/


/*F********************************************************************************/
/*!
    \Function _LanPlayCreate

    \Description
        Create the LanPlay module

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayCreate(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    char strTitleName[LOBBYLAN_GAMENAME_LEN] = "LobbyLan";
    char strPlyrName[LOBBYLAN_PLYRNAME_LEN] = "";
    DirtyAddrT LocalAddr;
    int32_t iArg;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create [name=<name> game=<game> maxp=<max packet size]]\n", argv[0]);
        return;
    }

    // make sure we don't already have a ref
    if (pRef->pLobbyLanRef != NULL)
    {
        ZPrintf("   %s: ref already created\n", argv[0]);
        return;
    }
    
    // init defaults
    pRef->iMaxPacketSize = NETGAME_DATAPKT_DEFSIZE;

    for (iArg = 2; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "name=", sizeof("name=")-1))
        {
            strcpy(strPlyrName, argv[iArg] + sizeof("name=")-1);
            NetPrintf(("lanplay: plyrname=%s\n", strPlyrName));
        }

        if (!strncmp(argv[iArg], "game=", sizeof("game=")-1))
        {
            strcpy(strTitleName, argv[iArg] + sizeof("game=")-1);
            NetPrintf(("lanplay: gamename=%s\n", strTitleName));
        }
        
        if (!strncmp(argv[iArg], "maxp=", sizeof("maxp=")-1))
        {
            pRef->iMaxPacketSize = atol(argv[iArg]+sizeof("maxp=")-1);
            NetPrintf(("lanplay: maxp=%d\n", pRef->iMaxPacketSize));
        }
    }

    // auto-fill name if not present
    if (strPlyrName[0] == '\0')
    {
        strcpy(strPlyrName, NetConnMAC());
    }

    // get local address
    DirtyAddrGetLocalAddr(&LocalAddr);

    // create module
    pRef->pLobbyLanRef = LobbyLanCreate(strPlyrName, &LocalAddr, strTitleName, 8);

    // add callback, and configure
    LobbyLanSetCallback(pRef->pLobbyLanRef, _LanPlayCb, pRef);
    LobbyLanControl(pRef->pLobbyLanRef, 'advf', 10, NULL);
    LobbyLanControl(pRef->pLobbyLanRef, 'mwid', pRef->iMaxPacketSize, NULL);

    // add update function
    NetConnIdleAdd(_LanPlayUpdate, pRef);
}

/*F********************************************************************************/
/*!
    \Function _LanPlayDestroy

    \Description
        Destroy the LanPlay module

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayDestroy(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (pRef->pLobbyLanRef != NULL)
    {
        LobbyLanDestroy(pRef->pLobbyLanRef);
        pRef->pLobbyLanRef = NULL;

        // remove idle function
        NetConnIdleDel(_LanPlayUpdate, pRef);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayHost

    \Description
        Host a LanPlay game

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayHost(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    char strGameName[LOBBYLAN_GAMENAME_LEN];
    char strGameNote[LOBBYLAN_PASSWORD_LEN] = "note";
    char strPassword[LOBBYLAN_PASSWORD_LEN] = "";
    int32_t iArg, iMaxPlayers = 2;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s host [name=<name> max=<maxplayers> pass=<password>]\n", argv[0]);
        return;
    }

    strcpy(strGameName, pRef->strName);

    for (iArg = 2; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "name=", sizeof("name=")-1))
        {
            strcpy(strGameName, argv[iArg] + sizeof("name=")-1);
            NetPrintf(("lanplay: gamename=%s\n", strGameName));
        }

        if (!strncmp(argv[iArg], "max=", sizeof("max=")-1))
        {
            iMaxPlayers = strtol(argv[iArg] + sizeof("max=")-1, NULL, 10);
            NetPrintf(("lanplay: iMaxPlayers=%d\n", iMaxPlayers));
        }

        if (!strncmp(argv[iArg], "pass=", sizeof("pass=")-1))
        {
            strcpy(strPassword, argv[iArg] + sizeof("pass=")-1);
            NetPrintf(("lanplay: password=%s\n", strPassword));
        }

        if (!strncmp(argv[iArg], "note=", sizeof("note=")-1))
        {
            strcpy(strGameNote, argv[iArg] + sizeof("note=")-1);
            NetPrintf(("lanplay: note=%s\n", strGameNote));
        }
    }

    if ((iArg = LobbyLanCreateGame(pRef->pLobbyLanRef, 0, strGameName, strGameNote, strPassword, iMaxPlayers)) < 0)
    {
        ZPrintf("   lanplay: error %d trying to create game\n", iArg);
    }
    else
    {
        ZPrintf("   lanplay: hosting game '%s'\n", strGameName);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayStartGame

    \Description
        Start a LanPlay game

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayStartGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s start\n", argv[0]);
        return;
    }

    LobbyLanStartGame(pRef->pLobbyLanRef);
}

/*F********************************************************************************/
/*!
    \Function _LanPlayListGames

    \Description
        Print a list of current LanPlay games

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayListGames(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    const LobbyLanGameListT *pGameList;
    const LobbyLanGameInfoT *pGameInfo;
    uint32_t uGame;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s games\n", argv[0]);
        return;
    }

    pGameList = LobbyLanGetGameList(pRef->pLobbyLanRef);
    
    ZPrintf("   %s: %d %s available\n", argv[0], pGameList->uNumGames, (pGameList->uNumGames != 1) ? "games" : "game");
    for (uGame = 0; uGame < pGameList->uNumGames; uGame++)
    {
        pGameInfo = &pGameList->Games[uGame];
        ZPrintf("    %d) %s %d/%d pw=%s note=%s\n", uGame,
            pGameInfo->strName, pGameInfo->uNumPlayers, pGameInfo->uMaxPlayers,
            pGameInfo->bPassword ? "yes" : "no",
            pGameInfo->strNote);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayListPlayers

    \Description
        Print a list of players in current LanPlay game, if any.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayListPlayers(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    LobbyLanPlyrListT PlyrList;
    uint32_t uPlyr;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s players\n", argv[0]);
        return;
    }

    LobbyLanGetPlyrList(pRef->pLobbyLanRef, &PlyrList);
    if (PlyrList.uNumPlayers > 0)
    {
        ZPrintf("   %s: %d/%d players\n", argv[0], PlyrList.uNumPlayers, PlyrList.uMaxPlayers);
        for (uPlyr = 0; uPlyr < PlyrList.uNumPlayers; uPlyr++)
        {
            ZPrintf("    %d) %s %s\n", uPlyr, PlyrList.pPlayers[uPlyr].strName,
                PlyrList.pPlayers[uPlyr].strNote);
        }
    }
    else
    {
        ZPrintf("   %s: not in a game\n", argv[0]);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayJoinGame

    \Description
        Join a game.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayJoinGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    char strGameName[LOBBYLAN_GAMENAME_LEN];
    char strPassword[LOBBYLAN_PASSWORD_LEN];
    int32_t iResult, iArg;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s join <name> [pass=<pass>]\n", argv[0]);
        return;
    }

    strcpy(strGameName, argv[2]);

    for (iArg = 3; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "pass=", sizeof("pass=")-1))
        {
            strcpy(strPassword, argv[iArg] + sizeof("pass=")-1);
            NetPrintf(("lanplay: password=%s\n", strPassword));
        }
    }

    if ((iResult = LobbyLanJoinGame(pRef->pLobbyLanRef, strGameName, strPassword)) < 0)
    {
        ZPrintf("   %s: error %d trying to join game '%s'\n", argv[0], iResult, strGameName);
    }
    else
    {
        ZPrintf("   %s: joining game '%s'\n", argv[0], strGameName);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlayLeaveGame

    \Description
        Leave a game.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 09/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayLeaveGame(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    int32_t iResult;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s leave");
        return;
    }

    if ((iResult = LobbyLanLeaveGame(pRef->pLobbyLanRef)) < 0)
    {
        ZPrintf("   %s: error %d trying to leave game\n", argv[0], iResult);
    }
    else
    {
        ZPrintf("   %s: left current game\n", argv[0]);
    }
}

/*F********************************************************************************/
/*!
    \Function _LanPlaySetNote

    \Description
        Set note field of current game.  Must be hosting.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 04/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlaySetNote(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s setnote <note>");
        return;
    }

    LobbyLanSetNote(pRef->pLobbyLanRef, argv[2]);    
}

/*F********************************************************************************/
/*!
    \Function _LanPlaySetPlyrNote

    \Description
        Set player note field for ourselves.  Must be hosting or joined to a game.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 04/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlaySetPlyrNote(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s setplyrnote <note>");
        return;
    }

    LobbyLanSetPlyrNote(pRef->pLobbyLanRef, argv[2]);    
}

/*F********************************************************************************/
/*!
    \Function _LanPlayXfer

    \Description
        Start/stop transferring data.

    \Inputs  Standard command arguments.
    
    \Output
        None.

    \Version 04/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _LanPlayXfer(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s xfer\n");
        return;
    }
    
    pRef->bSendEnabled = !pRef->bSendEnabled;
}

/*F*************************************************************************************/
/*!
    \Function _LanPlayControl
    
    \Description
        LobbyLan subcommand - LobbyLan control function
    
    \Input *_pRef
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false
    
    \Output
        None.
            
    \Version 1.0 06/12/2009 (cadam) First Version
*/
/**************************************************************************************F*/
static void _LanPlayControl(void *_pRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iCtrl;
    LanPlayRefT *pRef = (LanPlayRefT *)_pRef;

    if ((bHelp == TRUE) || (argc < 5))
    {
        ZPrintf("   usage: %s control [control] [value] [pvalue]\n", argv[0]);
        return;
    }

    iCtrl  = argv[2][0] << 24;
    iCtrl |= argv[2][1] << 16;
    iCtrl |= argv[2][2] << 8;
    iCtrl |= argv[2][3];

    LobbyLanControl(pRef->pLobbyLanRef, iCtrl, strtol(argv[3], NULL, 10), argv[4]);
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdLanPlay
    
    \Description
        LanPlay command dispatcher.
    
    \Input  - Standard Tester command argument list.
    
    \Output
        int32_t - zero
            
    \Version 1.0 09/15/2004 (jbrookes) First Version
*/
/**************************************************************************************F*/
int32_t CmdLanPlay(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    LanPlayRefT *pRef = &_LanPlay_Ref;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_LanPlay_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   manage lan game sessions using the lobbylan module\n");
        T2SubCmdUsage(argv[0], _LanPlay_Commands);
        return(0);
    }

    // if no ref yet, only allow create
    if ((pRef->pLobbyLanRef == NULL) && strcmp(pCmd->strName, "create"))
    {
        ZPrintf("   %s: ref has not been created - use create command\n", argv[0]);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pRef, argc, argv, bHelp);
    return(0);
}
