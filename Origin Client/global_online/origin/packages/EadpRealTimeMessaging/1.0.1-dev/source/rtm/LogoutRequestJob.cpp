// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include "LogoutRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using LogoutResponse = ResponseT<>;
constexpr char kLogoutRequestJobName[] = "LogoutRequestJob";

LogoutRequestJob::LogoutRequestJob(IHub* hub,
                                   SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService, 
                                   Callback callback)
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(EADPSDK_MOVE(callback))
{
}

void LogoutRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    auto allocator = getHub()->getAllocator();
    auto logoutRequest = allocator.makeShared<protocol::LogoutRequest>(allocator);

    auto header = allocator.makeShared<protocol::Header>(allocator);
    header->setType(protocol::CommunicationType::LOGOUT_REQUEST);
    header->setRequestId(requestId);

    auto communication = allocator.makeShared<protocol::Communication>(allocator);
    communication->setHeader(header);
    communication->setLogoutRequest(logoutRequest);

    m_communication = allocator.makeShared<eadp::realtimemessaging::CommunicationWrapper>(communication);
}

void LogoutRequestJob::onTimeout()
{
    EADPSDK_LOGE(getHub(), kLogoutRequestJobName, "Logout Request timed out.");
    LogoutResponse::post(getHub(), kLogoutRequestJobName, m_callback);
}

void LogoutRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either SuccessV1 or error response
        || (!rtmCommunication->getV1()->hasError()
            && !rtmCommunication->getV1()->hasSuccess()))
    {
        EADPSDK_LOGE(getHub(), kLogoutRequestJobName, "Logout Response message received is of incorrect type.");
        LogoutResponse::post(getHub(), kLogoutRequestJobName, m_callback);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess())
    {
        EADPSDK_LOGV(getHub(), kLogoutRequestJobName, "Logout Successful.");
        LogoutResponse::post(getHub(), kLogoutRequestJobName, m_callback);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to logout request.");
        }

        EADPSDK_LOGE(getHub(), kLogoutRequestJobName, message);
        LogoutResponse::post(getHub(), kLogoutRequestJobName, m_callback);
    }
}

void LogoutRequestJob::onSendError(const eadp::foundation::ErrorPtr& /* error */)
{
    LogoutResponse::post(getHub(), kLogoutRequestJobName, m_callback);
}
