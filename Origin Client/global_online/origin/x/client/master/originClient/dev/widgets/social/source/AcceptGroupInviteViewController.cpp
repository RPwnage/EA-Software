/////////////////////////////////////////////////////////////////////////////
// GroupInviteViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "AcceptGroupInviteViewController.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "chat/ChatGroup.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "originpushbutton.h"
#include "originwindow.h"

namespace Origin
{
    namespace Client
    {

        AcceptGroupInviteViewController::AcceptGroupInviteViewController(const QString& inviteFrom, const QString& groupName, const QString& groupGuid, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mInviteFrom(inviteFrom)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
        {

        }

        AcceptGroupInviteViewController::~AcceptGroupInviteViewController()
        {
            if(mWindow)
            {
                delete mWindow;
                mWindow = NULL;
            }

            if(mIGOWindow)
            {
                delete mIGOWindow;
                mIGOWindow = NULL;
            }
        }

        UIToolkit::OriginWindow* AcceptGroupInviteViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            QString body("%1\n\n%2");

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->setTitleBarText(tr("ebisu_client_invitation_to_a_chat_group"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_youve_been_invited_to_a_chat_group_title"), body.arg(tr("ebisu_client_has_invited_you_to").arg(mInviteFrom).arg(mGroupName)).arg(tr("ebisu_client_hang_out_and_talk_w_friends_create_additional")));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_accept"));
            myWindow->setButtonText(QDialogButtonBox::Cancel, tr("ebisu_client_decline"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onAcceptInvite()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onDeclineInvite()));

            return myWindow;
        }

        void AcceptGroupInviteViewController::show(const UIScope& scope)
        {
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mWindow = createWindow(scope);
                    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                }
                else
                {
                    mWindow->activateWindow();
                }
                mWindow->showWindow();

                //normally we don't want to call activateWindow because it would steal focus if this window all of a sudden popped up while the user was on another application.
                //however since these windows are only triggered from menu or origin:// its ok. If we don't activate the window, when using origin:// the window will show up minimized
                mWindow->activateWindow();
            }
            else if (scope == IGOScope && Engine::IGOController::instance() != NULL)
            {
                if (mIGOWindow == NULL)
                {
                    mIGOWindow = createWindow(scope);
                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, Engine::IIGOWindowManager::WindowProperties());
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                }
            }
        }

        void AcceptGroupInviteViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();        
        }
        
        void AcceptGroupInviteViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void AcceptGroupInviteViewController::closeWindow()
        {
            if (mWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow->deleteLater();
                mWindow = NULL;
                emit windowClosed();
            }
        }

        void AcceptGroupInviteViewController::onAcceptInvite()
        {
            Services::GroupJoinResponse* resp = Services::GroupServiceClient::joinGroup(
                Services::Session::SessionService::currentSession(), mGroupGuid);
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onAcceptInviteSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onAcceptInviteFail()));
        }

        void AcceptGroupInviteViewController::onAcceptInviteSuccess()
        {
            Origin::Services::GroupJoinResponse* resp = dynamic_cast<Origin::Services::GroupJoinResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                    socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

                groups->joinGroup(mGroupGuid, mGroupName);
            }
            close();
        }

        void AcceptGroupInviteViewController::onAcceptInviteFail()
        {
            // Close accept group dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_accept_invite");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_accept_room_invite"));

            if (Engine::IGOController::instance()->isActive())
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void AcceptGroupInviteViewController::onDeclineInvite()
        {
            close();
        }
    }
}
