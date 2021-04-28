/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Connection

    This class defines a connection object which manages state and processing of requests for a
    given client.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/component/component.h"
#include "framework/connection/connection.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/channel.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/selector.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/ratelimiter.h"
#include "framework/component/message.h"
#include "framework/protocol/protocol.h"
#include "framework/protocol/shared/fireframe.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"
#include "framework/util/locales.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/protocol/shared/tdfencoder.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "blazerpcerrors.h"
#include "framework/component/blazerpc.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
struct LargeBlockingFiberInitializer : Fiber::CreateParams
{
    LargeBlockingFiberInitializer()
    {
        blocking = true;
        blocksCaller = true;
        stackSize = Fiber::STACK_LARGE;
    };
};
static const LargeBlockingFiberInitializer LARGE_BLOCKING_FIBER;

/*** Public Methods ******************************************************************************/
Connection::Connection(ConnectionOwner& owner, ConnectionId ident, SocketChannel& channel, Blaze::Endpoint& endpoint)
    :mCloseReason(DISCONNECTTYPE_UNKNOWN),
    mSocketError(0),
    mReadWriteLoopFiberGroupId(Fiber::allocateFiberGroupId()),
    mProcessMessageFiberGroupId(Fiber::allocateFiberGroupId()),
    mShuttingDown(false),
    mShutdownStartTime(0),
    mQueuedOutputBytes(0),
    mQueuedAsyncBytes(0),
    mSendBytes(0),
    mSendCount(0),
    mRecvBytes(0),
    mRecvCount(0),
    mInactivitySuspensionTimeout(0),
    mInactivitySuspensionTimeoutEndTime(TimeValue::getTimeOfDay()),    
    mOwner(owner),
    mIdent(ident),
    mBlockingSocket(channel),
    mDefaultFrameSize(1024),
    mInputBuffer(nullptr),
    mBytesAvailable(0),
    mClosed(false),
    mQueuedForDeletion(false),
    mEndpoint(endpoint),
    mEncoder(owner.getOrCreateEncoder(endpoint.getEncoderType())),
    mDecoder(owner.getOrCreateDecoder(endpoint.getDecoderType())),
    mProtocol(ProtocolFactory::create(endpoint.getProtocolType(), endpoint.getMaxFrameSize())),
    mSignalUnsquelchedTimerId(INVALID_TIMER_ID),
    mShutdownAckTimer(INVALID_TIMER_ID),
    mClosingFiberId(Fiber::INVALID_FIBER_ID)
{
    mPeerName[0] = '\0';
    mEndpoint.addConnection();
}

Connection::~Connection()
{
    // This connection should already be queued at this point.
    mQueuedForDeletion = true;
    close();
    delete mProtocol;
    mEndpoint.removeConnection();
}

bool Connection::connect()
{
    Fiber::CreateParams fiberParams;
    fiberParams.blocking = true;
    fiberParams.blocksCaller = true;  //The calling fiber will block until our connect is done.
    fiberParams.stackSize = Fiber::STACK_MICRO;    
    fiberParams.namedContext = "Connection::connectInternal";
    fiberParams.groupId = mReadWriteLoopFiberGroupId;
    gFiberManager->scheduleCall(this, &Connection::connectInternal, fiberParams);
    
    return isConnected();        
}

bool Connection::accept()
{
    Fiber::CreateParams fiberParams;
    fiberParams.blocking = true;
    fiberParams.blocksCaller = true;  //The calling fiber will block until our accept is done.
    fiberParams.stackSize = Fiber::STACK_MICRO;    
    fiberParams.namedContext = "Connection::connectInternal";
    fiberParams.groupId = mReadWriteLoopFiberGroupId;

    //Underneath we do the same thing for an outbound vs. an inbound conn
    gFiberManager->scheduleCall(this, &Connection::connectInternal, fiberParams); 

    return isConnected();
}

