/////////////////////////////////////////////////////////////////////////////
// GroupInviteViewController.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef GROUPINVITEVIEWCONTROLLER_H
#define GROUPINVITEVIEWCONTROLLER_H

#include <QObject>

#include "UIScope.h"
#include <services/rest/GroupServiceResponse.h>

#include "chat/ChatGroup.h"


namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class InviteFriendsWindowView;

        class GroupInviteViewController : public QObject
        {
            Q_OBJECT
        public:

            GroupInviteViewController(const QString& groupGuid, const QString& channelId, const QString& conversationId, QWidget *parent, UIScope);
            virtual ~GroupInviteViewController();
            
            InviteFriendsWindowView* view() { return mInviteFriendsWindowView; }
            UIToolkit::OriginWindow* window() { return mWindow; }

            void show(UIScope scope);
            void close();

        signals:
            void queryGroupMembers();

        protected slots:
            void onLoadFinished(bool ok);
            void onTitleChanged(const QString& title);
            void onActivateWindow();
            void onInviteUsersToGroup(const QObjectList& users, const QString& groupGuid);
            void onInviteUsersToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId);

        protected:
            InviteFriendsWindowView *mInviteFriendsWindowView;
            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;
            Origin::Chat::ChatGroup* mChatGroup;
            QWidget* mWidgetParent;
            QString mGroupGuid;
            QString mChannelId;
            QString mConversationId;

        private slots:
            void onQueryGroupMembers();
            void onZoomed();
            void onWindowClosed();
            void closeWindow();
            void closeIGOWindow();
            void onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*> users);
        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);
        };

    }
} 

#endif //GROUPINVITEVIEWCONTROLLER_H
