// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "ChatServiceImpl.h"
#include "ChatChannelsRequestJob.h"
#include "ChatInitiateRequestJob.h"
#include "ChannelSubscribeRequestJob.h"
#include "ChannelUnsubscribeRequestJob.h"
#include "ChatMembersRequestJob.h"
#include "ChatLeaveRequestJob.h"
#include "DirectMessageRequestJob.h"
#include "FetchChannelHistoryRequestJob.h"
#include "FetchPreferencesRequestJob.h"
#include "PublishTextRequestJob.h"
#include "TypingIndicatorCommunicationJob.h"
#include "UpdateTranslationPreferenceRequestJob.h"
#include "rtm/RTMService.h"
#include <EAJson/JsonReader.h>
#include <DirtySDK/platform.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;

using ChannelMessageResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage>>;
constexpr char kChannelMessageResponseJobName[] = "ChannelMessageResponseJob";

using ChatInviteResponse = ResponseT<const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1>>;
constexpr char kChatInviteResponseJobName[] = "ChatInviteResponseJob";

using DirectMessageResponse = ResponseT<const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1>>;
constexpr char kDirectMessageResponseJobName[] = "DirectMessageResponseJob";

constexpr char kTypingIndicatorResponseJobName[] = "TypingIndicatorResponseJob";

/*!
 * @brief How often to send composing events in seconds (X value).
 */
constexpr uint32_t kDefaultTypingIndicatorComposingInterval = 3;


eadp::foundation::SharedPtr<ChatService> ChatService::createService(SharedPtr<IHub> hub)
{
    if (!hub)
    {
        EA_FAIL_MESSAGE("ChatService: hub is NULL, could not create ChatService");
        return nullptr;
    }

    EADPSDK_LOGV(hub.get(), "ChatService", "createService method invoked");
#if defined(EADP_MESSAGING_VERSION)
    EADPSDK_LOG_VERSION(hub.get(), "ChatService", EADP_MESSAGING_VERSION);
#endif
    auto service = hub->getAllocator().makeShared<ChatServiceImpl>(EADPSDK_MOVE(hub));
    return service;
}

ChatServiceImpl::ChatServiceImpl(SharedPtr<IHub> hub) 
: m_hub(hub)
, m_rtmService(nullptr)
, m_persistenceObject(hub->getAllocator().makeShared<Callback::Persistence>())
, m_typingIndicatorTimerMap(hub->getAllocator().make<HashStringMap<uint64_t>>())
, m_typingIndicatorFutex()
, m_composingIntervalInSeconds(kDefaultTypingIndicatorComposingInterval)
{
}

ChatServiceImpl::~ChatServiceImpl()
{
}

ErrorPtr ChatServiceImpl::initialize(SharedPtr<ConnectionService> connectionService)
{
    if (!connectionService || !connectionService->getRTMService())
    {
        return FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "Initialization failed. Ensure that ConnectionService was initialized");
    }
    m_rtmService = connectionService->getRTMService();

#if EA_DEBUG
    auto hub = m_hub;
    auto rateLimitLogCallback = [hub](SharedPtr<CommunicationWrapper> communication)
    {
        if (communication
            && communication->hasRtmCommunication()
            && communication->getRtmCommunication()->hasV1()
            && communication->getRtmCommunication()->getV1()->hasError()
            && communication->getRtmCommunication()->getV1()->getError()->hasChatTypingEventRequestError())
        {
            auto error = communication->getRtmCommunication()->getV1()->getError()->getChatTypingEventRequestError(); 
            EADPSDK_LOGE(hub.get(), "ChatService", "Received rate limit error from the server for channel id: %s", error->getChannelId().c_str());
        }
    };

    m_rtmService->addRTMCommunicationCallback(Internal::RTMService::RTMCommunicationCallback(rateLimitLogCallback, m_persistenceObject, Callback::kIdleInvokeGroupId));
#endif 

    return nullptr;
}

