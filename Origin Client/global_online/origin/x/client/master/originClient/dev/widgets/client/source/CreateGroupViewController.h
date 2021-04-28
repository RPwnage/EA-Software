/////////////////////////////////////////////////////////////////////////////
// CreateGroupViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef CREATE_GROUP_VIEW_CONTROLLER_H
#define CREATE_GROUP_VIEW_CONTROLLER_H

#include <QObject>
#include "UIScope.h"
#include "chat/ConnectedUser.h"
#include "ClientFlow.h"
#include "services/rest/GroupServiceResponse.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class CreateGroupView;

        class CreateGroupViewController : public QObject
        {
            Q_OBJECT

        public:
            CreateGroupViewController(QObject* parent = 0);
            ~CreateGroupViewController();

            void show(const UIScope& scope);
            void debugCreateGroupFail(){onCreateGroupError();};

        signals:
            void windowClosed();
            void createGroupSucceeded(const QString& groupGuid);

        public slots:
             void close();
             void closeWindow();
             void closeIGOWindow();

        protected slots:
             void onCreateGroup();
             void onCreateGroupSuccess();
             void onCreateGroupFail();
             void onCreateGroupError();

             void onAcceptButtonEnabled(bool isEnabled);
             void onCreateGroupFailPromptClosed(int);
        private slots:
             void onVisiblityChanged(Origin::Chat::ConnectedUser::Visibility);

        private:

            UIToolkit::OriginWindow* createWindow(const UIScope& scope);

            CreateGroupView* mCreateGroupView;
            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;
        };
    }
}
#endif //CREATE_GROUP_VIEW_CONTROLLER_H