/*H********************************************************************************/
/*!
    \File connapi.h

    \Description
        ConnApi is a high-level connection manager, that packages the "connect to
        peer" process into a single module.  Both game connections and voice
        connections can be managed.  Multiple peers are supported in a host/client
        model for the game connection, and a peer/peer model for the voice
        connections.

    \Copyright
        Copyright (c) Electronic Arts 2005. ALL RIGHTS RESERVED.

    \Version 01/04/2005 (jbrookes) first version
*/
/********************************************************************************H*/

#ifndef _connapi_h
#define _connapi_h

/*!
    \Moduledef Game Game

    \Description
        ConnApi is a high-level connection manager, that packages the "connect to
        peer" process into a single module.  Both game connections and voice
        connections can be managed.  Multiple peers are supported in a host/client
        model for the game connection, and a peer/peer model for the voice
        connections.
*/
//@{


/*** Include files ****************************************************************/

#include "dirtyaddr.h"
#include "netgameutil.h"
#include "netgamelink.h"
#include "netgamedist.h"
#include "commudp.h"

/*** Defines **********************************************************************/

//! maximum number of clients allowed
#define CONNAPI_MAXCLIENTS          (65)

// connection flags
#define CONNAPI_CONNFLAG_GAMECONN   (1)     //!< game connection supported
#define CONNAPI_CONNFLAG_VOIPCONN   (2)     //!< voip connection supported
#define CONNAPI_CONNFLAG_GAMEVOIP   (3)     //!< game and voip connections supported

// connection status flags
#define CONNAPI_CONNFLAG_CONNECTED  (4)     //!< set if connection succeeded
#define CONNAPI_CONNFLAG_GAMESERVER (8)     //!< set if a gameserver connection
#define CONNAPI_CONNFLAG_DEMANGLED  (16)     //!< set if demangler was attempted

// start flags
#define CONNAPI_GAMEFLAG_RANKED	    (1)     //!< game is a ranked game
#define CONNAPI_GAMEFLAG_PRIVATE    (2)     //!< game is a private game

// error codes
#define CONNAPI_ERROR_INVALID_STATE     (-1)    //!< connapi is in an invalid state
#define CONNAPI_ERROR_CLIENTLIST_FULL   (-2)    //!< client list is full
#define CONNAPI_ERROR_SLOT_USED         (-3)    //!< selected slot already in use
#define CONNAPI_CALLBACKS_FULL          (-4)    //!< maximum number of callbacks registered
#define CONNAPI_CALLBACK_NOT_FOUND      (-5)    //!< callback not found

#define CONNAPI_FLAG_PUBLICSLOT          (1)
#define CONNAPI_FLAG_PRIVATESLOT         (2)

// gameserver modes
typedef enum ConnApiGameServModeE
{
   CONNAPI_GAMESERV_BROADCAST,              //!< rebroadcast mode
   CONNAPI_GAMESERV_PEERCONN                //!< peer connection mode
} ConnApiGameServModeE;

//! ConnApi callback types
typedef enum ConnApiCbTypeE
{
    CONNAPI_CBTYPE_GAMEEVENT,   //!< a game event occurred
    CONNAPI_CBTYPE_DESTEVENT,   //!< link/util ref destruction event
    CONNAPI_CBTYPE_VOIPEVENT,   //!< a voip event occurred
    CONNAPI_CBTYPE_SESSEVENT,   //!< a session event occurred (Xenon only)
    CONNAPI_CBTYPE_RANKEVENT,   //!< game is ending, connapi requires ranking data (Xenon only)

    CONNAPI_NUMCBTYPES          //!< number of callback types
} ConnApiCbTypeE;

//!< connection status
typedef enum ConnApiConnStatusE
{
    CONNAPI_STATUS_INIT,        //!< initialization state
    CONNAPI_STATUS_CONN,        //!< connecting to peer
    CONNAPI_STATUS_MNGL,        //!< demangling
    CONNAPI_STATUS_ACTV,        //!< connection established
    CONNAPI_STATUS_CLSE,        //!< connection closing
    CONNAPI_STATUS_DISC,        //!< disconnected

    CONNAPI_NUMSTATUSTYPES      //!< max number of status types
} ConnApiConnStatusE;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! rank callback data
typedef struct ConnApiCbRankT
{
    int32_t iScore[CONNAPI_MAXCLIENTS];
    int32_t iTeam[CONNAPI_MAXCLIENTS];
    uint8_t uValid;
} ConnApiCbRankT;

//! callback info
typedef struct ConnApiCbInfoT
{
    int32_t iClientIndex;               //!< index of client event is for
    uint32_t eType;                     //!< type of event (CONNAPI_CBTYPE_*)
    uint32_t eOldStatus;                //!< old status (CONNAPI_STATUS_*)
    uint32_t eNewStatus;                //!< new status (CONNAPI_STATUS_*)
    union
    {
        ConnApiCbRankT* pRankData;
    } TypeData;
} ConnApiCbInfoT;

