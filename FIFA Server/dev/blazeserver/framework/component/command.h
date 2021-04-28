/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMMAND_H
#define BLAZE_COMMAND_H

/*** Include files *******************************************************************************/
#include <EASTL/intrusive_list.h>
#include "framework/component/blazerpc.h"
#include "framework/component/message.h"
#include "framework/component/componentstub.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/system/fiber.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"
#include "blazerpcerrors.h"
#include "EATDF/time.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace google
{
namespace protobuf
{
class Message;
class Any;
}

namespace rpc
{
class Status;
}
}

namespace Blaze
{


class Component;
class PeerInfo;

class Command
{
    NON_COPYABLE(Command)
public:
    static const size_t INVALID_COMMAND_INDEX = UINT64_MAX;

public:
    Command(const CommandInfo& commandInfo, const Message* message);
    virtual ~Command();

    BlazeRpcError executeCommand(ClientPlatformType callerPlatform);
    ComponentStub* getComponentStub() const;
    ComponentId getComponentId() const { return mCommandInfo.componentInfo->id; }
    CommandId getCommandId() const { return mCommandInfo.commandId; }
    size_t getCommandIndex() const { return mCommandInfo.index; }
    const char8_t* getContextString() const { return mCommandInfo.context; }
    
    virtual Fiber::StackSize getFiberStackSize() const { return mCommandInfo.fiberStackSize; }
    virtual bool isBlocking() { return mCommandInfo.isBlocking; }

    const char8_t* getComponentNameForLogging() const { return mCommandInfo.componentInfo->loggingName; }
    const char8_t* getCommandNameForLogging() const { return mCommandInfo.loggingName; }
    int32_t getLogCategory() const { return mCommandInfo.componentInfo->baseInfo.index; }
    bool getRequiresAuthentication() const { return mCommandInfo.requiresAuthentication; }
    bool getRequiresUserSession() const { return mCommandInfo.requiresUserSession; }

    uint32_t getMsgNum() const { return mMessage != nullptr ? mMessage->getMsgNum() : 0; }

    virtual EA::TDF::Tdf* getRequest() { return nullptr; }
    virtual EA::TDF::Tdf* getResponse() { return nullptr; }
    virtual EA::TDF::Tdf* getErrorResponse() { return nullptr; }

    SlaveSessionId getSlaveSessionId() const { return mSlaveSessionId; }
    UserSessionId getUserSessionId() const { return mUserSessionId; }
    
    // message can be nullptr in a case where one component calls another "local" component's RPC. In turn, PeerInfo
    // can be nullptr as well. 
    PeerInfo* getPeerInfo() const { return mMessage != nullptr ? &(mMessage->getPeerInfo()) : nullptr; }

    uint32_t getConnectionUserIndex() const { return mConnectionUserIndex; } 
    
    template <class ComponentType, class CommandType, class RequestType>
    static Command* createCommand(Message* msg, EA::TDF::Tdf* request, Component* component);

    template <class ComponentType, class CommandType>
    static Command* createCommandNoRequest(Message* msg, EA::TDF::Tdf* request, Component* component);  
    
    template<class ObjectType>
    static const EA::TDF::Tdf* protobufToTdfCast(const google::protobuf::Message* message);

    template<class ObjectType>
    static google::protobuf::Message* tdfToProtobufCast(const EA::TDF::Tdf* tdf);

    // Unlike legacy RPC Framework, Grpc always sends a response and the response type is always the same. 
    // So we marshal the specific tdf response object into the Grpc Response object using the 4 helpers below.
    
    // Command Success
    template<typename GrpcResponse, typename ResponseTdf>
    static bool sendGrpcResponse_ResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, const EA::TDF::Tdf* tdf);
    
    template<typename GrpcResponse>
    static bool sendGrpcResponse_NoResponseTdf(Blaze::Grpc::InboundRpcBase& rpc);

    // Command failure
    template<typename GrpcResponse, typename ErrorResponseTdf>
    static bool sendGrpcResponse_ErrorResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, BlazeRpcError err, const EA::TDF::Tdf* tdf);

    template<typename GrpcResponse>
    static bool sendGrpcResponse_NoErrorResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, BlazeRpcError err);


protected:
    uint64_t getContext() const { return mMessage != nullptr ? mMessage->getFrame().context : 0; }    

    void tickMetrics(BlazeRpcError err);

    virtual bool executeRequestHook(BlazeRpcError& err) { return true; }
    virtual void executeResponseHook(BlazeRpcError& err) {}

    const CommandInfo& mCommandInfo;
    const Message* mMessage;
    RawBufferPtr mRawResponseBuffer;
    EA::TDF::TimeValue mStartTime;
private:
    virtual BlazeRpcError executeInternal() = 0;
        
    SlaveSessionId mSlaveSessionId;
    UserSessionId mUserSessionId;
    uint32_t mConnectionUserIndex;    

    static void populateGrpcStatusFromErrCode(google::rpc::Status& status, BlazeRpcError err);
    static void convertTdfToAny(const EA::TDF::Tdf* tdf, google::protobuf::Any* any); // need to have an utility method due to circular dependencies in blaze.h
};


template <class ComponentType, class CommandType, class RequestType>
Command* Command::createCommand(Message* msg, EA::TDF::Tdf* request, Component* component)
{
    return CommandType::create(msg, static_cast<RequestType*>(request), static_cast<ComponentType*>(component));
}

