#include "JingleClient.h"
#include "DiscoverableInformation.h"

#include <string>

#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/network.h"
#include "talk/base/socketaddress.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/base/windowpickerfactory.h"

#include "talk/xmpp/constants.h"
#include "talk/xmpp/iqtask.h"
#include "talk/xmpp/mucroomconfigtask.h"
#include "talk/xmpp/mucroomlookuptask.h"
#include "talk/xmpp/pingtask.h"
#include "talk/xmpp/xmppthread.h"
#include "talk/xmpp/xep0115cache.h"
#include "talk/xmpp/xep0115tasks.h"

#include "services/log/LogService.h"
#include "services/platform//PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/config/OriginConfigService.h"

#include "version/version.h"

//Message handlers -- ones that don't need to be tasks
class AcceptFriendRequestMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class CancelFriendRequestMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class RejectFriendRequestMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class SendFriendRequestMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class RemoveFriendMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class RequestRosterMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class SetPresenceMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

class SetDirectedPresenceMessageHandler : public talk_base::MessageData, public talk_base::MessageHandler
{
public:
    void OnMessage(talk_base::Message* msg) override;
};

JingleClient::JingleClient(
    const Origin::Chat::DiscoverableInformation& discoInfo,
    buzz::XmppThread* xmppThread,
    buzz::XmppClient* xmppClient)
    : mXmppThread(xmppThread)
    , mXmppClient(xmppClient)
    , mXmppRosterModule(NULL)
    , mRosterHandler(NULL)
    , mChatroomHandler(NULL)
    , mMucInviteRecvTask(NULL)
    , mMucInviteSendTask(NULL)
    , mMucDeclineRecvTask(NULL)
    , mMucDeclineSendTask(NULL)
    , mMucInstantRoomRequest(NULL)
    , mMucConfigRequest(NULL)
    , mMucConfigResponse(NULL)
    , mMucConfigSubmission(NULL)
    , mMucRoomUnlocked(NULL)
    , mMucGrantMembership(NULL)
    , mMucRevokeMembership(NULL)
    , mMucGrantModerator(NULL)
    , mMucRevokeModerator(NULL)
    , mMucGrantAdmin(NULL)
    , mMucRevokeAdmin(NULL)
    , mMucKickOccupant(NULL)
    , mChatGroupRoomInviteRecvTask(NULL)
    , mChatGroupRoomInviteSendTask(NULL)
    , mBlockListSend(NULL)
    , mBlockListPush(NULL)
    , mBlockListRequest(NULL)
    , mBlockListRecv(NULL)
    , mSetPrivacyList(NULL)
    , mChatReceiveTask(NULL)
    , mChatSendTask(NULL)
    , mPingTask(NULL)
    , mCapabilitiesReceiveTask(NULL)
    , mAcceptFriendRequestMessageHandler(NULL)
    , mCancelFriendRequestMessageHandler(NULL)
    , mRejectFriendRequestMessageHandler(NULL)
    , mSendFriendRequestMessageHandler(NULL)
    , mRemoveFriendMessageHandler(NULL)
    , mRequestRosterMessageHandler(NULL)
    , mSetPresenceMessageHandler(NULL)
    , mSetDirectedPresenceMessageHandler()
    , mEvents(NULL)
    , mDiscoInfo(discoInfo)
    , mCarbonsRequest(NULL)
    , mCarbonsResponse(NULL)
{
    mXmppClient->SignalStateChange.connect(this, &JingleClient::OnStateChange);
}

JingleClient::~JingleClient()
{
    delete mChatroomHandler;
    mChatroomHandler = NULL;

    delete mRosterHandler;
    mRosterHandler = NULL;
}

void JingleClient::OnStateChange(buzz::XmppEngine::State state)
{
    switch (state)
    {
    case buzz::XmppEngine::STATE_START:
        ORIGIN_LOG_EVENT << "Jingle received state start";
        break;
    case buzz::XmppEngine::STATE_OPENING:
        ORIGIN_LOG_EVENT << "Jingle received state opening";
        break;
    case buzz::XmppEngine::STATE_OPEN:
        InitializeJingle();
        mEvents->Connected();
        EnableCarbons();
        ORIGIN_LOG_EVENT << "Jingle received state open";
        break;
    case buzz::XmppEngine::STATE_CLOSED:
    {
        delete mXmppRosterModule;
        mXmppRosterModule = NULL;

        for (auto it = mMucRooms.begin(); it != mMucRooms.end();)
        {
            MucRoomInformation info = *it;
            buzz::XmppChatroomModule* xmppChatRoomModule = info.module;
            it = mMucRooms.erase(it);

            delete xmppChatRoomModule;
        }

        delete mAcceptFriendRequestMessageHandler;
        mAcceptFriendRequestMessageHandler = NULL;

        delete mCancelFriendRequestMessageHandler;
        mCancelFriendRequestMessageHandler = NULL;

        delete mRejectFriendRequestMessageHandler;
        mRejectFriendRequestMessageHandler = NULL;

        delete mSendFriendRequestMessageHandler;
        mSendFriendRequestMessageHandler = NULL;

        delete mRemoveFriendMessageHandler;
        mRemoveFriendMessageHandler = NULL;

        delete mRequestRosterMessageHandler;
        mRequestRosterMessageHandler = NULL;

        delete mSetPresenceMessageHandler;
        mSetPresenceMessageHandler = NULL;

        mSetDirectedPresenceMessageHandler.reset();

        // ONLY NULL THESE, DO NOT DELETE THEM! Jingle deletes these in their taskrunner
        mMucInviteRecvTask = NULL;
        mMucInviteSendTask = NULL;
        mMucDeclineRecvTask = NULL;
        mMucDeclineSendTask = NULL;
        mMucInstantRoomRequest = NULL;
        mMucConfigRequest = NULL;
        mMucConfigResponse = NULL;
        mMucConfigSubmission = NULL;
        mMucRoomUnlocked = NULL;
        mMucGrantMembership = NULL;
        mMucRevokeMembership = NULL;
        mMucGrantModerator = NULL;
        mMucRevokeModerator = NULL;
        mMucGrantAdmin = NULL;
        mMucRevokeAdmin = NULL;
        mMucKickOccupant = NULL;
        mBlockListSend = NULL;
        mBlockListPush = NULL;
        mBlockListRequest = NULL;
        mBlockListRecv = NULL;
        mSetPrivacyList = NULL;
        mChatReceiveTask = NULL;
        mChatSendTask = NULL;
        mPingTask = NULL;
        mCapabilitiesReceiveTask = NULL;
        mCarbonsRequest = NULL;
        mCarbonsResponse = NULL;

        if (mXmppClient->engine()->GetStreamError())
        {
            mEvents->StreamError(*mXmppClient->engine()->GetStreamError());
            ORIGIN_LOG_EVENT << "Jingle received state closed for stream error";
        }
        else
        {
            int subcode = 0;
            buzz::XmppEngine::Error error = mXmppClient->engine()->GetError(&subcode);
            mEvents->Close(error);
            
            ORIGIN_LOG_EVENT << "Jingle received state closed with error " << error;
        }
        break;
    }
    default:
        break;
    }
}

