/////////////////////////////////////////////////////////////////////////////
// ChatWindowManager.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CHATWINDOWMANAGER_H
#define CHATWINDOWMANAGER_H

#include <QObject>
#include "ChatWindowViewController.h"
#include "chat/Conversable.h"
#include "chat/Message.h"
#include "engine/social/Conversation.h"
#include "engine/social/ConversationManager.h"
#include "services/plugin/PluginAPI.h"
#include "UIScope.h"

#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "Qt/originpushbutton.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API ChatWindowManager : public QObject
        {
            Q_OBJECT

        public:
            ChatWindowManager(QWidget *parent);
            ~ChatWindowManager();

            int windowCount() const;
            int igoWindowCount() const;

            void raiseMainChatWindow();
            void hideMainChatWindow();

            bool isChatWindowActive() const;
            bool isIGOChatWindowActive() const;

            void startConversation(Chat::Conversable *conversable, Engine::Social::Conversation::CreationReason, Engine::IIGOCommandController::CallOrigin callOrigin);

        public slots:
        void startConversation(QSharedPointer<Origin::Engine::Social::Conversation> conv);
        void startConversation(QSharedPointer<Origin::Engine::Social::Conversation> conv, Engine::IIGOCommandController::CallOrigin callOrigin);
        void onActivateWindow();

        private slots:
            void onWindowClosed(ChatWindowViewController* window);
            void onIGOWindowClosed(ChatWindowViewController* window);
            void onTabClosed(QSharedPointer<Engine::Social::Conversation>);
            void setCursor(int cursorShape);
            void onGroupDeleted(const QString&);
        private:

            void initOfflineConversations();
            void saveViewSizeAndPosition(UIToolkit::OriginWindow*);
            void restoreViewSizeAndPosition(UIToolkit::OriginWindow*);

            ChatWindowViewController* newWindow(QSharedPointer<Engine::Social::Conversation> conv, UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);

            QWidget* mWidgetParent;

            QList<ChatWindowViewController*> mWindowList;
            QList<ChatWindowViewController*> mIGOWindowList;

            bool mIgnoreStartingConversation;
        };
    }
}

#endif //FRIENDS_LIST_VIEW_CONTROLLER_H
