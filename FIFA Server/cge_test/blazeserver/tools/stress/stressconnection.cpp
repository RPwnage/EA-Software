/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stressconnection.h"
#include "stressinstance.h"
#include "instanceresolver.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/sslsocketchannel.h"
#include "EATDF/tdf.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/sslcontext.h"
#include "framework/system/fiber.h"
#include "framework/connection/selector.h"

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

const int32_t MAX_RETRY_ATTEMPTS = 20;

StressConnection::StressConnection(int32_t id, bool secure, InstanceResolver& resolver, bool isRdirConn)
    : mIsRdirConn(isRdirConn),
      mSecure(secure),
      mResolver(resolver),
      mChannel(nullptr),
      mConnected(false),
      mSeqno(1),
      mRecvBuf(nullptr),
      mIdent(id),
      mBlazeId(INVALID_BLAZE_ID),
      mConnectionHandler(nullptr),
      mLastRpcTimestamp(0),
      mLastReadTimestamp(0),
      mLastConnectionAttemptTimestamp(0),
      mLastConnectionSuccessTimestamp(0),
      mLastRpcSendTimestamp(0),
      mClientType(CLIENT_TYPE_GAMEPLAY_USER),
      mMigrating(false),
      mSendingPingToNewInstance(false),
      mResolvingInstanceByRdir(true),
      mResolvingInstanceByErrMoved(false),
      mRetryCount(0)
{
    *mSessionKey = '\0';
}

bool StressConnection::initialize(const char8_t* protocolType, const char8_t* encoderType, const char8_t* decoderType)
{
    mProtocol = (RpcProtocol *) ProtocolFactory::create(
            ProtocolFactory::getProtocolType(protocolType), 1048576);

    mEncoder = static_cast<TdfEncoder*>(EncoderFactory::create(
                EncoderFactory::getEncoderType(encoderType)));
    mDecoder = static_cast<TdfDecoder*>(DecoderFactory::create(
                DecoderFactory::getDecoderType(decoderType)));

    if ((mProtocol == nullptr) || (mEncoder == nullptr) || (mDecoder == nullptr))
    {
        if (mProtocol != nullptr)
        {
            delete mProtocol;
            mProtocol = nullptr;
        }
        if (mEncoder != nullptr)
        {
            delete mEncoder;
            mEncoder = nullptr;
        }
        if (mDecoder != nullptr)
        {
            delete mDecoder;
            mDecoder = nullptr;
        }
        return false;
    }
    return true;
}


StressConnection::~StressConnection()
{
    if (mProtocol != nullptr)
        delete mProtocol;
    if (mEncoder != nullptr)
        delete mEncoder;
    if (mDecoder != nullptr)
        delete mDecoder;
    if (mChannel != nullptr)
    {
        delete mChannel;
        mChannel = nullptr;
    }
    if (mRecvBuf != nullptr)
        delete mRecvBuf;
    delete &mResolver;
}

