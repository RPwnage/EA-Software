// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include "rtm/RTMService.h"
#include <eadp/realtimemessaging/ChatService.h>
#include <eathread/eathread_futex.h>
#if EADPSDK_USE_STD_STL
#include <atomic>
#else
#include <eathread/eathread_atomic.h>
#endif

namespace eadp
{
namespace realtimemessaging
{
class ChatServiceImpl : public ChatService
{
public:
    explicit ChatServiceImpl(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);
    virtual ~ChatServiceImpl();

    eadp::foundation::IHub* getHub() const override; 
    eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService) override;
    void setChannelMessageCallback(ChannelMessageCallback callback) override;
    void setChatInviteCallback(ChatInviteCallback callback) override;
    void setDirectMessageCallback(DirectMessageCallback callback) override; 
    void fetchChannelList(ChatChannelsCallback callback) override;
    void fetchChannelList(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> chatChannelsRequest, ChatChannelsCallback callback) override; 
    void fetchPreferences(PreferenceCallback callback) override;
    void fetchMessageHistory(const eadp::foundation::String& channelId, uint32_t count, FetchMessagesCallback callback, eadp::foundation::UniquePtr<eadp::foundation::Timestamp> timestamp, const eadp::foundation::String& primaryFilterType, const eadp::foundation::String& secondaryFilterType) override;
    void fetchMembers(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> chatMembersRequest, ChatMembersCallback chatMemberscallback) override;
    void initiateChat(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> chatInitiateRequest, InitiateChatCallback initiateChatCallback) override;
    void updateTranslationPreference(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> translationPreference, ErrorCallback errorCallback) override;
    void subscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::SubscribeRequest> subscribeRequest, ErrorCallback errorCallback) override;
    void unsubscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> unsubscribeRequest, ErrorCallback errorCallback) override;
    void sendMessageToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::PublishTextRequest> publishTextRequest, ErrorCallback errorCallback) override;
    void sendMessage(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message, ErrorCallback errorCallback) override;
    void leaveChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> chatLeaveRequest, ErrorCallback errorCallback) override;
    double getComposingTimeInterval() override;
    void setComposingTimeInterval(double interval) override;
    void sendTypingIndicator(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> typingIndicatorRequest, ErrorCallback errorCallback) override;
    double getChannelUpdateInterval() override;
    void setChannelUpdateInterval(double interval) override;
    void sendChannelUpdate(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> channelUpdateRequest, ErrorCallback errorCallback) override;
private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
    eadp::foundation::SharedPtr<eadp::foundation::Callback::Persistence> m_persistenceObject; 
    // Channel ID - Time Maps.
    // Used to keep track of rate limits for each channel.
    eadp::foundation::HashStringMap<uint64_t> m_typingIndicatorTimerMap; 
    eadp::foundation::HashStringMap<uint64_t> m_channelUpdateTimerMap; 
    EA::Thread::Futex m_typingIndicatorFutex;
    EA::Thread::Futex m_channelUpdateFutex; 
    // Server rate limits set during initialization.
    // Retrieved from the backend using the ServerConfig request.
    double m_typingEventLimit;
    double m_channelUpdateLimit; 
    // Integrator defined rate limits. 
    // Values may be greater than the server limits. 
    double m_composingIntervalInSeconds;
    double m_channelUpdateIntervalInSeconds;
    bool m_disableRateLimiting;
};
}
}

