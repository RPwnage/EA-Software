/*************************************************************************************************/
/*!
    \file blazesender.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazesender.h"
#include "BlazeSDK/serviceresolver.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/rpcjob.h"
#include "BlazeSDK/usermanager/usermanager.h"

#include "DirtySDK/dirtysock/netconn.h"

#include "EATDF/printencoder.h"
#include "EAAssert/eaassert.h"

//lint -esym(613, Blaze::BlazeConnection::mProtoFire)  Don't bother checking for these, they're always set

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


BlazeSender::BlazeSender(BlazeHub& hub)
 :  mHub(hub),
    mComponentManagers(nullptr),
    mDefaultRequestTimeout(mHub.getInitParams().DefaultRequestTimeout),
    mLastRequestTime(0),
    mLastReceiveTime(0),
    mPendingRpcs(0),
    mSendBuffer(mHub.getInitParams().OutgoingBufferSize),
    mReceiveBuffer(mHub.getInitParams().DefaultIncomingPacketSize),
    mOverflowOutgoingBufferSize(OVERFLOW_BUFFER_SIZE_MIN),
    mOverflowIncomingBufferSize(OVERFLOW_BUFFER_SIZE_MIN)
{
    //Allocate component managers.
    mComponentManagers = BLAZE_NEW_ARRAY(ComponentManager, mHub.getInitParams().UserCount, MEM_GROUP_FRAMEWORK, "ComponentManager");
    for (uint32_t counter = 0; counter < mHub.getInitParams().UserCount; counter++)
    {
        mComponentManagers[counter].init(&mHub, this, counter);
    }

    // Don't allow the client to reduce the size of the overflow buffers below the minimum.
    if (hub.getInitParams().OverflowOutgoingBufferSize > OVERFLOW_BUFFER_SIZE_MIN)
    {
        mOverflowOutgoingBufferSize = hub.getInitParams().OverflowOutgoingBufferSize;
    }
    else if (hub.getInitParams().OverflowOutgoingBufferSize < OVERFLOW_BUFFER_SIZE_MIN)
    {
        BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].ctor: Ignoring InitParameters::OverflowOutgoingBufferSize as it is less than " << OVERFLOW_BUFFER_SIZE_MIN);
    }

    if (hub.getInitParams().MaxIncomingPacketSize > OVERFLOW_BUFFER_SIZE_MIN)
    {
        mOverflowIncomingBufferSize = hub.getInitParams().MaxIncomingPacketSize;
    }
    else if (hub.getInitParams().MaxIncomingPacketSize < OVERFLOW_BUFFER_SIZE_MIN)
    {
        BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].ctor: Ignoring InitParameters::MaxIncomingPacketSize as it is less than " << OVERFLOW_BUFFER_SIZE_MIN);
    }
}

BlazeSender::~BlazeSender()
{
    // Remove any outstanding jobs who's JobProvider has been set to 'this'.
    mHub.getScheduler()->removeJob(this);
    
    // resetTransactionData does a cancel, but that'll be a non-op since we've already removed everything
    resetTransactionData(SDK_ERR_NOT_CONNECTED);

    BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK, mComponentManagers);
}

void BlazeSender::updateLastReceiveTime(uint32_t currentTime)
{
    mLastReceiveTime = (currentTime == 0 ? mHub.getCurrentTime() : currentTime);
}

void BlazeSender::updateLastRequestTime(uint32_t currentTime)
{
    mLastRequestTime = (currentTime == 0 ? mHub.getCurrentTime() : currentTime);
}

void BlazeSender::resetTransactionData(BlazeError error)
{
    // Cancel any outstanding jobs who's JobProvider has been set to 'this'.
    mHub.getScheduler()->cancelJob(this, error);

    mSendBuffer.resetToPrimary();
    mReceiveBuffer.resetToPrimary();

    // Don't clear our pending rpc count for two reasons.
    // 1. The above cancelJob is not guaranteed to have cancelled(and in turn, deleted) job. It only does so if the job was not executing.
    // 2. The sendRequest code can invoke this if it fails while it is in the middle of scheduling that job. 
    // In both the above scenarios, our calculations will be off because these jobs will be later deleted and decrement the count that they incremented.
    //mPendingRpcs = 0;
    mLastReceiveTime = 0;
    mLastRequestTime = 0;
}

ComponentManager* BlazeSender::getComponentManager(uint32_t userIndex) const
{
    ComponentManager *result = nullptr;
    if (userIndex < mHub.getInitParams().UserCount)
    {
        result = &mComponentManagers[userIndex];
    }
    return result;
}

bool BlazeSender::setServerConnInfo(const ServerConnectionInfo &connInfo)
{
    if (isActive())
    {
        BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].setServerConnInfo: Cannot change the connection info while already connected or connecting.");
        return false;
    }

    mConnInfo = connInfo;
    return true;
}

bool BlazeSender::setServerConnInfo(const char8_t *serviceName)
{
    return setServerConnInfo(ServerConnectionInfo(serviceName));
}

bool BlazeSender::setServerConnInfo(const char8_t *address, uint16_t port, bool secure, bool autoS2S /*= false*/)
{
    return setServerConnInfo(ServerConnectionInfo(address, port, secure, autoS2S));
}

