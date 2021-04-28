#ifndef BUDDYLISTJSHELPER_H
#define BUDDYLISTJSHELPER_H

#include <QObject>
#include <QList>

#include "WebWidget/ScriptOwnedObject.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Chat
    {
        class XMPPUser;
        class RemoteUser;
        class OriginConnection;
    }

    namespace Client
    {
        class ORIGIN_PLUGIN_API BuddyPresenceJs : public WebWidget::ScriptOwnedObject
        {
            Q_OBJECT
        public:
            BuddyPresenceJs(Chat::OriginConnection *conn, Chat::XMPPUser *user);

            Q_PROPERTY(QString userId READ userId);
            Q_PROPERTY(QString presence READ presence);
            Q_PROPERTY(QString richPresence READ richPresence);

            QString userId() const { return mUserId; }
            QString presence() const { return mPresence; }
            QString richPresence() const { return mRichPresence; }

        protected:
            QString mUserId;
            QString mPresence;
            QString mRichPresence;

        private:
            Q_DISABLE_COPY(BuddyPresenceJs);
        };

        class ORIGIN_PLUGIN_API BuddyListJsHelper : public QObject
        {
            Q_OBJECT
        public:
            BuddyListJsHelper(Chat::OriginConnection *conn, QObject *parent = NULL);

            public slots:
                Q_INVOKABLE QObject* MyCurrentPresence();

                Q_INVOKABLE void queryFriends();
                Q_INVOKABLE void getUserPresence();

signals:
                void presenceChanged(QObject*);
                void SelfPresenceChanged(QObject*);

                protected slots:
                    void queryFriendsAsync();
                    void getUserPresenceAsync();

                    void subscribeToContact(Origin::Chat::RemoteUser *);
                    void onContactChanged();
                    void onConnectedUserChanged();

        protected:
            Chat::OriginConnection *mConn;
        };

    }
}

#endif
