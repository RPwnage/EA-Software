/////////////////////////////////////////////////////////////////////////////
// ConfigIngestViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_INGEST_VIEW_CONTROLLER_H
#define CONFIG_INGEST_VIEW_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

	namespace Client
	{
		class ORIGIN_PLUGIN_API ConfigIngestViewController : public QObject
		{
			Q_OBJECT
		public:
            /// \brief The ConfigIngestViewController constructor.
            /// \param parent This object's parent object, if any.
			ConfigIngestViewController(QObject* parent = NULL);

            /// \brief The ConfigIngestViewController destructor.
            ~ConfigIngestViewController();

            /// \brief Sets the number of milliseconds between updates.
            /// \param msec The number of milliseconds between updates.
            void setTickInterval(int msec);
            
            /// \brief Sets the number of milliseconds before auto-accepting the dialog.
            /// \param msec The number of milliseconds before auto-accepting the dialog.
            void setTimeoutInterval(int msec);

            /// \brief Execs the config ingest dialog.
            ///
            /// setTickInterval and setTimeoutInterval should be called prior to calling this function.
            /// \return The return code (QDialog::Accepted or QDialog::Rejected).
			int exec();

		private slots:
            /// \brief Slot that is triggered when the tick timer times out.
			void onTick();

		private:
            UIToolkit::OriginWindow *mConfigIngestWindow; ///< The configuration ingest dialog.
            QTimer mTickTimer; ///< The tick timer.  Every timeout updates the UI.
            QTimer mTimeoutTimer; ///< The timeout timer.  When this times out, the dialog is auto-accepted.
            quint32 mTickCount;
		};
	}
}

#endif