bool StressConnection::connect(bool quiet)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    char8_t addressBuf[256];
    BLAZE_TRACE_LOG(Log::CONNECTION, "[" << getType() << "].connect: Doing StressConnection::connect");

    do {
        EA_ASSERT(!mConnected);

        mLastConnectionAttemptTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();

        if (!mAddress.isResolved()) {
            if (mResolvingInstanceByErrMoved) {
                BLAZE_ERR_LOG(Log::CONNECTION, "[" << getType() << "Connection].connect: Address " << mAddress.toString(addressBuf, sizeof(addressBuf)) <<
                    " returned by ERR_MOVED is not already resolved. Getting new address from redirector.");
                mResolvingInstanceByErrMoved = false;
            }
            mResolvingInstanceByRdir = true;
            if (!mResolver.resolve(mClientType, &mAddress))
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[" << getType() << "Connection].connect: " << mResolver.getName() << " failed to resolve address");
                if (shouldReconnect()) {
                    continue;
                }
                return false;
            }
        }

        mResolvingInstanceByRdir = false; // we have an valid address to connect to
        BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].connect: Connecting instance to " << mAddress.toString(addressBuf, sizeof(addressBuf)));

        if (mChannel != nullptr)
        {
            BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].connect: Already had a channel.  Deleting it before reconnecting");
            delete mChannel;
            mChannel = nullptr;
        }

        if (mSecure)
        {
            SslContextPtr sslContext = gSslContextManager->get();
            if (sslContext == nullptr)
                return false;
            mChannel = BLAZE_NEW SslSocketChannel(mAddress, sslContext);
        }
        else
            mChannel = BLAZE_NEW SocketChannel(mAddress);

        // Turn off the linger option for the socket managed by mChannel.  This limits the number of sockets that remain
        // in the TIME_WAIT state.  This can be important due to the number of local ports that are used by a machine running
        // 10s of 1000s of stress clients.
        mChannel->setLinger(false, 0);

        // don't resolve the IP more than once to avoid burning CPU on frequent connect/disconnect
        mChannel->setResolveAlways(false);

        BlazeRpcError rc = mChannel->connect();
        if (rc != ERR_OK)
        {
            if (quiet && !mResolvingInstanceByErrMoved)
            {
                BLAZE_TRACE_LOG(Log::CONNECTION,
                    "[" << getType() << "Connection].connect: "
                    "Channel failed to connect to " << mAddress.toString(addressBuf, sizeof(addressBuf)) << " (resumable error), err: " << ErrorHelp::getErrorName(rc));
            }
            else
            {
                BLAZE_ERR_LOG(Log::CONNECTION, 
                    "[" << getType() << "Connection].connect: "
                    "Channel failed to connect to " << mAddress.toString(addressBuf, sizeof(addressBuf)) <<
                    " (resolved by " << (mResolvingInstanceByErrMoved ? "ERR_MOVED" : mResolver.getName()) << "; resumable error), err: " 
                    << ErrorHelp::getErrorName(rc));
            }

            disconnect();
            // clear the address for reconnecting to redirector for new instance
            mResolvingInstanceByErrMoved = false;
            mAddress.reset();

            if (shouldReconnect())
                continue;

            return false;
        }
        else if (mMigrating)
        {
            BLAZE_INFO_LOG(Log::CONNECTION,
                "[" << getType() << "Connection].connect: Connected to new server " << mAddress.toString(addressBuf, sizeof(addressBuf)) << " during migration");
        }

        mChannel->setHandler(this);
        mChannel->addInterestOp(Channel::OP_READ);
        mConnected = true;

        if (mMigrating)
        {
            mSendingPingToNewInstance = true;
            if (mConnectionHandler != nullptr)
            {
                // stress instance will send PING rpc
                if (!mConnectionHandler->onUserSessionMigrated(this)) {
                    if (shouldReconnect())
                        continue;
                    return false;
                }
            }
        }
        mLastConnectionSuccessTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();
        return true;
    } while (true);
}

void StressConnection::disconnect()
{
    if (mChannel != nullptr)
    {
        mChannel->disconnect();
        delete mChannel;
        mChannel = nullptr;
    }

    if (mConnected)
    {
        mConnected = false;

        if (!mMigrating)
        {
            // clear out the send queues
            mSentRequestBySeqno.clear();
            mSendQueue.clear();

            typedef eastl::vector<SemaphoreId> SemaphoreIdList;
            SemaphoreIdList semaphoreIds;
            semaphoreIds.reserve(mProxyInfo.size());
            for (ProxyInfoBySeqno::const_iterator i = mProxyInfo.begin(), e = mProxyInfo.end(); i != e; ++i)
                semaphoreIds.push_back(i->second.semaphoreId);
            mProxyInfo.clear();
            // signal all pending semaphores
            for (SemaphoreIdList::const_iterator i = semaphoreIds.begin(), e = semaphoreIds.end(); i != e; ++i)
                gFiberManager->signalSemaphore(*i, ERR_DISCONNECTED);

            resetSessionKey("disconnect");

            // trigger the handler
            if (mConnectionHandler != nullptr)
                mConnectionHandler->onDisconnected(this);
        }
    }

    if (mResolvingInstanceByErrMoved) {
        mResolvingInstanceByErrMoved = false;

        char8_t addressBuf[256];
        BLAZE_ERR_LOG(Log::CONNECTION, "[" << getType() << "Connection].disconnect: Disconnecting from address " <<
            mAddress.toString(addressBuf, sizeof(addressBuf)) << " provided by ERR_MOVED; next connection attempt will resolve new instance from redirector");
    }
    // force connect() to resolve the IP again
    mAddress.reset();
}

void StressConnection::setConnectionHandler(ConnectionHandler* handler)
{
    mConnectionHandler = handler;
}

