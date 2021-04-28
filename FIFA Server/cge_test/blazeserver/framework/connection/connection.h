/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONNECTION_H
#define BLAZE_CONNECTION_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_ptr.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/fixed_list.h"
#include "EASTL/array.h"

#include "eathread/eathread_storage.h"

#include "framework/tdf/usersessiontypes_server.h"
#include "framework/connection/blockingsocket.h"
#include "framework/protocol/protocol.h"
#include "framework/util/shared/rawbuffer.h"
#include "EATDF/time.h"
#include "framework/util/dispatcher.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifndef CONNECTION_LOGGER_CATEGORY
    #ifdef LOGGER_CATEGORY
        #define CONNECTION_LOGGER_CATEGORY LOGGER_CATEGORY
    #else
        #define CONNECTION_LOGGER_CATEGORY Log::CONNECTION
    #endif
#endif

namespace Blaze
{
typedef uint32_t ConnectionId;

class ConnectionOwner;
class ConnectionListener;
class ConnectionManager;
class OutboundConnectionManager;
class ConnectionOwner;
class SocketChannel;
class TdfEncoder;
class TdfDecoder;
class ConnectionStatus;
class Endpoint;
struct RatesPerIp;

typedef eastl::intrusive_ptr<RawBuffer> RawBufferPtr;

class Connection : public eastl::intrusive_list_node
{
    NON_COPYABLE(Connection);

public:
    // Note: CONNECTION_IDENT_MAX is also used to effectively limit the number of lower bits storing
    // the connection ident part in ConnectionGroupId appropriately. See UserSessionManager::makeConnectionGroupId. 
    static const ConnectionId CONNECTION_IDENT_MAX = 1048575; //(2 power 20 - 1 )
    static const ConnectionId INVALID_IDENT = UINT32_MAX;
    static const uint32_t PEER_NAME_LEN = 128;

    Connection(ConnectionOwner& owner, ConnectionId ident, SocketChannel& channel, Blaze::Endpoint& endpoint);
    virtual ~Connection();

    ConnectionId getIdent() const { return mIdent; }
    ConnectionOwner& getOwner() const { return mOwner; }

    BlazeRpcError decodeTdf(RawBufferPtr buffer, EA::TDF::Tdf& tdf, Decoder::Type decoderType = Decoder::INVALID);
    const char8_t* getDecoderErrorMessage();
    bool encodeTdf(RawBufferPtr buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame, Encoder::Type encoderType = Encoder::INVALID);
    TdfEncoder& getEncoder() const { return *mEncoder; }
    TdfDecoder& getDecoder() const { return *mDecoder; }

    const InetAddress& getPeerAddress() const { return mBlockingSocket.getChannel().getPeerAddress(); }
    const InetAddress& getRealPeerAddress() const { return mBlockingSocket.getChannel().getRealPeerAddress(); }
    bool isRealPeerAddressResolved() const { return mBlockingSocket.getChannel().isRealPeerAddressResolved(); }

    bool isTrustedPeer() const { return mBlockingSocket.getChannel().isTrustedPeer(); }

    Endpoint& getEndpoint() const { return mEndpoint; }
    Protocol& getProtocolBase() const { return *mProtocol; }
    bool isPersistent() const { return getProtocolBase().isPersistent(); }
    bool isConnected() const { return !mClosed && mBlockingSocket.isConnected(); }
    bool isClosed() const { return mClosed; }
    bool isQueuedForDeletion() const { return mQueuedForDeletion; }

    bool isInactivityTimeoutSuspended();
    void setInactivitySuspensionTimeout(const EA::TDF::TimeValue& val){ mInactivitySuspensionTimeout = val; }
    EA::TDF::TimeValue getInactivitySuspensionTimeoutEndTime() const {return mInactivitySuspensionTimeoutEndTime;}
    void setInactivitySuspensionTimeoutEndTime(const EA::TDF::TimeValue& val){ mInactivitySuspensionTimeoutEndTime = val; }

