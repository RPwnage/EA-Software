/////////////////////////////////////////////////////////////////////////////
// GroupInviteViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "GroupInviteViewController.h"
#include "InviteFriendsWindowView.h"
#include "services/debug/DebugService.h"
#include "services/rest/GroupServiceClient.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "chat/OriginConnection.h"
#include "engine/social/SocialController.h"

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
#include "ClientFlow.h"
#include "SocialViewController.h"

#include "flows/SummonToGroupChannelFlow.h"

#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>

namespace Origin
{
    using namespace UIToolkit;

    namespace Client
    {
        GroupInviteViewController::GroupInviteViewController(const QString& groupGuid, const QString& channelId, const QString& conversationId, QWidget *parent, UIScope scope)
            : QObject(parent)
            , mInviteFriendsWindowView(NULL)
            , mWindow(NULL)
            , mIGOWindow(NULL)
            , mWidgetParent(parent)
            , mGroupGuid(groupGuid)
            , mChannelId(channelId)
            , mConversationId(conversationId)
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

        GroupInviteViewController::~GroupInviteViewController()
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

        UIToolkit::OriginWindow* GroupInviteViewController::createWindow(const UIScope& scope)
        {
            mInviteFriendsWindowView = new InviteFriendsWindowView(mWidgetParent, scope, mGroupGuid, mChannelId, mConversationId);
            mInviteFriendsWindowView->setDialogType( (mChannelId.size() > 0 ? InviteFriendsWindowJsInterface::Room : InviteFriendsWindowJsInterface::Group) );
            mInviteFriendsWindowView->setFixedWidth(450);
            mInviteFriendsWindowView->setMaximumHeight(471);
            QPalette palette = mInviteFriendsWindowView->palette();
            palette.setBrush(QPalette::Base, Qt::transparent);
            mInviteFriendsWindowView->setPalette(palette);
            mInviteFriendsWindowView->setAttribute(Qt::WA_OpaquePaintEvent, false);

            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
            OriginWindow::WindowContext windowContext = OriginWindow::OIG;
            if(scope != IGOScope)
            {
                windowContext = OriginWindow::Normal;
                titlebarItems |= OriginWindow::Minimize;
            }

            OriginWindow* myWindow = new OriginWindow(titlebarItems,
                mInviteFriendsWindowView,
                OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, 
                windowContext);

            myWindow->setObjectName((scope == IGOScope) ? "IGO Group Invite Window" : "Group Invite Window");
            myWindow->setIgnoreEscPress(true);
            myWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            myWindow->setContentMargin(0);
            myWindow->setSizeGripEnabled(false); 
            myWindow->setTitleBarText(tr("ebisu_client_group_invite_friends"));

            ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView, SIGNAL(inviteUsersToRoom(const QObjectList& , const QString&, const QString& )), this, SLOT(onInviteUsersToRoom(const QObjectList& , const QString&, const QString& )));    

            ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView, SIGNAL(inviteUsersToGroup(const QObjectList& , const QString& )), this, SLOT(onInviteUsersToGroup(const QObjectList& , const QString& )));    

            ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView, SIGNAL(queryGroupMembers(const QString&)), this, SLOT(onQueryGroupMembers()));    
            
            // Close our view when the webview window is closed
            ORIGIN_VERIFY_CONNECT(mInviteFriendsWindowView->page(), SIGNAL(windowCloseRequested()),this, SLOT(onWindowClosed()));

            // Close our window if the user leaves the chatroom
            Engine::Social::SocialController* socialController = Engine::LoginController::instance()->currentUser()->socialControllerInstance();
            Chat::OriginConnection* chatConnection = socialController->chatConnection();
            Chat::ConnectedUser* currentUser = chatConnection->currentUser();
            Chat::ChatGroups* groups = currentUser->chatGroups();
            Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupGuid);
            if (group)
            {
                Chat::ChatChannel* channel = group->channelForChannelId(mChannelId);
                if (channel)
                {
                    ORIGIN_VERIFY_CONNECT(channel, SIGNAL(leftRoom()),this, SLOT(onWindowClosed()));
                }
            }

            return myWindow;
        }

        void GroupInviteViewController::show(UIScope scope)
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
                mInviteFriendsWindowView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, 0);
                emit(mInviteFriendsWindowView->refresh());

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
                    mInviteFriendsWindowView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, 0);
                    emit(mInviteFriendsWindowView->refresh());
                }
            }
        }

        void GroupInviteViewController::close()
        {
            onWindowClosed();
        }

        void GroupInviteViewController::onWindowClosed()
        {
            if (mIGOWindow)
                closeIGOWindow();
            else
                closeWindow();
        }

        void GroupInviteViewController::closeIGOWindow()
        {
            if (mIGOWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mIGOWindow, SIGNAL(rejected()), this, SLOT(closeIGOWindow()));
                mIGOWindow->close();
                mIGOWindow->deleteLater();
                mIGOWindow = NULL;
            }
        }

        void GroupInviteViewController::closeWindow()
        {
            if (mWindow != NULL)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow->deleteLater();
                mWindow = NULL;
            }
        }

        void GroupInviteViewController::onInviteUsersToGroup(const QObjectList& users, const QString& groupGuid)
        {
#if 0 //disable for Origin X
            JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
            if (!social)
                return;

            JsInterface::ChatGroupProxy* proxy = social->groupProxyByGroupId(mGroupGuid);
            ORIGIN_VERIFY_CONNECT(proxy->proxied(), SIGNAL(usersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)), this, SLOT(onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)));

            QList<Origin::Chat::RemoteUser*> remoteUsers;
            for(QObjectList::const_iterator it = users.begin(); it != users.end(); it++) 
            {                
                JsInterface::RemoteUserProxy* pr = dynamic_cast<JsInterface::RemoteUserProxy*>(*it);
                if (pr) 
                {
                    remoteUsers.append(pr->proxied());
                }
            }
            proxy->proxied()->inviteUsersToGroup(remoteUsers);
            close();
#endif
        }

        void GroupInviteViewController::onInviteUsersToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId)
        {
            ClientFlow::instance()->socialViewController()->onInviteToRoom(users, groupGuid, channelId);
            close();
        }

        void GroupInviteViewController::onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*> users)
        {
#if 0 //disable for Origin X
            JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
            if (!social)
                return;

            JsInterface::ChatGroupProxy* proxy = social->groupProxyByGroupId(mGroupGuid);
            ORIGIN_VERIFY_DISCONNECT(proxy->proxied(), SIGNAL(usersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)), this, SLOT(onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)));

            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;

            ClientFlow::instance()->showGroupInviteSentDialog(users, scope);
#endif
        }

        void GroupInviteViewController::onActivateWindow()
        {
            mWindow->showWindow();
        }

        void GroupInviteViewController::onLoadFinished(bool /*ok*/)
        {
            //we don't care whether it loaded successfully or not, as long as its not in the middle of a load
            mInviteFriendsWindowView->setAttribute(Qt::WA_InputMethodEnabled, true);
        }

        void GroupInviteViewController::onTitleChanged(const QString& title)
        {
            mWindow->setTitleBarText(title);
        }

        void GroupInviteViewController::onZoomed()
        {
            mWindow->setGeometry(QDesktopWidget().availableGeometry(mWindow));
        }

        void GroupInviteViewController::onQueryGroupMembers()
        {
            mChatGroup->queryGroupMembers();            
        }
    }
}
