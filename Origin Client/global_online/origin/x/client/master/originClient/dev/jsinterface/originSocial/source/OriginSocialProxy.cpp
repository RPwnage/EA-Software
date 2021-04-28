#include "chat/RemoteUser.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "chat/Roster.h"
#include "chat/Presence.h"
#include "chat/BlockList.h"

#include "services/debug/DebugService.h"

#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserAvailabilityController.h"
#include "OriginSocialProxy.h"
#include "ToastHelper.h"
#include "ConversionHelpers.h"

#include "flows/ToInvisibleFlow.h"

#include "ClientFlow.h"
#include "SocialViewController.h"

using namespace Origin::Services;
using namespace Origin::Engine;
using namespace Origin::Chat;

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    
OriginSocialProxy::OriginSocialProxy(QObject *parent) :
    QObject(parent)
   
{
    UserRef user = Engine::LoginController::instance()->currentUser();
    // Get our top level objects
    Engine::Social::SocialController *socialController = user->socialControllerInstance();
    mChatConnection = socialController->chatConnection();
    mCurrentUser = mChatConnection->currentUser();
    mRoster = mCurrentUser->roster();
    mToastHelper = new OriginSocialToastHelper(parent);
    ORIGIN_VERIFY_CONNECT(mChatConnection, SIGNAL(connected()), this, SLOT(onConnected()));
    ORIGIN_VERIFY_CONNECT(mChatConnection, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)), this, SLOT(onDisconnected()));
    ORIGIN_VERIFY_CONNECT(mChatConnection, SIGNAL(mutualContactLimitReached(const buzz::Jid&)), this, SLOT(onMutualContactLimitReached(const buzz::Jid&)));
    ORIGIN_VERIFY_CONNECT(mRoster, SIGNAL(loaded()), this, SLOT(onRosterLoaded()));
    ORIGIN_VERIFY_CONNECT(mChatConnection, SIGNAL(messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)),this, SLOT(onMessageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)));
    ORIGIN_VERIFY_CONNECT(mChatConnection, SIGNAL(chatStateReceived(const Origin::Chat::Message &)), this, SLOT(onChatStateReceived(const Origin::Chat::Message &)));

    Chat::BlockList* blockList = user->socialControllerInstance()->chatConnection()->currentUser()->blockList();
    ORIGIN_VERIFY_CONNECT(blockList, SIGNAL(updated()),
        this, SIGNAL(blockListChanged()));

    ORIGIN_VERIFY_CONNECT(mRoster, SIGNAL(subscriptionRequested(const Origin::Chat::JabberID &)), this, SLOT(onSubscriptionRequested(const Origin::Chat::JabberID &)));
}
    
OriginSocialProxy::~OriginSocialProxy()
{
    ORIGIN_VERIFY_DISCONNECT(mChatConnection, SIGNAL(connected()), this, SIGNAL(onConnected()));
    ORIGIN_VERIFY_DISCONNECT(mChatConnection, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)), this, SLOT(onDisconnected()));
    ORIGIN_VERIFY_DISCONNECT(mRoster, SIGNAL(loaded()), this, SLOT(onRosterLoaded()));
    ORIGIN_VERIFY_DISCONNECT(mChatConnection, SIGNAL(messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)), this, SLOT(onMessageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)));
    ORIGIN_VERIFY_DISCONNECT(mChatConnection, SIGNAL(chatStateReceived(const Origin::Chat::Message &)), this, SLOT(onChatStateReceived(const Origin::Chat::Message &)));
}


