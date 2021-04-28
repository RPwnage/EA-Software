/////////////////////////////////////////////////////////////////////////////
// RemoveGroupUserViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "RemoveGroupUserViewController.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "OriginSocialProxy.h"
#include "engine/igo/IGOController.h"
#include "originpushbutton.h"
#include "originwindow.h"

namespace Origin
{
    namespace Client
    {

        RemoveGroupUserViewController::RemoveGroupUserViewController(const QString& groupGuid, const QString& userId, const QString& userNickname, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupGuid(groupGuid)
            , mUserNickname(userNickname)
            , mUserId(userId)
        {

        }

        RemoveGroupUserViewController::~RemoveGroupUserViewController()
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

        UIToolkit::OriginWindow* RemoveGroupUserViewController::createWindow(const UIScope& scope)
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
            myWindow->setTitleBarText(tr("ebisu_client_kick_from_chat_group"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_remove_this_person_from_text"), body.arg(tr("ebisu_client_removing_from_the_chat_group").arg(mUserNickname)).arg(tr("ebisu_client_anything_this_user_is_doing_will_be_interrupted_text")));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_kick_from_chat_group"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onRemoveUser()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void RemoveGroupUserViewController::setUserNickname(const QString& nickName)
        {
            using namespace UIToolkit;
            mUserNickname = nickName;
            if (mWindow)
            {        
                QString body("%1\n\n%2");
                mWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_remove_this_person_from_text"), body.arg(tr("ebisu_client_removing_from_the_chat_group").arg(mUserNickname)).arg(tr("ebisu_client_anything_this_user_is_doing_will_be_interrupted_text")));
            }

            if (mIGOWindow)
            {
                QString body("%1\n\n%2");
                mIGOWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_remove_this_person_from_text"), body.arg(tr("ebisu_client_removing_from_the_chat_group").arg(mUserNickname)).arg(tr("ebisu_client_anything_this_user_is_doing_will_be_interrupted_text")));
            }
        }

        void RemoveGroupUserViewController::show(const UIScope& scope)
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

        void RemoveGroupUserViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void RemoveGroupUserViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void RemoveGroupUserViewController::closeWindow()
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

        void RemoveGroupUserViewController::onRemoveUser()
        {
            Origin::Services::GroupRemoveMemberResponse* resp = Services::GroupServiceClient::removeGroupMember(
                Services::Session::SessionService::currentSession(), mGroupGuid, mUserId);
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onRemoveUserSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onRemoveUserFail()));
        }

        void RemoveGroupUserViewController::onRemoveUserSuccess()
        {
            close();
        }

        void RemoveGroupUserViewController::onRemoveUserFail()
        {
            // Close remove group user dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_remove_user");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_remove_from_chat_room"));

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