void JingleClient::InitializeJingle()
{
    // NOTE: All of these that we're creating are buzz::XmppTask objects. DO NOT DELETE them. Jingle
    // gets angry on shutdown if we do that, it seems to be cleaning them up on its own
    mSetPrivacyList = new SetPrivacyList(mXmppClient);
    mSetPrivacyList->SignalPrivacyReceived.connect(this,
        &JingleClient::OnPrivacyReceived);
    mSetPrivacyList->Start();

    mMucInviteRecvTask = new MucInviteRecvTask(mXmppClient);
    mMucInviteRecvTask->SignalInviteReceived.connect(this,
        &JingleClient::OnMucInviteReceived);
    mMucInviteRecvTask->Start();

    mMucInviteSendTask = new MucInviteSendTask(mXmppClient);
    mMucInviteSendTask->Start();

    mMucDeclineRecvTask = new MucDeclineRecvTask(mXmppClient);
    mMucDeclineRecvTask->SignalDeclineReceived.connect(this,
        &JingleClient::OnMucDeclineReceived);
    mMucDeclineRecvTask->Start();

    mMucDeclineSendTask = new MucDeclineSendTask(mXmppClient);
    mMucDeclineSendTask->Start();

    mMucInstantRoomRequest = new MucInstantRoomRequest(mXmppClient);
    mMucInstantRoomRequest->Start();

    mMucConfigRequest = new MucConfigRequest(mXmppClient);
    mMucConfigRequest->Start();

    mMucConfigResponse = new MucConfigResponse(mXmppClient);
    mMucConfigResponse->SignalConfigReceived.connect(this, &JingleClient::OnRoomConfigReceived);
    mMucConfigResponse->Start();

    mMucConfigSubmission = new MucConfigSubmission(mXmppClient);
    mMucConfigSubmission->Start();

    mMucRoomUnlocked = new MucRoomUnlocked(mXmppClient);
    mMucRoomUnlocked->SignalRoomUnlocked.connect(this, &JingleClient::OnRoomUnlocked);
    mMucRoomUnlocked->Start();

    mMucGrantMembership = new MucGrantMembership(mXmppClient);
    mMucGrantMembership->Start();

    mMucRevokeMembership = new MucRevokeMembership(mXmppClient);
    mMucRevokeMembership->Start();

    mMucGrantModerator = new MucGrantModerator(mXmppClient);
    mMucGrantModerator->Start();

    mMucRevokeModerator = new MucRevokeModerator(mXmppClient);
    mMucRevokeModerator->Start();

    mMucGrantAdmin = new MucGrantAdmin(mXmppClient);
    mMucGrantAdmin->Start();

    mMucRevokeAdmin = new MucRevokeAdmin(mXmppClient);
    mMucRevokeAdmin->Start();

    mMucKickOccupant = new MucKickOccupant(mXmppClient);
    mMucKickOccupant->Start();

    mChatGroupRoomInviteRecvTask = new ChatGroupRoomInviteRecvTask(mXmppClient);
    mChatGroupRoomInviteRecvTask->SignalInviteReceived.connect(this,
        &JingleClient::OnChatGroupRoomInviteReceived);
    mChatGroupRoomInviteRecvTask->Start();

    mChatGroupRoomInviteSendTask = new ChatGroupRoomInviteSendTask(mXmppClient);
    mChatGroupRoomInviteSendTask->Start();

    mBlockListSend = new BlockListSend(mXmppClient);
    mBlockListSend->Start();

    mBlockListRequest = new BlockListRequest(mXmppClient);
    mBlockListRequest->Start();

    mBlockListRecv = new BlockListRecv(mXmppClient);
    mBlockListRecv->SignalBlockListReceived.connect(this, &JingleClient::OnBlockListReceived);
    mBlockListRecv->Start();

    mBlockListPush = new BlockListPush(mXmppClient);
    mBlockListPush->SignalBlockListPushReceived.connect(this, &JingleClient::OnBlockListPushReceived);
    mBlockListPush->Start();

    mPingTask = new buzz::PingTask(mXmppClient, talk_base::Thread::Current(), 30000U, 30000U);
    mPingTask->Start();

    buzz::ClientInfo clientInfo = 
    {
        "client",
        EBISU_PRODUCT_NAME_STR " " EBISU_PRODUCT_VERSION_P_DELIMITED
        "pc"
    };

    mCapabilitiesReceiveTask = new buzz::CapabilitiesReceiveTask(mXmppClient, clientInfo, mDiscoInfo.supportedFeatureUrisForJingle());
    mCapabilitiesReceiveTask->Start();

    mCarbonsRequest = new CarbonsRequest(mXmppClient);
    mCarbonsRequest->Start();

    mCarbonsResponse = new CarbonsResponse(mXmppClient);
    mCarbonsResponse->SignalCarbonsResponded.connect(this, &JingleClient::OnCarbonsResponded);
    mCarbonsResponse->Start();

    mChatReceiveTask = new ChatReceiveTask(mXmppClient);
    mChatReceiveTask->SignalMessageReceived.connect(this, &JingleClient::OnMessageReceived);
    mChatReceiveTask->SignalChatStateReceived.connect(this, &JingleClient::OnChatStateReceived);
    mChatReceiveTask->Start();

    mChatSendTask = new ChatSendTask(mXmppClient);
    mChatSendTask->Start();

    mAcceptFriendRequestMessageHandler = new AcceptFriendRequestMessageHandler();
    mCancelFriendRequestMessageHandler = new CancelFriendRequestMessageHandler();
    mRejectFriendRequestMessageHandler = new RejectFriendRequestMessageHandler();
    mSendFriendRequestMessageHandler = new SendFriendRequestMessageHandler ();

    mRemoveFriendMessageHandler = new RemoveFriendMessageHandler();
    mRequestRosterMessageHandler = new RequestRosterMessageHandler();
    mSetPresenceMessageHandler = new SetPresenceMessageHandler ();
    mSetDirectedPresenceMessageHandler.reset(new SetDirectedPresenceMessageHandler());

    mXmppRosterModule = buzz::XmppRosterModule::Create();

    mRosterHandler = new RosterHandler(this);
    mXmppRosterModule->set_roster_handler(mRosterHandler);

    mXmppRosterModule->RegisterEngine(mXmppClient->engine());
    mXmppRosterModule->RequestRosterUpdate(Origin::Services::OriginConfigService::instance()->miscConfig().enable2kFriends);

    mChatroomHandler = new ChatroomHandler(this, mMucInstantRoomRequest, mMucConfigRequest);
}

