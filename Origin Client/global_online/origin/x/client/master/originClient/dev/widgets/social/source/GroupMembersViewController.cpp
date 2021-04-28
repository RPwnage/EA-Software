/////////////////////////////////////////////////////////////////////////////
// GroupMembersWindowViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "GroupMembersViewController.h"
#include "GroupMembersWindowView.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "ClientFlow.h"

#include <QDesktopWidget>

#include "originwindow.h"
#include "originwindowmanager.h"

#include <QVBoxLayout>
#include <QSizePolicy>

#include "OriginSocialProxy.h"
#include "OriginChatProxy.h"
#include "ChatGroupProxy.h"
#include "SocialUserProxy.h"
#include "RemoteUserProxy.h"

#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>

namespace Origin
{
    using namespace UIToolkit;
    using namespace Engine;

    namespace Client
    {
        GroupMembersViewController::GroupMembersViewController(const QString& groupGuid, QWidget *parent, UIScope scope)
            : QObject(parent)
            , mGroupMembersWindowView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mWidgetParent(parent)
            , mGroupGuid(groupGuid)
        {
            mChatGroup = NULL;
#if 0 //disable for Origin X
            JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
            if (social)
            {
                JsInterface::ChatGroupProxy* proxy = social->groupProxyByGroupId(mGroupGuid);
                if (proxy)
                    mChatGroup = proxy->proxied();
            }
#endif
        }

        GroupMembersViewController::~GroupMembersViewController()
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

        UIToolkit::OriginWindow* GroupMembersViewController::createWindow(const UIScope& scope)
        {
            // Create the view
            mGroupMembersWindowView = new GroupMembersWindowView(mWidgetParent, scope, mGroupGuid);
            mGroupMembersWindowView->setFixedWidth(450);
            mGroupMembersWindowView->setMaximumHeight(471);
            QPalette palette = mGroupMembersWindowView->palette();
            palette.setBrush(QPalette::Base, Qt::transparent);
            mGroupMembersWindowView->setPalette(palette);
            mGroupMembersWindowView->setAttribute(Qt::WA_OpaquePaintEvent, false);

            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            OriginWindow* myWindow = new OriginWindow(titlebarItems,
                mGroupMembersWindowView,
                OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, 
                windowContext);

            myWindow->setObjectName((scope == IGOScope) ? "IGO Group Members Window" : "Group Members Window");
            myWindow->setIgnoreEscPress(true);
            myWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            myWindow->setContentMargin(0);
            myWindow->setSizeGripEnabled(false); 
            myWindow->setTitleBarText(tr("ebisu_client_group_members"));

            //In IGO, we need to ignore input until the page is fully loaded because we crash in the qt widget code if we try to process input events and the widgets are not fully initialized
            if (scope == IGOScope)
            {
                mGroupMembersWindowView->setAttribute(Qt::WA_InputMethodEnabled, false);
                ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView->page(), SIGNAL(loadFinished(bool)),this, SLOT(onLoadFinished(bool)));
            }

            ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView, SIGNAL(promoteToAdmin(QObject*, const QString& )), this, SLOT(onPromoteToAdmin(QObject*, const QString& )));
            ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView, SIGNAL(demoteToMember(QObject*, const QString& )), this, SLOT(onDemoteToMember(QObject*, const QString& )));
            ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView, SIGNAL(transferOwner(QObject*, const QString& )), this, SLOT(onTransferOwner(QObject*, const QString& )));

            ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView, SIGNAL(queryGroupMembers(const QString& )), this, SLOT(onQueryGroupMembers()));    

            // Close our view when the webview window is closed
            ORIGIN_VERIFY_CONNECT(mGroupMembersWindowView->page(), SIGNAL(windowCloseRequested()),this, SLOT(onWindowClosed()));


            return myWindow;
        }

        void GroupMembersViewController::show(UIScope scope)
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
                mGroupMembersWindowView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, 0);
                emit(mGroupMembersWindowView->refresh());

#ifdef ORIGIN_MAC
                // Need to delay showing the window to avoid small flickering and jumping bug
                // This fixes EBIBUGS-23040
                QTimer::singleShot(1, mWindow, SLOT(showWindow()));
#else
                mWindow->showWindow();
