#include "stdafx.h"
#include "OriginEventClasses.h"
#include "OriginErrorFunctions.h"
#include "OriginSDK/OriginTypes.h"
#include "OriginSDK/OriginError.h"
#include <stdlib.h>
#include <assert.h>

namespace Origin
{

//----------------------------------------------------------------------
// template function and macro to reduce initialization boilerplate
template <int _INDEX, typename _EVENTHANDLERTYPE>
void InitEventHandler (OriginSDK& sdk, Origin::IEventHandler** handlers)
{
	handlers[_INDEX] = new _EVENTHANDLERTYPE(_INDEX);
	handlers[_INDEX]->RegisterMessageHandler(sdk);
}


//----------------------------------------------------------------------

void OriginSDK::InitializeEvents ()
{
	InitEventHandler<ORIGIN_EVENT_IGO, Origin::Event::IGO>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_INVITE, Origin::Event::Invite>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_LOGIN, Origin::Event::Login>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_PROFILE, Origin::Event::Profile>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_PRESENCE, Origin::Event::Presence>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_FRIENDS, Origin::Event::Friends>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_PURCHASE, Origin::Event::Purchase>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_CONTENT, Origin::Event::Content>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_BLOCKED, Origin::Event::Blocked>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_CHAT_MESSAGE, Origin::Event::ChatMessage>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_ONLINE, Origin::Event::Online>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_ACHIEVEMENT, Origin::Event::Achievement>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_INVITE_PENDING, Origin::Event::InvitePending>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_VISIBILITY, Origin::Event::Visibility>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_BROADCAST, Origin::Event::Broadcast>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_CURRENT_USER_PRESENCE, Origin::Event::CurrentUserPresence>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_NEW_ITEMS, Origin::Event::NewItems>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_CHUNK_STATUS, Origin::Event::ChunkProgress>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_IGO_UNAVAILABLE, Origin::Event::IGOUnavailable>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_MINIMIZE_REQUEST, Origin::Event::MinimizeRequest>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_RESTORE_REQUEST, Origin::Event::RestoreRequest>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_GAME_MESSAGE, Origin::Event::GameMessage>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_GROUP, Origin::Event::Group>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_GROUP_ENTER, Origin::Event::GroupEnter>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_GROUP_LEAVE, Origin::Event::GroupLeave>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_GROUP_INVITE, Origin::Event::GroupInvite>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_VOIP_STATUS, Origin::Event::Voip>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_USER_INVITED, Origin::Event::UserInvited>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_CHAT_STATE_UPDATED, Origin::Event::ChatStateUpdated>(*this, m_eventHandlers);
    InitEventHandler<ORIGIN_EVENT_STEAM_STORE, Origin::Event::OpenSteamStore>(*this, m_eventHandlers);
	InitEventHandler<ORIGIN_EVENT_STEAM_ACHIEVEMENT, Origin::Event::SteamAchievement>(*this, m_eventHandlers);
}


void OriginSDK::ShutdownEvents ()
{
	for(int i=0; i<ORIGIN_NUM_EVENTS; i++)
	{
		if(m_eventHandlers[i] != NULL)
		{
			m_eventHandlers[i]->DeregisterMessageHandler(*this);
			m_eventHandlers[i] = NULL;
		}
	}
}

OriginErrorT OriginSDK::RegisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* context)
{
    if(iEvents == ORIGIN_EVENT_FALLBACK)
    {
        for (uint32_t index = 0; index < ORIGIN_NUM_EVENTS; ++index)
        {
            m_eventHandlers[index]->SetEventFallbackCallback(callback, context);
        }
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        if(iEvents>=0 && iEvents<ORIGIN_NUM_EVENTS)
        {
            if(callback != NULL)
            {
                return m_eventHandlers[iEvents]->AddEventCallback(callback, context) ? REPORTERROR(ORIGIN_SUCCESS) : REPORTERROR(ORIGIN_ERROR_ALREADY_EXISTS);
            }
            else
            {
                return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
            }
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_NOT_FOUND);
        }
    }
}

