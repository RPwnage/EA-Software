/////////////////////////////////////////////////////////////////////////////
// CreateRoomViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef CREATE_ROOM_VIEW_CONTROLLER_H
#define CREATE_ROOM_VIEW_CONTROLLER_H

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
        class CreateRoomView;

        class CreateRoomViewController : public QObject
        {
            Q_OBJECT

        public:
            CreateRoomViewController(const QString& groupName, const QString& OriginAuthServiceResponse, QObject* parent = 0);
            ~CreateRoomViewController();

            void show(const UIScope& scope);

            const QString& groupGuid() const
            {
                return mGroupGuid;
            }

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

        protected slots:
             void onCreateRoom();
             void onAcceptButtonEnabled(bool isEnabled);
 
        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            CreateRoomView* mCreateRoomView;
            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupName;
            QString mGroupGuid;
        };
    }
}
#endif //CREATE_ROOM_VIEW_CONTROLLER_H