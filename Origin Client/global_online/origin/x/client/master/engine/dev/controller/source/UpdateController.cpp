/////////////////////////////////////////////////////////////////////////////
// UpdateController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "engine/updater/UpdateController.h"
#include "services/rest/UpdateServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "version/version.h"
#include "services/platform/PlatformService.h"

#include "ContentController.h"
#include "ContentProtocolSingleFile.h"

#ifdef ORIGIN_PC
#include <windows.h>
#include <ShlObj.h>
#endif

namespace Origin
{
    namespace Engine
    {
        static UpdateController* sInstance = NULL;

        void UpdateController::init()
        {
            if (!sInstance)
            {
                sInstance = new UpdateController();
            }
        }

        void UpdateController::release()
        {
            delete sInstance;
            sInstance = NULL;
        }

        UpdateController* UpdateController::instance()
        {
            return sInstance;
        }

        UpdateController::UpdateController()
            : mTimer(new QTimer(this))
            , mUpdateAvailableResponse(NULL)
            , mUpdateDownloadResponse(NULL)
            , mUpdateInProgress(false)
            , mInhibitUpdating(false)
            , mProtocolKillSent(false)
            , mUpdateContentLength(0)
            , mProtocol(NULL)
        {
            // 30 minutes
            mTimer->start(30 * 60 * 1000);
            ORIGIN_VERIFY_CONNECT(mTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerFired()));
        }

        void UpdateController::checkForUpdate(QString locale, QString type, QString platform)
        {
            if (mInhibitUpdating)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Auto-updating currently inhibited.  Skipping update check.";
                return;
            }

