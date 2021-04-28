/////////////////////////////////////////////////////////////////////////////
// DeleteRoomViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "DeleteRoomViewController.h"
#include "services/debug/DebugService.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "flows/ClientFlow.h"
#include "chat/ChatGroup.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "originpushbutton.h"
#include "originwindow.h"

namespace Origin
{
    namespace Client
    {

        DeleteRoomViewController::DeleteRoomViewController(const QString& groupGuid, const QString& channelId, const QString& roomName, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupGuid(groupGuid)
            , mChannelId(channelId)
            , mRoomName(roomName)
        {

        }

        DeleteRoomViewController::~DeleteRoomViewController()
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

        UIToolkit::OriginWindow* DeleteRoomViewController::createWindow(const UIScope& scope)
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
            myWindow->setTitleBarText(tr("ebisu_client_delete_chat_room"));
            myWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisebisu_client_delete_chat_group_title").arg(mRoomName), tr("ebisu_client_this_action_cannot_be_undone_everyone_text"));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_delete_chat_room"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onDeleteRoom()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void DeleteRoomViewController::show(const UIScope& scope)
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

        void DeleteRoomViewController::closeIGOWindow()
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

        void DeleteRoomViewController::closeWindow()
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

        void DeleteRoomViewController::hideWindow()
        {
            if (mIGOWindow)
            {
                mIGOWindow->setVisible(false);
            }
            else if (mWindow)
            {
                mWindow->setVisible(false);
            }
            else
            {
                ORIGIN_ASSERT_MESSAGE(false, "No valid window to hide.");
            }
        }

        void DeleteRoomViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void DeleteRoomViewController::onDeleteRoom()
        {
            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection();

            Chat::ChatGroups* groups = Engine::LoginController::instance()->currentUser()->
                socialControllerInstance()->chatConnection()->currentUser()->chatGroups();

            ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(destroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)),
                this, SLOT(onDestroyingMucRoom(const QString &, Origin::Chat::DestroyMucRoomOperation *)));

            const QString originId = Engine::LoginController::instance()->currentUser()->eaid();
            const QString nucleusId = QString::number(Engine::LoginController::instance()->currentUser()->userId());

            groups->removeChannelFromGroup(mGroupGuid, mChannelId, originId, nucleusId);

            hideWindow();
        }

        void DeleteRoomViewController::onDestroyingMucRoom(const QString &channelId, Origin::Chat::DestroyMucRoomOperation *op)
        {
            if (channelId != mChannelId)
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

        void DeleteRoomViewController::onRoomEnterFailed()
        {
            ClientViewController* clientViewController = ClientFlow::instance()->view();
            if (clientViewController)
            {
                clientViewController->showEnterRoomPasswordDialog(mGroupGuid, mChannelId, true);
            }
            close();
        }

        void DeleteRoomViewController::onRoomDeleted()
        {
            emit roomDeleted(mChannelId);
            close();
        }
    }
}