void Connection::connectInternal()
{
    if (EA_UNLIKELY(mQueuedForDeletion))
    {
        EA_ASSERT_MSG(false, "Conn already queued for deletion!");
        return;
    }

    clearCloseReason();

    if (isInboundConnection())
    {
        if (!isConnected())
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].connectInternal: InboundRpcConnection should already be connected at this point.");
            return;
        }
    }

    //Connect the channel.
    if (mBlockingSocket.connect(isKeepAliveRequired()) != 0)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].connectInternal: Failed to connect.");
        setCloseReason(DISCONNECTTYPE_CHANNEL_FAILED_TO_CONNECT, 0);
        close();
        return;
    }

    mClosed = false;

    BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].connectInternal: Connected.");

    Fiber::CreateParams fiberParams;
    fiberParams.blocking = true;
    fiberParams.stackSize = Fiber::STACK_MICRO;
    fiberParams.groupId = mReadWriteLoopFiberGroupId;
    fiberParams.tagged = true;

    if (!(mWriteFiberHandle.isValid() && Fiber::isFiberIdInUse(mWriteFiberHandle.fiberId)))
    {
        fiberParams.namedContext = "Connection::writeLoop";
        gFiberManager->scheduleCall(this, &Connection::writeLoop, fiberParams, &mWriteFiberHandle);
    }

    if (!(mReadFiberHandle.isValid() && Fiber::isFiberIdInUse(mReadFiberHandle.fiberId)))
    {
        fiberParams.namedContext = "Connection::readLoop";
        gFiberManager->scheduleCall(this, &Connection::readLoop, fiberParams, &mReadFiberHandle);
    }
}

void Connection::scheduleClose()
{
    if (mClosingFiberId == Fiber::INVALID_FIBER_ID)
    {
        gSelector->scheduleFiberCall(this, &Connection::close, "Connection::close");
    }
}

void Connection::close()
{
    if (!mClosed)
    {
        mClosingFiberId = Fiber::getCurrentFiberId();

        if (mSignalUnsquelchedTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mSignalUnsquelchedTimerId);
            mSignalUnsquelchedTimerId = INVALID_TIMER_ID;
        }

        if (mShutdownAckTimer != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mShutdownAckTimer);
            mShutdownAckTimer = INVALID_TIMER_ID;

            TimeValue timeDelta = TimeValue::getTimeOfDay() - mShutdownStartTime;
            
            BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".close: connection was closed successfully in " << timeDelta.getSec() << " seconds.");
        }

        mClosed = true;
        mBlockingSocket.close();

        const bool isProcessMessageFiber = mProcessMessageFiberGroupId == Fiber::getCurrentFiberGroupId();
        if (!isProcessMessageFiber)
        {
            if (mShuttingDown)
            {
                Fiber::cancelGroup(mProcessMessageFiberGroupId, ERR_CANCELED);
            }
            Fiber::join(mProcessMessageFiberGroupId);
        }

        Fiber::cancelGroup(mReadWriteLoopFiberGroupId, ERR_DISCONNECTED);

        mDispatcher.dispatch<Connection&>(&ConnectionListener::onConnectionDisconnected, *this);

        if (!mQueuedForDeletion)
        {
            mQueuedForDeletion = true;
            getOwner().queueForRemoval(*this);
        }

        mClosingFiberId = Fiber::INVALID_FIBER_ID;
    }
}

void Connection::readLoop()
{
    const bool isInbound = isInboundConnection();
    while (EA_LIKELY(isConnected()))
    {
        if (mInputBuffer == nullptr)
        {
            mInputBuffer = getBuffer();
            mBytesAvailable = 0;
        }

        //Temporarily bump a reference to the input buffer.  If we are blocked and the input buffer is cleared for this class, we want to keep a reference to it until 
        //we've eliminated the reference off our stack.
        RawBufferPtr inputBuf = mInputBuffer;
        int32_t bytesRead = mBlockingSocket.recv(*mInputBuffer.get(), 0);
        int32_t sockErr = mBlockingSocket.getLastError();

        inputBuf = nullptr;

        if (bytesRead > 0)
        {
            // drop inbound reads when graceful closure is initialized
            if (isInbound && mBlockingSocket.isShuttingDown())
            {   
                // We can't close the connection yet since there can be pending writes.  Also, have to keep reading until a FIN is received in the SocketChannel (ack closure of the remote). 
                // This is how we tell that the remote is done and so we can close the socket/connection.                
                BLAZE_SPAM_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: inbound read is being ignored since this connection is being gracefully closed. LastSockErr: " << sockErr << ". Bytes read: " << bytesRead << ".");

                // Make sure we empty the buffer so that we never end up looping back around and passing a full buffer in to blocking socket recv call - 
                // a full buffer will result in the underlying socket channel short circuiting and returning 0 bytes read which would be treated as 
                // 'would block' and mean that we would fail to detect the socket being closed.  This is quite likely to occur if incoming data has crossed
                // paths with our outgoing TCP FIN
                mInputBuffer->reset();

                continue;
            }
            else
            {
                mRecvBytes += bytesRead;
                ++mRecvCount;
                getEndpoint().incrementRecvMetrics(bytesRead);

                mBytesAvailable += bytesRead;
                processInputBufferWithLargeFiber();

                // An attempt to write to this conn while we were blocked in processInputBuffer may have closed the conn,
                // if we get back here and discover the conn is closed, we're done
                if (isClosed())
                    return;
            }
        }
        else
        {
            // Channel was closed
            if (!isClosed())
            {
                if (sockErr == 0)
                {
                    BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: Connection closed, no error.");
                    setCloseReason(DISCONNECTTYPE_CHANNEL_PEER_CLOSED_CONNECTION, 0);
                }
                else
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: Connection closed - error reading: "
                        << sockErr);
                    setCloseReason(DISCONNECTTYPE_CHANNEL_READ_ERROR, sockErr);
                }
                
                closeWithLargeFiber();
                return;
            }           
        }

        //In the case of a very large number of packets we might starve the other conns.  Lets yield
        //if we're doing too much.        
        BlazeRpcError yieldErr = Fiber::yieldOnAllottedTime(gSelector->getMaxConnectionProcessingTime());
        if (yieldErr != ERR_OK)
        {
            if (!isClosed())
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: Connection closed - Fiber::yieldOnAllottedTime error: "
                    << ErrorHelp::getErrorName(yieldErr));
                setCloseReason(DISCONNECTTYPE_ERROR_NORMAL, 0);
                closeWithLargeFiber();
            }
            return;
        }

        if (mEndpoint.isSquelchingEnabled() && getTotalQueuedBytes() >= mEndpoint.getOutputQueueConfig().getHighWatermark())
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: Queued output data reached high watermark (" 
                << getTotalQueuedBytes() << "); squelching input until queue drains.");
            if (!isClosed())
            {
                mEndpoint.incrementHighWatermarks();
            }
            mEndpoint.incrementTotalHighWatermarks();
            Fiber::getAndWait(mSquelchEvent, "Connection::readloop squelch wait");
        }
    }

    if (!isClosed())
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].readLoop: Read loop ended with connection still open, closing it now.");
        closeWithLargeFiber();
        return;
    }
}

