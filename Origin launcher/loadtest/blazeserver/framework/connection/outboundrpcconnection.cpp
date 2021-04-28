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
#include "framework/connection/outboundrpcconnection.h"

#include "framework/connection/socketchannel.h"
#include "framework/connection/selector.h"

#include "framework/protocol/rpcprotocol.h"
#include "framework/protocol/shared/fireframe.h"

#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"

#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/protocol/shared/tdfencoder.h"

#include "blazerpcerrors.h"
#include "framework/component/blazerpc.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

OutboundRpcConnection::OutboundRpcConnection(ConnectionOwner& owner, ConnectionId ident, SocketChannel& channel, Blaze::Endpoint& endpoint) :
    Connection(owner, ident, channel, endpoint), mInstanceId(INVALID_INSTANCE_ID),
    mSentRequests(BlazeStlAllocator("OutboundRpcConnection::mSentRequests")),
    mContextOverride(0)
{
    getChannel().enablePriorityChannel();
}

OutboundRpcConnection::~OutboundRpcConnection()
{
}

void OutboundRpcConnection::processMessage(RawBufferPtr& buffer)
{
    RpcProtocol::Frame inFrame;
    getProtocol().process(*buffer.get(), inFrame, getDecoder());

    BLAZE_TRACE_LOG (Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].processMessage: <- Received message "
            "component=" << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId) << "(" << SbFormats::Raw("0x%04x", inFrame.componentId) 
            << ") command=" << ((inFrame.messageType == RpcProtocol::NOTIFICATION) ? "" : BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId))
            << "(" << inFrame.commandId << ") msgNum=" << inFrame.msgNum << " type=" << inFrame.messageType << " ec=" <<inFrame.errorCode
            << " size=" << static_cast<int32_t> (buffer->datasize()) );

    switch (inFrame.messageType)
    {
        case RpcProtocol::REPLY:
        case RpcProtocol::ERROR_REPLY:
        {
            RequestsBySeqno::iterator find = mSentRequests.find(inFrame.msgNum);
            if (find != mSentRequests.end())
            {
                OutgoingRequest &requestInfo = find->second;
                               
                if (!requestInfo.ignoreReply)
                {
                    // fiber will cleans up the OutgoingRequest object
                    requestInfo.complete = true;
                    requestInfo.responseBuf = buffer;
                    requestInfo.respErrorCode = inFrame.errorCode;
                    requestInfo.movedTo = inFrame.movedTo;
                    Fiber::signal(requestInfo.eventHandle, ERR_OK);              
                }
                else
                {
                    BLAZE_TRACE_RPC_LOG(Log::CONNECTION, "[OutboundRpcConnection:" 
                        << ConnectionSbFormat(this) 
                        << "].processMessage: <- Received ignored response component=" << BlazeRpcComponentDb::getComponentNameById(requestInfo.component) <<"("
                        << SbFormats::Raw("0x%04x", requestInfo.component) <<") command="<< BlazeRpcComponentDb::getCommandNameById(requestInfo.component, requestInfo.command) <<"("
                        << requestInfo.command <<") seqno="<< inFrame.msgNum <<" ec=" << ErrorHelp::getErrorName(inFrame.errorCode) << "("
                        << inFrame.errorCode <<")");
                    mSentRequests.erase(find);
                }
            }
            else
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[OutboundRpcConnection:" 
                        << ConnectionSbFormat(this) << "].processMessage: <- Missing original request object for received reply component=" 
                        << BlazeRpcComponentDb::getComponentNameById(inFrame.componentId)
                        << "(" << SbFormats::Raw("0x%04x", inFrame.componentId) << ") command="
                        << BlazeRpcComponentDb::getCommandNameById(inFrame.componentId, inFrame.commandId)
                        << "(" << inFrame.commandId << ") msgNum=" << inFrame.msgNum);
            }
            break;
        }

        case RpcProtocol::NOTIFICATION:
        {
            Notification notification(inFrame, buffer);

            Fiber::CreateParams params;
            params.blocking = false;
            const char8_t* contextName = "Notification::executeLocal";
            const NotificationInfo* notificationInfo = ComponentData::getNotificationInfo(notification.getComponentId(), notification.getNotificationId());
            if (notificationInfo != nullptr && notificationInfo->context != nullptr)
                contextName = notificationInfo->context;

            gSelector->scheduleFiberCall(&notification, &Notification::executeLocal, contextName, params);

            break;
        }

        default:
            BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].processMessage: Received unknown message type " 
                          << inFrame.msgNum << ".");
    }
}