void JingleClient::OnMucInviteReceived(const buzz::Jid &inviter, const buzz::Jid &room, const QString& reason)
{
    if (!room.IsValid() || room.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle MUC invite received invalid";
        return;
    }

    mEvents->RoomInvitationReceived(inviter, room, reason);

    ORIGIN_LOG_EVENT << "Jingle received a MUC invite";
}

void JingleClient::OnMucDeclineReceived(const buzz::Jid& decliner, const buzz::Jid& room, const QString& reason)
{
    if (!room.IsValid() || room.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle MUC decline received invalid";
        return;
    }

    mEvents->RoomDeclineReceived(decliner, room, reason);

    ORIGIN_LOG_EVENT << "Jingle received a MUC decline";
}

void JingleClient::OnMessageReceived(const JingleMessage &msg)
{
    mEvents->MessageReceived(msg);
}

void JingleClient::OnChatStateReceived(const JingleMessage &msg)
{
    mEvents->ChatStateReceived(msg);
}

void JingleClient::SendChatMessage(const JingleMessage& message)
{
    if (mChatSendTask && mXmppThread)
    {
        mChatSendTask->Send(message, *mXmppThread);
    }    
}

struct FriendRequestData : public talk_base::MessageData
{
    FriendRequestData(const buzz::Jid& jid_, buzz::XmppRosterModule* xmppRosterModule_)
    : jid (jid_)
    , xmppRosterModule (xmppRosterModule_)
    {
    }
    
    const buzz::Jid jid;
    buzz::XmppRosterModule *xmppRosterModule;
};

void JingleClient::SendFriendRequest(const buzz::Jid& jid)
{
    if (!jid.IsValid() || jid.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle send friend request invalid";
        return;
    }

    if (!mXmppRosterModule)
        return;

    if (mSendFriendRequestMessageHandler)
    {
        mXmppThread->Post(mSendFriendRequestMessageHandler, Origin::Chat::MSG_SENDFRIENDREQUEST, new FriendRequestData(jid, mXmppRosterModule));
    }
}

void SendFriendRequestMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_SENDFRIENDREQUEST)
    {
        const FriendRequestData& data = *dynamic_cast<const FriendRequestData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->RequestSubscription (data.jid);
            ORIGIN_LOG_EVENT << "Jingle sending a friend request";
        }
    }
    delete msg->pdata;
    msg->pdata = NULL;
}

void JingleClient::AcceptFriendRequest(const buzz::Jid& jid)
{
    if (!jid.IsValid() || jid.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle accept friend request invalid";
        return;
    }

    if (!mXmppRosterModule)
        return;

    if (mAcceptFriendRequestMessageHandler)
    {
        mXmppThread->Post(mAcceptFriendRequestMessageHandler, Origin::Chat::MSG_ACCEPTFRIENDREQUEST, new FriendRequestData(jid, mXmppRosterModule));
    }
}

void AcceptFriendRequestMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_ACCEPTFRIENDREQUEST)
    {
        const FriendRequestData& data = *dynamic_cast<const FriendRequestData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->ApproveSubscriber(data.jid);
            ORIGIN_LOG_EVENT << "Jingle accepting a friend request";
        }
    } 

    delete msg->pdata;
    msg->pdata = NULL;
}

void JingleClient::RejectFriendRequest(const buzz::Jid& jid)
{
    if (!jid.IsValid() || jid.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle reject friend request invalid";
        return;
    }

    if (!mXmppRosterModule)
        return;

    if (mRejectFriendRequestMessageHandler)
    {
        mXmppThread->Post(mRejectFriendRequestMessageHandler, Origin::Chat::MSG_REJECTFRIENDREQUEST, new FriendRequestData(jid, mXmppRosterModule));
    }
}

void RejectFriendRequestMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_REJECTFRIENDREQUEST)
    {
        const FriendRequestData& data = *dynamic_cast<const FriendRequestData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->CancelSubscriber(data.jid);
            ORIGIN_LOG_EVENT << "Jingle rejecting a friend request";
        }
    }

    delete msg->pdata;
    msg->pdata = NULL;
}

void JingleClient::CancelFriendRequest(const buzz::Jid& jid)
{
    if (!jid.IsValid() || jid.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle cancel friend request invalid";
        return;
    }

    if (!mXmppRosterModule)
        return;

    if (mCancelFriendRequestMessageHandler)
    {
        mXmppThread->Post(mCancelFriendRequestMessageHandler, Origin::Chat::MSG_CANCELFRIENDREQUEST, new FriendRequestData(jid, mXmppRosterModule));
    }
}

void CancelFriendRequestMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_CANCELFRIENDREQUEST)
    {
        const FriendRequestData& data = *dynamic_cast<const FriendRequestData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->CancelSubscription(data.jid);
            ORIGIN_LOG_EVENT << "Jingle canceling a friend request";
        }
    }

    delete msg->pdata;
    msg->pdata = NULL;
}



void JingleClient::RemoveFriend(const buzz::Jid& jid)
{
    if (!jid.IsValid() || jid.node() == "")
    {
        ORIGIN_LOG_ERROR << "Jingle remove friend invalid";
        return;
    }

    if (!mXmppRosterModule)
        return;

    if (mRemoveFriendMessageHandler)
    {
        mXmppThread->Post(mRemoveFriendMessageHandler, Origin::Chat::MSG_REMOVEFRIEND, new FriendRequestData(jid, mXmppRosterModule));
    }
}

void RemoveFriendMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_REMOVEFRIEND)
    {
        const FriendRequestData& data = *dynamic_cast<const FriendRequestData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->RequestRosterRemove(data.jid);
            ORIGIN_LOG_EVENT << "Jingle removing a friend";
        }    
    }

    delete msg->pdata;
    msg->pdata = NULL;

}

struct RequestRosterData : public talk_base::MessageData
{
    RequestRosterData(buzz::XmppRosterModule* xmppRosterModule_)
    : xmppRosterModule (xmppRosterModule_)
    {
    }
    
    buzz::XmppRosterModule *xmppRosterModule;
};

void JingleClient::RequestRoster()
{
    if (!mXmppRosterModule)
        return;

    if (mRequestRosterMessageHandler)
    {
        mXmppThread->Post(mRequestRosterMessageHandler, Origin::Chat::MSG_REQUESTROSTER, new RequestRosterData(mXmppRosterModule));
    }
}

void RequestRosterMessageHandler::OnMessage (talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_REQUESTROSTER)
    {
        const RequestRosterData& data = *dynamic_cast<const RequestRosterData*>(msg->pdata);
        if (data.xmppRosterModule)
        {
            data.xmppRosterModule->RequestRosterUpdate(Origin::Services::OriginConfigService::instance()->miscConfig().enable2kFriends);
            ORIGIN_LOG_EVENT << "Jingle requesting roster";
        }
    }

    delete msg->pdata;
    msg->pdata = NULL;
}

void JingleClient::SetBlockList(QList<buzz::Jid>* blockList)
{
    if (mBlockListSend && mXmppThread)
    {
        mBlockListSend->Send(*blockList, *mXmppThread);
    }

    ORIGIN_LOG_EVENT << "Jingle setting block list";
}

void JingleClient::RequestBlockList()
{
    if (mBlockListRequest && mXmppThread)
    {
        mBlockListRequest->Send(*mXmppThread);
    }

    ORIGIN_LOG_EVENT << "Jingle requesting block list";
}

void JingleClient::OnBlockListReceived(const QList<buzz::Jid>& list)
{
    mEvents->BlockListUpdate(list);

    ORIGIN_LOG_EVENT << "Jingle received block list";
}

void JingleClient::OnBlockListPushReceived()
{
    // Our block list was updated, request a new one so we can update the client
    if (mBlockListRequest && mXmppThread)
    {
        mBlockListRequest->Send(*mXmppThread);
    }    
}

void JingleClient::OnPrivacyReceived()
{
    mEvents->PrivacyReceived();
}

void JingleClient::OnChatGroupRoomInviteReceived(const buzz::Jid& fromJid, const QString& groupId, const QString& channelId)
{
    mEvents->ChatGroupRoomInviteReceived(fromJid, groupId, channelId);

    ORIGIN_LOG_EVENT << "Jingle received group room invite";
}

void JingleClient::RemoveMucRoomInfo(const buzz::Jid& jid)
{
    MucRoomInformation info = mMucRooms.take(jid);
    buzz::XmppChatroomModule* xmppChatRoomModule = info.module;

    if (xmppChatRoomModule)
    {
        delete xmppChatRoomModule;
    }
}

void JingleClient::SetActivePrivacyList(const QString& list)
{
    if (mSetPrivacyList && mXmppThread)
    {
        mSetPrivacyList->Send(list, *mXmppThread);
    }
}

struct SetPresenceData : public talk_base::MessageData
{
    SetPresenceData(
        const Origin::Chat::DiscoverableInformation& discoInfo_,
        const buzz::XmppPresenceShow presence_,
        const buzz::XmppPresenceAvailable availability_,
        const QString* status_,
        const QString* title_, 
        const QString* offerId_,
        const QString* multiplayerId_,
        const QString* gamePresence_,
        bool joinable_, 
        bool joinable_invite_only_, 
        const QString* twitchPresence_,
        buzz::XmppRosterModule* xmppRosterModule_)
                               
    : discoInfo(discoInfo_)
    , presence(presence_)
    , availability (availability_)
    , status (status_ ? *status_ : QString())
    , title (title_ ? *title_ : QString())
    , offerId (offerId_ ? *offerId_ : QString())
    , multiplayerId (multiplayerId_ ? *multiplayerId_ : QString())
    , gamePresence (gamePresence_ ? *gamePresence_ : QString())
    , joinable (joinable_)
    , joinable_invite_only (joinable_invite_only_)
    , twitchPresence (twitchPresence_ ? *twitchPresence_ : QString())
    , xmppRosterModule (xmppRosterModule_)
    {
    }
    
    const Origin::Chat::DiscoverableInformation discoInfo;
    const buzz::XmppPresenceShow presence;
    const buzz::XmppPresenceAvailable availability;
    const QString status;
    const QString title;
    const QString offerId;
    const QString multiplayerId;
    const QString gamePresence;
    bool  joinable;
    bool  joinable_invite_only;
    const QString twitchPresence;
    buzz::XmppRosterModule *xmppRosterModule;
};

void JingleClient::EnableCarbons()
{
    if (mCarbonsRequest && mXmppThread)
    {
        mCarbonsRequest->Send(true, *mXmppThread);
    }    
}