// called by readLoop(), to ensure stack is large enough, to prevent crashes
void Connection::closeWithLargeFiber()
{
    gSelector->scheduleFiberCall(this, &Connection::close, "Connection::close", LARGE_BLOCKING_FIBER);
}

// called by readLoop(), to ensure stack is large enough, to prevent crashes
void Connection::gracefulCloseWithLargeFiber()
{
    gSelector->scheduleFiberCall(this, &Connection::gracefulClose, "Connection::gracefulClose", LARGE_BLOCKING_FIBER);
}

// called by readLoop(), to ensure stack is large enough, to prevent crashes
void Connection::processInputBufferWithLargeFiber()
{
    gSelector->scheduleFiberCall(this, &Connection::processInputBuffer, "Connection::processInputBuffer", LARGE_BLOCKING_FIBER);
}

const char8_t* Connection::toString(char8_t* buf, uint32_t len) const
{
    int32_t bytesWritten = blaze_snzprintf(buf, len, "ep=%s/id=%d/chan=%d/peer=",
            mEndpoint.getName(), mIdent, mBlockingSocket.getIdent());

    if (mPeerName[0] != '\0')
        bytesWritten += blaze_snzprintf(buf + bytesWritten, len - bytesWritten, "%s|", mPeerName);

    getPeerAddress().toString(buf + bytesWritten, len - bytesWritten, InetAddress::IP_PORT);

    return buf;
}