BlazeRpcError StressConnection::sendRequest(ComponentId component, CommandId command, const EA::TDF::Tdf* request, 
    EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory/* = Blaze::Log::CONNECTION*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = nullptr*/)
{
    return sendRequestWithType(RpcProtocol::MESSAGE, component, command, request, responseTdf, errorTdf, movedTo, logCategory, options, responseRaw);
}

BlazeRpcError StressConnection::sendPing()
{
    InstanceId movedTo;
    return sendRequestWithType(RpcProtocol::PING, 0, 0, nullptr, nullptr, nullptr, movedTo);
}

BlazeRpcError StressConnection::sendRequestWithType(RpcProtocol::MessageType messageType, ComponentId component, CommandId command, const EA::TDF::Tdf* request, 
    EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory/* = Blaze::Log::CONNECTION*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = nullptr*/)
{
    VERIFY_WORKER_FIBER();

    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    mLastRpcSendTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();

    if (!mConnected)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendRequest: "
            "send failed, not connected.");
        return ERR_SYSTEM;
    }

    // Frame the request
    RawBufferPtr buffer = BLAZE_NEW RawBuffer(2048);
    Blaze::RpcProtocol::Frame outFrame;
    outFrame.componentId = component;
    outFrame.commandId = command;
    outFrame.messageType = messageType;
    
    if (mMigrating && isLoggedIn())
    {
        // if user logout, sessionkey is reset
        outFrame.setSessionKey(mSessionKey);
    }

    mProtocol->setupBuffer(*buffer);
    if (request != nullptr)
    {
        if (!mEncoder->encode(*buffer, *request, &outFrame))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, 
                "[" << getType() << "Connection].sendRequest: "
                "failed to encode request for component: " << BlazeRpcComponentDb::getComponentNameById(outFrame.componentId) <<
                " rpc: " << BlazeRpcComponentDb::getCommandNameById(outFrame.componentId, outFrame.commandId, nullptr));
        }
    }
    outFrame.msgNum = mSeqno++;

    int32_t frameSize = (int32_t)buffer->datasize();
    mProtocol->framePayload(*buffer, outFrame, frameSize, *mEncoder);

    bool wasMigrating = mMigrating;
    if (!mMigrating)
    {
        // Hold the request until receives response in case migration happens.
        mSentRequestBySeqno[outFrame.msgNum] = buffer;
        mSendQueue.push_back(buffer);
        if (!flushSendQueue())
        {
            // flushSendQueue() will already output sufficient logging.
            return ERR_SYSTEM;
        }

        BLAZE_TRACE_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendRequest: frame(" << frameSize << ") " << component
            << "/" << command << ", " << outFrame.msgNum);
    }
    else
    {
        if (!mSendingPingToNewInstance)
        {
            // HOLD HERE
            BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendRequest: Hold frame(" << frameSize << ") " << component
                << "/" << command << ", " << outFrame.msgNum << " while usersession migrating");
            mSentRequestBySeqno[outFrame.msgNum] = buffer;
            mSendQueue.push_back(buffer);
        }
        else
        {
            // sending Ping rpc to new server
            BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendRequest: (migration) frame(" << frameSize << ") " << component
                << "/" << command << ", " << outFrame.msgNum);
            bool success = writeFrame(buffer);
            mSendingPingToNewInstance = false;
            if (!success)
                return ERR_SYSTEM;
        }
    }

    // Save info to response can be processed when it is received
    BlazeRpcError result;
    ProxyInfo info;
    info.semaphoreId = gFiberManager->getSemaphore("StressConnection::sendRequest");
    info.error = &result;
    info.response = responseTdf;
    info.errorResponse = errorTdf;
    mProxyInfo[outFrame.msgNum] = info;

    BlazeRpcError sleepError = gFiberManager->waitSemaphore(info.semaphoreId);
    if (sleepError != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, 
            "[" << getType() << "Connection].sendRequest: "
            "failed waiting for semaphore " << (wasMigrating ? "(migration)":"") << ", err: " <<
            ErrorHelp::getErrorDescription(sleepError));
        return sleepError;
    }

    mLastRpcTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();
    if (result == ERR_AUTHENTICATION_REQUIRED)
        resetSessionKey("RPC returned ERR_AUTHENTICATION_REQUIRED");
    return result;
}

void StressConnection::resetSessionKey(const char8_t* reason)
{
    if (mSessionKey[0] != '\0')
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].resetSessionKey: " << reason);
        *mSessionKey = '\0';
    }
}


void StressConnection::addBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler)
{
    mBaseAsyncHandlers[(component << 16) | type] = handler;
}

