/////////////////////////////////////////////////////////////////////////////
// DeleteGroupViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef DELETE_GROUP_VIEW_CONTROLLER_H
#define DELETE_GROUP_VIEW_CONTROLLER_H

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
        class DeleteGroupViewController : public QObject
        {
            Q_OBJECT

        public:
            DeleteGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent = 0);
            ~DeleteGroupViewController();

            void show(const UIScope& scope);

            const QString& groupGuid() const
            {
                return mGroupGuid;
            }
            void debugDeleteGroupFail(){onDeleteGroupFail();};

        signals:
            void groupDeleted(const QString&);
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onDeleteGroup();
             void onDeleteGroupSuccess();
             void onDeleteGroupFail();

        private:

            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupName;
            QString mGroupGuid;
        };
    }
}
#endif //DELETE_GROUP_VIEW_CONTROLLER_H