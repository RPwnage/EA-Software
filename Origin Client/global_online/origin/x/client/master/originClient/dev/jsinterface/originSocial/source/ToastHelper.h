#ifndef _ORIGINSOCIALPRESENCETOAST_H
#define _ORIGINSOCIALPRESENCETOAST_H

#include <QVariantMap>
#include "services/plugin/PluginAPI.h"
#include "chat/Connection.h"
namespace Origin
{

    namespace Chat
    {
        class RemoteUser;
        class Presence;
    }
    namespace Client
    {
        namespace JsInterface
        {
            class ORIGIN_PLUGIN_API OriginSocialToastHelper : public QObject
            {
                Q_OBJECT
            public:
                OriginSocialToastHelper(QObject *parent = NULL);
                virtual ~OriginSocialToastHelper();
                void updatePresenceToast(Origin::Chat::RemoteUser *contact, const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence);
                void updateMessageToast(Origin::Chat::RemoteUser* contact, const Origin::Chat::Message & message);
            private slots:
                void onOpenBroadcastUrl(const QString& url);
                void showToast(Chat::RemoteUser* contact, const QString& toastName, const QString& title, const QString& message, bool showAvatar);
                void showChatWindowForFriend(QObject* contact);

            };

        }
    }
}

#endif
