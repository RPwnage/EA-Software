// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class UpdateTranslationPreferenceRequestJob : public RTMRequestJob
{
public:
    UpdateTranslationPreferenceRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> translationPreference,
        ChatService::ErrorCallback callback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> m_translationPreference;
    ChatService::ErrorCallback m_callback;
};
}
}