            if (mProtocol)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Auto-updater currently downloading.  Skipping update check.";
                return;
            }

            // If we don't have the content controller yet
            if (currentContentController.isNull())
            {
                currentContentController = Content::ContentController::currentUserContentController();

                if (currentContentController.isNull() == false)
                {
                    // Hook up the signals so we know when the user is playing a game
                    ORIGIN_VERIFY_CONNECT(currentContentController, SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)), this, SLOT(onPlayStarted(Origin::Engine::Content::EntitlementRef, bool)));
                    ORIGIN_VERIFY_CONNECT(currentContentController, SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)));
                }
            }

            // Store this information to use it for when our timer fires
            // This relies on always checking for updates when the client first starts, otherwise we 
            // won't have it for when the timer fires
            mLocale = locale;
            mType = type;
            mPlatform = platform;

            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Checking for update";

            mUpdateAvailableResponse = Services::UpdateServiceClient::isUpdateAvailable(locale, type, platform);
            ORIGIN_VERIFY_CONNECT(mUpdateAvailableResponse, SIGNAL(finished()), this, SLOT(onCheckForUpdateComplete()));
        }

        void UpdateController::setUpdateInhibitFlag(bool inhibitUpdates)
        {
            // If we're transitioning to an un-inhibited state
            if (mInhibitUpdating && !inhibitUpdates)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Resume normal auto-updating behavior.";

                // Schedule an update check in one minute because we may have missed the previous one
                // The one minute delay is to act as a sort of hysteresis to keep us from starting and stopping in rapid succession
                QTimer::singleShot(1000 * 60, this, SLOT(onUpdateTimerFired()));
            }
            // If we're transitioning to an inhibiting state
            else if (!mInhibitUpdating && inhibitUpdates)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Inhibit auto-updating.";

                // If there's an active update download going, kill it
                stopDownloading();
            }

            mInhibitUpdating = inhibitUpdates;
        }

        void UpdateController::onCheckForUpdateComplete()
        {
            if( mUpdateAvailableResponse != NULL &&
                mUpdateAvailableResponse->error() == Origin::Services::restErrorSuccess)
            {
                Services::AvailableUpdate update = mUpdateAvailableResponse->getUpdate();
                mUpdateAvailableResponse->deleteLater();
                mUpdateAvailableResponse = NULL;
                
                if (update.version.compare(EALS_VERSION_P_DELIMITED) == 0)
                {
                    // The version we're being told to download matches the DLL we have, ignore it
                    return;
                }
                if (update.activationDate.isValid() && update.activationDate > QDateTime::currentDateTimeUtc())
                {
                    emit (updatePending(update));
                    return;
                }
                if (update.version == mAvailableUpdate.version)
                {
                    // This is the same version we were already downloading
                    //emit(updateInProgress());

                    if (mUpdateInProgress)
                    {
                        ORIGIN_LOG_EVENT << "[Origin Autoupdater] Update already in progress, resuming...";

                        // Resume it
                        downloadUpdate(mAvailableUpdate);

                        return;
                    }
                }
                else if (mAvailableUpdate.version.compare("") != 0)
                {
                    // We've started to download another version. It's possible that this client has been inactive
                    // Delete our old download
                    mUpdateInProgress = false;

                    ORIGIN_LOG_EVENT << "[Origin Autoupdater] Newer update data found, clearing and restarting.";

                    // Clean up the final file, if any
                    if (QFile::exists(mStagedFileName))
                    {
                        ORIGIN_LOG_EVENT << "[Origin Autoupdater] Deleting staged update data: " << mStagedFileName;
                        QFile::remove(mStagedFileName);
                    }

                    // Clean up the partial file we might have had, if any
                    if (QFile::exists(mStagedFileNameWorking))
                    {
                        ORIGIN_LOG_EVENT << "[Origin Autoupdater] Deleting partial update data: " << mStagedFileNameWorking;
                        QFile::remove(mStagedFileNameWorking);
                    }

                    // Clean up the chunk map
                    if (QFile::exists(mStagedFileNameWorking + ".map"))
                    {
                        ORIGIN_LOG_EVENT << "[Origin Autoupdater] Deleting partial update metadata: " << mStagedFileNameWorking << ".map";
                        QFile::remove(mStagedFileNameWorking + ".map");
                    }
                }

                if (!update.downloadURL.isEmpty())
                {
                    if (update.updateRule == Services::UpdateRule_Mandatory)
                    {
                        // Surface this to the user
                        mAvailableUpdate = update;
                        emit(updateAvailable(update));
                    }
                    else
                    {
                        if (Origin::Services::readSetting(Services::SETTING_AUTOUPDATE))
                        {
                            // Download silently in the background
                            mAvailableUpdate = update;
                            downloadUpdate(update);
                        }
                        else
                        {
                            emit(updateNotAvailable());
                        }
                    }
                }
                else
                {
                    emit(updateNotAvailable());
                }
            }
        }

        void UpdateController::downloadUpdate(Services::AvailableUpdate update)
        {
            mUpdateInProgress = true;

            if (mInhibitUpdating)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Auto-updating currently inhibited.  Skipping update download.";
                return;
            }

            QString downloadPath;

#if defined(ORIGIN_MAC)
            downloadPath = Services::PlatformService::applicationBundleRootPath();
#elif defined(ORIGIN_PC)
			downloadPath = Services::PlatformService::commonAppDataPath() + "SelfUpdate\\";

            // Ensure our SelfUpdate working folder exists
            QDir updateDir(downloadPath);
            if (!updateDir.exists())
            {
                // Create it (Use SHCreateDirectoryEx because it will create the full path if neccesary)
                int dirResult = SHCreateDirectoryEx(NULL, downloadPath.utf16(), NULL);
                if (dirResult != ERROR_SUCCESS && dirResult != ERROR_ALREADY_EXISTS)
                {
                    ORIGIN_LOG_ERROR << "[Origin Autoupdater] Unable to create working path: " << downloadPath;
                }
            }
