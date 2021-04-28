// Copyright(C) 2020 Electronic Arts, Inc. All rights reserved

#pragma once

#include "RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class LoginRequestV3Job : public RTMRequestJob
{
public:

    using Callback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginResponse, 
                                                      eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError,
                                                      const eadp::foundation::ErrorPtr& error)>;

    LoginRequestV3Job(eadp::foundation::IHub* hub,
                    eadp::foundation::SharedPtr<Internal::RTMService> rtmService, 
                    const eadp::foundation::String& productId,
                    const eadp::foundation::String& clientVersion, // identify the source of the request (Juno web, Juno client, Origin client, etc.)
                    const eadp::foundation::String& accessToken,
                    bool isReconnect, 
                    bool forceDisconnect, 
                    const eadp::foundation::String& forceDisconnectSessionKey,
                    const eadp::foundation::String& reconnectSessionKey,
                    const Callback& loginCallback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;

private:
    void onValidationError(const eadp::foundation::String& message);

    Callback m_callback;
    eadp::foundation::String m_productId;
    eadp::foundation::String m_clientVersion;
    eadp::foundation::String m_accessToken;
    bool m_isReconnect;
    bool m_forceDisconnect;
    eadp::foundation::String m_forceDisconnectSessionKey;
    eadp::foundation::String m_reconnectSessionKey;
};
}
}
