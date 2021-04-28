// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class ServerConfigRequestJob : public RTMRequestJob
{
public:
    using ServerConfigCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1> config, 
                                                                  const eadp::foundation::ErrorPtr& error)>;

    ServerConfigRequestJob(eadp::foundation::IHub* hub,
                           eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
                           ServerConfigCallback callback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ServerConfigCallback m_callback;
};
}
}
