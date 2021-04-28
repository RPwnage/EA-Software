// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class ChannelSubscribeRequestJob : public RTMRequestJob
{
public:
    ChannelSubscribeRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> subscribeRequest,
        ChatService::ErrorCallback errorCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ChatService::ErrorCallback m_callback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> m_subscribeRequest;
};
}
}