OriginErrorT OriginSDK::UnregisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* context)
{
    if(iEvents == ORIGIN_EVENT_FALLBACK)
    {
        for (uint32_t index = 0; index < ORIGIN_NUM_EVENTS; ++index)
        {
            m_eventHandlers[index]->ResetEventFallbackCallback();
        }
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        if(iEvents>=0 && iEvents<ORIGIN_NUM_EVENTS)
        {
            return m_eventHandlers[iEvents]->RemoveEventCallback(callback, context) ? REPORTERROR(ORIGIN_SUCCESS) : REPORTERROR(ORIGIN_ERROR_NOT_FOUND);
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_NOT_FOUND);
        }
    }
}


namespace Event
{
    //---------------------------------------------------------------------------------------

    void Broadcast::TypeConvert (const lsx::BroadcastEventT& input, size_t /*index*/, uint32_t& output)
    {
        output = input.State;
    }

	//---------------------------------------------------------------------------------------

	void IGO::TypeConvert (const lsx::IGOEventT& input, size_t /*index*/, uint32_t& output)
	{
		output = input.State;
	}

	//---------------------------------------------------------------------------------------

	void Invite::TypeConvert (const lsx::MultiplayerInviteT& input, size_t /*index*/, OriginInviteT& output)
	{
		output.FromId = input.FromId;
		output.bInitial = input.Initial;
        output.MultiplayerId = input.MultiplayerId.c_str();
		output.SessionInformation = input.SessionInformation.c_str();
        output.GroupName = input.GroupName.c_str();
        output.GroupId = input.GroupId.c_str();
	}

	//---------------------------------------------------------------------------------------

	void Login::TypeConvert (const lsx::LoginT& input, size_t /*index*/, OriginLoginT& output)
	{
		output.IsLoggedIn = input.IsLoggedIn ? ORIGIN_LOGINSTATUS_ONLINE : ORIGIN_LOGINSTATUS_OFFLINE;
        output.UserIndex = input.UserIndex;
        output.LoginReasonCode = (enumLoginReasonCode)input.LoginReasonCode;
	}

	//---------------------------------------------------------------------------------------

	void Profile::TypeConvert (const lsx::ProfileEventT& input, size_t /*index*/, OriginProfileChangeT& output)
	{
		output.Changed = (enumProfileChangeItem)input.Changed;
        output.UserId = input.UserId;
	}

	//---------------------------------------------------------------------------------------

	void Presence::TypeConvert (const lsx::GetPresenceResponseT& input, size_t /*index*/, OriginPresenceT& output)
	{
		output.UserId = input.UserId;
		output.PresenceState = (enumPresence)input.Presence;
		output.Presence = input.RichPresence.c_str();
		output.GamePresence = input.GamePresence.c_str();

		output.Title = input.Title.c_str();
		output.TitleId = input.TitleId.c_str();
		output.MultiplayerId = input.MultiplayerId.c_str();

        output.Group = input.Group.c_str();
        output.GroupId = input.GroupId.c_str();

	}

    //---------------------------------------------------------------------------------------
	void CurrentUserPresence::TypeConvert (const lsx::CurrentUserPresenceEventT& input, size_t /*index*/, OriginPresenceT& output)
	{
		output.UserId = input.UserId;
		output.PresenceState = (enumPresence)input.Presence;
		output.Presence = input.RichPresence.c_str();
		output.GamePresence = input.GamePresence.c_str();

		output.Title = input.Title.c_str();
		output.TitleId = input.TitleId.c_str();
		output.MultiplayerId = input.MultiplayerId.c_str();

        output.Group = input.Group.c_str();
        output.GroupId = input.GroupId.c_str();
	}
	//---------------------------------------------------------------------------------------

	size_t Friends::TypeCount (const lsx::QueryFriendsResponseT& input) const
	{
		return input.GetCount();
	}

	void Friends::Preprocess (lsx::QueryFriendsResponseT& /*input*/) const
	{
		// No preprocessing necessary.
	}

	void Friends::TypeConvert (const lsx::QueryFriendsResponseT& input, size_t index, OriginFriendT & output)
	{
		OriginSDK::ConvertData(output, input.Friends[index]);
	}

