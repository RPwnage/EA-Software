// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/CommunicationV1.h>
#include <eadp/foundation/internal/ProtobufWireFormat.h>
#include <eadp/foundation/internal/Utils.h>
#include <eadp/foundation/Hub.h>

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

CommunicationV1::CommunicationV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.CommunicationV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_requestId(m_allocator_.emptyString())
, m_bodyCase(BodyCase::kNotSet)
{
}

void CommunicationV1::clear()
{
    m_cachedByteSize_ = 0;
    m_requestId.clear();
    clearBody();
}

size_t CommunicationV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_requestId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_requestId);
    }
    if (hasPresenceSubscribe() && m_body.m_presenceSubscribe)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presenceSubscribe);
    }
    if (hasPresenceUnsubscribe() && m_body.m_presenceUnsubscribe)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presenceUnsubscribe);
    }
    if (hasPresenceSubscriptionError() && m_body.m_presenceSubscriptionError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presenceSubscriptionError);
    }
    if (hasPresenceUpdate() && m_body.m_presenceUpdate)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presenceUpdate);
    }
    if (hasPresenceUpdateError() && m_body.m_presenceUpdateError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presenceUpdateError);
    }
    if (hasPresence() && m_body.m_presence)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_presence);
    }
    if (hasNotification() && m_body.m_notification)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_notification);
    }
    if (hasChatInitiate() && m_body.m_chatInitiate)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatInitiate);
    }
    if (hasChatLeave() && m_body.m_chatLeave)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatLeave);
    }
    if (hasChatMembersRequest() && m_body.m_chatMembersRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatMembersRequest);
    }
    if (hasChatMembers() && m_body.m_chatMembers)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatMembers);
    }
    if (hasError() && m_body.m_error)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_error);
    }
    if (hasReconnectRequest() && m_body.m_reconnectRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_reconnectRequest);
    }
    if (hasSuccess() && m_body.m_success)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_success);
    }
    if (hasPointToPointMessage() && m_body.m_pointToPointMessage)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_pointToPointMessage);
    }
    if (hasChatChannelsRequest() && m_body.m_chatChannelsRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatChannelsRequest);
    }
    if (hasChatChannels() && m_body.m_chatChannels)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatChannels);
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginRequest);
    }
    if (hasHeartbeat() && m_body.m_heartbeat)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_heartbeat);
    }
    if (hasWorldChatAssign() && m_body.m_worldChatAssign)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatAssign);
    }
    if (hasWorldChatResponse() && m_body.m_worldChatResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatResponse);
    }
    if (hasMuteUser() && m_body.m_muteUser)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_muteUser);
    }
    if (hasUnmuteUser() && m_body.m_unmuteUser)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_unmuteUser);
    }
    if (hasUserMembershipChange() && m_body.m_userMembershipChange)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_userMembershipChange);
    }
    if (hasWorldChatConfigurationRequestV1() && m_body.m_worldChatConfigurationRequestV1)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatConfigurationRequestV1);
    }
    if (hasWorldChatConfigurationResponse() && m_body.m_worldChatConfigurationResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatConfigurationResponse);
    }
    if (hasWorldChatChannelsRequestV1() && m_body.m_worldChatChannelsRequestV1)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatChannelsRequestV1);
    }
    if (hasWorldChatChannelsResponse() && m_body.m_worldChatChannelsResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_worldChatChannelsResponse);
    }
    if (hasPromoteStickyMessageRequest() && m_body.m_promoteStickyMessageRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_promoteStickyMessageRequest);
    }
    if (hasRemoveStickyMessageRequest() && m_body.m_removeStickyMessageRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_removeStickyMessageRequest);
    }
    if (hasFetchStickyMessagesRequest() && m_body.m_fetchStickyMessagesRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_fetchStickyMessagesRequest);
    }
    if (hasStickyMessageResponse() && m_body.m_stickyMessageResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_stickyMessageResponse);
    }
    if (hasRolesRequest() && m_body.m_rolesRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_rolesRequest);
    }
    if (hasRolesResponse() && m_body.m_rolesResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_rolesResponse);
    }
    if (hasPreferenceRequest() && m_body.m_preferenceRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_preferenceRequest);
    }
    if (hasGetPreferenceRequest() && m_body.m_getPreferenceRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_getPreferenceRequest);
    }
    if (hasGetPreferenceResponse() && m_body.m_getPreferenceResponse)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_getPreferenceResponse);
    }
    if (hasLoginRequestV2() && m_body.m_loginRequestV2)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginRequestV2);
    }
    if (hasSessionNotification() && m_body.m_sessionNotification)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_sessionNotification);
    }
    if (hasChatTypingEventRequest() && m_body.m_chatTypingEventRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatTypingEventRequest);
    }
    if (hasChatChannelsRequestV2() && m_body.m_chatChannelsRequestV2)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatChannelsRequestV2);
    }
    if (hasChatChannelUpdate() && m_body.m_chatChannelUpdate)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatChannelUpdate);
    }
    if (hasServerConfigRequest() && m_body.m_serverConfigRequest)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_serverConfigRequest);
    }
    if (hasServerConfig() && m_body.m_serverConfig)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_serverConfig);
    }
    if (hasLoginRequestV3() && m_body.m_loginRequestV3)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginRequestV3);
    }
    if (hasSessionRequestV1() && m_body.m_sessionRequestV1)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_sessionRequestV1);
    }
    if (hasSessionResponseV1() && m_body.m_sessionResponseV1)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_sessionResponseV1);
    }
    if (hasSessionCleanupV1() && m_body.m_sessionCleanupV1)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_sessionCleanupV1);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* CommunicationV1::onSerialize(uint8_t* target) const
{
    if (!getRequestId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getRequestId(), target);
    }
    if (hasPresenceSubscribe() && m_body.m_presenceSubscribe)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_body.m_presenceSubscribe, target);
    }
    if (hasPresenceUnsubscribe() && m_body.m_presenceUnsubscribe)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_body.m_presenceUnsubscribe, target);
    }
    if (hasPresenceSubscriptionError() && m_body.m_presenceSubscriptionError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_body.m_presenceSubscriptionError, target);
    }
    if (hasPresenceUpdate() && m_body.m_presenceUpdate)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_body.m_presenceUpdate, target);
    }
    if (hasPresenceUpdateError() && m_body.m_presenceUpdateError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_body.m_presenceUpdateError, target);
    }
    if (hasPresence() && m_body.m_presence)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_body.m_presence, target);
    }
    if (hasNotification() && m_body.m_notification)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(8, *m_body.m_notification, target);
    }
    if (hasChatInitiate() && m_body.m_chatInitiate)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(9, *m_body.m_chatInitiate, target);
    }
    if (hasChatLeave() && m_body.m_chatLeave)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(10, *m_body.m_chatLeave, target);
    }
    if (hasChatMembersRequest() && m_body.m_chatMembersRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(11, *m_body.m_chatMembersRequest, target);
    }
    if (hasChatMembers() && m_body.m_chatMembers)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(12, *m_body.m_chatMembers, target);
    }
    if (hasError() && m_body.m_error)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(13, *m_body.m_error, target);
    }
    if (hasReconnectRequest() && m_body.m_reconnectRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(14, *m_body.m_reconnectRequest, target);
    }
    if (hasSuccess() && m_body.m_success)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(15, *m_body.m_success, target);
    }
    if (hasPointToPointMessage() && m_body.m_pointToPointMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(16, *m_body.m_pointToPointMessage, target);
    }
    if (hasChatChannelsRequest() && m_body.m_chatChannelsRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(17, *m_body.m_chatChannelsRequest, target);
    }
    if (hasChatChannels() && m_body.m_chatChannels)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(18, *m_body.m_chatChannels, target);
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(19, *m_body.m_loginRequest, target);
    }
    if (hasHeartbeat() && m_body.m_heartbeat)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(20, *m_body.m_heartbeat, target);
    }
    if (hasWorldChatAssign() && m_body.m_worldChatAssign)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(21, *m_body.m_worldChatAssign, target);
    }
    if (hasWorldChatResponse() && m_body.m_worldChatResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(22, *m_body.m_worldChatResponse, target);
    }
    if (hasMuteUser() && m_body.m_muteUser)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(23, *m_body.m_muteUser, target);
    }
    if (hasUnmuteUser() && m_body.m_unmuteUser)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(24, *m_body.m_unmuteUser, target);
    }
    if (hasUserMembershipChange() && m_body.m_userMembershipChange)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(25, *m_body.m_userMembershipChange, target);
    }
    if (hasWorldChatConfigurationRequestV1() && m_body.m_worldChatConfigurationRequestV1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(26, *m_body.m_worldChatConfigurationRequestV1, target);
    }
    if (hasWorldChatConfigurationResponse() && m_body.m_worldChatConfigurationResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(27, *m_body.m_worldChatConfigurationResponse, target);
    }
    if (hasWorldChatChannelsRequestV1() && m_body.m_worldChatChannelsRequestV1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(28, *m_body.m_worldChatChannelsRequestV1, target);
    }
    if (hasWorldChatChannelsResponse() && m_body.m_worldChatChannelsResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(29, *m_body.m_worldChatChannelsResponse, target);
    }
    if (hasPromoteStickyMessageRequest() && m_body.m_promoteStickyMessageRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(30, *m_body.m_promoteStickyMessageRequest, target);
    }
    if (hasRemoveStickyMessageRequest() && m_body.m_removeStickyMessageRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(31, *m_body.m_removeStickyMessageRequest, target);
    }
    if (hasFetchStickyMessagesRequest() && m_body.m_fetchStickyMessagesRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(32, *m_body.m_fetchStickyMessagesRequest, target);
    }
    if (hasStickyMessageResponse() && m_body.m_stickyMessageResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(33, *m_body.m_stickyMessageResponse, target);
    }
    if (hasRolesRequest() && m_body.m_rolesRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(34, *m_body.m_rolesRequest, target);
    }
    if (hasRolesResponse() && m_body.m_rolesResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(35, *m_body.m_rolesResponse, target);
    }
    if (hasPreferenceRequest() && m_body.m_preferenceRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(36, *m_body.m_preferenceRequest, target);
    }
    if (hasGetPreferenceRequest() && m_body.m_getPreferenceRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(37, *m_body.m_getPreferenceRequest, target);
    }
    if (hasGetPreferenceResponse() && m_body.m_getPreferenceResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(38, *m_body.m_getPreferenceResponse, target);
    }
    if (hasLoginRequestV2() && m_body.m_loginRequestV2)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(39, *m_body.m_loginRequestV2, target);
    }
    if (hasSessionNotification() && m_body.m_sessionNotification)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(40, *m_body.m_sessionNotification, target);
    }
    if (hasChatTypingEventRequest() && m_body.m_chatTypingEventRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(41, *m_body.m_chatTypingEventRequest, target);
    }
    if (hasChatChannelsRequestV2() && m_body.m_chatChannelsRequestV2)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(42, *m_body.m_chatChannelsRequestV2, target);
    }
    if (hasChatChannelUpdate() && m_body.m_chatChannelUpdate)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(43, *m_body.m_chatChannelUpdate, target);
    }
    if (hasServerConfigRequest() && m_body.m_serverConfigRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(44, *m_body.m_serverConfigRequest, target);
    }
    if (hasServerConfig() && m_body.m_serverConfig)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(45, *m_body.m_serverConfig, target);
    }
    if (hasLoginRequestV3() && m_body.m_loginRequestV3)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(46, *m_body.m_loginRequestV3, target);
    }
    if (hasSessionRequestV1() && m_body.m_sessionRequestV1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(47, *m_body.m_sessionRequestV1, target);
    }
    if (hasSessionResponseV1() && m_body.m_sessionResponseV1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(48, *m_body.m_sessionResponseV1, target);
    }
    if (hasSessionCleanupV1() && m_body.m_sessionCleanupV1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(49, *m_body.m_sessionCleanupV1, target);
    }
    return target;
}

