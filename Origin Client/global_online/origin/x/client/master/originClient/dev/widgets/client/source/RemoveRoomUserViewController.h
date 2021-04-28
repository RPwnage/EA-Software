/////////////////////////////////////////////////////////////////////////////
// RemoveRoomUserViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef REMOVE_ROOM_USER_VIEW_CONTROLLER_H
#define REMOVE_ROOM_USER_VIEW_CONTROLLER_H

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
        class RemoveRoomUserViewController : public QObject
        {
            Q_OBJECT

        public:
            RemoveRoomUserViewController(const QString& groupGuid, const QString& channelId, const QString& userId, QObject* parent = 0);
            ~RemoveRoomUserViewController();

            void show(const UIScope& scope);
            void setUserId(const QString& userId){mUserId = userId;};

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onRemoveUser();

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupGuid;
            QString mChannelId;
            QString mUserId;
        };
    }
}
#endif //REMOVE_ROOM_USER_VIEW_CONTROLLER_H