void ChatServiceImpl::setChannelMessageCallback(ChannelMessageCallback callback)
{
    auto hub = m_hub;
    auto rtmCommunicationCallback = [callback, hub](SharedPtr<CommunicationWrapper> communication)
    {
        if (communication 
            && communication->hasSocialCommunication()
            && communication->getSocialCommunication()->hasHeader()
            && communication->getSocialCommunication()->getHeader()->getType() == com::ea::eadp::antelope::protocol::CommunicationType::CHANNEL_MESSAGE)
        {
            ChannelMessageResponse::post(hub.get(), kChannelMessageResponseJobName, callback, communication->getSocialCommunication()->getChannelMessage());
        }
    };

    m_rtmService->addRTMCommunicationCallback(Internal::RTMService::RTMCommunicationCallback(rtmCommunicationCallback, m_persistenceObject, Callback::kIdleInvokeGroupId));
}

void ChatServiceImpl::setChatInviteCallback(ChatInviteCallback callback)
{
    auto hub = m_hub;
    auto rtmCommunicationCallback = [callback, hub](SharedPtr<CommunicationWrapper> communication)
    {
        if (communication 
            && communication->hasRtmCommunication()
            && communication->getRtmCommunication()->hasV1()
            && communication->getRtmCommunication()->getV1()->hasNotification()
            && communication->getRtmCommunication()->getV1()->getNotification()->getType() == "CHAT_INVITE")
        {
            ChatInviteResponse::post(hub.get(), kChatInviteResponseJobName, callback, communication->getRtmCommunication()->getV1()->getNotification());
        }
    };

    m_rtmService->addRTMCommunicationCallback(Internal::RTMService::RTMCommunicationCallback(rtmCommunicationCallback, m_persistenceObject, Callback::kIdleInvokeGroupId));
}

void ChatServiceImpl::setDirectMessageCallback(DirectMessageCallback callback)
{
    auto hub = m_hub;
    auto rtmCommunicationCallback = [callback, hub](SharedPtr<CommunicationWrapper> communication)
    {
        if (communication 
            && communication->hasRtmCommunication()
            && communication->getRtmCommunication()->hasV1()
            && communication->getRtmCommunication()->getV1()->hasPointToPointMessage())
        {
            DirectMessageResponse::post(hub.get(), kDirectMessageResponseJobName, callback, communication->getRtmCommunication()->getV1()->getPointToPointMessage());
        }
    };

    m_rtmService->addRTMCommunicationCallback(Internal::RTMService::RTMCommunicationCallback(rtmCommunicationCallback, m_persistenceObject, Callback::kIdleInvokeGroupId));
}

void ChatServiceImpl::fetchChannelList(ChatChannelsCallback callback)
{
    auto chatChannelsRequestJob = m_hub->getAllocator().makeUnique<ChatChannelsRequestJob>(m_hub.get(), m_rtmService, callback);
    m_hub->addJob(EADPSDK_MOVE(chatChannelsRequestJob));
}

void ChatServiceImpl::fetchPreferences(PreferenceCallback callback)
{
    auto fetchPreferencesRequestJob = m_hub->getAllocator().makeUnique<FetchPreferencesRequestJob>(m_hub.get(), m_rtmService, callback);
    m_hub->addJob(EADPSDK_MOVE(fetchPreferencesRequestJob));
}

void ChatServiceImpl::fetchMessageHistory(const String& channelId, uint32_t count, FetchMessagesCallback callback, UniquePtr<eadp::foundation::Timestamp> timestamp)
{
    auto fetchChannelHistoryRequestJob = m_hub->getAllocator().makeUnique<FetchChannelHistoryRequestJob>(m_hub.get(), m_rtmService, channelId, count, EADPSDK_MOVE(timestamp), callback);
    m_hub->addJob(EADPSDK_MOVE(fetchChannelHistoryRequestJob));
}

void ChatServiceImpl::fetchMembers(UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> chatMembersRequest, ChatMembersCallback chatMemberscallback)
{
    auto chatMembersRequestJob = m_hub->getAllocator().makeUnique<ChatMembersRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(chatMembersRequest), chatMemberscallback);
    m_hub->addJob(EADPSDK_MOVE(chatMembersRequestJob));
}

void ChatServiceImpl::initiateChat(UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> chatInitiateRequest, InitiateChatCallback initiateChatCallback)
{
    auto chatInitiateRequestJob = m_hub->getAllocator().makeUnique<ChatInitiateRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(chatInitiateRequest), initiateChatCallback);
    m_hub->addJob(EADPSDK_MOVE(chatInitiateRequestJob));
}

void ChatServiceImpl::updateTranslationPreference(UniquePtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> translationPreference, ErrorCallback errorCallback)
{
    auto updateTranslationPreferenceRequestJob = m_hub->getAllocator().makeUnique<UpdateTranslationPreferenceRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(translationPreference), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(updateTranslationPreferenceRequestJob));
}