/*
Server-initiated graceful socket closure (for inbound):
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


***************************************
             TCP level  
***************************************

Inbound connections:
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                       Server                                       |                                                   Client
--------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------
1.  When the RPC command is processed and all bytes are sent,       |           
    call shutdown(WRITE) to generate its FIN to the client.         |
                                                                    |
    Server keeps read side opened watching for return value         |
    of 0 (step 4). Note that the server will not accept any         |
    additional request from the read.                               |    
                                                                    |   2. Receives all bytes and lastly the FIN causing recv() returns 0, indicating that all bytes have been received
                                                                    |      and that graceful shutdown is in-progress
                                                                    |   3. To acknowledge the graceful shutdown, the client can either
                                                                    |      - 3a: close the socket. Graceful shutdown is then complete from client's point of view. 
                                                                    |      - 3b: call shutdown(SHUT_HOW_WRITE) for its turn of graceful shutdown "handshake". 
                                                                    |            Client keeps read side opened watching for return value of 0 (step 5)
                                                                    |      Both will equally send its FIN to the server acknowledging the graceful shutdown and that the client has no more 
                                                                    |      bytes to send either
4.  Recv() returns 0 (from client's FIN), server closes the socket. |      
    Graceful shutdown is then complete from server's point of view. |
                                                                    |   5. For 3b, recv returns 0 (from the FIN due to server's socket closure), client closes the socket.
                                                                    |      Graceful shutdown is complete from client's point of view.
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

***************************************
    Blaze Server Internal Operation 
***************************************

For Inbound:
- At Connectionmanager shutdown(), it's where the graceful shutdown of all inbound connections begin.  
  Here are the steps:
  1. Mark all connections mShuttingDown=true, by calling gracefulClose()
  2. Attempt to proceed with graceful shutdown of all connections, if the connecton is ready to be gracefully closed*.

  Meanwhile:
  - within the writeLoop(), just before it goes to sleep after a successful send(), if the connection is shuttingDown, provided graceful shutdown condition is met*, gracefulClose() will be called
  - within the readLoop(), when the inbound connection is shutting down, all new reads will be dropped.  It will also watch for recv to return 0, indicating that the peer closure ack is received, in which case will call close().

  *Condition for graceful closure of inbound: 
  - total queued bytes = 0
  - outstanding commands = 0
  - the only exception is if it's the blocking socket is already closed (eg. a resumable connection that's disconnected), in which case will go ahead and force close the connection (no point to wait during shutdown).

More details can be found in TDD: https://docs.developer.ea.com/display/TEAMS/Minimal+impact+shutdown+mechanism+in+framework

*/
void Connection::gracefulClose()
{
    mShuttingDown = true;
    const bool isInbound = isInboundConnection();

    // For resumable connections the underlying blocking socket will be already closed, simply complete by forcing to close the connection.
    if (!mBlockingSocket.isConnected())
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: resumable connection is already cleaning up so just forcing to close the connection.");
        close();
        return;
    }
    
    if (getTotalQueuedBytes() > 0)
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: the underlying socket is not ready for graceful closure since there are still " << getTotalQueuedBytes() << " queued output bytes of data.");
        return;
    }

    // Inbound connection has to finish processing all outstanding commands before graceful closure happens
    if (isInbound && hasOutstandingCommands())
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: the underlying socket is not ready for graceful closure since it still has outstanding commands.");
        return;
    }    

    if (mBlockingSocket.isShuttingDown())
    {
        int32_t lastSockErr = mBlockingSocket.getLastError();
        BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: ignoring duplicate gracefulClose() on the socket. Last sockErr:" << strerror(lastSockErr) << "(" << lastSockErr);
        return;
    }
    
    BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: issuing shutdown(WRITE) on the socket.");

    mShutdownStartTime = TimeValue::getTimeOfDay();
    int32_t result = mBlockingSocket.shutdown(SHUT_HOW_WRITE);
    int32_t lastError = mBlockingSocket.getLastError();

    if (result == 0)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: socket shutdown(WRITE) returned 0. This Connection will be removed when we received a FIN (caused by a shutdown/close from the remote).");
    }  
    else
    {                        
        BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: socket shutdown(WRITE) returned with unexpected last sockError: " << strerror(lastError) << "(" << lastError << "). Unexpected socket errors encountered.  Closing the connection.");
        close();
        return;
    }

    // for inbound, schedule to expire and close connections if are not closed within the acknowledgement timeout period
    if (isInbound && mShutdownAckTimer == INVALID_TIMER_ID)
    {
        const TimeValue& ackTimeout = gController->getInboundGracefulClosureAckTimeout();
        BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << ".gracefulClose: schedule when expired to close connection in " << ackTimeout.getSec() << " seconds.");
        mShutdownAckTimer = gSelector->scheduleTimerCall(mShutdownStartTime + ackTimeout, this, &Connection::gracefulClosureAckExpired, "Connection::gracefulCloseAckExpired");
    }

    return;
}

void Connection::gracefulClosureAckExpired()
{
    mShutdownAckTimer = INVALID_TIMER_ID;
    BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].gracefulCloseAckExpired: graceful close acknowledgement expired.  Forcing to close the connection.");
    close();
}