    EA::TDF::TimeValue getLastActivityTime() { return mLastActivityTimeOverride > mBlockingSocket.getLastActivityTime() ? mLastActivityTimeOverride : mBlockingSocket.getLastActivityTime(); }
    void setLastActivityTimeOverride(const EA::TDF::TimeValue& currentTime) { mLastActivityTimeOverride = currentTime; }

    void getStatusInfo(ConnectionStatus& status) const;
    bool isSquelching() const { return mSquelchEvent.isValid(); }
    bool isAboveLowWatermark() const;

    RawBufferPtr getBuffer(size_t frameSize = 0);

    void addListener(ConnectionListener& listener) { mDispatcher.addDispatchee(listener); }
    void removeListener(ConnectionListener& listener) { mDispatcher.removeDispatchee(listener); }

    void setRealPeerAddress(const InetAddress& addr) { mBlockingSocket.getChannel().setRealPeerAddress(addr); }

    bool connect();
    bool accept();
    virtual void close();

    void scheduleClose();

    virtual bool isServerConnection() const { return false; }
    virtual bool isInboundConnection() const { return false; }
    virtual bool getIgnoreInactivityTimeout() const { return false; }

    void setCloseReason(UserSessionDisconnectType closeReason, int32_t socketError);
    void clearCloseReason();
    bool isResumableCloseReason(UserSessionDisconnectType closeReason) const;

    UserSessionDisconnectType getCloseReason() const
    {
        return mCloseReason;
    }

    int32_t getSocketError() const
    {
        return mSocketError;
    }

    // Get server name, ip address, and port
    const char8_t* toString(char8_t* buf, uint32_t len) const;
    
    const char8_t* getPeerName() const { return mPeerName; }

    void setPeerName(const char8_t* name);

    bool isShuttingDown() const { return mShuttingDown; }

protected:
    friend class ConnectionOwner; // needs access to isReadyForDeletion()
    friend class ConnectionManager; // needs access to setCloseReason()
    friend class OutboundConnectionManager; // needs access to setCloseReason()
    friend class UserSessionMaster; // needs access to setCloseReason()
    friend class UserSessionManager; // needs access to setCloseReason()

    void processInputBuffer();
    virtual void processMessage(RawBufferPtr& buffer) = 0;

    BlockingSocket& getBlockingSocket() { return mBlockingSocket; }
    const BlockingSocket& getBlockingSocket() const { return mBlockingSocket; }
    SocketChannel& getChannel() { return mBlockingSocket.getChannel(); }
    const SocketChannel& getChannel() const { return mBlockingSocket.getChannel(); }

    void queueOutputData(const RawBufferPtr& buffer, bool queueDirectly = true) { queueOutputData(buffer, RawBufferPtr(), queueDirectly); }
    void queueOutputData(const RawBufferPtr& buf1, const RawBufferPtr& buf2, bool queueDirectly);
    void spliceOutputQueues();
    void setupEncoder(Encoder::Type encoderType);

    bool isOutputQueueEmpty() const { return mOutputQueue.empty(); }
    uint32_t getOutputQueueSize() const { return (uint32_t) mOutputQueue.size(); }
    size_t getTotalQueuedBytes() const { return mQueuedAsyncBytes + mQueuedOutputBytes; }
    
    int32_t getLastError();

    void gracefulClose();
    
    virtual bool isKeepAliveRequired() const { return false; }
    virtual bool getQueueDirectly() const { return true; }

    // logging info
    UserSessionDisconnectType mCloseReason;
    int32_t mSocketError;

    //Used to track any child fibers we may have.
    Fiber::FiberGroupId mReadWriteLoopFiberGroupId;
    Fiber::FiberGroupId mProcessMessageFiberGroupId;

    bool mShuttingDown;
    EA::TDF::TimeValue mShutdownStartTime;
    
    // clears the input buffer.  on the next connection read, a new buffer will be allocated.
    void clearInputState(); 

    virtual bool hasOutstandingCommands() const = 0;
private:
    void connectInternal();
    void readLoop();
    void writeLoop();
    void signalUnsquelched(Fiber::EventHandle squelchEvent);

    void gracefulClosureAckExpired();
    
    void closeWithLargeFiber();
    void gracefulCloseWithLargeFiber();
    void processInputBufferWithLargeFiber();

