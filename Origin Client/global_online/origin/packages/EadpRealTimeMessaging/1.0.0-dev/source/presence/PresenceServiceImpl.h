// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/realtimemessaging/PresenceService.h>

namespace eadp
{

namespace realtimemessaging
{

class EADPREALTIMEMESSAGING_API PresenceServiceImpl : public PresenceService
{
public:

    explicit PresenceServiceImpl(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);
    ~PresenceServiceImpl();
    eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService,
                                          PresenceUpdateCallback presenceUpdateCallback,
                                          PresenceSubscriptionErrorCallback presenceSubscriptionErrorCallback,
                                          PresenceUpdateErrorCallback presenceUpdateErrorCallback) override;
    void updateStatus(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> presenceUpdate, ErrorCallback errorCallback) override;
    void subscribe(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> presenceSubscribeRequest, ErrorCallback errorCallback) override;
    void unsubscribe(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> presenceUnsubscribeRequest, ErrorCallback errorCallback) override;

private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
    eadp::foundation::SharedPtr<eadp::foundation::Callback::Persistence> m_persistenceObject;

};

}
}