void Connection::writeLoop()
{
    while (EA_LIKELY(isConnected()))
    {
        if (mOutputQueue.empty())
        {
            // if the connection is being shutdown, perform graceful closure and if successful, bail out this method to avoid the explicit close() that
            // comes after this while loop, but that doesn't mean that we might not have already closed the conn.
            if (mShuttingDown)
            {
                gracefulCloseWithLargeFiber();
                if (!isClosed())
                {
                    BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: graceful closure is successfully initiated.");
                    return;
                }
            }
            
            BlazeRpcError queueWaitErr = Fiber::getAndWait(mQueueWaitEvent, "Connection::writeLoop wait on empty queue");
            if (queueWaitErr != ERR_OK)
            {
                // NOTE: The write loop is often woken with ERR_DISCONNECTED when the readLoop() has called Fiber::cancelGroup() on the
                // conn's fiber group id. Since the connection is closed already the write loop should exit silently because the 'closer'
                // of the conn (usually readLoop()) has already provided the reason for the close and logged any relevant messages earlier. 
                if (!isClosed())
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: Connection closed, Fiber::getAndWait error: "
                        << ErrorHelp::getErrorName(queueWaitErr) << "(" << SbFormats::HexLower(queueWaitErr) << ")");
                    setCloseReason(DISCONNECTTYPE_ERROR_NORMAL, 0);
                    closeWithLargeFiber();
                }
                return;
            }

            if (mOutputQueue.empty())
            {
                continue;
            }
        }

        size_t totalWritten = 0;
        OutputChunk& chunk = mOutputQueue.front();
        for (BufferArray::iterator itr = chunk.mData.begin(); itr != chunk.mData.end(); ++itr)
        {
            if (!*itr) //skip empty buffer
                continue;

            int32_t writeResult = mBlockingSocket.send(*(*itr).get());

            if (writeResult >= 0)
            {
                totalWritten += writeResult;
            }
            else
            {                
                if (!isClosed())
                {
                    // IF the connection was closed cleanly
                    int32_t sockErr = mBlockingSocket.getLastError();
                    
                    if (sockErr == 0)
                    {
                        BLAZE_TRACE_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: Connection closed on write, no error.");
                        setCloseReason(DISCONNECTTYPE_WRITE_NORMAL, 0);
                    }
                    else
                    {
                        BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: Error writing: " << sockErr);
                        setCloseReason(DISCONNECTTYPE_CONNECTION_OUTPUT_ERROR, sockErr);
                    }

                    // Just return here, as we don't want to spawn all the events that come with calling close() while in the writeLoop.
                    // The readLoop() will pick up the dead socket, and call close() as expected.
                    return;
                }
                break;
            }
        }

        //If we successfully sent, add this write to our totals and pop.
        if (!isClosed())
        {
            mQueuedOutputBytes -= totalWritten;
            mSendBytes += totalWritten;
            ++mSendCount;
            getEndpoint().incrementSendMetrics(totalWritten);
            mOutputQueue.pop_front();
        }
        
        //once we've written, see if we were squelched, and if we need to be unsquelched.
        if (mEndpoint.isSquelchingEnabled() && mSquelchEvent.isValid() &&  getTotalQueuedBytes() <= mEndpoint.getOutputQueueConfig().getLowWatermark())
        {
            //We've dropped below the water mark - let the reader know that it can continue if it's blocking on squelch
            BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: "
                "Queued output data drained below low watermark (" << mEndpoint.getOutputQueueConfig().getLowWatermark() << "); unsquelching input.");
            mEndpoint.decrementHighWatermarks();

            // Schedule a task to wake up the read loop (if it was waiting on the squelch event). We don't call Fiber::signal here because we want to wait
            // until control is returned to the selector before we begin processing any RPCs sent through this connection while it was squelched [GOS-27752]
            if (mSignalUnsquelchedTimerId == INVALID_TIMER_ID)
                mSignalUnsquelchedTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay(), this, &Connection::signalUnsquelched, mSquelchEvent, "Connection::signalUnsquelched");
        }
    }
    
    if (!isClosed())
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].writeLoop: Write loop ended with connection still open, closing it now.");
        closeWithLargeFiber();
        return;
    }
}

bool Connection::isAboveLowWatermark() const
{
    return getTotalQueuedBytes() > mEndpoint.getOutputQueueConfig().getLowWatermark();
}

void Connection::signalUnsquelched(Fiber::EventHandle squelchEvent)
{
    mSignalUnsquelchedTimerId = INVALID_TIMER_ID;
    Fiber::signal(squelchEvent, ERR_OK);
}

void Connection::setupEncoder(Encoder::Type encoderType)
{
    if (encoderType != Encoder::INVALID)
    {
        if (EA_UNLIKELY(encoderType != mEncoder->getType()))
        {
            TdfEncoder* encoder = mOwner.getOrCreateEncoder(encoderType);
            if (encoder != nullptr)
            {
                mEncoder = encoder;
            }
        }
    }
}

void Connection::setPeerName(const char8_t* name)
{
    blaze_strnzcpy(mPeerName, name, sizeof(mPeerName));
}

bool Connection::encodeTdf(RawBufferPtr buffer, const EA::TDF::Tdf& tdf, const RpcProtocol::Frame* frame, Encoder::Type encoderType)
{
    if (encoderType != Encoder::INVALID)
    {
        if(EA_UNLIKELY(encoderType != mEncoder->getType()))
        {
            TdfEncoder* encoder = mOwner.getOrCreateEncoder(encoderType);
            if (encoder != nullptr)
            {
                mEncoder = encoder;
            }
            else
            {
                return false;
            }
        }
    }

    return mEncoder->encode(*buffer.get(), tdf, frame);
}

