/////////////////////////////////////////////////////////////////////////////
// RemoveGroupUserViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef REMOVE_GROUP_USER_VIEW_CONTROLLER_H
#define REMOVE_GROUP_USER_VIEW_CONTROLLER_H

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
        class RemoveGroupUserViewController : public QObject
        {
            Q_OBJECT

        public:
            RemoveGroupUserViewController(const QString& groupGuid, const QString& userId, const QString& userNickname, QObject* parent = 0);
            ~RemoveGroupUserViewController();

            void show(const UIScope& scope);
            void setUserId(const QString& userId){mUserId = userId;};
            void setUserNickname(const QString& nickName);
            void debugRemoveUserFail(){onRemoveUserFail();};

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onRemoveUser();
             void onRemoveUserSuccess();
             void onRemoveUserFail();

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupGuid;
            QString mUserNickname;
            QString mUserId;
        };
    }
}
#endif //REMOVE_GROUP_USER_VIEW_CONTROLLER_H