QJsonObject OriginSocialProxy::buildReceivedMessage(const Chat::Message &msg, const Chat::JabberID& jid)
{
    QJsonObject message;

    message["jid"] = jid.toJingle().Str().c_str();
    const QString from = msg.from().toJingle().Str().c_str();
    message["from"] = from;
    const QString to = msg.to().asBare().toJingle().Str().c_str();
    message["to"] = to;
    message["type"] = msg.type();
    message["msgBody"] = msg.body();
    QString chatState;
    using namespace Chat;
    ChatState state = msg.state();
    switch (state)
    {
    case ChatStateNone:
        break;
    case ChatStateActive:
        chatState = "ACTIVE";
        break;
    case ChatStateInactive:
        chatState = "INACTIVE";
        break;
    case ChatStateGone:
        chatState = "GONE";
        break;
    case ChatStateComposing:
        chatState = "COMPOSING";
        break;
    case ChatStatePaused:
        chatState = "PAUSED";
        break;
    }
    message["chatState"] = chatState;
    //QJsonObject gameActivityObj;

    //presenceObj ["jid"] = jidStr;

    return message;
}

QJsonObject OriginSocialProxy::createSubscribeJson(const Origin::Chat::JabberID& jid)
{
    QJsonObject presenceObj;

    // Emulate what Strophe does

    const QString jidStr = QString(jid.toJingle().Str().c_str());
    presenceObj["jid"] = jidStr;
    presenceObj["show"] = ConversionHelpers::availabilityToString(Origin::Chat::Presence::Online);
    presenceObj["presenceType"] = "SUBSCRIBE";

    QJsonObject gameActivityObj;
    presenceObj["gameActivity"] = gameActivityObj;

    return presenceObj;
}

void OriginSocialProxy::sendRosterChanged(Chat::RemoteUser *remoteUser, const QString& subState, bool subReqSent)
{
    QJsonObject rosterChangeObj;

    Chat::JabberID jid = remoteUser->jabberId();

    const QString jidStr = QString(jid.toJingle().Str().c_str());
    rosterChangeObj["jid"] = jidStr;
    rosterChangeObj["subState"] = subState;
    rosterChangeObj ["subReqSent"] = subReqSent;

    emit rosterChanged(rosterChangeObj.toVariantMap());
}

void OriginSocialProxy::onSubscriptionRequested(const Origin::Chat::JabberID &jid)
{
    QJsonArray array;
    array.append(createSubscribeJson(jid));
    emit presenceChanged(array.toVariantList());
}

void OriginSocialProxy::onContactAdded(Origin::Chat::RemoteUser *remoteUser)
{
        // roster changed event
    sendRosterChanged(remoteUser, ConversionHelpers::subscriptionStateToString(remoteUser->subscriptionState()), remoteUser->subscriptionState().isPendingContactApproval());
   
    // Connect any changed signals up
    ORIGIN_VERIFY_CONNECT(
        remoteUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
        this, SLOT(onRemoteUserPresenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)));
    // Make this a queued connection so we give the chat window time to be created if needed.
    /*
    ORIGIN_VERIFY_CONNECT_EX(remoteUser, SIGNAL(messageReceived(const Origin::Chat::Message &)),
        this, SLOT(onContactMessageReceived(const Origin::Chat::Message &)), Qt::QueuedConnection);
*/
    ORIGIN_VERIFY_CONNECT(
        remoteUser, SIGNAL(subscriptionStateChanged(Origin::Chat::SubscriptionState, Origin::Chat::SubscriptionState)),
        this, SLOT(onContactChanged()));
}

void OriginSocialProxy::onContactRemoved(Chat::RemoteUser *remoteUser)
{
    // Remove this contact
    sendRosterChanged(remoteUser, "REMOVE", remoteUser->subscriptionState().isPendingContactApproval());

    // Clean up after ourselves
    ORIGIN_VERIFY_DISCONNECT(
        remoteUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
        this, SLOT(onRemoteUserPresenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)));
    
    ORIGIN_VERIFY_DISCONNECT(remoteUser, SIGNAL(messageReceived(const Origin::Chat::Message &)),
        this, SLOT(onContactMessageReceived(const Origin::Chat::Message &)));
        
    ORIGIN_VERIFY_DISCONNECT(
        remoteUser, SIGNAL(subscriptionStateChanged(Origin::Chat::SubscriptionState, Origin::Chat::SubscriptionState)),
        this, SLOT(onContactChanged()));


}

