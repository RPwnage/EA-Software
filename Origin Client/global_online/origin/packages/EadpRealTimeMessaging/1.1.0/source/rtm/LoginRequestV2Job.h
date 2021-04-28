// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved

#pragma once

#include "RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class LoginRequestV2Job : public RTMRequestJob
{
public:

    using Callback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginSuccess, 
                                                      const eadp::foundation::ErrorPtr& error)>;

    LoginRequestV2Job(eadp::foundation::IHub* hub,
                    eadp::foundation::SharedPtr<Internal::RTMService> rtmService, 
                    const eadp::foundation::String& productId, 
                    const eadp::foundation::String& accessToken, 
                    bool isReconnect, 
                    bool forceDisconnect,
                    Callback loginCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;

private:
    Callback m_callback;
    eadp::foundation::String m_productId;
    eadp::foundation::String m_accessToken;
    bool m_isReconnect;
    bool m_forceDisconnect;
};
}
}
