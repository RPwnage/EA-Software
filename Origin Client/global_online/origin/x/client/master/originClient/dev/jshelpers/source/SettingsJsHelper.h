/////////////////////////////////////////////////////////////////////////////
// SettingsJsHelper.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NEWUSEREXPJSHELPER_H
#define NEWUSEREXPJSHELPER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API SettingsJsHelper : public QObject
        {
            Q_OBJECT
        public:
            SettingsJsHelper(QObject* parent = NULL);
            ~SettingsJsHelper();

        public slots:
            void showSettings();

            /// \brief Opens and shows the Change Avatar Window
            void uploadAvatar();

            /// \brief slot to catch when the user clicked on email verification banner
            void verifyEmailClicked();

        signals:
            void openSettings();

            /// \brief emitted when the user clicked on email verification banner
            void emailClicked();
        };
    }
}


#endif