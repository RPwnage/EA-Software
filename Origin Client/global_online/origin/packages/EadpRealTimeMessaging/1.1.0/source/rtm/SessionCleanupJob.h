// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved

#pragma once

#include "RTMCommunicationJob.h"

namespace eadp
{
namespace realtimemessaging
{
class SessionCleanupJob : public RTMCommunicationJob
{
public:

    using Callback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error)>;

    SessionCleanupJob(eadp::foundation::IHub* hub,
                     eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
                     const eadp::foundation::String& sessionKey,
                     Callback callback);

    void setupOutgoingCommunication() override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;

private:
    eadp::foundation::String m_sessionKey;
    Callback m_callback; 
};
}
}