void StressConnection::removeBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler)
{
    mBaseAsyncHandlers.erase((component << 16) | type);
}

void StressConnection::addAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler)
{
    AsyncHandlerList& handlers = mAsyncHandlers[(component << 16) | type];
    for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
    {
        if (*itr == handler)
        {
            return;//already added
        }
    }
    handlers.push_back(handler);
}

void StressConnection::removeAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler)
{
    AsyncHandlerMap::iterator mapItr = mAsyncHandlers.find((component << 16) | type);
    if (mapItr != mAsyncHandlers.end())
    {
        AsyncHandlerList& handlers = mapItr->second;
        for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
        {
            if (*itr == handler)
            {
                handlers.erase(itr);
                break;
            }
        }
    }
}

/*** Private Methods *****************************************************************************/


void StressConnection::onRead(Channel& channel)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    EA_ASSERT(&channel == mChannel);
    while (mConnected)
    {
        if (mRecvBuf == nullptr)
            mRecvBuf = BLAZE_NEW RawBuffer(4096);

        size_t needed = 0;
        if (!mProtocol->receive(*mRecvBuf, needed))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, 
                "[" << getType() << "Connection].onRead: "
                "protocol receive failed, disconnecting (non-resumable error).");

            disconnect();
            return;
        }
        if (needed == 0)
        {
            processIncomingMessage();
            continue;
        }

        // Make sure the buffer is big enough to read everything we want
        if (mRecvBuf->acquire(needed) == nullptr)
        {
            BLAZE_WARN_LOG(Log::CONNECTION, 
                "[" << getType() << "Connection].onRead: "
                "recv buffer failed to acquire " << needed << " bytes, disconnecting (non-resumable error).");
            disconnect();
            return;
        }

        // Try and read data off the channel
        int32_t bytesRead = mChannel->read(*mRecvBuf, needed);

        if (bytesRead == 0)
        {
            // No more data currently available
            mLastReadTimestamp = TimeValue::getTimeOfDay().getMicroSeconds();
            return;
        }

        if (bytesRead < 0)
        {
            BLAZE_INFO_LOG(Log::CONNECTION, 
                "[" << getType() << "Connection].onRead: "
                "peer closed channel, disconnecting (resumable error).");

            if (mResolvingInstanceByRdir)
            {
                disconnect();
            }
            else
            {
                beginUserMigration("onRead");
            }
            return;
        }
    }
}

