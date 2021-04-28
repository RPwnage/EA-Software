/////////////////////////////////////////////////////////////////////////////
// EditGroupViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef EDIT_GROUP_VIEW_CONTROLLER
#define EDIT_GROUP_VIEW_CONTROLLER

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
        class CreateGroupView;

        class EditGroupViewController : public QObject
        {
            Q_OBJECT

        public:
            EditGroupViewController(const QString& groupName, const QString& groupGuid, QObject* parent = 0);
            ~EditGroupViewController();

            void show(const UIScope& scope);

            const QString& groupGuid() const
            {
                return mGroupGuid;
            }

            void debugEditGroupFail(){onEditGroupFail();};

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

        protected slots:
             void onEditGroup();
             void onEditGroupSuccess();
             void onEditGroupFail();

             void onAcceptButtonEnabled(bool isEnabled);

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            CreateGroupView* mEditGroupView;

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupName;
            QString mGroupGuid;
        };
    }
}
#endif //EDIT_GROUP_VIEW_CONTROLLER