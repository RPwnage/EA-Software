/////////////////////////////////////////////////////////////////////////////
// SingleLoginView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SINGLE_LOGIN_VIEW_H
#define SINGLE_LOGIN_VIEW_H

#include "flows/SingleLoginFlow.h"
#include "services/plugin/PluginAPI.h"
#include <QScopedPointer>
#include <QWidget>

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class ORIGIN_PLUGIN_API SingleLoginView : public QWidget
        {
            Q_OBJECT

        public:
            SingleLoginView(QWidget *parent = 0);
            ~SingleLoginView();

            void showFirstUserDialog();
            void showSecondUserDialog(LoginType loginType);

            void showCloudSyncConflictDialog();

        signals:

            void loginOnline();
            void loginOffline();
            void cancelLogin();

        protected slots:
            void onLogInBtnClicked();
            void onOfflineBtnClicked();
            void onCancelClicked();
            void onCancelWaitingForCloudClicked();
            void closeCloudSyncConflictDialog();

        private:
            void init();
            UIToolkit::OriginWindow* createSecondUserDialog(const LoginType& loginType, const int& context);
            void closeMessageDialog();

            UIToolkit::OriginWindow* mMessageBox;
            UIToolkit::OriginWindow* mOIGMessageBox;
        };
    }
}
#endif // SINGLE_LOGIN_VIEW_H