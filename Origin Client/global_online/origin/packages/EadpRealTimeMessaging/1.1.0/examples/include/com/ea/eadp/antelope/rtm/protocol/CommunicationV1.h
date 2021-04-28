// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelUpdateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsRequestV2.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatLeaveV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatTypingEventRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/FetchStickyMessagesRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/GetPreferenceRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/GetPreferenceResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/GetRolesRequest.h>
#include <com/ea/eadp/antelope/rtm/protocol/GetRolesResponse.h>
#include <com/ea/eadp/antelope/rtm/protocol/HeartbeatV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginRequestV2.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginRequestV3.h>
#include <com/ea/eadp/antelope/rtm/protocol/MuteUserV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/NotificationV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PointToPointMessageV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceSubscribeV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceSubscriptionErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUnsubscribeV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUpdateErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUpdateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PromoteStickyMessageRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ReconnectRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/RemoveStickyMessageRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ServerConfigRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ServerConfigV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionCleanupV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionNotificationV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/StickyMessageResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SuccessV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/UnmuteUserV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/UpdatePreferenceRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/UserMembershipChangeV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatAssignV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatChannelsRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatChannelsResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatConfigurationRequestV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatConfigurationResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/WorldChatResponseV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatChannelUpdateV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatChannelsRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatChannelsRequestV2;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatChannelsV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatInitiateV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatLeaveV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatMembersRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatMembersV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatTypingEventRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class FetchStickyMessagesRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class GetPreferenceRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class GetPreferenceResponseV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class GetRolesRequest;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class GetRolesResponse;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class HeartbeatV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginRequestV2;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginRequestV3;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class MuteUserV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class NotificationV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PointToPointMessageV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceSubscribeV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceSubscriptionErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceUnsubscribeV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceUpdateErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceUpdateV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PresenceV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PromoteStickyMessageRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ReconnectRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class RemoveStickyMessageRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ServerConfigRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ServerConfigV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class SessionCleanupV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class SessionNotificationV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class SessionRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class SessionResponseV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class StickyMessageResponseV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class SuccessV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class UnmuteUserV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class UpdatePreferenceRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class UserMembershipChangeV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatAssignV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatChannelsRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatChannelsResponseV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatConfigurationRequestV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatConfigurationResponseV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class WorldChatResponseV1;  } } } } } }

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace rtm
{

namespace protocol
{

class EADPGENERATED_API CommunicationV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit CommunicationV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~CommunicationV1()
    {
        clearBody();
    }

    /**@{
     */
    const ::eadp::foundation::String& getRequestId() const;
    void setRequestId(const ::eadp::foundation::String& value);
    void setRequestId(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> getPresenceSubscribe();
    void setPresenceSubscribe(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1>);
    bool hasPresenceSubscribe() const;
    void releasePresenceSubscribe();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> getPresenceUnsubscribe();
    void setPresenceUnsubscribe(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1>);
    bool hasPresenceUnsubscribe() const;
    void releasePresenceUnsubscribe();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> getPresenceSubscriptionError();
    void setPresenceSubscriptionError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1>);
    bool hasPresenceSubscriptionError() const;
    void releasePresenceSubscriptionError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> getPresenceUpdate();
    void setPresenceUpdate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1>);
    bool hasPresenceUpdate() const;
    void releasePresenceUpdate();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> getPresenceUpdateError();
    void setPresenceUpdateError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1>);
    bool hasPresenceUpdateError() const;
    void releasePresenceUpdateError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> getPresence();
    void setPresence(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1>);
    bool hasPresence() const;
    void releasePresence();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> getNotification();
    void setNotification(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1>);
    bool hasNotification() const;
    void releaseNotification();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> getChatInitiate();
    void setChatInitiate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1>);
    bool hasChatInitiate() const;
    void releaseChatInitiate();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> getChatLeave();
    void setChatLeave(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1>);
    bool hasChatLeave() const;
    void releaseChatLeave();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> getChatMembersRequest();
    void setChatMembersRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1>);
    bool hasChatMembersRequest() const;
    void releaseChatMembersRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1> getChatMembers();
    void setChatMembers(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1>);
    bool hasChatMembers() const;
    void releaseChatMembers();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1> getError();
    void setError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1>);
    bool hasError() const;
    void releaseError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1> getReconnectRequest();
    void setReconnectRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1>);
    bool hasReconnectRequest() const;
    void releaseReconnectRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1> getSuccess();
    void setSuccess(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1>);
    bool hasSuccess() const;
    void releaseSuccess();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> getPointToPointMessage();
    void setPointToPointMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1>);
    bool hasPointToPointMessage() const;
    void releasePointToPointMessage();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1> getChatChannelsRequest();
    void setChatChannelsRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1>);
    bool hasChatChannelsRequest() const;
    void releaseChatChannelsRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1> getChatChannels();
    void setChatChannels(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1>);
    bool hasChatChannels() const;
    void releaseChatChannels();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1> getLoginRequest();
    void setLoginRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1>);
    bool hasLoginRequest() const;
    void releaseLoginRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1> getHeartbeat();
    void setHeartbeat(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1>);
    bool hasHeartbeat() const;
    void releaseHeartbeat();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1> getWorldChatAssign();
    void setWorldChatAssign(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1>);
    bool hasWorldChatAssign() const;
    void releaseWorldChatAssign();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1> getWorldChatResponse();
    void setWorldChatResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1>);
    bool hasWorldChatResponse() const;
    void releaseWorldChatResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1> getMuteUser();
    void setMuteUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1>);
    bool hasMuteUser() const;
    void releaseMuteUser();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1> getUnmuteUser();
    void setUnmuteUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1>);
    bool hasUnmuteUser() const;
    void releaseUnmuteUser();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1> getUserMembershipChange();
    void setUserMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1>);
    bool hasUserMembershipChange() const;
    void releaseUserMembershipChange();
    /**@}*/

    /**@{
     *  World Chat requests & responses.
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1> getWorldChatConfigurationRequestV1();
    void setWorldChatConfigurationRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1>);
    bool hasWorldChatConfigurationRequestV1() const;
    void releaseWorldChatConfigurationRequestV1();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1> getWorldChatConfigurationResponse();
    void setWorldChatConfigurationResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1>);
    bool hasWorldChatConfigurationResponse() const;
    void releaseWorldChatConfigurationResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1> getWorldChatChannelsRequestV1();
    void setWorldChatChannelsRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1>);
    bool hasWorldChatChannelsRequestV1() const;
    void releaseWorldChatChannelsRequestV1();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1> getWorldChatChannelsResponse();
    void setWorldChatChannelsResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1>);
    bool hasWorldChatChannelsResponse() const;
    void releaseWorldChatChannelsResponse();
    /**@}*/

    /**@{
     *  Sticky message requests & responses.
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1> getPromoteStickyMessageRequest();
    void setPromoteStickyMessageRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1>);
    bool hasPromoteStickyMessageRequest() const;
    void releasePromoteStickyMessageRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1> getRemoveStickyMessageRequest();
    void setRemoveStickyMessageRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1>);
    bool hasRemoveStickyMessageRequest() const;
    void releaseRemoveStickyMessageRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1> getFetchStickyMessagesRequest();
    void setFetchStickyMessagesRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1>);
    bool hasFetchStickyMessagesRequest() const;
    void releaseFetchStickyMessagesRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1> getStickyMessageResponse();
    void setStickyMessageResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1>);
    bool hasStickyMessageResponse() const;
    void releaseStickyMessageResponse();
    /**@}*/

    /**@{
     *  Chat moderation requests & responses.
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest> getRolesRequest();
    void setRolesRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest>);
    bool hasRolesRequest() const;
    void releaseRolesRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse> getRolesResponse();
    void setRolesResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse>);
    bool hasRolesResponse() const;
    void releaseRolesResponse();
    /**@}*/

    /**@{
     *  Update preference request, response will be SuccessV1 or ErrorV1
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1> getPreferenceRequest();
    void setPreferenceRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1>);
    bool hasPreferenceRequest() const;
    void releasePreferenceRequest();
    /**@}*/

    /**@{
     *  Fetch current preferences
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1> getGetPreferenceRequest();
    void setGetPreferenceRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1>);
    bool hasGetPreferenceRequest() const;
    void releaseGetPreferenceRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1> getGetPreferenceResponse();
    void setGetPreferenceResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1>);
    bool hasGetPreferenceResponse() const;
    void releaseGetPreferenceResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2> getLoginRequestV2();
    void setLoginRequestV2(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2>);
    bool hasLoginRequestV2() const;
    void releaseLoginRequestV2();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> getSessionNotification();
    void setSessionNotification(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1>);
    bool hasSessionNotification() const;
    void releaseSessionNotification();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> getChatTypingEventRequest();
    void setChatTypingEventRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1>);
    bool hasChatTypingEventRequest() const;
    void releaseChatTypingEventRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> getChatChannelsRequestV2();
    void setChatChannelsRequestV2(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2>);
    bool hasChatChannelsRequestV2() const;
    void releaseChatChannelsRequestV2();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> getChatChannelUpdate();
    void setChatChannelUpdate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1>);
    bool hasChatChannelUpdate() const;
    void releaseChatChannelUpdate();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1> getServerConfigRequest();
    void setServerConfigRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1>);
    bool hasServerConfigRequest() const;
    void releaseServerConfigRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1> getServerConfig();
    void setServerConfig(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1>);
    bool hasServerConfig() const;
    void releaseServerConfig();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3> getLoginRequestV3();
    void setLoginRequestV3(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3>);
    bool hasLoginRequestV3() const;
    void releaseLoginRequestV3();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1> getSessionRequestV1();
    void setSessionRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1>);
    bool hasSessionRequestV1() const;
    void releaseSessionRequestV1();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> getSessionResponseV1();
    void setSessionResponseV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1>);
    bool hasSessionResponseV1() const;
    void releaseSessionResponseV1();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1> getSessionCleanupV1();
    void setSessionCleanupV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1>);
    bool hasSessionCleanupV1() const;
    void releaseSessionCleanupV1();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class BodyCase
    {
        kNotSet = 0,
        kPresenceSubscribe = 2,
        kPresenceUnsubscribe = 3,
        kPresenceSubscriptionError = 4,
        kPresenceUpdate = 5,
        kPresenceUpdateError = 6,
        kPresence = 7,
        kNotification = 8,
        kChatInitiate = 9,
        kChatLeave = 10,
        kChatMembersRequest = 11,
        kChatMembers = 12,
        kError = 13,
        kReconnectRequest = 14,
        kSuccess = 15,
        kPointToPointMessage = 16,
        kChatChannelsRequest = 17,
        kChatChannels = 18,
        kLoginRequest = 19,
        kHeartbeat = 20,
        kWorldChatAssign = 21,
        kWorldChatResponse = 22,
        kMuteUser = 23,
        kUnmuteUser = 24,
        kUserMembershipChange = 25,
        kWorldChatConfigurationRequestV1 = 26,
        kWorldChatConfigurationResponse = 27,
        kWorldChatChannelsRequestV1 = 28,
        kWorldChatChannelsResponse = 29,
        kPromoteStickyMessageRequest = 30,
        kRemoveStickyMessageRequest = 31,
        kFetchStickyMessagesRequest = 32,
        kStickyMessageResponse = 33,
        kRolesRequest = 34,
        kRolesResponse = 35,
        kPreferenceRequest = 36,
        kGetPreferenceRequest = 37,
        kGetPreferenceResponse = 38,
        kLoginRequestV2 = 39,
        kSessionNotification = 40,
        kChatTypingEventRequest = 41,
        kChatChannelsRequestV2 = 42,
        kChatChannelUpdate = 43,
        kServerConfigRequest = 44,
        kServerConfig = 45,
        kLoginRequestV3 = 46,
        kSessionRequestV1 = 47,
        kSessionResponseV1 = 48,
        kSessionCleanupV1 = 49,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    BodyCase getBodyCase() { return static_cast<BodyCase>(m_bodyCase); }

    /*!
     * Serializes the message contents into a buffer.
    Called by serialize.
     */
    uint8_t* onSerialize(uint8_t* target) const override;

    /*!
     * Deserializes the message contents from an input stream.
    Called by deserialize.
     */
    bool onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input) override;

    /*!
     * Returns a previously calculated serialized size of the message in protobuf wire format.
     */
    size_t getCachedSize() const override
    {
        return m_cachedByteSize_;
    }

    /*!
     * Calculates and returns the serialized size of the message in protobuf wire format.
     */
    size_t getByteSize() override;

    /*!
     * Returns a string representing the contents of the message for debugging and logging.
     */
    ::eadp::foundation::String toString(const ::eadp::foundation::String& indent) const override;