void JingleClient::OnCarbonsResponded(bool enable)
{
    ORIGIN_LOG_EVENT << "Carbons " << (enable ? "enabled" : "disabled");
}

QString JingleClient::StripString(const QString* in)
{
    QString out = "";
    if (in)
    {
        out.append(in);
        // Need to remove these characters to prevent users from causing issues with social    
        out.remove(QRegExp("[\\x0000-\\x0008\\x000B\\x000C\\x000E-\\x001F]"));
        out.remove(QRegExp("<[^>]*>"));
    }
    return out;
}

bool JingleClient::SetPresence(
    const buzz::XmppPresenceShow presence,
    const buzz::XmppPresenceAvailable availability,
    const QString* status /* = NULL */,
    const QString* title /* = NULL */, 
    const QString* offerId /* = NULL */,
    const QString* multiplayerId /* = NULL */,
    const QString* gamePresence /* = NULL */,
    bool joinable /* = false */, 
    bool joinable_invite_only /* = false */, 
    const QString* twitchPresence /* = NULL */)
{
    if (!mXmppRosterModule)
    {
        ORIGIN_LOG_ERROR << "Jingle MUC trying to set presence with no roster";
        return false;
    }

    if (!mSetPresenceMessageHandler)
        return false;

    // This is a double check to try and "future proof" the code
    // If any of these character make it into the presence the user and all friends 
    // will be disconnected from Social when the presence is changed.
    // https://developer.origin.com/support/browse/EBIBUGS-28447
    QString strippedTitle = StripString(title);
    QString strippedStatus = StripString(status);
    
    mXmppThread->Post(
        mSetPresenceMessageHandler,
        Origin::Chat::MSG_SETPRESENCE,
        new SetPresenceData(
            mDiscoInfo,
            presence,
            availability,
            &strippedStatus,
            &strippedTitle,
            offerId, 
            multiplayerId,
            gamePresence,
            joinable,
            joinable_invite_only,
            twitchPresence,
            mXmppRosterModule
        )
    );

    //assume success
    return true;
}

void SetPresenceMessageHandler::OnMessage(talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_SETPRESENCE)
    {
        const SetPresenceData& data = *dynamic_cast<const SetPresenceData*>(msg->pdata);

        if (data.xmppRosterModule)
        {
            // Grab our outgoing presence and modify it
            buzz::XmppPresence* outgoingPresence = data.xmppRosterModule->outgoing_presence();

            outgoingPresence->set_presence_show(data.presence);
            outgoingPresence->set_status(data.status.toStdString());
            outgoingPresence->set_available(data.availability);

            // XEP-0108 extension
            if (!data.title.isEmpty() && !data.offerId.isEmpty())
            {
                QString body = QString("%1;%2;%3;%4;%5;%6").arg(data.title).arg(data.offerId).arg((data.joinable) ? "JOINABLE" : (data.joinable_invite_only ? "JOINABLE_INVITE_ONLY" : "INGAME")).arg(data.twitchPresence).arg(data.gamePresence).arg(data.multiplayerId);
                buzz::XmppActivity activity("relaxing", "gaming", body.toStdString());

                outgoingPresence->set_activity(activity);

                ORIGIN_LOG_EVENT << "Jingle setting activity to " << body;
            }
            else
            {
                outgoingPresence->set_activity(buzz::XmppActivity());
            }
    
            // XEP-0115 extension - entity capabilities
            QByteArray hash(data.discoInfo.sha1VerificationHash());
            outgoingPresence->set_caps("sha-1", "http://www.origin.com/origin", hash.data());
    
            data.xmppRosterModule->BroadcastPresence();
        }
    }

    delete msg->pdata;
    msg->pdata = NULL;
}

struct SetDirectedPresenceData : public talk_base::MessageData
{
    SetDirectedPresenceData(
        const buzz::Jid& to_,
        const std::string& status_, 
        const std::string& category_, 
        const std::string& instance_,
        const std::string& body_,
        const QByteArray& xep0115Hash_,
        buzz::XmppRosterModule& xmppRosterModule_)

        : to(to_)
        , status(status_)
        , category(category_)
        , instance(instance_)
        , body (body_)
        , xep0115Hash(xep0115Hash_)
        , xmppRosterModule(xmppRosterModule_)
    {
    }

    const buzz::Jid to;
    const std::string status;
    const std::string category;
    const std::string instance;
    const std::string body;
    const QByteArray xep0115Hash;
    buzz::XmppRosterModule& xmppRosterModule;
};


bool JingleClient::SendDirectedPresence(
    const Origin::Chat::JingleExtendedPresence presence,
    const QString& status,
    const buzz::Jid& to,
    const QString& body)
{
    if (!mXmppRosterModule)
    {
        ORIGIN_LOG_ERROR << "Jingle: trying to set directed presence with no roster";
        return false;
    }

    if (!mSetDirectedPresenceMessageHandler)
        return false;

    std::string category;
    std::string instance;

    switch(presence)
    {
    case Origin::Chat::JINGLE_PRESENCE_VOIP_CALLING:
        category = Origin::Chat::DP_CALLING;
        instance = Origin::Chat::DP_VOICECHAT;
        break;
    case Origin::Chat::JINGLE_PRESENCE_VOIP_HANGINGUP:
        category = Origin::Chat::DP_HANGINGUP;
        instance = Origin::Chat::DP_VOICECHAT;
        break;
    case Origin::Chat::JINGLE_PRESENCE_VOIP_LEAVE:
        category = Origin::Chat::DP_LEAVE;
        instance = Origin::Chat::DP_VOICECHAT;
        break;
    case Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN:
        category = Origin::Chat::DP_JOIN;
        instance = Origin::Chat::DP_VOICECHAT;
        break;
    default:
        ORIGIN_LOG_ERROR << "Unexpected directed presence: " << presence;
        return false;
    }

    ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "SendDirectedPresence: " << category << ", " << instance << ", " << status;

    mXmppThread->Post(
        mSetDirectedPresenceMessageHandler.data(),
        Origin::Chat::MSG_SETDIRECTEDPRESENCE,
        new SetDirectedPresenceData(
            to,
            status.toStdString(),
            category,
            instance,
            body.toStdString(),
            mDiscoInfo.sha1VerificationHash(),
            *mXmppRosterModule
        )   
    );

    //assume success
    return true;
}

