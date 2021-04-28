/*************************************************************************************************/
/*!
    \file fire2connection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/fire2connection.h"
#include "BlazeSDK/serviceresolver.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/rpcjob.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usermanager.h"

#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/fire2frame.h"
#include "EATDF/printencoder.h"
#include "EAAssert/eaassert.h"

#ifdef USE_TELEMETRY_SDK
#include "BlazeSDK/util/telemetryapi.h"
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h" // For SocketT
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"


//lint -esym(613, Blaze::Fire2Connection::mProtoFire)  Don't bother checking for these, they're always set

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static Fire2Frame::MessageType convertMessageType(BlazeSender::MessageType msgType);
static BlazeSender::MessageType convertMessageType(Fire2Frame::MessageType msgType);

// The time in ms before inactivity timeout that we send a ping to the server in case other situations are indicating that we are heading for a disconnect.
// Try this as a last resort to avoid failure in a situation that could be caused by random events. 
const uint32_t SAFETYNET_FOR_PING_MS = 5000; 

const size_t RESUME_BUFFER_SIZE = 1024; // resume buffer is used for sending Util::Ping when migration in progress

const size_t MAX_RECONNECT_ATTEMPTS_DEFAULT = 15;
const uint32_t RECONNECT_DEALY_MS = 1000;

/*** Public Methods ******************************************************************************/

#ifdef BLAZE_ENABLE_FUZZING
Fire2Connection::FuzzCallbackT *Fire2Connection::mFuzzCb = nullptr;

void Fire2Connection::setFuzzCallback(FuzzCallbackT *pCallback)
{
    mFuzzCb = pCallback;
}
#endif

Fire2Connection::Fire2Connection(BlazeHub& hub)
 :  BlazeSender(hub),
    mConnectionState(Fire2Connection::OFFLINE),
    mIsMigrating(false),
    mConnectInternalJobId(INVALID_JOB_ID),
    mLastConnectionAttempt(0),
    mNextMessageId(Fire2Frame::MSGNUM_INVALID),
    mAutoConnect(false),
    mAutoReconnect(true),
    mReconnectAttempts(0),
    mMaxReconnectAttempts(MAX_RECONNECT_ATTEMPTS_DEFAULT),
    mProtoSSLRef(nullptr),
    mProtocolState(READ_HEADER),
    mSendPingJobId(INVALID_JOB_ID),
    mPingPeriodMs(15000),
    mConnectionTimeout(getBlazeHub().getInitParams().BlazeConnectionTimeout),
    mReconnectTimeout(3000),
    mInactivityTimeout(20000),
    mIsReconnecting(false),
    mDisconnectError(SDK_ERR_INVALID_STATE),  // ERR_OK is a valid disconnect reason (user triggered) so we use a different value here.
    mResumeBuffer(RESUME_BUFFER_SIZE),
    mUseResumeBuffer(false)
{
    DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_FRAMEWORK));
    mProtoSSLRef = ProtoSSLCreate();
    DirtyMemGroupLeave();

    if (hub.getCertData() != nullptr)
        ProtoSSLControl(mProtoSSLRef, 'scrt', (int32_t)strlen(hub.getCertData()), 0, (void*)hub.getCertData());
    if (hub.getKeyData() != nullptr)
        ProtoSSLControl(mProtoSSLRef, 'skey', (int32_t)strlen(hub.getKeyData()), 0, (void*)hub.getKeyData());
}

Fire2Connection::~Fire2Connection()
{
    getBlazeHub().removeIdler(this);
    if (getConnectionState() != Fire2Connection::OFFLINE)
    {
        disconnect();
    }

    ProtoSSLDestroy(mProtoSSLRef);
}

void Fire2Connection::setConnectionCallbacks(const ConnectionCallback& connectCallback, const ConnectionCallback& disconnectCallback,
                                             const ReconnectCallback& reconnectBeginCallback, const ReconnectCallback& reconnectEndCallback)
{
    mConnectCallback = connectCallback;
    mDisconnectCallback = disconnectCallback;
    mReconnectBeginCallback = reconnectBeginCallback;
    mReconnectEndCallback = reconnectEndCallback;
}

void Fire2Connection::connect()
{
    reconnectInternal(false);
}

void Fire2Connection::reconnect()
{
    reconnectInternal(true);
}

void Fire2Connection::reconnectInternal(bool attemptingReconnect)
{
    if (attemptingReconnect && 
        (!mIsMigrating || mDisconnectError != ERR_MOVED))
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].reconnectInternal: Migration reconnection attempt cancelled.");
        return;
    }

    if (isActive())
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].reconnectInternal: Cannot call connect() while already connected or connecting.");
        return;
    }
    
    mConnectionState = Fire2Connection::CONNECTING;
    mDisconnectError = SDK_ERR_INVALID_STATE;

    // set last connection attempt for normal connect or migration
    if (mReconnectAttempts == 0)
        mLastConnectionAttempt = getBlazeHub().getCurrentTime();

    if (mIsMigrating)
    {
        ++mReconnectAttempts;
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].reconnectInternal: Beginning a reconnect attempt for migration. Attempts: " << mReconnectAttempts);
    }
    else
        mReconnectAttempts = 0; // Set the number of attempts to 0 when starting a fresh connection

    // We actually connect on the next idle, not right now to ensure there's an idle
    // if we immediately fail to connect.
    if (mConnectInternalJobId == INVALID_JOB_ID)
        mConnectInternalJobId = getBlazeHub().getScheduler()->scheduleMethod("connectInternal", this, &Fire2Connection::connectInternal, this);
}

