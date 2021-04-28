//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: John Wade

#include "services/downloader/Common.h"
#include "services/settings/SettingsManager.h"
#include "InstallSession.h"
#include "services/escalation/IEscalationClient.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/debug/DebugService.h"
#include "services/process/IProcess.h"
#include <QCoreApplication>


namespace Origin
{

namespace Downloader
{

unsigned long InstallSession::sInstallTimeout = 30*60*1000; //30 min
QSemaphore InstallSession::sBarrier(1);

InstallSession::InstallSession(Origin::Engine::Content::EntitlementRef content) :
    mSemaphoreAcquired(false),
    mInErrorState(false),
    mContent(content)
{

}

bool InstallSession::cancel()
{
	//TODO implement
	return false;
}

void InstallSession::makeInstallRequest(const Downloader::ExecuteInstallerRequest& req)
{
    mInErrorState = false;
    InstallRequestJob* installJob = new InstallRequestJob(sBarrier, sInstallTimeout, req, this);
    installJob->start();
}

void InstallSession::onInstallError(Origin::Services::IProcess::ProcessError error, quint32 systemErrorValue)
{
    // Only release the semaphore if it was successfully acquired
    if (mSemaphoreAcquired)
    {
        sBarrier.release();
    }
    
    if (!mInErrorState)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);

        switch(error)
        {
        case Origin::Services::IProcess::RequiresAdminRights:
            errorInfo.mErrorType = InstallError::Escalation;
            errorInfo.mErrorCode = static_cast<qint32>(Origin::Escalation::kCommandErrorNotElevatedUser);
            break;

        case Origin::Services::IProcess::UACRequired:
            errorInfo.mErrorType = InstallError::Escalation;
            errorInfo.mErrorCode = static_cast<qint32>(Origin::Escalation::kCommandErrorProcessExecutionFailure);
            break;

        case Origin::Services::IProcess::InvalidPID:
            errorInfo.mErrorType = InstallError::InstallerExecution;
            errorInfo.mErrorCode = static_cast<qint32>(systemErrorValue);
            break;

        case Origin::Services::IProcess::InvalidHandle:
            errorInfo.mErrorType = InstallError::InvalidHandle;
            errorInfo.mErrorCode = static_cast<qint32>(systemErrorValue);
            break;

        case Origin::Services::IProcess::Timedout:
            errorInfo.mErrorType = InstallError::InstallerBlocked;
            errorInfo.mErrorCode = static_cast<qint32>(0);
            break;

        default:
            errorInfo.mErrorType = InstallError::InstallerExecution;
            errorInfo.mErrorCode = static_cast<qint32>(systemErrorValue);
        }

        mInErrorState = true;
        emit installFailed(errorInfo);
    }
}

void InstallSession::onInstallFinished(uint32_t pid, int exitCode, Origin::Services::IProcess::ExitStatus status)
{
    // Only release the semaphore if it was successfully acquired
    if (mSemaphoreAcquired)
    {
        sBarrier.release();
    }

    if (!mInErrorState)
    {
	    if (exitCode != 0)
	    {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);

            if (exitCode != InstallError::InstallerHung)
            {
                errorInfo.mErrorType = InstallError::InstallerFailed;
                errorInfo.mErrorCode = static_cast<qint32>(exitCode);
            }
            else
            {
                errorInfo.mErrorType = InstallError::InstallerHung;
                errorInfo.mErrorCode = static_cast<qint32>(0);
            }
        
		    emit installFailed(errorInfo);
	    }
	    else if (mContent->localContent()->installed(true) || !mContent->contentConfiguration()->monitorInstall())
	    {
		    emit installFinished();
	    }
        else
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = InstallError::InstallCheckFailed;
            errorInfo.mErrorCode = static_cast<qint32>(0);

            emit installFailed(errorInfo);
        }
    }
}


InstallSession::InstallRequestJob::InstallRequestJob(QSemaphore& barrier, unsigned long timeout,
    const Downloader::ExecuteInstallerRequest& req, InstallSession* parent) :
    mBarrier(barrier),
    mTimeout(timeout),
    mRequest(req),
    mParent(parent)
{
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(finished()), this, SLOT(deleteLater()), Qt::QueuedConnection);
}

void InstallSession::InstallRequestJob::run()
{
    Services::ThreadNameSetter::setCurrentThreadName("InstallRequestJob Thread");

    // Initialize flag to indicate that we have yet to acquire the semaphore successfully
    mParent->mSemaphoreAcquired = false;

    // Try to acquire the semaphore for the specified timeout value.  If we fail (waited too long), report back an error
    if (!mBarrier.tryAcquire(1, mTimeout))
    {
        mParent->onInstallError(Origin::Services::IProcess::Timedout, 0);

        return;
    }

    // Mark the semaphore as acquired
    mParent->mSemaphoreAcquired = true;

    Origin::Services::IProcess *mInstallRequestProcess = Origin::Services::IProcess::createNewProcess(mRequest.getInstallerPath(), mRequest.getCommandLineArguments(), mRequest.getCurrentDirectory(), "", mRequest.waitForExit(), true, true, false, false, mRequest.useProxy(), false, 0);
    if(mRequest.waitForExit())
    {
        ORIGIN_VERIFY_CONNECT_EX(mInstallRequestProcess, SIGNAL(finished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), mParent, SLOT(onInstallFinished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mInstallRequestProcess, SIGNAL(error(Origin::Services::IProcess::ProcessError, quint32)), mParent, SLOT(onInstallError(Origin::Services::IProcess::ProcessError, quint32)), Qt::QueuedConnection);
    }
    mInstallRequestProcess->start();

    if(!mRequest.waitForExit())
    {
        mParent->onInstallFinished(0, 0, Origin::Services::IProcess::NormalExit);
    }

    delete mInstallRequestProcess;
}

} // namespace Downloader
} // namespace Origin