void StressConnection::processIncomingMessage()
{
    // Decode a frame
    RpcProtocol::Frame inFrame;
    mProtocol->process(*mRecvBuf, inFrame, *mDecoder);

    RawBuffer* payload = mRecvBuf;
    mRecvBuf = nullptr;

    if (inFrame.getSessionKey() != nullptr)
        blaze_strnzcpy(mSessionKey, inFrame.getSessionKey(), sizeof(mSessionKey));

    if (inFrame.messageType == RpcProtocol::NOTIFICATION)
    {
        AsyncHandlerMap::iterator async = mAsyncHandlers.find((inFrame.componentId << 16) | inFrame.commandId);
        if (((async != mAsyncHandlers.end()) && !async->second.empty()) || mBaseAsyncHandlers.find((inFrame.componentId << 16) | inFrame.commandId) != mBaseAsyncHandlers.end())
        {
            //dispatch to subscribers
            gSelector->scheduleDedicatedFiberCall<StressConnection, uint16_t, uint16_t, Blaze::RawBuffer*> (this, &StressConnection::dispatchOnAsyncFiber, inFrame.componentId, inFrame.commandId, payload, BlazeRpcComponentDb::getNotificationNameById(inFrame.componentId, inFrame.commandId));
        }
    }
    else if (inFrame.messageType == RpcProtocol::REPLY || inFrame.messageType == RpcProtocol::ERROR_REPLY || inFrame.messageType == RpcProtocol::PING_REPLY)
    {
        ProxyInfoBySeqno::iterator pi = mProxyInfo.find(inFrame.msgNum);
        if (pi != mProxyInfo.end())
        {
            ProxyInfo proxyInfo = pi->second;
            mProxyInfo.erase(pi);

            *proxyInfo.error = static_cast<BlazeRpcError>(inFrame.errorCode);

            if (inFrame.messageType == RpcProtocol::REPLY && proxyInfo.response != nullptr)
            {
                if (mMigrating)
                {
                    // we have received the PING_REPLY from the new server
                    mSendingPingToNewInstance = false;
                    mRetryCount = 0; // reset the counter
                    // Now we release all the holding RPCs
                    for (SentRequestBySeqno::const_iterator i = mSentRequestBySeqno.begin(), e = mSentRequestBySeqno.end(); i != e; ++i)
                    {
                        BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].processIncomingMessage:"
                            " Re-send held request (Seqno: " << i->first << ") after usersession migration complete.");
                        RawBufferPtr frame = i->second;
                        writeFrame(frame);
                    }
                    mSentRequestBySeqno.clear(); // release all holding requests

                    // The entire queue has been drained so clear any write ops on the selector and migration complete
                    mChannel->removeInterestOp(Channel::OP_WRITE);
                    mResolvingInstanceByErrMoved = false;
                    mMigrating = false;

                    BLAZE_INFO_LOG(Log::CONNECTION, "[" << getType() << "Connection].processIncomingMessage: Recieved PING_REPLY from "
                        << mAddress);
                }

                BlazeRpcError err = Blaze::ERR_OK;
                err = mDecoder->decode(*payload, *proxyInfo.response);
                if (err != Blaze::ERR_OK)
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, 
                        "[" << getType() << "Connection].processIncomingMessage: "
                        "failed to decode response for component: " << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId) 
                        << ", rpc: " << BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId, nullptr) 
                        << ", err: " << ErrorHelp::getErrorName(err));
                }
            }
            else if (inFrame.messageType == RpcProtocol::ERROR_REPLY && proxyInfo.errorResponse != nullptr)
            {
                BlazeRpcError err = Blaze::ERR_OK;
                err = mDecoder->decode(*payload, *proxyInfo.errorResponse);
                if (err != Blaze::ERR_OK)
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, 
                        "[" << getType() << "Connection].processIncomingMessage: "
                        "failed to decode error response for component: " << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId) 
                        << ", rpc: " << BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId, nullptr) 
                        << ", err: %s" << ErrorHelp::getErrorName(err));
                }
            }

            gFiberManager->signalSemaphore(proxyInfo.semaphoreId);
        }

        // migrating to the instance provided in the Fire2Metadata
        if (inFrame.movedToAddr.getIp(InetAddress::Order::HOST) != 0)
        {
            beginUserMigration("processIncomingMessage", inFrame.movedToAddr);
        }

        // Ack the request if migration not happens
        if (inFrame.errorCode != ERR_MOVED)
        {
            SentRequestBySeqno::iterator si = mSentRequestBySeqno.find(inFrame.msgNum);
            if (si != mSentRequestBySeqno.end())
            {
                mSentRequestBySeqno.erase(si);
            }
        }
    }
    else if (inFrame.messageType == RpcProtocol::PING)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[" << getType() << "Connection].processIncomingMessage:"
            " Received PING with msgId(" << inFrame.msgNum << "), userIndex(" << inFrame.userIndex << ")");
        
        Fiber::CreateParams params;
        params.namedContext = "StressConnection::sendPingReply";
        gFiberManager->scheduleCall<StressConnection, BlazeRpcError, uint16_t, uint16_t, uint32_t>(this, &StressConnection::sendPingReply, inFrame.componentId, inFrame.commandId, inFrame.msgNum, nullptr, params);
    }

    // we must delete the payload here, TDFs no longer take ownership of the payload
    delete payload;
}

void StressConnection::beginUserMigration(const char8_t* caller, InetAddress addr)
{
    if (mIsRdirConn)
    {
        disconnect();
        return;
    }

    BLAZE_INFO_LOG(Log::CONNECTION,
        "[" << getType() << "Connection].beginUserMigration: Triggering user migration from " << caller);

    mMigrating = true;
    disconnect();

    if (addr.isResolved())
    {
        mAddress = addr;
        mResolvingInstanceByErrMoved = true;

        char8_t addressBuf[256];
        BLAZE_INFO_LOG(Log::CONNECTION,
            "[" << getType() << "Connection].beginUserMigration: Migrating to new server " << mAddress.toString(addressBuf, sizeof(addressBuf)) << ".");
    }
    mConnectionHandler->onBeginUserSessionMigration();

    gFiberManager->scheduleCall<StressConnection, bool>(this, &StressConnection::connect, false);
}

void StressConnection::cancelUserMigration()
{
    mMigrating = false;
    disconnect();
}

