// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class ChatInitiateRequestJob : public RTMRequestJob
{
public:
    ChatInitiateRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> chatInitiateRequest,
        ChatService::InitiateChatCallback initiateChatCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ChatService::InitiateChatCallback m_callback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> m_chatInitiateV1;
};
}
}