#else
#error "Need platform-specific definition!"
#endif

            QString fileName = downloadPath + "OriginUpdate";

            // Parse version and add it
            QStringList versionNumbers = update.version.split(".");
            QTextStream stream(&fileName);
            for (int i = 0; i < 4; i++)
            {
                stream << "_" << versionNumbers[i];
            }
            stream << ".zip";

            mStagedFileName = fileName;

            if(QFile::exists(mStagedFileName))
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Staged update for version " << update.version << " found on disk, verifying file size.";
                
                // Let's do a HEAD request and attempt to verify the size of the file...
                QNetworkRequest request(update.downloadURL);
                mUpdateDownloadResponse = Origin::Services::NetworkAccessManager::threadDefaultInstance()->head(request);
                
                ORIGIN_VERIFY_CONNECT(mUpdateDownloadResponse, SIGNAL(finished()), this, SLOT(headFinished()));
            }
            else
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Beginning background download of update version " << update.version;

                mStagedFileNameWorking = mStagedFileName + ".part";

                // Create a single file protocol
                Origin::Engine::Content::ContentController *cc = Origin::Engine::Content::ContentController::currentUserContentController();
                if (cc == NULL)
                    return;
                Origin::Engine::UserRef usrRef = cc->user();
                if (usrRef == NULL)
                    return;
                mProtocol = new Origin::Downloader::ContentProtocolSingleFile("Client_Update", usrRef->getSession());
                mProtocol->setGameTitle(QString("Origin Client Update - %1").arg(fileName));
                // Move the protocol to the proper thread
                Origin::Downloader::ContentProtocols::RegisterProtocol(mProtocol);
                mProtocolKillSent = false;
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(Protocol_Error(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(ContentLength(qint64, qint64)), this, SLOT(onProtocolContentLength(qint64)), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Started()), this, SLOT(onProtocolStarted()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Resumed()), this, SLOT(onProtocolResumed()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Paused()), this, SLOT(onProtocolPaused()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Canceled()), this, SLOT(onProtocolCanceled()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Finished()), this, SLOT(onProtocolFinished()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SLOT(onProtocolTransferProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);

                // Kick off the protocol
                mProtocol->Initialize(update.downloadURL, mStagedFileNameWorking, false);
            }
        }

        void UpdateController::onUpdateTimerFired()
        {
            ORIGIN_ASSERT(!mLocale.isEmpty());
            ORIGIN_ASSERT(!mType.isEmpty());
            ORIGIN_ASSERT(!mPlatform.isEmpty());

            checkForUpdate(mLocale, mType, mPlatform);
        }

        void UpdateController::headFinished()
        {
            qint64 expectedSize = -1;
            if( mUpdateDownloadResponse->hasRawHeader("Content-Length") )
            {
                QString contentLength = mUpdateDownloadResponse->rawHeader("Content-Length");
                expectedSize = contentLength.toLongLong();
            }

            // Cleanup the QNetworkReply
            if (mUpdateDownloadResponse)
            {
                mUpdateDownloadResponse->deleteLater();
                mUpdateDownloadResponse = NULL;
            }

            QFile stagedFile(mStagedFileName);

            if(stagedFile.exists())
            {
                qint64 actualSize = stagedFile.size();
                stagedFile.close();

                // We already have the update and it's the correct size, no need to re-download...
                if(actualSize == expectedSize)
                {	
                    ORIGIN_LOG_EVENT << "[Origin Autoupdater] Staged update for version " << mAvailableUpdate.version << " verified."; 
                    emit(updateReadyToInstall());
                    return;
                }
                else if(!stagedFile.remove())
                {
                    // This read-only file is completely blocking our update path, however the bootstrap should handle it more robustly on next start
                    ORIGIN_LOG_ERROR << "[Origin Autoupdater] Staged update for version " << mAvailableUpdate.version << " could not be removed and does not match server file size [" << expectedSize << " != " << actualSize << "].";
                    return;
                }
                else
                {
                    ORIGIN_LOG_WARNING << "[Origin Autoupdater] Staged update for version " << mAvailableUpdate.version << " was not valid, redownloading."; 
                    // We deleted the blocking update file, redownload
                    downloadUpdate(mAvailableUpdate);
                }
            }
            else
            {
                // somehow the staged file magically was deleted while we were waiting for the HEAD request to finish
                // reinitiate the download...
                downloadUpdate(mAvailableUpdate);
            }
        }
        
        void UpdateController::onPlayStarted(Origin::Engine::Content::EntitlementRef entitlement, bool externalNonRtp)
        {
            Q_UNUSED(entitlement);
            Q_UNUSED(externalNonRtp);

            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Inhibit auto-update due to play start.";

            setUpdateInhibitFlag(true);
        }
                
        void UpdateController::onPlayFinished(Origin::Engine::Content::EntitlementRef entitlement)
        {
            Q_UNUSED(entitlement);

            // If any games are still playing, don't un-inhibit
            if (!currentContentController.isNull())
            {
                if (currentContentController->isPlaying())
                {
                    ORIGIN_LOG_EVENT << "[Origin Autoupdater] Can't uninhibit because games are still playing.";
                    return;
                }
            }
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Uninhibit auto-update due to play stop.";

            setUpdateInhibitFlag(false);
        }

        void UpdateController::downloadFinished()
        {
            qint64 expectedSize = mUpdateContentLength;
            qint64 actualSize = 0;

            QFile stagedUpdateFile(mStagedFileNameWorking);

            if (stagedUpdateFile.exists())
                actualSize = stagedUpdateFile.size();

            if( actualSize == expectedSize )
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Completed background download of update version " << mAvailableUpdate.version << " successfully.";

                // Rename this to the final name, ie without .part
                stagedUpdateFile.rename(mStagedFileName);

                // Clean up the chunk map
                if (QFile::exists(mStagedFileNameWorking + ".map"))
                {
                    QFile::remove(mStagedFileNameWorking + ".map");
                }

                // If we aren't currently inhibiting updating, send the signal that we are ready (otherwise we'll send it later during the periodic check)
                if (!mInhibitUpdating)
                {
                    emit(updateReadyToInstall());
                }
            }
            else
            {
                ORIGIN_LOG_WARNING << "Background download of update version " << mAvailableUpdate.version << " failed.";
            }

            mUpdateInProgress = false;
        }

        void UpdateController::stopDownloading()
        {
            // If there's an active update download going, kill it
            if (mProtocol && !mProtocolKillSent && mProtocol->protocolState() == Origin::Downloader::kContentProtocolRunning)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Stopping in-progress download.";

                mProtocolKillSent = true;
                mProtocol->Pause();
            }
        }

        void UpdateController::cleanupProtocol()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Cleaning up protocol.";

            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mProtocol->deleteLater();
            mProtocol = NULL;
        }

        // Protocol signal handlers
        void UpdateController::Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo)
        {
            ORIGIN_LOG_WARNING << "[Origin Autoupdater] Protocol error: [errorCode:" << errorInfo.mErrorCode << ", errorType:" << errorInfo.mErrorType << "]";
            //emit protocolError();
        }

        void UpdateController::onProtocolInitialized()
        {  
            if(mProtocol->protocolState() == Origin::Downloader::kContentProtocolInitialized)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Update download initialized.";

                if (!mInhibitUpdating)
                {
                    // Fetch the content length
                    mProtocol->GetContentLength();
                }
                else
                {
                    ORIGIN_LOG_EVENT << "[Origin Autoupdater] Updating inhibited, cleaning up protocol.";
                    cleanupProtocol();
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << "[Origin Autoupdater] Failed to initialize protocol with URL.";
            }
        }

        void UpdateController::onProtocolContentLength ( qint64 contentLength )
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Update content length received: " << contentLength;

            // Save the content length
            mUpdateContentLength = contentLength;

            if (!mInhibitUpdating)
            {
                // Tell it to go ahead and start
                mProtocol->Start();
            }
            else
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Updating inhibited, cleaning up protocol.";
                cleanupProtocol();
            }
        }

        void UpdateController::onProtocolStarted()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Started download.";

            if (mInhibitUpdating)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Updating inhibited, stopping protocol.";

                stopDownloading();
            }
        }

        void UpdateController::onProtocolResumed()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Resumed download.";

            if (mInhibitUpdating)
            {
                ORIGIN_LOG_EVENT << "[Origin Autoupdater] Updating inhibited, stopping protocol.";

                stopDownloading();
            }
        }
 
        void UpdateController::onProtocolPaused()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Paused download.";

            cleanupProtocol();
        }

        void UpdateController::onProtocolCanceled()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Canceled download.";

            cleanupProtocol();
        }

        void UpdateController::onProtocolFinished()
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Finished download.";

            cleanupProtocol();

            downloadFinished();
        }

        void UpdateController::onProtocolTransferProgress(qint64 bytesDownloaded , qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
        {
            ORIGIN_LOG_EVENT << "[Origin Autoupdater] Progress.  Bytes Downloaded: " << bytesDownloaded << " Total Bytes: " << totalBytes;
        }
    }
}