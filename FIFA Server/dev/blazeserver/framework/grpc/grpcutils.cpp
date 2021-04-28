/*************************************************************************************************/
/*!
\file   grpcutils.cpp

\attention
    (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/grpc/grpcutils.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "framework/grpc/inboundgrpcrequesthandler.h"
#include "framework/logger.h"

#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <grpc/support/alloc.h>

namespace Blaze
{
namespace Grpc
{

const char* sAsyncOpStartedMsg[] =
{
    "InitOp started.",
    "ReadOp started.",
    "WriteOp started.",
    "WritesDoneOp started.",
    "FinishOp started.",
};

const char* sAsyncOpEndedMsg[] =
{
    "InitOp ended.",
    "ReadOp ended.",
    "WriteOp ended.",
    "WritesDoneOp ended.",
    "FinishOp ended.",
};

static_assert(ASYNC_OP_TYPE_COUNT == EAArrayCount(sAsyncOpStartedMsg), "If you see this compile error, make sure AsyncOpType and sAsyncOpStartedMsg are in sync.");
static_assert(ASYNC_OP_TYPE_COUNT == EAArrayCount(sAsyncOpEndedMsg), "If you see this compile error, make sure AsyncOpType and sAsyncOpEndedMsg are in sync.");


//These methods define the memory handlers grpc core will use
static void *grpc_core_malloc_callback(size_t size)
{
    return BLAZE_ALLOC_MGID(size, MEM_GROUP_FRAMEWORK_GRPC_CORE, "GrpcCoreMalloc");
}

static void *grpc_core_realloc_callback(void *ptr, size_t size)
{
    return BLAZE_REALLOC_MGID(ptr, size, MEM_GROUP_FRAMEWORK_GRPC_CORE, "GrpcCoreRealloc");
}

static void grpc_core_free_callback(void *ptr)
{
    BLAZE_FREE(ptr);
}

void grpcCoreSetupAllocationCallbacks()
{
    gpr_allocation_functions allocCallbacks;

    allocCallbacks.malloc_fn = &grpc_core_malloc_callback;
    allocCallbacks.realloc_fn = &grpc_core_realloc_callback;
    allocCallbacks.free_fn = &grpc_core_free_callback;
    allocCallbacks.zalloc_fn = nullptr; // Intentional. Grpc deals with it internally.

    gpr_set_allocation_functions(allocCallbacks);
}


void processIncomingRequestHandler(InboundRpcBase& rpc, const google::protobuf::Message* message)
{
    EA_ASSERT_MSG(rpc.getServerContext() != nullptr, "This function should only be passed an InboundRpcBase that has a valid grpc::ServerContext.");

    const EA::TDF::Tdf* requestTdf = rpc.getCommandInfo().requestProtobufToTdf ? rpc.getCommandInfo().requestProtobufToTdf(message) : nullptr;
    rpc.logRpcPreamble(requestTdf);

    // only process ipv4 clients until rest of the code base understands ipv6
    Blaze::InetAddress peerAddr = rpc.getInetAddressFromServerContext();
    if (!peerAddr.isValid())
    {
        auto peer = rpc.getServerContext()->peer();
        BLAZE_WARN_LOG(Log::GRPC, "processIncomingRequestHandler: ignored incoming " << rpc.getName() << " rpc as we do not understand peer ip address " << peer.c_str() << ". Most likely ipv6.");
        rpc.finishWithError(grpc::Status(grpc::INTERNAL, "Unsupported client address."));
        return;
    }

    // Access to an internal endpoint (BIND_INTERNAL) is enforced by either
    // the internal NIC (for on-prem) or internal port allocation (on BitC).
    // In addition to that restriction... Does this endpoint restrict the IP address of clients?
    // Note that isAllowedIpAddress returns true if the endpoint's allowedIpList config is empty.
    if (!rpc.getEndpoint().isAllowedIpListEmpty())
    {
        if (!rpc.getEndpoint().isAllowedIpAddress(peerAddr))
        {
            rpc.getEndpoint().getAllowedIpAddressAlertInfo().increment();
            if (rpc.getEndpoint().getAllowedIpAddressAlertInfo().shouldLog())
            {
                BLAZE_WARN_LOG(Log::GRPC, "processIncomingRequestHandler: dropping rpc from untrusted peer("
                    << peerAddr.getIpAsString() << ") on endpoint '" << rpc.getEndpoint().getName() << "'; " << rpc.getEndpoint().getAllowedIpAddressAlertInfo().getCount() << " others also rejected.");
                rpc.getEndpoint().getAllowedIpAddressAlertInfo().reset();
            }

            // This could be a security attack. Send a general error code and message.
            rpc.finishWithError(grpc::Status(grpc::INTERNAL, "Unsupported operation."));
            return;
        }
    }

    InboundGrpcRequestHandler* requestHandler = nullptr;

    // Check if this rpc belongs to an existing request handler/session
    FixedString64 sessionKey;
    auto& requestMetadata = rpc.getServerContext()->client_metadata();
    {
        auto it = requestMetadata.find("x-blaze-session");
        if (it != requestMetadata.end())
        {
            sessionKey.assign((*it).second.data(), (*it).second.length());
        }
    }

    if (!sessionKey.empty())
    {
        requestHandler = rpc.getEndpoint().getRequestHandler(sessionKey.c_str());
        if (requestHandler == nullptr)
        {
            // The client provided a session key but the instance knows nothing about it. 
            // Could be shutting down or Most likely coming via user session migration. Go ahead and create a request handler. 
            // If shutting down, following will fail as well. If the session key is bogus, it'll be rejected later.
            requestHandler = rpc.getEndpoint().createRequestHandler();
        }
    }
    else
    {
        requestHandler = rpc.getEndpoint().createRequestHandler();
    }

    // Make sure that request handler is not already shutdown and just waiting to be cleaned up.
    // If the key is expired but the handler is not shutdown, the rpc will fail. 
    if (requestHandler && !requestHandler->isShutdown()) 
        requestHandler->processMessage(rpc, message);
    else
    {
        BLAZE_WARN_LOG(Log::GRPC, "processIncomingRequestHandler: ignored incoming " << rpc.getName() << " rpc as a request handler can't be created.");
        rpc.finishWithError(grpc::Status(grpc::ABORTED, "Can't handle request....rpc aborted."));
    }
}


void readFile(const char8_t* filePath, grpc::string& data)
{
    if (filePath == nullptr || filePath[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcUtils].readFile: file (" << filePath << ") was not specified. Unable to load.");
        return;
    }

    EA::IO::FileStream fileStream(filePath);

    if (!fileStream.Open())
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcUtils].readFile: Unable to open file (" << filePath << ")");
        return;
    }

    auto fileSize = EA::IO::File::GetSize(filePath);

    data.resize(fileSize);
    fileStream.Read((void*)(data.data()), fileSize);

    fileStream.Close();
}

const char8_t* gRPCStatusCodeToString(grpc::StatusCode code)
{
    using namespace grpc;

    switch (code)
    {
    case OK:
        return "OK";
    case CANCELLED:
        return "CANCELLED";
    case UNKNOWN:
        return "UNKNOWN";
    case INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case DEADLINE_EXCEEDED:
        return "DEADLINE_EXCEEDED";
    case NOT_FOUND:
        return "NOT_FOUND";
    case ALREADY_EXISTS:
        return "ALREADY_EXISTS";
    case PERMISSION_DENIED:
        return "PERMISSION_DENIED";
    case UNAUTHENTICATED:
        return "UNAUTHENTICATED";
    case RESOURCE_EXHAUSTED:
        return "RESOURCE_EXHAUSTED";
    case FAILED_PRECONDITION:
        return "FAILED_PRECONDITION";
    case ABORTED:
        return "ABORTED";
    case UNIMPLEMENTED:
        return "UNIMPLEMENTED";
    case INTERNAL:
        return "INTERNAL";
    case UNAVAILABLE:
        return "UNAVAILABLE";
    case DATA_LOSS:
        return "DATA_LOSS";

    default:
        return "IMPLEMENT_STRING_CONVERSION";
    }
}

// https://groups.google.com/forum/#!msg/grpc-io/X_bUx3T8S7s/x38FU429CgAJ
// Map the Blaze system errors to standard grpc errors. Obviously, they don't have a one-to-one mapping so the custom message string
// can be used to identify the details. If there turns out to be a real need for all the Blaze system errors, we'll need to send
// them as metadata. Until then, we send unmapped errors as UNKNOWN errors.
grpc::Status getGrpcStatusFromBlazeError(BlazeRpcError err)
{
    switch (err)
    {
    case ERR_OK:
        return grpc::Status(grpc::OK, ErrorHelp::getErrorDescription(ERR_OK));

    case ERR_SYSTEM:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_SYSTEM));

    case ERR_COMPONENT_NOT_FOUND:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_COMPONENT_NOT_FOUND));

    case ERR_COMMAND_NOT_FOUND:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_COMMAND_NOT_FOUND));

    case ERR_AUTHENTICATION_REQUIRED:
        return grpc::Status(grpc::UNAUTHENTICATED, ErrorHelp::getErrorDescription(ERR_AUTHENTICATION_REQUIRED));

    case ERR_TIMEOUT:
        return grpc::Status(grpc::DEADLINE_EXCEEDED, ErrorHelp::getErrorDescription(ERR_TIMEOUT));

    case ERR_DISCONNECTED:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DISCONNECTED));

    case ERR_DUPLICATE_LOGIN:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DUPLICATE_LOGIN));

    case ERR_AUTHORIZATION_REQUIRED:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_AUTHORIZATION_REQUIRED));

    case ERR_CANCELED:
        return grpc::Status(grpc::CANCELLED, ErrorHelp::getErrorDescription(ERR_CANCELED));

    case ERR_CUSTOM_REQUEST_HOOK_FAILED:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_CUSTOM_REQUEST_HOOK_FAILED));

    case ERR_CUSTOM_RESPONSE_HOOK_FAILED:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_CUSTOM_RESPONSE_HOOK_FAILED));

    case ERR_TDF_STRING_TOO_LONG:
        return grpc::Status(grpc::OUT_OF_RANGE, ErrorHelp::getErrorDescription(ERR_TDF_STRING_TOO_LONG));

    case ERR_INVALID_TDF_ENUM_VALUE:
        return grpc::Status(grpc::OUT_OF_RANGE, ErrorHelp::getErrorDescription(ERR_INVALID_TDF_ENUM_VALUE));

    case ERR_MOVED:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_MOVED));

    case ERR_COMMAND_FIBERS_MAXED_OUT:
        return grpc::Status(grpc::RESOURCE_EXHAUSTED, ErrorHelp::getErrorDescription(ERR_COMMAND_FIBERS_MAXED_OUT));

    case ERR_INVALID_ENDPOINT:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_INVALID_ENDPOINT));

    case ERR_DB_SYSTEM:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_SYSTEM));

    case ERR_DB_NOT_CONNECTED:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_DB_NOT_CONNECTED));

    case ERR_DB_NOT_SUPPORTED:
        return grpc::Status(grpc::UNIMPLEMENTED, ErrorHelp::getErrorDescription(ERR_DB_NOT_SUPPORTED));

    case ERR_DB_NO_CONNECTION_AVAILABLE:
        return grpc::Status(grpc::RESOURCE_EXHAUSTED, ErrorHelp::getErrorDescription(ERR_DB_NO_CONNECTION_AVAILABLE));

    case ERR_DB_DUP_ENTRY:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DB_DUP_ENTRY));

    case ERR_DB_NO_SUCH_TABLE:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_DB_NO_SUCH_TABLE));

    case ERR_DB_TIMEOUT:
        return grpc::Status(grpc::DEADLINE_EXCEEDED, ErrorHelp::getErrorDescription(ERR_DB_TIMEOUT));

    case ERR_DB_DISCONNECTED:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_DISCONNECTED));

    case ERR_DB_INIT_FAILED:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_DB_INIT_FAILED));

    case ERR_DB_TRANSACTION_NOT_COMPLETE:
        return grpc::Status(grpc::ABORTED, ErrorHelp::getErrorDescription(ERR_DB_TRANSACTION_NOT_COMPLETE));

    case ERR_DB_LOCK_DEADLOCK:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_DB_LOCK_DEADLOCK));

    case ERR_DB_DROP_PARTITION_NON_EXISTENT:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_DB_DROP_PARTITION_NON_EXISTENT));

    case ERR_DB_SAME_NAME_PARTITION:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DB_SAME_NAME_PARTITION));

    case ERR_SERVER_BUSY:
        return grpc::Status(grpc::UNAVAILABLE, ErrorHelp::getErrorDescription(ERR_SERVER_BUSY));

    case ERR_GUEST_SESSION_NOT_ALLOWED:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_GUEST_SESSION_NOT_ALLOWED));

    case ERR_DB_USER_DEFINED_EXCEPTION:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_USER_DEFINED_EXCEPTION));

    case ERR_COULDNT_CONNECT:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_COULDNT_CONNECT));

    case ERR_DB_PREVIOUS_TRANSACTION_IN_PROGRESS:
        return grpc::Status(grpc::ABORTED, ErrorHelp::getErrorDescription(ERR_DB_PREVIOUS_TRANSACTION_IN_PROGRESS));

    case ERR_UNAVAILABLE:
        return grpc::Status(grpc::UNAVAILABLE, ErrorHelp::getErrorDescription(ERR_UNAVAILABLE));

    case ERR_SERVICE_INTERNAL_ERROR:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_SERVICE_INTERNAL_ERROR));

    case ERR_COULDNT_RESOLVE_HOST:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_COULDNT_RESOLVE_HOST));

    default:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_SYSTEM));
    }
}

}
}