void Fire2Connection::connectInternal()
{
    // connectInternal() could be triggered by a RPC directly when offline, we need to set the last connection attempt
    if (mReconnectAttempts == 0)
        mLastConnectionAttempt = getBlazeHub().getCurrentTime();

    if (mConnectInternalJobId != INVALID_JOB_ID)
    {
        // In case this method was called manually, make sure we get rid of any outstanding
        // connectionInternal() jobs in the queue.
        Job *job = getBlazeHub().getScheduler()->getJob(mConnectInternalJobId);
        if ((job != nullptr) && !job->isExecuting())
            getBlazeHub().getScheduler()->removeJob(job);
        mConnectInternalJobId = INVALID_JOB_ID;
    }

    mConnectionState = Fire2Connection::CONNECTING;

    if (!getServerConnInfo().isResolved())
    {
        // Once the service name is resolved, connectInternal() will be called again.
        resolveServiceName();
        return;
    }

    getBlazeHub().addIdler(this, "Fire2Connection");

    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].connectInternal: Connecting to " <<
        (getServerConnInfo().getSecure() ? "secure" : "insecure") << " " << getServerConnInfo().getAddress() << ":" << getServerConnInfo().getPort());

    // Fire off a connection
    int32_t result = ProtoSSLConnect(mProtoSSLRef, (getServerConnInfo().getSecure() ? 1 : 0), getServerConnInfo().getAddress(), 0, getServerConnInfo().getPort());
    if (result != SOCKERR_NONE)
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].connectInternal: ProtoSSLConnect() failed with err(" << result << ":"<< getProtoSocketErrorName(result) << " ).");

        if (mIsMigrating)
        {
            disconnectInternal(ERR_MOVED);
            getBlazeHub().getScheduler()->scheduleMethod("reconnect", this, &Fire2Connection::reconnect, this, RECONNECT_DEALY_MS);
        }
        else
            onConnectFinished(SDK_ERR_BLAZE_CONN_FAILED);
    }
    else
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].connectInternal: Connection attempt will timeout in " << getConnectionTimeout() << " ms.");
    }
}

void Fire2Connection::onConnectFinished(BlazeError error)
{
    int32_t sslError = 0, sockError = 0;
    getProtoErrors(sslError, sockError);

    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].onConnectFinished: err(" << getBlazeHub().getComponentManager()->getErrorName(error)
        << "), sslErr(" << sslError << ":" << getProtoSSlErrorName(sslError)
        << "), sockErr(" << sockError << ":" << getProtoSocketErrorName(sockError) << ").");

    if (error == ERR_OK)
    {
        mConnectionState = Fire2Connection::ONLINE;
        updateLastReceiveTime();
    }
    else
    {
        // We failed to connect, cleanup the socket and any outstanding request/response data.
        disconnectInternal(error);
    }

    // We don't want to bother the user with mConnectCallback notifications if re are
    // re-connecting.  Instead, the mReconnectBeginCallback and mReconnectEndCallback will
    // be called at the appropriate time.
    if (!mIsReconnecting)
    {
        if (mConnectCallback.isValid())
            mConnectCallback(error, sslError, sockError);
    }
}

void Fire2Connection::disconnect()
{
    if (!isActive())
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnect: Cannot call disconnect() while not already connected or connecting.");
        return;
    }

    // ERR_OK prevents the mDisconnectCallback from being called, which is by design
    // when the user intentionally disconnects.
    disconnectInternal(ERR_OK);
}

void Fire2Connection::disconnectInternal(BlazeError error)
{
    // Update the disconnect reason - Only attempt a reconnect if no other ERR occurs in between. 
    if (mDisconnectError == ERR_MOVED || mDisconnectError == SDK_ERR_INVALID_STATE)
        mDisconnectError = error;

    int32_t sslError = 0, sockError = 0;
    getProtoErrors(sslError, sockError);

    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnectInternal: err("
        << ((getBlazeHub().getComponentManager() != nullptr) ? getBlazeHub().getComponentManager()->getErrorName(error) : "nullptr")
        << "), sslErr(" << sslError << ":" << getProtoSSlErrorName(sslError)
        << "), sockErr(" << sockError << ":" << getProtoSocketErrorName(sockError) << ").");
    
    reportDisconnectToTelemetry(error, sslError, sockError);

    if (mConnectInternalJobId != INVALID_JOB_ID)
    {
        getBlazeHub().getScheduler()->removeJob(mConnectInternalJobId);
        mConnectInternalJobId = INVALID_JOB_ID;
    }
    if (mSendPingJobId != INVALID_JOB_ID)
    {
        getBlazeHub().getScheduler()->removeJob(mSendPingJobId);
        mSendPingJobId = INVALID_JOB_ID;
    }

    ProtoSSLDisconnect(mProtoSSLRef);

    ConnectionState priorState = mConnectionState;
    mConnectionState = OFFLINE;
    mProtocolState = READ_HEADER;
    mIsReconnecting = false;

    if (error != ERR_MOVED)
    {
        mIsMigrating = false;
        // Clear out all transaction buffers, jobs, states, etc. maintained by BlazeSender if we are not migrating
        // Returning SDK_ERR_NOT_CONNECTED here, since this will cancel all outstanding jobs and trigger user callbacks, which will not expect most errors.
        resetTransactionData(SDK_ERR_NOT_CONNECTED);
    }
    else
    {
        if (mReconnectAttempts >= mMaxReconnectAttempts)
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnectInternal: Maximum number of reconnection attempts (" << mMaxReconnectAttempts << ") has been reached.");
            mIsMigrating = false;
            if (mDisconnectCallback.isValid())
                getBlazeHub().getScheduler()->scheduleFunctor("disconnectInternalCb", mDisconnectCallback, error, sslError, sockError);
        }
    }
    getBlazeHub().removeIdler(this);

    // if this is an unexpected disconnection, then:
    if (error != ERR_OK && error != ERR_MOVED)
    {
        // Can/should we try to resume the connection and session on the same server instance?
        if (shouldResumeConnection(error))
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnectInternal: Beginning a re-connection attempt.");
            mIsReconnecting = true;
            ++mReconnectAttempts;

            uint32_t nextConnTime = mLastConnectionAttempt + getReconnectTimeout();
            uint32_t delay = (getBlazeHub().getCurrentTime() > nextConnTime ? 0 : nextConnTime - getBlazeHub().getCurrentTime());

            if (mSendPingJobId == INVALID_JOB_ID)
                mSendPingJobId = getBlazeHub().getScheduler()->scheduleMethod("sendPing", this, &Fire2Connection::sendPing, this, delay);

            if (mReconnectBeginCallback.isValid())
                getBlazeHub().getScheduler()->scheduleFunctor("reconnectBeginCb", mReconnectBeginCallback);
        }
        else
        {
            if (mReconnectAttempts < mMaxReconnectAttempts)
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnectInternal: No re-connection attempt will be made.");
                // Only call the disconnect callback if we are transitioning from the ONLINE state, indicating
                // this was an unexpected disconnection, which was not resumable.
                if (mDisconnectCallback.isValid() && (priorState == ONLINE))
                    getBlazeHub().getScheduler()->scheduleFunctor("disconnectInternalCb", mDisconnectCallback, error, sslError, sockError);
            }
            else
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].disconnectInternal: Maximum number of reconnection attempts (" << mMaxReconnectAttempts << ") has been reached.");
                if (mDisconnectCallback.isValid())
                    getBlazeHub().getScheduler()->scheduleFunctor("disconnectInternalCb", mDisconnectCallback, error, sslError, sockError);
            }
        }
    }

    if (!mIsMigrating)
    {
        mReconnectAttempts = 0; // If we're not going to reconnect for migration, reset this
    }

    // If we're not going to reconnect, and a service name exists, then...
    if (!mIsReconnecting && (*getServerConnInfo().getServiceName() != '\0'))
    {
        // ...clear out everything but the service name so it will be resolved again.
        // This allows the redirector to do all of it good stuff (connection balancing, display message, etc.)
        BlazeSender::ServerConnectionInfo connInfo(getServerConnInfo().getServiceName());
        setServerConnInfo(connInfo);
    }
}

