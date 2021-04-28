/////////////////////////////////////////////////////////////////////////////
// NetPromoterViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NET_PROMOTER_VIEW_CONTROLLER_H
#define NET_PROMOTER_VIEW_CONTROLLER_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class NetPromoterWidget;
}
}

namespace Origin
{
    namespace UIToolkit
    {
    class OriginWindow;
    }
    namespace Services
    {
    class Setting;
    }
	namespace Client
	{
        /// \brief Controller for NetPromoter windows. 
		class ORIGIN_PLUGIN_API NetPromoterViewController : public QObject
		{
			Q_OBJECT

		public:
            /// \brief Constructor
            /// \param parent The parent of the NetPromoterViewController - shouldn't be used.
            NetPromoterViewController(QWidget* parent = 0);

            /// \brief Destructor - 
			~NetPromoterViewController();

            /// \brief Shows the net promoter window.
			void show();

            /// \brief Checks if window should be suppressed.
            bool okToShow();

            /// \brief -1 = service not received yet, 0 = false, 1 = true
            static int isSurveyVisible();

		signals:
			void onNoNetPromoter();


		protected slots:
            /// \brief Reaction for when the user presses X in window titlebar.
            /// Records telemetry and calls closeNetPromoterDialog.
            void onUserSubmitNetPromoterDialog();

            void onUserCancelNetPromoterDialog();

            /// \brief Closes the NetPromoter window.
			void closeNetPromoterDialog();

            void showConfirmationDialog();

			void onShow();

			void onNoShow();

            void onTimeout();
			void logTimeOut();
            void logResponseError();

            void suveryChanged(bool);

		private:

			UIToolkit::OriginWindow* mDialog;
			NetPromoterWidget* mNetPromoter;

			QTimer *mResponseTimer;
        };
	}
}

#endif