const char8_t* Connection::getDecoderErrorMessage()
{
    if (mDecoder == nullptr)
        return "(Missing Decoder)";
    
    return mDecoder->getErrorMessage();
}

BlazeRpcError Connection::decodeTdf(RawBufferPtr buffer, EA::TDF::Tdf& tdf, Decoder::Type decoderType)
{
    if(decoderType != Decoder::INVALID)
    {
        if(EA_UNLIKELY(decoderType != mDecoder->getType()))
        {
            //Create a new decoder - we must have switched
            TdfDecoder* decoder = mOwner.getOrCreateDecoder(decoderType);
            if (decoder != nullptr)
            {
                mDecoder = decoder;
            }
            else
            {
                return ERR_SYSTEM;
            }            
        }
    }

    return mDecoder->decode(*buffer.get(), tdf);
}

void Connection::processInputBuffer()
{
    // Keep processing frames until there isn't anything left in the buffer
    while (isConnected() && mInputBuffer != nullptr && mInputBuffer->datasize() > 0)
    {
        // Process raw data into a request
        size_t needed = 0;

        if (!getProtocolBase().receive(*mInputBuffer.get(), needed))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].processInputBuffer: Incoming frame "
                "rejected; closing connection.");
            setCloseReason(DISCONNECTTYPE_CONNECTION_FRAME_REJECTED, 0);
            close();
            return;
        }

        if (needed > 0)
        {
            if (mInputBuffer->acquire(needed) == nullptr)
            {
                // buffer isn't big enough to read the entire frame so close connection
                BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].processInputBuffer: Packet too large; closing "
                    "connection");
                setCloseReason(DISCONNECTTYPE_CONNECTION_PACKET_TOO_LARGE, 0);
                close();
                return;
            }

            // Don't have the whole frame yet so bail out until we read more data
            return;
        }

        // Setup for the next incoming request
        RawBufferPtr msg = mInputBuffer;
        size_t frameSize = mInputBuffer->size() - getProtocolBase().getFrameOverhead();
        if (frameSize < mBytesAvailable)
        {
            // Take any remaining bytes at the end of the next buffer
            size_t remainingBytes = mBytesAvailable - frameSize;
            RawBufferPtr raw = getBuffer(remainingBytes);
            memcpy(raw->tail(), mInputBuffer->tail(), remainingBytes);
            mBytesAvailable -= frameSize;
            raw->put(mBytesAvailable);
            mInputBuffer = raw;
        }
        else
        {
            mInputBuffer = nullptr;
            mBytesAvailable = 0;
        }

        //
        // A complete frame has been received
        //       
        processMessage(msg);
    }
}


