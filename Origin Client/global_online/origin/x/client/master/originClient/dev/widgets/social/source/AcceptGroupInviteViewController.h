/////////////////////////////////////////////////////////////////////////////
// GroupInviteViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef ACCEPT_GROUP_INVITE_VIEW_CONTROLLER_H
#define ACCEPT_GROUP_INVITE_VIEW_CONTROLLER_H

#include <QObject>
#include "UIScope.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class AcceptGroupInviteViewController : public QObject
        {
            Q_OBJECT

        public:
            AcceptGroupInviteViewController(const QString& inviteFrom, const QString& groupName, const QString& groupGuid, QObject* parent = 0);
            ~AcceptGroupInviteViewController();

            void show(const UIScope& scope);
            void debugAcceptInviteFail(){onAcceptInviteFail();};

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onAcceptInvite();
             void onAcceptInviteSuccess();
             void onAcceptInviteFail();

             void onDeclineInvite();

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mInviteFrom;
            QString mGroupName;
            QString mGroupGuid;
        };
    }
}
#endif //GROUP_INVITE_VIEW_CONTROLLER_H