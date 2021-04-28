/////////////////////////////////////////////////////////////////////////////
// LogoutExitViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef LOGOUT_EXIT_VIEW_CONTROLLER_H
#define LOGOUT_EXIT_VIEW_CONTROLLER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
	namespace Client
	{
        class LogoutExitView;

		class ORIGIN_PLUGIN_API LogoutExitViewController : public QObject
		{
			Q_OBJECT

		public:

			LogoutExitViewController();
			~LogoutExitViewController();

            void init ();

            // User prompts
            bool promptLogoutConfirmation();
            bool promptExitConfirmation();
            bool showCancelCloudSaves();
            bool promptCancelCloudSavesonExit();
            void promptCancelInstallationOnLogout(const QString& aGamename);
            void promptCancelInstallationOnExit(const QString& aGamename);
            bool promptPlayingGameOnExit(const QString& aGamename);
            void promptPlayingGameOnLogout(const QString& aGamename);

            /// \brief displayed when user attempts to go Online and has to log back in to go online but is playing a game
            void promptPlayingGameOnLogoutBadCredentials ();

			/// \brief displays dialog in case there are SDK-enabled games being played when chosen to close Origin
			void promptCannotCloseSDKGamePlaying(const QString& aGamename);

        public slots:
            void onAllCloudSyncsComplete ();

        signals:
            void cancelLogout();
            void cloudSyncCancelled ();
            void lastSyncFinished ();

        private slots:
            void onCloudSyncComplete ();
            void onCloudSyncCancelled();


		private:
            void disconnectSignals ();

            LogoutExitView *mLogoutExitView;
            Origin::UIToolkit::OriginWindow * mCloudSyncProgressDialog;

		};
	}
}

#endif //LOGOUT_EXIT_VIEW_CONTROLLER_H
