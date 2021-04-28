/////////////////////////////////////////////////////////////////////////////
// TransferOwnerConfirmViewController.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef TRANSFER_OWNER_CONFIRM_VIEW_CONTROLLER_H
#define TRANSFER_OWNER_CONFIRM_VIEW_CONTROLLER_H

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
        class RemoteUser;
    }
    namespace Client
    {
        class TransferOwnerConfirmViewController : public QObject
        {
            Q_OBJECT

        public:
            TransferOwnerConfirmViewController(const QString& groupGuid, Chat::RemoteUser* user, const QString& userNickname, QObject* parent = 0);
            ~TransferOwnerConfirmViewController();

            void show(const UIScope& scope);
            void setUser(Chat::RemoteUser* user, const QString& userNickname);

        signals:
            void windowClosed();

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

         protected slots:
             void onTransferOwner();

        private:
            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            QString mGroupGuid;
            QString mUserNickname;

            Chat::RemoteUser* mUser;
        };
    }
}
#endif //TRANSFER_OWNER_CONFIRM_VIEW_CONTROLLER_H