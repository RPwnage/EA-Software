/////////////////////////////////////////////////////////////////////////////
// UpdateController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef UPDATE_CONTROLLER_H
#define UPDATE_CONTROLLER_H

#include "services/rest/UpdateServiceResponse.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"

#include <QTimer>
#include <QPointer>

namespace Origin
{
    namespace Downloader
    {
        class ContentProtocolSingleFile;
    }
    namespace Engine
    {
        namespace Content
        {
            class ContentController;
        }

        class ORIGIN_PLUGIN_API UpdateController : public QObject
        {
            Q_OBJECT
        public:
            /// \brief Prepares the UpdateController for use.
            static void init();

            /// \brief Frees up the UpdateController.
            static void release();

            /// \brief Returns the current UpdateController instance.
            static UpdateController* instance();

            /// \brief Checks if there is an update available for the Origin Client
            void checkForUpdate(QString locale, QString type, QString platform);

            /// \brief Tells the update controller whether it should inhibit downloading updates for now
            void setUpdateInhibitFlag(bool inhibitUpdates);

        signals:
            /// \brief Emitted when an update is available
            void updateAvailable(const Services::AvailableUpdate &update);

            /// \brief Emitted when an update is pending
            void updatePending(const Services::AvailableUpdate &update);

            /// \brief Emitted when an update is not available
            void updateNotAvailable();

            /// \brief Emitted when an update is in progress
            //void updateInProgress();

            /// \brief Emitted when an update is ready to install
            void updateReadyToInstall();

        protected slots:
            /// \brief Slot that is triggered when we have finished checking for an update
            void onCheckForUpdateComplete();

            /// \brief Slot that is triggered when our update timer fires
            void onUpdateTimerFired();

            /// \brief Slot that is triggered when we have finished our head request
            void headFinished();

            /// \brief Slot that is triggered when the user has started playing a game.
            void onPlayStarted(Origin::Engine::Content::EntitlementRef entitlement, bool externalNonRtp);
                
            /// \brief Slot that is triggered when the user has finished playing the game.
            void onPlayFinished(Origin::Engine::Content::EntitlementRef entitlement);

        private:
            /// \brief The UpdateController constructor.
            UpdateController();

            /// \brief The UpdateController destructor.
            ~UpdateController() {}

            /// \brief The UpdateController assignment operator.
            const UpdateController& operator=(const UpdateController&);

            /// \brief Downloads an update
            void downloadUpdate(Services::AvailableUpdate update);

            /// \brief Handles when our update has finished downloading
            void downloadFinished();

            /// \brief Handles stopping any in-progress update download
            void stopDownloading();

            /// \brief Cleans up the SingleFile protocol object
            void cleanupProtocol();

        private slots:
            // Internal slots used for communicating with the content protocol
            void Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo);
            void onProtocolInitialized();
            void onProtocolContentLength ( qint64 contentLength );
            void onProtocolStarted(); 
            void onProtocolResumed(); 
            void onProtocolPaused();
            void onProtocolCanceled();
            void onProtocolFinished();
            void onProtocolTransferProgress(qint64 bytesDownloaded , qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

        private:
            /// \brief Our internal timer for when we should check for an update
            QTimer* mTimer;

            Services::UpdateQueryResponse* mUpdateAvailableResponse;
            QNetworkReply* mUpdateDownloadResponse;

            QPointer<Origin::Engine::Content::ContentController> currentContentController;

            bool mUpdateInProgress;
            bool mInhibitUpdating;
            bool mProtocolKillSent;
            QString mLocale;
            QString mType;
            QString mPlatform;
            QString mStagedFileName;
            QString mStagedFileNameWorking;
            Services::AvailableUpdate mAvailableUpdate;
            qint64 mUpdateContentLength;
            Origin::Downloader::ContentProtocolSingleFile* mProtocol;
        };
    }
}

#endif //UPDATE_CONTROLLER_H