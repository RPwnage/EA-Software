/*H********************************************************************************/
/*!
    \File lobbylan.h

    \Description
        LAN-based lobby matchmaking module.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 09/15/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _lobbylan_h
#define _lobbylan_h

/*!
\Moduledef LobbyLan LobbyLan
\Modulemember Misc
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/comm/commudp.h"

/*** Defines **********************************************************************/

// game string size constants
#define LOBBYLAN_GAMENAME_LEN (32)          //!< max game name length
#define LOBBYLAN_PLYRNAME_LEN (32)          //!< max player name length
#define LOBBYLAN_PASSWORD_LEN (16)          //!< max game password length
#define LOBBYLAN_GAMENOTE_LEN (116)         //!< max game parameter length
#define LOBBYLAN_PLYRNOTE_LEN (116)         //!< max player parameter length

// game creation flags
#define LOBBYLAN_GAMEFLAG_NONE      (0)     //!< no game flags
#define LOBBYLAN_GAMEFLAG_AUTOSTART (1)     //!< autostart when game is filled

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! LobbyLan callback types
typedef enum LobbyLanCBTypeE
{
    // informational callbacks (user callback should return zero)
    LOBBYLAN_CBTYPE_GAMELIST,   //!< the list of available games has changed
    LOBBYLAN_CBTYPE_GAMEJOIN,   //!< indicates game has been successfully joined
    LOBBYLAN_CBTYPE_GAMEFULL,   //!< indicates game is full
    LOBBYLAN_CBTYPE_GAMEPASS,   //!< indicates password was rejected
    LOBBYLAN_CBTYPE_GAMEREFU,   //!< host refused join
    LOBBYLAN_CBTYPE_PLYRLIST,   //!< the list of players in game room has changed
    LOBBYLAN_CBTYPE_GAMESTRT,   //!< the game has been started
    LOBBYLAN_CBTYPE_GAMEKILL,   //!< the game has been withdrawn by the host
    LOBBYLAN_CBTYPE_PLYRKILL,   //!< the connection to the given player has been terminated

    // query callbacks (user callback must return a value)
    LOBBYLAN_CBTYPE_JOINREQU,   //!< [host only] - client join request - return >0 to accept, <0 to reject, 0 to wait

    LOBBYLAN_NUMCBTYPES         //!< number of callback types
} LobbyLanCBTypeE;

//! player info
typedef struct LobbyLanPlyrInfoT
{
    char        strName[LOBBYLAN_PLYRNAME_LEN]; //!< player name
    char        strNote[LOBBYLAN_PLYRNOTE_LEN]; //!< application-defined player info
    DirtyAddrT  Addr;                           //!< player address
} LobbyLanPlyrInfoT;                            //!< player info

//! player list
typedef struct LobbyLanPlyrListT
{
    uint32_t uNumPlayers;                   //!< number of players in list
    uint32_t uMaxPlayers;                   //!< max number of players allowed
    LobbyLanPlyrInfoT *pPlayers;                //!< array of one or more games
} LobbyLanPlyrListT;                            //!< player info

//! game info
typedef struct LobbyLanGameInfoT
{
    uint32_t uNumPlayers;                   //!< number of players currently in game
    uint32_t uMaxPlayers;                   //!< max number of players allowed in game
    uint32_t uHostAddr;                     //!< host address
    uint32_t bPassword;                     //!< TRUE if password required
    char strName[LOBBYLAN_GAMENAME_LEN];        //!< game name
    char strNote[LOBBYLAN_GAMENOTE_LEN];        //!< game-private data
} LobbyLanGameInfoT;

//! list of games
typedef struct LobbyLanGameListT
{
    uint32_t uNumGames;         //!< number of games in list
    uint32_t uMaxGames;         //!< maximum number of games in list
    LobbyLanGameInfoT Games[1];     //!< array of one or more games
} LobbyLanGameListT;

//! opaque LobbyLan ref
typedef struct LobbyLanRefT LobbyLanRefT;

//! LobbyLan callback function prototype
typedef int32_t (LobbyLanCallbackT)(LobbyLanRefT *pRef, LobbyLanCBTypeE eType, void *pCbData, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define LobbyLanCreate(_pSelfName, _pSelfAddr, _pKind, _uMaxGames) LobbyLanCreate2(_pSelfName, _pSelfAddr, _pKind, _uMaxGames, (CommAllConstructT *)CommUDPConstruct)

// create the LobbyLan module
DIRTYCODE_API LobbyLanRefT *LobbyLanCreate2(const char *pSelfName, const DirtyAddrT *pSelfAddr, const char *pKind, uint32_t uMaxGames, CommAllConstructT *pConstruct);

// destroy the LobbyLan module
DIRTYCODE_API void LobbyLanDestroy(LobbyLanRefT *pRef);

// set LobbyLan callback
DIRTYCODE_API void LobbyLanSetCallback(LobbyLanRefT *pRef, LobbyLanCallbackT *pCallback, void *pUserData);

// reset LobbyLan state
DIRTYCODE_API void LobbyLanReset(LobbyLanRefT *pRef);

// create a new game as host and join it
DIRTYCODE_API int32_t LobbyLanCreateGame(LobbyLanRefT *pRef, uint32_t uGameFlags, const char *pName, const char *pNote, const char *pPassword, uint32_t uMaxPlayers);

// update game note, if hosting a game
DIRTYCODE_API void LobbyLanSetNote(LobbyLanRefT *pRef, const char *pNote);

// update our player note
DIRTYCODE_API void LobbyLanSetPlyrNote(LobbyLanRefT *pRef, const char *pNote);

// start a game (game host only)
DIRTYCODE_API int32_t LobbyLanStartGame(LobbyLanRefT *pRef);

// join a game (client only)
DIRTYCODE_API int32_t LobbyLanJoinGame(LobbyLanRefT *pRef, const char *pName, const char *pPassword);

// join a game (client only)
DIRTYCODE_API int32_t LobbyLanJoinGameByIndex(LobbyLanRefT *pRef, int32_t iGameIndex, const char *pPassword);

// send data to a peer
DIRTYCODE_API int32_t LobbyLanSendto(LobbyLanRefT *pRef, NetGamePacketT *pPacket, int32_t iClientId);

// receive data from a peer
DIRTYCODE_API int32_t LobbyLanRecvfrom(LobbyLanRefT *pRef, NetGamePacketT *pPacket, int32_t iClientId);

// leave a game - if a host leaves the game, the game will be destroyed
DIRTYCODE_API int32_t LobbyLanLeaveGame(LobbyLanRefT *pRef);

// get current list of games
DIRTYCODE_API const LobbyLanGameListT *LobbyLanGetGameList(LobbyLanRefT *pRef);

// get game info
DIRTYCODE_API const LobbyLanGameInfoT *LobbyLanGetGameInfo(LobbyLanRefT *pRef, const char *pName);

// get player list (local game only)
DIRTYCODE_API void LobbyLanGetPlyrList(LobbyLanRefT *pRef, LobbyLanPlyrListT *pPlyrList);

// get module status
DIRTYCODE_API int32_t LobbyLanStatus(LobbyLanRefT *pRef, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// control behavior of module
DIRTYCODE_API int32_t LobbyLanControl(LobbyLanRefT *pRef, int32_t iControl, int32_t iValue, void *pValue);

#ifdef __cplusplus
};
#endif

//@}

#endif // _lobbylan_h