private:
    ::eadp::foundation::Allocator m_allocator_;
    size_t m_cachedByteSize_;
    ::eadp::foundation::String m_requestId;

    BodyCase m_bodyCase;

    union BodyUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> m_presenceSubscribe;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> m_presenceUnsubscribe;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> m_presenceSubscriptionError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> m_presenceUpdate;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> m_presenceUpdateError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> m_presence;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> m_notification;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> m_chatInitiate;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> m_chatLeave;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> m_chatMembersRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1> m_chatMembers;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1> m_error;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1> m_reconnectRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1> m_success;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> m_pointToPointMessage;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1> m_chatChannelsRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1> m_chatChannels;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1> m_loginRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1> m_heartbeat;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1> m_worldChatAssign;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1> m_worldChatResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1> m_muteUser;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1> m_unmuteUser;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1> m_userMembershipChange;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1> m_worldChatConfigurationRequestV1;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1> m_worldChatConfigurationResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1> m_worldChatChannelsRequestV1;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1> m_worldChatChannelsResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1> m_promoteStickyMessageRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1> m_removeStickyMessageRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1> m_fetchStickyMessagesRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1> m_stickyMessageResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest> m_rolesRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse> m_rolesResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1> m_preferenceRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1> m_getPreferenceRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1> m_getPreferenceResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2> m_loginRequestV2;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> m_sessionNotification;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> m_chatTypingEventRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> m_chatChannelsRequestV2;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> m_chatChannelUpdate;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1> m_serverConfigRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1> m_serverConfig;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3> m_loginRequestV3;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1> m_sessionRequestV1;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> m_sessionResponseV1;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1> m_sessionCleanupV1;

        BodyUnion() {}
        ~BodyUnion() {}
    } m_body;

    void clearBody();

};

}
}
}
}
}
}