void SetDirectedPresenceMessageHandler::OnMessage(talk_base::Message* msg)
{
    if (!msg || !msg->pdata)
        return;

    if (msg->message_id != Origin::Chat::MSG_SETDIRECTEDPRESENCE)
        return;

    const SetDirectedPresenceData* data = dynamic_cast<const SetDirectedPresenceData*>(msg->pdata);
    if (!data)
        return;

    buzz::XmppPresence* directedPresence = buzz::XmppPresence::Create();
    if (!directedPresence)
        return;

    if (data->status.size() > 0)
        directedPresence->set_status(data->status);
    directedPresence->set_activity(buzz::XmppActivity(data->category, data->instance, data->body));
    directedPresence->set_caps("sha-1", "http://www.origin.com/origin", data->xep0115Hash.data());

    data->xmppRosterModule.SendDirectedPresence(directedPresence, data->to);

    delete msg->pdata;
    msg->pdata = NULL;
}

void JingleClient::MUCInvite(const buzz::Jid& room, const buzz::Jid& to, const QString& reason, const QString& threadId)
{
    if (!room.IsValid())
    {
        ORIGIN_LOG_ERROR << "Jingle MUC invite invalid";
        return;
    }

    if (mMucInviteSendTask && mXmppThread)
    {
        mMucInviteSendTask->Send(to, room, reason, threadId, *mXmppThread);
    }    

    ORIGIN_LOG_EVENT << "Jingle inviting someone to a MUC room";
}

void JingleClient::MUCDecline(const buzz::Jid& room, const buzz::Jid& to, const QString& reason)
{
    if (!room.IsValid())
    {
        ORIGIN_LOG_ERROR << "Jingle MUC decline invalid";
        return;
    }

    if (mMucDeclineSendTask && mXmppThread)
    {
        mMucDeclineSendTask->Send(to, room, reason, *mXmppThread);
    }
    
    ORIGIN_LOG_EVENT << "Jingle declining a MUC invite";
}

void JingleClient::OnRoomConfigReceived(QMap<QString, QString> config, const buzz::Jid& room)
{
    QMapIterator<QString, QString> iter(config);
    while (iter.hasNext())
    {
        iter.next();
        if (iter.key().compare("muc#roomconfig_roomname") == 0)
        {
            config["muc#roomconfig_roomname"] = mMucRooms[room].roomName;
        }
        if (iter.key().compare("muc#roomconfig_passwordprotectedroom") == 0)
        {
            config["muc#roomconfig_passwordprotectedroom"] = mMucRooms[room].roomPassword.isEmpty() ? "0" : "1";
        }
        if (iter.key().compare("muc#roomconfig_roomsecret") == 0)
        {
            config["muc#roomconfig_roomsecret"] = mMucRooms[room].roomPassword;        
        }
    }

    if (mMucConfigSubmission && mXmppThread)
    {
        mMucConfigSubmission->Send(room, config, *mXmppThread);
    }    
}

void JingleClient::OnRoomUnlocked(const buzz::Jid& room)
{
    // If we're unlocking a room then we would have just created it, safe to enter as a moderator.
    MucRoomInformation info = mMucRooms[room];
    buzz::XmppChatroomModule* xmppChatRoomModule = info.module;

    if (!xmppChatRoomModule)
    {
        return;
    }

    mEvents->RoomEntered(*xmppChatRoomModule, "moderator");

    ORIGIN_LOG_EVENT << "Jingle room unlocked";
}

void JingleClient::MUCEnterRoom(const QString& nickname, const buzz::Jid& jid, const QString& password, const QString& roomName)
{
    if (!jid.IsValid())
    {
        return;
    }

    buzz::XmppChatroomModule* xmppChatRoomModule = buzz::XmppChatroomModule::Create();
    xmppChatRoomModule->set_chatroom_handler(mChatroomHandler);
    xmppChatRoomModule->RegisterEngine(mXmppClient->engine());

    xmppChatRoomModule->set_nickname(nickname.toStdString());
    xmppChatRoomModule->set_chatroom_jid(jid);
    xmppChatRoomModule->setInstantRoom(false);
    xmppChatRoomModule->RequestEnterChatroom(password.toStdString(), "", "");

    MucRoomInformation info;
    info.module = xmppChatRoomModule;
    info.roomName = roomName;
    info.roomPassword = password;
    mMucRooms[jid] = info;

    ORIGIN_LOG_EVENT << "Jingle entering a MUC room";
}

void JingleClient::MUCDestroyRoom(const QString& nickname, const QString& nucleusId, const buzz::Jid& jid, const QString& password, const QString& roomName)
{
    if (!jid.IsValid())
    {
        return;
    }
    
    MucRoomInformation info = mMucRooms.take(jid);
    buzz::XmppChatroomModule* xmppChatRoomModule = info.module;

    if (xmppChatRoomModule)
    {
        xmppChatRoomModule->setDestroyRoom(true); // lets ChatroomHandler::ChatroomEnteredStatus() know that we are entering the room to destroy it
        xmppChatRoomModule->RequestDestroyChatroom(nucleusId.toStdString());

        // Do not delete 'xmppChatRoomModule' here
        // since we need it in order to receive a room destruction confirmation from the server
    }
    else
    {
        // We're not in the room. Need to create a module object
        buzz::XmppChatroomModule* xmppChatRoomModule = buzz::XmppChatroomModule::Create();
        xmppChatRoomModule->set_chatroom_handler(mChatroomHandler);
        xmppChatRoomModule->RegisterEngine(mXmppClient->engine());

        // Enter room first
        xmppChatRoomModule->set_nickname(nickname.toStdString());
        xmppChatRoomModule->set_chatroom_jid(jid);
        xmppChatRoomModule->setInstantRoom(false);
        xmppChatRoomModule->setDestroyRoom(true); // lets ChatroomHandler::ChatroomEnteredStatus() know that we are entering the room to destroy it
        xmppChatRoomModule->setDestroyReason(nucleusId.toStdString());
        xmppChatRoomModule->RequestEnterChatroom(password.toStdString(), "", "");

        MucRoomInformation info;
        info.module = xmppChatRoomModule;
        info.roomName = roomName;
        info.roomPassword = password;
        mMucRooms[jid] = info;

        ORIGIN_LOG_EVENT << "Jingle entering a MUC room in order to destroy it";
    }

    ORIGIN_LOG_EVENT << "Jingle destroying a MUC room";
}

