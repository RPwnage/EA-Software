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
\Moduledef ConnApi ConnApi
\Modulemember Game
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"
#include "DirtySDK/comm/commudp.h"
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **********************************************************************/

//! maximum number of clients allowed
#if DIRTYCODE_XENON
    #define CONNAPI_MAXCLIENTS          (71)
#endif

// connection flags
#define CONNAPI_CONNFLAG_GAMECONN   (1)     //!< game connection supported
#define CONNAPI_CONNFLAG_VOIPCONN   (2)     //!< voip connection supported
#define CONNAPI_CONNFLAG_GAMEVOIP   (3)     //!< game and voip connections supported

// connection status flags
#define CONNAPI_CONNFLAG_CONNECTED   (4)     //!< set if connection succeeded
#define CONNAPI_CONNFLAG_GAMESERVER  (8)     //!< set if a gameserver connection
#define CONNAPI_CONNFLAG_DEMANGLED   (16)    //!< set if demangler was attempted
#define CONNAPI_CONNFLAG_PKTRECEIVED (32)    //!< set if packets are received (for game connection only)

// start flags
#define CONNAPI_GAMEFLAG_RANKED     (1)     //!< game is a ranked game
#define CONNAPI_GAMEFLAG_PRIVATE    (2)     //!< game is a private game

// error codes
#define CONNAPI_ERROR_INVALID_STATE     (-1)    //!< connapi is in an invalid state
#define CONNAPI_ERROR_CLIENTLIST_FULL   (-2)    //!< client list is full
#define CONNAPI_ERROR_SLOT_USED         (-3)    //!< selected slot already in use
#define CONNAPI_ERROR_SLOT_OUT_OF_RANGE (-4)    //!< selected slot is not an valid index into the client array
#define CONNAPI_CALLBACKS_FULL          (-5)    //!< $$TODO DEPRECATE (unused) - maximum number of callbacks registered
#define CONNAPI_CALLBACK_NOT_FOUND      (-6)    //!< $$TODO DEPRECATE (unused) - callback not found

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

#if DIRTYCODE_XENON
//! rank callback data
typedef struct ConnApiCbRankT
{
    int32_t iScore[CONNAPI_MAXCLIENTS];
    int32_t iTeam[CONNAPI_MAXCLIENTS];
    uint8_t uReportValid;
    uint8_t uPartialTableValid;
    uint8_t uScoreValid[CONNAPI_MAXCLIENTS];
} ConnApiCbRankT;

//! user info
typedef struct ConnApiUserInfoT
{
    uint16_t    uFlags;                 //!< user flags. (public or private slot, for example)
    char        strName[32];            //!< name of user
    DirtyAddrT  DirtyAddr;              //!< dirtyaddr address of user (some platform have per-user addresses (like XUIDs))
    uint8_t     bUserJoined;            //!< on platform with per-user join operations, indicate whether this was done.
} ConnApiUserInfoT;
#endif

//! connection info
typedef struct ConnApiConnInfoT
{
    uint16_t            uLocalPort;     //!< local (bind) port
    uint16_t            uMnglPort;      //!< demangled port, if any
    uint8_t             bDemangling;    //!< is demangling process ongoing? (internal use only)
    uint8_t             uConnFlags;     //!< connection status flags (CONNAPI_CONNSTAT_*)
    ConnApiConnStatusE  eStatus;        //!< status of connection (CONNAPI_STATUS_*)
    uint8_t             uPeerConnFailed; //!< peer connection fail flag (CONNAPI_CONNFLAG_*)
    #if DIRTYCODE_LOGGING
    uint8_t             bFallbackTimeoutLogged; //! fallback timeout was logged, no need to log again
    #endif
} ConnApiConnInfoT;

typedef struct ConnApiClientInfoT
{
    uint32_t            uId;                //!< unique connapi client id
    uint32_t            uAddr;              //!< external internet address of user
    uint32_t            uLocalAddr;         //!< internal address of user
    uint16_t            uGamePort;          //!< external (send) port to use for game connection, or zero to use global port
    uint16_t            uVoipPort;          //!< external (send) port to use for voip connection, or zero to use global port
    uint16_t            uLocalGamePort;     //!< local (bind) port to use for game connection, or zero to use send port
    uint16_t            uLocalVoipPort;     //!< local (bind) port to use for voip connection, or zero to use send port
    uint16_t            uTunnelPort;        //!< user's tunnel port, or zero to use default
    uint16_t            uLocalTunnelPort;   //!< user's local tunnel port
    uint8_t             uTunnelMode;        //!< prototunnel mode for this client
    uint8_t             bEnableQos;         //!< enable QoS for this client, call ConnApiControl() with 'sqos' and 'lqos' to configure QoS settings
    uint8_t             _pad[2];
    DirtyAddrT          DirtyAddr;          //!< dirtyaddr address of client
    char                strTunnelKey[64];   //!< user's contribution to the tunnel key
#if DIRTYCODE_XENON
    ConnApiUserInfoT
        UserInfo[NETCONN_MAXLOCALUSERS];    //!< info about users
#endif
} ConnApiClientInfoT;