void OriginSocialProxy::onContactChanged()
{
    Chat::RemoteUser *remoteUser = dynamic_cast<Chat::RemoteUser*>(sender());
    ORIGIN_ASSERT(remoteUser);

    // Subscription state has changed
    sendRosterChanged(remoteUser, ConversionHelpers::subscriptionStateToString(remoteUser->subscriptionState()), remoteUser->subscriptionState().isPendingContactApproval());

}
void OriginSocialProxy::onMessageReceived(Origin::Chat::Conversable *conv, const Origin::Chat::Message &msg)
{
    emit messageReceived(buildReceivedMessage(msg, conv->jabberId()).toVariantMap());
}

void OriginSocialProxy::onChatStateReceived(const Origin::Chat::Message &msg)
{
    emit chatStateReceived(buildReceivedMessage(msg, msg.from()).toVariantMap());
}
void OriginSocialProxy::onRosterLoaded()
{
    listenForRemoteUserPresence();
    QJsonObject data;
    data["roster"] = buildFullRoster();
    emit rosterLoaded(data.toVariantMap());
}

void OriginSocialProxy::onRosterChanged(Chat::RemoteUser *remoteUser, const QString& subState)
{
    QJsonObject rosterChangeObj;
    const QString jidStr = QString(remoteUser->jabberId().toJingle().Str().c_str());
    rosterChangeObj["jid"] = jidStr;
    rosterChangeObj["subState"] = subState;

    emit rosterChanged(rosterChangeObj.toVariantMap());
}
void OriginSocialProxy::onDisconnected() 
{
    emit connectionChanged(false);
}

void OriginSocialProxy::onConnected()
{
    emit connectionChanged(true);
}
QJsonObject OriginSocialProxy::buildRemoteUserObject(RemoteUser *remoteUser) 
{
    QJsonObject obj;

    switch (remoteUser->subscriptionState().direction())
    {
    case SubscriptionState::DirectionNone:
        obj["subState"] = "NONE";
        break;
    case SubscriptionState::DirectionTo:
        obj["subState"] = "TO";
        break;
    case SubscriptionState::DirectionFrom:
        obj["subState"] = "FROM";
        break;
    case SubscriptionState::DirectionBoth:
        obj["subState"] = "BOTH";
        break;
    }

    obj["jid"] = QString(remoteUser->jabberId().toEncoded());
    obj["originId"] = !remoteUser->originId().isNull() ? remoteUser->originId() : "";
    obj["subReqSent"] = remoteUser->subscriptionState().isPendingContactApproval();

    return obj;
}

bool OriginSocialProxy::isRosterLoaded()
{
    return mRoster->hasLoaded();
}

bool OriginSocialProxy::isConnectionEstablished()
{
    bool connected = false;
    if (mChatConnection)
    {
        connected = mChatConnection->established();
    }
    return connected;
}


void OriginSocialProxy::approveSubscriptionRequest(QString id)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());
    mRoster->approveSubscriptionRequest(jid);
}

void OriginSocialProxy::denySubscriptionRequest(QString id)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());
    mRoster->denySubscriptionRequest(jid);
}

void OriginSocialProxy::cancelSubscriptionRequest(QString id)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());
    mRoster->cancelSubscriptionRequest(jid);
}

void OriginSocialProxy::subscriptionRequest(const QString &id)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());
    mRoster->requestSubscription(jid);
}

void OriginSocialProxy::removeFriend(QString id)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());
    Chat::RemoteUser* removeUser = mChatConnection->remoteUserForJabberId(jid);

    mRoster->removeContact(removeUser);
}

void OriginSocialProxy::blockUser(const QString &nucleusId)
{
    UserRef user = Engine::LoginController::instance()->currentUser();
    Engine::Social::SocialController *socialController = user->socialControllerInstance();
    Chat::OriginConnection *chatConnection = socialController->chatConnection();

    Chat::JabberID jabberId = chatConnection->nucleusIdToJabberId(nucleusId.toULongLong());
    Chat::RemoteUser* blockUser = chatConnection->remoteUserForJabberId(jabberId);

    if (blockUser) 
    {
        ClientFlow::instance()->socialViewController()->blockUser(blockUser->nucleusId());
    }
}

