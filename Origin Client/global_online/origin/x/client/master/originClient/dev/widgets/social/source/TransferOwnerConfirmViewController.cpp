/////////////////////////////////////////////////////////////////////////////
// TransferOwnerConfirmViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "TransferOwnerConfirmViewController.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "OriginSocialProxy.h"
#include "engine/igo/IGOController.h"
#include "ClientFlow.h"
#include "chat/Connection.h"
#include "originpushbutton.h"
#include "originwindow.h"
#include "chat/RemoteUser.h"

namespace Origin
{
    namespace Client
    {

        TransferOwnerConfirmViewController::TransferOwnerConfirmViewController(const QString& groupGuid, Chat::RemoteUser* user, const QString& userNickname, QObject* parent)
            : QObject(parent)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mGroupGuid(groupGuid)
            , mUserNickname(userNickname)
            , mUser(user)
        {            
            ORIGIN_VERIFY_CONNECT(user->connection(), SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)), this, SLOT(close()));
        }

        TransferOwnerConfirmViewController::~TransferOwnerConfirmViewController()
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

        UIToolkit::OriginWindow* TransferOwnerConfirmViewController::createWindow(const UIScope& scope)
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
            myWindow->setTitleBarText(tr("ebisu_client_transfer_ownership"));
            myWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_transfer_ownership"));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onTransferOwner()));
            ORIGIN_VERIFY_CONNECT(myWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

            return myWindow;
        }

        void TransferOwnerConfirmViewController::setUser(Chat::RemoteUser* user, const QString& userNickname)
        {
            using namespace UIToolkit;

            mUserNickname = userNickname;
            mUser = user;

            if (mWindow)
            {
                mWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_give_ownership_of_your_group_to_X_title").arg(mUserNickname.toUpper()), 
                    tr("ebisu_client_once_you_transfer_ownership_you_will_no_longer_be_owner"));
            }

            if (mIGOWindow)
            {
                mIGOWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_give_ownership_of_your_group_to_X_title").arg(mUserNickname.toUpper()), 
                    tr("ebisu_client_once_you_transfer_ownership_you_will_no_longer_be_owner"));
            }
        }

        void TransferOwnerConfirmViewController::show(const UIScope& scope)
        {
            using namespace UIToolkit;
            if (scope == ClientScope)
            {
                if(mWindow == NULL)
                {
                    mWindow = createWindow(scope);
                    mWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_give_ownership_of_your_group_to_X_title").arg(mUserNickname.toUpper()), 
                        tr("ebisu_client_once_you_transfer_ownership_you_will_no_longer_be_owner"));
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
                    mIGOWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_are_about_to_give_ownership_of_your_group_to_X_title").arg(mUserNickname.toUpper()), 
                        tr("ebisu_client_once_you_transfer_ownership_you_will_no_longer_be_owner"));
                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, Engine::IIGOWindowManager::WindowProperties());
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                }
            }
        }

        void TransferOwnerConfirmViewController::close()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void TransferOwnerConfirmViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void TransferOwnerConfirmViewController::closeWindow()
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

        void TransferOwnerConfirmViewController::onTransferOwner()
        {
            mUser->transferOwnership(mGroupGuid);
            close();
        }
    }
}
