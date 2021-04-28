/////////////////////////////////////////////////////////////////////////////
// InviteFriendsWindowView.cpp
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "InviteFriendsWindowView.h"

#include <QApplication>

#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/rest/GroupServiceClient.h"

#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidgetController.h"
#include "NativeBehaviorManager.h"
#include "engine/igo/IGOController.h"
#ifdef ORIGIN_MAC
#include "OriginApplicationDelegateOSX.h"
#endif
#include "ClientFlow.h"
#include "SocialViewController.h"

using namespace WebWidget;

namespace Origin
{
    namespace Client
    {
        InviteFriendsWindowView::InviteFriendsWindowView(QWidget* parent, UIScope scope, const QString& groupGuid, const QString& channelId, const QString& conversationId)
            : WebWidget::WidgetView(parent)
            ,mJsInterface(new InviteFriendsWindowJsInterface(this))
            ,mScope(scope)
        {
            QPalette palette = this->palette();
            palette.setBrush(QPalette::Base, Qt::transparent);
            setPalette(palette);
            setAttribute(Qt::WA_OpaquePaintEvent, false);
            setMaximumWidth(450);
            setMaximumHeight(470);

            // Set up the initial user admin window
            {
                WebWidget::UriTemplate::VariableMap variables;
                variables["groupGuid"] = groupGuid;
                variables["channelId"] = channelId;
                variables["conversationId"] = conversationId;

                // Allow external network access through our network access manager
                widgetPage()->setExternalNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

                // Add a JavaScript object that we can use to talk to the HTML user admin window
                NativeInterface nativeAdminWindow("friendInviteWindow", mJsInterface);
                widgetPage()->addPageSpecificInterface(nativeAdminWindow);

                const WebWidget::WidgetLink link("http://widgets.dm.origin.com/linkroles/invitefriendswindow", variables);
                WebWidgetController::instance()->loadSharedWidget(this->widgetPage(), "chat", link);

                QObject::installEventFilter(this);

                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(inviteToGame(const QObjectList& )), this, SIGNAL(inviteUsersToGame(const QObjectList& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(inviteToGroup(const QObjectList&, const QString& )), this, SIGNAL(inviteUsersToGroup(const QObjectList& , const QString& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(inviteToRoom(const QObjectList&, const QString&, const QString& )), this, SIGNAL(inviteUsersToRoom(const QObjectList& , const QString&, const QString& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(queryGroupMembers(const QString& )), this, SIGNAL(queryGroupMembers(const QString& )));
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(refresh()), mJsInterface, SIGNAL(refresh()));
            }

            // Allow access to the clipboard
            widgetPage()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);

            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(this);

            setProperty("origin-allow-drag", true);
        }

        InviteFriendsWindowView::~InviteFriendsWindowView()
        {
            //added this here to possibly address the crash where focus was trying to be set on not fully initialized widgets
            clearFocus();
        }

        bool InviteFriendsWindowView::event(QEvent* event)
        {
            switch(event->type())
            {
#ifdef ORIGIN_MAC
                // We trap the Hide event because the Close event was never getting hit
                case QEvent::Hide:
                {
                    // Send total number of notifications for display on badge icon
                    Origin::Client::setDockTile(0);
                    break;
                }
#endif
                case QEvent::WindowActivate:
                {
                    mJsInterface->focusEvent();
                    break;
                }
                case QEvent::WindowDeactivate:
                {
                    mJsInterface->blurEvent();
                    break;
                }
                case QEvent::CursorChange:
                {
                    emit cursorChanged(cursor().shape());
                    break;
                }
                case QEvent::Leave:
                {
                    emit cursorChanged(Qt::ArrowCursor);
                    break;
                }
                default:
                    break;
            }
            return WebWidget::WidgetView::event(event);
        }

        InviteFriendsWindowJsInterface::InviteFriendsWindowJsInterface(InviteFriendsWindowView *groupInviteWindow) : 
            QObject(groupInviteWindow),
            mWindow(groupInviteWindow),
            mDialogType(Group)
        {
        }

        void InviteFriendsWindowJsInterface::inviteUsersToGame(const QObjectList& users)
        {
            emit(inviteToGame(users));
        }
        void InviteFriendsWindowJsInterface::inviteUsersToGroup(const QObjectList& users, const QString& groupGuid)
        {
            emit(inviteToGroup(users, groupGuid));
        }
        void InviteFriendsWindowJsInterface::inviteUsersToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId)
        {
            emit(inviteToRoom(users, groupGuid, channelId));
        }

        void InviteFriendsWindowJsInterface::callQueryGroupMembers(const QString& groupGuid)
        {
            emit(queryGroupMembers(groupGuid));
        }

        void InviteFriendsWindowJsInterface::focusEvent()
        {
            emit(nativeFocus());
        }

        void InviteFriendsWindowJsInterface::blurEvent()
        {
            emit(nativeBlur());
        }
    }
}