bool BlazeSender::setServerConnInfo(const char8_t *serviceName, const char8_t * address, uint16_t port, bool secure, bool autoS2S /*= false*/)
{
    return setServerConnInfo(ServerConnectionInfo(serviceName, address, port, secure, autoS2S));
}

JobId BlazeSender::sendRequest(uint32_t userIndex, uint16_t component, uint16_t command,
    const EA::TDF::Tdf *request, RpcJobBase *job, const JobId& reserveId, uint32_t _timeout)
{
    uint32_t timeout = 0;
    uint32_t msgId = 0;
    size_t dataSize = 0;
   
    if (reserveId != INVALID_JOB_ID)
        job->setId(reserveId);

    MultiBuffer& sendBuffer = chooseSendBuffer();

    BlazeError callbackErr = canSendRequest();
    if (callbackErr == ERR_OK)
    {
        msgId = getNextMessageId();
        uint8_t *originalTail = sendBuffer.tail();
        callbackErr = sendRequestToBuffer(userIndex, component, command, MESSAGE, request, msgId);
        if (callbackErr != ERR_OK) 
        {
            // Try again after asking for a bigger buffer.
            BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].sendRequest: attempt to use overflow");
            sendBuffer.useOverflow(mOverflowOutgoingBufferSize);
            originalTail = getSendBuffer().tail();
            callbackErr = sendRequestToBuffer(userIndex, component, command, MESSAGE, request, msgId);
        }

        if (callbackErr == ERR_OK)
        {
            dataSize = (size_t)(sendBuffer.tail() - originalTail);
            // Now that the request has been serialized to the SendBuffer, we need check if we
            // have a valid address, or if the service name still needs to be resolved.
            if (mConnInfo.isResolved())
            {
                callbackErr = sendRequestFromBuffer();
            }
            else if (!isActive())
            {
                resolveServiceName();
            }

            if (callbackErr == ERR_OK)
            {
#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
                BlazeSenderStateListener* listener = getBlazeHub().getBlazeSenderStateListener();
                if (listener != nullptr)
                {
                    listener->onRequest(dataSize, msgId, component, command);
                }
#endif
                callbackErr = ERR_TIMEOUT;
                timeout = (_timeout != USE_DEFAULT_TIMEOUT ? _timeout : mDefaultRequestTimeout);
                updateLastRequestTime(); 
            }
        }
    }
    // log message here as the job is always scheduled. Otherwise, we see unbalanced ->req and <- resp entries in the log. 
    // If the callbackErr is ERR_TIMEOUT, it means no error occurred at this point so we pass ERR_OK to logMessage to avoid log printing the error code.
    logMessage(userIndex, false, false, "req", request, msgId, component, command, (callbackErr == ERR_TIMEOUT) ? ERR_OK : callbackErr, dataSize);

    const char8_t* commandName = mHub.getComponentManager(userIndex) ? mHub.getComponentManager(userIndex)->getCommandName(component, command) : "";

    job->setProvider(this);
    job->setProviderId(msgId);
    job->setCallbackErr(callbackErr);
    mHub.getScheduler()->scheduleJob(commandName, job, nullptr, timeout);  // Use the component/command id as the name.
    
    return job->getId();
}

void BlazeSender::resolveServiceName()
{
    if (mHub.getServiceResolver()->resolveService(mConnInfo.mServiceName, ServiceResolver::ResolveCb(this, &BlazeSender::onRedirectorResponse)) != INVALID_JOB_ID)
    {
        // Clear out any existing redirector messages from the last service name resolution request.
        // The new redirector messages will be provided in the onRedirectorResponse() callback.
        mDisplayMessages.clear();

        onServiceNameResolutionStarted();
    }
}

