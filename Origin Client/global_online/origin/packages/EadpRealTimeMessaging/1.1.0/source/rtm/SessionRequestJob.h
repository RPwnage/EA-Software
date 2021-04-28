// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved

#pragma once

#include "RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class SessionRequestJob : public RTMRequestJob
{
public:

    using Callback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> sessionResponse,
        const eadp::foundation::ErrorPtr& error)>;

    SessionRequestJob(eadp::foundation::IHub* hub,
                     eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
                     Callback callback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;

private:
    Callback m_callback; 
};
}
}