#endif

            }
            else if (scope == IGOScope && Engine::IGOController::instance() != NULL)
            {
                if (mIGOWindow == NULL)
                {
                    mIGOWindow = createWindow(scope);
                    Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
                    behavior.supportsContextMenu = true;
                    Engine::IGOController::instance()->igowm()->addPopupWindow(mIGOWindow, Engine::IIGOWindowManager::WindowProperties(), behavior);
                    ORIGIN_VERIFY_CONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                    mGroupMembersWindowView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, 0);
                    emit(mGroupMembersWindowView->refresh());
                }
            }
        }

        void GroupMembersViewController::close()
        {
            onWindowClosed();
        }

        void GroupMembersViewController::onWindowClosed()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void GroupMembersViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void GroupMembersViewController::closeWindow()
        {
            if (mWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow->deleteLater();
                mWindow = NULL;
            }
        }

        void GroupMembersViewController::onPromoteToAdmin(QObject* user, const QString& groupGuid)
        {
            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ORIGIN_VERIFY_CONNECT(remoteUser, SIGNAL(promoteToAdminSuccess(QObject*)), this, SLOT(onPromoteToAdminFinished(QObject*)));
            emit(remoteUser->promoteToAdmin(groupGuid));
        }

        void GroupMembersViewController::onDemoteToMember(QObject* user, const QString& groupGuid)
        {
            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ORIGIN_VERIFY_CONNECT(remoteUser, SIGNAL(demoteToMemberSuccess(QObject*)), this, SLOT(onDemoteToMemberFinished(QObject*)));
            emit(remoteUser->demoteToMember(groupGuid));
        }

        void GroupMembersViewController::onPromoteToAdminFinished(QObject* user)
        {
            // Refresh group member list
            mGroupMembersWindowView->refresh();

            // Show success dialog
            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ClientFlow::instance()->showPromoteToAdminSuccessDialog(mChatGroup->getName(), remoteUser->proxied()->originId(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));
        
            ORIGIN_VERIFY_DISCONNECT(remoteUser, SIGNAL(promoteToAdminSuccess(QObject*)), this, SLOT(onPromoteToAdminFinished(QObject*)));
        }

        void GroupMembersViewController::onDemoteToMemberFinished(QObject* user)
        {
            // Refresh group member list
            mGroupMembersWindowView->refresh();

            // Show success dialog
            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ClientFlow::instance()->showDemoteToMemberSuccessDialog(mChatGroup->getName(), remoteUser->proxied()->originId(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));

            ORIGIN_VERIFY_DISCONNECT(remoteUser, SIGNAL(demoteToMemberSuccess(QObject*)), this, SLOT(onDemoteToMemberFinished(QObject*)));
        }

        void GroupMembersViewController::onActivateWindow()
        {
            mWindow->showWindow();
        }

        void GroupMembersViewController::onLoadFinished(bool /*ok*/)
        {
            //we don't care whether it loaded successfully or not, as long as its not in the middle of a load
            mGroupMembersWindowView->setAttribute(Qt::WA_InputMethodEnabled, true);
        }

        void GroupMembersViewController::onZoomed()
        {
            mWindow->setGeometry(QDesktopWidget().availableGeometry(mWindow));
        }

        void GroupMembersViewController::onQueryGroupMembers()
        {
            mChatGroup->queryGroupMembers();
        }

        void GroupMembersViewController::onTransferOwner(QObject* user, const QString& groupGuid)
        {
            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ORIGIN_VERIFY_CONNECT(remoteUser, SIGNAL(transferOwnershipSuccess(QObject*)), this, SLOT(onTransferOwnerFinished(QObject*)));
            remoteUser->transferGroupOwnership(groupGuid);
        }

        void GroupMembersViewController::onTransferOwnerFinished(QObject* user)
        {
            // Refresh group member list
            mGroupMembersWindowView->refresh();

            JsInterface::RemoteUserProxy* remoteUser = dynamic_cast<JsInterface::RemoteUserProxy*>(user);
            ORIGIN_VERIFY_DISCONNECT(remoteUser, SIGNAL(transferOwnershipSuccess(QObject*)), this, SLOT(onTransferOwnerFinished(QObject*)));
        }

        void GroupMembersViewController::onUpdateOwnerFinished()
        {
            Origin::Services::GroupUserUpdateRoleResponse* resp = dynamic_cast<Origin::Services::GroupUserUpdateRoleResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                if (!resp->failed()) 
                {
                    // Refresh the group member list
                    mGroupMembersWindowView->refresh();
                }
                // else error dialog here ?
            }
        }
    }
}