	//---------------------------------------------------------------------------------------

	void Purchase::TypeConvert (const lsx::PurchaseEventT& input, size_t /*index*/, const OriginCharT*& output)
	{
		output = input.manifest.c_str();
	}

	//---------------------------------------------------------------------------------------

	void Content::Preprocess (lsx::CoreContentUpdatedT& /*input*/) const
	{
		// remove any <Game> nodes that are about things we don't care about...
		//uint32_t index = 0;
		//while (index < input.GetCount())
		//{
		//	if (input.Games[index].state != lsx::CONTENTSTATE_READY_TO_PLAY)
		//		input.Games.erase(input.Games.begin()+index);
		//	else
		//		index += 1;
		//}
	}

	size_t Content::TypeCount (const lsx::CoreContentUpdatedT& input) const
	{
		return input.GetCount();
	}

	void Content::TypeConvert (const lsx::CoreContentUpdatedT& input, size_t index, OriginContentT& output)
	{
		assert(index < this->TypeCount(input));
		output.ItemId = input.Games[index].contentID.c_str();
		output.Progress = input.Games[index].progressValue;
		output.State = ConvertContentState(input.Games[index].state);
        output.InstalledVersion = input.Games[index].installedVersion.c_str();
        output.AvailableVersion = input.Games[index].availableVersion.c_str();
        output.DisplayName = input.Games[index].displayName.c_str();
    }

	//---------------------------------------------------------------------------------------

	void Blocked::TypeConvert (const lsx::BlockListUpdatedT& /*input*/, size_t /*index*/, uint32_t& output)
	{
		output = 0;
	}

	//---------------------------------------------------------------------------------------

    void ChatMessage::TypeConvert(const lsx::ChatMessageEventT& input, size_t /*index*/, OriginChatMessageT &output)
	{
        output.GroupId = input.GroupId.c_str();
		output.FromId = input.FromId;
		output.Message = input.Message.c_str();
        output.Thread = input.Thread.c_str();
	}

	void Online::TypeConvert( const lsx::OnlineStatusEventT &input, size_t /*index*/, bool &output)
	{
		output = input.isOnline;
	}


    //---------------------------------------------------------------------------------------

    size_t Achievement::TypeCount (const lsx::AchievementSetsT& input) const
    {
        return input.GetCount();
    }

    void Achievement::Preprocess (lsx::AchievementSetsT& /*input*/) const
    {
        // No preprocessing necessary.
    }

    void Achievement::TypeConvert (const lsx::AchievementSetsT& input, size_t index, OriginAchievementSetT & output)
    {
        OriginSDK::ConvertData(this, output, input.AchievementSet[index], false);
    }

    //---------------------------------------------------------------------------------------

    void InvitePending::TypeConvert (const lsx::MultiplayerInvitePendingT& input, size_t /*index*/, OriginInvitePendingT& output)
    {
        output.FromId = input.FromId;
        output.GroupName = input.GroupName.c_str();
        output.GroupId = input.GroupId.c_str();
        output.MultiplayerId = input.MultiplayerId.c_str();
    }

	void Visibility::TypeConvert( const lsx::PresenceVisibilityEventT &input, size_t /*index*/, bool &output)
	{
		output = input.Visible;
	}

    //---------------------------------------------------------------------------------------

    size_t NewItems::TypeCount (const lsx::QueryEntitlementsResponseT& input) const
    {
        return input.GetCount();
    }

    void NewItems::Preprocess (lsx::QueryEntitlementsResponseT& /*input*/) const
    {
        // No preprocessing necessary.
    }

    void NewItems::TypeConvert (const lsx::QueryEntitlementsResponseT& input, size_t index, OriginItemT& output)
    {
        OriginSDK::ConvertData(output, input.Entitlements[index]);
    }


	void ChunkProgress::TypeConvert( const lsx::ChunkStatusT &input, size_t /*index*/, OriginChunkStatusT &output)
	{
		output.ItemId = input.ItemId.c_str();
		output.Name = input.Name.c_str();
		output.ChunkId = input.ChunkId;
		output.Progress = input.Progress;
		output.Size = input.Size;
		output.Type = (enumChunkType)input.Type;
		output.State = (enumChunkState)input.State;
        output.ChunkETA = input.ChunkETA;
        output.TotalETA = input.TotalETA;
	}

