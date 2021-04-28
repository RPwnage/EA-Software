/*************************************************************************************************/
/*!
    \file fire2connection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef FIRE2_CONNECTION_H
#define FIRE2_CONNECTION_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/******* Include files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/alloc.h"
#include "BlazeSDK/allocdefines.h"
#include "BlazeSDK/blazesender.h"
#include "BlazeSDK/blaze_eastl/string.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "BlazeSDK/component/framework/tdf/protocol.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"

/******* Defines/Macros/Constants/Typedefs ***********************************************************/

struct ProtoSSLRefT;

namespace Blaze
{

class TdfEncoder;
class TdfDecoder;
class Error;
class BlazeHub;
class ComponentManager;
class RpcJobBase;
class Fire2Frame;
class ProtoFire;
namespace ConnectionManager
{
    class ConnectionManager;
}

/*! ***********************************************************************/
/*! \class Fire2Connection
    \brief Represents a connection to a Blaze server.

    A Fire2Connection instance is the main entry point to the Blaze low-level 
    framework.  It contains the necessary information to connect to the server
    as well as an entry point to the connection's ComponentManager instances.
    These instances access components, which in turn send RPC messages.  The
    connection has two event functors, one for when the connection is made
    (or times out) and another for when the connection disconnects.

    \note
    This class cannot and should not be constructed/deconstructed directly.  Instead, the static
    Create and Destroy methods should be used.

    \nosubgrouping
***************************************************************************/
class BLAZESDK_API Fire2Connection : public BlazeSender, protected Idler
{
public:

    /*! ***************************************************************/
    /*! \brief The various states of the connection.
    *******************************************************************/
    enum ConnectionState
    {
        OFFLINE, ///< No connection is established.
        CONNECTING, ///< connect has been called, but the connection is not yet established.
        ONLINE ///< The connection is established.
    };

public:

    /*! ***************************************************************/
    /*! \brief Fire2Connection result callback functor.
    *******************************************************************/
    typedef Functor3<BlazeError, int32_t, int32_t> ConnectionCallback;
    typedef Functor ReconnectCallback;

    /*! ***************************************************************/
    /*! \name Construction/Deconstruction.
    *******************************************************************/

    /*! ***************************************************************************/
    /*! \brief Constructor
    
        \param hub Blaze hub this connection refers to.
    *******************************************************************************/
    Fire2Connection(BlazeHub& hub);
    virtual ~Fire2Connection();

    /*! ***************************************************************/
    /*! \name Connection Functions
    *******************************************************************/

    /*! ***************************************************************/
    /*! \brief Provide the various callback methods.
            
        \param connectCallback The callback to be made when the connection is established or times out.
        \param disconnectCallback The callback to be made when the connection disconnects.
        \param reconnectBeginCallback Optional.  The callback is made when reconnection starts.
        \param reconnectEndCallback Optional. The callback is made when the reconnection attempt completes.
    *******************************************************************/
    void setConnectionCallbacks(
        const ConnectionCallback& connectCallback, const ConnectionCallback& disconnectCallback,
        const ReconnectCallback& reconnectBeginCallback, const ReconnectCallback& reconnectEndCallback);

    /*! ***************************************************************/
    /*! \brief Start connecting to a Blaze server.
            
        Once connect is called, the caller should idle the connection every 
        frame by calling idle().  If the connection is established, 
        ConnectionCallback will be called with ERR_OK as the return integer.  
        If the connection times out or is refused, ConnectionCallback will 
        be called with an error value as its parameter.

        This function will start the connection attempt on the next idle after
        connect is called.  In that way, there is only a single error path to 
        guard against.

        Specify what server to connect to by calling the parent BlazeSender::setServerConnInfo() method
    *******************************************************************/
    void connect();
    
    /*! ***************************************************************/
    /*! \brief Disconnects from the Blaze server.
        
        This function immediately disconnects from the Blaze server.  
        
        \note If the connection is terminated in this manner, the DisconnectCallback
              will not be called.
    *******************************************************************/
    void disconnect();  

