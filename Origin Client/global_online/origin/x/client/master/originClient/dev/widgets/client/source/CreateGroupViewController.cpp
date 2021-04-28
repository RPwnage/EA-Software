/////////////////////////////////////////////////////////////////////////////
// CreateGroupViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CreateGroupViewController.h"
#include "CreateGroupView.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/rest/GroupServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "chat/ChatGroup.h"
#include "chat/OriginConnection.h"
#include "originwindow.h"
#include "originpushbutton.h"

namespace Origin
{
    namespace Client
    {

        CreateGroupViewController::CreateGroupViewController(QObject* parent)
            : QObject(parent)
            , mCreateGroupView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
        {

        }

        CreateGroupViewController::~CreateGroupViewController()
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

        UIToolkit::OriginWindow* CreateGroupViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            mCreateGroupView = new CreateGroupView(NULL);

            OriginWindow* myWindow = new OriginWindow(titlebarItems, mCreateGroupView, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            myWindow->setTitleBarText(tr("application_name"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_create_chat_room"), tr("ebisu_client_create_chat_group_text"));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_create"));
            mCreateGroupView->setFocus();
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onCreateGroup()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            // close this window if the current user goes invisible
            ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser(), SIGNAL(visibilityChanged(Origin::Chat::ConnectedUser::Visibility)), this, SLOT(onVisiblityChanged(Origin::Chat::ConnectedUser::Visibility)));
            
            ORIGIN_VERIFY_CONNECT(mCreateGroupView, SIGNAL(acceptButtonEnabled(bool)), 
                this, SLOT(onAcceptButtonEnabled(bool)));

            myWindow->button(QDialogButtonBox::Ok)->setEnabled(false);

            return myWindow;
        }

        void CreateGroupViewController::show(const UIScope& scope)
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

        void CreateGroupViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void CreateGroupViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void CreateGroupViewController::closeWindow()
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

        void CreateGroupViewController::onCreateGroup()
        {
            // Disconnect here so we only create one group
            if (mWindow) 
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onCreateGroup()));
            }

            if (mIGOWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onCreateGroup()));
            }
            
            Origin::Services::GroupCreateResponse* resp = Services::GroupServiceClient::createGroup(
                Services::Session::SessionService::currentSession(), mCreateGroupView->getGroupName(), "test");
            resp->setDeleteOnFinish(true);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onCreateGroupSuccess()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onCreateGroupError()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(createGroupFailed()), this, SLOT(onCreateGroupFail()));
        }

        void CreateGroupViewController::onCreateGroupSuccess()
        {
            Origin::Services::GroupCreateResponse* resp = dynamic_cast<Origin::Services::GroupCreateResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                    socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

                groups->createGroup(resp->getGroupGuid(), resp->getGroupName());

                emit(createGroupSucceeded(resp->getGroupGuid()));
            }

            close();
        }

        void CreateGroupViewController::onCreateGroupFail()
        {
            // Close create group dialog
            close();

            Origin::Services::GroupCreateResponse* resp = dynamic_cast<Origin::Services::GroupCreateResponse*>(sender());
            ORIGIN_ASSERT(resp);

            Services::GroupResponse::GroupResponseError errorCode = resp->getGroupResponseError();

            UIToolkit::OriginWindow* window = NULL;

            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_create_room");
            QString description = tr("ebisu_client_error_try_again_later");

            // Bring up error dialog specific to error returned
            if (errorCode == Services::GroupResponse::GroupResponseError::TooManyGroupCreatePerDayError)
            {
                description = tr("ebisu_client_you_have_exceeded_the_max_number_of_groups_text");
                window = UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_ok"));

            } 
            else if (errorCode == Services::GroupResponse::GroupResponseError::TooManyGroupCreatePerMinuteError)
            {
                description = tr("ebisu_client_created_too_many_groups_too_quickly");
                window = UIToolkit::OriginWindow::promptNonModal(icon, heading, description, tr("ebisu_client_create_group"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
                ORIGIN_VERIFY_CONNECT(window, SIGNAL(finished(int)), this, SLOT(onCreateGroupFailPromptClosed(int)));
            }  

            if (window)
            {
                window->setTitleBarText(tr("application_name"));

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

        void CreateGroupViewController::onCreateGroupError()
        {
            // Close create group dialog
            close();

            // Bring up generic error dialog
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;
            QString heading = tr("ebisu_client_unable_to_create_room");
            QString description = tr("ebisu_client_error_try_again_later");

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_create_chat_room"));

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

        void CreateGroupViewController::onCreateGroupFailPromptClosed(int result)
        {
            if (result == QDialog::Accepted)
            {
                ClientFlow::instance()->view()->showCreateGroupDialog();                
            }
        }
        void CreateGroupViewController::onAcceptButtonEnabled(bool isEnabled)
        {
            if (mIGOWindow)
            {
                mIGOWindow->button(QDialogButtonBox::Ok)->setEnabled(isEnabled);
            } else {
                mWindow->button(QDialogButtonBox::Ok)->setEnabled(isEnabled);
            }
        }

        // close this window if the current user goes invisible
        void CreateGroupViewController::onVisiblityChanged(Origin::Chat::ConnectedUser::Visibility vis)
        {
            if (vis==Origin::Chat::ConnectedUser::Invisible)
            {
                close();
            }
        }
    }
}