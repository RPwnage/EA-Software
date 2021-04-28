#ifndef JINGLE_CLIENT_H
#define JINGLE_CLIENT_H

#include <QObject>
#include <QMap>
#include <QString>

#include <map>
#include <string>
#include <vector>

#include "talk/base/scoped_ptr.h"
#include "talk/base/sslidentity.h"
#include "talk/xmpp/presencestatus.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/xmpp/rostermodule.h"
#include "talk/xmpp/chatroommodule.h"

#include "JingleMucTasks.h"
#include "JingleChatGroupTasks.h"
#include "JingleChatTasks.h"
#include "JinglePrivacyTasks.h"
#include "DiscoverableInformation.h"

#include "XMPPImplTypes.h"

namespace buzz
{
    class Muc;
    class IqTask;
    class MucRoomConfigTask;
    class MucRoomLookupTask;
    class PingTask;
    class MucPresenceStatus;
    class XmlElement;
    class XmppThread;
    struct MucRoomInfo;
    class CapabilitiesReceiveTask;
}

class JingleClient;

class JingleEvents
{
public:
    virtual void Connected() = 0;
    virtual void StreamError(const buzz::XmlElement& error) = 0;
    virtual void Close(buzz::XmppEngine::Error error) = 0;

    virtual void MessageReceived(const JingleMessage &msg) = 0;
    virtual void ChatStateReceived(const JingleMessage &msg) = 0;
    virtual void AddRequest(const buzz::Jid& from) = 0;
    virtual void RosterUpdate(buzz::XmppRosterModule& roster) = 0;
    virtual void PresenceUpdate(const buzz::XmppPresence* presence) = 0;
    virtual void PrivacyReceived() = 0;

    /// \brief Event triggered when a user's capabilities change (typically when they're first set)
    virtual void CapabilitiesChanged(
        const buzz::Jid entity,
        const buzz::Capabilities features) = 0;

    virtual void ContactLimitReached(const buzz::Jid& from) = 0;

    virtual void BlockListUpdate(const QList<buzz::Jid>& blockList) = 0;

    // MUC
    virtual void RoomEntered(buzz::XmppChatroomModule& room, const QString& role) = 0;
    virtual void RoomEnterFailed(buzz::XmppChatroomModule& room, const buzz::XmppChatroomEnteredStatus& status) = 0;
    virtual void RoomDestroyed(buzz::XmppChatroomModule& room) = 0;
    virtual void RoomKicked(buzz::XmppChatroomModule& room) = 0;
    virtual void RoomPasswordIncorrect(buzz::XmppChatroomModule& room) = 0;
    virtual void RoomInvitationReceived(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason) = 0;
    virtual void RoomUserJoined(const buzz::XmppChatroomMember& member) = 0;
    virtual void RoomUserLeft(const buzz::XmppChatroomMember& member) = 0;
    virtual void RoomUserChanged(const buzz::XmppChatroomMember& member) = 0;
    virtual void RoomMessageReceived(const buzz::XmlElement&, const buzz::Jid& room, const buzz::Jid& from) = 0;
    virtual void RoomDeclineReceived(const buzz::Jid& decliner, const buzz::Jid& room, const QString& reason) = 0;

    // Group Chat
    virtual void ChatGroupRoomInviteReceived(const buzz::Jid&, const QString&, const QString&) = 0;
};

class RosterHandler : public buzz::XmppRosterHandler
{
public:
    RosterHandler(JingleClient* client)
        : mClient(client)
    {

    }
    virtual ~RosterHandler() {}

    virtual void SubscriptionRequest(buzz::XmppRosterModule* roster,
        const buzz::Jid& requesting_jid,
        buzz::XmppSubscriptionRequestType type,
        const buzz::XmlElement* raw_xml);

    virtual void SubscriptionError(buzz::XmppRosterModule* roster,
        const buzz::Jid& from,
        const buzz::XmlElement* raw_xml);

    virtual void RosterError(buzz::XmppRosterModule* roster,
        const buzz::XmlElement* raw_xml);

    virtual void IncomingPresenceChanged(buzz::XmppRosterModule* roster,
        const buzz::XmppPresence* presence);

    virtual void ContactChanged(buzz::XmppRosterModule* roster,
        const buzz::XmppRosterContact* old_contact,
        size_t index);

    virtual void ContactsAdded(buzz::XmppRosterModule* roster,
        size_t index, size_t number);

    virtual void ContactRemoved(buzz::XmppRosterModule* roster,
        const buzz::XmppRosterContact* removed_contact,
        size_t index);