void StressConnection::dispatchOnAsyncFiber(uint16_t component, uint16_t command, RawBuffer* payload)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    AsyncHandlerBaseMap::iterator basync = mBaseAsyncHandlers.find((component << 16) | command);
    if (basync != mBaseAsyncHandlers.end())
    {
        basync->second->onAsyncBase(component, command, payload);
        return;
    }

    AsyncHandlerMap::iterator async = mAsyncHandlers.find((component << 16) | command);
    if (async != mAsyncHandlers.end())
    {
        payload->mark();
        AsyncHandlerList& handlers = async->second;
        for (AsyncHandlerList::iterator itr = handlers.begin(), end = handlers.end(); itr != end; ++itr)
        {
            (*itr)->onAsync(component, command, payload);
            // we need to reset to mark, as each subscriber's onAsync may advance payload's data ptr to the tail
            payload->resetToMark();
        }
    }
}

void StressConnection::onWrite(Channel& channel)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    if (!mMigrating)
        flushSendQueue();
}

void StressConnection::onError(Channel& channel)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    BLAZE_WARN_LOG(Log::CONNECTION,
        "[" << getType() << "Connection].onError: "
        "channel signaled error, disconnecting (resumable error).");
    disconnect();

}

void StressConnection::onClose(Channel& channel)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    BLAZE_INFO_LOG(Log::CONNECTION,
        "[" << getType() << "Connection].onClose: "
        "channel signaled close, disconnecting (resumable error).");
    disconnect();

}

bool StressConnection::writeFrame(RawBufferPtr frame)
{
    int32_t res = mChannel->write(*frame);
    if (res < 0)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, 
            "[" << getType() << "Connection].writeOutputQueue: "
            "failed writing output queue, socket error: " << res << ", disconnecting (resumable error).");

        disconnect();
        return false;
    }

    if (frame->datasize() != 0)
    {
        // There's still more to write but the socket is full so enable write notifications
        mChannel->addInterestOp(Channel::OP_WRITE);
        return true;
    }

    return true;
}

bool StressConnection::flushSendQueue()
{
    while (!mSendQueue.empty() && !mMigrating)
    {
        RawBufferPtr frame = mSendQueue.front();
        mSendQueue.pop_front();
        writeFrame(frame);
    }

    if (mConnected)
    {
        // The entire queue has been drained so clear any write ops on the selector
        mChannel->removeInterestOp(Channel::OP_WRITE);
    }

    return true;
}

BlazeRpcError StressConnection::sendPingReply(uint16_t component, uint16_t command, uint32_t msgId)
{
    VERIFY_WORKER_FIBER();

    if (!mConnected)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendPingReply: send failed, not connected.");
        return ERR_SYSTEM;
    }

    // Frame the request
    RawBuffer* frame = BLAZE_NEW RawBuffer(2048);
    Blaze::RpcProtocol::Frame outFrame;
    outFrame.componentId = component;
    outFrame.commandId = command;
    outFrame.messageType = RpcProtocol::PING_REPLY;
    
    mProtocol->setupBuffer(*frame);

    // no body to encode

    outFrame.msgNum = msgId;

    int32_t frameSize = (int32_t)frame->datasize();
    mProtocol->framePayload(*frame, outFrame, frameSize, *mEncoder);

    writeFrame(frame);

    BLAZE_TRACE_LOG(Log::CONNECTION, "[" << getType() << "Connection].sendPingReply: frame(" << frameSize << ") "
        << component << "/" << command << ", " << outFrame.msgNum);

    return ERR_OK;
}

bool StressConnection::shouldReconnect()
{
    if (!mMigrating)
        return false;

    if (mRetryCount < MAX_RETRY_ATTEMPTS)
    {
        mRetryCount++;
        BlazeRpcError error = gSelector->sleep(2 * 1000 * 1000);
        if (error != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, 
                "[" << getType() << "Connection].connect: Sleep expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
        }

        BLAZE_INFO_LOG(Log::CONNECTION, 
            "[" << getType() << "Connection].connect: Issue a retry attempt, current retry count(" << mRetryCount << ")");
        return true;
    }
    else
    {
        mMigrating = false;
        BLAZE_ERR_LOG(Log::CONNECTION,
            "[" << getType() << "Connection].connect: Maximum number of reconnection attempts (" << MAX_RETRY_ATTEMPTS << ") has been reached.");
        return false;
    }
}

} // namespace Stress
} // namespace Blaze

