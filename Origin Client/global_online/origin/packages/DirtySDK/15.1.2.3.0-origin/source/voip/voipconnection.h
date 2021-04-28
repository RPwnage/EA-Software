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
//! maximum number of session IDs per VOIP connection
#define VOIP_MAXSESSIONIDS  (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! voice receive callback
typedef void (VoipConnectRecvVoiceCbT)(VoipUserT *pRemoteUsers, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData);

//! new remote user register callback
typedef void (VoipConnectRegUserCbT)(VoipUserT *pRemoteUser, uint32_t bRegister, void *pUserData);

typedef struct SendAddrFallbackT
{
    uint32_t        SendAddr;           //!< send address
    uint8_t         bReachable;         //!< FALSE if voip detected the IP addr as being unreachable
} SendAddrFallbackT;

//! type used to create linked list of reliable data entries
typedef struct LinkedReliableDataT
{
    struct LinkedReliableDataT *pNext;  //<! pointer to next entry
    uint32_t uEnqueueTick;              //<! time at which entry was enqueue
    ReliableDataT data;                 //<! reliable data buffe
} LinkedReliableDataT;

//! connection structure
typedef struct VoipConnectionT
{
    struct sockaddr_in  SendAddr;       //!< address we send to
    uint32_t        uSessionId[VOIP_MAXSESSIONIDS]; //!< list of IDs representing the set of sessions sharing this connection

    #if defined(DIRTYCODE_XBOXONE)
    VoipUserExT     RemoteUsers[VOIP_MAXLOCALUSERS_EXTENDED]; //!< remote user(s) we're connecting/connected to
    #else
    VoipUserT       RemoteUsers[VOIP_MAXLOCALUSERS_EXTENDED]; //!< remote user(s) we're connecting/connected to
    #endif

    uint32_t        uRemoteClientId;    //!< clientId of remote user
    uint32_t        uLocalClientId;     //!< local client id associated with the high-level connection
    uint32_t        uRemoteUserStatus[VOIP_MAXLOCALUSERS];      //!< VOIP_REMOTE_*
    uint32_t        uRemoteConnStatus;  //!< VOIP_CONN_*
    int32_t         iVoipServerConnId;  //!< conn id used by the voip server to identify this connection, contains VOIP_CONNID_NONE if unknown by voip server
    uint32_t        uUserLastRecv[VOIP_MAXLOCALUSERS]; 

    enum
    {
        ST_DISC,
        ST_CONN,
        ST_ACTV
    } eState;                           //!< connection status

    LinkedReliableDataT *pOutboundReliableDataQueue;                //!< linked list of outbound reliable data entries

    VoipMicrPacketT VoipMicrPacket[VOIP_MAXLOCALUSERS_EXTENDED];    //!< buffered voip packet (for sending), one per local user

    uint32_t        aVoiceSendTimer[VOIP_MAXLOCALUSERS_EXTENDED];   //!< timer used to rate limit voice packets to ten per second
    uint32_t        aRecvChannels[VOIP_MAXLOCALUSERS_EXTENDED];     //!< id of the channels to which remote users on this connection are subscribed to
    uint32_t        aLastRecvChannels[VOIP_MAXLOCALUSERS_EXTENDED]; //!< last value of aRecvChannels

    uint32_t        uLastSend;          //!< last time data was sent
    uint32_t        uLastRecv;          //!< last time data was received
    uint32_t        uSendSeqn;          //!< packet send sequence
    uint32_t        uRecvSeqn;          //!< sequence number of last received voice packet

    int32_t         iChangePort;        //!< number of port changes
    int32_t         iChangeAddr;        //!< number of address changes
    int32_t         iRecvPackets;       //!< number of valid packets received
    int32_t         iRecvData;          //!< total amount of data received
    int32_t         iSendResult;        //!< previous socket send result
    int32_t         iMaxSubPktSize;     //!< max outbound sub-pkt size encountered during the lifetime of this connection

    //! variables used to implement outbound reliability flow  (reliable DATA from us to remote peer)
    uint8_t         bPeerIsAcking;      //!< TRUE if peer is still acking us
    uint8_t         uFirstReliableSeq;  //!< sequence number of first reliable date entry on this connection 
    uint8_t         bFirstReliableDone; //!< TRUE if sending of first reliable date entry is done on this connection 

    //! variables used to implement inbound reliability flow  (reliable DATA from remote peer to us)
    uint8_t         uInSeqNb;           //!< last successfully received seq nb from peer
    uint8_t         bPeerNeedsAck;      //!< TRUE if peer still expects acked seq number in packets

    //! misc
    uint8_t         bMuted;             //!< true if muted, else false
    uint8_t         bConnPktRecv;       //!< true if Conn packet received from other party, else false
    uint8_t         bAutoMuted;         //!< true if auto muting has already been performed we should only apply auto mute once per connection (only used on Xbox One)
} VoipConnectionT;

