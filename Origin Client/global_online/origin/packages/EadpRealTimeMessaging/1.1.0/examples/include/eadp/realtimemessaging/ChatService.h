// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Service.h>
#include <eadp/realtimemessaging/ConnectionService.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateSuccessV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsRequestV2.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatTypingEventRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelUpdateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/GetPreferenceResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/TranslationPreferenceV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatLeaveV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/NotificationV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PointToPointMessageV1.h>
#include <com/ea/eadp/antelope/protocol/ChannelMessage.h>
#include <com/ea/eadp/antelope/protocol/SubscribeRequest.h>
#include <com/ea/eadp/antelope/protocol/UnsubscribeRequest.h>
#include <com/ea/eadp/antelope/protocol/PublishTextRequest.h>

namespace eadp
{
namespace realtimemessaging
{
/*!
 * @brief ChatService is used for accessing features such as Chat, Chat Invitations, Direct Messages, Typing Indicators, etc.
 */
class EADPREALTIMEMESSAGING_API ChatService : public eadp::foundation::IService
{
public:

    /*!
     * @brief This callback is invoked whenever a ChannelMessage is received.
     */
    using ChannelMessageCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage> message)>;

    /*!
     * @brief This callback is invoked in response to ChatMembersRequestV1 with the list of members for the Channel specified in the request or error object if the request failed.
     *
     * @param chatMembers The success response. Null if an error occurred.
     * @param error The error response. Null if the request is successful.
     */
    using ChatMembersCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1> chatMembers, const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief This callback is invoked in response to fetchMessageHistory() call with the list of messages in the channel or error if the request failed.
     *
     * @param channelId The channel id associated with the message history. 
     * @param channelMessages The messages. Null if an error occurred. 
     * @param error The error response. Null if the request is successful.
     */
    using FetchMessagesCallback = eadp::foundation::CallbackT<void(const eadp::foundation::String& channelId,
                                                                   eadp::foundation::UniquePtr<eadp::foundation::Vector<eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage>>> channelMessages,
                                                                   const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief This callback is invoked when the user receives a Chat Invite.
     */
    using ChatInviteCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> chatInvite)>;

    /*!
     * @brief This callback is invoked when a point-to-point message is received from another user.
     */
    using DirectMessageCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message)>;