bool Fire2Connection::shouldResumeConnection(BlazeError error)
{
    return ((mReconnectAttempts < mMaxReconnectAttempts) && isResumableError(error));
}

bool Fire2Connection::isResumableError(BlazeError error)
{
    // Add errors as needed
    return mAutoReconnect &&
        ((error == SDK_ERR_BLAZE_CONN_FAILED) ||
        (error == SDK_ERR_BLAZE_CONN_TIMEOUT) ||
        (error == SDK_ERR_SERVER_DISCONNECT) ||
        (error == SDK_ERR_INACTIVITY_TIMEOUT));
}

void Fire2Connection::reportDisconnectToTelemetry(BlazeError error, int32_t sslError, int32_t sockError)
{
#ifdef USE_TELEMETRY_SDK
    if (getBlazeHub().getUserManager() != nullptr)
    {
        Blaze::Telemetry::TelemetryAPI* telemetryApi = getBlazeHub().getTelemetryAPI(getBlazeHub().getPrimaryLocalUserIndex());
        if ((telemetryApi != nullptr) && telemetryApi->isDisconnectTelemetryEnabled())
        {
            // Build the telemetry event, and send it off. Note, telemetry error codes are guaranteed to be negative.

            // Initialize the telemetry event.
            TelemetryApiEvent3T telemEvent;
            int32_t retVal = TelemetryApiInitEvent3(&telemEvent,'BLAZ','FMWK','DISC');

            // Add some info to the telemetry event that we will send.
            retVal += TelemetryApiEncAttributeString(&telemEvent, 'bers', getBlazeHub().getComponentManager()->getErrorName(error));
            retVal += TelemetryApiEncAttributeInt(&telemEvent, 'ssle', sslError);
            retVal += TelemetryApiEncAttributeInt(&telemEvent, 'serr', sockError);

            if (retVal == TC_OKAY)
            {
                // Send the telemetry event.
                telemetryApi->queueBlazeSDKTelemetry(telemEvent);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[Fire2Connection].reportDisconnectToTelemetry: unable to queue BlazeSDK telemetry");
            }
        }
    }
#endif
}

void Fire2Connection::dirtyDisconnect()
{
    // Low level disconnect for debugging purposes
    ProtoSSLDisconnect(mProtoSSLRef);
}

void Fire2Connection::migrationComplete()
{
    if (mIsMigrating)
    {
        // We have connected to the correct server, migration is complete
        mIsMigrating = false;
        mReconnectAttempts = 0;
        auto& sendBuffer = getSendBuffer();
        size_t queuedDataSize = sendBuffer.data() - sendBuffer.ackOffset();
        if (queuedDataSize > 0)
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].migrationComplete: Flush all held requests. Send: " << queuedDataSize << " bytes.");
            sendBuffer.push(queuedDataSize); // send all requests from last ack'ed offset position
            sendRequestFromBuffer();
            if (sendBuffer.datasize() == 0)
                sendBuffer.reset();
            sendBuffer.resetToPrimary();
        }
        else
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].migrationComplete: No held requests need flushing.");
        }
    }
    else
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].migrationComplete: No migration in progress, no-op.");
    }
}

void Fire2Connection::onServiceNameResolutionStarted()
{
    // If the underlying BlazeSender decides it needs to resolve the service name,
    // we need to transition to CONNECTING to reflect a connection is being attempted.
    mConnectionState = CONNECTING;
}

void Fire2Connection::onServiceNameResolutionFinished(BlazeError result)
{
    if (result == ERR_OK)
    {
        // The service name resolution was successful, kick off a connection attempt.
        connectInternal();
    }
    else
    {
        // We failed to resolve the service name, quit any current connection attempt.
        if (mConnectionState == CONNECTING)
            onConnectFinished(result);
        else
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].onServiceNameResolutionFinished: failed to resolve service name");
            disconnectInternal(result);
        }
    }
}

void Fire2Connection::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    {
        BLAZE_SDK_SCOPE_TIMER("Fire2Connection_protoSSLUpdate");
        ProtoSSLUpdate(mProtoSSLRef);
    }

    switch (mConnectionState)
    {        
        case CONNECTING:
        {
            BLAZE_SDK_SCOPE_TIMER("Fire2Connection_connecting");

            if (currentTime - mLastConnectionAttempt > getConnectionTimeout())
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].idle: Connection Timeout! Unable to connect for more than: " << getConnectionTimeout() << " ms.");
                onConnectFinished(SDK_ERR_BLAZE_CONN_TIMEOUT);
                break;
            }

            // 'stat' will track whether the connection is in progress or has failed
            int32_t sslStat = ProtoSSLStat(mProtoSSLRef, 'stat', nullptr, 0);
            if (sslStat > 0)
                onConnectFinished(ERR_OK);
            else if (sslStat < 0)
            {
                if (mIsMigrating)
                {
                    disconnectInternal(ERR_MOVED);
                    getBlazeHub().getScheduler()->scheduleMethod("reconnect", this, &Fire2Connection::reconnect, this, RECONNECT_DEALY_MS);
                }
                else
                    onConnectFinished(SDK_ERR_BLAZE_CONN_FAILED);
            }

            break;
        }
        case ONLINE:
        {
            // Process any received data
            processIncoming(currentTime);

            // Send any data waiting in the mSendBuffer
            if (mConnectionState == ONLINE)
                sendRequestFromBuffer();

            // See if it's time to send out a ping.
            if (mConnectionState == ONLINE)
                checkPing();

            if (mConnectionState == ONLINE)
            {
                if (!getBlazeHub().getInitParams().IgnoreInactivityTimeout &&
                    (currentTime - getLastReceiveTime() > getInactivityTimeout()))
                {
                    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].idle: Inactivity Timeout! Nothing has been received for more than: " << getInactivityTimeout() << " ms.");
                    disconnectInternal(SDK_ERR_INACTIVITY_TIMEOUT);
                }
            }

            break;
        }
        case OFFLINE:
            //nothing to do here
            break;
    }
}

