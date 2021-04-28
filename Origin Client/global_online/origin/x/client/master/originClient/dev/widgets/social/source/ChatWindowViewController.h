/////////////////////////////////////////////////////////////////////////////
// ChatWindowViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CHATWINDOWVIEWCONTROLLER_H
#define CHATWINDOWVIEWCONTROLLER_H

#include <QObject>

#include "engine/social/Conversation.h"
#include "services/plugin/PluginAPI.h"
#include "chat/Message.h"
#include "UIScope.h"
#include "engine/igo/IGOController.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
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

        class ORIGIN_PLUGIN_API ChatWindowViewController : public QObject, public Origin::Engine::IIGOWindowManager::IScreenListener
        {
            Q_OBJECT
        public:

            ChatWindowViewController(QWidget *parent, QSharedPointer<Engine::Social::Conversation>, UIScope);
            ~ChatWindowViewController();
            
            // IScreenListener impl
            ChatWindowView* view() { return mChatWindowView; }
            UIToolkit::OriginWindow* window() { return mWindow; }

            void insertConversation(QSharedPointer<Engine::Social::Conversation>);
            void raiseConversation(QSharedPointer<Engine::Social::Conversation>);
            void closeConversation(QSharedPointer<Engine::Social::Conversation>);
            void closeConversation(const QString& groupGuid);

            QPoint defaultPosition() const { return QPoint(50, 50); }

        signals:
            void windowClosed(ChatWindowViewController*);
            void tabClosed(QSharedPointer<Engine::Social::Conversation>);

        public slots:
            virtual void onSizeChanged(uint32_t width, uint32_t height); // IScreenListener impl

        protected slots:
            void onLoadFinished(bool ok);
            void onWindowClosed();
            void onTitleChanged(const QString& title);
            void onActivateWindow();

        private slots:
            void onZoomed();

        private:
            QSize mPrevSizeChange;
            ChatWindowView *mChatWindowView;
            UIToolkit::OriginWindow* mWindow;

            QWidget* mWidgetParent;
        };
    }
} 

#endif //CHATWINDOWVIEWCONTROLLER_H
