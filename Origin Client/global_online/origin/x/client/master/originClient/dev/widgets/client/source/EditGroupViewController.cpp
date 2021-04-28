/////////////////////////////////////////////////////////////////////////////
// EditGroupViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EditGroupViewController.h"
#include "CreateGroupView.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "chat/ChatGroup.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "originwindow.h"
#include "originpushbutton.h"

namespace Origin
{
    namespace Client
    {

        EditGroupViewController::EditGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent /* = 0 */)
            : QObject(parent)
            , mEditGroupView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
        {

        }

        EditGroupViewController::~EditGroupViewController()
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

        UIToolkit::OriginWindow* EditGroupViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            mEditGroupView = new CreateGroupView(NULL);
            mEditGroupView->setGroupName(mGroupName);

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->setTitleBarText(tr("application_name"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_edit_chat_group_title"), tr("ebisu_client_create_chat_group_text"), mEditGroupView);
            mEditGroupView->setFocus();
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onEditGroup()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));
            
            ORIGIN_VERIFY_CONNECT(mEditGroupView, SIGNAL(acceptButtonEnabled(bool)), 
                this, SLOT(onAcceptButtonEnabled(bool)));

            return myWindow;
        }

        void EditGroupViewController::show(const UIScope& scope)
        {
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mWindow = createWindow(scope);
                    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                }
                mWindow->showWindow();
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

        void EditGroupViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void EditGroupViewController::closeIGOWindow()
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

        void EditGroupViewController::closeWindow()
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

        void EditGroupViewController::onEditGroup()
        {
            Origin::Services::GroupChangeNameResponse* resp = Services::GroupServiceClient::changeGroupName(
                Services::Session::SessionService::currentSession(), mGroupGuid, mEditGroupView->getGroupName());
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onEditGroupSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onEditGroupFail()));
        }

        void EditGroupViewController::onEditGroupSuccess()
        {
            Origin::Services::GroupChangeNameResponse* resp = dynamic_cast<Origin::Services::GroupChangeNameResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                    socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

                groups->editGroupName(resp->getGroupGuid(), resp->getGroupName());
            }

            close();
        }

        void EditGroupViewController::onEditGroupFail()
        {
            // Close edit group dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_edit_room");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_group_edit_room"));

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

        void EditGroupViewController::onAcceptButtonEnabled(bool isEnabled)
        {
            if (mIGOWindow)
            {
                mIGOWindow->button(QDialogButtonBox::Ok)->setEnabled(isEnabled);
            } 
            else 
            {
                mWindow->button(QDialogButtonBox::Ok)->setEnabled(isEnabled);
            }
        }
    }
}