//! connection list
typedef struct VoipConnectionlistT
{
    SocketT         *pSocket;           //!< master socket, used to send/recv all data
    NetCritT        NetCrit;            //!< critical section

    VoipConnectRegUserCbT   *pRegUserCb;//!< callback to call when a new user is registered
    VoipConnectRecvVoiceCbT *pVoiceCb;  //!< callback to call when voice data is received
    void            *pCbUserData;       //!< user data to be sent to callbacks

    int32_t         iMaxConnections;    //!< maximum number of connections
    VoipConnectionT *pConnections;      //!< connection array

    //! memory group used to build the pre-allocated pFreeReliableDataPool
    int32_t         iMemGroup;          //!< module memgroup id
    void            *pMemGroupUserData; //!< user data associated with memgroup

    LinkedReliableDataT *pFreeReliableDataPool; //!< linked list of free pre-allocated reliable data buffers

    uint32_t        uUserSendMask;      //!< application-specified send mask
    uint32_t        uSendMask;          //!< bitmask of who we are sending voice data to
    uint32_t        uUserRecvMask;      //!< application-specified recv mask
    uint32_t        uRecvMask;          //!< bitmask of who we are accepting voice data from
    uint32_t        uRecvVoice;         //!< bitmask of who we are currently receiving voice from
    uint32_t        uLocalUserStatus[VOIP_MAXLOCALUSERS]; //!< current local status (VOIP_LOCAL_*)
    uint32_t        uFriendsMask;       //!< Computed mask specifying which channels are to friends
    uint32_t        uLastVoiceTime[VOIP_MAXLOCALUSERS];     //!< used to time out VOIP_LOCAL_USER_TALKING status

    #if defined(DIRTYCODE_XBOXONE)
    VoipUserExT     LocalUsers[VOIP_MAXLOCALUSERS_EXTENDED]; //!< Local Users
    #else
    VoipUserT       LocalUsers[VOIP_MAXLOCALUSERS_EXTENDED]; //!< Local Users
    #endif

    uint8_t         aIsParticipating[VOIP_MAXLOCALUSERS_EXTENDED]; //!< TRUE if user was moved to “participating” state with VoipActivateLocalUser(); FALSE otherwise

    uint32_t        aChannels[VOIP_MAXLOCALUSERS_EXTENDED]; //!< ids of the voip channels that local users are subscribed to

    uint32_t        uClientId;          //!< clientId sent in every packet

    uint32_t        uBindPort;          //!< port socket is bound to for receive

    int32_t         iDataTimeout;       //!< connection data timeout in milliseconds

    uint8_t         bSentVoiceData;     //!< current voice data has been sent (used in server mode)
    uint8_t         bUpdateMute;        //!< TRUE if the mute list needs to be updated, else FALSE
    int8_t          iFriendConnId;
  
      //! variables used to implement outbound reliability flow  (reliable DATA from us to remote peer)
    uint8_t         uOutSeqNb;          //!< seq nb to be assigned to next outbound reliable data entry (broadcasted on all active connections)
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
int32_t  VoipConnectionStart(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t uAddr, uint32_t uConnPort, uint32_t uBindPort, uint32_t uClientId, uint32_t uSessionId);

// update virtual connections
void VoipConnectionUpdate(VoipConnectionlistT *pConnectionlist);

// stop a virtual connection
void VoipConnectionStop(VoipConnectionlistT *pConnectionlist, int32_t iConnID, int32_t bSendDiscMsg);

// remove a connection and contract connectionlist
void VoipConnectionRemove(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// flush any currently queued voice data on given connection (currently only does anything on Xenon)
int32_t VoipConnectionFlush(VoipConnectionlistT *pConnectionlist, int32_t iConnID);

// send voice data on connection(s)
void VoipConnectionSend(VoipConnectionlistT *pConnectionlist, const uint8_t *pVoiceData, int32_t iDataSize, const uint8_t *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIndex, uint8_t uSendSeqn);

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

// broadcast local user join-in-progress data that needs to be sent reliably on all active connections
void VoipConnectionReliableBroadcastUser(VoipConnectionlistT *pConnectionlist, uint8_t uLocalUserIndex, uint32_t bParticipating);

// broadcast opaque reliable data that needs to be sent reliably on all active connections
void VoipConnectionReliableBroadcastOpaque(VoipConnectionlistT *pConnectionlist, const uint8_t *pData, uint16_t uDataSize);

#ifdef __cplusplus
};
#endif

#endif // _voipconnection_h