/*************************************************************************************************/
/*!
     \brief queueOutputData
 
      Queues a RawBuffer for being written to the channel

      \param[in] buffer - the buffer to write to the channel

*/
/*************************************************************************************************/
void Connection::queueOutputData(const RawBufferPtr& buf1, const RawBufferPtr& buf2, bool queueDirectly)
{
    size_t dataSize = (buf1 ? buf1->datasize() : 0) + (buf2 ? buf2->datasize() : 0);
    if (EA_UNLIKELY(dataSize == 0))
        return;

    const bool wasEmptyQueue = (getTotalQueuedBytes() == 0);

    OutputChunk* chunk;
    if (queueDirectly) //add it directly to the output queue.
    {
        chunk = &mOutputQueue.push_back();
        mQueuedOutputBytes += dataSize;
        getEndpoint().incrementTotalQueuedOutputBytes(dataSize);
    }
    else //add it to the async queue for delayed writing to the output channel
    {
        chunk = &mAsyncQueue.push_back();
        mQueuedAsyncBytes += dataSize;
        getEndpoint().incrementTotalQueuedAsyncBytes(dataSize);
    }

    (*chunk).mData[0] = buf1;
    (*chunk).mData[1] = buf2;

    //When we queue data, we need to check on our maxes
    if (mEndpoint.isSquelchingEnabled())
    {
        uint32_t queuedData = getTotalQueuedBytes();

        const EndpointConfig::OutputQueueConfig& qConfig = mEndpoint.getOutputQueueConfig();
        if (queuedData >= qConfig.getHighWatermark())
        {
            // check if we haven't been able to write anything for a while and grace period is configured
            if (!wasEmptyQueue && qConfig.getGracePeriod() > 0 && (TimeValue::getTimeOfDay() - mBlockingSocket.getLastWriteTime()) > qConfig.getGracePeriod())
            {
                // We've exceeded the max queued data so close the connection down
                BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].queueOutputData: Amount of queued data (" << queuedData << ") "
                    "above HighWatermark (" << qConfig.getHighWatermark() << ") for longer than the GracePeriod (" << qConfig.getGracePeriod().getMillis() << "ms); closing connection.");

                if (!isClosed())
                {
                    if (isSquelching())
                    {
                        mEndpoint.decrementHighWatermarks();
                    }
                    mEndpoint.incrementTotalExceededDataCloses();
                }
                setCloseReason(DISCONNECTTYPE_QUEUE_LIMIT_AND_GRACE_PERIOD_EXCEEDED, 0);
                scheduleClose();
            }
            else if (queuedData >= qConfig.getMax())
            {
                // We've exceeded the max queued data so close the connection down
                BLAZE_WARN_LOG(Log::CONNECTION, "[Connection:" << ConnectionSbFormat(this) << "].queueOutputData: Queued output data (" 
                    << queuedData << ") reached max configured (" << qConfig.getMax() << "); closing connection.");

                if (!isClosed())
                {
                    if (isSquelching())
                    {
                        mEndpoint.decrementHighWatermarks();
                    }
                    mEndpoint.incrementTotalExceededDataCloses();
                }
                setCloseReason(DISCONNECTTYPE_QUEUE_LIMIT_AND_DATA_SIZE_EXCEEDED, 0);
                scheduleClose();
                return;
            }
        }
    }

    if (!mOutputQueue.empty())
    {
        //Finally, wake up the writeLoop if its waiting on something going into the queue (and we've put something there).
        Fiber::signal(mQueueWaitEvent, ERR_OK);
    }
}

/*************************************************************************************************/
/*!
     \brief spliceOutputQueues
 
      Appends all queued up async notifications to the end of the output queue
*/
/*************************************************************************************************/
void Connection::spliceOutputQueues()
{
    if (!mAsyncQueue.empty())
    {
        mOutputQueue.splice(mOutputQueue.end(), mAsyncQueue);
        mQueuedOutputBytes += mQueuedAsyncBytes;
        getEndpoint().incrementTotalQueuedOutputBytes(mQueuedAsyncBytes);
        mQueuedAsyncBytes = 0;

        Fiber::signal(mQueueWaitEvent, ERR_OK);
    }
}

RawBufferPtr Connection::getBuffer(size_t initialSize)
{
    uint32_t overhead = getProtocolBase().getFrameOverhead();
    RawBuffer* buf = BLAZE_NEW RawBuffer((initialSize + overhead < mDefaultFrameSize) ? mDefaultFrameSize : initialSize + overhead);
    getProtocolBase().setupBuffer(*buf);

    return RawBufferPtr(buf);
}

void Connection::getStatusInfo(ConnectionStatus& status) const
{
    status.setIdent(mIdent);

    uint32_t addr = mBlockingSocket.getChannel().getPeerAddress().getIp();
    char buf[64];
    blaze_snzprintf(buf, sizeof(buf), "%d.%d.%d.%d", NIPQUAD(addr));
    status.setPeerAddress(buf);
    status.setPeerPort(mBlockingSocket.getChannel().getPeerAddress().getPort(InetAddress::HOST));

    addr = mBlockingSocket.getChannel().getAddress().getIp();
    blaze_snzprintf(buf, sizeof(buf), "%d.%d.%d.%d", NIPQUAD(addr));
    status.setAddress(buf);
    status.setPort(mBlockingSocket.getChannel().getAddress().getPort(InetAddress::HOST));

    status.setProtocol(getProtocolBase().getName());
    status.setEncoder(mEncoder->getName());
    status.setDecoder(mDecoder->getName());
    status.setSendBytes(mSendBytes);
    status.setSendCount(mSendCount);
    status.setRecvBytes(mRecvBytes);
    status.setRecvCount(mRecvCount);
    status.setOutputQueueSize((uint32_t)mOutputQueue.size());
    status.setOutputQueueBytes((uint32_t)mQueuedOutputBytes);
}

void Connection::setCloseReason(UserSessionDisconnectType closeReason, int32_t socketError)
{
    if (mCloseReason == DISCONNECTTYPE_UNKNOWN)
    {
        mCloseReason = closeReason;
        mSocketError = socketError;
    }
}

void Connection::clearCloseReason()
{
    mCloseReason = DISCONNECTTYPE_UNKNOWN;
    mSocketError = 0;
}

