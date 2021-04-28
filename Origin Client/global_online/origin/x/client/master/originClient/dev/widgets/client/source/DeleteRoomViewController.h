/////////////////////////////////////////////////////////////////////////////
// DeleteRoomViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef DELETE_ROOM_VIEW_CONTROLLER_H
#define DELETE_ROOM_VIEW_CONTROLLER_H

#include <QObject>
#include "UIScope.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Chat
    {
        class DestroyMucRoomOperation;
    }

    namespace Client
    {
        class DeleteRoomViewController : public QObject
        {
            Q_OBJECT

        public:
            DeleteRoomViewController(const QString& groupGuid, const QString& channelId, const QString& roomName, QObject* parent = 0);
            ~DeleteRoomViewController();

            void show(const UIScope& scope);

            const QString& channelId() const
            {
                return mChannelId;
            }

        signals:
            void windowClosed();
            void roomDeleted(const QString&);

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onDeleteRoom();
             void onDestroyingMucRoom(const QString &channelId, Origin::Chat::DestroyMucRoomOperation *op);
             void onRoomEnterFailed();
             void onRoomDeleted();

        private:
            void hideWindow();

            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupGuid;
            QString mChannelId;
            QString mRoomName;
        };
    }
}
#endif //DELETE_ROOM_VIEW_CONTROLLER_H