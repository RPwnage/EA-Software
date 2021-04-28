/////////////////////////////////////////////////////////////////////////////
// CreateRoomViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CreateRoomViewController.h"
#include "CreateRoomView.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "chat/ChatChannel.h"
#include "chat/ChatGroup.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "originwindow.h"
#include "originpushbutton.h"

namespace Origin
{
    namespace Client
    {

        CreateRoomViewController::CreateRoomViewController(const QString& groupName, const QString& groupGuid, QObject* parent)
            : QObject(parent)
            , mCreateRoomView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupName(groupName)
            , mGroupGuid(groupGuid)
        {

        }

        CreateRoomViewController::~CreateRoomViewController()
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

        UIToolkit::OriginWindow* CreateRoomViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            mCreateRoomView = new CreateRoomView(NULL);

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            
            myWindow->setTitleBarText(tr("application_name"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_create_chat_room_title"), tr("ebisu_client_room_is_a_separate_area_of_your_chat_group"));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_create"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), 
                this, SLOT(onCreateRoom()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), 
                this, SLOT(close()));
            
            ORIGIN_VERIFY_CONNECT(mCreateRoomView, SIGNAL(acceptButtonEnabled(bool)), 
                this, SLOT(onAcceptButtonEnabled(bool)));

            myWindow->button(QDialogButtonBox::Ok)->setEnabled(false);

            return myWindow;
        }

        void CreateRoomViewController::show(const UIScope& scope)
        {
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mWindow = createWindow(scope);
                    mWindow->setContent(mCreateRoomView);
                    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                }
                else
                {
                    mWindow->activateWindow();
                }
                mWindow->showWindow();
            }
            else if (scope == IGOScope && Engine::IGOController::instance() != NULL)
            {
                if (mIGOWindow == NULL)
                {
                    mIGOWindow = createWindow(scope);
                    mIGOWindow->setContent(mCreateRoomView);
                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, Engine::IIGOWindowManager::WindowProperties());
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                }
            }
        }

        void CreateRoomViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void CreateRoomViewController::closeIGOWindow()
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

        void CreateRoomViewController::closeWindow()
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

        void CreateRoomViewController::onCreateRoom()
        {
            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

            Chat::ChatChannel* channel = groups->addChannelToGroup(mGroupGuid, mCreateRoomView->getRoomName(),
                mCreateRoomView->isRoomLocked());

            QString password;
            if (mCreateRoomView->isRoomLocked())
            {
                password = mCreateRoomView->getPassword();
            }

            channel->createChannel(password);
            channel->unlockChannel();

            if (mCreateRoomView->isRoomLocked())
            {
                // Need to write out this password to disk so we can automatically login later
                Services::Setting roomPasswordSetting(channel->getChannelId(), Services::Variant::String, "", Services::Setting::LocalPerAccount, Services::Setting::ReadWrite, Services::Setting::Encrypted);
                Services::writeSetting(roomPasswordSetting, mCreateRoomView->getPassword());
            }

            close();
        }

        void CreateRoomViewController::onAcceptButtonEnabled(bool isEnabled)
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
