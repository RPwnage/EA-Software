/////////////////////////////////////////////////////////////////////////////
// GroupMembersWindowView.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef GroupMembersWindowView_H
#define GroupMembersWindowView_H


#include "WebWidget/WidgetView.h"
#include "UIScope.h"

namespace Ui
{
    class GroupMembersWindowView;
}

namespace Origin
{
    namespace Engine
    {
        namespace Social
        {
            class Conversation;
        }
    }

    namespace Client
    {
        class GroupMembersWindowView;

        class GroupMembersWindowJsInterface : public QObject
        {
            Q_OBJECT
        public:
            GroupMembersWindowJsInterface(GroupMembersWindowView *groupMembersWindow);

            void focusEvent();
            void blurEvent();

            Q_PROPERTY(QString conversationId READ conversationId);
            QString conversationId() { return mConversationId; }
            void setConversationId(const QString& id) { mConversationId = id; }

            Q_INVOKABLE void transferGroupOwner(QObject* user, const QString& groupGuid);
            Q_INVOKABLE void promoteUserToAdmin(QObject* user, const QString& groupGuid);
            Q_INVOKABLE void demoteAdminToMember(QObject* user, const QString& groupGuid);
            Q_INVOKABLE void callQueryGroupMembers(const QString& groupGuid);

        signals:
            void nativeFocus();
            void nativeBlur();
            void queryGroupMembers(const QString& groupGuid);
            void refresh();
            void transferOwner(QObject* user, const QString& groupGuid);
            void promoteToAdmin(QObject* user, const QString& groupGuid);
            void demoteToMember(QObject* user, const QString& groupGuid);

        private:
            GroupMembersWindowView *mWindow;
            QStringList mVisibleButtons;
            bool mShowFriends;
            bool mShowMembers;
            bool mShowOwners;
            QString mChannelName;
            QString mConversationId;
        };

        class GroupMembersWindowView : public WebWidget::WidgetView
        {
            Q_OBJECT
        public:
            GroupMembersWindowView(QWidget *parent, UIScope scope, const QString& groupGuid);
            ~GroupMembersWindowView();

            virtual bool event(QEvent *);
            UIScope scope() const { return mScope; }
            void setConversationId(const QString& id) { mJsInterface->setConversationId(id); }

        signals:
            /// \brief Emitted when the cursor of the web view changes. Used in OIG.
            void cursorChanged(int);
            void queryGroupMembers(const QString& groupGuid);
            void refresh();
            void transferOwner(QObject* user, const QString& groupGuid);
            void promoteToAdmin(QObject* user, const QString& groupGuid);
            void demoteToMember(QObject* user, const QString& groupGuid);

        private:
            Ui::GroupMembersWindowView* ui;
            GroupMembersWindowJsInterface *mJsInterface;
            UIScope mScope;
        };
        
    }
}

#endif
