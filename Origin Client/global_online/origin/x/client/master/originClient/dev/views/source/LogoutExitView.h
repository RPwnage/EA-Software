/////////////////////////////////////////////////////////////////////////////
// LogoutExitView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef LOGOUTEXIT_H
#define LOGOUTEXIT_H

#include <QWidget>
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
        class OriginTransferBar;
    }

    namespace Client
    {
        class ORIGIN_PLUGIN_API CloudSyncProgressBar : public QWidget
        {
	        Q_OBJECT

	        public:
		        CloudSyncProgressBar(float currProgress = 0.0f, QWidget* pParent = NULL);
                ~CloudSyncProgressBar();

                void showCloudProgressBar (bool show);

	        public slots:
		        void onCloudProgressChanged(float, qint64, qint64);
		        void onCloudSyncSucceeded();

	        signals:
                void cloudSyncFailed(Origin::Engine::CloudSaves::SyncFailure);
		        void cloudSyncSuccess();
                void cloudSyncCancelled ();

	        private:
		        Origin::UIToolkit::OriginTransferBar* mCloudProgressBar;
		        bool mCloudSyncSucceded;
        };


        class ORIGIN_PLUGIN_API LogoutExitView : public QWidget
        {
            Q_OBJECT
            public:
                LogoutExitView(QWidget *parent = NULL);
                ~LogoutExitView();

            UIToolkit::OriginWindow * createCloudSyncProgressDialog (const QString& gameName, float currProgress);

            signals:
                void cloudSyncCancelled();
        };


    }


}

#endif