void Fire2Connection::checkPing()
{
    // Pings should only be sent when there nothing else going on atm.
    if (mSendPingJobId == INVALID_JOB_ID)
    {
        if (getPendingRpcs() == 0)
        {
            if (NetTickDiff(getBlazeHub().getLastIdleTime(), getLastRequestTime()) > (int32_t)getPingPeriod())
            {
                if (mIsMigrating)
                {
                    // Don't queue up pings while the connection is not 'ready' to avoid having the queued ping frames consume unnecessary send buffer space.
                    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].checkPing: Migration in progress. Skip sending ping.");
                }
                else
                {
                    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].checkPing: RPC count 0. Send ping.");
                    mSendPingJobId = getBlazeHub().getScheduler()->scheduleMethod("sendPing", this, &Fire2Connection::sendPing, this);
                }
            }
        }
        // as a last ditch effort, check when we received the last response. If it is too long, send a ping before we attempt to 
        // disconnect.
        else if (NetTickDiff(getBlazeHub().getLastIdleTime(), getLastReceiveTime()) > (int32_t)(getInactivityTimeout() - SAFETYNET_FOR_PING_MS))
        {
            if (mIsMigrating)
            {
                // Don't queue up pings while the connection is not 'ready' to avoid having the queued ping frames consume unnecessary send buffer space.
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].checkPing: Migration in progress. Skip sending ping.");
            }
            else
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].checkPing: Send ping in the safety net time period to avoid disconnection.");
                mSendPingJobId = getBlazeHub().getScheduler()->scheduleMethod("sendPing", this, &Fire2Connection::sendPing, this);
            }
        }
    }
}

void Fire2Connection::sendPing()
{
    uint32_t msgId = getNextMessageId();
    BlazeError callbackErr = sendRequestToBuffer(getBlazeHub().getPrimaryLocalUserIndex(), 0, 0, PING, nullptr, msgId);
    if (callbackErr != ERR_OK)
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendPing: attempt to use overflow");
        MultiBuffer& sendBuffer = chooseSendBuffer();
        sendBuffer.useOverflow(getOverflowOutgoingBufferSize());
        callbackErr = sendRequestToBuffer(getBlazeHub().getPrimaryLocalUserIndex(), 0, 0, PING, nullptr, msgId);
    }

    if (callbackErr == ERR_OK)
    {
        // Manually trigger the send, which may invoke a connection attempt.
        callbackErr = sendRequestFromBuffer();
        if (callbackErr == ERR_OK)
        {
            BLAZE_SDK_DEBUGF("%s%s: ID[%i], UI[%u], %s::%s [0x%.4X::0x%.4X]%s\n", 
                "-> ", "req", msgId, 
                getBlazeHub().getPrimaryLocalUserIndex(), 
                "Fire2Connection",
                "PING", 
                0, 0, "");
            
            updateLastRequestTime();
            // Don't increment the pending rpc count for a manual ping. Because the sendRequestFromBuffer may not send the ping to the server and may not fail right away.
            // And if that happens, we will never see a response back.
            // We'll still not cause extra pings because we check for the ping job to be invalid before sending a ping
        }
    }

    if (callbackErr != ERR_OK)
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendPing: failed request");
        // reset/clear the job so that we're not prevented from trying to send another ping later
        if (mSendPingJobId != INVALID_JOB_ID)
        {
            getBlazeHub().getScheduler()->removeJob(mSendPingJobId);
            mSendPingJobId = INVALID_JOB_ID;
        }
    }
}

void Fire2Connection::handleReceivedPingReply(uint32_t userIndex, uint32_t msgId, Fire2Metadata &metadata)
{
    if (mSendPingJobId != INVALID_JOB_ID)
    {
        getBlazeHub().getScheduler()->removeJob(mSendPingJobId);
        mSendPingJobId = INVALID_JOB_ID;
    }

    BlazeError error = metadata.getErrorCode();
    
    BLAZE_SDK_DEBUGF("%s%s: ID[%i], UI[%u], %s::%s [0x%.4X::0x%.4X] %s\n", 
        "<- ", "resp", msgId, 
        userIndex, 
        "Fire2Connection",
        "PING_REPLY", 
        0, 0, getBlazeHub().getComponentManager()->getErrorName(error));
   
    if (error == ERR_OK)
    {
        if (mReconnectEndCallback.isValid() && mIsReconnecting)
            mReconnectEndCallback();
        mIsReconnecting = false;
    }
    else
    {
        // We're here probably because the server refused the session key that was provided.
        // Now, all we can do is completely disconnect, forcing the user to re-authenticate.
        disconnectInternal(SDK_ERR_INVALID_SESSION);
    }
}

void Fire2Connection::handleReceivedPing(uint32_t userIndex, uint32_t msgId, Fire2Metadata &metadata)
{
    BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].handleReceivedPing: Received a PING from the server, sending a PING_REPLY");
    sendRequestToBuffer(userIndex, 0, 0, BlazeSender::PING_REPLY, nullptr, msgId);
}

uint32_t Fire2Connection::getNextMessageId()
{
    mNextMessageId = Fire2Frame::getNextMessageNum(mNextMessageId);
    return mNextMessageId;
}