//! user info
typedef struct ConnApiUserInfoT
{
    uint32_t    uAddr;                  //!< external internet address of user
    uint32_t    uLocalAddr;             //!< internal address of user
    uint32_t    uClientId;              //!< client identifier
    uint16_t    uGamePort;              //!< external (send) port to use for game connection, or zero to use global port
    uint16_t    uVoipPort;              //!< external (send) port to use for voip connection, or zero to use global port
    uint16_t    uLocalGamePort;         //!< local (bind) port to use for game connection, or zero to use send port
    uint16_t    uLocalVoipPort;         //!< local (bind) port to use for voip connection, or zero to use send port
    uint16_t    uTunnelPort;            //!< user's tunnel port, or zero to use default 
    uint16_t    uLocalTunnelPort;       //!< user's local tunnel port
    uint16_t    uClientFlags;           //!< client flags. (public or private slot, for example)
    DirtyAddrT  DirtyAddr;              //!< dirtyaddr address of user
    char        strName[32];            //!< name of user
    char        strUniqueId[32];        //!< unique id
} ConnApiUserInfoT;

//! connection info
typedef struct ConnApiConnInfoT
{
    uint16_t            uLocalPort;     //!< local (bind) port
    uint16_t            uMnglPort;      //!< demangled port, if any
    uint8_t             bDemangling;    //!< is demangling process ongoing? (internal use only)
    uint8_t             uConnFlags;     //!< connection status flags (CONNAPI_CONNSTAT_*)
    uint8_t             eStatus;        //!< status of connection (CONNAPI_STATUS_*)
    uint8_t             uPeerConnFailed; //!< peer connection fail flag (CONNAPI_CONNFLAG_*)
} ConnApiConnInfoT;

//! connection type
typedef struct ConnApiClientT
{
    ConnApiUserInfoT    UserInfo;       //!< info about user
    ConnApiConnInfoT    GameInfo;       //!< info about game connection
    ConnApiConnInfoT    VoipInfo;       //!< info about voip connection
    NetGameUtilRefT     *pGameUtilRef;  //!< util ref for connection
    NetGameLinkRefT     *pGameLinkRef;  //!< link ref for connection
    NetGameDistRefT     *pGameDistRef;  //!< dist ref for connection (for app use; not managed by ConnApi)
    int32_t             iConnStart;     //!< NetTick() recorded at connection start
    int32_t             iTunnelId;      //!< tunnel identifier (if any)
    int16_t             iVoipConnId;    //!< voip connection identifier
    uint16_t            uConnFlags;     //!< CONNAPI_CONNFLAG_* describing the connection type (read-only)
    uint16_t            uFlags;         //!< internal client flags
    uint8_t             bAllocated;     //!< TRUE if this slot is allocated, else FALSE
    uint8_t             _pad;
} ConnApiClientT;

//! connection list type
typedef struct ConnApiClientListT
{
    int32_t         iNumClients;    //!< number of clients in list
    int32_t         iMaxClients;    //!< max number of clients
    ConnApiClientT  Clients[1];     //!< client array (variable length)
} ConnApiClientListT;

//! opaque module ref
typedef struct ConnApiRefT ConnApiRefT;

//! connection event callback function prototype
typedef void (ConnApiCallbackT)(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
#define ConnApiCreate(_iGamePort, _iMaxClients, _pCallback, _pUserData) ConnApiCreate2(_iGamePort, _iMaxClients, _pCallback, _pUserData, (CommAllConstructT *)CommUDPConstruct)

// create the module state
ConnApiRefT *ConnApiCreate2(int32_t iGamePort, int32_t iMaxClients, ConnApiCallbackT *pCallback, void *pUserData, CommAllConstructT *pConstruct);

// this function should be called once the user has logged on and the input parameters are available
void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, const char *pSelfName, DirtyAddrT *pSelfAddr);

// destroy the module state
void ConnApiDestroy(ConnApiRefT *pConnApi);

// host or connect to a game, with flags, and the possibility to import connection.
void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiUserInfoT *pClientList, int32_t iClientListSize, int32_t iTopologyHostIndex, int32_t iPlatformHostIndex, int32_t iSessId, uint32_t uGameFlags);

// add a new client to a pre-existing game. 
int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo);

// add a new client to a pre-existing game in the specified index.
int32_t ConnApiAddClient2(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, int32_t iClientIdx);

// return the ConnApiClientT for the specified client (by name)
uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, ConnApiClientT *pOutClient);

// return the ConnApiClientT for the specified client (by id)
uint8_t ConnApiFindClientById(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, ConnApiClientT *pOutClient);

// remove a current client from a game
void ConnApiRemoveClient(ConnApiRefT *pConnApi, const char *pClientName, int32_t iClientIdx);

// redo all connections, using the host specified
void ConnApiMigrateTopologyHost(ConnApiRefT *pConnApi, int32_t iNewTopologyHostIndex);

// move platform ownership to new client
void ConnApiMigratePlatformHost(ConnApiRefT *pConnApi, int32_t iNewPlatformHostIndex, void *pPlatformData);

// start game session
void ConnApiStart(ConnApiRefT *pConnApi, uint32_t uStartFlags);

// stop game session
void ConnApiStop(ConnApiRefT *pConnApi);

// rematch game session
void ConnApiRematch(ConnApiRefT *pConnApi);

// rematch game session
void ConnApiSetPresence(ConnApiRefT *pConnApi, uint8_t uPresenceEnabled);

// disconnect from game 
void ConnApiDisconnect(ConnApiRefT *pConnApi);

// get list of current connections
const ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *pConnApi);

// connapi status
int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize);

// connapi status (version 2)
int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize);

// connapi control
int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update connapi module (must be called directly if auto-update is disabled)
void ConnApiUpdate(ConnApiRefT *pConnApi);

// Register a new callback
int32_t ConnApiAddCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

// Removes a callback previously registered
int32_t ConnApiRemoveCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

#ifdef __cplusplus
};
#endif

//@}

#endif // _connapi_h

