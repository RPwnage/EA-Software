/////////////////////////////////////////////////////////////////////////////
// ChatWindowViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ChatWindowViewController.h"
#include "ChatWindowView.h"
#include "services/debug/DebugService.h"

#include <QDesktopWidget>

#include "originwindow.h"
#include "originwindowmanager.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/Conversation.h"
#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"
#include "chat/Roster.h"

#include <QVBoxLayout>

namespace Origin
{
    using namespace UIToolkit;

    namespace Client
    {

        ChatWindowViewController::ChatWindowViewController(QWidget *parent, QSharedPointer<Engine::Social::Conversation> conv, UIScope scope)
            : QObject(parent)
            , mChatWindowView(NULL)
            , mWidgetParent(parent)
        {
            // Create the view
            mChatWindowView = new ChatWindowView(mWidgetParent, conv, scope);

            // Get the title of the webview to set as the window titlebar text
            ORIGIN_VERIFY_CONNECT(mChatWindowView, SIGNAL(titleChanged(const QString&)), this, SLOT(onTitleChanged(const QString&)));

            // Close our view when the webview window is closed
            ORIGIN_VERIFY_CONNECT(mChatWindowView->page(), SIGNAL(windowCloseRequested()),this, SLOT(onWindowClosed()));

            //In IGO, we need to ignore input until the page is fully loaded because we crash in the qt widget code if we try to process input events and the widgets are not fully initialized
            if (scope == IGOScope)
            {
                mChatWindowView->setAttribute(Qt::WA_InputMethodEnabled, false);
                ORIGIN_VERIFY_CONNECT(mChatWindowView->page(), SIGNAL(loadFinished(bool)),this, SLOT(onLoadFinished(bool)));
            }

            // Tell everyone the convesation tab has been closed so we can keep the two chat windows Desktop and OIG in sync
            ORIGIN_VERIFY_CONNECT(mChatWindowView, SIGNAL(tabClosed(QSharedPointer<Engine::Social::Conversation>)), this, SIGNAL(tabClosed(QSharedPointer<Engine::Social::Conversation>)));

            // Create the enclosing window frame
            OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;

            if (scope == ClientScope)
            {
                titlebarItems |= OriginWindow::Minimize | OriginWindow::Maximize;
            }

            mWindow = new OriginWindow(titlebarItems,
                mChatWindowView,
                OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, 
                scope == IGOScope ? OriginWindow::OIG : OriginWindow::Normal);

            mWindow->setObjectName((scope == IGOScope) ? "IGO Chat Window" : "Chat Window");
            mWindow->setIgnoreEscPress(true);

            ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onWindowClosed()));

            if (mWindow->manager())
            {
                ORIGIN_VERIFY_CONNECT(mWindow->manager(), SIGNAL(customizeZoomedSize()), this, SLOT(onZoomed()));
            }

            mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            mWindow->setContentMargin(0);
            mWindow->setMinimumWidth(400);
            mWindow->setMinimumHeight(400);
#ifdef ORIGIN_MAC
            // Need to delay showing the window to avoid small flickering and jumping bug
            // This fixes EBIBUGS-23040
            QTimer::singleShot(1, mWindow, SLOT(show()));
#else
            mWindow->show();
#endif
        }
        
        void ChatWindowViewController::onActivateWindow()
        {
            mWindow->showWindow();
        }

        ChatWindowViewController::~ChatWindowViewController()
        {
            if (mWindow)
            {
                mWindow->deleteLater();
                mWindow = NULL;
            }

        }

        void ChatWindowViewController::insertConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            mChatWindowView->insertConversation(conv);
        }

        void ChatWindowViewController::raiseConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            mChatWindowView->raiseConversation(conv);
        }

        void ChatWindowViewController::closeConversation(QSharedPointer<Engine::Social::Conversation> conv)
        {
            mChatWindowView->closeConversation(conv);
        }

        void ChatWindowViewController::closeConversation(const QString& groupGuid)
        {
            mChatWindowView->closeConversation(groupGuid);
        }

        void ChatWindowViewController::onWindowClosed()
        {
            if (mWindow)
            {
                // Do not delete mWindow here the ChatWindowManger will handle this
                mWindow->close();
                emit windowClosed(this);
            }
        }

        void ChatWindowViewController::onLoadFinished(bool /*ok*/)
        {
            //we don't care whether it loaded successfully or not, as long as its not in the middle of a load
            mChatWindowView->setAttribute(Qt::WA_InputMethodEnabled, true);
        }

        void ChatWindowViewController::onTitleChanged(const QString& title)
        {
            mWindow->setTitleBarText(title);
        }

        void ChatWindowViewController::onZoomed()
        {
            mWindow->setGeometry(QDesktopWidget().availableGeometry(mWindow));
        }
        
        // Called from IGO when the game resolution changes
        void ChatWindowViewController::onSizeChanged(uint32_t width, uint32_t height)
        {
            // The Chat window may stay alive between games -> we don't want to override the current position is not necessary
            if (width != mPrevSizeChange.width() || height != mPrevSizeChange.height())
            {
                mPrevSizeChange = QSize(width, height);
                Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(mWindow, defaultPosition());
            }
        }
    }
}
