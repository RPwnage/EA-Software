// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ServerConfigRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace 
{ 
constexpr char kServerConfigRequestJobName[] = "ServerConfigRequestJob";
using ServerConfigResponse = ResponseT<const SharedPtr<rtm::protocol::ServerConfigV1>, const ErrorPtr&>;
}

ServerConfigRequestJob::ServerConfigRequestJob(IHub* hub,
                                               SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                               ServerConfigCallback callback)
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(callback)
{
}

void ServerConfigRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    auto allocator = getHub()->getAllocator(); 
    SharedPtr<rtm::protocol::CommunicationV1> v1 = allocator.makeShared<rtm::protocol::CommunicationV1>(allocator);
    v1->setRequestId(requestId);
    v1->setServerConfigRequest(allocator.makeShared<rtm::protocol::ServerConfigRequestV1>(allocator));

    SharedPtr<rtm::protocol::Communication> communication = allocator.makeShared<rtm::protocol::Communication>(allocator);
    communication->setV1(v1);

    m_communication = allocator.makeShared<CommunicationWrapper>(communication);
}

void ServerConfigRequestJob::onTimeout()
{
    auto allocator = getHub()->getAllocator();
    String message = allocator.make<String>("Server Config Request timed out.");
    EADPSDK_LOGE(getHub(), kServerConfigRequestJobName, message);
    auto error = RealTimeMessagingError::create(allocator, RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);
    ServerConfigResponse::post(getHub(), kServerConfigRequestJobName, m_callback, nullptr, error);
}

void ServerConfigRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either server config or error response
        || (!rtmCommunication->getV1()->hasServerConfig() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("Server Config Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kServerConfigRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ServerConfigResponse::post(getHub(), kServerConfigRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasServerConfig())
    {
        EADPSDK_LOGV(getHub(), kServerConfigRequestJobName, "Server config request successful");
        ServerConfigResponse::post(getHub(), kServerConfigRequestJobName, m_callback, v1->getServerConfig(), nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Server Config request.");
        if (v1->getError()->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", v1->getError()->getErrorCode().c_str());
        }
        if (v1->getError()->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", v1->getError()->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kServerConfigRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ServerConfigResponse::post(getHub(), kServerConfigRequestJobName, m_callback, nullptr, error);
    }
}

void ServerConfigRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ServerConfigResponse::post(getHub(), kServerConfigRequestJobName, m_callback, nullptr, error);
}