void JingleClient::MUCLeaveRoom(const buzz::Jid& jid)
{
    if (!jid.IsValid())
    {
        return;
    }

    MucRoomInformationMapIt it = mMucRooms.find(jid);
    if (it != mMucRooms.end() )
    {
        MucRoomInformation info = mMucRooms.take(jid);
        buzz::XmppChatroomModule* xmppChatRoomModule = info.module;

        if (xmppChatRoomModule)
        {
            xmppChatRoomModule->RequestExitChatroom();

            delete xmppChatRoomModule;
            xmppChatRoomModule = NULL;
        }

        ORIGIN_LOG_EVENT << "Jingle leaving a MUC room";
    }
    else // this muc room may have been previously destroyed
    {
        ORIGIN_LOG_WARNING << "Jingle could not find MUC room [jid = " << jid.Str() << "]";
    }
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCGrantModerator(const buzz::Jid& room, const buzz::Jid& user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucGrantModerator && mXmppThread)
    {
        mMucGrantModerator->Send(room, user, *mXmppThread);
    }    
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCRevokeModerator(const buzz::Jid& room, const buzz::Jid& user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucRevokeModerator && mXmppThread)
    {
        mMucRevokeModerator->Send(room, user, *mXmppThread);
    }    
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCGrantAdmin(const buzz::Jid& room, const buzz::Jid& user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucGrantAdmin && mXmppThread)
    {
        mMucGrantAdmin->Send(room, user, *mXmppThread);
    }    
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCRevokeAdmin(const buzz::Jid& room, const buzz::Jid& user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucRevokeAdmin && mXmppThread)
    {
        mMucRevokeAdmin->Send(room, user, *mXmppThread);
    }    
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCGrantMembership(const buzz::Jid &room, const buzz::Jid &user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucGrantMembership && mXmppThread)
    {
        mMucGrantMembership->Send(room, user, *mXmppThread);
    }    
}

// This is not used yet. Function will most likely change, also need to add logging
void JingleClient::MUCRevokeMembership(const buzz::Jid &room, const buzz::Jid &user)
{
    if (!room.IsValid() || !user.IsValid())
    {
        return;
    }

    if (mMucRevokeMembership && mXmppThread)
    {
        mMucRevokeMembership->Send(room, user, *mXmppThread);
    }    
}

void JingleClient::MUCKickOccupant(const buzz::Jid& room, const QString& userNickname, const QString& by)
{
    if (!room.IsValid())
    {
        return;
    }

    if (mMucKickOccupant && mXmppThread)
    {
        mMucKickOccupant->Send(room, userNickname, by, *mXmppThread);
    }
}

void JingleClient::ChatGroupRoomInvite(const buzz::Jid& toJid, const QString& groupId, const QString& channelId)
{
    qDebug() << "################## ChatGroupRoomInvite() !!!!!!!!!!!!!!\n";
    if (mChatGroupRoomInviteSendTask && mXmppThread)
    {
        mChatGroupRoomInviteSendTask->Send(toJid, groupId, channelId, *mXmppThread);
    }
}

void RosterHandler::SubscriptionRequest(buzz::XmppRosterModule* roster,
                                        const buzz::Jid& requesting_jid,
                                        buzz::XmppSubscriptionRequestType type,
                                        const buzz::XmlElement* raw_xml)
{
    if (type == buzz::XMPP_REQUEST_SUBSCRIBE)
    {
        mClient->Events()->AddRequest(requesting_jid);

        ORIGIN_LOG_EVENT << "Jingle received friend request";
    }    
}

// A list of possible codes we can get back from the friends service is here
// https://developer.origin.com/documentation/display/social/Friends+Error+Codes
// Right now only handling 22001 since that's all we expect to be getting
void RosterHandler::SubscriptionError(buzz::XmppRosterModule* roster,
                                      const buzz::Jid& from,
                                      const buzz::XmlElement* raw_xml)
{
    const buzz::XmlElement* error = raw_xml->FirstNamed(buzz::QN_ERROR);
    if (error != NULL)
    {
        const buzz::XmlElement* text = error->FirstNamed(buzz::QN_STANZA_TEXT);
        if (text != NULL)
        {
            std::string body(text->BodyText());
            if (body.find("22001") != std::string::npos)
            {
                mClient->Events()->ContactLimitReached(from);

                ORIGIN_LOG_EVENT << "Jingle friend limit reached";
            }
        }
    }
}

void RosterHandler::RosterError(buzz::XmppRosterModule* roster,
                                const buzz::XmlElement* raw_xml)
{
    ORIGIN_LOG_EVENT << "Jingle roster error";
}

void RosterHandler::IncomingPresenceChanged(buzz::XmppRosterModule* roster,
                                            const buzz::XmppPresence* presence)
{
    mClient->Events()->PresenceUpdate(presence);

    ORIGIN_LOG_EVENT << "Jingle contact presence changed";}

void RosterHandler::CapabilitiesChanged(
    buzz::XmppRosterModule* roster,
    const buzz::Jid from,
    const buzz::Capabilities caps)
{
    mClient->Events()->CapabilitiesChanged(from, caps);
}

