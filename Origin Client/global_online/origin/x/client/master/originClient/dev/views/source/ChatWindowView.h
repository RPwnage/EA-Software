/////////////////////////////////////////////////////////////////////////////
// ChatWindowView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CHATWINDOWVIEW_H
#define CHATWINDOWVIEW_H


#include "WebWidget/WidgetView.h"
#include "engine/social/Conversation.h"
#include "services/plugin/PluginAPI.h"

#include "chat/Message.h"
#include "engine/social/Conversation.h"
#include "UIScope.h"
#include <QtCore/QScopedPointer>

namespace Ui
{
    class ChatWindowView;
}

namespace Origin
{
    namespace Chat
    {
        class Conversable;
    }

    namespace Engine
    {
        namespace Social
        {
            class Conversation;
        }
    }

    namespace Client
    {
        class ChatWindowView;

        class ORIGIN_PLUGIN_API ChatWindowJsInterface : public QObject
        {
            Q_OBJECT
        public:
            ChatWindowJsInterface(ChatWindowView *chatWindow);

            void requestInsertConversation(const QString &);
            void requestRaiseConversation(const QString &);
            void requestCloseConversation(const QString &);
            void requestCloseActiveConversation();

            Q_INVOKABLE void notify(int notificationCount, const QString &id);

            /// \brief Bridges the scope of the associated chat window to JS - normal == 0, IGO == 1
            Q_INVOKABLE int scope() const;

            Q_INVOKABLE void onTabClosed(const QString& conversationId);

            void focusEvent();
            void blurEvent();

            void advanceActiveTabEvent();
            void rewindActiveTabEvent();

        signals:
            void insertConversation(const QString &);
            void raiseConversation(const QString &);
            void closeConversation(const QString &);
            void closeActiveConversation();
            void tabClosed(const QString &);

            void nativeFocus();
            void nativeBlur();

            void advanceActiveTab();
            void rewindActiveTab();

        private:
            ChatWindowView *mChatWindow;
        };

        class ORIGIN_PLUGIN_API ChatWindowView : public WebWidget::WidgetView
        {
            Q_OBJECT
        public:
            ChatWindowView(QWidget *parent, QSharedPointer<Engine::Social::Conversation>, UIScope scope);
            ~ChatWindowView();

            void insertConversation(QSharedPointer<Engine::Social::Conversation>);
            void raiseConversation(QSharedPointer<Engine::Social::Conversation>);
            void closeConversation(QSharedPointer<Engine::Social::Conversation>);
            void closeConversation(const QString& groupGuid);
            void closeActiveConversation();

            virtual bool event(QEvent *);

            UIScope scope() const { return mScope; }

        signals:
            /// \brief Emitted when the cursor of the web view changes. Used in OIG.
            void cursorChanged(int);
            void tabClosed(QSharedPointer<Engine::Social::Conversation>);

        private slots:
            void onTabClosed(const QString &);

        private:
            QScopedPointer<Ui::ChatWindowView> ui;
            QScopedPointer<ChatWindowJsInterface> mJsInterface;
            UIScope mScope;
        };
        
    }
}

#endif
