/////////////////////////////////////////////////////////////////////////////
// GroupMembersWindowView.cpp
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "GroupMembersWindowView.h"
#include "ui_GroupMembersWindowView.h"

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
        GroupMembersWindowView::GroupMembersWindowView(QWidget* parent, UIScope scope, const QString& groupGuid)
            : WebWidget::WidgetView(parent)
            ,ui(new Ui::GroupMembersWindowView())
            ,mJsInterface(new GroupMembersWindowJsInterface(this))
            ,mScope(scope)
        {
			ui->setupUi(this);

            // Set up the initial group members window
            {
                WebWidget::UriTemplate::VariableMap variables;
                variables["groupGuid"] = groupGuid;

                // Allow external network access through our network access manager
                widgetPage()->setExternalNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

                // Add a JavaScript object that we can use to talk to the HTML user admin window
                NativeInterface nativeAdminWindow("groupMembersWindow", mJsInterface);
                widgetPage()->addPageSpecificInterface(nativeAdminWindow);

                const WebWidget::WidgetLink link("http://widgets.dm.origin.com/linkroles/groupmemberswindow", variables);
                WebWidgetController::instance()->loadSharedWidget(this->widgetPage(), "chat", link);

                QObject::installEventFilter(this);

                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(transferOwner(QObject*, const QString& )), this, SIGNAL(transferOwner(QObject*, const QString& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(promoteToAdmin(QObject*, const QString& )), this, SIGNAL(promoteToAdmin(QObject*, const QString& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(demoteToMember(QObject*, const QString& )), this, SIGNAL(demoteToMember(QObject*, const QString& )));
                ORIGIN_VERIFY_CONNECT(mJsInterface, SIGNAL(queryGroupMembers(const QString& )), this, SIGNAL(queryGroupMembers(const QString& )));

                ORIGIN_VERIFY_CONNECT(this, SIGNAL(refresh()), mJsInterface, SIGNAL(refresh()));
            }

            // Allow access to the clipboard
            widgetPage()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);

            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(this);

            setProperty("origin-allow-drag", true);
        }

        GroupMembersWindowView::~GroupMembersWindowView()
        {
            //added this here to possibly address the crash where focus was trying to be set on not fully initalized widgets
            clearFocus();
        }

        bool GroupMembersWindowView::event(QEvent* event)
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
  
        GroupMembersWindowJsInterface::GroupMembersWindowJsInterface(GroupMembersWindowView *groupMembersWindow) : 
            QObject(groupMembersWindow),
            mWindow(groupMembersWindow)
        {
        }

        void GroupMembersWindowJsInterface::transferGroupOwner(QObject* user, const QString& groupGuid)
        {
            emit(transferOwner(user, groupGuid));
        }

        void GroupMembersWindowJsInterface::promoteUserToAdmin(QObject* user, const QString& groupGuid)
        {
            emit(promoteToAdmin(user, groupGuid));
        }

        void GroupMembersWindowJsInterface::demoteAdminToMember(QObject* user, const QString& groupGuid)
        {
            emit(demoteToMember(user, groupGuid));
        }

        void GroupMembersWindowJsInterface::callQueryGroupMembers(const QString& groupGuid)
        {
            emit(queryGroupMembers(groupGuid));
        }

        void GroupMembersWindowJsInterface::focusEvent()
        {
            emit(nativeFocus());
        }

        void GroupMembersWindowJsInterface::blurEvent()
        {
            emit(nativeBlur());
        }

    }
}
