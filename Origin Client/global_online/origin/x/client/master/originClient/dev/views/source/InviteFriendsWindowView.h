/////////////////////////////////////////////////////////////////////////////
// InviteFriendsWindowView.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef InviteFriendsWindowView_H
#define InviteFriendsWindowView_H


#include "WebWidget/WidgetView.h"
#include "UIScope.h"

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
        class InviteFriendsWindowView;

        class InviteFriendsWindowJsInterface : public QObject
        {
            Q_OBJECT
        public:
            InviteFriendsWindowJsInterface(InviteFriendsWindowView *groupInviteWindow);

            void focusEvent();
            void blurEvent();

            Q_ENUMS(InviteDialogType);
            Q_PROPERTY(InviteDialogType dialogType READ getDialogType);
            enum InviteDialogType
            {
                Room // Invite friends to chat room
                , Group   // Invite friends to chat group
                , Game  // Invite friends to game
            };
            InviteDialogType getDialogType() { return mDialogType; }
            void setDialogType(InviteDialogType type) { mDialogType = type; }

            Q_INVOKABLE void inviteUsersToGame(const QObjectList& users);
            Q_INVOKABLE void inviteUsersToGroup(const QObjectList& users, const QString& groupGuid);
            Q_INVOKABLE void inviteUsersToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId);
            Q_INVOKABLE void callQueryGroupMembers(const QString& groupGuid);

        signals:
            void nativeFocus();
            void nativeBlur();
            void refresh();
            void queryGroupMembers(const QString& groupGuid);
            void inviteToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId);
            void inviteToGroup(const QObjectList& users, const QString& groupGuid);
            void inviteToGame(const QObjectList& users);

        private:
            InviteFriendsWindowView *mWindow;
            InviteDialogType mDialogType;
        };

        class InviteFriendsWindowView : public WebWidget::WidgetView
        {
            Q_OBJECT
        public:
            InviteFriendsWindowView(QWidget *parent, UIScope scope, const QString& groupGuid, const QString& channelId, const QString& conversationId);
            ~InviteFriendsWindowView();

            virtual bool event(QEvent *);
            UIScope scope() const { return mScope; }

            void setDialogType(InviteFriendsWindowJsInterface::InviteDialogType type) { mJsInterface->setDialogType(type); }

        signals:
            /// \brief Emitted when the cursor of the web view changes. Used in OIG.
            void cursorChanged(int);

            void queryGroupMembers(const QString& groupGuid);
            void refresh();

            void inviteUsersToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId);
            void inviteUsersToGroup(const QObjectList& users, const QString& groupGuid);
            void inviteUsersToGame(const QObjectList& users);

        private:
            InviteFriendsWindowJsInterface *mJsInterface;
            UIScope mScope;
        };
        
    }
}

#endif
