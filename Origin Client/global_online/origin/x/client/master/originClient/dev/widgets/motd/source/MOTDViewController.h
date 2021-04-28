/////////////////////////////////////////////////////////////////////////////
// MOTDViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MOTD_VIEW_CONTROLLER_H
#define MOTD_VIEW_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QDateTime>

#include "services/network/HttpCacheValidator.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/plugin/PluginAPI.h"

class QWebView;

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Client
    {
        class MOTDJsHelper;
        class CommonJsHelper;

		class ORIGIN_PLUGIN_API MOTDViewController : public QObject
		{
			Q_OBJECT
		public:
			// motdUrlTempate has %1 replaced with the current locale
			MOTDViewController(const QString &motdUrlTemplate, QObject *parent = NULL);

            static bool isBrowserVisible();

            void raise();

        private slots:
            // Poll the entry MOTD URL
            void pollMotd();
            // Poll a specific URL
            void pollMotd(const QUrl &url);

            void pollTimerFired();

            void requestFinished();
            void armPollTimer(); 
            void destroyPage();

            void onUserConnectionStateChanged(bool isOnline, Origin::Services::Session::SessionRef session);

        private:
            QString mMotdUrlTemplate;

            /**
             * Used to suppress duplicate MOTD messages
             *
             * If the MOTD doesn't have a validator we just stop showing MOTDs until the
             * next next login. Last URL is used to handle redirects properly
             */
            Origin::Services::HttpCacheValidator mLastMotdValidator;

            QWebView* mMOTDBrowser;
            UIToolkit::OriginWindow* mMOTDWindow;
            MOTDJsHelper* mMotdJsHelper;
            CommonJsHelper* mCommonJsHelper;

            bool mPollTimerArmed;
            QDateTime mLastMotdCheck;
        };
    }
}

#endif