void RosterHandler::ContactChanged(buzz::XmppRosterModule* roster,
                                   const buzz::XmppRosterContact* old_contact,
                                   size_t index)
{
    const buzz::XmppRosterContact* new_contact = roster->GetRosterContact(index);
    if(old_contact && new_contact)
    {
        //contact changed is called regardless of whether the contact actually changed, so we need to check ourselves
        //that something actually changed.
        if(old_contact->subscription_state() != new_contact->subscription_state())
        {
            mClient->Events()->RosterUpdate(*roster);
            ORIGIN_LOG_EVENT << "Jingle contact changed";
        }
    }
}


void RosterHandler::ContactsAdded(buzz::XmppRosterModule* roster,
                                  size_t index, size_t number)
{
    mClient->Events()->RosterUpdate(*roster);

    ORIGIN_LOG_EVENT << "Jingle contact added to roster";
}

void RosterHandler::ContactRemoved(buzz::XmppRosterModule* roster,
                                   const buzz::XmppRosterContact* removed_contact,
                                   size_t index)
{
    mClient->Events()->RosterUpdate(*roster);

    ORIGIN_LOG_EVENT << "Jingle contact removed from roster";
}

void ChatroomHandler::ChatroomEnteredStatus(buzz::XmppChatroomModule* room,
                                            const buzz::XmppPresence* presence,
                                            buzz::XmppChatroomEnteredStatus status)
{
    if (status == buzz::XMPP_CHATROOM_ENTERED_FAILURE_PASSWORD_REQUIRED ||
        status == buzz::XMPP_CHATROOM_ENTERED_FAILURE_PASSWORD_INCORRECT)
    {
        mClient->Events()->RoomPasswordIncorrect(*room);

        if (room->getDestroyRoom())
        {
            // remove the stale MucRoomInformation for this chatroom 
            mClient->RemoveMucRoomInfo(room->chatroom_jid());
        }
        return;
    }

    if (status == buzz::XMPP_CHATROOM_ENTERED_FAILURE_NOT_A_MEMBER || 
        status == buzz::XMPP_CHATROOM_ENTERED_FAILURE_GROUP_NOT_FOUND ||
        status == buzz::XMPP_CHATROOM_ENTERED_FAILURE_MAX_USERS)
    {
        mClient->Events()->RoomEnterFailed(*room, status);
    }

    if (status != buzz::XMPP_CHATROOM_ENTERED_SUCCESS)
        return;

    if (!room || !presence)
        return;

    // In the middle of destroying a room
    if (room->getDestroyRoom())
    {
        mClient->MUCDestroyRoom("", QString::fromStdString(room->destroyReason()), room->chatroom_jid(), "", "");
        return;
    }

    // Depending on the status codes in the presence, we might need to configure this room since it's
    // just being created
    const buzz::XmlElement* presenceElem = presence->raw_xml();
    const buzz::XmlElement* xstanza = presenceElem->FirstNamed(buzz::QN_MUC_USER_X);
    const buzz::XmlElement* statusElem;

    bool created = false;

    statusElem = xstanza->FirstNamed(buzz::QN_MUC_USER_STATUS);
    // Possible race condition on the GOS side where we might be entering this room too fast after getting kicked
    // If we get no status element in the xml need to tell user the enter room request failed
    if (statusElem == NULL)
    {
        mClient->Events()->RoomEnterFailed(*room, status);
        //TODO: would be nice if we could try to automatically re-enter the room here for the user, at least once.
        return;
    }

    do
    {
        std::string code(statusElem->Attr(buzz::QN_CODE));

        if (code.compare("201") == 0)
        {
            created = true;
            break;
        }

        statusElem = statusElem->NextNamed(buzz::QN_MUC_USER_STATUS);
    }
    while (statusElem != NULL);
        
    if (created)
    {
        if (room->getInstantRoom())
        {
            if (mMucInstantRoomRequest && mClient->XmppThread())
            {
                mMucInstantRoomRequest->Send(room->chatroom_jid(), *mClient->XmppThread());
            }
        }
        else
        {
            if (mMucConfigRequest && mClient->XmppThread())
            {
                mMucConfigRequest->Send(room->chatroom_jid(), *mClient->XmppThread());
            }
        }
        ORIGIN_LOG_EVENT << "Jingle sent room config request for newly created room";
    }
    else
    {
        mClient->Events()->RoomEntered(*room, QString::fromUtf8(presence->role().c_str()));
        ORIGIN_LOG_EVENT << "Jingle entering an already existing room";
    }
}

void ChatroomHandler::ChatroomExitedStatus(buzz::XmppChatroomModule* room,
                                           buzz::XmppChatroomExitedStatus status)
{
    if (status == buzz::XMPP_CHATROOM_EXITED_UNSPECIFIED)
    {
        mClient->Events()->RoomDestroyed(*room);
        ORIGIN_LOG_EVENT << "Jingle destroyed room";
    }
    else if(status == buzz::XMPP_CHATROOM_EXITED_KICKED)
    {
        mClient->Events()->RoomKicked(*room);
        ORIGIN_LOG_EVENT << "Jingle kicked from room";
    }

    // remove the stale MucRoomInformation for this chatroom 
    mClient->RemoveMucRoomInfo(room->chatroom_jid());
}

void ChatroomHandler::MemberEntered(buzz::XmppChatroomModule* room,
                                    const buzz::XmppChatroomMember* member)
{
    mClient->Events()->RoomUserJoined(*member);

    ORIGIN_LOG_EVENT << "Jingle member entered MUC room";
}

void ChatroomHandler::MemberExited(buzz::XmppChatroomModule* room,
                                   const buzz::XmppChatroomMember* member)
{
    mClient->Events()->RoomUserLeft(*member);

    ORIGIN_LOG_EVENT << "Jingle member left MUC room";
}

void ChatroomHandler::MemberChanged(buzz::XmppChatroomModule* room,
                                    const buzz::XmppChatroomMember* member)
{
    mClient->Events()->RoomUserChanged(*member);

    ORIGIN_LOG_EVENT << "Jingle member changed in MUC room";
}

void ChatroomHandler::MessageReceived(buzz::XmppChatroomModule* room,
                                      const buzz::XmlElement& message)
{
    mClient->Events()->RoomMessageReceived(message, room->chatroom_jid(), buzz::Jid());
}
