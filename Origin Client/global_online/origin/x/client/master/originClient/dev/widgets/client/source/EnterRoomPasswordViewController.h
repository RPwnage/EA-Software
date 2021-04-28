/////////////////////////////////////////////////////////////////////////////
// EnterRoomPasswordViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef ENTER_ROOM_PASSWORD_VIEW_CONTROLLER_H
#define ENTER_ROOM_PASSWORD_VIEW_CONTROLLER_H

#include <QObject>
#include <chat/Connection.h>
#include "UIScope.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Chat
    {
        class EnterMucRoomOperation;
        class DestroyMucRoomOperation;
        class MucRoom;
    }

    namespace Client
    {
        class EnterRoomPasswordView;

        class EnterRoomPasswordViewController : public QObject
        {
            Q_OBJECT

        public:
            EnterRoomPasswordViewController(const QString& groupGuid, const QString& channelId, bool deleteRoom, QObject* parent = 0);
            ~EnterRoomPasswordViewController();

            void show(const UIScope& scope);

            const QString& channelId() const
            {
                return mChannelId;
            }

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onEnterRoom();
             void onEnteringMucRoom(const QString &roomId, Origin::Chat::EnterMucRoomOperation *op);
             void onRoomEntered(Origin::Chat::MucRoom* room);
             void onRoomEnterFailed(Origin::Chat::ChatroomEnteredStatus status);

             void onDeleteRoom();
             void onDestroyingMucRoom(const QString &roomId, Origin::Chat::DestroyMucRoomOperation *op);
             void onRoomDeleted();

        private:

            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            EnterRoomPasswordView* mEnterRoomPasswordView;
            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupGuid;
            QString mChannelId;
            bool mDeleteRoom;
        };
    }
}
#endif //ENTER_ROOM_PASSWORD_VIEW_CONTROLLER_H