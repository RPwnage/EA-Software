#ifndef _OriginSocialProxy_H
#define _OriginSocialProxy_H

#include <QVariantMap>
#include "services/plugin/PluginAPI.h"
#include "chat/Connection.h"
namespace Origin
{

    namespace Chat
    {
        class RemoteUser;
        class Roster;
        class ConnectedUser;
        class OriginConnection;
        class Presence;
        class JabberID;
        class Message;
        class Conversable;
        class MucRoom;
    }
    namespace Client
    {
        namespace JsInterface
        {
            class OriginSocialToastHelper;

            class ORIGIN_PLUGIN_API OriginSocialProxy : public QObject
            {
                Q_OBJECT
            public:
                OriginSocialProxy(QObject *parent = NULL);
                virtual ~OriginSocialProxy();

                static OriginSocialProxy *instance();

                Q_INVOKABLE bool isRosterLoaded();
                Q_INVOKABLE bool isConnectionEstablished();
                Q_INVOKABLE QVariantList roster();
                Q_INVOKABLE void setInitialPresence(QString presence);
                Q_INVOKABLE void sendMessage(QString id, QString msgBody, QString type);
                Q_INVOKABLE void setTypingState(QString state, QString remoteUserId);
                Q_INVOKABLE void requestPresenceChange(QString presence);
                Q_INVOKABLE QVariantList requestInitialPresenceForUserAndFriends();
                Q_INVOKABLE void approveSubscriptionRequest(QString id);
                Q_INVOKABLE void denySubscriptionRequest(QString id);
                Q_INVOKABLE void cancelSubscriptionRequest(QString id);
                Q_INVOKABLE void removeFriend(QString id);
                Q_INVOKABLE void joinRoom(QString jid, QString originId);
                Q_INVOKABLE void leaveRoom(QString jid, QString originId);
                Q_INVOKABLE void subscriptionRequest(const QString &id);
                Q_INVOKABLE void blockUser(const QString &nucleusId);
                Q_INVOKABLE void unblockUser(const QString &nucleusId);
                Q_INVOKABLE bool isBlocked(const QString& nucleusId);

            signals:
                void rosterLoaded(QVariantMap rosterObj);
                void presenceChanged(QVariantList);
                void connectionChanged(bool);
                void messageReceived(QVariantMap messageObj);
                void chatStateReceived(QVariantMap chatStateObj);
                void rosterChanged(QVariantMap rosterChangedObj);
                void blockListChanged();

            private slots:
                void onRosterLoaded();
                void onCurrentUserPresenceChanged(const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence);
                void onRemoteUserPresenceChanged(const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence);
                void onChatStateReceived(const Origin::Chat::Message &msg);
                void onMessageReceived(Origin::Chat::Conversable *conv, const Origin::Chat::Message &msg);
                void onRosterChanged(Chat::RemoteUser *remoteUser, const QString& subState);
                void onSubscriptionRequested(const Origin::Chat::JabberID &jid);
                void onContactAdded(Origin::Chat::RemoteUser *remoteUser);
                void onContactRemoved(Origin::Chat::RemoteUser *remoteUser);
                void onContactChanged();
                void onConnected();
                void onDisconnected();
                void onEnteredCreatedMucRoom(Origin::Chat::MucRoom *room);
                void onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus);
                void onUserJoined(const Origin::Chat::JabberID & user, const Origin::Chat::MucRoom *room);
                void onUserLeft(const Origin::Chat::JabberID & user, const Origin::Chat::MucRoom *room);
                void onContactMessageReceived(const Origin::Chat::Message & message);
                void onMutualContactLimitReached(const buzz::Jid&);
            private:
                void listenForRemoteUserPresence();
                void setRequestedVisibility(const QString &str);
                void setRequestedAvailability(const QString &str);
                
                QJsonArray buildFullRoster();
                QJsonObject buildRemoteUserObject(Origin::Chat::RemoteUser *remoteUser);
                QJsonObject buildReceivedMessage(const Chat::Message &msg, const Chat::JabberID& jid);
                QJsonObject createPresenceJson(const Origin::Chat::JabberID& jid, const Origin::Chat::Presence& presence);
                QJsonObject createPresenceJson(const Origin::Chat::JabberID& user, const Origin::Chat::MucRoom *room, const Origin::Chat::Presence& presence);
                QJsonObject createSubscribeJson(const Origin::Chat::JabberID& jid);

                void sendRosterChanged(Chat::RemoteUser *remoteUser, const QString& subState, bool subReqSent);
                QVariantMap mRosterVariantMap;
                Chat::Roster *mRoster;
                Chat::ConnectedUser *mCurrentUser;
                Chat::OriginConnection *mChatConnection;
                QHash<QString, Chat::MucRoom*> mActiveMucRooms;
                OriginSocialToastHelper *mToastHelper;
            };

        }
    }
}

#endif