//! connection type
typedef struct ConnApiClientT
{
    ConnApiClientInfoT  ClientInfo;
    ConnApiConnInfoT    GameInfo;       //!< info about game connection
    ConnApiConnInfoT    VoipInfo;       //!< info about voip connection
    NetGameUtilRefT     *pGameUtilRef;  //!< util ref for connection
    NetGameLinkRefT     *pGameLinkRef;  //!< link ref for connection
    NetGameDistRefT     *pGameDistRef;  //!< dist ref for connection (for app use; not managed by ConnApi)
    int32_t             iConnStart;     //!< NetTick() recorded at connection start
    int32_t             iTunnelId;      //!< tunnel identifier (if any)
    uint16_t            uConnFlags;     //!< CONNAPI_CONNFLAG_* describing the connection type (read-only)
    int16_t             iVoipConnId;    //!< voip connection identifier
    uint16_t            uFlags;         //!< internal client flags
    uint8_t             bAllocated;     //!< TRUE if this slot is allocated, else FALSE
    uint8_t             bEstablishVoip; //!< used to establish voip when enable QoS is set (after QoS has been validated), call ConnApiControl() with 'estv' to configure
} ConnApiClientT;

//! connection list type
typedef struct ConnApiClientListT
{
    int32_t         iNumClients;    //!< number of clients in list
    int32_t         iMaxClients;    //!< max number of clients
    ConnApiClientT  Clients[1];     //!< client array (variable length)
} ConnApiClientListT;

//! callback info
typedef struct ConnApiCbInfoT
{
    int32_t iClientIndex;               //!< index of client event is for
    uint32_t eType;                     //!< type of event (CONNAPI_CBTYPE_*)
    uint32_t eOldStatus;                //!< old status (CONNAPI_STATUS_*)
    uint32_t eNewStatus;                //!< new status (CONNAPI_STATUS_*)
    const ConnApiClientT* pClient;      //!< pointer to the corresponding client structure
#if DIRTYCODE_XENON    
    union
    {
        ConnApiCbRankT* pRankData;
    } TypeData;
#endif
} ConnApiCbInfoT;

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
DIRTYCODE_API ConnApiRefT *ConnApiCreate2(int32_t iGamePort, int32_t iMaxClients, ConnApiCallbackT *pCallback, void *pUserData, CommAllConstructT *pConstruct);

// this function should be called once the user has logged on and the input parameters are available
DIRTYCODE_API void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, uint32_t uSelfId);

// destroy the module state
DIRTYCODE_API void ConnApiDestroy(ConnApiRefT *pConnApi);

// host or connect to a game, with flags, and the possibility to import connection.
DIRTYCODE_API void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iClientListSize, int32_t iTopologyHostIndex, int32_t iPlatformHostIndex, int32_t iSessId, uint32_t uGameFlags);

// add a new client to a pre-existing game in the specified index.
DIRTYCODE_API int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, int32_t iClientIdx);

#if DIRTYCODE_XENON
// add a new user to a specific client in a pre-existing game.
DIRTYCODE_API int32_t ConnApiAddUser(ConnApiRefT *pConnApi, int32_t iClientIdx, ConnApiUserInfoT *pUserInfo);
#endif

// return the ConnApiClientT for the specified client (by id)
DIRTYCODE_API uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient);

// remove a current client from a game
DIRTYCODE_API void ConnApiRemoveClient(ConnApiRefT *pConnApi, int32_t iClientIdx);

#if DIRTYCODE_XENON
// remove a user from a specific client in a game.
DIRTYCODE_API void ConnApiRemoveUser(ConnApiRefT *pConnApi, int32_t iClientIdx, int32_t iUserIdx);
#endif

// redo all connections, using the host specified
DIRTYCODE_API void ConnApiMigrateTopologyHost(ConnApiRefT *pConnApi, int32_t iNewTopologyHostIndex);

// move platform ownership to new client
DIRTYCODE_API void ConnApiMigratePlatformHost(ConnApiRefT *pConnApi, int32_t iNewPlatformHostIndex, void *pPlatformData);

// start game session
DIRTYCODE_API void ConnApiStart(ConnApiRefT *pConnApi, uint32_t uStartFlags);

// stop game session
DIRTYCODE_API void ConnApiStop(ConnApiRefT *pConnApi);

// rematch game session
DIRTYCODE_API void ConnApiRematch(ConnApiRefT *pConnApi);

// rematch game session
DIRTYCODE_API void ConnApiSetPresence(ConnApiRefT *pConnApi, uint8_t uPresenceEnabled);

// disconnect from game
DIRTYCODE_API void ConnApiDisconnect(ConnApiRefT *pConnApi);

// get list of current connections
DIRTYCODE_API const ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *pConnApi);

// connapi status
DIRTYCODE_API int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize);

// connapi status (version 2)
DIRTYCODE_API int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize);

// connapi control
DIRTYCODE_API int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update connapi module (must be called directly if auto-update is disabled)
DIRTYCODE_API void ConnApiUpdate(ConnApiRefT *pConnApi);

// Register a new callback
DIRTYCODE_API int32_t ConnApiAddCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

// Removes a callback previously registered
DIRTYCODE_API int32_t ConnApiRemoveCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData);

//xenon only. Get the pattern the Session string would contain, for a given first-party session
DIRTYCODE_API char* ConnApiGetSessionPattern(const void* pFirstPartyStruct, int32_t *pFrom, int32_t *pLength);

#ifdef __cplusplus
};
#endif

//@}

#endif // _connapi_h

