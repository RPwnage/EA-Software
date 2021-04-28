// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/realtimemessaging/NotificationService.h>
#include "rtm/RTMService.h"

namespace eadp
{
namespace realtimemessaging
{
class NotificationServiceImpl : public NotificationService
{
public:
    explicit NotificationServiceImpl(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);
    virtual ~NotificationServiceImpl();

    eadp::foundation::IHub* getHub() const override;
    eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService, NotificationCallback notificationCallback) override;

private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
    eadp::foundation::SharedPtr<eadp::foundation::Callback::Persistence> m_persistenceObject;

};
}
}