template <class ComponentType, class CommandType>
Command* Command::createCommandNoRequest(Message* msg, EA::TDF::Tdf*, Component* component)
{
    return CommandType::create(msg, static_cast<ComponentType*>(component));
}

template <class ObjectType>
const EA::TDF::Tdf* Command::protobufToTdfCast(const google::protobuf::Message* message)
{
    return static_cast<const ObjectType*>(message);
}

template <class ObjectType>
google::protobuf::Message* Command::tdfToProtobufCast(const EA::TDF::Tdf* tdf)
{
    return static_cast<ObjectType*>(const_cast<EA::TDF::Tdf*>(tdf));
}


template<typename GrpcResponse, typename TdfResponse>
bool Command::sendGrpcResponse_ResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, const EA::TDF::Tdf* tdf)
{
    GrpcResponse grpcResponse;

    populateGrpcStatusFromErrCode(grpcResponse.getStatus(), ERR_OK);

    if (tdf != nullptr)
    {
        grpcResponse.setResponse(*(static_cast<const TdfResponse*>(tdf)));
    }

    return rpc.sendResponse(&grpcResponse);
}

template<typename GrpcResponse>
bool Command::sendGrpcResponse_NoResponseTdf(Blaze::Grpc::InboundRpcBase& rpc)
{
    GrpcResponse grpcResponse;
    populateGrpcStatusFromErrCode(grpcResponse.getStatus(), ERR_OK);
    return rpc.sendResponse(&grpcResponse);
}

template<typename GrpcResponse, typename TdfErrorResponse>
bool Command::sendGrpcResponse_ErrorResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, BlazeRpcError err, const EA::TDF::Tdf* tdf)
{
    GrpcResponse grpcResponse;

    populateGrpcStatusFromErrCode(grpcResponse.getStatus(), err);
    if (tdf != nullptr)
    {
        convertTdfToAny(tdf, grpcResponse.getStatus().add_details());
    }

    return rpc.sendResponse(&grpcResponse);
}

template<typename GrpcResponse>
bool Command::sendGrpcResponse_NoErrorResponseTdf(Blaze::Grpc::InboundRpcBase& rpc, BlazeRpcError err)
{
    GrpcResponse grpcResponse;
    populateGrpcStatusFromErrCode(grpcResponse.getStatus(), err);
    return rpc.sendResponse(&grpcResponse);
}


template <class ComponentStubClass, const CommandInfo& CMD_INFO>
class NotificationSubscribeCommand : public Command
{
public:
    static Command* create(Message* msg, ComponentStub* comp) { return BLAZE_NEW NotificationSubscribeCommand<ComponentStubClass, CMD_INFO>(msg, *comp); }

    NotificationSubscribeCommand(Message* message, ComponentStub& component)
        :   Command(CMD_INFO, message),
        mComponentStub(component)
    {
    }    
    
protected:
    BlazeRpcError executeInternal() override 
    { 
        mComponentStub.addNotificationSubscription(*InboundRpcConnection::smCurrentSlaveSession);
        return ERR_OK;
    }

private:

    ComponentStub& mComponentStub;
};

template <class ComponentStubClass, const CommandInfo& CMD_INFO>
class NotificationSubscribeGrpcCommand : public Command
{
public:
    static Command* create(Message* msg, ComponentStub* comp) { return BLAZE_NEW NotificationSubscribeGrpcCommand<ComponentStubClass, CMD_INFO>(msg, *comp); }

    NotificationSubscribeGrpcCommand(Message* message, ComponentStub& component)
        : Command(CMD_INFO, message),
        mComponentStub(component)
    {
    }

protected:
    virtual BlazeRpcError executeInternal()
    {
        return ERR_OK;
    }

private:

    ComponentStub& mComponentStub;
};


template <class ComponentStubClass, const CommandInfo& CMD_INFO>
class ReplicationSubscribeCommand : public Command
{
public:
    static Command* create(Message* msg, ReplicationSubscribeRequest* request, ComponentStubClass* comp) { return BLAZE_NEW ReplicationSubscribeCommand<ComponentStubClass, CMD_INFO>(msg, request, comp); }

    ReplicationSubscribeCommand(Message* message, ReplicationSubscribeRequest* request, ComponentStubClass* component)
        :   Command(CMD_INFO, message),
        mRequest(*request),
        mComponentStub(*component)
    {
    }

protected:
    BlazeRpcError executeInternal() override
    {
        mComponentStub.addReplicationSubscription(*InboundRpcConnection::smCurrentSlaveSession, mRequest);
        return ERR_OK;
    }

private:
    ReplicationSubscribeRequest& mRequest;
    ComponentStubClass& mComponentStub;
};

template <class ComponentStubClass, const CommandInfo& CMD_INFO>
class ReplicationUnsubscribeCommand : public Command
{
public:
    static Command* create(Message* msg, ComponentStubClass* comp) { return BLAZE_NEW ReplicationUnsubscribeCommand<ComponentStubClass, CMD_INFO>(msg, comp); }

    ReplicationUnsubscribeCommand(Message* message, ComponentStubClass* component)
        :   Command(CMD_INFO, message),
        mComponentStub(*component)
    {
    }

protected:
    BlazeRpcError executeInternal() override
    {
        mComponentStub.removeReplicationSubscription(*InboundRpcConnection::smCurrentSlaveSession);
        return ERR_OK;
    }

private:

    ComponentStubClass& mComponentStub;
};


} // Blaze

#endif // BLAZE_COMMAND_H

