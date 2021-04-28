/*H********************************************************************************/
/*!
    \File voiptunnel.h

    \Description
        This module implements the main logic for the VoipTunnel server.

        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voiptunnel_h
#define _voiptunnel_h

/*!
\Moduledef VoipTunnel VoipTunnel
\Modulemember Voip
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

//! max number of clients that can appear in a group
#define VOIPTUNNEL_MAXGROUPSIZE         (32)

//! default amount of time elapsed before incoming voice is assumed to be stopped, in milliseconds
#define VOIPTUNNEL_RECVVOICE_TIMEOUT_DEFAULT        (1 * 1000)

//! default maximum number of players can be talking at once in a single game
#define VOIPTUNNEL_MAX_BROADCASTING_VOICES_DEFAULT   4

// client flags
#define VOIPTUNNEL_CLIENTFLAG_RECVVOICE          (1<<0) //!< voiptunnel is currently receiving voice from this client
#define VOIPTUNNEL_CLIENTFLAG_MAX_VOICES_REACHED   (1<<1) //!< this client has reached its max simultaneous talkers

#define VOIPTUNNEL_CLIENTFLAG_BROADCASTING_VOICE (1<<1) //!< TODO: V9 Deprecate  voiptunnel is broadcasting voice from this client
#define VOIPTUNNEL_GAMEFLAG_MAX_VOICES_REACHED   (1<<0) //!< TODO: V9 Deprecate  this game has reached its max simultaneous talkers

/*** Type Definitions *************************************************************/

//! voiptunnel event types
typedef enum VoipTunnelEventE
{
    VOIPTUNNEL_EVENT_ADDCLIENT,     //!< client added to client list
    VOIPTUNNEL_EVENT_DELCLIENT,     //!< client removed from client list
    VOIPTUNNEL_EVENT_MATCHADDR,     //!< client matched addr
    VOIPTUNNEL_EVENT_MATCHPORT,     //!< client matched port
    VOIPTUNNEL_EVENT_RECVVOICE,     //!< received voice data from client
    VOIPTUNNEL_EVENT_SENDVOICE,     //!< sending voice data to client
    VOIPTUNNEL_EVENT_DEADVOICE,     //!< voice connection has gone dead
    VOIPTUNNEL_EVENT_MAXDVOICE,     //!< additional clients attempting to send voice will not be rebroadcast
    VOIPTUNNEL_EVENT_AVLBVOICE,     //!< additional clients may once again send voice

    VOIPTUNNEL_NUMEVENTS            //!< total number of events
} VoipTunnelEventE;

//! voiptunnel client info
typedef struct VoipTunnelClientT
{
    uint32_t uRemoteAddr;       //!< remote address
    uint16_t uRemoteGamePort;   //!< remote game port
    uint16_t uRemoteVoipPort;   //!< remote voip port
    int16_t  iGameIdx;          //!< game index client is associated with (optional)
    uint8_t  uFlags;            //!< client flags
    uint8_t  uPacketVersion;    //!< version of voip protocol as identified by connection packets
    uint8_t  uTunnelMode;       //!< PROTOTUNNEL_MODE_*
    uint8_t  _pad[3];
    uint32_t uClientId;         //!< unique client identifier
    uint32_t uLastUpdate;       //!< last update time in milliseconds
    uint32_t uLastRecvVoice;    //!< last tick voice mic data was received from this client
    uint32_t uLastRecvGame;     //!< last tick game data was received from this client (not managed by voiptunnel; for user convenience)
    uint32_t uSendMask;         //!< current voice send mask (from client)
    uint32_t uGameSendMask;     //!< current voice send mask (from game)
    uint32_t uTunnelId;         //!< tunnel id -- for (optional) use by application
    int32_t  iNumClients;       //!< number of clients in client list
    int32_t  iNumTalker;        //!< number of other clients talking to this client
    int32_t  iUserData0;        //!< storage for user-specified data (not managed by voiptunnel; for user convenience)
    int32_t  aClientIds[VOIPTUNNEL_MAXGROUPSIZE];   //!< list of other clients in send group (GAMEFLAG_SEND_MULTI)
    char strTunnelKey[64];      //!< this client's portion of the tunnel key
} VoipTunnelClientT;

