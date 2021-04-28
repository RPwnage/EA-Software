/////////////////////////////////////////////////////////////////////////////
// ChatWindowView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ChatWindowView.h"
#include "ui_ChatWindowView.h"

#include <QApplication>
#include <QKeyEvent>

#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/platform/PlatformService.h"

#include "chat/Conversable.h"

#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidgetController.h"
#include "NativeBehaviorManager.h"
#include "OriginChatProxy.h"
#include "engine/social/Conversation.h"
#include "engine/igo/IGOController.h"
#ifdef ORIGIN_MAC
#include "OriginApplicationDelegateOSX.h"
#endif
#include "ClientFlow.h"
#include "SocialViewController.h"
#include "ConversationProxy.h"

using namespace WebWidget;

namespace Origin
{
    namespace Client
    {
        using JsInterface::OriginChatProxy;

        ChatWindowView::ChatWindowView(QWidget* parent, QSharedPointer<Engine::Social::Conversation> conv, UIScope scope)
            : WebWidget::WidgetView(parent)
            ,ui(new Ui::ChatWindowView())
            ,mJsInterface(new ChatWindowJsInterface(this))
            ,mScope(scope)
        {
			ui->setupUi(this);

            // Set up the initial chat window
            {
                WebWidget::UriTemplate::VariableMap variables;
                variables["initialConversations"] = OriginChatProxy::instance()->idForConversation(conv);

                // Allow external network access through our network access manager
                widgetPage()->setExternalNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

                // Add a JavaScript object that we can use to talk to the HTML chat window
                NativeInterface nativeChatWindow("nativeChatWindow", mJsInterface.data());
                widgetPage()->addPageSpecificInterface(nativeChatWindow);

                const WebWidget::WidgetLink link("http://widgets.dm.origin.com/linkroles/chatwindow", variables);
                WebWidgetController::instance()->loadSharedWidget(this->widgetPage(), "chat", link);

                QObject::installEventFilter(this);
            }

            // Allow access to the clipboard
            widgetPage()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);

            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(this);

            setProperty("origin-allow-drag", true);

            // Make this connection so we can keep the Desktop and OIG chat windows in sync
            ORIGIN_VERIFY_CONNECT(mJsInterface.data(), SIGNAL(tabClosed(const QString&)), this, SLOT(onTabClosed(const QString&)));
        }

        ChatWindowView::~ChatWindowView()
        {
            //added this here to possibly address the crash where focus was trying to be set on not fully initalized widgets
            clearFocus();
        }

        bool ChatWindowView::event(QEvent* event)
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
                case QEvent::KeyPress:
                {
                    // Allow use of CTRL-TAB to switch chat tabs
                    QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
#ifdef ORIGIN_MAC
                    if (keyEvent && (keyEvent->modifiers() & Qt::MetaModifier)) // GRUMBLE GRUMBLE QT
#else
                    if (keyEvent && (keyEvent->modifiers() & Qt::ControlModifier))
#endif
                    {
                        if (keyEvent->key() == Qt::Key_Tab)
                        {
                            mJsInterface->advanceActiveTabEvent();
                        }
                        else if (keyEvent->key() == Qt::Key_Backtab)
                        {
                            mJsInterface->rewindActiveTabEvent();
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            return WebWidget::WidgetView::event(event);
        }
  
        void ChatWindowView::insertConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            // Open a new tab but don't switch to it
            mJsInterface->requestInsertConversation(OriginChatProxy::instance()->idForConversation(conv));
        }

        void ChatWindowView::raiseConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            // Open a new tab and switch to it
            mJsInterface->requestRaiseConversation(OriginChatProxy::instance()->idForConversation(conv));
        }

        void ChatWindowView::closeConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            // Close this conversation
            mJsInterface->requestCloseConversation(OriginChatProxy::instance()->idForConversation(conv));
        }

        void ChatWindowView::closeConversation(const QString& groupGuid)
        {
            mJsInterface->requestCloseConversation(OriginChatProxy::instance()->conversationIdForGroupGuid(groupGuid));
        }

        void ChatWindowView::closeActiveConversation()
        {
            mJsInterface->requestCloseActiveConversation();
        }

        ChatWindowJsInterface::ChatWindowJsInterface(ChatWindowView *chatWindow) : 
            QObject(chatWindow),
            mChatWindow(chatWindow)
        {
        }

        void ChatWindowJsInterface::requestInsertConversation(const QString &id)
        {
            emit insertConversation(id);
        }

        void ChatWindowJsInterface::requestRaiseConversation(const QString &id)
        {
            emit raiseConversation(id);
        }

        void ChatWindowJsInterface::requestCloseConversation(const QString &id)
        {
            emit closeConversation(id);
        }

        void ChatWindowJsInterface::requestCloseActiveConversation()
        {
            emit closeActiveConversation();
        }

        void ChatWindowView::onTabClosed(const QString &conversationId)
        {
            // Grab out conversation object and let everyone know it's been closed
            using namespace Origin::Client::JsInterface;
            QVariant var = OriginChatProxy::instance()->getConversationById(conversationId);
            ConversationProxy* proxy = var.value<ConversationProxy*>();
            if (proxy)
            {
                QSharedPointer<Engine::Social::Conversation> conv = proxy->proxied();
                if (conv)
                {
                    emit tabClosed(conv);
                }
            }
        }

        void ChatWindowJsInterface::notify(int notificationCount, const QString &id)
        {
            if (mChatWindow->scope() != ClientScope) return; // Ignore notifications from things like IGO

            if(!id.isEmpty())
            {
                Origin::Client::ClientFlow* clientFlow = Origin::Client::ClientFlow::instance();
                if(clientFlow)
                {
                    Origin::Client::SocialViewController* controller = clientFlow->socialViewController();

                    if (controller)
                    {
                        controller->setLastNotifiedConversationId(id);
                    }
                }
            }
#ifdef ORIGIN_MAC
            // Send total number of notifications for display on badge icon
            Origin::Client::setDockTile(notificationCount);
#endif
            if (!id.isEmpty())
            {
#if defined(ORIGIN_MAC)
                QApplication::alert(mChatWindow->topLevelWidget(), 1500);
#elif defined(ORIGIN_PC)
                // On Windows: Two flashes then stay lit on third flash (if not in focus)
                QWidget* activeWindow = QApplication::activeWindow();
                QWidget* chatWindow = mChatWindow->topLevelWidget();
                // EBIBUGS-28159 TODO: In the future use QApplication::alert. It doesn't work as explained in their documentation.
                // I submitted a bug to them. https://bugreports.qt-project.org/browse/QTBUG-41744
                // For now just constantly blink.
                if (chatWindow != activeWindow)
                    Services::PlatformService::flashTaskbarIcon(chatWindow);
#endif
            }
        }

        int ChatWindowJsInterface::scope() const
        {
            return (mChatWindow) ? mChatWindow->scope() : 0;
        }

        void ChatWindowJsInterface::onTabClosed(const QString& conversationId)
        {
            emit tabClosed(conversationId);
        }
       
        void ChatWindowJsInterface::focusEvent()
        {
            emit(nativeFocus());
        }
        
        void ChatWindowJsInterface::blurEvent()
        {
            emit(nativeBlur());
        }

        void ChatWindowJsInterface::advanceActiveTabEvent()
        {
            emit(advanceActiveTab());
        }

        void ChatWindowJsInterface::rewindActiveTabEvent()
        {
            emit(rewindActiveTab());
        }

    }
}
