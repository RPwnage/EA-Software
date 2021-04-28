// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ChatService.h>
#include "rtm/RTMRequestJob.h"

namespace eadp
{
namespace realtimemessaging
{
class FetchChannelHistoryRequestJob : public RTMRequestJob
{
public:
    FetchChannelHistoryRequestJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService,
        const eadp::foundation::String& channelId,
        uint32_t count,
        const eadp::foundation::SharedPtr<eadp::foundation::Timestamp> timestamp,
        const eadp::foundation::String& primaryFilterType,
        const eadp::foundation::String& secondaryFilterType,
        ChatService::FetchMessagesCallback callback);

    void setupOutgoingCommunication(const eadp::foundation::String& requestId) override;
    void onTimeout() override;
    void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> communication) override;
    void onSendError(const eadp::foundation::ErrorPtr& error) override;
private:
    ChatService::FetchMessagesCallback m_callback;
    eadp::foundation::String m_channelId;
    uint32_t m_count;
    const eadp::foundation::SharedPtr<eadp::foundation::Timestamp> m_timestamp;
    eadp::foundation::String m_primaryFilterType;
    eadp::foundation::String m_secondaryFilterType;
};
}
}