void OriginSocialProxy::unblockUser(const QString &nucleusId)
{
    UserRef user = Engine::LoginController::instance()->currentUser();
    Engine::Social::SocialController *socialController = user->socialControllerInstance();
    Chat::OriginConnection *chatConnection = socialController->chatConnection();
    Chat::JabberID jabberId = chatConnection->nucleusIdToJabberId(nucleusId.toULongLong());

    socialController->chatConnection()->currentUser()->blockList()->removeJabberID(jabberId.asBare());
}

void OriginSocialProxy::sendMessage(QString id, QString msgBody, QString type)
{
    Origin::Chat::JabberID jid = Chat::JabberID::fromEncoded(id.toUtf8());


    Origin::Chat::Message msg(type, mCurrentUser->jabberId(), jid, Chat::ChatStateActive);
    msg.setBody(msgBody);

    mChatConnection->sendMessage(msg);
}


void OriginSocialProxy::setRequestedVisibility(const QString &str)
{
    Chat::ConnectedUser::Visibility visibility;
    UserRef user = Engine::LoginController::instance()->currentUser();
    if (!user)
    {
        return;
    }
    if (str == "VISIBLE")
    {
        visibility = Chat::ConnectedUser::Visible;
    }
    else if (str == "INVISIBLE")
    {
        visibility = Chat::ConnectedUser::Invisible;
    }
    else
    {
        return;
    }

    ToInvisibleFlow *flow = new ToInvisibleFlow(user->socialControllerInstance());
    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(bool)), flow, SLOT(deleteLater()));

    flow->start(visibility);
}

void OriginSocialProxy::setRequestedAvailability(const QString &str)
{
    UserRef user = Engine::LoginController::instance()->currentUser();
    if (!user)
    {
        return;
    }
    bool parsedOk;

    // Attempt to parse the passed string
    const Chat::Presence::Availability wantedAvailability(ConversionHelpers::stringToAvailability(str, parsedOk));

    if (parsedOk)
    {
        // Do a high-level presence transition
        user->socialControllerInstance()->userAvailability()->transitionTo(wantedAvailability);
    }
}

void OriginSocialProxy::requestPresenceChange(QString presence)
{
    if (presence.compare("invisible") == 0)
        setRequestedVisibility(presence.toUpper());
    else
    {
        setRequestedAvailability(presence.toUpper());
        setRequestedVisibility("VISIBLE");
    }
}


void OriginSocialProxy::setTypingState(QString state, QString remoteUserId)
{
    // get nucleus Id of remote user
    int indexOfAtSymbol = remoteUserId.indexOf("@");
    remoteUserId.truncate(indexOfAtSymbol);
    quint64 remoteUserNucleusId = remoteUserId.toULongLong();

    const Chat::JabberID remoteUserJid(mChatConnection->nucleusIdToJabberId(remoteUserNucleusId));
    Chat::RemoteUser* remoteUser = mChatConnection->remoteUserForJabberId(remoteUserJid);

    if (state == "composing")
    {
        Origin::Chat::Message message("chat", mCurrentUser->jabberId(), remoteUser->jabberId(), Origin::Chat::ChatStateComposing);
        mChatConnection->sendMessage(message);
    }
    else if (state == "paused")
    {
        Origin::Chat::Message message("chat", mCurrentUser->jabberId(), remoteUser->jabberId(), Origin::Chat::ChatStatePaused);
        mChatConnection->sendMessage(message);
    }
    else if (state == "active")
    {
        Origin::Chat::Message message("chat", mCurrentUser->jabberId(), remoteUser->jabberId(), Origin::Chat::ChatStateActive);
        mChatConnection->sendMessage(message);
    }
}

void OriginSocialProxy::onCurrentUserPresenceChanged(const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence)
{
    QJsonArray array;
    array.append(createPresenceJson(mCurrentUser->jabberId(), newPresence));
    emit presenceChanged(array.toVariantList());
}

