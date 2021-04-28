/////////////////////////////////////////////////////////////////////////////
// LeaveGroupViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef LEAVE_GROUP_VIEW_CONTROLLER_H
#define LEAVE_GROUP_VIEW_CONTROLLER_H

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
        class LeaveGroupViewController : public QObject
        {
            Q_OBJECT

        public:
            LeaveGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent = 0);
            ~LeaveGroupViewController();

            void show(const UIScope& scope);

            const QString& groupGuid() const
            {
                return mGroupGuid;
            }

        signals:
            void windowClosed();
            void groupDeparted(const QString&);

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onLeaveGroup();
             void onLeaveGroupSuccess();
             void onLeaveGroupFail();

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupName;
            QString mGroupGuid;
        };
    }
}
#endif //LEAVE_GROUP_VIEW_CONTROLLER_H