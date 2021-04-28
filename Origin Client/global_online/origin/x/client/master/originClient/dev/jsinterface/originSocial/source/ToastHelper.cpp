#include "chat/RemoteUser.h"
#include "chat/Presence.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"

#include "originnotificationdialog.h"
#include "flows/ClientFlow.h"
#include "OriginToastManager.h"
#include "TelemetryAPIDLL.h"
#include "SocialViewController.h"
#include "ToastHelper.h"

using namespace Origin::Services;
using namespace Origin::Engine;
using namespace Origin::Chat;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

    OriginSocialToastHelper::OriginSocialToastHelper(QObject *parent) :
        QObject(parent)

    {
    }

    OriginSocialToastHelper::~OriginSocialToastHelper()
    {
    }

    void OriginSocialToastHelper::updatePresenceToast(Origin::Chat::RemoteUser *contact, const Origin::Chat::Presence& newPresence, const Origin::Chat::Presence& oldPresence)
    {
        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR << "ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR << "OriginToastManager not available";
            return;
        }

        if (contact)
        {
            QString friendName = contact->originId();
            QString boldfriend = QString("<b>%1</b>").arg(friendName);
            // Need to check for game activity and availability fixes EBIBUGS-27532
            if (newPresence.gameActivity() != oldPresence.gameActivity() && (newPresence.availability() != Origin::Chat::Presence::Unavailable))
            {
                // Figure out if the presence event is game-related or broadcast related
                // If we are broadcasting, this takes higher priority since people already know we're playing
                QString gameTitle = newPresence.gameActivity().gameTitle();
                QString broadcastUrl = newPresence.gameActivity().broadcastUrl();
                Origin::Engine::Social::SocialController* socialController = Origin::Engine::LoginController::currentUser()->socialControllerInstance();
                Origin::Engine::Social::AvatarManager* avatarManager = socialController->smallAvatarManager();
                QPixmap avatar = avatarManager->pixmapForNucleusId(contact->nucleusId());
                QString boldfriend = QString("<b>%1</b>").arg(friendName);
                if (broadcastUrl.length())
                {
                    if (oldPresence.gameActivity().broadcastUrl().length() == 0)    // only show "Started broadcasting" message if we were not broadcasting just prior (fixes EBIBUGS-26530)
                    {
                        UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_FRIENDSTARTBROADCAST.name(),
                            tr("ebisu_client_soref_user_started_broadcasting_game").arg(boldfriend).arg(gameTitle));
                        if (toast)
                        {
                            QSignalMapper* signalMapper = new QSignalMapper(toast->content());
                            signalMapper->setMapping(toast, broadcastUrl);
                            ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
                            ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(QString)), this, SLOT(onOpenBroadcastUrl(const QString&)));
                        }
                    }
                }
                else if (oldPresence.gameActivity().broadcastUrl().length())
                {
                    otm->showToast(Services::SETTING_NOTIFY_FRIENDSTOPBROADCAST.name(),
                        tr("ebisu_client_soref_user_stopped_broadcasting_game").arg(boldfriend).arg(oldPresence.gameActivity().gameTitle()));
                }
                // The user is currently playing a game
                else if (gameTitle.length())
                {
                    // If the user wasn't playing a game previously and they are now
                    // OR if the old game title doesn't match the new one.
                    if (oldPresence.gameActivity().isNull() || oldPresence.gameActivity().productId() != newPresence.gameActivity().productId())
                    {
                        // Friend started playing a game and we haven't already told the user about it.
                        showToast(contact, Services::SETTING_NOTIFY_FRIENDSTARTSGAME.name(), tr("ebisu_client_friend_starts_game_message").arg(boldfriend).arg(newPresence.gameActivity().gameTitle()), "", false);
                    }
                }
                else if (oldPresence.gameActivity().gameTitle().length())
                {
                    // If the user isn't playing a game anymore
                    // or the other game they are playing (multiple games open at once) isn't the same game they just closed.
                    if (newPresence.gameActivity().isNull() || oldPresence.gameActivity().productId() != newPresence.gameActivity().productId())
                    {
                        // Stopped playing a game
                        showToast(contact, Services::SETTING_NOTIFY_FRIENDQUITSGAME.name(), tr("ebisu_client_friend_quits_game_message").arg(boldfriend).arg(oldPresence.gameActivity().gameTitle()), "", false);
                    }
                }
            }
            else if (newPresence.availability() != oldPresence.availability())
            {
                switch (newPresence.availability())
                {
                case Origin::Chat::Presence::Online:
                    if (oldPresence.availability() != Origin::Chat::Presence::Away)
                    {
                        // TODO: , dont Play sound);
                        showToast(contact, Services::SETTING_NOTIFY_FRIENDSIGNINGIN.name(), tr("ebisu_client_friend_signing_in_message").arg(boldfriend), "", true);
                    }
                    break;
                case Origin::Chat::Presence::Away:
                    if (oldPresence.availability() == Origin::Chat::Presence::Unavailable)
                    {
                        showToast(contact, Services::SETTING_NOTIFY_FRIENDSIGNINGIN.name(), tr("ebisu_client_friend_signing_in_message").arg(boldfriend), "", true);
                    }
                    break;
                case Origin::Chat::Presence::Unavailable:
                {
                    showToast(contact, Services::SETTING_NOTIFY_FRIENDSIGNINGOUT.name(), tr("ebisu_client_friend_signing_out_message").arg(boldfriend), "", true);
                }
                break;
                default:
                    // ignore other cases
                    break;
                }
            }
        }
    }

    void OriginSocialToastHelper::onOpenBroadcastUrl(const QString& url)
    {
        GetTelemetryInterface()->Metric_BROADCAST_JOINED("NOTIFICATION");
        Services::PlatformService::asyncOpenUrl(QUrl::fromEncoded(url.toUtf8()));
    }

    void OriginSocialToastHelper::updateMessageToast(Origin::Chat::RemoteUser* contact, const Origin::Chat::Message & message)
    {
        const QString title = QString("<b>%0</b>").arg(contact->originId());

        // This used to be a emit signalShowToast made no sense to me, was this neccessary?
        showToast(contact, Services::SETTING_NOTIFY_INCOMINGTEXTCHAT.name(), title, message.body(), true);
    }

    void OriginSocialToastHelper::showToast(Chat::RemoteUser* contact, const QString& toastName, const QString& title, const QString& message, bool showAvatar)
    {
        // Don't send notification if the Desktop Chat window has focus
        if (Services::SETTING_NOTIFY_INCOMINGTEXTCHAT.name() == toastName &&
            ClientFlow::instance()->socialViewController()->chatWindowManager()->isChatWindowActive())
            return;

        // Don't send notification if the OIG Chat window has focus
        if (Origin::Engine::IGOController::instance()->igowm()->visible() && ClientFlow::instance()->socialViewController()->chatWindowManager()->isIGOChatWindowActive())
        {
            return;
        }

        ClientFlow* clientFlow = ClientFlow::instance();
        if (!clientFlow)
        {
            ORIGIN_LOG_ERROR << "ClientFlow instance not available";
            return;
        }
        OriginToastManager* otm = clientFlow->originToastManager();
        if (!otm)
        {
            ORIGIN_LOG_ERROR << "OriginToastManager not available";
            return;
        }

        Origin::Engine::Social::SocialController* socialController = Origin::Engine::LoginController::currentUser()->socialControllerInstance();
        Origin::Engine::Social::AvatarManager* avatarManager = socialController->smallAvatarManager();
        UIToolkit::OriginNotificationDialog* toast = NULL;
        if (showAvatar)
        {
            QPixmap avatar = avatarManager->pixmapForNucleusId(contact->nucleusId());
            toast = otm->showToast(toastName, title, message, avatar);
        }
        else
        {
            toast = otm->showToast(toastName, title, message);
        }
        if (toastName == Services::SETTING_NOTIFY_FRIENDSIGNINGOUT.name())
        {
            // we don't want to listen for this notification.
            return;
        }
        if (toast)
        {
            QSignalMapper* signalMapper = new QSignalMapper(toast->content());
            signalMapper->setMapping(toast, contact);
            ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(showChatWindowForFriend(QObject*)));
        }
    }

    void OriginSocialToastHelper::showChatWindowForFriend(QObject* contact)
    {
        Chat::RemoteUser* remoteUser = dynamic_cast<Chat::RemoteUser*>(contact);
        if (remoteUser)
        {
            ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(remoteUser, Origin::Engine::Social::Conversation::InternalRequest, IIGOCommandController::CallOrigin_CLIENT);
        }
        else
        {
            // Can't find the user - just bring the chat window to the foreground
            ClientFlow::instance()->socialViewController()->chatWindowManager()->raiseMainChatWindow();
        }
    }

}
}
}