BlazeError Fire2Connection::canSendRequest()
{
    return (mAutoConnect || (mConnectionState != OFFLINE) ? ERR_OK : SDK_ERR_NOT_CONNECTED);
}

BlazeError Fire2Connection::sendRequestToBuffer(uint32_t userIndex, uint16_t component, uint16_t command, MessageType type, const EA::TDF::Tdf *msg, uint32_t msgId)
{
    MultiBuffer& buffer = chooseSendBuffer();
    return sendRequestToBufferInternal(buffer, type, component, command, msgId, userIndex, msg);
}

BlazeError Fire2Connection::sendRequestToBufferInternal(MultiBuffer &buffer, MessageType type, uint16_t component, uint16_t command, uint32_t msgId, uint32_t userIndex, const EA::TDF::Tdf * msg )
{
    Fire2Metadata *metadata = nullptr;
    if (mIsReconnecting || mIsMigrating)
    {
        const Blaze::UserManager::LocalUser* localUser = getBlazeHub().getUserManager()->getLocalUser(userIndex);
        if (localUser != nullptr)
        {
            mMetadata.setSessionKey(localUser->getUserSessionKey());
            metadata = &mMetadata;
        }
    }

    // Save our spot on the buffer, so we can roll back in case of an error.
    uint8_t *originalTail = buffer.tail();

    // Verify we at least have room for the header.
    uint32_t headerSize = Fire2Frame::HEADER_SIZE + (metadata == nullptr ? 0 : Fire2Frame::METADATA_SIZE_MAX);

    if (buffer.tailroom() < headerSize)
    {
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: not enough buffer tailroom (" << (uint32_t)buffer.tailroom() << ") for header (" << headerSize << ")");
        return SDK_ERR_RPC_SEND_FAILED;
    }

    Fire2Frame::MessageType msgType = convertMessageType(type);

    // Copy the frame and data into the send buffer
    Fire2Frame frame(buffer.tail(), 0, 0, component, command, msgId, msgType, userIndex, Fire2Frame::OPTION_NONE);
    buffer.put(Fire2Frame::HEADER_SIZE);

    // We don't know how much room the metadata and tdf will take, so just try
    // and stuff it on the buffer and see what happens.
    BlazeError sendResult = ERR_OK;
    if (metadata != nullptr)
    {
        if (!mEncoder.encode(buffer, *metadata)) //false == errors
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: failed to encode metadata");

            // We failed, go ahead and roll back the buffer
            buffer.trim((size_t)(buffer.tail() - originalTail));
            sendResult = SDK_ERR_RPC_SEND_FAILED;
        }
        else
        {
            const size_t metadataSize = buffer.tail() - (originalTail + Fire2Frame::HEADER_SIZE);
            if (metadataSize > UINT16_MAX)
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: metadataSize is too big in a Fire2 packet, metadataSize: " << (uint64_t)metadataSize);

                // We failed, go ahead and roll back the buffer
                buffer.trim(buffer.tail() - originalTail);
                sendResult = SDK_ERR_RPC_SEND_FAILED;
            }
            else
            {
#if ENABLE_BLAZE_SDK_LOGGING
                Blaze::StringBuilder sb;
                metadata->print(sb, 0, true); // intentionally print on single line
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: metadataSize(" << (uint64_t) metadataSize << ")," << sb);
#endif
                frame.setMetadataSize((uint16_t)metadataSize);
            }
        }
    }

    if ((sendResult == 0) && (msg != nullptr))
    {
        if (!mEncoder.encode(buffer, *msg)) //false == errors
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: failed to encode message");

            // We failed, go ahead and roll back the buffer
            buffer.trim((uint32_t)(buffer.tail() - originalTail));
            sendResult = SDK_ERR_RPC_SEND_FAILED;
        }
        else
        {
            const size_t payloadSize = buffer.tail() - (originalTail + Fire2Frame::HEADER_SIZE + frame.getMetadataSize());
            if (payloadSize > UINT32_MAX)
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestToBuffer: payloadSize is too big in a Fire2 packet, payloadSize: " << (uint64_t)payloadSize);

                // We failed, go ahead and roll back the buffer
                buffer.trim(buffer.tail() - originalTail);
                sendResult = SDK_ERR_RPC_SEND_FAILED;
            }
            else
            {
                frame.setPayloadSize((uint32_t)payloadSize);
            }            
        }
    }

    return sendResult;
}

bool Fire2Connection::prepareReceiveBuffer(const Fire2Frame& currentFrame)
{
    bool bufferPrepared = BlazeSender::prepareReceiveBuffer((size_t)currentFrame.getTotalSize());
    if (!bufferPrepared)
    {
        // This packet is bigger than even our overflow buffer, or is somehow
        // corrupted.  Just bail out and kill the connection.
        disconnectInternal(SDK_ERR_DISCONNECT_OVERFLOW);
    }

    return bufferPrepared;
}