BlazeRpcError OutboundRpcConnection::sendRequestNonblocking(ComponentId component, CommandId command, const EA::TDF::Tdf* request, MsgNum& msgNum,
    int32_t logCategory/* = CONNECTION_LOGGER_CATEGORY*/, const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = nullptr*/)
{
    RpcProtocol::Frame outFrame;
    outFrame.messageType = RpcProtocol::MESSAGE;
    outFrame.componentId = component;
    outFrame.commandId = command;   
    outFrame.msgNum = msgNum = getProtocol().getNextSeqno();
    outFrame.sliverIdentity = options.routeTo.getAsSliverIdentity();
    outFrame.superUserPrivilege = UserSession::hasSuperUserPrivilege();
    if (gLogContext != nullptr)
    {
        outFrame.logContext = *gLogContext;
    }

    outFrame.context = getContext();

    RawBufferPtr buffer = getBuffer();
    getProtocol().framePayload(*buffer.get(), outFrame, (int32_t)buffer->datasize(), getEncoder(), request);

    RawBuffer logBuf(0);

    BLAZE_TRACE_RPC_LOG(logCategory, "[OutboundRpcConnection:" 
            <<  ConnectionSbFormat(this)
            << "].sendRequestNonblocking: -> Sending request [component=" << BlazeRpcComponentDb::getComponentNameById(component) << "("
            << SbFormats::Raw("0x%04x", component) <<") command=" << BlazeRpcComponentDb::getCommandNameById(component, command)
            << "(" << command << ") seqno=" << outFrame.msgNum <<"] "
            << ((request != nullptr) ? "\n" : "") << request );

    mContextOverride = 0;
   
    //Send on the request
    return sendRequestInternal(outFrame, buffer, options);
}

BlazeRpcError OutboundRpcConnection::sendRawRequest(ComponentId component, CommandId command, RawBufferPtr& requestBuf,  
    RawBufferPtr& responseBuf, InstanceId& movedTo, int32_t logCategory/* = CONNECTION_LOGGER_CATEGORY*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    RpcProtocol::Frame outFrame;
    outFrame.messageType = RpcProtocol::MESSAGE;
    outFrame.componentId = component;
    outFrame.commandId = command;   
    outFrame.msgNum = getProtocol().getNextSeqno();
    outFrame.sliverIdentity = options.routeTo.getAsSliverIdentity();
    outFrame.superUserPrivilege = UserSession::hasSuperUserPrivilege();
    if (gLogContext != nullptr)
    {
        outFrame.logContext = *gLogContext;
    }
    outFrame.context = getContext();

    getProtocol().framePayload(*requestBuf.get(), outFrame, (int32_t)requestBuf->datasize(), getEncoder());

    BlazeRpcError result = sendRequestInternal(outFrame, requestBuf, options);
    if (result == ERR_OK)
    {
        result = waitForRequestInternal(outFrame.msgNum, component, command, responseBuf, movedTo, options);
    }
    
    return result;
}

BlazeRpcError OutboundRpcConnection::sendRequestInternal(RpcProtocol::Frame& outFrame, RawBufferPtr& requestBuf, const RpcCallOptions &options)
{
    if (isClosed())
        return ERR_DISCONNECTED;
    
    RequestsBySeqno::insert_return_type ret = mSentRequests.insert(outFrame.msgNum);
    OutgoingRequest &requestInfo = ret.first->second;
    requestInfo.component = outFrame.componentId;
    requestInfo.command = outFrame.commandId;   
    requestInfo.ignoreReply = options.ignoreReply;

    //put the payload on the output queue
    queueOutputData(requestBuf);

    BlazeRpcError result = ERR_OK;

    //double check if we closed after queuing, which can happen if this send occurred between the close and taking action on the close
    if (isClosed())
    {
        mSentRequests.erase(outFrame.msgNum);

        BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].sendRequestInternal: Closed when writing output queue.");
        result = ERR_DISCONNECTED;
    }

    return result;
}

BlazeRpcError OutboundRpcConnection::waitForRequest(MsgNum msgNum, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory/* = CONNECTION_LOGGER_CATEGORY*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = nullptr*/)
{    
    if (options.ignoreReply)
    {
        //This would be supremely boneheaded
        EA_FAIL_MESSAGE("Attempted to wait on a request that was sent with ignoreReply set to true.");
        return ERR_SYSTEM;
    }

    ComponentId component = Component::INVALID_COMPONENT_ID;
    CommandId command = Component::INVALID_COMMAND_ID;
    RawBufferPtr respBuf;
    BlazeRpcError result = waitForRequestInternal(msgNum, component, command, respBuf, movedTo, options);

    EA::TDF::Tdf* tdfToDecode = ((result == ERR_OK) ? responseTdf : errorTdf);
    if (tdfToDecode != nullptr)
    {
        BlazeRpcError decodeErr = decodeTdf(respBuf, *tdfToDecode);
        if (decodeErr != ERR_OK)
        {
            BLAZE_ERR_LOG (Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].waitForRequest: <- Failed to decode reply: "
                "component=" << BlazeRpcComponentDb::getComponentNameById(component) << "(" << SbFormats::Raw("0x%04x", component) 
                << ") command=" << BlazeRpcComponentDb::getCommandNameById(component, command)
                << "(" << command << ") msgNum=" << msgNum << " type=" << (uint32_t) ((result == ERR_OK) ? RpcProtocol::REPLY : RpcProtocol::ERROR_REPLY) << " ec=" << result
                << " size=" << static_cast<int32_t> (respBuf->datasize()) << " decodeErr=" << decodeErr << ", Decoder error= " << getDecoderErrorMessage());

            return decodeErr;
        }
    }

    BLAZE_TRACE_RPC_LOG(logCategory, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].waitForRequest: <- Received response [component=" << BlazeRpcComponentDb::getComponentNameById(component) <<"("
        << SbFormats::Raw("0x%04x", component) <<") command="<< BlazeRpcComponentDb::getCommandNameById(component, command) <<"("
        << command <<") seqno="<< msgNum <<" ec=" << ErrorHelp::getErrorName(result) << "("
        << result <<")]" << (tdfToDecode != nullptr ? "\n" : "") 
        << tdfToDecode);

    return result;
}