void OriginSocialProxy::onRemoteUserPresenceChanged(const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence)
{
    Origin::Chat::RemoteUser *contact = dynamic_cast<Origin::Chat::RemoteUser*>(sender());

    if (mToastHelper)
    {
        mToastHelper->updatePresenceToast(contact, newPresence, oldPresence);
    }

    QJsonArray array;
    array.append(createPresenceJson(contact->jabberId(), newPresence));
    emit presenceChanged(array.toVariantList());
}

void OriginSocialProxy::setInitialPresence(QString presence)
{
    setRequestedAvailability("VISIBLE");
    QMetaObject::invokeMethod(mCurrentUser, "applyConnectionState", Qt::QueuedConnection);
}

QVariantList OriginSocialProxy::requestInitialPresenceForUserAndFriends()
{
    QJsonArray array;
    QSet<Chat::RemoteUser*> contacts = mRoster->contacts();

    for (QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
        it != contacts.constEnd();
        it++)
    {
        Origin::Chat::RemoteUser* remoteUser = (*it);
        array.append(QJsonValue(createPresenceJson(remoteUser->jabberId(), remoteUser->presence())));
    }

    // GIVE AN INITIAL PRESENCE FOR THE CURRENT USER AS WELL
    array.append(QJsonValue(createPresenceJson(mCurrentUser->jabberId(), mCurrentUser->presence())));

    return array.toVariantList();
}

QVariantList OriginSocialProxy::roster()
{
    return buildFullRoster().toVariantList();
}

QJsonArray OriginSocialProxy::buildFullRoster()
{
    // Add all existing contacts
    QSet<Chat::RemoteUser*> contacts = mRoster->contacts();


    QJsonArray rosterList;

    for (QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
        it != contacts.constEnd();
        it++)
    {
        rosterList.append(buildRemoteUserObject(*it));
    }
    return rosterList;
}

void OriginSocialProxy::listenForRemoteUserPresence()
{

    // We want presence change events for the current user
    ORIGIN_VERIFY_CONNECT(
        mCurrentUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
        this, SLOT(onCurrentUserPresenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)));

    if (mRoster->hasLoaded())
    {
        QSet<Chat::RemoteUser*> contacts = mRoster->contacts();

        for (QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
            it != contacts.constEnd();
            it++)
        {
            // Connect any changed signals up
            ORIGIN_VERIFY_CONNECT(
                (*it), SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
                this, SLOT(onRemoteUserPresenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)));
            
            ORIGIN_VERIFY_CONNECT_EX((*it), SIGNAL(messageReceived(const Origin::Chat::Message &)),
                this, SLOT(onContactMessageReceived(const Origin::Chat::Message &)), Qt::QueuedConnection);
    
            ORIGIN_VERIFY_CONNECT(
                (*it), SIGNAL(subscriptionStateChanged(Origin::Chat::SubscriptionState, Origin::Chat::SubscriptionState)),
                this, SLOT(onContactChanged()));
           

        }
        ORIGIN_VERIFY_CONNECT(mRoster, SIGNAL(contactAdded(Origin::Chat::RemoteUser*)), this, SLOT(onContactAdded(Origin::Chat::RemoteUser*)));
        ORIGIN_VERIFY_CONNECT(mRoster, SIGNAL(contactRemoved(Origin::Chat::RemoteUser*)), this, SLOT(onContactRemoved(Origin::Chat::RemoteUser*)));

    }
}