void Fire2Connection::processIncoming(uint32_t currentTime)
{
    BLAZE_SDK_SCOPE_TIMER("Fire2Connection_processIncoming");

    // We will loop and slurp in as much data as possible until there's none left.  
    // tryNextMessage is set to true when a message completes.  Otherwise we break.
    bool tryNextMessage = true;
    while (tryNextMessage) 
    {
        tryNextMessage = false;
        Fire2Frame currentFrame(getReceiveBuffer().head());
        if (mProtocolState == READ_HEADER)
        {
            // Try to get the rest of the header. Note, this will disconnect() if it fails.
            if (Fire2Frame::HEADER_SIZE < getReceiveBuffer().datasize()) 
            {
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: error, receive buffer data size "
                    << (uint32_t)getReceiveBuffer().datasize() << " exceeds Fire2Frame header size "<< Fire2Frame::HEADER_SIZE << ". Integer overflow.");
                disconnectInternal(SDK_ERR_DISCONNECT_OVERFLOW);
            }
            else if (!receiveToBuffer((uint32_t)(Fire2Frame::HEADER_SIZE - getReceiveBuffer().datasize()), currentTime)) 
            {
                return;
            }

            if (getReceiveBuffer().datasize() == Fire2Frame::HEADER_SIZE)
            {
                if (!prepareReceiveBuffer(currentFrame)) // This will disconnect() if it fails
                    return;
                mProtocolState = READ_BODY;
            }
        }

        if (mProtocolState == READ_BODY)
        {
            // we already ensured that the frame's total size is within the uint32_t limit in previous protocol state (in prepareReceiveBuffer() above)
            uint32_t remainingBytes = (uint32_t)(currentFrame.getTotalSize() - getReceiveBuffer().datasize());

            if (!receiveToBuffer(remainingBytes, currentTime)) // This will disconnect() if it fails
                return;

            // See if the call to receiveToBuffer() got everything we wanted
            if (getReceiveBuffer().datasize() == currentFrame.getTotalSize())
            {
                // Move data() up to the beginning of the Fire2Metadata.
                getReceiveBuffer().pull(Fire2Frame::HEADER_SIZE);

                Fire2Metadata currentMetadata;
                char8_t defaultSessionKey[Blaze::MAX_SESSION_KEY_LENGTH];
                blaze_strnzcpy(defaultSessionKey, currentMetadata.getSessionKey(), sizeof(defaultSessionKey));
                if (currentFrame.getMetadataSize() > 0)
                {
                    // The decoder will pull() the mData pointer all the way to mTail, so we temporarily adjust
                    // the mTail so that it is at the end of the metadata.  Then once we're done decoding we need
                    // to put() the mTail back were it was.
                    getReceiveBuffer().trim(currentFrame.getPayloadSize());
                    BlazeError decoderErr = mDecoder.decode(getReceiveBuffer(), currentMetadata, true);
                    if (decoderErr == ERR_OK)
                    {
#if ENABLE_BLAZE_SDK_LOGGING
                        Blaze::StringBuilder sb;
                        currentMetadata.print(sb, 0, true); // intentionally print on single line
                        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: metadataSize("<< currentFrame.getMetadataSize() << ")," << sb);
#endif
                    }
                    else
                    {
                        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: Failed to decode the metadata in a Fire2 packet, err(" << getBlazeHub().getComponentManager()->getErrorName(decoderErr) << ")");
                        disconnectInternal(decoderErr);
                        return;
                    }
                    getReceiveBuffer().put(currentFrame.getPayloadSize());
                }

                if (currentFrame.getMsgType() == Fire2Frame::PING)
                {
                    // Special handling for PING messages.                    
                    handleReceivedPing(currentFrame.getUserIndex(), currentFrame.getMsgNum(), currentMetadata);
                }
                else if (currentFrame.getMsgType() == Fire2Frame::PING_REPLY)
                {
                    // Special handling for PING_REPLY messages.
                    handleReceivedPingReply(currentFrame.getUserIndex(), currentFrame.getMsgNum(), currentMetadata);
                }
                else
                {
#ifdef BLAZE_ENABLE_FUZZING
                    if (mFuzzCb != nullptr)
                    {
                        size_t datasize = (size_t) currentFrame.getPayloadSize();
                        mFuzzCb(getReceiveBuffer().data(), datasize);
                        currentFrame.setPayloadSize((uint32_t) datasize);
                    }
#endif
                    bool handlePacket = true;
                    auto errCode = currentMetadata.getErrorCode();
                    if (!mIsMigrating)
                    {
                        if (errCode != ERR_MOVED)
                        {
                            // Acked frame when client is not in migration state and not ERR_MOVED
                            ackFrame(currentFrame);
                        }
                        else
                        {
                            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: Skip pulling Ack frame (" << currentFrame.getMsgNum() << ") when ERR_MOVED received.");
                            handlePacket = false;
                        }
                    }
                    else
                    {
                        if (currentFrame.getMsgType() == Fire2Frame::ERROR_REPLY)
                        {
                            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: Migrating, errCode(" << getBlazeHub().getComponentManager()->getErrorName(errCode) << ") received.");
                            if (currentMetadata.getMovedToAddr().getIp() == 0) // intentionally not using getMovedToAddr().isSet() because change tracking can be ifdef'ed out on the SDK
                            {
                                // No available server has been found who owns our usersession when migrating, disconnect
                                mIsMigrating = false;
                                disconnectInternal(SDK_ERR_SERVER_DISCONNECT);
                                break;
                            }
                        }
                    }

                    // migrating to the instance provided in the Fire2Metadata
                    if (currentMetadata.getMovedToAddr().getIp() != 0) // intentionally not using getMovedToAddr().isSet() because change tracking can be ifdef'ed out on the SDK
                    {
                        mIsMigrating = true;
                        // cache off service name && secure mode because disconnect resets the ServerConnInfo
                        ServerConnectionInfo cachedInfo(getServerConnInfo());

                        // ERR_MOVED prevents the mDisconnectCallback from being called, which is by design when migrating to the new instance
                        disconnectInternal(ERR_MOVED);

                        uint16_t port = currentMetadata.getMovedToAddr().getPort();

                        InternetAddress inetAddr(currentMetadata.getMovedToAddr().getIp(), port);
                        char8_t address[SERVER_ADDRESS_MAX + 1];
                        inetAddr.asString(address, sizeof(address));

                        const char8_t* hostName = currentMetadata.getMovedToHostName();
                        if (hostName[0] != '\0')
                        {
                            setServerConnInfo(cachedInfo.getServiceName(), hostName, port, cachedInfo.getSecure());
                        }
                        else
                        {                            
                            setServerConnInfo(cachedInfo.getServiceName(), address, inetAddr.getPort(), cachedInfo.getSecure());
                        }                        

                        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].processIncoming: Migrating to new server, hostname(" << hostName << "), IP(" << address << ") and Port(" 
                            << port << "). Secure: " << cachedInfo.getSecure() << ", Servicename: "<< cachedInfo.getServiceName());

                        reconnect();
                    }

                    if (handlePacket)
                    {
                        // Let the generic BlazeSender handle everything else.
                        handleReceivedPacket(
                            currentFrame.getMsgNum(), convertMessageType(currentFrame.getMsgType()),
                            currentFrame.getComponent(), currentFrame.getCommand(), currentFrame.getUserIndex(),
                            currentMetadata.getErrorCode(),
                            mDecoder, getReceiveBuffer().data(), currentFrame.getPayloadSize());
                    }
                }

                // Reset the receive buffer, and start over.
                getReceiveBuffer().resetToPrimary();

                mProtocolState = READ_HEADER;
                tryNextMessage = (mConnectionState == ONLINE);
            }
        }
    }
}