BlazeRpcError OutboundRpcConnection::waitForRequestInternal(MsgNum msgNum, ComponentId& component, CommandId& command, 
    RawBufferPtr& responseBuf, InstanceId& movedTo, const RpcCallOptions &options)
{
    if (isClosed())
        return ERR_DISCONNECTED;

    BlazeRpcError result = ERR_OK;

    RequestsBySeqno::iterator itr = mSentRequests.find(msgNum);
    if (itr != mSentRequests.end())
    {
        OutgoingRequest& requestInfo = itr->second;
        
        component = requestInfo.component;
        command = requestInfo.command;

        if (!requestInfo.complete)
        {
            //Now we go to sleep - when we wake up result will have been filled out.
            result = Fiber::getAndWait(requestInfo.eventHandle, "OutboundRpcConnection::waitForRequestInternal", options.timeoutOverride > 0 ? options.timeoutOverride + TimeValue::getTimeOfDay() : 0);
        }

        if (result == ERR_OK)
        {
            //if the wait was ERR_OK, get the response & err code.  Otherwise something happened while we were asleep (cancel, timeout)
            //so ignore the err code and response from the request
            if (requestInfo.complete)
            {               
                responseBuf = requestInfo.responseBuf;
                result = requestInfo.respErrorCode;
                movedTo = requestInfo.movedTo;
            }
            else
            {
                //Shouldn't happen
                BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].waitForRequestInternal: Message " << msgNum << " finished waiting but was not complete.");
                result = ERR_SYSTEM;
            }
        }

        //would be nice to reuse the iterator, but we slept so it might be invalid at this point
        mSentRequests.erase(msgNum);
    }
    else
    {
        //shouldn't happen
        BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundRpcConnection:" << ConnectionSbFormat(this) << "].waitForRequestInternal: Message " << msgNum << " does not have a request record.");
        result = ERR_SYSTEM;
    }

    return result;
}

uint64_t OutboundRpcConnection::getContext() const
{
    if (mContextOverride != 0)
        return mContextOverride;

    if ((gController != nullptr) && (gCurrentUserSession != nullptr))
        return gCurrentUserSession->getUserSessionId();

    return INVALID_USER_SESSION_ID;
}

void OutboundRpcConnection::close()
{
    while (!mSentRequests.empty())
    {
        RequestsBySeqno::iterator it = mSentRequests.begin();
        
        //Signal will usually erase the request, so cache the key
        uint32_t msgNum = it->first;

        //Signal rpc requests
        Fiber::signal(it->second.eventHandle, ERR_DISCONNECTED);

        //Erase here to ensure we can't spin forever
        mSentRequests.erase(msgNum);
    }
    
    Connection::close();
}

} // Blaze

