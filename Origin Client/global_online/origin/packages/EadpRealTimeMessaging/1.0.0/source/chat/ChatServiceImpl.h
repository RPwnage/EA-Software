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

    eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService) override;
    void setChannelMessageCallback(ChannelMessageCallback callback) override;
    void setChatInviteCallback(ChatInviteCallback callback) override;
    void setDirectMessageCallback(DirectMessageCallback callback) override; 
    void fetchChannelList(ChatChannelsCallback callback) override;
    void fetchPreferences(PreferenceCallback callback) override;
    void fetchMessageHistory(const eadp::foundation::String& channelId, uint32_t count, FetchMessagesCallback callback, eadp::foundation::UniquePtr<eadp::foundation::Timestamp> timestamp) override;
    void fetchMembers(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> chatMembersRequest, ChatMembersCallback chatMemberscallback) override;
    void initiateChat(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> chatInitiateRequest, InitiateChatCallback initiateChatCallback) override;
    void updateTranslationPreference(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> translationPreference, ErrorCallback errorCallback) override;
    void subscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::SubscribeRequest> subscribeRequest, ErrorCallback errorCallback) override;
    void unsubscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> unsubscribeRequest, ErrorCallback errorCallback) override;
    void sendMessageToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::PublishTextRequest> publishTextRequest, ErrorCallback errorCallback) override;
    void sendMessage(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message, ErrorCallback errorCallback) override;
    void leaveChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> chatLeaveRequest, ErrorCallback errorCallback) override;
    uint32_t getComposingTimeInterval() override;
    void setComposingTimeInterval(uint32_t interval) override;
    void sendTypingIndicator(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> typingIndicatorRequest, ErrorCallback errorCallback) override;
private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
    eadp::foundation::SharedPtr<eadp::foundation::Callback::Persistence> m_persistenceObject; 
    eadp::foundation::HashStringMap<uint64_t> m_typingIndicatorTimerMap; 
    EA::Thread::Futex m_typingIndicatorFutex;

#if EADPSDK_USE_STD_STL
    std::atomic_uint32_t m_composingIntervalInSeconds;
#else
    EA::Thread::AtomicUint32 m_composingIntervalInSeconds;
#endif

};
}
}