// for Group Chat
QJsonObject OriginSocialProxy::createPresenceJson(
    const Origin::Chat::JabberID& user,
    const Origin::Chat::MucRoom * room,
    const Origin::Chat::Presence& presence)
{
    QJsonObject presenceObj;
    QJsonObject gameActivityObj;

    presenceObj["from"] = QString(user.asBare().toJingle().Str().c_str());
    const QString jidStr = QString(room->jabberId().withResource(user.resource()).toJingle().Str().c_str());
    presenceObj["jid"] = jidStr;
    presenceObj["nick"] = user.resource();

    // set up presence
    if (presence.availability() == Origin::Chat::Presence::Online)
    {
        presenceObj["show"] = ConversionHelpers::availabilityToString(presence.availability());
    }
    else if (presence.availability() == Origin::Chat::Presence::Unavailable)
    {
        presenceObj["show"] = ConversionHelpers::availabilityToString(Origin::Chat::Presence::Online);
        presenceObj["presenceType"] = ConversionHelpers::availabilityToString(Origin::Chat::Presence::Unavailable);
    }

    Origin::Chat::OriginGameActivity gameActivity = presence.gameActivity();

    if (!gameActivity.isNull())
    {
        gameActivityObj["title"] = gameActivity.gameTitle();
        gameActivityObj["productId"] = gameActivity.productId();
        gameActivityObj["joinable"] = gameActivity.joinable();
        gameActivityObj["twitchPresence"] = gameActivity.broadcastUrl();
        gameActivityObj["gamePresence"] = gameActivity.gamePresence();
        gameActivityObj["multiplayerId"] = gameActivity.multiplayerId();
    }

    presenceObj["gameActivity"] = gameActivityObj;

    return presenceObj;
}

QJsonObject OriginSocialProxy::createPresenceJson(const Origin::Chat::JabberID& jid, const Origin::Chat::Presence& presence)
{
    QJsonObject presenceObj;
    QJsonObject gameActivityObj;

    const QString jidStr = QString(jid.toJingle().Str().c_str());
    presenceObj["jid"] = jidStr;
    presenceObj["show"] = ConversionHelpers::availabilityToString(presence.availability());

    Origin::Chat::OriginGameActivity gameActivity = presence.gameActivity();

    if (!gameActivity.isNull())
    {
        gameActivityObj["title"] = gameActivity.gameTitle();
        gameActivityObj["productId"] = gameActivity.productId();
        gameActivityObj["joinable"] = gameActivity.joinable();
        gameActivityObj["twitchPresence"] = gameActivity.broadcastUrl();
        gameActivityObj["gamePresence"] = gameActivity.gamePresence();
        gameActivityObj["multiplayerId"] = gameActivity.multiplayerId();
    }

    presenceObj["gameActivity"] = gameActivityObj;

    return presenceObj;
}

void OriginSocialProxy::onEnteredCreatedMucRoom(Origin::Chat::MucRoom *room)
{
    Chat::EnterMucRoomOperation *enterOp = dynamic_cast<Chat::EnterMucRoomOperation*>(sender());
    UserRef user = Engine::LoginController::instance()->currentUser();
    if (enterOp)
    {
        // connect signal for other users joining / leaving the room
        ORIGIN_VERIFY_CONNECT(
            mChatConnection, SIGNAL(userJoined(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)),
            this, SLOT(onUserJoined(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)));
        ORIGIN_VERIFY_CONNECT(
            mChatConnection, SIGNAL(userLeft(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)),
            this, SLOT(onUserLeft(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)));

        // send presence to JS
        Chat::JabberID jabberUser = room->jabberIdForNickname(room->nickname());

        Chat::Presence presence(Chat::Presence::Online);
        QJsonArray array;
        array.append(createPresenceJson(jabberUser, room, presence));
        emit presenceChanged(array.toVariantList());

    }
}

void OriginSocialProxy::onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)
{
    Chat::EnterMucRoomOperation *enterOp = dynamic_cast<Origin::Chat::EnterMucRoomOperation*>(sender());
    if (enterOp)
    {
        Chat::JabberID jid = enterOp->jabberId();
        // clean up the MucRoom
        QHash<QString, Chat::MucRoom *>::iterator it = mActiveMucRooms.find(jid.node());
        if (it != mActiveMucRooms.end())
        {
            delete (*it);
        }
        else
        {
            // could not find the MucRoom to be deleted
            ORIGIN_ASSERT(false);
        }
    }
}

void OriginSocialProxy::onUserJoined(const Origin::Chat::JabberID & user, const Origin::Chat::MucRoom *room)
{
    // send presence to JS
    Chat::Presence presence(Chat::Presence::Online);
    QJsonArray array;
    array.append(createPresenceJson(user, room, presence));
    emit presenceChanged(array.toVariantList());
}

