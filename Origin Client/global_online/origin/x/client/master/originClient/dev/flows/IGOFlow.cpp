/////////////////////////////////////////////////////////////////////////////
// IGOFlow.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "IGOFlow.h"
#include "SocialViewController.h"
#include "flows/ClientFlow.h"
#include "engine/igo/IGOController.h"
#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "engine/social/ConversationManager.h"
#include "engine/social/SocialController.h"
#include "engine/igo/IIGOCommandController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "chat/OriginConnection.h"
#include "chat/RemoteUser.h"
#include "chat/Message.h"
#include "widgets/notifications/source/ToastViewController.h"
#include "OriginNotificationDialog.h"
#include "HotkeyToastView.h"
#include "engine/igo/IGOQWidget.h"
#include "LocalizedDateFormatter.h"

namespace Origin
{
    namespace Client
    {
        void IGOFlow::start()
        {
            Engine::IGOController* igoController = Engine::IGOController::instance();

            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(showMyProfile(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showMyProfile(Origin::Engine::IIGOCommandController::CallOrigin)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(showFriendsProfile(qint64, Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showFriendsProfile(qint64, Origin::Engine::IIGOCommandController::CallOrigin)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(showAvatarSelect(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showAvatarSelect(Origin::Engine::IIGOCommandController::CallOrigin)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(showFindFriends(Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showFindFriends(Origin::Engine::IIGOCommandController::CallOrigin)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(showChat(QString, QStringList, QString, Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(showChat(QString, QStringList, QString, Origin::Engine::IIGOCommandController::CallOrigin)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(requestGoOnline()), this, SLOT(onRequestGoOnline()));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(igoShowToast(const QString&, const QString&, const QString&)), this, SLOT(showToast(const QString&, const QString&, const QString&)));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(igoShowOpenOIGToast()), this, SLOT(showOpenOIGToast()));
            ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(igoShowTrialWelcomeToast(const QString&, const qint64&)), this, SLOT(showTrialWelcomeToast(const QString&, const qint64&)));
        }

        void IGOFlow::showMyProfile(Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            ClientFlow::instance()->showMyProfile(IGOScope, ShowProfile_Undefined, callOrigin);
        }

        void IGOFlow::showFriendsProfile(qint64 user, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            ClientFlow::instance()->showProfile(user, IGOScope, ShowProfile_Undefined, callOrigin);
        }

        void IGOFlow::showAvatarSelect(Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            ClientFlow::instance()->showSelectAvatar(IGOScope, callOrigin);
        }

        void IGOFlow::showFindFriends(Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            ClientFlow::instance()->showFriendSearchDialog(IGOScope, callOrigin);
        }

        void IGOFlow::showChat(QString from, QStringList to, QString msg, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            Chat::OriginConnection *conn = Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection();
            for (int i = 0; i < to.length(); i++)
            {
                QString recepient = to[i];

                const Chat::JabberID jid(conn->nucleusIdToJabberId(recepient.toULongLong()));
                Chat::RemoteUser *remoteUser = conn->remoteUserForJabberId(jid);

                ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(remoteUser, Engine::Social::Conversation::InternalRequest, callOrigin);
                if (!msg.isEmpty())
                {
                    remoteUser->sendMessage(msg, Chat::Message::generateUniqueThreadId());
                }                
            }
        }

        void IGOFlow::onRequestGoOnline()
        {
            ClientFlow::instance()->requestOnlineMode();
        }

        void IGOFlow::showOpenOIGToast()
        {
            showToast("POPUP_OIG_HOTKEY", tr("ebisu_client_open_app").arg(tr("ebisu_client_igo_name")), "");
        }

        void IGOFlow::showTrialWelcomeToast(const QString& title, const qint64& timeRemaining)
        {
            // timeRemaining comes in as milliseconds, covert to seconds.
            const qint64 timeRemainingToSeconds = timeRemaining / 1000;
            QString text = tr("ebisu_subs_trial_welcome")
                .arg(LocalizedDateFormatter().formatInterval(timeRemainingToSeconds, LocalizedDateFormatter::Seconds, LocalizedDateFormatter::Days));
            showToast("POPUP_OIG_HOTKEY", text, "");
        }

        void IGOFlow::showToast(const QString& toastName, const QString& title, const QString& message)
        {
            ClientFlow* clientFlow = ClientFlow::instance();
            if (!clientFlow)
            {
                ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
                return;
            }
            OriginToastManager* otm = clientFlow->originToastManager();
            if (!otm)
            {
                ORIGIN_LOG_ERROR<<"OriginToastManager not available";
                return;
            }

            UIToolkit::OriginNotificationDialog* toast = otm->showToast(toastName,title, message);
            // very unlikely but need to consider it
            if (!toast)
                return;
            // HACK
            if(toastName == "POPUP_BROADCAST_STOPPED")
            {
                QSignalMapper* signalMapper = new QSignalMapper(toast->content());
                signalMapper->setMapping(toast, Origin::Engine::IIGOCommandController::CMD_SHOW_BROADCAST);
                ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(int)), Origin::Engine::IGOController::instance(), SIGNAL(igoCommand(int)));
            }
        }
    }
}