//! game structure
typedef struct VoipTunnelGameT
{
    int32_t iNumClients;                //!< current number of clients
    int32_t iNumClientsBroadcasting;    //!< TODO: V9 deprecate  number of clients talking simultaneously
    uint32_t uVoiceDataDropMetric;      //!< the number of voice packets that were not rebroadcast in this game due to max broadcasters constraint
    uint32_t uFlags;                    //!< game flags
    uint32_t aClientList[VOIPTUNNEL_MAXGROUPSIZE];
} VoipTunnelGameT;

//! opaque module state
typedef struct VoipTunnelRefT VoipTunnelRefT;

//! voiptunnel callback data
typedef struct VoipTunnelEventDataT
{
    VoipTunnelEventE    eEvent;
    VoipTunnelClientT  *pClient;
    int32_t             iDataSize;
} VoipTunnelEventDataT;

//! voiptunnel event callback
typedef void (VoipTunnelCallbackT)(VoipTunnelRefT *pVoipTunnel, const VoipTunnelEventDataT *pEventData, void *pUserData);

//! voiptunnel match function
typedef int32_t (VoipTunnelMatchFuncT)(VoipTunnelClientT *pClient, void *pUserData);

/*** Function Prototypes **********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
DIRTYCODE_API VoipTunnelRefT *VoipTunnelCreate(uint32_t uVoipPort, int32_t iMaxClients, int32_t iMaxGames);

// set optional voiptunnel event callback
DIRTYCODE_API void VoipTunnelCallback(VoipTunnelRefT *pVoipTunnel, VoipTunnelCallbackT *pCallback, void *pUserData);

// destroy the module state
DIRTYCODE_API void VoipTunnelDestroy(VoipTunnelRefT *pVoipTunnel);

// add a client to the client list
DIRTYCODE_API int32_t VoipTunnelClientListAdd(VoipTunnelRefT *pVoipTunnel, const VoipTunnelClientT *pClientInfo, VoipTunnelClientT **ppNewClient);

// add a client to the client list, at a specific index
DIRTYCODE_API int32_t VoipTunnelClientListAdd2(VoipTunnelRefT *pVoipTunnel, const VoipTunnelClientT *pClientInfo, VoipTunnelClientT **ppNewClient, int32_t iIndex);

// remove a client from the client list
DIRTYCODE_API void VoipTunnelClientListDel(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iClient);

// add a game to game list
DIRTYCODE_API int32_t VoipTunnelGameListAdd(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx);

// remove a game from game list
DIRTYCODE_API int32_t VoipTunnelGameListDel(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx);

// return client matching given address
DIRTYCODE_API VoipTunnelClientT *VoipTunnelClientListMatchAddr(VoipTunnelRefT *pVoipTunnel, uint32_t uRemoteAddr);

// return client matching given client identifier
DIRTYCODE_API VoipTunnelClientT *VoipTunnelClientListMatchId(VoipTunnelRefT *pVoipTunnel, uint32_t uClientId);

// return client at given client index
DIRTYCODE_API VoipTunnelClientT *VoipTunnelClientListMatchIndex(VoipTunnelRefT *pVoipTunnel, uint32_t uClientIndex);

// return client matching given address and port
DIRTYCODE_API VoipTunnelClientT *VoipTunnelClientListMatchSockaddr(VoipTunnelRefT *pVoipTunnel, struct sockaddr *pSockaddr);

// return client using user-supplied match function
DIRTYCODE_API VoipTunnelClientT *VoipTunnelClientListMatchFunc(VoipTunnelRefT *pVoipTunnel, VoipTunnelMatchFuncT *pMatchFunc, void *pUserData);

// let api know client's send mask should be updated
DIRTYCODE_API void VoipTunnelClientRefreshSendMask(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient);

// get module status
DIRTYCODE_API int32_t VoipTunnelStatus(VoipTunnelRefT *pVoipTunnel, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// control the module
DIRTYCODE_API int32_t VoipTunnelControl(VoipTunnelRefT *pVoipTunnel, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// update voiptunnel state
DIRTYCODE_API void VoipTunnelUpdate(VoipTunnelRefT *pVoipTunnel);

#ifdef __cplusplus
};
#endif

//@}

#endif // _voiptunnel_h

