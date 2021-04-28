// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include "rtm/RTMCommunicationJob.h"
#include <eadp/realtimemessaging/ChatService.h>

namespace eadp
{
namespace realtimemessaging
{
class TypingIndicatorCommunicationJob : public RTMCommunicationJob
{
public:
    TypingIndicatorCommunicationJob(eadp::foundation::IHub* hub,
                                    eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
                                    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> request,
                                    ChatService::ErrorCallback errorCallback);

    void onSendError(const eadp::foundation::ErrorPtr& error) override;
    void setupOutgoingCommunication() override;

private:
    ChatService::ErrorCallback m_callback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> m_request;
};
}
}