    virtual void CapabilitiesChanged(
        buzz::XmppRosterModule* roster,
        const buzz::Jid from,
        const buzz::Capabilities caps);

private:

    JingleClient* mClient;
};

class ChatroomHandler : public buzz::XmppChatroomHandler
{
public:
    ChatroomHandler(JingleClient* client, MucInstantRoomRequest* instant, MucConfigRequest* config)
        : mClient(client)
        , mMucInstantRoomRequest(instant)
        , mMucConfigRequest(config)
    {
    }

    virtual ~ChatroomHandler() {}

    virtual void ChatroomEnteredStatus(buzz::XmppChatroomModule* room,
        const buzz::XmppPresence* presence,
        buzz::XmppChatroomEnteredStatus status);

    virtual void ChatroomExitedStatus(buzz::XmppChatroomModule* room,
        buzz::XmppChatroomExitedStatus status);

    virtual void MemberEntered(buzz::XmppChatroomModule* room,
        const buzz::XmppChatroomMember* member);

    virtual void MemberExited(buzz::XmppChatroomModule* room,
        const buzz::XmppChatroomMember* member);

    virtual void MemberChanged(buzz::XmppChatroomModule* room,
        const buzz::XmppChatroomMember* member);

    virtual void MessageReceived(buzz::XmppChatroomModule* room,
        const buzz::XmlElement& message);

private:
    JingleClient* mClient;
    MucInstantRoomRequest* mMucInstantRoomRequest;
    MucConfigRequest* mMucConfigRequest;
};

//forward declare Message handlers -- ones that don't need to be tasks
class AcceptFriendRequestMessageHandler;
class CancelFriendRequestMessageHandler;
class RejectFriendRequestMessageHandler;
class SendFriendRequestMessageHandler;
class RemoveFriendMessageHandler;
class RequestRosterMessageHandler;
class SetPresenceMessageHandler;
class SetDirectedPresenceMessageHandler;

class JingleClient: public sigslot::has_slots<>
{
public:

    JingleClient(
        const Origin::Chat::DiscoverableInformation& discoInfo,
        buzz::XmppThread* xmppThread,
        buzz::XmppClient* xmppClient);
    virtual ~JingleClient();

    JingleEvents* Events()
    {
        return mEvents;
    }
    
    void SetEvents(JingleEvents* events)
    {
        mEvents = events;
    }
    
    buzz::XmppThread* XmppThread() const
    {
        return mXmppThread;
    }

    buzz::XmppClient* GetXmppClient() const
    {
        return mXmppClient;
    }

    void RequestRoster();

    void SetBlockList(QList<buzz::Jid>* blockList);
    void RequestBlockList();

    void SendChatMessage(const JingleMessage& message);

    void SendFriendRequest(const buzz::Jid& jid);
    void AcceptFriendRequest(const buzz::Jid& jid);
    void RejectFriendRequest(const buzz::Jid& jid);
    void CancelFriendRequest(const buzz::Jid& jid);
    void RemoveFriend(const buzz::Jid& jid);

    void SetActivePrivacyList(const QString& list);

    bool SetPresence(
        const buzz::XmppPresenceShow presence,
        const buzz::XmppPresenceAvailable availability,
        const QString* status = NULL,
        const QString* title = NULL,
        const QString* offerId = NULL, 
        const QString* multiplayerId = NULL,
        const QString* gamePresence = NULL,
        bool joinable = false,
        bool joinable_invite_only = false,
        const QString* twitchPresence = NULL);

    bool SendDirectedPresence(
        const Origin::Chat::JingleExtendedPresence presence,
        const QString& status,
        const buzz::Jid& to,
        const QString& body);

    void RemoveMucRoomInfo(const buzz::Jid& room);

    void MUCInvite(const buzz::Jid& room, const buzz::Jid& to, const QString& reason, const QString& threadId);
    void MUCDecline(const buzz::Jid& room, const buzz::Jid& to, const QString& reason);
    void MUCEnterRoom(const QString& nickname, const buzz::Jid& jid, const QString& password, const QString& roomName);
    void MUCDestroyRoom(const QString& nickname, const QString& nucleusId, const buzz::Jid& jid, const QString& password, const QString& roomName);
    void MUCLeaveRoom(const buzz::Jid& jid);
    void MUCGrantModerator(const buzz::Jid& room, const buzz::Jid& user);
    void MUCRevokeModerator(const buzz::Jid& room, const buzz::Jid& user);
    void MUCGrantMembership(const buzz::Jid& room, const buzz::Jid& user);
    void MUCRevokeMembership(const buzz::Jid& room, const buzz::Jid& user);
    void MUCGrantAdmin(const buzz::Jid& room, const buzz::Jid& user);
    void MUCRevokeAdmin(const buzz::Jid& room, const buzz::Jid& user);
    void MUCKickOccupant(const buzz::Jid& room, const QString& userNickname, const QString& by);
    SetPrivacyList* PrivacyList() {return mSetPrivacyList;};
    void ChatGroupRoomInvite(const buzz::Jid& toJid, const QString& groupId, const QString& channelId);
    QString StripString(const QString* in); 