void BlazeSender::onRedirectorResponse(BlazeError result, const char8_t* serviceName,
                                       const Redirector::ServerInstanceInfo* response,
                                       const Redirector::ServerInstanceError* errorResponse, int32_t sslError, int32_t sockError)
{
    // Save off any DisplayMessages to be acccessed later by the user.
    if ((result == ERR_OK) && (response != nullptr))
        response->getMessages().copyInto(mDisplayMessages);
    if ((result != ERR_OK) && (errorResponse != nullptr))
        errorResponse->getMessages().copyInto(mDisplayMessages);

    if (result == ERR_OK)
    {
        // override the service name with the Redirector's supplied trial service name if there is one
        if (response->getTrialServiceName()[0] != '\0')
        {
            blaze_strnzcpy(mConnInfo.mServiceName, response->getTrialServiceName(), sizeof(mConnInfo.mServiceName));
        }

        const Redirector::IpAddress* const pIpAddr = response->getAddress().getIpAddress();

        // Cannot deref to retrieve the hostname if IpAddress is nullptr, so simply bail out
        if (pIpAddr == nullptr)
        {
            return;
        }

        const char8_t* host = pIpAddr->getHostname();
        if ((host == nullptr) || (host[0] == '\0'))
        {
            InternetAddress inetAddr(pIpAddr->getIp(), 0);
            inetAddr.asString(mConnInfo.mAddress, sizeof(mConnInfo.mAddress));
        }
        else
        {
            blaze_strnzcpy(mConnInfo.mAddress, host, sizeof(mConnInfo.mAddress));
        }

        mConnInfo.mPort = pIpAddr->getPort();
        mConnInfo.mSecure = response->getSecure();
    }

    onServiceNameResolutionFinished(result);

    // If all is well, kick off the send process, which may include establishing a new connection
    if (result == ERR_OK)
        sendRequestFromBuffer();
}

void BlazeSender::handleReceivedPacket(uint32_t msgId, MessageType msgType, ComponentId component, uint16_t command, uint32_t userIndex, BlazeError error, TdfDecoder &decoder, const uint8_t *data, size_t dataSize)
{
    // It's possible that the user may have become deauthenticated while a reconnect was in progress.  
    // To avoid missing that callback (since no notification would come in), we send the deauth callback here: 
    if (error == ERR_AUTHENTICATION_REQUIRED)
    {
        if (getBlazeHub().getUserManager() && getBlazeHub().getUserManager()->getLocalUser(userIndex) != nullptr)
        {
            getBlazeHub().getUserManager()->onLocalUserDeAuthenticated(userIndex);
        }
    }


    if ((msgType == REPLY) || (msgType == ERROR_REPLY))
    {
        // This is a job we're tracking - lookup the job.
        RpcJobBase *job = static_cast<RpcJobBase *>(getBlazeHub().getScheduler()->getJob(this, msgId));
        if (job != nullptr)
        {
            // Remove the job from the scheduler - we take on ownership after this.  This
            // needs to happen before any callbacks just like when the JobScheduler executes the job.
            // This protects against any callbacks killing jobs by associated objects and the like.
            getBlazeHub().getScheduler()->removeJob(job, false);

            // Prep the payload
            Blaze::RawBuffer rawBuffer(const_cast<uint8_t *>(data), dataSize);
            rawBuffer.put(dataSize);

            // Here we execute the job by hand after having it decode the response            
            getBlazeHub().getScheduler()->internalStartJobExecuting(job);
            job->handleReply(error, decoder, rawBuffer);
            getBlazeHub().getScheduler()->internalEndJobExecuting(job);

#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
            BlazeSenderStateListener* listener = getBlazeHub().getBlazeSenderStateListener();
            if (listener != nullptr)
                listener->onResponse(dataSize, msgId, component, command, error);
#endif
            // We own the job now, delete it
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job); 
        }
        else
        {
            // Unexpected/unknown RPC Response; log & drop the packet
            logMessage(userIndex, true, false, "drop", nullptr, msgId, component, command, error, dataSize);

#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
            BlazeSenderStateListener* listener = getBlazeHub().getBlazeSenderStateListener();
            if (listener != nullptr)
                listener->onBadResponse(dataSize, msgId, component, command, error);
#endif
        }
    }
    else if (msgType == NOTIFICATION)
    {
        // Hand the notification down to the component manager for processing.
        getComponentManager(userIndex)->handleNotification(component, command, data, dataSize);

#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
        BlazeSenderStateListener* listener = getBlazeHub().getBlazeSenderStateListener();
        if (listener != nullptr)
            listener->onNotification(dataSize, msgId, component, command);
#endif
    }
}

