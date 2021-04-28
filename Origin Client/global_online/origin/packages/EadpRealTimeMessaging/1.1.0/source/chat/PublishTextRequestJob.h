// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class PublishTextRequestJob : public RTMRequestJob
{
public:
    PublishTextRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> publishTextRequest,
        ChatService::ErrorCallback errorCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ChatService::ErrorCallback m_callback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> m_publishTextRequest;
};
}
}