    /*! ***************************************************************/
    /*! \brief Executes a socket level disconnect [DEBUG ONLY!]
        
        This is to only be used by testing/debugging code and not meant
        for development.
        
        \note This tricks the Blaze connection manager into thinking
                the server dropped the connection.
    *******************************************************************/
    void dirtyDisconnect();
   
    /*! ***************************************************************/
    /*! \name Connection Accessors
    *******************************************************************/

    /*! ***************************************************************/
    /*! \brief Returns the current state of the connection.
    *******************************************************************/
    ConnectionState getConnectionState() const { return mConnectionState; }

    /*! ***************************************************************/
    /*! \brief Returns true if the connection is online and actively sending
               and receiving data, or if it is trying to reconnect to the server.
    *******************************************************************/
    virtual bool isActive() { return (mConnectionState != Fire2Connection::OFFLINE) || mIsReconnecting; }

    /*! ***************************************************************/
    /*! \brief Returns true if the connection was lost unexpectedly, and
               reconnection attempts are underway.
    *******************************************************************/
    bool isReconnecting() const { return mIsReconnecting; }

    /*! ***************************************************************/
    /*! \brief When true, if not connected at the time an RPC is sent, a connection
               attempt will be automatically started.  Including service name resolution.
    *******************************************************************/
    bool getAutoConnect() { return mAutoConnect; }
    void setAutoConnect(bool autoConnect) { mAutoConnect = autoConnect; }

    /*! ***************************************************************/
    /*! \brief When true, if an unexpected disconnection occurs, the ReconnectBeginCallback
               will be called instead of DisconnectCallback, and periodic attempts to
               reconnect to the server will begin.
    *******************************************************************/
    bool getAutoReconnect() { return mAutoReconnect; }
    void setAutoReconnect(bool autoReconnect) { mAutoReconnect = autoReconnect; }

    /*! ***************************************************************/
    /*! \brief Get or set the interval (in milliseconds) between pings to the server.
    *******************************************************************/
    uint32_t getPingPeriod() const { return mPingPeriodMs; }
    void setPingPeriod(uint32_t milliseconds) { mPingPeriodMs = milliseconds; }

    /*! ***************************************************************/
    /*! \brief Get or set the amount of time (in milliseconds) each individual reconnection
               attempt has to succeed.  If this timeout elapses while reconnecting, the last connection
               attempt will be canceled, and a new reconnection attempt will be start once again.
    *******************************************************************/
    uint32_t getReconnectTimeout() const { return mReconnectTimeout; }
    void setReconnectTimeout(uint32_t timeout) { mReconnectTimeout = timeout; }

    /*! ***************************************************************/
    /*! \brief Get or set the amount of time (in milliseconds) before the an automatic disconnection
               will occur due to no data being received from the server.
               NOTE: This time is overridden by the ReconnectTimeout while reconnecting.
    *******************************************************************/
    uint32_t getInactivityTimeout() const { return (mIsReconnecting ? mReconnectTimeout : mInactivityTimeout); }
    void setInactivityTimeout(uint32_t timeout) { mInactivityTimeout = timeout; }

    /*! ***************************************************************/
    /*! \brief Get or set the amount of time (in milliseconds) a connection attempt has to succeed.
               NOTE: This time is overridden by the ReconnectTimeout while reconnecting.
    *******************************************************************/
    uint32_t getConnectionTimeout() const { return (mIsReconnecting ? mReconnectTimeout : mConnectionTimeout); }
    void setConnectionTimeout(uint32_t timeout) { mConnectionTimeout = timeout; }

    /*! ***************************************************************/
    /*! \brief Set the maximum number of reconnect attempts to make before giving up.
    *******************************************************************/
    void setMaxReconnectAttempts(uint32_t maxReconnectAttempts) { mMaxReconnectAttempts = maxReconnectAttempts; }