    /*!
     * @brief Initiate Chat callback
     *
     * @param chatInitiateSuccess The success response. Null if an error occurred. 
     * @param error The error response. Null if the request is successful. 
     */
    using InitiateChatCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1> chatInitiateSuccess, const eadp::foundation::ErrorPtr& error)>;

    /*!
    * @brief Generic callback used in multiple places.
    * 
    * @param error If the error is null, the operation is a success. 
    */
    using ErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief Callback for Chat Channel list.
     *
     * @param chatChannels The success response. Null if an error occurred.
     * @param error The error response. Null if the request is successful.
     */
    using ChatChannelsCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1> chatChannels, const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief Callback for Messaging Preference of the current user.
     *
     * @param preferenceResponse The success response. Null if an error occurred.
     * @param error The error response. Null if the request is successful. 
     */
    using PreferenceCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1> preferenceResponse, const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief Get an instance of the ChatService. 
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub object.
     * @return The ChatService instance
     */
    static eadp::foundation::SharedPtr<ChatService> createService(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);

    /*!
     * @brief Initializes the ChatService object.
     *
     * @param connectionService Requires a ConnectionService object. The connection on ConnectionService should be established.
     */
    virtual eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService) = 0;

    /*!
     * @brief Set the chat invite callback. 
     *
     * @param callback The ChatInviteCallback to be invoked whenever a Chat Invite is received
     */
    virtual void setChatInviteCallback(ChatInviteCallback callback) = 0;

    /*!
     * @brief Set the channel message callback. 
     *
     * @param callback The ChannelMessageCallback to be invoked whenever a Message is received on any User Channel
     */
    virtual void setChannelMessageCallback(ChannelMessageCallback callback) = 0;

    /*!
     * @brief Set the direct message callback for point to point messages. 
     * 
     * @param callback The DirectMessageCallback to be invoked whenever a point to point message is received
     */
    virtual void setDirectMessageCallback(DirectMessageCallback callback) = 0; 

    /*!
    * @brief Fetches RTM preferences such as autoTranslation and profanity filter settings.
    *
    * @param callback Called with the Preferences Response
    */
    virtual void fetchPreferences(PreferenceCallback callback) = 0;

    /*!
     * @brief Retrieves the list of channels of which the user is a member.
     * 
     * The response will include the unread message count and last message in each channel.
     * The response will not include a valid last read timestamp for each channel. 
     * 
     * @param callback The ChatChannelsCallback to be invoked once the channels are available
     */
    virtual void fetchChannelList(ChatChannelsCallback callback) = 0;

    /*!
     * @brief Retrieves the list of channels of which the user is a member.
     * 
     * This method exposes the chat channel request directly to allow for more customized Chat Channel responses. 
     *
     * @param chatChannelsRequest The request object containing information on the expected chat channels to receive. 
     * @param callback The ChatChannelsCallback to be invoked once the channels are available
     */
    virtual void fetchChannelList(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> chatChannelsRequest, ChatChannelsCallback callback) = 0;

    /*!
    * @brief Fetch members for this channel.
    *
    * @param ChatMembersRequestV1 can be used for paging and fetching one page at a time.
    * @param ChatMembersCallback is called with the members response
    */
    virtual void fetchMembers(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> chatMembersRequest, ChatMembersCallback chatMemberscallback) = 0;

    /*!
    * @brief Fetches historical messages before current time(pass nullptr as timestamp) or a specified time from a channel. Optional filter parameters to filter on ChannelMessageType.
    *
    * @param channelId ChannelId of the channel to fetch the messages for
    * @param count Count of messages to be fetched
    * @param callback callback invoked on success/error with the fetched Messages for the channel
    * @param timestamp Timestamp for the time before which to fetch the messages
    * @param primaryFilterType primary filter on ChannelMessageType to filter the messages. This must be one of the ChannelMessageType enums as a String.
    * @param secondaryFilterType secondary filter on ChannelMessageType to filter the messages. This must be one of the ChannelMessageType enums as a String.
    */
    virtual void fetchMessageHistory(
        const eadp::foundation::String& channelId
        , uint32_t count
        , FetchMessagesCallback callback
        , eadp::foundation::UniquePtr<eadp::foundation::Timestamp> timestamp = nullptr
        , const eadp::foundation::String& primaryFilterType = ""
        , const eadp::foundation::String& secondaryFilterType = "") = 0;

    /*!
    * @brief Creates a MessagingChannel to chat with specified users. Channel is returned via a ChatInvitationReceived event.
    *
    * @param chatInitiateRequest Request object to initiate chat
    * @param initiateChatCallback callback invoked on success/error
    */
    virtual void initiateChat(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> chatInitiateRequest, InitiateChatCallback initiateChatCallback) = 0;
    /* @brief Enables or disables the RTM autoTranslate setting. Must specify a destination language when enabling auto translate.
    *
    * @param translationPreference Translation Preference to be updated
    * @param errorCallback callback invoked on success/error
    */
    virtual void updateTranslationPreference(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> translationPreference, ErrorCallback errorCallback) = 0;

    /*!
    * @brief Subscribe to receive messages on this channel.
    *
    * @param subscribeRequest Request to subscribe to channel
    * @param errorCallback callback invoked on success/error
    */
    virtual void subscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::SubscribeRequest> subscribeRequest, ErrorCallback errorCallback) = 0;

    /*!
    * @brief Stop listening to messages for this channel.
    *
    * @param subscribeRequest Request to unsubscribe to channel
    * @param errorCallback callback invoked on success/error
    */
    virtual void unsubscribeToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> unsubscribeRequest, ErrorCallback errorCallback) = 0;

    /*!
    * @brief Send a text message to a given channel.
    *
    * @param publishTextRequest Request to send a message to a channel
    * @param errorCallback callback invoked on success/error
    */
    virtual void sendMessageToChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::protocol::PublishTextRequest> publishTextRequest, ErrorCallback errorCallback) = 0;

    /*!
     * @brief Send a direct message to another user.
     * 
     * @param directMessage the point to point message to send
     * @param errorCallback callback invoked on success/error
     */
    virtual void sendMessage(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> message, ErrorCallback errorCallback) = 0;

    /*!
    * @brief Leave a channel.
    *
    * @param chatLeaveRequest request object to leave the chat channel
    * @param errorCallback callback called on success or error
    */
    virtual void leaveChannel(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> chatLeaveRequest, ErrorCallback errorCallback) = 0;

    /*!
     * @brief Get how frequently composing events are sent (X).
     *
     * @return how often the Chat Service sends composing events in seconds.
     */
    virtual double getComposingTimeInterval() = 0;

    /*!
     * @brief Set how frequently the typing indicator is sent (X).
     *
     * The value may not be lower than the server rate limit.
     * The value is clamped to the server rate limit if `interval` is too small.
     *
     * @param interval how often to send new composing events in seconds.
     */
    virtual void setComposingTimeInterval(double interval) = 0;

    /*!
     * @brief Send a typing indicator to a given channel.
     *
     * The request is throttled and will be sent to the backend every X seconds. 
     * If the method is called before X seconds have elapsed, the error callback
     * is invoked with code RealTimeMessagingError::Code::RATE_LIMIT_EXCEEDED.
     *
     * @param typingIndicatorRequest the typing indicator request to send
     * @param errorCallback callback invoked on success/error.
     */
    virtual void sendTypingIndicator(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> typingIndicatorRequest, ErrorCallback errorCallback) = 0;

    /*!
     * @brief Get how frequently channel updates are sent. 
     *
     * @return how often the Chat Service can send a channel update in seconds. 
     */
    virtual double getChannelUpdateInterval() = 0;
    
    /*!
     * @brief Set how frequently the chat channel updates are sent. 
     *
     * The value may not be lower than the server rate limit. 
     * The value is clamped to the server rate limit if `interval` is too small.
     *
     * @param interval how often to send new chat channel updates in seconds.
     */
    virtual void setChannelUpdateInterval(double interval) = 0;

    /*!
     * @brief Send an update to a given channel.
     *
     * The request is throttled and will be sent to the backend depending on the channel update interval.
     * If the method is called before the channel update interval, the error callback
     * is invoked with code RealTimeMessagingError::Code::RATE_LIMIT_EXCEEDED.
     *
     * @param channelUpdateRequest the channel update request to send.
     * @param errorCallback callback invoked on success/error.
     */
    virtual void sendChannelUpdate(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> channelUpdateRequest, ErrorCallback errorCallback) = 0; 

    virtual ~ChatService() = default; 
};
}
}