bool Connection::isResumableCloseReason(UserSessionDisconnectType closeReason) const
{
    return mEndpoint.getAllowResumeConnection() && getOwner().isActive() &&
        closeReason != DISCONNECTTYPE_UNKNOWN &&
        closeReason != DISCONNECTTYPE_CLIENT_LOGOUT &&
        closeReason != DISCONNECTTYPE_CLIENT_DISCONNECTED &&
        closeReason != DISCONNECTTYPE_CHANNEL_PEER_CLOSED_CONNECTION &&
        closeReason != DISCONNECTTYPE_QUEUE_LIMIT_AND_GRACE_PERIOD_EXCEEDED &&
        closeReason != DISCONNECTTYPE_QUEUE_LIMIT_AND_DATA_SIZE_EXCEEDED &&
        closeReason != DISCONNECTTYPE_DUPLICATE_LOGIN && 
        closeReason != DISCONNECTTYPE_CONNECTION_FRAME_REJECTED &&
        closeReason != DISCONNECTTYPE_CONNECTION_PACKET_TOO_LARGE &&
        closeReason != DISCONNECTTYPE_FORCED_LOGOUT;
}

void Connection::clearInputState()
{
    mInputBuffer = nullptr;
    mBytesAvailable = 0;
    delete mProtocol;
    mProtocol = ProtocolFactory::create(mEndpoint.getProtocolType(), mEndpoint.getMaxFrameSize());
}

/*** Protected Methods ***************************************************************************/

/*!
     \brief isInactivityTimeoutSuspended
 
      returns 
      true if the inactivitySuspensionTimeout has been exceeded/suspended (this means that the user has suspended the Ping)
      false otherwise
*/
/*************************************************************************************************/
bool Connection::isInactivityTimeoutSuspended()
{
    if( mInactivitySuspensionTimeout != 0)
    {
        if ( ( TimeValue::getTimeOfDay() >= getInactivitySuspensionTimeoutEndTime() ) )
        {
            setInactivitySuspensionTimeout(0);
            setLastActivityTimeOverride(getInactivitySuspensionTimeoutEndTime());
            return true;
        }
        return true;
    }

    return false;
}

int32_t Connection::getLastError()
{
    return getBlockingSocket().getLastError();
}


ConnectionOwner::~ConnectionOwner()
{
    for (EncoderByTypeMap::iterator i = mEncoderByTypeMap.begin(), e = mEncoderByTypeMap.end(); i != e; ++i)
        delete i->second;
    for (DecoderByTypeMap::iterator i = mDecoderByTypeMap.begin(), e = mDecoderByTypeMap.end(); i != e; ++i)
        delete i->second;
}

TdfEncoder* ConnectionOwner::getOrCreateEncoder(Encoder::Type type)
{
    EncoderByTypeMap::iterator it = mEncoderByTypeMap.find(type);
    if (it != mEncoderByTypeMap.end())
        return it->second;
    TdfEncoder* encoder = static_cast<TdfEncoder*>(EncoderFactory::create(type));
    if (encoder != nullptr)
        mEncoderByTypeMap[type] = encoder;
    return encoder;
}

TdfDecoder* ConnectionOwner::getOrCreateDecoder(Decoder::Type type)
{
    DecoderByTypeMap::iterator it = mDecoderByTypeMap.find(type);
    if (it != mDecoderByTypeMap.end())
        return it->second;
    TdfDecoder* decoder = static_cast<TdfDecoder*>(DecoderFactory::create(type));
    if (decoder != nullptr)
        mDecoderByTypeMap[type] = decoder;
    return decoder;
}

void ConnectionOwner::queueForRemoval(Connection& connection)
{
    mPendingRemovalQueue.push_back(connection);

    // schedule removal whether or not the connection manager is active, because in a graceful closure case, connectionmanager and outboundconnectionmanager 
    // rely each connections to be removed so that when the last connection is removed they'll get signaled so to indicate that all their connections are gracefully closed.
    if (!mQueuedRemoval)
    {
        // Queue reaping of this connection onto the scheduled fiber
        Fiber::CreateParams params;
        params.stackSize = Fiber::STACK_SMALL;
        gSelector->scheduleFiberTimerCall(0, this, &ConnectionOwner::removeConnections, "ConnectionOwner::removeConnections(0)", params);
        mQueuedRemoval = true;
    }
}

void ConnectionOwner::removeConnections()
{
    while (!mPendingRemovalQueue.empty())
    {
        Connection& conn = mPendingRemovalQueue.front();
        mPendingRemovalQueue.pop_front();
        removeConnection(conn);
    }
    mQueuedRemoval = false;
}

/*** Private Methods *****************************************************************************/


} // Blaze