    void useResumeBuffer() { mUseResumeBuffer = true; }
    bool isMigrationInProgress() { return mIsMigrating; }
    void migrationComplete();

#ifdef BLAZE_ENABLE_FUZZING
    /*! ***************************************************************/
    /*! \brief Set the callback that can allow modification of payload data before packet processing.
               NOTE: This is intended for fuzzy testing.
    *******************************************************************/
    typedef void (FuzzCallbackT)(uint8_t *pData, size_t &dataSize);
    static void setFuzzCallback(FuzzCallbackT *pCallback);
#endif

protected:
    virtual void onServiceNameResolutionStarted();
    virtual void onServiceNameResolutionFinished(BlazeError result);

    virtual uint32_t getNextMessageId();

    virtual BlazeError canSendRequest();
    virtual BlazeError sendRequestToBuffer(uint32_t userIndex, uint16_t component, uint16_t command, MessageType type, const EA::TDF::Tdf *msg, uint32_t msgId);

    BlazeError sendRequestToBufferInternal(MultiBuffer &buffer, MessageType type, uint16_t component, uint16_t command, uint32_t msgId, uint32_t userIndex, const EA::TDF::Tdf * msg);

    virtual BlazeError sendRequestFromBuffer();

    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);

    virtual MultiBuffer& chooseSendBuffer();

private:
    void connectInternal();
    void reconnect();
    void reconnectInternal(bool attemptingReconnect);
    void onConnectFinished(BlazeError error);

    void disconnectInternal(BlazeError error);

    bool shouldResumeConnection(BlazeError error);
    bool isResumableError(BlazeError error);

    void processIncoming(uint32_t currentTime);

    void ackFrame(Fire2Frame &currentFrame);

    bool prepareReceiveBuffer(const Fire2Frame& currentFrame);
    bool receiveToBuffer(uint32_t expected, uint32_t currentTime);

    void checkPing();
    void sendPing();
    void handleReceivedPingReply(uint32_t userIndex, uint32_t msgId, Fire2Metadata &metadata);
    void handleReceivedPing(uint32_t userIndex, uint32_t msgId, Fire2Metadata &metadata);

    void reportDisconnectToTelemetry(BlazeError error, int32_t sslError, int32_t sockError);
    
    void getProtoErrors(int32_t& sslErr, int32_t& sockErr);
    const char8_t* getProtoSSlErrorName(int32_t sslErr);
    const char8_t* getProtoSocketErrorName(int32_t sockErr);

private:
    ConnectionState mConnectionState;
    bool mIsMigrating;

    JobId mConnectInternalJobId;
    uint32_t mLastConnectionAttempt;

    ConnectionCallback mConnectCallback;
    ConnectionCallback mDisconnectCallback;
    ReconnectCallback mReconnectBeginCallback;
    ReconnectCallback mReconnectEndCallback;

    Fire2Metadata mMetadata;

    uint32_t mNextMessageId;

    bool mAutoConnect;
    bool mAutoReconnect;
    uint32_t mReconnectAttempts;
    uint32_t mMaxReconnectAttempts;

    ProtoSSLRefT *mProtoSSLRef;

    Heat2Encoder mEncoder;
    Heat2Decoder mDecoder;

    enum ProtocolState
    {
        READ_HEADER,
        READ_BODY
    };

    ProtocolState mProtocolState;

    JobId mSendPingJobId;
    uint32_t mPingPeriodMs;
    uint32_t mConnectionTimeout;
    uint32_t mReconnectTimeout;
    uint32_t mInactivityTimeout;
    bool mIsReconnecting;

    BlazeError mDisconnectError;        // Checked to see if the disconnect was caused by a MOVE event or other action. 

#ifdef BLAZE_ENABLE_FUZZING
    static FuzzCallbackT *mFuzzCb;
#endif

    MultiBuffer mResumeBuffer;
    bool mUseResumeBuffer;
};

}; //Blaze

#endif //FIRE2_CONNECTION_H
