///////////////////////////////////////////////////////////////////////////////
// InstallProgress.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
// Author: John Wade
///////////////////////////////////////////////////////////////////////////////
#ifndef INSTALLSESSION_H
#define INSTALLSESSION_H

#include "ExecuteInstallerRequest.h"

#include <QMetaType>
#include <QObject>
#include <QSemaphore>
#include <QThread>
#include "services/process/IProcess.h"
#include "services/downloader/DownloaderErrors.h"
#include "engine/content/ContentTypes.h"

namespace Origin
{

namespace Downloader
{
/// \brief Manages the installation of an entitlement.
class InstallSession : public QObject
{
    Q_OBJECT
public:
    /// \brief The InstallSession constructor.
    /// \param content The entitlement to be installed.
    InstallSession(Origin::Engine::Content::EntitlementRef content);

signals:
    /// \brief Emitted when an installation finishes successfully.
    void installFinished();

    /// \brief Emitted when an installation fails.
    /// \param errorInfo Object containing any relevant information about the error encountered.
    void installFailed(Origin::Downloader::DownloadErrorInfo errorInfo);

public:
    /// \brief Creates and starts an install job.
    /// \param req Object that contains necessary data for the installation (install path, etc.).
    void makeInstallRequest(const Downloader::ExecuteInstallerRequest& req);

    /// \brief Cancels the install job.
    /// \return True if the install job was successfully canceled.
    bool cancel();

private slots:
    /// \brief Slot that is triggered when an install job completes.
    /// \param exitCode The exit code that the process finished with.
    /// \param status The status of the install, described by Services::IProcess::ExitStatus.
	void onInstallFinished(uint32_t pid, int exitCode, Origin::Services::IProcess::ExitStatus status);

    /// \brief Slot that is triggered when an install job fails.
    /// \param error The error that the installer encountered, described by Services::IProcess::ProcessError.
    /// \param systemErrorValue The OS error code that caused the error.
    void onInstallError(Origin::Services::IProcess::ProcessError error, quint32 systemErrorValue);

private:
    /// \brief Time to wait on an install for considering it failed, in milliseconds.
    static unsigned long sInstallTimeout;

    /// \brief Prevents more than one install job from running at a time.
    static QSemaphore sBarrier;

    /// \brief Flag which indicates whether we acquired the semaphore (and whether we should release it)
    bool mSemaphoreAcquired;

    /// \brief True if the install is in an error state.
    bool mInErrorState;

    /// \brief a Pointer to the entitlement that is being installed.
    Origin::Engine::Content::EntitlementRef mContent;

    /// \brief Manages an install job on its own thread.
    class InstallRequestJob : public QThread
    {
        public:
            /// \brief The InstallRequestJob constructor.
            /// \param barrier A reference to InstallSession's sBarrier semaphore.
            /// \param timeout Time to wait on an install for considering it failed, in milliseconds.
            /// \param req Object that contains necessary data for the installation (install path, etc.).
            /// \param parent The InstallSession parent of this job.
            InstallRequestJob(QSemaphore& barrier, unsigned long timeout, const Downloader::ExecuteInstallerRequest& req, InstallSession* parent);

            /// \brief Starts the install job.  This function will fail if there is an install job currently running and the timeout expires.
            virtual void run();

        private:
            /// \brief A reference to InstallSession's sBarrier semaphore.
            QSemaphore& mBarrier;
            
            /// \brief Time to wait on an install for considering it failed, in milliseconds.
            unsigned long mTimeout;

            /// \brief Object that contains necessary data for the installation (install path, etc.).
            Downloader::ExecuteInstallerRequest mRequest;

            /// \brief The InstallSession parent of this job.
            InstallSession* mParent;
    };
};

} // Downloader
} // Origin
#endif // INSTALLSESSION_H