bool Fire2Connection::receiveToBuffer(uint32_t expected, uint32_t currentTime)
{
    if (expected == 0)
        return true;

    if (expected > getReceiveBuffer().tailroom())
    {
        // No room, throw a disconnect error
        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].receiveToBuffer: error, expected data size "
            << expected << " exceeds receive buffer room " << (uint32_t)getReceiveBuffer().tailroom());
        disconnectInternal(SDK_ERR_DISCONNECT_OVERFLOW);
        return false;
    }

    // check if the sock is still valid
    int32_t result = ProtoSSLStat(mProtoSSLRef, 'stat', nullptr, 0);
    if (result != -1)
    {
        result = ProtoSSLRecv(mProtoSSLRef, (char8_t*)getReceiveBuffer().tail(), expected);
    }

    if (result < 0)
    {
        int32_t sslError = 0, sockError = 0;
        getProtoErrors(sslError, sockError);

        BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].receiveToBuffer: ProtoSSLRecv() failed."
            << "sslErr(" << sslError << ":" << getProtoSSlErrorName(sslError)
            << "), sockErr(" << sockError << ":" << getProtoSocketErrorName(sockError) << ").");

        // Our connection has been dropped. We don't know why. The server might have crashed or we were inactive/force closed or our session was
        // migrated gracefully or whatever else might have happened. 
        // Let's try and reconnecting to a random server to ask who owns our user session now (if it is still around), and reconnect to that instance.
        if (!mIsMigrating)
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].receiveToBuffer: starting searching for new instance to migrate.");

            mIsMigrating = true;
            // ERR_MOVED prevents the mDisconnectCallback from being called, which is by design when reconnecting to a new instance.
            // We also don't use the SDK_ERR_SERVER_DISCONNECT right away because we might be able to reconnect to some other server.
            disconnectInternal(ERR_MOVED);
            reconnect();
        }
        else
        {
            disconnectInternal(SDK_ERR_SERVER_DISCONNECT);
        }
    }
    else if (result > 0)
    {
        updateLastReceiveTime(currentTime);

        // Catch up the buffer with what we read
        getReceiveBuffer().put(result);
    }

    return (result >= 0);
}

BlazeError Fire2Connection::sendRequestFromBuffer()
{
    BLAZE_SDK_SCOPE_TIMER("Fire2Connection_sendRequestFromBuffer");

    MultiBuffer& buffer = chooseSendBuffer();

    if (buffer.datasize() == 0)
        return ERR_OK;

    switch (mConnectionState)
    {
    case OFFLINE:
        {
            if (mAutoConnect && (mConnectInternalJobId == INVALID_JOB_ID))
                mConnectInternalJobId = getBlazeHub().getScheduler()->scheduleMethod("connectInternal", this, &Fire2Connection::connectInternal, this);
            break;
        }
    case CONNECTING:
        {
            break;
        }
    case ONLINE:
        {
            int32_t sendResult = ProtoSSLSend(mProtoSSLRef, (const char *)buffer.data(), (int32_t)buffer.datasize());
            if (sendResult >= 0)
            {
                buffer.pull(sendResult);
            }
            else
            {
                // Send request fail due to connection to server is dropped. 
                // we will hold the request for now. if ProtoSSLRecv() when reading from socket also failed, we will trigger a migration attempt from there, and re-send the holding requests if sucess.
                // Otherwise, trigger disconnect flow.
                BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].sendRequestFromBuffer: ProtoSSLSend() failed, err(" << sendResult 
                    << "). Request has been held and waiting to be re-sent.");
            }
            break;
        }
    }

    if (mUseResumeBuffer)
    {
        // ResumerBuffer is only used for the util::Ping/util::Preauth rpcs for usersession migration,
        // we fire and reset the buffer since we do not need to hold the request
        mUseResumeBuffer = false; 
        mResumeBuffer.resetToPrimary();
    }

    return ERR_OK;
}

Fire2Frame::MessageType convertMessageType(BlazeSender::MessageType msgType)
{
    switch (msgType)
    {
    case BlazeSender::MESSAGE:       return Fire2Frame::MESSAGE;
    case BlazeSender::REPLY:         return Fire2Frame::REPLY;
    case BlazeSender::NOTIFICATION:  return Fire2Frame::NOTIFICATION;
    case BlazeSender::ERROR_REPLY:   return Fire2Frame::ERROR_REPLY;
    case BlazeSender::PING:          return Fire2Frame::PING;
    case BlazeSender::PING_REPLY:    return Fire2Frame::PING_REPLY;
    default:
        EA_FAIL();
        return Fire2Frame::MESSAGE;
    }
}

BlazeSender::MessageType convertMessageType(Fire2Frame::MessageType msgType)
{
    switch (msgType)
    {
    case Fire2Frame::MESSAGE:       return BlazeSender::MESSAGE;
    case Fire2Frame::REPLY:         return BlazeSender::REPLY;
    case Fire2Frame::NOTIFICATION:  return BlazeSender::NOTIFICATION;
    case Fire2Frame::ERROR_REPLY:   return BlazeSender::ERROR_REPLY;
    case Fire2Frame::PING:          return BlazeSender::PING;
    case Fire2Frame::PING_REPLY:    return BlazeSender::PING_REPLY;
    default:
        EA_FAIL();
        return BlazeSender::MESSAGE;
    }
}

