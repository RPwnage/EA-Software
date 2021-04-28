/////////////////////////////////////////////////////////////////////////////
// EnterRoomPasswordViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EnterRoomPasswordViewController.h"
#include "EnterRoomPasswordView.h"
#include "services/debug/DebugService.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "chat/ChatGroup.h"
#include "chat/ChatChannel.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "originpushbutton.h"
#include "originwindow.h"

namespace Origin
{
    namespace Client
    {

        EnterRoomPasswordViewController::EnterRoomPasswordViewController(const QString& groupGuid, const QString& channelId, bool deleteRoom, QObject* parent)
            : QObject(parent)
            , mEnterRoomPasswordView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupGuid(groupGuid)
            , mChannelId(channelId)
            , mDeleteRoom(deleteRoom)
        {

        }

        EnterRoomPasswordViewController::~EnterRoomPasswordViewController()
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

        UIToolkit::OriginWindow* EnterRoomPasswordViewController::createWindow(const UIScope& scope)
        {
            using namespace UIToolkit;
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            mEnterRoomPasswordView = new EnterRoomPasswordView(NULL);
            mEnterRoomPasswordView->setNormal();

            OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::MsgBox,
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, windowContext);
            if(mDeleteRoom)
            {
                myWindow->setTitleBarText(tr("application_name"));
                myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_password_required_title"), tr("ebisu_client_chat_room_password_protected"));
                myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_delete_chat_room"));
                ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onDeleteRoom()));
            }
            else
            {
                myWindow->setTitleBarText(tr("application_name"));
                myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_group_enter_password_title"), tr("ebisu_client_group_enter_password_text"));
                ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onEnterRoom()));
            }
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void EnterRoomPasswordViewController::show(const UIScope& scope)
        {
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mWindow = createWindow(scope);
                    mWindow->setContent(mEnterRoomPasswordView);
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
                    mIGOWindow->setContent(mEnterRoomPasswordView);
                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, Engine::IIGOWindowManager::WindowProperties());
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                }
            }
        }

        void EnterRoomPasswordViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void EnterRoomPasswordViewController::closeIGOWindow()
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

        void EnterRoomPasswordViewController::closeWindow()
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

        void EnterRoomPasswordViewController::onEnterRoom()
        {
            mEnterRoomPasswordView->setValidating();

            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection();
            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

            ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(enteringMucRoom(const QString &, Origin::Chat::EnterMucRoomOperation *)),
                this, SLOT(onEnteringMucRoom(const QString &, Origin::Chat::EnterMucRoomOperation *)));

            groups->enterChannelInGroup(mGroupGuid, mChannelId, mEnterRoomPasswordView->getPassword());
        }

        void EnterRoomPasswordViewController::onEnteringMucRoom(const QString &roomId, Origin::Chat::EnterMucRoomOperation *op)
        {
            if (roomId != mChannelId)
            {
                return;
            }

            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection();

            ORIGIN_VERIFY_DISCONNECT(
                connection, SIGNAL(enteringMucRoom(const QString &, Origin::Chat::EnterMucRoomOperation *)),
                this, SLOT(onEnteringMucRoom(const QString &, Origin::Chat::EnterMucRoomOperation *)));

            ORIGIN_VERIFY_CONNECT(
                op, SIGNAL(succeeded(Origin::Chat::MucRoom*)), 
                this, SLOT(onRoomEntered(Origin::Chat::MucRoom*)))

            ORIGIN_VERIFY_CONNECT(
                op, SIGNAL(failed(Origin::Chat::ChatroomEnteredStatus)), 
                this, SLOT(onRoomEnterFailed(Origin::Chat::ChatroomEnteredStatus)));
        }

        void EnterRoomPasswordViewController::onRoomEntered(Origin::Chat::MucRoom* room)
        {
            // Need to write out this password to disk so we can automatically enter the room later
            Services::Setting roomPasswordSetting(mChannelId, Services::Variant::String, "", Services::Setting::LocalPerAccount, Services::Setting::ReadWrite, Services::Setting::Encrypted);
            Services::writeSetting(roomPasswordSetting, mEnterRoomPasswordView->getPassword());

            Chat::ChatChannel* channel = dynamic_cast<Chat::ChatChannel*>(room);
            if (room)
            {
                channel->unlockChannel();
            }
            else
            {
                ORIGIN_ASSERT(0);
            }

            close();
        }

        void EnterRoomPasswordViewController::onRoomEnterFailed(Origin::Chat::ChatroomEnteredStatus status)
        {
            mEnterRoomPasswordView->setError();
        }

        void EnterRoomPasswordViewController::onDeleteRoom()
        {
            mEnterRoomPasswordView->setValidating();

            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection();
            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

            ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(destroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)),
                this, SLOT(onDestroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)));

            const QString originId = Engine::LoginController::instance()->currentUser()->eaid();
            const QString nucleusId = QString::number(Engine::LoginController::instance()->currentUser()->userId());

            groups->removeChannelFromGroup(mGroupGuid, mChannelId, originId, nucleusId, mEnterRoomPasswordView->getPassword());
        }

        void EnterRoomPasswordViewController::onDestroyingMucRoom(const QString &roomId, Origin::Chat::DestroyMucRoomOperation *op)
        {
            if (roomId != mChannelId)
            {
                return;
            }

            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection();

            ORIGIN_VERIFY_DISCONNECT(
                connection, SIGNAL(destroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)),
                this, SLOT(onDestroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)));

            ORIGIN_VERIFY_CONNECT(
                op, SIGNAL(succeeded()),
                this, SLOT(onRoomDeleted()));

                ORIGIN_VERIFY_CONNECT(
                op, SIGNAL(failed()),
                this, SLOT(onRoomEnterFailed()));
        }

        void EnterRoomPasswordViewController::onRoomDeleted()
        {
            close();
        }

    }
}
