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

    // only process ipv4 clients until rest of the code base understands ipv6
    Blaze::InetAddress peerAddr = rpc.getInetAddressFromServerContext();
    if (!peerAddr.isValid())
    {
        auto peer = rpc.getServerContext()->peer();
        BLAZE_WARN_LOG(Log::GRPC, "processIncomingRequestHandler: ignored incoming " << rpc.getName() << " rpc as we do not understand peer ip address " << peer.c_str() << ". Most likely ipv6.");
        rpc.finishWithError(grpc::Status(grpc::INTERNAL, "Unsupported client address."));
        return;
    }

    // Bit of a quirk here
    // If trust setting is configured, honor it. If trust setting is not configured, isTrustedAddress returns true...in order to provide same behavior as legacy endpoints.
    // Can't check rpc.getIsTrustedEnvoyClient() here because the rpc has not been initialized yet.
    // That's okay because Envoy requests will come from the local ingress Envoy node (with a trusted address).
    if (!rpc.getEndpoint().isTrustedAddress(peerAddr))
    {
        rpc.getEndpoint().getTrustedAddressAlertInfo().increment();
        if (rpc.getEndpoint().getTrustedAddressAlertInfo().shouldLog())
        {
            BLAZE_WARN_LOG(Log::GRPC, "processIncomingRequestHandler: dropping rpc from untrusted peer("
                << peerAddr.getIpAsString() << ") on endpoint '" << rpc.getEndpoint().getName() << "'; " << rpc.getEndpoint().getTrustedAddressAlertInfo().getCount() << " others also rejected.");
            rpc.getEndpoint().getTrustedAddressAlertInfo().reset();
        }

        // This could be a security attack. Send a general error code and message.
        rpc.finishWithError(grpc::Status(grpc::INTERNAL, "Unsupported operation."));
        return;
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
            // The client provided a session key but the instance knows nothing about it. Most likely coming via user session migration. Go ahead and create a request handler. 
            // If the session key is bogus, it'll be rejected later.
            requestHandler = rpc.getEndpoint().createRequestHandler();
        }
    }
    else
    {
        requestHandler = rpc.getEndpoint().createRequestHandler();
    }

    if (requestHandler)
        requestHandler->processMessage(rpc, message);
    else
        rpc.finishWithError(grpc::Status(grpc::ABORTED, "Can't handle request....rpc aborted."));
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

}
}