    void EnableCarbons();

private:
    void InitializeJingle();

    void OnStateChange(buzz::XmppEngine::State state);

    // Task responses
    void OnMucInviteReceived(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason);
    void OnMucDeclineReceived(const buzz::Jid& decliner, const buzz::Jid& room, const QString& reason);
    void OnRoomConfigReceived(QMap<QString, QString> config, const buzz::Jid& room);
    void OnRoomUnlocked(const buzz::Jid& room);
    void OnMessageReceived(const JingleMessage &msg);
    void OnChatStateReceived(const JingleMessage &msg);
    void OnBlockListReceived(const QList<buzz::Jid>& list);
    void OnBlockListPushReceived();
    void OnPrivacyReceived();
    void OnChatGroupRoomInviteReceived(const buzz::Jid& fromJid, const QString& groupId, const QString& channelId);
    void OnCarbonsResponded(bool enable);

    buzz::XmppThread* mXmppThread;
    buzz::XmppClient* mXmppClient;
    buzz::XmppRosterModule* mXmppRosterModule;

    RosterHandler* mRosterHandler;
    ChatroomHandler* mChatroomHandler;

    MucInviteRecvTask* mMucInviteRecvTask;
    MucInviteSendTask* mMucInviteSendTask;
    MucDeclineRecvTask* mMucDeclineRecvTask;
    MucDeclineSendTask* mMucDeclineSendTask;

    MucInstantRoomRequest* mMucInstantRoomRequest;
    MucConfigRequest* mMucConfigRequest;
    MucConfigResponse* mMucConfigResponse;
    MucConfigSubmission* mMucConfigSubmission;
    MucRoomUnlocked* mMucRoomUnlocked;

    MucGrantMembership* mMucGrantMembership;
    MucRevokeMembership* mMucRevokeMembership;

    MucGrantModerator* mMucGrantModerator;
    MucRevokeModerator* mMucRevokeModerator;

    MucGrantAdmin* mMucGrantAdmin;
    MucRevokeAdmin* mMucRevokeAdmin;

    MucKickOccupant* mMucKickOccupant;

    ChatGroupRoomInviteRecvTask* mChatGroupRoomInviteRecvTask;
    ChatGroupRoomInviteSendTask* mChatGroupRoomInviteSendTask;

    BlockListSend* mBlockListSend;
    BlockListPush* mBlockListPush;
    BlockListRequest* mBlockListRequest;
    BlockListRecv* mBlockListRecv;

    SetPrivacyList* mSetPrivacyList;

    ChatReceiveTask* mChatReceiveTask;
    ChatSendTask* mChatSendTask;

    CarbonsRequest* mCarbonsRequest;
    CarbonsResponse* mCarbonsResponse;

    buzz::PingTask* mPingTask;

    buzz::CapabilitiesReceiveTask* mCapabilitiesReceiveTask;

    AcceptFriendRequestMessageHandler *mAcceptFriendRequestMessageHandler;
    CancelFriendRequestMessageHandler *mCancelFriendRequestMessageHandler;
    RejectFriendRequestMessageHandler *mRejectFriendRequestMessageHandler;
    SendFriendRequestMessageHandler *mSendFriendRequestMessageHandler;

    RemoveFriendMessageHandler *mRemoveFriendMessageHandler;
    RequestRosterMessageHandler *mRequestRosterMessageHandler;
    SetPresenceMessageHandler* mSetPresenceMessageHandler;
    QScopedPointer<SetDirectedPresenceMessageHandler> mSetDirectedPresenceMessageHandler;

    JingleEvents* mEvents;

    struct MucRoomInformation
    {
        MucRoomInformation()
        {
            module = NULL;
        }
        buzz::XmppChatroomModule* module;
        QString roomName;
        QString roomPassword;
    };

    QMap<buzz::Jid, MucRoomInformation> mMucRooms;

    typedef QMap<buzz::Jid, MucRoomInformation>::iterator MucRoomInformationMapIt;
    Origin::Chat::DiscoverableInformation mDiscoInfo;
};

#endif //JINGLE_CLIENT_H
