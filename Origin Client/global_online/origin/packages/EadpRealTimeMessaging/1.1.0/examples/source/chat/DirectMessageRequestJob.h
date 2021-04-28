// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include "rtm/RTMRequestJob.h"
#include <eadp/realtimemessaging/ChatService.h>

namespace eadp
{
namespace realtimemessaging
{
class DirectMessageRequestJob : public RTMRequestJob
{
public:
    DirectMessageRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message,
        ChatService::ErrorCallback errorCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ChatService::ErrorCallback m_callback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> m_message;
};
}
}