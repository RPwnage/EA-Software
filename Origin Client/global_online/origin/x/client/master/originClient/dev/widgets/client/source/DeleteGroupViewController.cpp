/////////////////////////////////////////////////////////////////////////////
// DeleteGroupViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "DeleteGroupViewController.h"
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
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Client
    {

        DeleteGroupViewController::DeleteGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
        {

        }

        DeleteGroupViewController::~DeleteGroupViewController()
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

        UIToolkit::OriginWindow* DeleteGroupViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->setTitleBarText(tr("ebisu_client_delete_chat_group"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisebisu_client_delete_chat_group_title").arg(mGroupName.toHtmlEscaped()), tr("ebisu_client_delete_chat_group_text_w_voice"));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_delete_chat_group"));

            // Check to see whether QA testing is forcing a 'delete group' to fail
            if (Origin::Services::readSetting(Origin::Services::SETTING_ForceChatGroupDeleteFail))
            {
                ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onDeleteGroupFail()));
            }
            else
            {
                ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onDeleteGroup()));
            }
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void DeleteGroupViewController::show(const UIScope& scope)
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

        void DeleteGroupViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void DeleteGroupViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
                emit windowClosed();
            }
        }

        void DeleteGroupViewController::closeWindow()
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

        void DeleteGroupViewController::onDeleteGroup()
        {
            // Prepare for group deletion
            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();
            ORIGIN_ASSERT(groups);
            if (groups)
            {
                groups->prepareForGroupDeletion(mGroupGuid);
            }

            // Request group deletion on server
            Origin::Services::GroupDeleteResponse* resp = Services::GroupServiceClient::deleteGroup(
                Services::Session::SessionService::currentSession(), mGroupGuid);
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onDeleteGroupSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onDeleteGroupFail()));
        }

        void DeleteGroupViewController::onDeleteGroupSuccess()
        {
            Origin::Services::GroupDeleteResponse* resp = dynamic_cast<Origin::Services::GroupDeleteResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QString groupGuid = resp->getGroupGuid();
                Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                    socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

                groups->removeGroupUI(groupGuid);
                // Send telemetry that we are deleting our group
                GetTelemetryInterface()->Metric_CHATROOM_DELETE_GROUP(groupGuid.toLatin1(), groups->count());

                emit groupDeleted(groupGuid);
            }

            close();
        }

        void DeleteGroupViewController::onDeleteGroupFail()
        {
            // save for 'ForceChatGroupDeleteFail'
            QString groupGuid = mGroupGuid;

            // Close delete group dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_delete_room");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_delete_chat_room"));

            if (Engine::IGOController::instance()->isActive())
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }

            //
            // send telemetry that the 'delete group' failed
            //
            if (!Origin::Services::readSetting(Origin::Services::SETTING_ForceChatGroupDeleteFail))
            {
                Origin::Services::GroupDeleteResponse* resp = dynamic_cast<Origin::Services::GroupDeleteResponse*>(sender());
                ORIGIN_ASSERT(resp);

                if (resp != NULL)
                {
                    groupGuid = resp->getGroupGuid();
                }
            }

            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

            // Send telemetry that the 'delete group' failed
            GetTelemetryInterface()->Metric_CHATROOM_DELETE_GROUP_FAILED(groupGuid.toLatin1(), groups->count());

        }
    }
}