    typedef eastl::array<RawBufferPtr, 2> BufferArray;
    struct OutputChunk
    {
        BufferArray mData;
    };

    typedef eastl::fixed_list<OutputChunk, 256> OutputList; // min list size is 256*16 = 4K

    // Queue of item buffers pending output to connected channel
    OutputList mOutputQueue;

    //Queue of async notifications that are pending output to connected channel after the current message is finished processing.
    OutputList mAsyncQueue;

    uint64_t mQueuedOutputBytes;
    uint64_t mQueuedAsyncBytes;

    uint64_t mSendBytes;
    uint64_t mSendCount;
    uint64_t mRecvBytes;
    uint64_t mRecvCount;
    EA::TDF::TimeValue mInactivitySuspensionTimeout;
    EA::TDF::TimeValue mInactivitySuspensionTimeoutEndTime;

    Fiber::FiberHandle mWriteFiberHandle;
    Fiber::FiberHandle mReadFiberHandle;
    Fiber::EventHandle mQueueWaitEvent;
    Fiber::EventHandle mSquelchEvent;

    ConnectionOwner& mOwner;

    typedef Dispatcher<ConnectionListener> ConnectionDispatcher;
    ConnectionDispatcher mDispatcher;
    ConnectionDispatcher& getDispatcher() { return mDispatcher; }

    ConnectionId mIdent;
    BlockingSocket mBlockingSocket;

    uint32_t mDefaultFrameSize;

    RawBufferPtr mInputBuffer;
    size_t mBytesAvailable;

    // Indicates if the connection is closed
    bool mClosed;

    // Indicates if connection has already been queued for deletion
    bool mQueuedForDeletion;

    Blaze::Endpoint& mEndpoint;

    // These pointers can change but never become nullptr, they point to objects owned by the ConnectionOwner.
    TdfEncoder* mEncoder;
    TdfDecoder* mDecoder;

    Protocol* mProtocol;

    // The last time a request was received on this connection
    EA::TDF::TimeValue mLastActivityTimeOverride;

    char8_t mPeerName[PEER_NAME_LEN];

    TimerId mSignalUnsquelchedTimerId;
        
    TimerId mShutdownAckTimer;

    Fiber::FiberId mClosingFiberId;
};

class ConnectionOwner
{
public:
    ConnectionOwner() : mQueuedRemoval(false) {}
    virtual bool isActive() const = 0;
    virtual void removeConnection(Connection& connection) = 0;    
    virtual ~ConnectionOwner();

    bool queuedRemovingConnections() { return mQueuedRemoval; }
    
    TdfEncoder* getOrCreateEncoder(Encoder::Type type);
    TdfDecoder* getOrCreateDecoder(Decoder::Type type);

protected:
    void removeConnections();

private:
    void queueForRemoval(Connection& connection);

private:
    friend class Connection;
    typedef eastl::intrusive_list<Connection> RemovalQueue;
    typedef eastl::vector_map<Encoder::Type, TdfEncoder*> EncoderByTypeMap;
    typedef eastl::vector_map<Decoder::Type, TdfDecoder*> DecoderByTypeMap;
    EncoderByTypeMap mEncoderByTypeMap;
    DecoderByTypeMap mDecoderByTypeMap;
    bool mQueuedRemoval;
    RemovalQueue mPendingRemovalQueue;
};

class ConnectionListener
{
public:
    virtual void onConnectionDisconnected(Connection& connection) = 0;
    virtual ~ConnectionListener() { }
};

class ConnectionSbFormat : public SbFormats::String
{
    NON_COPYABLE(ConnectionSbFormat);
public:
    ConnectionSbFormat(const Connection* connection)
        : mConnection(connection)
    {
    }

    const char8_t* getFormattedString() const override
    {
        if (mConnection != nullptr)
            mConnection->toString(mBuffer, sizeof(mBuffer));
        else
            blaze_strnzcpy(mBuffer, "(null)", sizeof(mBuffer));
        return mBuffer;
    };

private:
    const Connection* mConnection;
    mutable char8_t mBuffer[256];
};

} // Blaze

#endif // BLAZE_CONNECTION_H