/*! ************************************************************************************************/
/*!
    \brief Write a message to the log

    \param[in] userIndex User index.
    \param[in] inbound True if message was from the server, false if from the client.
    \param[in] prefix Header for the message    
    \param[in] payload  The payload of the message.
    \param[in] msgId The message id for this request (fire frame id)
    \param[in] component the componentId for this request
    \param[in] command the commandId for this request
    \param[in] errorCode Error code (if any)
***************************************************************************************************/
void BlazeSender::logMessage(uint32_t userIndex, bool inbound, bool notification, const char8_t *prefix, const EA::TDF::Tdf *payload,
                             uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode, size_t dataSize) const
{
    logMessage(mHub.getComponentManager(userIndex), inbound, notification, prefix, payload, msgId, 
        componentId, commandId, errorCode, dataSize);
}

bool BlazeSender::prepareReceiveBuffer(size_t size)
{
    if (size > getReceiveBuffer().capacity())
    {
        // This is an overflow packet - allocate our overflow buffer
        if (size <= mOverflowIncomingBufferSize)
        {
            getReceiveBuffer().useOverflow(size);
        }
        else
        {
            BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].prepareReceiveBuffer: Error: incoming packet size "
                << (uint32_t)size << " exceeds overflow incoming buffer size " << mOverflowIncomingBufferSize);

            return false;
        }
    }

    return true;
}

/*! ************************************************************************************************/
/*!
    \brief Write a message to the log

    \param[in] manager ComponentManager.
    \param[in] inbound True if message was from the server, false if from the client.
    \param[in] prefix Header for the message    
    \param[in] payload  The payload of the message.
    \param[in] msgId The message id for this request (fire frame id)
    \param[in] component the componentId for this request
    \param[in] command the commandId for this request
    \param[in] errorCode Error code (if any)
***************************************************************************************************/
void BlazeSender::logMessage(const ComponentManager *manager, bool inbound, bool notification, const char8_t *prefix, const EA::TDF::Tdf *payload,
                             uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode, size_t dataSize)
{
    char8_t errString[256] = {0};
    if (errorCode != ERR_OK)
        blaze_snzprintf(errString, sizeof(errString), ", ERR[%s (0x%X)]", manager ? manager->getErrorName(errorCode) : "", errorCode);

#if ENABLE_BLAZE_SDK_LOGGING
    Debug::logStart();
#endif

    BLAZE_SDK_DEBUGF("%s%s: ID[%i], SIZE[%u], UI[%u], %s::%s [0x%.4X::0x%.4X]%s\n", 
        inbound ? "<- " : "-> ", prefix, msgId, (uint32_t)dataSize,
        manager? manager->getUserIndex() : 0, 
        manager ? manager->getComponentName(componentId) : "",
        manager ? (notification ? manager->getNotificationName(componentId, commandId) 
                                : manager->getCommandName(componentId, commandId)) 
                : "",
        componentId, commandId, errString);

#if ENABLE_BLAZE_SDK_LOGGING
    bool loggingEnabled = false;
    if (manager)
    {
        BlazeHub::TDFLoggingOptions options = manager->getBlazeHub()->getComponentCommandTDFLoggingOptions(componentId, commandId, notification);
        loggingEnabled = inbound ? options.logInbound : options.logOutbound;
    }
    if (payload != nullptr && loggingEnabled && manager->getBlazeHub())
    {
        Blaze::StringBuilder sb;
        payload->print(sb, 1);
        {
            // IMPORTANT: feed the formatted output line by line into SDK_DEBUGF which internally uses a fixed (1KB!) line buffer.
            char8_t* head = const_cast<char8_t*>(sb.get());
            char8_t* lineStart = (*head == '\n') ? head + 1 : head; // omit first newline since we already have a newline in the message header above
            char8_t* lineEnd = lineStart;
            while ((lineEnd = EA::StdC::Strchr(lineStart, '\n')) != nullptr)
            {
                *lineEnd = '\0'; // terminate current line, overwriting the \n
                BLAZE_SDK_DEBUGF("%s\n", lineStart);
                lineStart = lineEnd + 1;
            }
            if (lineStart < (head + sb.length()))
            {
                BLAZE_SDK_DEBUGF("%s", lineStart);
            }
        }
    }

    Debug::logEnd();
#endif
}

} // Blaze