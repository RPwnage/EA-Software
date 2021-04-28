/*H********************************************************************************/
/*!
    \File voipconnection.h

    \Description
        VoIP virtual connection manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 12/10/2008 (mclouatre) Added ref count to VOIP connections.
    \Version 1.2 01/06/2009 (mclouatre) Added field RemoteAddrFallbacks to VoipConnectionT structure. Create related functions
    \Version 1.3 10/26/2009 (mclouatre) Renamed from xbox/voipconnection.h to xenon/voipconnectionxenon.h
*/
/********************************************************************************H*/

#ifndef _voipconnection_h
#define _voipconnection_h

/*** Include files ****************************************************************/

#include "voippacket.h"

/*** Defines **********************************************************************/

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
//! maximum number of fallback send addresses per VOIP connection
#define VOIP_MAXADDRFALLBACKS  (8)
#endif

//! maximum number of session IDs per VOIP connection
#define VOIP_MAXSESSIONIDS  (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! voice receive callback
typedef void (VoipConnectRecvVoiceCbT)(VoipUserT *pRemoteUser, VoipMicrPacketT *pVoicePacket, void *pUserData);

//! new user register callback
typedef void (VoipConnectRegUserCbT)(VoipUserT *pRemoteUser, int32_t iLocalIdx, uint32_t bRegister, void *pUserData);

typedef struct SendAddrFallbackT
{
    uint32_t SendAddr;  //!< send address
    uint8_t bReachable; //!< FALSE if voip detected the IP addr as being unreachable
} SendAddrFallbackT;

//! connection structure
typedef struct VoipConnectionT
{
    struct sockaddr_in  SendAddr;       //!< address we send to

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    //! send addresses received from other references. Will be used only if send address in use becomes unreachable.
    SendAddrFallbackT SendAddrFallbacks[VOIP_MAXADDRFALLBACKS];
    #endif

    int32_t         iPrimaryRemoteUser;
    uint32_t        uClientId;      //!< clientId of remote user
    uint32_t        uSessionId[VOIP_MAXSESSIONIDS];  //!< list of IDs representing the set of sessions sharing this connection
    VoipUserT       RemoteUsers[VOIP_MAXLOCALUSERS]; //!< remote user(s) we're connecting/connected to
    uint32_t        uRemoteStatus;  //!< VOIP_REMOTE_*

    int32_t         iVoipServerConnId; //!< conn id used by the voip server to identify this connection, contains VOIP_CONNID_NONE if unknown by voip server

    enum
    {
        ST_DISC,
        ST_CONN,
        ST_ACTV
    } eState;                           //!< connection status

    VoipMicrPacketT VoipMicrPacket;     //!< buffered voip packet (for sending)
    uint8_t         aUserSubPktCount[VOIP_MAXLOCALUSERS]; //!< user-specific counters of sub-packets in the voip packet being built

    uint32_t        uLastSend;          //!< last time data was sent
    uint32_t        uLastRecv;          //!< last time data was received
    uint32_t        uSendSeqn;          //!< packet send sequence
    uint32_t        uRecvSeqn;          //!< sequence number of last received voice packet

    uint32_t        uVoiceSendTimer;    //!< timer used to rate limit voice packets to ten per second

    int32_t         iChangePort;        //!< number of port changes
    int32_t         iChangeAddr;        //!< number of address changes
    int32_t         iRecvPackets;       //!< number of valid packets received
    int32_t         iRecvData;          //!< total amount of data received
    int32_t         iSendResult;        //!< previous socket send result

    uint32_t        uRecvChannelId;     //!< id of the channels the other client is sending on
    uint32_t        uLastRecvChannelId; //!< last channel that our customer was notified about

    uint8_t         bMuted;             //!< true if muted, else false
    uint8_t         bConnPktRecv;       //!< true if Conn packet received from other party, else false
    uint8_t         _pad[2];
} VoipConnectionT;