void ChatServiceImpl::subscribeToChannel(UniquePtr<com::ea::eadp::antelope::protocol::SubscribeRequest> subscribeRequest, ErrorCallback errorCallback)
{
    auto subscribeRequestJob = m_hub->getAllocator().makeUnique<ChannelSubscribeRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(subscribeRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(subscribeRequestJob));
}

void ChatServiceImpl::unsubscribeToChannel(UniquePtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> unsubscribeRequest, ErrorCallback errorCallback)
{
    auto unsubscribeRequestJob = m_hub->getAllocator().makeUnique<ChannelUnsubscribeRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(unsubscribeRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(unsubscribeRequestJob));
}

void ChatServiceImpl::sendMessageToChannel(UniquePtr<com::ea::eadp::antelope::protocol::PublishTextRequest> publishTextRequest, ErrorCallback errorCallback)
{
    auto publishTextRequestJob = m_hub->getAllocator().makeUnique<PublishTextRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(publishTextRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(publishTextRequestJob));
}

void ChatServiceImpl::sendMessage(UniquePtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message, ErrorCallback errorCallback)
{
    auto publishTextRequestJob = m_hub->getAllocator().makeUnique<DirectMessageRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(message), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(publishTextRequestJob));
}

void ChatServiceImpl::leaveChannel(UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> chatLeaveRequest, ErrorCallback errorCallback)
{
    auto chatLeaveRequestJob = m_hub->getAllocator().makeUnique<ChatLeaveRequestJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(chatLeaveRequest), errorCallback);
    m_hub->addJob(EADPSDK_MOVE(chatLeaveRequestJob));
}

uint32_t ChatServiceImpl::getComposingTimeInterval()
{
    return m_composingIntervalInSeconds;
}

void ChatServiceImpl::setComposingTimeInterval(uint32_t interval)
{
    m_composingIntervalInSeconds = interval < kDefaultTypingIndicatorComposingInterval ? kDefaultTypingIndicatorComposingInterval : interval;
}

void ChatServiceImpl::sendTypingIndicator(UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> typingIndicatorRequest, ErrorCallback errorCallback)
{
    // Invalid argument error 
    if (!typingIndicatorRequest || typingIndicatorRequest->getChannelId().empty())
    {
        String message = m_hub->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(m_hub.get(), "SendTypingIndicatorRequest", message);
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ErrorResponse::post(m_hub.get(), kTypingIndicatorResponseJobName, errorCallback, error);
        return; 
    }

    uint64_t currentTime = ds_timeinsecs();
    auto channelId = typingIndicatorRequest->getChannelId();

    EA::Thread::AutoFutex lock(m_typingIndicatorFutex);
    auto channelIdTimerSearch = m_typingIndicatorTimerMap.find(channelId);
    if (channelIdTimerSearch == m_typingIndicatorTimerMap.end())
    {
        // Set time and send request
        m_typingIndicatorTimerMap.emplace(channelId, currentTime);
        auto typingIndicatorJob = m_hub->getAllocator().makeUnique<TypingIndicatorCommunicationJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(typingIndicatorRequest), errorCallback);
        m_hub->addJob(EADPSDK_MOVE(typingIndicatorJob));
    }
    else
    {
        // Existing Channel Id 
        uint64_t lastSendTime = channelIdTimerSearch->second;
        if (m_composingIntervalInSeconds > (currentTime - lastSendTime))
        {
            // Request sent too quickly 
            auto message = m_hub->getAllocator().make<String>("Attempting to send a typing indicator too quickly.");
            EADPSDK_LOGE(m_hub.get(), "ChatService", message);
            auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::RATE_LIMIT_EXCEEDED, message);
            ErrorResponse::post(m_hub.get(), kTypingIndicatorResponseJobName, errorCallback, error); 
        }
        else
        {
            // Update time and send request
            m_typingIndicatorTimerMap[channelId] = currentTime;
            auto typingIndicatorJob = m_hub->getAllocator().makeUnique<TypingIndicatorCommunicationJob>(m_hub.get(), m_rtmService, EADPSDK_MOVE(typingIndicatorRequest), errorCallback);
            m_hub->addJob(EADPSDK_MOVE(typingIndicatorJob));
        }
    }
}