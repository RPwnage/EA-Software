// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class ChatMembersRequestJob : public RTMRequestJob
{
public:
    ChatMembersRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> chatMembersRequest,
        ChatService::ChatMembersCallback callback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> m_chatMembersRequest;
    ChatService::ChatMembersCallback m_callback;
};
}
}
