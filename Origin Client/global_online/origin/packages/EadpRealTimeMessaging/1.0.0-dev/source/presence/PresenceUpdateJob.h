// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include "rtm/RTMCommunicationJob.h"
#include <eadp/realtimemessaging/PresenceService.h>

namespace eadp
{
namespace realtimemessaging
{
class PresenceUpdateJob : public RTMCommunicationJob
{
public:
    PresenceUpdateJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<Internal::RTMService> rtmService, 
        const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> request,
        PresenceService::ErrorCallback callback);

    void onSendError(const eadp::foundation::ErrorPtr& error) override;
    void setupOutgoingCommunication() override;

private:
    PresenceService::ErrorCallback m_errorCallback;
    const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> m_request;
};
}
}