// the sendbuffer now holds all the requests we already sent and about to send, AckOffset points to the request that is sent but have not received response
// Once we receive a response and if it matches the current ack frame, we will pull the AckOffset to point to the next frame which waiting to be acked now
void Fire2Connection::ackFrame(Fire2Frame &currentFrame)
{
    if (getSendBuffer().data() - getSendBuffer().ackOffset() <= 0)
    {
        return; // nothing to ACK
    }

    // Skip PING and PING_REPLY in ack frames
    Fire2Frame ackFrame(getSendBuffer().ackOffset());
    while (ackFrame.getTotalSize() != 0 && (ackFrame.getMsgType() == Fire2Frame::PING || ackFrame.getMsgType() == Fire2Frame::PING_REPLY))
    {
        getSendBuffer().pullAckOffset((uint32_t)ackFrame.getTotalSize());
        if (getSendBuffer().data() > getSendBuffer().ackOffset())
        {
            ackFrame.setBuffer(getSendBuffer().ackOffset());
        }
        else
            return;
    }

    // Ack frame when matching currentFrame
    if (currentFrame.getMsgType() == Fire2Frame::REPLY || currentFrame.getMsgType() == Fire2Frame::ERROR_REPLY)
    {
        if (ackFrame.getMsgNum() == currentFrame.getMsgNum())
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].ackFrame: frame(" << ackFrame.getMsgNum() << ") has been ACKed");
            getSendBuffer().pullAckOffset((uint32_t)ackFrame.getTotalSize());
            if (getSendBuffer().data() == getSendBuffer().ackOffset())
            {
                // we have Acked all the requests we sent, now it's time to reset the send buffer
                if (getSendBuffer().datasize() == 0)
                {
                    getSendBuffer().reset();
                    getSendBuffer().resetToPrimary();
                }
            }
        }
        else
        {
            BLAZE_SDK_DEBUGSB("[Fire2Connection::" << this << "].ackFrame: Error occured when Acking frame, frame waiting for Acked is: " << ackFrame.getMsgNum() << ", but current frame msg is: "
                << currentFrame.getMsgNum());

            // We have a mismatch frame, have to drop all the currently holding requests for recovery. Reset Ack offset to rawbuffer data ptr
            size_t pullSize = getSendBuffer().data() - getSendBuffer().ackOffset();
            if (pullSize > 0)
            {
                getSendBuffer().pullAckOffset(getSendBuffer().data() - getSendBuffer().ackOffset());
            }
        }
    }
}

BlazeSender::MultiBuffer& Fire2Connection::chooseSendBuffer() 
{ 
    if (mIsMigrating && mUseResumeBuffer)
    {
        // Use migration buffer for sending request.
        return mResumeBuffer;
    }
    else
        return getSendBuffer(); 
}

void Fire2Connection::getProtoErrors(int32_t& sslErr, int32_t& sockErr)
{
    sslErr = ProtoSSLStat(mProtoSSLRef, 'fail', nullptr, 0);
    sockErr = ProtoSSLStat(mProtoSSLRef, 'serr', nullptr, 0);
}

const char8_t* Fire2Connection::getProtoSSlErrorName(int32_t sslErr)
{
    switch (sslErr)
    {
    case PROTOSSL_ERROR_NONE:
        return "PROTOSSL_ERROR_NONE";
    
    case PROTOSSL_ERROR_DNS:
        return "PROTOSSL_ERROR_DNS";
    
    case PROTOSSL_ERROR_CONN:
        return "PROTOSSL_ERROR_CONN";
    
    case PROTOSSL_ERROR_CONN_SSL2:
        return "PROTOSSL_ERROR_CONN_SSL2";
    
    case PROTOSSL_ERROR_CONN_NOTSSL:
        return "PROTOSSL_ERROR_CONN_NOTSSL";
    
    case PROTOSSL_ERROR_CONN_MINVERS:
        return "PROTOSSL_ERROR_CONN_MINVERS";
    
    case PROTOSSL_ERROR_CONN_MAXVERS:
        return "PROTOSSL_ERROR_CONN_MAXVERS";

    case PROTOSSL_ERROR_CONN_NOCIPHER:
        return "PROTOSSL_ERROR_CONN_NOCIPHER";

    case PROTOSSL_ERROR_CONN_NOCURVE:
        return "PROTOSSL_ERROR_CONN_NOCURVE";

    case PROTOSSL_ERROR_CERT_INVALID:
        return "PROTOSSL_ERROR_CERT_INVALID";
    
    case PROTOSSL_ERROR_CERT_HOST:
        return "PROTOSSL_ERROR_CERT_HOST";

    case PROTOSSL_ERROR_CERT_NOTRUST:
        return "PROTOSSL_ERROR_CERT_NOTRUST";

    case PROTOSSL_ERROR_CERT_MISSING:
        return "PROTOSSL_ERROR_CERT_MISSING";

    case PROTOSSL_ERROR_CERT_BADDATE:
        return "PROTOSSL_ERROR_CERT_BADDATE";

    case PROTOSSL_ERROR_CERT_REQUEST:
        return "PROTOSSL_ERROR_CERT_REQUEST";

    case PROTOSSL_ERROR_SETUP:
        return "PROTOSSL_ERROR_SETUP";

    case PROTOSSL_ERROR_SECURE:
        return "PROTOSSL_ERROR_SECURE";

    case PROTOSSL_ERROR_UNKNOWN:
        return "PROTOSSL_ERROR_UNKNOWN";

    default:
        return ""; // We don't fold it as ERROR_UNKNOWN so that we can actually update our code for a newly added error that is not handled.
    }
}


const char8_t* Fire2Connection::getProtoSocketErrorName(int32_t sockErr)
{
    switch (sockErr)
    {
    case SOCKERR_NONE:
        return "SOCKERR_NONE";

    case SOCKERR_CLOSED:
        return "SOCKERR_CLOSED";

    case SOCKERR_NOTCONN:
        return "SOCKERR_NOTCONN";

    case SOCKERR_BLOCKED:
        return "SOCKERR_BLOCKED";

    case SOCKERR_ADDRESS:
        return "SOCKERR_ADDRESS";

    case SOCKERR_UNREACH:
        return "SOCKERR_UNREACH";

    case SOCKERR_REFUSED:
        return "SOCKERR_REFUSED";

    case SOCKERR_OTHER:
        return "SOCKERR_OTHER";

    case SOCKERR_NOMEM:
        return "SOCKERR_NOMEM";

    case SOCKERR_NORSRC:
        return "SOCKERR_NORSRC";

    case SOCKERR_UNSUPPORT:
        return "SOCKERR_UNSUPPORT";

    case SOCKERR_INVALID:
        return "SOCKERR_INVALID";

    case SOCKERR_ADDRINUSE:
        return "SOCKERR_ADDRINUSE";

    case SOCKERR_CONNRESET:
        return "SOCKERR_CONNRESET";

    case SOCKERR_BADPIPE:
        return "SOCKERR_BADPIPE";

    default:
        return ""; 
    }
}
} // Blaze
