/////////////////////////////////////////////////////////////////////////////
// SingleLoginViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SINGLE_LOGIN_VIEW_CONTROLLER_H
#define SINGLE_LOGIN_VIEW_CONTROLLER_H

#include "flows/SingleLoginFlow.h"
#include "services/plugin/PluginAPI.h"
#include <QObject>
#include <QWidget>

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API SingleLoginViewController : public QObject
        {
            Q_OBJECT

        public:

            SingleLoginViewController(QWidget *parent = 0);
            ~SingleLoginViewController();

            void showFirstUserDialog();
            void showSecondUserDialog(LoginType loginType);

            void showCloudSyncDialog();
            void closeCloudSyncDialog();

            bool isCloudDialogShowing() { return mCloudDialogShowing; }

            void armTimer();

        signals:
            
            void pollCloudSaves();

            void loginOnline();
            void loginOffline();
            void cancelLogin();

            void cloudSyncComplete();

        protected slots:

            void checkCloudSave();

        private:
            int     mCloudCheckInterval;
            int     mCloudCheckIntervalChange;
            int     mCloudCheckTimeElapsed;
            bool    mCloudDialogShowing;

        };
    }
}
#endif // SINGLE_LOGIN_VIEW_CONTROLLER_H