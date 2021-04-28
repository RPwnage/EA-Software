/////////////////////////////////////////////////////////////////////////////
// GroupMembersViewController.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef GROUPMEMBERSVIEWCONTROLLER_H
#define GROUPMEMBERSVIEWCONTROLLER_H

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
        class GroupMembersWindowView;

        class GroupMembersViewController : public QObject
        {
            Q_OBJECT
        public:

            GroupMembersViewController(const QString& groupGuid, QWidget *parent, UIScope);
            virtual ~GroupMembersViewController();
            
            GroupMembersWindowView* view() { return mGroupMembersWindowView; }
            UIToolkit::OriginWindow* window() { return mWindow; }

            /// \brief Sets the member list of a chat group
            void show(UIScope scope);
            void setChatGroupMemberList();
            void close();
        signals:
            void windowClosed();
            void queryGroupMembers();

        public slots:
            void onTransferOwner(QObject* user, const QString& groupGuid);
            void closeWindow();
            void closeIGOWindow();

        protected slots:
            void onLoadFinished(bool ok);
            void onWindowClosed();
            void onActivateWindow();
            void onPromoteToAdmin(QObject* user, const QString& groupGuid);
            void onDemoteToMember(QObject* user, const QString& groupGuid);            

        private slots:
            void onQueryGroupMembers();
            void onZoomed();
            void onPromoteToAdminFinished(QObject* user);
            void onDemoteToMemberFinished(QObject* user);
            void onUpdateOwnerFinished();
            void onTransferOwnerFinished(QObject* user);

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);
            GroupMembersWindowView *mGroupMembersWindowView;
            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;
            Origin::Chat::ChatGroup* mChatGroup;
            QWidget* mWidgetParent;
            QString mGroupGuid;

        };
    }
} 

#endif //GROUPMEMBERSVIEWCONTROLLER_H