//! connection list
typedef struct VoipConnectionlistT
{
    SocketT                 *pSocket;           //!< master socket, used to send/recv all data
    NetCritT                NetCrit;            //!< critical section

    VoipConnectRecvVoiceCbT *pVoiceCb;          //!< callback to call when voice data is received
    VoipConnectRegUserCbT   *pRegUserCb;        //!< callback to call when a new user is registered
    void                    *pCbUserData;       //!< user data to be sent to callbacks

    int32_t                 iMaxConnections;    //!< maximum number of connections
    VoipConnectionT         *pConnections;      //!< connection array

    uint32_t                uUserSendMask;      //!< application-specified send mask
    uint32_t                uSendMask;          //!< bitmask of who we are sending voice data to
    uint32_t                uUserRecvMask;      //!< application-specified recv mask
    uint32_t                uRecvMask;          //!< bitmask of who we are accepting voice data from
    uint32_t                uRecvVoice;         //!< bitmask of who we are currently receiving voice from
    uint32_t                uLocalStatus;       //!< current local status (VOIP_LOCAL_*)
    uint32_t                uFriendsMask;       //!< Computed mask specifying which channels are to friends
    uint32_t                uLastVoiceTime;     //!< used to time out VOIP_LOCAL_TALKING status

    VoipUserT               LocalUsers[VOIP_MAXLOCALUSERS]; //!< local user's userID; sent to remote peers during connect phase
    int32_t                 iPrimaryLocalUser;  //!< index of "primary" user in LocalUser array
    uint32_t                uChannelId;         //!< id of the channels this client is sending on

    uint32_t                uClientId;          //!< clientId sent in every packet

    uint32_t                uBindPort;          //!< port socket is bound to for receive

    int32_t                 iDataTimeout;       //!< connection data timeout in milliseconds

    uint8_t                 bSentVoiceData;     //!< current voice data has been sent (used in server mode)
    uint8_t                 bUpdateMute;        //!< TRUE if the mute list needs to be updated, else FALSE
    int8_t                  iFriendConnId;
} VoipConnectionlistT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init a connectionlist
int32_t  VoipConnectionStartup(VoipConnectionlistT *pConnectionlist, int32_t iMaxPeers);

// set callbacks
void VoipConnectionSetCallbacks(VoipConnectionlistT *pConnectionlist, VoipConnectRecvVoiceCbT *pVoiceCb, VoipConnectRegUserCbT *pRegUserCb, void *pUserData);

// close a connectionlist
void VoipConnectionShutdown(VoipConnectionlistT *pConnectionlist);

// Check whether a given Connection ID can be allocated in this Connection List
uint8_t  VoipConnectionCanAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// start a virtual connection with peer
int32_t  VoipConnectionStart(VoipConnectionlistT *pConnectionlist, int32_t iConnID, VoipUserT *pUserID, uint32_t uAddr, uint32_t uConnPort, uint32_t uBindPort, uint32_t uClientId, uint32_t uSessionId);

// update virtual connections
void VoipConnectionUpdate(VoipConnectionlistT *pConnectionlist);

// stop a virtual connection
void VoipConnectionStop(VoipConnectionlistT *pConnectionlist, int32_t iConnID, int32_t bSendDiscMsg);

// remove a connection and contract connectionlist
void VoipConnectionRemove(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// flush any currently queued voice data on given connection (currently only does anything on Xenon)
int32_t VoipConnectionFlush(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// send voice data on connection(s)
void VoipConnectionSend(VoipConnectionlistT *pConnectionlist, const uint8_t *pVoiceData, int32_t iDataSize, uint32_t uLocalIndex, uint8_t uSendSeqn);

// set connections to send to
void VoipConnectionSetSendMask(VoipConnectionlistT *pConnectionlist, uint32_t uConnMask);

// set connections to receive from
void VoipConnectionSetRecvMask(VoipConnectionlistT *pConnectionlist, uint32_t uRecvMask);

// register/unregister remote talkers associated with the given connectionID
void VoipConnectionRegisterRemoteTalkers(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t bRegister);

// add a session ID to the set of sessions sharing the voip connection
int32_t VoipConnectionAddSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId);

// delete a session ID to the set of sessions sharing the voip connection
int32_t VoipConnectionDeleteSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId);

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
// add an address to the list of send address fallbacks
int32_t VoipConnectionAddFallbackAddr(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uAddr);

// delete an address from the list of send address fallbacks
int32_t VoipConnectionDeleteFallbackAddr(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uAddr);
#endif

#ifdef __cplusplus
};
#endif

#endif // _voipconnection_h

