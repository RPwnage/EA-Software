///////////////////////////////////////////////////////////////////////////////
// FirstLaunchFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef FIRSTLAUNCHFLOW_H
#define FIRSTLAUNCHFLOW_H

#include "flows/AbstractFlow.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Client
	{
        /// \brief Handles all high-level actions related to the first time application is launched on a machine before first login.
        ///     Currently relevant on Mac only.
		class ORIGIN_PLUGIN_API FirstLaunchFlow : public AbstractFlow
		{
			Q_OBJECT

		public:
            /// \brief The FirstLaunchFlow constructor.
			FirstLaunchFlow();

            /// \brief Public interface for starting the FirstLaunchFlow.
			virtual void start();

            /// \brief Populates and shows the first time settings dialog. DON'T CALL THIS UNLESS TO CALL FROM DEBUG MENU.
            void showFirstTimeSettingsDialog();

        private slots:
            /// \brief Triggered when users checks/unchecks auto start setting, stores choice in memory
            void onAutoStartChanged(const int& state);

            /// \brief Triggered when user checks/unchecks auto update setting, stores choice in memory
            void onAutoupdateChanged(const int& state);

            /// \brief Triggered when auto update setting 'Explain this' link clicked
            void onAutoupdateExplainThisClicked();
            
            /// \brief Triggered when user checks/unchecks anonymous HW data setting, stores choice in memory
            void onSendAnonymousHWDataChanged(const int& state);

            /// \brief Triggered when anonymous HW data setting 'Explain this' link clicked
            void onSendAnonymousHWDataExplainThisClicked();
            
            /// \brief Triggered when user closes the first launch settings window, commits settings to system.
            void onWindowClosed();

		private:
            bool mAutoStart;
            bool mAutoUpdate;
            bool mSendAnonymousHWData;
		};
	}
}

#endif // FIRSTLAUNCHFLOW_H