void OriginSocialProxy::onUserLeft(const Origin::Chat::JabberID & user, const Origin::Chat::MucRoom *room)
{
    // send presence to JS
    Chat::Presence presence(Chat::Presence::Unavailable);
    QJsonArray array;
    array.append(createPresenceJson(user, room, presence));
    emit presenceChanged(array.toVariantList());

}

void OriginSocialProxy::joinRoom(QString jid, QString originId)
{
    // get room id (e.g., 'gac_xxxxx')

    QString roomId = jid;
    roomId.truncate(roomId.indexOf("@"));

    // create temporary muc room
    Origin::Chat::JabberID roomJid = Chat::JabberID::fromEncoded(jid.toUtf8());
    Chat::MucRoom *room = new Chat::MucRoom(mChatConnection, roomJid);
    mActiveMucRooms[jid] = room;

    Chat::EnterMucRoomOperation *enterOp = mChatConnection->enterMucRoom(room, roomId, originId);
    ORIGIN_VERIFY_CONNECT(
        enterOp, SIGNAL(succeeded(Origin::Chat::MucRoom *)),
        this, SLOT(onEnteredCreatedMucRoom(Origin::Chat::MucRoom *)));

    // We need to always track these failure so we can properly debug possible issues
    ORIGIN_VERIFY_CONNECT(
        enterOp, SIGNAL(failed(Origin::Chat::ChatroomEnteredStatus)),
        this, SLOT(onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)));
}

void OriginSocialProxy::leaveRoom(QString jid, QString originId)
{
    QString roomId = jid;
    roomId.truncate(roomId.indexOf("@"));
    mChatConnection->leaveMucRoom(roomId, originId);

    // clean up the MucRoom
    QHash<QString, Chat::MucRoom *>::iterator it = mActiveMucRooms.find(jid);
    if (it != mActiveMucRooms.end())
    {
        delete (*it);

        // disconnect signal for other users joining / leaving the room
        ORIGIN_VERIFY_DISCONNECT(
            mChatConnection, SIGNAL(userJoined(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)),
            this, SLOT(onUserJoined(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)));
        ORIGIN_VERIFY_DISCONNECT(
            mChatConnection, SIGNAL(userLeft(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)),
            this, SLOT(onUserLeft(const Origin::Chat::JabberID&, const Origin::Chat::MucRoom *)));
    }
    else
    {
        // could not find the MucRoom to be deletedeatest-gameflow3@ea.com
        ORIGIN_ASSERT(false);
    }
}

void OriginSocialProxy::onContactMessageReceived(const Origin::Chat::Message & message)
{
    Origin::Chat::RemoteUser* contact = dynamic_cast<Origin::Chat::RemoteUser*>(sender());
    if (mToastHelper)
    {
        mToastHelper->updateMessageToast(contact, message);
    }
}

bool OriginSocialProxy::isBlocked(const QString& nucleusId)
{
    UserRef user = Engine::LoginController::instance()->currentUser();
    Engine::Social::SocialController *socialController = user->socialControllerInstance();
    Chat::OriginConnection *chatConnection = socialController->chatConnection();

    Chat::BlockList* blockList = socialController->chatConnection()->currentUser()->blockList();
    Chat::JabberID jabberId = chatConnection->nucleusIdToJabberId(nucleusId.toULongLong());
    return blockList->jabberIDs().contains(jabberId);

}

void OriginSocialProxy::onMutualContactLimitReached(const buzz::Jid& jid)
{
    // Emulate what Strophe does
    QJsonArray array;
    QJsonObject presenceObj;

    const QString jidStr = QString(jid.Str().c_str());
    presenceObj["jid"] = jidStr;
    presenceObj["show"] = ConversionHelpers::availabilityToString(Origin::Chat::Presence::Online);
    presenceObj["presenceType"] = "ERROR";

    QJsonObject gameActivityObj;
    presenceObj["gameActivity"] = gameActivityObj;

    array.append(presenceObj);
    emit presenceChanged(array.toVariantList());
}

}
}
}