bool CommunicationV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 16383) goto handle_unusual;
        switch (::eadp::foundation::Internal::ProtobufWireFormat::getTagFieldNumber(tag))
        {
            case 1:
            {
                if (tag == 10)
                {
                    if (!input->readString(&m_requestId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_presenceSubscribe;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_presenceSubscribe:
                    clearBody();
                    if (!input->readMessage(getPresenceSubscribe().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_presenceUnsubscribe;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_presenceUnsubscribe:
                    clearBody();
                    if (!input->readMessage(getPresenceUnsubscribe().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_presenceSubscriptionError;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_presenceSubscriptionError:
                    clearBody();
                    if (!input->readMessage(getPresenceSubscriptionError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_presenceUpdate;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_presenceUpdate:
                    clearBody();
                    if (!input->readMessage(getPresenceUpdate().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_presenceUpdateError;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_presenceUpdateError:
                    clearBody();
                    if (!input->readMessage(getPresenceUpdateError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_presence;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_presence:
                    clearBody();
                    if (!input->readMessage(getPresence().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_notification;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_notification:
                    clearBody();
                    if (!input->readMessage(getNotification().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_chatInitiate;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_chatInitiate:
                    clearBody();
                    if (!input->readMessage(getChatInitiate().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(82)) goto parse_chatLeave;
                break;
            }

            case 10:
            {
                if (tag == 82)
                {
                    parse_chatLeave:
                    clearBody();
                    if (!input->readMessage(getChatLeave().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(90)) goto parse_chatMembersRequest;
                break;
            }

            case 11:
            {
                if (tag == 90)
                {
                    parse_chatMembersRequest:
                    clearBody();
                    if (!input->readMessage(getChatMembersRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(98)) goto parse_chatMembers;
                break;
            }

            case 12:
            {
                if (tag == 98)
                {
                    parse_chatMembers:
                    clearBody();
                    if (!input->readMessage(getChatMembers().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(106)) goto parse_error;
                break;
            }

            case 13:
            {
                if (tag == 106)
                {
                    parse_error:
                    clearBody();
                    if (!input->readMessage(getError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(114)) goto parse_reconnectRequest;
                break;
            }

            case 14:
            {
                if (tag == 114)
                {
                    parse_reconnectRequest:
                    clearBody();
                    if (!input->readMessage(getReconnectRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(122)) goto parse_success;
                break;
            }

            case 15:
            {
                if (tag == 122)
                {
                    parse_success:
                    clearBody();
                    if (!input->readMessage(getSuccess().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(130)) goto parse_pointToPointMessage;
                break;
            }

            case 16:
            {
                if (tag == 130)
                {
                    parse_pointToPointMessage:
                    clearBody();
                    if (!input->readMessage(getPointToPointMessage().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(138)) goto parse_chatChannelsRequest;
                break;
            }

            case 17:
            {
                if (tag == 138)
                {
                    parse_chatChannelsRequest:
                    clearBody();
                    if (!input->readMessage(getChatChannelsRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(146)) goto parse_chatChannels;
                break;
            }

            case 18:
            {
                if (tag == 146)
                {
                    parse_chatChannels:
                    clearBody();
                    if (!input->readMessage(getChatChannels().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(154)) goto parse_loginRequest;
                break;
            }

            case 19:
            {
                if (tag == 154)
                {
                    parse_loginRequest:
                    clearBody();
                    if (!input->readMessage(getLoginRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(162)) goto parse_heartbeat;
                break;
            }

            case 20:
            {
                if (tag == 162)
                {
                    parse_heartbeat:
                    clearBody();
                    if (!input->readMessage(getHeartbeat().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(170)) goto parse_worldChatAssign;
                break;
            }

            case 21:
            {
                if (tag == 170)
                {
                    parse_worldChatAssign:
                    clearBody();
                    if (!input->readMessage(getWorldChatAssign().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(178)) goto parse_worldChatResponse;
                break;
            }

            case 22:
            {
                if (tag == 178)
                {
                    parse_worldChatResponse:
                    clearBody();
                    if (!input->readMessage(getWorldChatResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(186)) goto parse_muteUser;
                break;
            }

            case 23:
            {
                if (tag == 186)
                {
                    parse_muteUser:
                    clearBody();
                    if (!input->readMessage(getMuteUser().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(194)) goto parse_unmuteUser;
                break;
            }

            case 24:
            {
                if (tag == 194)
                {
                    parse_unmuteUser:
                    clearBody();
                    if (!input->readMessage(getUnmuteUser().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(202)) goto parse_userMembershipChange;
                break;
            }

            case 25:
            {
                if (tag == 202)
                {
                    parse_userMembershipChange:
                    clearBody();
                    if (!input->readMessage(getUserMembershipChange().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(210)) goto parse_worldChatConfigurationRequestV1;
                break;
            }

            case 26:
            {
                if (tag == 210)
                {
                    parse_worldChatConfigurationRequestV1:
                    clearBody();
                    if (!input->readMessage(getWorldChatConfigurationRequestV1().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(218)) goto parse_worldChatConfigurationResponse;
                break;
            }

            case 27:
            {
                if (tag == 218)
                {
                    parse_worldChatConfigurationResponse:
                    clearBody();
                    if (!input->readMessage(getWorldChatConfigurationResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(226)) goto parse_worldChatChannelsRequestV1;
                break;
            }

            case 28:
            {
                if (tag == 226)
                {
                    parse_worldChatChannelsRequestV1:
                    clearBody();
                    if (!input->readMessage(getWorldChatChannelsRequestV1().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(234)) goto parse_worldChatChannelsResponse;
                break;
            }

            case 29:
            {
                if (tag == 234)
                {
                    parse_worldChatChannelsResponse:
                    clearBody();
                    if (!input->readMessage(getWorldChatChannelsResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(242)) goto parse_promoteStickyMessageRequest;
                break;
            }

            case 30:
            {
                if (tag == 242)
                {
                    parse_promoteStickyMessageRequest:
                    clearBody();
                    if (!input->readMessage(getPromoteStickyMessageRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(250)) goto parse_removeStickyMessageRequest;
                break;
            }

            case 31:
            {
                if (tag == 250)
                {
                    parse_removeStickyMessageRequest:
                    clearBody();
                    if (!input->readMessage(getRemoveStickyMessageRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(258)) goto parse_fetchStickyMessagesRequest;
                break;
            }

            case 32:
            {
                if (tag == 258)
                {
                    parse_fetchStickyMessagesRequest:
                    clearBody();
                    if (!input->readMessage(getFetchStickyMessagesRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(266)) goto parse_stickyMessageResponse;
                break;
            }

            case 33:
            {
                if (tag == 266)
                {
                    parse_stickyMessageResponse:
                    clearBody();
                    if (!input->readMessage(getStickyMessageResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(274)) goto parse_rolesRequest;
                break;
            }

            case 34:
            {
                if (tag == 274)
                {
                    parse_rolesRequest:
                    clearBody();
                    if (!input->readMessage(getRolesRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(282)) goto parse_rolesResponse;
                break;
            }

            case 35:
            {
                if (tag == 282)
                {
                    parse_rolesResponse:
                    clearBody();
                    if (!input->readMessage(getRolesResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(290)) goto parse_preferenceRequest;
                break;
            }

            case 36:
            {
                if (tag == 290)
                {
                    parse_preferenceRequest:
                    clearBody();
                    if (!input->readMessage(getPreferenceRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(298)) goto parse_getPreferenceRequest;
                break;
            }

            case 37:
            {
                if (tag == 298)
                {
                    parse_getPreferenceRequest:
                    clearBody();
                    if (!input->readMessage(getGetPreferenceRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(306)) goto parse_getPreferenceResponse;
                break;
            }

            case 38:
            {
                if (tag == 306)
                {
                    parse_getPreferenceResponse:
                    clearBody();
                    if (!input->readMessage(getGetPreferenceResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(314)) goto parse_loginRequestV2;
                break;
            }

            case 39:
            {
                if (tag == 314)
                {
                    parse_loginRequestV2:
                    clearBody();
                    if (!input->readMessage(getLoginRequestV2().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(322)) goto parse_sessionNotification;
                break;
            }

            case 40:
            {
                if (tag == 322)
                {
                    parse_sessionNotification:
                    clearBody();
                    if (!input->readMessage(getSessionNotification().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(330)) goto parse_chatTypingEventRequest;
                break;
            }

            case 41:
            {
                if (tag == 330)
                {
                    parse_chatTypingEventRequest:
                    clearBody();
                    if (!input->readMessage(getChatTypingEventRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(338)) goto parse_chatChannelsRequestV2;
                break;
            }

            case 42:
            {
                if (tag == 338)
                {
                    parse_chatChannelsRequestV2:
                    clearBody();
                    if (!input->readMessage(getChatChannelsRequestV2().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(346)) goto parse_chatChannelUpdate;
                break;
            }

            case 43:
            {
                if (tag == 346)
                {
                    parse_chatChannelUpdate:
                    clearBody();
                    if (!input->readMessage(getChatChannelUpdate().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(354)) goto parse_serverConfigRequest;
                break;
            }

            case 44:
            {
                if (tag == 354)
                {
                    parse_serverConfigRequest:
                    clearBody();
                    if (!input->readMessage(getServerConfigRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(362)) goto parse_serverConfig;
                break;
            }

            case 45:
            {
                if (tag == 362)
                {
                    parse_serverConfig:
                    clearBody();
                    if (!input->readMessage(getServerConfig().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(370)) goto parse_loginRequestV3;
                break;
            }

            case 46:
            {
                if (tag == 370)
                {
                    parse_loginRequestV3:
                    clearBody();
                    if (!input->readMessage(getLoginRequestV3().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(378)) goto parse_sessionRequestV1;
                break;
            }

            case 47:
            {
                if (tag == 378)
                {
                    parse_sessionRequestV1:
                    clearBody();
                    if (!input->readMessage(getSessionRequestV1().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(386)) goto parse_sessionResponseV1;
                break;
            }

            case 48:
            {
                if (tag == 386)
                {
                    parse_sessionResponseV1:
                    clearBody();
                    if (!input->readMessage(getSessionResponseV1().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(394)) goto parse_sessionCleanupV1;
                break;
            }

            case 49:
            {
                if (tag == 394)
                {
                    parse_sessionCleanupV1:
                    clearBody();
                    if (!input->readMessage(getSessionCleanupV1().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectAtEnd()) return true;
                break;
            }

            default: {
            handle_unusual:
                if (tag == 0)
                {
                    return true;
                }
                if(!input->skipField(tag)) return false;
                break;
            }
        }
    }
}
::eadp::foundation::String CommunicationV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_requestId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  requestId: \"%s\",\n", indent.c_str(), m_requestId.c_str());
    }
    if (hasPresenceSubscribe() && m_body.m_presenceSubscribe)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presenceSubscribe: %s,\n", indent.c_str(), m_body.m_presenceSubscribe->toString(indent + "  ").c_str());
    }
    if (hasPresenceUnsubscribe() && m_body.m_presenceUnsubscribe)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presenceUnsubscribe: %s,\n", indent.c_str(), m_body.m_presenceUnsubscribe->toString(indent + "  ").c_str());
    }
    if (hasPresenceSubscriptionError() && m_body.m_presenceSubscriptionError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presenceSubscriptionError: %s,\n", indent.c_str(), m_body.m_presenceSubscriptionError->toString(indent + "  ").c_str());
    }
    if (hasPresenceUpdate() && m_body.m_presenceUpdate)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presenceUpdate: %s,\n", indent.c_str(), m_body.m_presenceUpdate->toString(indent + "  ").c_str());
    }
    if (hasPresenceUpdateError() && m_body.m_presenceUpdateError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presenceUpdateError: %s,\n", indent.c_str(), m_body.m_presenceUpdateError->toString(indent + "  ").c_str());
    }
    if (hasPresence() && m_body.m_presence)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  presence: %s,\n", indent.c_str(), m_body.m_presence->toString(indent + "  ").c_str());
    }
    if (hasNotification() && m_body.m_notification)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  notification: %s,\n", indent.c_str(), m_body.m_notification->toString(indent + "  ").c_str());
    }
    if (hasChatInitiate() && m_body.m_chatInitiate)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatInitiate: %s,\n", indent.c_str(), m_body.m_chatInitiate->toString(indent + "  ").c_str());
    }
    if (hasChatLeave() && m_body.m_chatLeave)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatLeave: %s,\n", indent.c_str(), m_body.m_chatLeave->toString(indent + "  ").c_str());
    }
    if (hasChatMembersRequest() && m_body.m_chatMembersRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatMembersRequest: %s,\n", indent.c_str(), m_body.m_chatMembersRequest->toString(indent + "  ").c_str());
    }
    if (hasChatMembers() && m_body.m_chatMembers)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatMembers: %s,\n", indent.c_str(), m_body.m_chatMembers->toString(indent + "  ").c_str());
    }
    if (hasError() && m_body.m_error)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  error: %s,\n", indent.c_str(), m_body.m_error->toString(indent + "  ").c_str());
    }
    if (hasReconnectRequest() && m_body.m_reconnectRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  reconnectRequest: %s,\n", indent.c_str(), m_body.m_reconnectRequest->toString(indent + "  ").c_str());
    }
    if (hasSuccess() && m_body.m_success)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  success: %s,\n", indent.c_str(), m_body.m_success->toString(indent + "  ").c_str());
    }
    if (hasPointToPointMessage() && m_body.m_pointToPointMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  pointToPointMessage: %s,\n", indent.c_str(), m_body.m_pointToPointMessage->toString(indent + "  ").c_str());
    }
    if (hasChatChannelsRequest() && m_body.m_chatChannelsRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatChannelsRequest: %s,\n", indent.c_str(), m_body.m_chatChannelsRequest->toString(indent + "  ").c_str());
    }
    if (hasChatChannels() && m_body.m_chatChannels)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatChannels: %s,\n", indent.c_str(), m_body.m_chatChannels->toString(indent + "  ").c_str());
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginRequest: %s,\n", indent.c_str(), m_body.m_loginRequest->toString(indent + "  ").c_str());
    }
    if (hasHeartbeat() && m_body.m_heartbeat)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  heartbeat: %s,\n", indent.c_str(), m_body.m_heartbeat->toString(indent + "  ").c_str());
    }
    if (hasWorldChatAssign() && m_body.m_worldChatAssign)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatAssign: %s,\n", indent.c_str(), m_body.m_worldChatAssign->toString(indent + "  ").c_str());
    }
    if (hasWorldChatResponse() && m_body.m_worldChatResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatResponse: %s,\n", indent.c_str(), m_body.m_worldChatResponse->toString(indent + "  ").c_str());
    }
    if (hasMuteUser() && m_body.m_muteUser)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  muteUser: %s,\n", indent.c_str(), m_body.m_muteUser->toString(indent + "  ").c_str());
    }
    if (hasUnmuteUser() && m_body.m_unmuteUser)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unmuteUser: %s,\n", indent.c_str(), m_body.m_unmuteUser->toString(indent + "  ").c_str());
    }
    if (hasUserMembershipChange() && m_body.m_userMembershipChange)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  userMembershipChange: %s,\n", indent.c_str(), m_body.m_userMembershipChange->toString(indent + "  ").c_str());
    }
    if (hasWorldChatConfigurationRequestV1() && m_body.m_worldChatConfigurationRequestV1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatConfigurationRequestV1: %s,\n", indent.c_str(), m_body.m_worldChatConfigurationRequestV1->toString(indent + "  ").c_str());
    }
    if (hasWorldChatConfigurationResponse() && m_body.m_worldChatConfigurationResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatConfigurationResponse: %s,\n", indent.c_str(), m_body.m_worldChatConfigurationResponse->toString(indent + "  ").c_str());
    }
    if (hasWorldChatChannelsRequestV1() && m_body.m_worldChatChannelsRequestV1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatChannelsRequestV1: %s,\n", indent.c_str(), m_body.m_worldChatChannelsRequestV1->toString(indent + "  ").c_str());
    }
    if (hasWorldChatChannelsResponse() && m_body.m_worldChatChannelsResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldChatChannelsResponse: %s,\n", indent.c_str(), m_body.m_worldChatChannelsResponse->toString(indent + "  ").c_str());
    }
    if (hasPromoteStickyMessageRequest() && m_body.m_promoteStickyMessageRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  promoteStickyMessageRequest: %s,\n", indent.c_str(), m_body.m_promoteStickyMessageRequest->toString(indent + "  ").c_str());
    }
    if (hasRemoveStickyMessageRequest() && m_body.m_removeStickyMessageRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  removeStickyMessageRequest: %s,\n", indent.c_str(), m_body.m_removeStickyMessageRequest->toString(indent + "  ").c_str());
    }
    if (hasFetchStickyMessagesRequest() && m_body.m_fetchStickyMessagesRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fetchStickyMessagesRequest: %s,\n", indent.c_str(), m_body.m_fetchStickyMessagesRequest->toString(indent + "  ").c_str());
    }
    if (hasStickyMessageResponse() && m_body.m_stickyMessageResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyMessageResponse: %s,\n", indent.c_str(), m_body.m_stickyMessageResponse->toString(indent + "  ").c_str());
    }
    if (hasRolesRequest() && m_body.m_rolesRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  rolesRequest: %s,\n", indent.c_str(), m_body.m_rolesRequest->toString(indent + "  ").c_str());
    }
    if (hasRolesResponse() && m_body.m_rolesResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  rolesResponse: %s,\n", indent.c_str(), m_body.m_rolesResponse->toString(indent + "  ").c_str());
    }
    if (hasPreferenceRequest() && m_body.m_preferenceRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  preferenceRequest: %s,\n", indent.c_str(), m_body.m_preferenceRequest->toString(indent + "  ").c_str());
    }
    if (hasGetPreferenceRequest() && m_body.m_getPreferenceRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  getPreferenceRequest: %s,\n", indent.c_str(), m_body.m_getPreferenceRequest->toString(indent + "  ").c_str());
    }
    if (hasGetPreferenceResponse() && m_body.m_getPreferenceResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  getPreferenceResponse: %s,\n", indent.c_str(), m_body.m_getPreferenceResponse->toString(indent + "  ").c_str());
    }
    if (hasLoginRequestV2() && m_body.m_loginRequestV2)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginRequestV2: %s,\n", indent.c_str(), m_body.m_loginRequestV2->toString(indent + "  ").c_str());
    }
    if (hasSessionNotification() && m_body.m_sessionNotification)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionNotification: %s,\n", indent.c_str(), m_body.m_sessionNotification->toString(indent + "  ").c_str());
    }
    if (hasChatTypingEventRequest() && m_body.m_chatTypingEventRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatTypingEventRequest: %s,\n", indent.c_str(), m_body.m_chatTypingEventRequest->toString(indent + "  ").c_str());
    }
    if (hasChatChannelsRequestV2() && m_body.m_chatChannelsRequestV2)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatChannelsRequestV2: %s,\n", indent.c_str(), m_body.m_chatChannelsRequestV2->toString(indent + "  ").c_str());
    }
    if (hasChatChannelUpdate() && m_body.m_chatChannelUpdate)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatChannelUpdate: %s,\n", indent.c_str(), m_body.m_chatChannelUpdate->toString(indent + "  ").c_str());
    }
    if (hasServerConfigRequest() && m_body.m_serverConfigRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  serverConfigRequest: %s,\n", indent.c_str(), m_body.m_serverConfigRequest->toString(indent + "  ").c_str());
    }
    if (hasServerConfig() && m_body.m_serverConfig)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  serverConfig: %s,\n", indent.c_str(), m_body.m_serverConfig->toString(indent + "  ").c_str());
    }
    if (hasLoginRequestV3() && m_body.m_loginRequestV3)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginRequestV3: %s,\n", indent.c_str(), m_body.m_loginRequestV3->toString(indent + "  ").c_str());
    }
    if (hasSessionRequestV1() && m_body.m_sessionRequestV1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionRequestV1: %s,\n", indent.c_str(), m_body.m_sessionRequestV1->toString(indent + "  ").c_str());
    }
    if (hasSessionResponseV1() && m_body.m_sessionResponseV1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionResponseV1: %s,\n", indent.c_str(), m_body.m_sessionResponseV1->toString(indent + "  ").c_str());
    }
    if (hasSessionCleanupV1() && m_body.m_sessionCleanupV1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionCleanupV1: %s,\n", indent.c_str(), m_body.m_sessionCleanupV1->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& CommunicationV1::getRequestId() const
{
    return m_requestId;
}

void CommunicationV1::setRequestId(const ::eadp::foundation::String& value)
{
    m_requestId = value;
}

void CommunicationV1::setRequestId(const char8_t* value)
{
    m_requestId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> CommunicationV1::getPresenceSubscribe()
{
    if (!hasPresenceSubscribe())
    {
        setPresenceSubscribe(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1>(m_allocator_));
    }
    return m_body.m_presenceSubscribe;
}

bool CommunicationV1::hasPresenceSubscribe() const
{
    return m_bodyCase == BodyCase::kPresenceSubscribe;
}

void CommunicationV1::setPresenceSubscribe(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> value)
{
    clearBody();
    new(&m_body.m_presenceSubscribe) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1>(value);
    m_bodyCase = BodyCase::kPresenceSubscribe;
}

void CommunicationV1::releasePresenceSubscribe()
{
    if (hasPresenceSubscribe())
    {
        m_body.m_presenceSubscribe.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> CommunicationV1::getPresenceUnsubscribe()
{
    if (!hasPresenceUnsubscribe())
    {
        setPresenceUnsubscribe(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1>(m_allocator_));
    }
    return m_body.m_presenceUnsubscribe;
}

bool CommunicationV1::hasPresenceUnsubscribe() const
{
    return m_bodyCase == BodyCase::kPresenceUnsubscribe;
}

void CommunicationV1::setPresenceUnsubscribe(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> value)
{
    clearBody();
    new(&m_body.m_presenceUnsubscribe) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1>(value);
    m_bodyCase = BodyCase::kPresenceUnsubscribe;
}

void CommunicationV1::releasePresenceUnsubscribe()
{
    if (hasPresenceUnsubscribe())
    {
        m_body.m_presenceUnsubscribe.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> CommunicationV1::getPresenceSubscriptionError()
{
    if (!hasPresenceSubscriptionError())
    {
        setPresenceSubscriptionError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1>(m_allocator_));
    }
    return m_body.m_presenceSubscriptionError;
}

bool CommunicationV1::hasPresenceSubscriptionError() const
{
    return m_bodyCase == BodyCase::kPresenceSubscriptionError;
}

void CommunicationV1::setPresenceSubscriptionError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> value)
{
    clearBody();
    new(&m_body.m_presenceSubscriptionError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1>(value);
    m_bodyCase = BodyCase::kPresenceSubscriptionError;
}

void CommunicationV1::releasePresenceSubscriptionError()
{
    if (hasPresenceSubscriptionError())
    {
        m_body.m_presenceSubscriptionError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> CommunicationV1::getPresenceUpdate()
{
    if (!hasPresenceUpdate())
    {
        setPresenceUpdate(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1>(m_allocator_));
    }
    return m_body.m_presenceUpdate;
}

bool CommunicationV1::hasPresenceUpdate() const
{
    return m_bodyCase == BodyCase::kPresenceUpdate;
}

void CommunicationV1::setPresenceUpdate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> value)
{
    clearBody();
    new(&m_body.m_presenceUpdate) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1>(value);
    m_bodyCase = BodyCase::kPresenceUpdate;
}

void CommunicationV1::releasePresenceUpdate()
{
    if (hasPresenceUpdate())
    {
        m_body.m_presenceUpdate.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> CommunicationV1::getPresenceUpdateError()
{
    if (!hasPresenceUpdateError())
    {
        setPresenceUpdateError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1>(m_allocator_));
    }
    return m_body.m_presenceUpdateError;
}

bool CommunicationV1::hasPresenceUpdateError() const
{
    return m_bodyCase == BodyCase::kPresenceUpdateError;
}

void CommunicationV1::setPresenceUpdateError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> value)
{
    clearBody();
    new(&m_body.m_presenceUpdateError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1>(value);
    m_bodyCase = BodyCase::kPresenceUpdateError;
}

void CommunicationV1::releasePresenceUpdateError()
{
    if (hasPresenceUpdateError())
    {
        m_body.m_presenceUpdateError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> CommunicationV1::getPresence()
{
    if (!hasPresence())
    {
        setPresence(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PresenceV1>(m_allocator_));
    }
    return m_body.m_presence;
}

bool CommunicationV1::hasPresence() const
{
    return m_bodyCase == BodyCase::kPresence;
}

void CommunicationV1::setPresence(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> value)
{
    clearBody();
    new(&m_body.m_presence) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1>(value);
    m_bodyCase = BodyCase::kPresence;
}

void CommunicationV1::releasePresence()
{
    if (hasPresence())
    {
        m_body.m_presence.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> CommunicationV1::getNotification()
{
    if (!hasNotification())
    {
        setNotification(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::NotificationV1>(m_allocator_));
    }
    return m_body.m_notification;
}

bool CommunicationV1::hasNotification() const
{
    return m_bodyCase == BodyCase::kNotification;
}

void CommunicationV1::setNotification(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> value)
{
    clearBody();
    new(&m_body.m_notification) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1>(value);
    m_bodyCase = BodyCase::kNotification;
}

void CommunicationV1::releaseNotification()
{
    if (hasNotification())
    {
        m_body.m_notification.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> CommunicationV1::getChatInitiate()
{
    if (!hasChatInitiate())
    {
        setChatInitiate(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1>(m_allocator_));
    }
    return m_body.m_chatInitiate;
}

bool CommunicationV1::hasChatInitiate() const
{
    return m_bodyCase == BodyCase::kChatInitiate;
}

void CommunicationV1::setChatInitiate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1> value)
{
    clearBody();
    new(&m_body.m_chatInitiate) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateV1>(value);
    m_bodyCase = BodyCase::kChatInitiate;
}

void CommunicationV1::releaseChatInitiate()
{
    if (hasChatInitiate())
    {
        m_body.m_chatInitiate.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> CommunicationV1::getChatLeave()
{
    if (!hasChatLeave())
    {
        setChatLeave(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1>(m_allocator_));
    }
    return m_body.m_chatLeave;
}

bool CommunicationV1::hasChatLeave() const
{
    return m_bodyCase == BodyCase::kChatLeave;
}

void CommunicationV1::setChatLeave(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1> value)
{
    clearBody();
    new(&m_body.m_chatLeave) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeaveV1>(value);
    m_bodyCase = BodyCase::kChatLeave;
}

void CommunicationV1::releaseChatLeave()
{
    if (hasChatLeave())
    {
        m_body.m_chatLeave.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> CommunicationV1::getChatMembersRequest()
{
    if (!hasChatMembersRequest())
    {
        setChatMembersRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1>(m_allocator_));
    }
    return m_body.m_chatMembersRequest;
}

bool CommunicationV1::hasChatMembersRequest() const
{
    return m_bodyCase == BodyCase::kChatMembersRequest;
}

void CommunicationV1::setChatMembersRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1> value)
{
    clearBody();
    new(&m_body.m_chatMembersRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestV1>(value);
    m_bodyCase = BodyCase::kChatMembersRequest;
}

void CommunicationV1::releaseChatMembersRequest()
{
    if (hasChatMembersRequest())
    {
        m_body.m_chatMembersRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1> CommunicationV1::getChatMembers()
{
    if (!hasChatMembers())
    {
        setChatMembers(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1>(m_allocator_));
    }
    return m_body.m_chatMembers;
}

bool CommunicationV1::hasChatMembers() const
{
    return m_bodyCase == BodyCase::kChatMembers;
}

void CommunicationV1::setChatMembers(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1> value)
{
    clearBody();
    new(&m_body.m_chatMembers) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersV1>(value);
    m_bodyCase = BodyCase::kChatMembers;
}

void CommunicationV1::releaseChatMembers()
{
    if (hasChatMembers())
    {
        m_body.m_chatMembers.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1> CommunicationV1::getError()
{
    if (!hasError())
    {
        setError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ErrorV1>(m_allocator_));
    }
    return m_body.m_error;
}

bool CommunicationV1::hasError() const
{
    return m_bodyCase == BodyCase::kError;
}

void CommunicationV1::setError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1> value)
{
    clearBody();
    new(&m_body.m_error) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ErrorV1>(value);
    m_bodyCase = BodyCase::kError;
}

void CommunicationV1::releaseError()
{
    if (hasError())
    {
        m_body.m_error.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1> CommunicationV1::getReconnectRequest()
{
    if (!hasReconnectRequest())
    {
        setReconnectRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1>(m_allocator_));
    }
    return m_body.m_reconnectRequest;
}

bool CommunicationV1::hasReconnectRequest() const
{
    return m_bodyCase == BodyCase::kReconnectRequest;
}

void CommunicationV1::setReconnectRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1> value)
{
    clearBody();
    new(&m_body.m_reconnectRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ReconnectRequestV1>(value);
    m_bodyCase = BodyCase::kReconnectRequest;
}

void CommunicationV1::releaseReconnectRequest()
{
    if (hasReconnectRequest())
    {
        m_body.m_reconnectRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1> CommunicationV1::getSuccess()
{
    if (!hasSuccess())
    {
        setSuccess(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SuccessV1>(m_allocator_));
    }
    return m_body.m_success;
}

bool CommunicationV1::hasSuccess() const
{
    return m_bodyCase == BodyCase::kSuccess;
}

void CommunicationV1::setSuccess(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1> value)
{
    clearBody();
    new(&m_body.m_success) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SuccessV1>(value);
    m_bodyCase = BodyCase::kSuccess;
}

void CommunicationV1::releaseSuccess()
{
    if (hasSuccess())
    {
        m_body.m_success.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> CommunicationV1::getPointToPointMessage()
{
    if (!hasPointToPointMessage())
    {
        setPointToPointMessage(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1>(m_allocator_));
    }
    return m_body.m_pointToPointMessage;
}

bool CommunicationV1::hasPointToPointMessage() const
{
    return m_bodyCase == BodyCase::kPointToPointMessage;
}

void CommunicationV1::setPointToPointMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1> value)
{
    clearBody();
    new(&m_body.m_pointToPointMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PointToPointMessageV1>(value);
    m_bodyCase = BodyCase::kPointToPointMessage;
}

void CommunicationV1::releasePointToPointMessage()
{
    if (hasPointToPointMessage())
    {
        m_body.m_pointToPointMessage.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1> CommunicationV1::getChatChannelsRequest()
{
    if (!hasChatChannelsRequest())
    {
        setChatChannelsRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1>(m_allocator_));
    }
    return m_body.m_chatChannelsRequest;
}

bool CommunicationV1::hasChatChannelsRequest() const
{
    return m_bodyCase == BodyCase::kChatChannelsRequest;
}

void CommunicationV1::setChatChannelsRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1> value)
{
    clearBody();
    new(&m_body.m_chatChannelsRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV1>(value);
    m_bodyCase = BodyCase::kChatChannelsRequest;
}

void CommunicationV1::releaseChatChannelsRequest()
{
    if (hasChatChannelsRequest())
    {
        m_body.m_chatChannelsRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1> CommunicationV1::getChatChannels()
{
    if (!hasChatChannels())
    {
        setChatChannels(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1>(m_allocator_));
    }
    return m_body.m_chatChannels;
}

bool CommunicationV1::hasChatChannels() const
{
    return m_bodyCase == BodyCase::kChatChannels;
}

void CommunicationV1::setChatChannels(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1> value)
{
    clearBody();
    new(&m_body.m_chatChannels) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsV1>(value);
    m_bodyCase = BodyCase::kChatChannels;
}

void CommunicationV1::releaseChatChannels()
{
    if (hasChatChannels())
    {
        m_body.m_chatChannels.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1> CommunicationV1::getLoginRequest()
{
    if (!hasLoginRequest())
    {
        setLoginRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1>(m_allocator_));
    }
    return m_body.m_loginRequest;
}

bool CommunicationV1::hasLoginRequest() const
{
    return m_bodyCase == BodyCase::kLoginRequest;
}

void CommunicationV1::setLoginRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1> value)
{
    clearBody();
    new(&m_body.m_loginRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV1>(value);
    m_bodyCase = BodyCase::kLoginRequest;
}

void CommunicationV1::releaseLoginRequest()
{
    if (hasLoginRequest())
    {
        m_body.m_loginRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1> CommunicationV1::getHeartbeat()
{
    if (!hasHeartbeat())
    {
        setHeartbeat(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1>(m_allocator_));
    }
    return m_body.m_heartbeat;
}

bool CommunicationV1::hasHeartbeat() const
{
    return m_bodyCase == BodyCase::kHeartbeat;
}

void CommunicationV1::setHeartbeat(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1> value)
{
    clearBody();
    new(&m_body.m_heartbeat) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::HeartbeatV1>(value);
    m_bodyCase = BodyCase::kHeartbeat;
}

void CommunicationV1::releaseHeartbeat()
{
    if (hasHeartbeat())
    {
        m_body.m_heartbeat.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1> CommunicationV1::getWorldChatAssign()
{
    if (!hasWorldChatAssign())
    {
        setWorldChatAssign(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1>(m_allocator_));
    }
    return m_body.m_worldChatAssign;
}

bool CommunicationV1::hasWorldChatAssign() const
{
    return m_bodyCase == BodyCase::kWorldChatAssign;
}

void CommunicationV1::setWorldChatAssign(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1> value)
{
    clearBody();
    new(&m_body.m_worldChatAssign) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatAssignV1>(value);
    m_bodyCase = BodyCase::kWorldChatAssign;
}

void CommunicationV1::releaseWorldChatAssign()
{
    if (hasWorldChatAssign())
    {
        m_body.m_worldChatAssign.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1> CommunicationV1::getWorldChatResponse()
{
    if (!hasWorldChatResponse())
    {
        setWorldChatResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1>(m_allocator_));
    }
    return m_body.m_worldChatResponse;
}

bool CommunicationV1::hasWorldChatResponse() const
{
    return m_bodyCase == BodyCase::kWorldChatResponse;
}

void CommunicationV1::setWorldChatResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1> value)
{
    clearBody();
    new(&m_body.m_worldChatResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatResponseV1>(value);
    m_bodyCase = BodyCase::kWorldChatResponse;
}

void CommunicationV1::releaseWorldChatResponse()
{
    if (hasWorldChatResponse())
    {
        m_body.m_worldChatResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1> CommunicationV1::getMuteUser()
{
    if (!hasMuteUser())
    {
        setMuteUser(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::MuteUserV1>(m_allocator_));
    }
    return m_body.m_muteUser;
}

bool CommunicationV1::hasMuteUser() const
{
    return m_bodyCase == BodyCase::kMuteUser;
}

void CommunicationV1::setMuteUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1> value)
{
    clearBody();
    new(&m_body.m_muteUser) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MuteUserV1>(value);
    m_bodyCase = BodyCase::kMuteUser;
}

void CommunicationV1::releaseMuteUser()
{
    if (hasMuteUser())
    {
        m_body.m_muteUser.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1> CommunicationV1::getUnmuteUser()
{
    if (!hasUnmuteUser())
    {
        setUnmuteUser(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1>(m_allocator_));
    }
    return m_body.m_unmuteUser;
}

bool CommunicationV1::hasUnmuteUser() const
{
    return m_bodyCase == BodyCase::kUnmuteUser;
}

void CommunicationV1::setUnmuteUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1> value)
{
    clearBody();
    new(&m_body.m_unmuteUser) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UnmuteUserV1>(value);
    m_bodyCase = BodyCase::kUnmuteUser;
}

void CommunicationV1::releaseUnmuteUser()
{
    if (hasUnmuteUser())
    {
        m_body.m_unmuteUser.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1> CommunicationV1::getUserMembershipChange()
{
    if (!hasUserMembershipChange())
    {
        setUserMembershipChange(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1>(m_allocator_));
    }
    return m_body.m_userMembershipChange;
}

bool CommunicationV1::hasUserMembershipChange() const
{
    return m_bodyCase == BodyCase::kUserMembershipChange;
}

void CommunicationV1::setUserMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1> value)
{
    clearBody();
    new(&m_body.m_userMembershipChange) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UserMembershipChangeV1>(value);
    m_bodyCase = BodyCase::kUserMembershipChange;
}

void CommunicationV1::releaseUserMembershipChange()
{
    if (hasUserMembershipChange())
    {
        m_body.m_userMembershipChange.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1> CommunicationV1::getWorldChatConfigurationRequestV1()
{
    if (!hasWorldChatConfigurationRequestV1())
    {
        setWorldChatConfigurationRequestV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1>(m_allocator_));
    }
    return m_body.m_worldChatConfigurationRequestV1;
}

bool CommunicationV1::hasWorldChatConfigurationRequestV1() const
{
    return m_bodyCase == BodyCase::kWorldChatConfigurationRequestV1;
}

void CommunicationV1::setWorldChatConfigurationRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1> value)
{
    clearBody();
    new(&m_body.m_worldChatConfigurationRequestV1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationRequestV1>(value);
    m_bodyCase = BodyCase::kWorldChatConfigurationRequestV1;
}

void CommunicationV1::releaseWorldChatConfigurationRequestV1()
{
    if (hasWorldChatConfigurationRequestV1())
    {
        m_body.m_worldChatConfigurationRequestV1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1> CommunicationV1::getWorldChatConfigurationResponse()
{
    if (!hasWorldChatConfigurationResponse())
    {
        setWorldChatConfigurationResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1>(m_allocator_));
    }
    return m_body.m_worldChatConfigurationResponse;
}

bool CommunicationV1::hasWorldChatConfigurationResponse() const
{
    return m_bodyCase == BodyCase::kWorldChatConfigurationResponse;
}

void CommunicationV1::setWorldChatConfigurationResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1> value)
{
    clearBody();
    new(&m_body.m_worldChatConfigurationResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatConfigurationResponseV1>(value);
    m_bodyCase = BodyCase::kWorldChatConfigurationResponse;
}

void CommunicationV1::releaseWorldChatConfigurationResponse()
{
    if (hasWorldChatConfigurationResponse())
    {
        m_body.m_worldChatConfigurationResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1> CommunicationV1::getWorldChatChannelsRequestV1()
{
    if (!hasWorldChatChannelsRequestV1())
    {
        setWorldChatChannelsRequestV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1>(m_allocator_));
    }
    return m_body.m_worldChatChannelsRequestV1;
}

bool CommunicationV1::hasWorldChatChannelsRequestV1() const
{
    return m_bodyCase == BodyCase::kWorldChatChannelsRequestV1;
}

void CommunicationV1::setWorldChatChannelsRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1> value)
{
    clearBody();
    new(&m_body.m_worldChatChannelsRequestV1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsRequestV1>(value);
    m_bodyCase = BodyCase::kWorldChatChannelsRequestV1;
}

void CommunicationV1::releaseWorldChatChannelsRequestV1()
{
    if (hasWorldChatChannelsRequestV1())
    {
        m_body.m_worldChatChannelsRequestV1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1> CommunicationV1::getWorldChatChannelsResponse()
{
    if (!hasWorldChatChannelsResponse())
    {
        setWorldChatChannelsResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1>(m_allocator_));
    }
    return m_body.m_worldChatChannelsResponse;
}

bool CommunicationV1::hasWorldChatChannelsResponse() const
{
    return m_bodyCase == BodyCase::kWorldChatChannelsResponse;
}

void CommunicationV1::setWorldChatChannelsResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1> value)
{
    clearBody();
    new(&m_body.m_worldChatChannelsResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatChannelsResponseV1>(value);
    m_bodyCase = BodyCase::kWorldChatChannelsResponse;
}

void CommunicationV1::releaseWorldChatChannelsResponse()
{
    if (hasWorldChatChannelsResponse())
    {
        m_body.m_worldChatChannelsResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1> CommunicationV1::getPromoteStickyMessageRequest()
{
    if (!hasPromoteStickyMessageRequest())
    {
        setPromoteStickyMessageRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1>(m_allocator_));
    }
    return m_body.m_promoteStickyMessageRequest;
}

bool CommunicationV1::hasPromoteStickyMessageRequest() const
{
    return m_bodyCase == BodyCase::kPromoteStickyMessageRequest;
}

void CommunicationV1::setPromoteStickyMessageRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1> value)
{
    clearBody();
    new(&m_body.m_promoteStickyMessageRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PromoteStickyMessageRequestV1>(value);
    m_bodyCase = BodyCase::kPromoteStickyMessageRequest;
}

void CommunicationV1::releasePromoteStickyMessageRequest()
{
    if (hasPromoteStickyMessageRequest())
    {
        m_body.m_promoteStickyMessageRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1> CommunicationV1::getRemoveStickyMessageRequest()
{
    if (!hasRemoveStickyMessageRequest())
    {
        setRemoveStickyMessageRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1>(m_allocator_));
    }
    return m_body.m_removeStickyMessageRequest;
}

bool CommunicationV1::hasRemoveStickyMessageRequest() const
{
    return m_bodyCase == BodyCase::kRemoveStickyMessageRequest;
}

void CommunicationV1::setRemoveStickyMessageRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1> value)
{
    clearBody();
    new(&m_body.m_removeStickyMessageRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RemoveStickyMessageRequestV1>(value);
    m_bodyCase = BodyCase::kRemoveStickyMessageRequest;
}

void CommunicationV1::releaseRemoveStickyMessageRequest()
{
    if (hasRemoveStickyMessageRequest())
    {
        m_body.m_removeStickyMessageRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1> CommunicationV1::getFetchStickyMessagesRequest()
{
    if (!hasFetchStickyMessagesRequest())
    {
        setFetchStickyMessagesRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1>(m_allocator_));
    }
    return m_body.m_fetchStickyMessagesRequest;
}

bool CommunicationV1::hasFetchStickyMessagesRequest() const
{
    return m_bodyCase == BodyCase::kFetchStickyMessagesRequest;
}

void CommunicationV1::setFetchStickyMessagesRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1> value)
{
    clearBody();
    new(&m_body.m_fetchStickyMessagesRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FetchStickyMessagesRequestV1>(value);
    m_bodyCase = BodyCase::kFetchStickyMessagesRequest;
}

void CommunicationV1::releaseFetchStickyMessagesRequest()
{
    if (hasFetchStickyMessagesRequest())
    {
        m_body.m_fetchStickyMessagesRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1> CommunicationV1::getStickyMessageResponse()
{
    if (!hasStickyMessageResponse())
    {
        setStickyMessageResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1>(m_allocator_));
    }
    return m_body.m_stickyMessageResponse;
}

bool CommunicationV1::hasStickyMessageResponse() const
{
    return m_bodyCase == BodyCase::kStickyMessageResponse;
}

void CommunicationV1::setStickyMessageResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1> value)
{
    clearBody();
    new(&m_body.m_stickyMessageResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageResponseV1>(value);
    m_bodyCase = BodyCase::kStickyMessageResponse;
}

void CommunicationV1::releaseStickyMessageResponse()
{
    if (hasStickyMessageResponse())
    {
        m_body.m_stickyMessageResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest> CommunicationV1::getRolesRequest()
{
    if (!hasRolesRequest())
    {
        setRolesRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest>(m_allocator_));
    }
    return m_body.m_rolesRequest;
}

bool CommunicationV1::hasRolesRequest() const
{
    return m_bodyCase == BodyCase::kRolesRequest;
}

void CommunicationV1::setRolesRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest> value)
{
    clearBody();
    new(&m_body.m_rolesRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesRequest>(value);
    m_bodyCase = BodyCase::kRolesRequest;
}

void CommunicationV1::releaseRolesRequest()
{
    if (hasRolesRequest())
    {
        m_body.m_rolesRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse> CommunicationV1::getRolesResponse()
{
    if (!hasRolesResponse())
    {
        setRolesResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse>(m_allocator_));
    }
    return m_body.m_rolesResponse;
}

bool CommunicationV1::hasRolesResponse() const
{
    return m_bodyCase == BodyCase::kRolesResponse;
}

void CommunicationV1::setRolesResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse> value)
{
    clearBody();
    new(&m_body.m_rolesResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetRolesResponse>(value);
    m_bodyCase = BodyCase::kRolesResponse;
}

void CommunicationV1::releaseRolesResponse()
{
    if (hasRolesResponse())
    {
        m_body.m_rolesResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1> CommunicationV1::getPreferenceRequest()
{
    if (!hasPreferenceRequest())
    {
        setPreferenceRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1>(m_allocator_));
    }
    return m_body.m_preferenceRequest;
}

bool CommunicationV1::hasPreferenceRequest() const
{
    return m_bodyCase == BodyCase::kPreferenceRequest;
}

void CommunicationV1::setPreferenceRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1> value)
{
    clearBody();
    new(&m_body.m_preferenceRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::UpdatePreferenceRequestV1>(value);
    m_bodyCase = BodyCase::kPreferenceRequest;
}

void CommunicationV1::releasePreferenceRequest()
{
    if (hasPreferenceRequest())
    {
        m_body.m_preferenceRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1> CommunicationV1::getGetPreferenceRequest()
{
    if (!hasGetPreferenceRequest())
    {
        setGetPreferenceRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1>(m_allocator_));
    }
    return m_body.m_getPreferenceRequest;
}

bool CommunicationV1::hasGetPreferenceRequest() const
{
    return m_bodyCase == BodyCase::kGetPreferenceRequest;
}

void CommunicationV1::setGetPreferenceRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1> value)
{
    clearBody();
    new(&m_body.m_getPreferenceRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceRequestV1>(value);
    m_bodyCase = BodyCase::kGetPreferenceRequest;
}

void CommunicationV1::releaseGetPreferenceRequest()
{
    if (hasGetPreferenceRequest())
    {
        m_body.m_getPreferenceRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1> CommunicationV1::getGetPreferenceResponse()
{
    if (!hasGetPreferenceResponse())
    {
        setGetPreferenceResponse(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1>(m_allocator_));
    }
    return m_body.m_getPreferenceResponse;
}

bool CommunicationV1::hasGetPreferenceResponse() const
{
    return m_bodyCase == BodyCase::kGetPreferenceResponse;
}

void CommunicationV1::setGetPreferenceResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1> value)
{
    clearBody();
    new(&m_body.m_getPreferenceResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::GetPreferenceResponseV1>(value);
    m_bodyCase = BodyCase::kGetPreferenceResponse;
}

void CommunicationV1::releaseGetPreferenceResponse()
{
    if (hasGetPreferenceResponse())
    {
        m_body.m_getPreferenceResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2> CommunicationV1::getLoginRequestV2()
{
    if (!hasLoginRequestV2())
    {
        setLoginRequestV2(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2>(m_allocator_));
    }
    return m_body.m_loginRequestV2;
}

bool CommunicationV1::hasLoginRequestV2() const
{
    return m_bodyCase == BodyCase::kLoginRequestV2;
}

void CommunicationV1::setLoginRequestV2(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2> value)
{
    clearBody();
    new(&m_body.m_loginRequestV2) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV2>(value);
    m_bodyCase = BodyCase::kLoginRequestV2;
}

void CommunicationV1::releaseLoginRequestV2()
{
    if (hasLoginRequestV2())
    {
        m_body.m_loginRequestV2.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> CommunicationV1::getSessionNotification()
{
    if (!hasSessionNotification())
    {
        setSessionNotification(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1>(m_allocator_));
    }
    return m_body.m_sessionNotification;
}

bool CommunicationV1::hasSessionNotification() const
{
    return m_bodyCase == BodyCase::kSessionNotification;
}

void CommunicationV1::setSessionNotification(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> value)
{
    clearBody();
    new(&m_body.m_sessionNotification) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1>(value);
    m_bodyCase = BodyCase::kSessionNotification;
}

void CommunicationV1::releaseSessionNotification()
{
    if (hasSessionNotification())
    {
        m_body.m_sessionNotification.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> CommunicationV1::getChatTypingEventRequest()
{
    if (!hasChatTypingEventRequest())
    {
        setChatTypingEventRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1>(m_allocator_));
    }
    return m_body.m_chatTypingEventRequest;
}

bool CommunicationV1::hasChatTypingEventRequest() const
{
    return m_bodyCase == BodyCase::kChatTypingEventRequest;
}

void CommunicationV1::setChatTypingEventRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1> value)
{
    clearBody();
    new(&m_body.m_chatTypingEventRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestV1>(value);
    m_bodyCase = BodyCase::kChatTypingEventRequest;
}

void CommunicationV1::releaseChatTypingEventRequest()
{
    if (hasChatTypingEventRequest())
    {
        m_body.m_chatTypingEventRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> CommunicationV1::getChatChannelsRequestV2()
{
    if (!hasChatChannelsRequestV2())
    {
        setChatChannelsRequestV2(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2>(m_allocator_));
    }
    return m_body.m_chatChannelsRequestV2;
}

bool CommunicationV1::hasChatChannelsRequestV2() const
{
    return m_bodyCase == BodyCase::kChatChannelsRequestV2;
}

void CommunicationV1::setChatChannelsRequestV2(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2> value)
{
    clearBody();
    new(&m_body.m_chatChannelsRequestV2) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelsRequestV2>(value);
    m_bodyCase = BodyCase::kChatChannelsRequestV2;
}

void CommunicationV1::releaseChatChannelsRequestV2()
{
    if (hasChatChannelsRequestV2())
    {
        m_body.m_chatChannelsRequestV2.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> CommunicationV1::getChatChannelUpdate()
{
    if (!hasChatChannelUpdate())
    {
        setChatChannelUpdate(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1>(m_allocator_));
    }
    return m_body.m_chatChannelUpdate;
}

bool CommunicationV1::hasChatChannelUpdate() const
{
    return m_bodyCase == BodyCase::kChatChannelUpdate;
}

void CommunicationV1::setChatChannelUpdate(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1> value)
{
    clearBody();
    new(&m_body.m_chatChannelUpdate) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatChannelUpdateV1>(value);
    m_bodyCase = BodyCase::kChatChannelUpdate;
}

void CommunicationV1::releaseChatChannelUpdate()
{
    if (hasChatChannelUpdate())
    {
        m_body.m_chatChannelUpdate.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1> CommunicationV1::getServerConfigRequest()
{
    if (!hasServerConfigRequest())
    {
        setServerConfigRequest(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1>(m_allocator_));
    }
    return m_body.m_serverConfigRequest;
}

bool CommunicationV1::hasServerConfigRequest() const
{
    return m_bodyCase == BodyCase::kServerConfigRequest;
}

void CommunicationV1::setServerConfigRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1> value)
{
    clearBody();
    new(&m_body.m_serverConfigRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigRequestV1>(value);
    m_bodyCase = BodyCase::kServerConfigRequest;
}

void CommunicationV1::releaseServerConfigRequest()
{
    if (hasServerConfigRequest())
    {
        m_body.m_serverConfigRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1> CommunicationV1::getServerConfig()
{
    if (!hasServerConfig())
    {
        setServerConfig(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1>(m_allocator_));
    }
    return m_body.m_serverConfig;
}

bool CommunicationV1::hasServerConfig() const
{
    return m_bodyCase == BodyCase::kServerConfig;
}

void CommunicationV1::setServerConfig(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1> value)
{
    clearBody();
    new(&m_body.m_serverConfig) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ServerConfigV1>(value);
    m_bodyCase = BodyCase::kServerConfig;
}

void CommunicationV1::releaseServerConfig()
{
    if (hasServerConfig())
    {
        m_body.m_serverConfig.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3> CommunicationV1::getLoginRequestV3()
{
    if (!hasLoginRequestV3())
    {
        setLoginRequestV3(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3>(m_allocator_));
    }
    return m_body.m_loginRequestV3;
}

bool CommunicationV1::hasLoginRequestV3() const
{
    return m_bodyCase == BodyCase::kLoginRequestV3;
}

void CommunicationV1::setLoginRequestV3(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3> value)
{
    clearBody();
    new(&m_body.m_loginRequestV3) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginRequestV3>(value);
    m_bodyCase = BodyCase::kLoginRequestV3;
}

void CommunicationV1::releaseLoginRequestV3()
{
    if (hasLoginRequestV3())
    {
        m_body.m_loginRequestV3.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1> CommunicationV1::getSessionRequestV1()
{
    if (!hasSessionRequestV1())
    {
        setSessionRequestV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1>(m_allocator_));
    }
    return m_body.m_sessionRequestV1;
}

bool CommunicationV1::hasSessionRequestV1() const
{
    return m_bodyCase == BodyCase::kSessionRequestV1;
}

void CommunicationV1::setSessionRequestV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1> value)
{
    clearBody();
    new(&m_body.m_sessionRequestV1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionRequestV1>(value);
    m_bodyCase = BodyCase::kSessionRequestV1;
}

void CommunicationV1::releaseSessionRequestV1()
{
    if (hasSessionRequestV1())
    {
        m_body.m_sessionRequestV1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> CommunicationV1::getSessionResponseV1()
{
    if (!hasSessionResponseV1())
    {
        setSessionResponseV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1>(m_allocator_));
    }
    return m_body.m_sessionResponseV1;
}

bool CommunicationV1::hasSessionResponseV1() const
{
    return m_bodyCase == BodyCase::kSessionResponseV1;
}

void CommunicationV1::setSessionResponseV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> value)
{
    clearBody();
    new(&m_body.m_sessionResponseV1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1>(value);
    m_bodyCase = BodyCase::kSessionResponseV1;
}

void CommunicationV1::releaseSessionResponseV1()
{
    if (hasSessionResponseV1())
    {
        m_body.m_sessionResponseV1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1> CommunicationV1::getSessionCleanupV1()
{
    if (!hasSessionCleanupV1())
    {
        setSessionCleanupV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1>(m_allocator_));
    }
    return m_body.m_sessionCleanupV1;
}

bool CommunicationV1::hasSessionCleanupV1() const
{
    return m_bodyCase == BodyCase::kSessionCleanupV1;
}

void CommunicationV1::setSessionCleanupV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1> value)
{
    clearBody();
    new(&m_body.m_sessionCleanupV1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionCleanupV1>(value);
    m_bodyCase = BodyCase::kSessionCleanupV1;
}

void CommunicationV1::releaseSessionCleanupV1()
{
    if (hasSessionCleanupV1())
    {
        m_body.m_sessionCleanupV1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void CommunicationV1::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kPresenceSubscribe:
            m_body.m_presenceSubscribe.reset();
            break;
        case BodyCase::kPresenceUnsubscribe:
            m_body.m_presenceUnsubscribe.reset();
            break;
        case BodyCase::kPresenceSubscriptionError:
            m_body.m_presenceSubscriptionError.reset();
            break;
        case BodyCase::kPresenceUpdate:
            m_body.m_presenceUpdate.reset();
            break;
        case BodyCase::kPresenceUpdateError:
            m_body.m_presenceUpdateError.reset();
            break;
        case BodyCase::kPresence:
            m_body.m_presence.reset();
            break;
        case BodyCase::kNotification:
            m_body.m_notification.reset();
            break;
        case BodyCase::kChatInitiate:
            m_body.m_chatInitiate.reset();
            break;
        case BodyCase::kChatLeave:
            m_body.m_chatLeave.reset();
            break;
        case BodyCase::kChatMembersRequest:
            m_body.m_chatMembersRequest.reset();
            break;
        case BodyCase::kChatMembers:
            m_body.m_chatMembers.reset();
            break;
        case BodyCase::kError:
            m_body.m_error.reset();
            break;
        case BodyCase::kReconnectRequest:
            m_body.m_reconnectRequest.reset();
            break;
        case BodyCase::kSuccess:
            m_body.m_success.reset();
            break;
        case BodyCase::kPointToPointMessage:
            m_body.m_pointToPointMessage.reset();
            break;
        case BodyCase::kChatChannelsRequest:
            m_body.m_chatChannelsRequest.reset();
            break;
        case BodyCase::kChatChannels:
            m_body.m_chatChannels.reset();
            break;
        case BodyCase::kLoginRequest:
            m_body.m_loginRequest.reset();
            break;
        case BodyCase::kHeartbeat:
            m_body.m_heartbeat.reset();
            break;
        case BodyCase::kWorldChatAssign:
            m_body.m_worldChatAssign.reset();
            break;
        case BodyCase::kWorldChatResponse:
            m_body.m_worldChatResponse.reset();
            break;
        case BodyCase::kMuteUser:
            m_body.m_muteUser.reset();
            break;
        case BodyCase::kUnmuteUser:
            m_body.m_unmuteUser.reset();
            break;
        case BodyCase::kUserMembershipChange:
            m_body.m_userMembershipChange.reset();
            break;
        case BodyCase::kWorldChatConfigurationRequestV1:
            m_body.m_worldChatConfigurationRequestV1.reset();
            break;
        case BodyCase::kWorldChatConfigurationResponse:
            m_body.m_worldChatConfigurationResponse.reset();
            break;
        case BodyCase::kWorldChatChannelsRequestV1:
            m_body.m_worldChatChannelsRequestV1.reset();
            break;
        case BodyCase::kWorldChatChannelsResponse:
            m_body.m_worldChatChannelsResponse.reset();
            break;
        case BodyCase::kPromoteStickyMessageRequest:
            m_body.m_promoteStickyMessageRequest.reset();
            break;
        case BodyCase::kRemoveStickyMessageRequest:
            m_body.m_removeStickyMessageRequest.reset();
            break;
        case BodyCase::kFetchStickyMessagesRequest:
            m_body.m_fetchStickyMessagesRequest.reset();
            break;
        case BodyCase::kStickyMessageResponse:
            m_body.m_stickyMessageResponse.reset();
            break;
        case BodyCase::kRolesRequest:
            m_body.m_rolesRequest.reset();
            break;
        case BodyCase::kRolesResponse:
            m_body.m_rolesResponse.reset();
            break;
        case BodyCase::kPreferenceRequest:
            m_body.m_preferenceRequest.reset();
            break;
        case BodyCase::kGetPreferenceRequest:
            m_body.m_getPreferenceRequest.reset();
            break;
        case BodyCase::kGetPreferenceResponse:
            m_body.m_getPreferenceResponse.reset();
            break;
        case BodyCase::kLoginRequestV2:
            m_body.m_loginRequestV2.reset();
            break;
        case BodyCase::kSessionNotification:
            m_body.m_sessionNotification.reset();
            break;
        case BodyCase::kChatTypingEventRequest:
            m_body.m_chatTypingEventRequest.reset();
            break;
        case BodyCase::kChatChannelsRequestV2:
            m_body.m_chatChannelsRequestV2.reset();
            break;
        case BodyCase::kChatChannelUpdate:
            m_body.m_chatChannelUpdate.reset();
            break;
        case BodyCase::kServerConfigRequest:
            m_body.m_serverConfigRequest.reset();
            break;
        case BodyCase::kServerConfig:
            m_body.m_serverConfig.reset();
            break;
        case BodyCase::kLoginRequestV3:
            m_body.m_loginRequestV3.reset();
            break;
        case BodyCase::kSessionRequestV1:
            m_body.m_sessionRequestV1.reset();
            break;
        case BodyCase::kSessionResponseV1:
            m_body.m_sessionResponseV1.reset();
            break;
        case BodyCase::kSessionCleanupV1:
            m_body.m_sessionCleanupV1.reset();
            break;
        case BodyCase::kNotSet:
        default:
            break;
    }
    m_bodyCase = BodyCase::kNotSet;
}


}
}
}
}
}
}
