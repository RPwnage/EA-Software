/////////////////////////////////////////////////////////////////////////////
// LeaveGroupViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "LeaveGroupViewController.h"
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

        LeaveGroupViewController::LeaveGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
        {

        }

        LeaveGroupViewController::~LeaveGroupViewController()
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

        UIToolkit::OriginWindow* LeaveGroupViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            QString body("%1<br><br>%2");

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->setTitleBarText(tr("ebisu_client_group_leave"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_leave_group_chat_title"), body.arg(tr("ebisu_client_leaving_group_chat").arg(mGroupName.toHtmlEscaped())).arg(tr("ebisu_client_once_you_leave_the_group_you_will_not_text")));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_group_leave"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onLeaveGroup()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void LeaveGroupViewController::show(const UIScope& scope)
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

        void LeaveGroupViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void LeaveGroupViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }

            if (!mWindow && !mIGOWindow)
                emit windowClosed();
        }

        void LeaveGroupViewController::closeWindow()
        {
            if (mWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow->deleteLater();
                mWindow = NULL;
            }

            if (!mWindow && !mIGOWindow)
                emit windowClosed();
        }

        void LeaveGroupViewController::onLeaveGroup()
        {
            //Before attempting to leave group, check that this user has not been made group owner
            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();
            Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupGuid);
            // check for the group here to handle race condition between getting 
            // kicked out of and group and leaving a group
            if (group && group->getGroupRole()!="superuser")
            {
                Origin::Services::GroupLeaveResponse* resp = Services::GroupServiceClient::leaveGroup(
                    Services::Session::SessionService::currentSession(), mGroupGuid);
                resp->setDeleteOnFinish(true);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onLeaveGroupSuccess()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onLeaveGroupFail()));
            } else {
                onLeaveGroupFail();
            }
        }

        void LeaveGroupViewController::onLeaveGroupSuccess()
        {
            Origin::Services::GroupLeaveResponse* resp = dynamic_cast<Origin::Services::GroupLeaveResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QString groupGuid = resp->getGroupGuid();
                Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                    socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

                groups->leaveGroup(groupGuid);

                emit groupDeparted(groupGuid);
            }

            close();
        }

        void LeaveGroupViewController::onLeaveGroupFail()
        {
            // Close leave group dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_leave_group");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_group_leave"));

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
    }
}