	void IGOUnavailable::TypeConvert(const lsx::IGOUnavailableT& input, size_t /*index*/, uint32_t& output)
	{
		output = input.Reason;
	}

    void MinimizeRequest::TypeConvert(const lsx::MinimizeRequestT& input, size_t /*index*/, uint32_t& output)
    {
        EA_UNUSED(input);
        output = 0;
    }

    void RestoreRequest::TypeConvert(const lsx::RestoreRequestT& input, size_t /*index*/, uint32_t& output)
    {
        EA_UNUSED(input);
        output = 0;
    }

    void GameMessage::TypeConvert(const lsx::GameMessageEventT& input, size_t /*index*/, OriginGameMessageT& output)
    {
        output.GameId = input.GameId.c_str();
        output.Message = input.Message.c_str();
    }

    size_t Group::TypeCount(const lsx::GroupEventT& input) const
    {
        return input.GetCount();
    }

    void Group::Preprocess(lsx::GroupEventT& /*input*/) const
    {
        // No preprocessing necessary.
    }

    void Group::TypeConvert(const lsx::GroupEventT& input, size_t index, OriginFriendT & output)
    {
        OriginSDK::ConvertData(output, input.Members[index]);
    }

    void GroupEnter::TypeConvert(const lsx::GroupEnterEventT& input, size_t /*index*/, OriginGroupInfoT& output)
    {
        output.GroupName = input.GroupInfo.GroupName.c_str();
        output.GroupId = input.GroupInfo.GroupId.c_str();
        output.GroupType = (enumGroupType)input.GroupInfo.GroupType;
        output.bUserCanInviteNewMembers = input.GroupInfo.CanInviteNewMembers;
        output.bUserCanRemoveMembers = input.GroupInfo.CanRemoveMembers;
        output.bUserCanSendGameInvites = input.GroupInfo.CanSendGameInvites;
		output.MaxGroupSize = input.GroupInfo.MaxGroupSize;
    }

    void GroupLeave::TypeConvert(const lsx::GroupLeaveEventT& input, size_t /*index*/, const char *& output)
    {
        output = input.GroupId.c_str();
    }

    void GroupInvite::TypeConvert(const lsx::GroupInviteEventT& input, size_t /*index*/, OriginGroupInviteT& output)
    {
        output.GroupName = input.GroupName.c_str();
        output.GroupId = input.GroupId.c_str();
        output.GroupType = (enumGroupType)input.GroupType;
        output.Inviter = input.FromId;
    }

    void Voip::TypeConvert(const lsx::VoipStatusEventT& input, size_t index, OriginVoipStatusEventT& output)
    {
		output.Type = static_cast<enumVoipState>(input.Status);
		output.UserId = input.UserId;
    }

    void UserInvited::TypeConvert(const lsx::UserInvitedEventT& input, size_t index, OriginUserT &output)
    {
        output = input.UserId;
    }

    void ChatStateUpdated::TypeConvert(const lsx::ChatStateUpdateEventT& input, size_t index, OriginChatStateUpdateMessageT &output)
    {
        output.State = static_cast<enumChatState>(input.State);
        output.UserId = input.UserId;
    }

    void OpenSteamStore::TypeConvert(const lsx::SteamActivateOverlayToStoreEventT& input, size_t index, OriginOpenSteamStoreT& output)
    {
        output.Flag = static_cast<enumSteamStoreFlag>(input.Flag);
        output.AppId = input.AppId.c_str();
        output.OfferId = input.OfferId.c_str();
        output.IsBaseGame = input.IsBaseGame;
    }

	void SteamAchievement::TypeConvert(const lsx::SteamAchievementEventT& input, size_t index, SteamAchievementT& output)
	{
		output.AchievementId = input.AchievementId.c_str();
		output.Points = input.Points;
	}

} // namespace Event
} // namespace Origin


