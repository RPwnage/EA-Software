// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "PresenceServiceImpl.h"
#include "PresenceUpdateJob.h"
#include "PresenceUnsubscribeJob.h"
#include "PresenceSubscribeJob.h"
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation; 
using namespace eadp::realtimemessaging; 

using PresenceUpdateResponse = ResponseT<SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1>>;
constexpr char kPresenceUpdateResponseJobName[] = "PresenceUpdateResponseJob";

using PresenceSubscriptionErrorResponse = ResponseT<SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1>>;
constexpr char kPresenceSubscriptionErrorResponseJobName[] = "PresenceSubscriptionErrorResponseJob";

using PresenceUpdateErrorResponse = ResponseT<SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1>>;
constexpr char kPresenceUpdateErrorResponseJobName[] = "PresenceUpdateErrorResponseJob";

SharedPtr<PresenceService> PresenceService::createService(SharedPtr<IHub> hub)
{
    if (!hub)
    {
        EA_FAIL_MESSAGE("PresenceService: hub is NULL, could not create PresenceService");
        return nullptr;
    }

    EADPSDK_LOGV(hub.get(), "PresenceService", "createService method invoked");
#if defined(EADP_MESSAGING_VERSION)
    EADPSDK_LOG_VERSION(hub.get(), "PresenceService", EADP_MESSAGING_VERSION);
#endif
    auto service = hub->getAllocator().makeShared<PresenceServiceImpl>(EADPSDK_MOVE(hub));
    return service;
}

PresenceServiceImpl::PresenceServiceImpl(SharedPtr<IHub> hub)
: m_hub(hub)
, m_rtmService(nullptr)
, m_persistenceObject(hub->getAllocator().makeShared<Callback::Persistence>())
{
}

PresenceServiceImpl::~PresenceServiceImpl()
{
}

ErrorPtr PresenceServiceImpl::initialize(SharedPtr<ConnectionService> connectionService, 
                                         PresenceUpdateCallback presenceUpdateCallback,
                                         PresenceSubscriptionErrorCallback presenceSubscriptionErrorCallback,
                                         PresenceUpdateErrorCallback presenceUpdateErrorCallback)
{
    // Connection Service is null
    if (!connectionService)
    {
        const char* reason = "Initialization failed. The ConnectionService is null."; 
        EADPSDK_LOGE(m_hub.get(), "PresenceService", reason);
        return RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SERVICE_NOT_CONNECTED, reason);
    }
    
    m_rtmService = connectionService->getRTMService();

    // RTM Service is empty
    if (!m_rtmService)
    {
        const char* reason = "Initialization failed. Ensure the RTM Service is initialized.";
        EADPSDK_LOGE(m_hub.get(), "PresenceService", reason);
        return RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SERVICE_NOT_CONNECTED, reason);
    }

    auto hub = m_hub; 

    // Presence Update Callback Wrapper
    auto rtmPresenceUpdateCallback = [presenceUpdateCallback, hub](SharedPtr<CommunicationWrapper> communicationWrapper)
    {
        if (communicationWrapper 
            && communicationWrapper->hasRtmCommunication()
            && communicationWrapper->getRtmCommunication()->hasV1()
            && communicationWrapper->getRtmCommunication()->getV1()->hasPresence())
        {
            PresenceUpdateResponse::post(hub.get(), kPresenceUpdateResponseJobName, presenceUpdateCallback, communicationWrapper->getRtmCommunication()->getV1()->getPresence());
        }
    }; 

    // Subscription Error Callback Wrapper
    auto rtmSubscriptionErrorCallback = [presenceSubscriptionErrorCallback, hub](SharedPtr<CommunicationWrapper> communicationWrapper)
    {
        if (communicationWrapper 
            && communicationWrapper->hasRtmCommunication()
            && communicationWrapper->getRtmCommunication()->hasV1()
            && communicationWrapper->getRtmCommunication()->getV1()->hasPresenceSubscriptionError())
        {
            PresenceSubscriptionErrorResponse::post(hub.get(), kPresenceSubscriptionErrorResponseJobName, presenceSubscriptionErrorCallback, communicationWrapper->getRtmCommunication()->getV1()->getPresenceSubscriptionError());
        }
    };

    // Presence Status Update Error Callback Wrapper
    auto rtmStatusUpdateErrorCallback = [presenceUpdateErrorCallback, hub](SharedPtr<CommunicationWrapper> communicationWrapper)
    {
        if (communicationWrapper 
            && communicationWrapper->hasRtmCommunication()
            && communicationWrapper->getRtmCommunication()->hasV1()
            && communicationWrapper->getRtmCommunication()->getV1()->hasPresenceUpdateError())
        {
            PresenceUpdateErrorResponse::post(hub.get(), kPresenceUpdateErrorResponseJobName, presenceUpdateErrorCallback, communicationWrapper->getRtmCommunication()->getV1()->getPresenceUpdateError());
        }
    };

    
    // Register Callback Wrappers on RTM Service
    m_rtmService->addRTMCommunicationCallback(eadp::realtimemessaging::Internal::RTMService::RTMCommunicationCallback(rtmPresenceUpdateCallback, m_persistenceObject, presenceUpdateCallback.getGroupId()));
    m_rtmService->addRTMCommunicationCallback(eadp::realtimemessaging::Internal::RTMService::RTMCommunicationCallback(rtmSubscriptionErrorCallback, m_persistenceObject, presenceSubscriptionErrorCallback.getGroupId()));
    m_rtmService->addRTMCommunicationCallback(eadp::realtimemessaging::Internal::RTMService::RTMCommunicationCallback(rtmStatusUpdateErrorCallback, m_persistenceObject, presenceUpdateErrorCallback.getGroupId()));

    return nullptr; 
}

void PresenceServiceImpl::updateStatus(UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> presenceUpdate, ErrorCallback errorCallback)
{
    auto updateStatusJob = m_hub->getAllocator().makeUnique<PresenceUpdateJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(presenceUpdate), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(updateStatusJob));
}

void PresenceServiceImpl::subscribe(UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> presenceSubscribeRequest, ErrorCallback errorCallback)
{
    auto subscribeJob = m_hub->getAllocator().makeUnique<PresenceSubscribeJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(presenceSubscribeRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(subscribeJob));
}

void PresenceServiceImpl::unsubscribe(UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> presenceUnsubscribeRequest, ErrorCallback errorCallback)
{
    auto unsubscribeJob = m_hub->getAllocator().makeUnique<PresenceUnsubscribeJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(presenceUnsubscribeRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(unsubscribeJob));
}