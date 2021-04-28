// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "NotificationServiceImpl.h"
#include "rtm/RTMService.h"
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/internal/ResponseT.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;

using NotificationResponse = ResponseT<const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1>>;
constexpr char kNotificationResponseJobName[] = "NotificationResponseJob";

eadp::foundation::SharedPtr<NotificationService> NotificationService::createService(SharedPtr<IHub> hub)
{
    if (!hub)
    {
        EA_FAIL_MESSAGE("NotificationService: hub is NULL, could not create NotificationService");
        return nullptr;
    }

    EADPSDK_LOGV(hub.get(), "NotificationService", "createService method invoked");
#if defined(EADP_MESSAGING_VERSION)
    EADPSDK_LOG_VERSION(hub.get(), "NotificationService", EADP_MESSAGING_VERSION);
#endif
    auto service = hub->getAllocator().makeShared<NotificationServiceImpl>(EADPSDK_MOVE(hub));
    return service;
}

NotificationServiceImpl::NotificationServiceImpl(SharedPtr<IHub> hub) 
: m_hub(hub)
, m_rtmService(nullptr)
, m_persistenceObject(hub->getAllocator().makeShared<Callback::Persistence>())
{
}

NotificationServiceImpl::~NotificationServiceImpl()
{
}

IHub* NotificationServiceImpl::getHub() const
{
    return m_hub.get(); 
}

ErrorPtr NotificationServiceImpl::initialize(SharedPtr<ConnectionService> connectionService, NotificationCallback notificationCallback)
{
    if (!connectionService || !connectionService->getRTMService())
    {
        return FoundationError::create(m_hub->getAllocator(), FoundationError::Code::INVALID_STATUS, "Initialization failed. Ensure that ConnectionService was initialized");
    }
    m_rtmService = connectionService->getRTMService();

    auto hub = m_hub; 
    auto rtmNotificationCallback = [notificationCallback, hub](eadp::foundation::SharedPtr<CommunicationWrapper> communication)
    {
        if (communication 
            && communication->hasRtmCommunication()
            && communication->getRtmCommunication()->hasV1() 
            && communication->getRtmCommunication()->getV1()->hasNotification()
            && communication->getRtmCommunication()->getV1()->getNotification()->getType() != "CHAT_INVITE") // Chat Notifications delivered through the Chat Service
        {
            NotificationResponse::post(hub.get(), kNotificationResponseJobName, notificationCallback, communication->getRtmCommunication()->getV1()->getNotification());
        }
    };

    m_rtmService->addRTMCommunicationCallback(eadp::realtimemessaging::Internal::RTMService::RTMCommunicationCallback(rtmNotificationCallback, m_persistenceObject, Callback::kIdleInvokeGroupId));
    return nullptr;
}
