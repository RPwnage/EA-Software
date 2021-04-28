/*************************************************************************************************/
/*!
    \file   grpcutils.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#ifndef GRPC_UTILS_H
#define GRPC_UTILS_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"

#include <grpc/grpc.h>
#include <grpc/support/log.h>
#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/support/async_stream.h>

namespace Blaze
{
namespace Grpc
{

static const int64_t SECONDS_TO_MICROSECONDS = 1000 * 1000;

enum RpcType
{
    RPC_TYPE_INVALID = 0,
    RPC_TYPE_UNARY,
    RPC_TYPE_SERVER_STREAMING,
    RPC_TYPE_CLIENT_STREAMING,
    RPC_TYPE_BIDI_STREAMING
};

enum AsyncOpType
{
    ASYNC_OP_TYPE_INIT = 0,
    ASYNC_OP_TYPE_READ,
    ASYNC_OP_TYPE_WRITE,
    ASYNC_OP_TYPE_WRITESDONE,
    ASYNC_OP_TYPE_FINISH,
    
    // Sentinel value - do not use. Add more Ops types above this line.
    ASYNC_OP_TYPE_COUNT 
};

extern const char* sAsyncOpStartedMsg[];
extern const char* sAsyncOpEndedMsg[];

class InboundRpcBase;

// We add a 'TagProcessor' to the completion queue for each event. This way, each tag knows how to process itself. 
using TagProcessor = std::function<void(bool)>;
struct TagInfo
{
    TagProcessor* tagProcessor; // The function to be called to process incoming event

    // Explanation of 'ok' bool - https://groups.google.com/forum/#!topic/grpc-io/ywATt88Ef_I 
    // Note that 'ok' being false is not necessarily an error but sometimes it is so meaning depends on the context. 
    // Relevant bits from above link:
    //+
    //Server - side request an RPC : 
    // ok indicates that the RPC has indeed been started.If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

    //Client - side start an RPC : 
    // ok indicates that the RPC is going to go to the wire.If it is false, it not going to the wire.This would happen if the channel is either permanently broken or transiently broken but with the fail - fast option.

    //Client - side Write, 
    //Client - side WritesDone, 
    //Server - side Write, 
    //Server - side Finish, 
    //Server - side SendInitialMetadata(which is typically included in Write or Finish when not done explicitly) 
    // ok means that the data / metadata / status / etc is going to go to the wire.If it is false, it not going to the wire because the call is already dead(i.e., canceled, deadline expired, other side dropped the channel, etc).

    //Client - side Read, 
    //Server - side Read, 
    //Client - side RecvInitialMetadata(which is typically included in Read if not done explicitly) : 
    // ok indicates whether there is a valid message that got read.If not, you know that there are certainly no more messages that can ever be read from this stream.

    //Client - side Finish : 
    // ok should always be true

    //Server - side AsyncNotifyWhenDone : 
    // ok should always be true
    //-



    // So for common scenarios,  
    // If you submitted a 'read' operation and 'ok' was false, read operation did not happen and will not happen in future either. So for client streaming scenarios(ClientStreamingRpc and BidirectionalStreamingRpc),
    // it means client is done streaming it's requests.

    // If you submitted a 'write' operation and 'ok' was false, write operation failed and no future writes would ever return true (so it is futile to try and write again). There are many things that can cause
    // this. For example, a client going dead or simply cancel the rpc before server could respond.  

    // If you submitted a 'write' operation and 'ok' was true, it does not necessarily mean that client saw your response. All it means is that the response was saved by the grpc library or kernel in their own
    // buffer for future processing. This is not surprising but worth pointing out.
    bool ok;

    int64_t fiberPriority;
};

using TagList = std::list<TagInfo>;



// Entry point for the rpc processing
void processIncomingRequestHandler(InboundRpcBase& rpc, const google::protobuf::Message* request);
void grpcCoreSetupAllocationCallbacks();
void readFile(const char8_t* filePath, grpc::string& data);


} // namespace Grpc
} // namespace Blaze

#endif // GRPC_UTILS_H
