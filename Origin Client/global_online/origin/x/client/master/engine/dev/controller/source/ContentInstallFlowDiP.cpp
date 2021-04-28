/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowDiP.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/downloader/Common.h"
#include "engine/downloader/ContentInstallFlowDiP.h"
#include "DiPManifest.h"
#include "engine/downloader/DiPUpdate.h"
#include "InstallSession.h"
#include "InstallProgress.h"
#include "ContentProtocolPackage.h"
#include "engine/downloader/ContentServices.h"
#include "services/platform/EnvUtils.h"
#include "ExecuteInstallerRequest.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/downloader/DynamicDownloadController.h"
#include "engine/login/LoginController.h"
#include "TelemetryAPIDLL.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/qt/QtUtil.h"

#include "services/downloader/StringHelpers.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/Parameterizer.h"

#include "services/settings/SettingsManager.h"

#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/escalation/IEscalationService.h"
#include "services/connection/ConnectionStatesService.h"

#ifdef ORIGIN_PC
#include "OriginApplication.h"
#endif
#include "engine/utilities/StringUtils.h"
#include "engine/utilities/ContentUtils.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFinalState>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <QUuid>

#ifdef ORIGIN_MAC
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

// Log dumps information on Mac OS mounted filesystems
static void dumpFileSystemInformation()
{
    int count = getfsstat(0, 0, MNT_NOWAIT);
    struct statfs fslist[count];
    count = getfsstat(fslist, count * sizeof(struct statfs), MNT_NOWAIT);
    
    for (int i=0; i != count; ++i)
    {
        int64_t space =  fslist[i].f_bsize *  fslist[i].f_blocks;
        int64_t avail =  fslist[i].f_bsize *  fslist[i].f_bavail;
        avail /= 1024; // in Kilobytes
        
        ORIGIN_LOG_EVENT << "Filesystem: " << fslist[i].f_mntonname << " (type " << fslist[i].f_fstypename << "; Avail/Size = " << avail << " K / " << space << " K)";
    }
}
#endif // ORIGIN_MAC

// The number of times to retry when data corruption is detected (switches CDNs)
quint16 sMaxDataCorruptionRetries = 2;

/// \brief Sends Grey Market "language selection" telemetry event.
///
///        This event shows what language was selected for this offer ID by either
///        the user's choice or auto-selected for the user. The event also includes
///        the list of languages available to the user.
///
///        This event is used to verify how often grey market is being used and for
///        what content. It also provides a way to verify whether grey market is
///        being used correctly and whether or not users are working around/hacking
///        grey market in some fashion.
///
static void sendGreyMarketLanguageSelectionTelemetry(
    const bool supportsGreyMarket,
    const QString& offerID,
    const QString& selectedLanguage,
    const QStringList& availableLanguages,
    const QStringList& manifestLanguages)
{
    // For telemetry, collapse available language string list into single string 
    // separated by comma
    QString availableLanguagesString(Origin::StringUtils::formatLocaleListAsQString(availableLanguages));
    QString manifestLanguagesString(Origin::StringUtils::formatLocaleListAsQString(manifestLanguages));

    if (supportsGreyMarket)
    {
        // Grey Market telemetry - language selection
        GetTelemetryInterface()->Metric_GREYMARKET_LANGUAGE_SELECTION(
            offerID.toLocal8Bit().constData(), 
            selectedLanguage.toLocal8Bit(),
            availableLanguagesString.toLocal8Bit().constData(),
            manifestLanguagesString.toLocal8Bit().constData());
    }

    else
    {
        // Grey Market telemetry - language selection for "bypass" titles
        GetTelemetryInterface()->Metric_GREYMARKET_LANGUAGE_SELECTION_BYPASS(
            offerID.toLocal8Bit().constData(), 
            selectedLanguage.toLocal8Bit(),
            availableLanguagesString.toLocal8Bit().constData(),
            manifestLanguagesString.toLocal8Bit().constData());
    }
}

/// \brief Sends Grey Market "bypass filer" event.
///
///        This event shows which titles are not using the grey market feature,
///        which allows us to verify whether grey market is working correctly
///        and/or whether users are working around/hacking the feature.
///
static void sendGreyMarketBypassFilterTelemetry(
    const QString& offerID,
    const QStringList& availableLanguages,
    const QStringList& manifestLanguages)
{
    // For telemetry, collapse available language string list into single string 
    // separated by comma
    QString availableLanguagesString(Origin::StringUtils::formatLocaleListAsQString(availableLanguages));
    QString manifestLanguagesString(Origin::StringUtils::formatLocaleListAsQString(manifestLanguages));

    // Grey Market telemetry - bypass filter
    GetTelemetryInterface()->Metric_GREYMARKET_BYPASS_FILTER(
        offerID.toLocal8Bit().constData(),
        availableLanguagesString.toLocal8Bit().constData(),
        manifestLanguagesString.toLocal8Bit().constData());
}

/// \brief Sends Grey Market "language selection error" telemetry event.
///
///        A language selection error occurs when the user's entitled languages
///        for the content doesn't match the content they're installing. This
///        indicates that either the content is configured incorrectly in OFB
///        or the user is trying to install content they're not entitled to.
static void sendGreyMarketLanguageSelectionError(
    const QString& offerID, 
    const QStringList& availableLanguages,
    const QStringList& manifestLanguages)
{
    // For telemetry, collapse languages string into single string separated by comma
    QString manifestLanguagesString(Origin::StringUtils::formatLocaleListAsQString(manifestLanguages));
    QString availableLanguagesString(Origin::StringUtils::formatLocaleListAsQString(availableLanguages));

    // Grey Market telemetry - language selection error
    GetTelemetryInterface()->Metric_GREYMARKET_LANGUAGE_SELECTION_ERROR(
        offerID.toLocal8Bit().constData(),
        availableLanguagesString.toLocal8Bit().constData(),
        manifestLanguagesString.toLocal8Bit().constData());
}

namespace Origin
{
namespace Downloader
{

    const int kDelayBeforeJitRetry = 4000;

    ContentInstallFlowDiP::ContentInstallFlowDiP(Origin::Engine::Content::EntitlementRef content, Origin::Engine::UserRef user) :
        IContentInstallFlow(content, user),
        mProtocol(NULL),
        mManifest(NULL),
        mUpdateFinalizationStateMachine(NULL),
        mInstaller(new InstallSession(content)),
        mInstallProgress(new InstallProgress()),
        mRepair(false),
        mInstallProgressConnected(false),
        mInstallProgressSignalsConnected(false),
		mIsUpdate(false),
        mIsPreinstallRepair(false),
        mResumeOnPaused(false),
        mProcessingSuspend (false),
        mIsDynamicDownload(false),
        mIsDynamicUpdateFinalize(false),
        mDynamicRequiredPortionSize(0),
        mUpdateFinalizeTransition(NULL),
        mDiPManifestFinishedTransition(NULL),
        mPreTransferFinishedTransition(NULL),
        mDiPManifestMachine(NULL),
        mPreTransferMachine(NULL),
        mDataCorruptionRetryCount(0),
        mUseAlternateCDN(false),
        mUseDownloaderSafeMode(false),
        mEulasAccepted(false),
        mUseDefaultInstallOptions(false),
        mTryToMoveToTop(false)
    {
        initializeIntstallArgumentRequest();
        initializeEulaStateRequest();
        initializeFlow();
        initializeUpdateFlag(); // Must do this after the flow is initialized (relies on manifest being loaded)
        startFlow();

        ORIGIN_VERIFY_CONNECT(Origin::Services::Network::GlobalConnectionStateMonitor::instance(), SIGNAL(connectionStateChanged(Origin::Services::Connection::ConnectionStateField)), this, SLOT(onGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField)));

        ContentProtocols::RegisterWithInstallFlowThread(this);
    }

    ContentInstallFlowDiP::~ContentInstallFlowDiP()
    {
        delete mManifest;
        delete mInstaller;
        delete mInstallProgress;

        if (mProtocol)
            mProtocol->deleteLater();
    }

    void ContentInstallFlowDiP::deleteAsync()
    {
        if (!mProtocol)
        {
            onProtocolDeleted();
            return;
        }


        Origin::Services::ThreadObjectReaper* reaper = new Origin::Services::ThreadObjectReaper(mProtocol);
        ORIGIN_VERIFY_CONNECT(reaper, SIGNAL(objectsDeleted()), this, SLOT(onProtocolDeleted()));
        Downloader::ContentProtocols::RegisterWithProtocolThread(reaper);
        reaper->start();

        mProtocol = NULL;
    }

    void ContentInstallFlowDiP::onProtocolDeleted()
    {
        // This is just a safety in case of some weird race condition that would force a pause during an install (in which case
        // the InstallProgress monitoring would fail to close the pipe properly = installs would automatically fail)
        // ie this should really never happen - you cannot exit/logout during an install, but...
        if (mInstaller && mInstallProgressSignalsConnected)
        {
            // stop monitoring install progress
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Connected()), this, SLOT(onInstallProgressConnected()));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(ReceivedStatus(float, QString)), this, SLOT(onInstallProgressUpdate(float, QString)));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Disconnected()), this, SLOT(onInstallProgressDisconnected()));
            
            // disconnect install session signals
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFailed(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onInstallerError(Origin::Downloader::DownloadErrorInfo)));
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFinished()), this, SLOT(onInstallFinished()));
            
		    mInstallProgress->StopMonitor();
            mInstallProgressSignalsConnected = false;
        }
            
        deleteLater();
    }

    void ContentInstallFlowDiP::initializeInstallManifest()
    {
        // The download has been initialized/re-initialized, so reset the counter that keeps track of data corruption-related errors
        mDataCorruptionRetryCount = 0;

        ContentServices::InitializeInstallManifest(mContent, mInstallManifest);
        mInstallManifest.SetBool("eulasaccepted", mContent->localContent()->installed(true) || mEulasAccepted);
        mInstallManifest.SetBool("installDesktopShortCut", false);
        mInstallManifest.SetBool("installStartMenuShortCut", false);
        mInstallManifest.Set64BitInt("optionalComponentsToInstall", static_cast<qint64>(0));
        mInstallManifest.SetBool("installerChanged", false);
        mInstallManifest.Set64BitInt("stagedFileCount", static_cast<qint64>(0));
        mInstallManifest.SetBool("isLocalSource", false); // indicates if we are reading the zip off of physical media (e.g. ITO discs)
        mInstallManifest.SetBool("isITOFlow", false); //use this to indicate ITO (for both zips on disc and zips from CDN)
        mInstallManifest.SetBool("DynamicDownload", false); // Used by Dynamic Download games... indicates that the current flow was/is doing a dynamic download
        mInstallManifest.SetBool("DDinstallAlreadyCompleted", false); // Used by Dynamic Download games... indicates we've already run the installer
        mInstallManifest.SetBool("DDInitialDownload", false); // Used by Dynamic Download games... indicates that we're doing the initial download

        // Clear the DiP install path value, we will set it later when the flow begins
        if (!contentIsPdlc())
        {
            mInstallManifest.SetString("dipInstallPath", "");
        }
    }

    void ContentInstallFlowDiP::initiateIntersessionResume()
    {
        mInstallArgumentResponse.installDesktopShortCut = mInstallManifest.GetBool("installDesktopShortCut");
        mInstallArgumentResponse.installStartMenuShortCut = mInstallManifest.GetBool("installStartMenuShortCut");
        mInstallArgumentResponse.optionalComponentsToInstall = static_cast<qint32>(mInstallManifest.Get64BitInt("optionalComponentsToInstall"));
        mEulaLanguageResponse.selectedLanguage = mInstallManifest.GetString("locale");
    }

    void ContentInstallFlowDiP::initializePublicStateMachine()
    {
        QState* readyToStart = getState(ContentInstallFlowState::kReadyToStart);
        ORIGIN_VERIFY_CONNECT(readyToStart, SIGNAL(entered()), this, SLOT(onReadyToStart()));

        QState* starting = getState(ContentInstallFlowState::kInitializing);
        ORIGIN_VERIFY_CONNECT(starting, SIGNAL(entered()), this, SLOT(initializeProtocol()));

        QState* preTransfer = getState(ContentInstallFlowState::kPreTransfer);
        ORIGIN_VERIFY_CONNECT(preTransfer, SIGNAL(entered()), this, SLOT(onPreTransfer()));

        QState* langSelection = getState(ContentInstallFlowState::kPendingEulaLangSelection);
        ORIGIN_VERIFY_CONNECT(langSelection, SIGNAL(entered()), this, SLOT(onEulaLanguageSelection()));

        QState* pendingInstallArguments = getState(ContentInstallFlowState::kPendingInstallInfo);
        ORIGIN_VERIFY_CONNECT(pendingInstallArguments, SIGNAL(entered()), this, SLOT(onInstallInfo()));

        QState* pendingEula = getState(ContentInstallFlowState::kPendingEula);
        ORIGIN_VERIFY_CONNECT(pendingEula, SIGNAL(entered()), this, SLOT(onPendingEulaState()));

		QState* enqueued = getState(ContentInstallFlowState::kEnqueued);
		ORIGIN_VERIFY_CONNECT(enqueued, SIGNAL(entered()), this, SLOT(onEnqueued()));

        QState* transferring = getState(ContentInstallFlowState::kTransferring);
        ORIGIN_VERIFY_CONNECT(transferring, SIGNAL(entered()), this, SLOT(onTransferring()));

        QState* postTransfer = getState(ContentInstallFlowState::kPostTransfer);
        ORIGIN_VERIFY_CONNECT(postTransfer, SIGNAL(entered()), this, SLOT(onPostTransfer()));

        QState* install = getState(ContentInstallFlowState::kInstalling);
        ORIGIN_VERIFY_CONNECT(install, SIGNAL(entered()), this, SLOT(onInstall()));

		QState* postInstall = getState(ContentInstallFlowState::kPostInstall);
		ORIGIN_VERIFY_CONNECT(postInstall, SIGNAL(entered()), this, SLOT(OnPostInstall()));

        QState* pendingDiscChangeState = getState(ContentInstallFlowState::kPendingDiscChange);
        ORIGIN_VERIFY_CONNECT(pendingDiscChangeState, SIGNAL(entered()), this, SLOT(onPendingDiscChange()));

        QState* complete = getState(ContentInstallFlowState::kCompleted);
        ORIGIN_VERIFY_CONNECT(complete, SIGNAL(entered()), this, SLOT(onComplete()));

        QState* paused = getState(ContentInstallFlowState::kPaused);
        ORIGIN_VERIFY_CONNECT(paused, SIGNAL(entered()), this, SLOT(onPaused()));

        QState* readyToInstall = getState(ContentInstallFlowState::kReadyToInstall);
        QState* pausing = getState(ContentInstallFlowState::kPausing);
        QState* resuming = getState(ContentInstallFlowState::kResuming);
        QState* canceling = getState(ContentInstallFlowState::kCanceling);
        QState* error = getState(ContentInstallFlowState::kError);
        ORIGIN_VERIFY_CONNECT(error, SIGNAL(entered()), this, SLOT(onFlowError()));

        readyToStart->addTransition(this, SIGNAL(beginInitializeProtocol()), starting);
		readyToStart->addTransition(this, SIGNAL(enqueueRepair()), enqueued);	// for repairs
        readyToStart->addTransition(this, SIGNAL(canceling()), canceling);

        starting->addTransition(this, SIGNAL(protocolInitialized()), preTransfer);
        starting->addTransition(this, SIGNAL(canceling()), canceling);

        pendingInstallArguments->addTransition(this, SIGNAL(installArgumentsSet()), pendingEula);
        pendingInstallArguments->addTransition(this, SIGNAL(canceling()), canceling);

        pendingEula->addTransition(this, SIGNAL(pendingEulaComplete()), enqueued);
		pendingEula->addTransition(this, SIGNAL(protocolStarted()), transferring);	// for repairs/updates
		// The protocolFinished transition will happen on updates if the protocol determines that it
		// has nothing to download.
		pendingEula->addTransition(this, SIGNAL(protocolFinished()), postInstall);	// for repairs
		pendingEula->addTransition(this, SIGNAL(canceling()), canceling);
		
		enqueued->addTransition(this, SIGNAL(beginInitializeProtocol()), starting);	// for repairs
		enqueued->addTransition(this, SIGNAL(protocolStarted()), transferring);
		// The protocolFinished transition will happen on updates if the protocol determines that it
		// has nothing to download.
		enqueued->addTransition(this, SIGNAL(protocolFinished()), postInstall);
		enqueued->addTransition(this, SIGNAL(canceling()), canceling);

        transferring->addTransition(this, SIGNAL(protocolFinished()), postTransfer);
        transferring->addTransition(this, SIGNAL(dynamicDownloadRequiredChunksReadyToInstall()), postTransfer);
        transferring->addTransition(this, SIGNAL(pausing()), pausing);
        transferring->addTransition(this, SIGNAL(canceling()), canceling);
        transferring->addTransition(this, SIGNAL(protocolCannotFindSource(int, int, int)), pendingDiscChangeState);
        transferring->addTransition(this, SIGNAL(protocolPaused()), paused);

        pendingDiscChangeState->addTransition (this, SIGNAL(discChanged()), transferring);
        pendingDiscChangeState->addTransition(this, SIGNAL(canceling()), canceling);

        postTransfer->addTransition(this, SIGNAL(beginInstall()), install);
        postTransfer->addTransition(this, SIGNAL(dynamicDownloadRequiredChunksComplete()), install);
        postTransfer->addTransition(this, SIGNAL(protocolPaused()), paused);
        postTransfer->addTransition(this, SIGNAL(canceling()), canceling);

        readyToInstall->addTransition(this, SIGNAL(beginInstall()), install);

        install->addTransition(this, SIGNAL(finishedTouchupInstaller()), postInstall);
        install->addTransition(this, SIGNAL(pauseInstall()), readyToInstall);
        install->addTransition(this, SIGNAL(canceling()), canceling);

		postInstall->addTransition(this, SIGNAL(postInstallToComplete()), complete);
		postInstall->addTransition(this, SIGNAL(resumeExistingTransfer()), transferring);
		postInstall->addTransition(this, SIGNAL(canceling()), canceling);

        pausing->addTransition(this, SIGNAL(protocolPaused()), paused);

        paused->addTransition(this, SIGNAL(resuming()), resuming);
        paused->addTransition(this, SIGNAL(canceling()), canceling);

        resuming->addTransition(this, SIGNAL(protocolResumed()), transferring);
        resuming->addTransition(this, SIGNAL(protocolPaused()), paused);

        error->addTransition(this, SIGNAL(pausing()), pausing);
        error->addTransition(this, SIGNAL(protocolPaused()), paused);
        error->addTransition(this, SIGNAL(pauseInstall()), readyToInstall);
        error->addTransition(this, SIGNAL(resetForReadyToStart()), readyToStart);
    }

    void ContentInstallFlowDiP::StartDiPManifestMachine(QState* previousState, QState* nextState)
    {
        if (mDiPManifestMachine)
        {
            mDiPManifestMachine->deleteLater();
            mDiPManifestMachine = NULL;
        }

        mDiPManifestMachine = new QStateMachine(this);
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), mDiPManifestMachine, SLOT(stop()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(canceling()), mDiPManifestMachine, SLOT(stop()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(stopped()), mDiPManifestMachine, SLOT(stop()));
        mDiPManifestFinishedTransition = previousState->addTransition(mDiPManifestMachine, SIGNAL(finished()), nextState);

        QState* retrieveManifest = new QState(mDiPManifestMachine);
        mDiPManifestMachine->setInitialState(retrieveManifest);
        ORIGIN_VERIFY_CONNECT(retrieveManifest, SIGNAL(entered()), this, SLOT(retrieveDipManifest()));

        QState* processManifest = new QState(mDiPManifestMachine);
        ORIGIN_VERIFY_CONNECT(processManifest, SIGNAL(entered()), this, SLOT(processDipManifest()));

        QState* prerequisites = new QState(mDiPManifestMachine);
        ORIGIN_VERIFY_CONNECT(prerequisites, SIGNAL(entered()), this, SLOT(checkPrerequisites()));

        QFinalState* done = new QFinalState(mDiPManifestMachine);

        retrieveManifest->addTransition(this, SIGNAL(receivedFile(const QString&)), processManifest);
        processManifest->addTransition(this, SIGNAL(processedManifest()), prerequisites);
        prerequisites->addTransition(this, SIGNAL(prequisitesMet()), done);

        ContentProtocols::RegisterWithInstallFlowThread(mDiPManifestMachine);
        mDiPManifestMachine->start();
    }

    void ContentInstallFlowDiP::StartPreTransferMachine(QState* previousState, QState* nextState)
    {
        if (mPreTransferMachine)
        {
            mPreTransferMachine->deleteLater();
            mPreTransferMachine = NULL;
        }

        mPreTransferMachine = new QStateMachine(this);
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), mPreTransferMachine, SLOT(stop()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(canceling()), mPreTransferMachine, SLOT(stop()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(stopped()), mPreTransferMachine, SLOT(stop()));
        mPreTransferFinishedTransition = previousState->addTransition(mPreTransferMachine, SIGNAL(finished()), nextState);

        QState* exclusions = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT(exclusions, SIGNAL(entered()), this, SLOT(onProcessExclusionsAndDeletes()));

        QState* optionalEulas = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT(optionalEulas, SIGNAL(entered()), this, SLOT(requestOptionalEulas()));

        QState* receiveOptionalEulas = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT(receiveOptionalEulas, SIGNAL(entered()), this, SLOT(onReceivedOptionalEula()));

        QState* retrieveEulas = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT(retrieveEulas, SIGNAL(entered()), this, SLOT(getEulas()));

        QState* displayEulas = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT(displayEulas, SIGNAL(entered()), this, SLOT(onPendingUpdateEulaState()));

        QState* contentSize = new QState(mPreTransferMachine);
        ORIGIN_VERIFY_CONNECT_EX(contentSize, SIGNAL(entered()), mProtocol, SLOT(GetContentLength()), Qt::QueuedConnection);

        QFinalState* done = new QFinalState(mPreTransferMachine);

        mPreTransferMachine->setInitialState(exclusions);
        exclusions->addTransition(this, SIGNAL(processedExclusionsAndDeletes()), optionalEulas);
        
        optionalEulas->addTransition(this, SIGNAL(finishedOptionalEulas()), retrieveEulas);
        optionalEulas->addTransition(this, SIGNAL(receivedFile(const QString&)), receiveOptionalEulas);
        receiveOptionalEulas->addTransition(this, SIGNAL(receivedFile(const QString&)), receiveOptionalEulas);
        receiveOptionalEulas->addTransition(this, SIGNAL(finishedOptionalEulas()), retrieveEulas);
        retrieveEulas->addTransition(this, SIGNAL(receivedFile(const QString&)), retrieveEulas);
        if (!mIsUpdate)
        {
            // EULAs get displayed later for non-updates
            retrieveEulas->addTransition(this, SIGNAL(finishedEulas()), contentSize);
        }
        else
        {
            // For updates, we will display the EULAs now
            retrieveEulas->addTransition(this, SIGNAL(finishedEulas()), displayEulas);
            displayEulas->addTransition(this, SIGNAL(pendingEulaComplete()), contentSize);
        }
        contentSize->addTransition(this, SIGNAL(receivedContentSize(qint64, qint64)), done);
        

        ContentProtocols::RegisterWithInstallFlowThread(mPreTransferMachine);
        mPreTransferMachine->start();
    }

    qint64 ContentInstallFlowDiP::getContentTotalByteCount()
    {
        qint64 totalBytes = mInstallManifest.Get64BitInt("totalbytes");
        return totalBytes;
    }

    qint64 ContentInstallFlowDiP::getContentSizeToDownload()
    {
        qint64 totalBytes = mInstallManifest.Get64BitInt("totaldownloadbytes");
        return totalBytes;
    }

    qint64 ContentInstallFlowDiP::getDownloadedByteCount()
    {
        qint64 savedBytes = mInstallManifest.Get64BitInt("savedbytes");
        return savedBytes;
    }

    QString ContentInstallFlowDiP::getDownloadLocation()
    {
        // Retrieve it from the manifest if this is not PDLC
        if (!mContent->contentConfiguration()->isPDLC())
        {
            // Retrieve it from the install manifest
            QString installPath = mInstallManifest.GetString("dipInstallPath");
            if (installPath.isEmpty())
            {
                installPath = mContent->localContent()->dipInstallPath();
            }
            return installPath;
        }
        else
        {
            return mContent->localContent()->dipInstallPath();
        }
    }

    quint16 ContentInstallFlowDiP::getErrorRetryCount() const
    {
        return mDataCorruptionRetryCount + mDownloadRetryCount;
    }

    void  ContentInstallFlowDiP::initInstallParameters (QString& installLocale, QString& srcFile, ContentInstallFlags flags)
    {
        mRepair = flags & kRepair;

        if (installLocale.isEmpty())
        {
            // Pdlc should use the same locale as the parent.
            if (contentIsPdlc() && !mContent->parent().isNull())
            {
                // Set pdlc locale to the locale in the parents .dat file
                installLocale = mContent->parent()->localContent()->installedLocale();
                if (installLocale.isEmpty())
                {
                    // The parent .dat did not have a locale. Default to client locale.
                    installLocale = QLocale().name();

                    // Check for locale setting in the parent's install manifest.
                    Parameterizer parentInstallManifest;
                    ContentServices::LoadInstallManifest(mContent->parent(), parentInstallManifest);
                    if (parentInstallManifest.IsParameterPresent("locale"))
                    {
                        installLocale = parentInstallManifest.GetString("locale");
                        mContent->parent()->localContent()->setInstalledLocale(installLocale);
                    }
                    mContent->parent()->localContent()->saveNonUserSpecificData();
                }
                mContent->localContent()->setInstalledLocale(installLocale);
            }
            // Updates and repairs of main content should already have the locale set in the .dat file.
            // If installLocale is empty, we need to check the install manifest for it.
            else if (mIsUpdate || mRepair)
            {
                installLocale = mInstallManifest.GetString("locale");
                if (installLocale.isEmpty())
                {
                    installLocale = QLocale().name();
                }
                mContent->localContent()->setInstalledLocale(installLocale);
            }
        }

        mEulaLanguageResponse.selectedLanguage = installLocale;
		if (mEulaLanguageResponse.selectedLanguage.size() == 0)
		{
			ORIGIN_LOG_ERROR << "Setting mEulaLanguageResponse.selectedLanguage to an empty string from installLocale in initInstallParameters()";
		}

        if (!srcFile.isEmpty())
        {
            mUrl = srcFile;
            mIsOverrideUrl = true;
        }
        else
        {
            initUrl();
        }


        //sets the bit flags for both localSource and ITO
        //an ITO build from a disc will have both the kLocalSource and kITO bits set
        //an ITO build that downloads from CDN will just have the kITO bit set
        mInstallManifest.SetBool ("isLocalSource", flags & kLocalSource);  
        mInstallManifest.SetBool ("isITOFlow", flags & kITO);
        
        mInstallManifest.SetBool("isrepair", mRepair);

        if (flags & kITO)
        {
            mEulasAccepted = flags & kEulasAccepted;
            mUseDefaultInstallOptions = flags & kDefaultInstall;
        }
        else
        {
            mEulasAccepted = false;
            mUseDefaultInstallOptions = false;
        }

        mTryToMoveToTop = flags & kTryToMoveToTop;
        
        // Persist download path
        // Only save the folder for base game, because all other content is relative to parent content.
        // This is because DLC/etc flows are initialized potentially before the base game is installed, and we wouldn't know the
        // proper path to use yet.
        if (!contentIsPdlc())
        {
            mInstallManifest.SetString("dipInstallPath", mContent->localContent()->dipInstallPath());
        }

        mSuppressDialogs = flags & kSuppressDialogs;
    }

    void ContentInstallFlowDiP::onReadyToStart()
    {
        mWaitingForInstallerFlow = false;
        mInstallerFlowCreated = false;
    }

    void ContentInstallFlowDiP::onPreTransfer()
    {
        StartDiPManifestMachine(getState(ContentInstallFlowState::kPreTransfer), getState(ContentInstallFlowState::kPendingEulaLangSelection));
    }

    void ContentInstallFlowDiP::makeJitUrlRequest()
    {
        if (!makeOnlineCheck("User is offline. Unable to retrieve content url"))
        {
            // The flow will get suspended in IContentInstallFlow::onConnectionStateChanged
            return;
        }

        // If we prefer a certain CDN, set that here
        QString preferredCDN = "";
        if (mUseAlternateCDN)
        {
            if (mLastCDN.compare("akamai", Qt::CaseInsensitive)==0)
                preferredCDN = "level3";
            else if (mLastCDN.compare("lvlt", Qt::CaseInsensitive)==0) // This is intentional, the URL is 'lvlt', the REST call expects 'level3'
                preferredCDN = "akamai";

            ORIGIN_LOG_WARNING << "Overriding default behavior to use CDN: " << preferredCDN;
        }

        QString buildId = mContent->contentConfiguration()->buildIdentifierOverride();
        if (buildId.isEmpty())
        {
            // (OFM-3433) use the buildId in the install manifest (which if populated means we are in the middle of a download and are resuming) to
            // make sure we are resuming with the same version even if not the latest.  We currently are only doing this for dynamic downloads (aka Progressive Install)
            QString tmp = mInstallManifest.GetString("buildid");
            if (!tmp.isEmpty() && mInstallManifest.GetBool("DynamicDownload"))  
            {
                buildId = tmp;
            }
        }

        // keep in-sync with the install manifest (will get cleared if anything goes wrong with JitUrl request)
        if (buildId.isEmpty())
            mInstallManifest.SetString("buildId", mContent->contentConfiguration()->liveBuildId());
        else
            mInstallManifest.SetString("buildId", buildId);
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        bool useHTTPS = false;

        // Use SSL/TLS when in safe mode
        bool downloaderSafeMode = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloaderSafeMode);
        if (downloaderSafeMode || mUseDownloaderSafeMode)
        {
            useHTTPS = true;

            ORIGIN_LOG_EVENT << "Requesting HTTPS URL due to Downloader Safe Mode.";
        }
        else
        {
            // Use SSL/TLS when enabled via EACore.ini override
            bool httpsOverrideEnabled = Origin::Services::readSetting(Origin::Services::SETTING_EnableHTTPS);
            if (httpsOverrideEnabled)
            {
                useHTTPS = true;

                ORIGIN_LOG_EVENT << "Requesting HTTPS URL due to EACore.ini override.";
            }
            else
            {
                // Use SSL/TLS when we've CDN swapped due to data corruption
                if (mUseAlternateCDN)
                {
                    ORIGIN_LOG_EVENT << "Requesting HTTPS URL due to data corruption/CDN failover.";

                    useHTTPS = true;
                }
            }
        }

        Services::Publishing::DownloadUrlResponse* resp = Services::Publishing::DownloadUrlProviderManager::instance()->downloadUrl(Services::Session::SessionService::currentSession(), 
            mContent->contentConfiguration()->productId(), buildId, mContent->contentConfiguration()->buildReleaseVersionOverride(), preferredCDN, useHTTPS);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(completeInitializeProtocolWithJitUrl()));
    }

    void ContentInstallFlowDiP::initializeProtocol()
    {
        createProtocol();
        mProtocol->SetUpdateFlag(mIsUpdate || contentIsPdlc());
        mProtocol->SetRepairFlag(mRepair);
        
        // Protocol defaults to being in safe-cancel mode
        mProtocol->SetSafeCancelFlag(true);

        // Since main content and pdlc files are stored in the same crc map, an id has to be
        // assigned to each map entry so update and repair jobs know which files to check.
        mProtocol->SetCrcMapKey(QString("%1").arg(qHash(mContent->contentConfiguration()->installCheck())));

        // creates the cache dir if it doesn't exist
        QString cachePath;
        int contentDownloadErrorType = 0;
        int elevation_error = 0;
        if (contentIsPdlc())
        {
            ContentServices::GetContentInstallCachePath(mContent->parent(), cachePath, true, &contentDownloadErrorType, &elevation_error);
        }
        else
        {
            ContentServices::GetContentInstallCachePath(mContent, cachePath, true, &contentDownloadErrorType, &elevation_error);
        }

        // if the install cache folder creation returned an escalation error
		// we cannot continue, so report the error and exit. (EBIBUGS-18168)
		if (contentDownloadErrorType != ContentDownloadError::OK)
		{
			CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
			errorInfo.mErrorType = contentDownloadErrorType;
			errorInfo.mErrorCode = elevation_error;
            errorInfo.mErrorContext = cachePath;
			onProtocolError(errorInfo);
			return;
		}

        if (!mIsOverrideUrl || mContent->contentConfiguration()->overrideUsesJitService()) // JIT url generation
        {
            makeJitUrlRequest();
            return;
        }

        // Grab the sync package url from the settings, since we aren't using the JIT service
        mSyncPackageUrl = mContent->contentConfiguration()->overrideSyncPackageUrl();

        mProtocol->Initialize(mUrl, getDownloadLocation(), isUsingLocalSource());
    }

    void ContentInstallFlowDiP::createProtocol()
    {
        QString cachePath;

        if (contentIsPdlc())
        {
            ContentServices::GetContentInstallCachePath(mContent->parent(), cachePath);
        }
        else
        {
            ContentServices::GetContentInstallCachePath(mContent, cachePath);
        }

        if (mProtocol == NULL)
        {
            Engine::UserRef user(mUser.toStrongRef());
            ORIGIN_ASSERT(user);
            QString crcMapPrefix;
            if (mContent->contentConfiguration()->isPDLC())
                crcMapPrefix = mContent->contentConfiguration()->installationDirectory();
            mProtocol = new ContentProtocolPackage(mContent->contentConfiguration()->productId(), cachePath, crcMapPrefix, user->getSession());
            mProtocol->setGameTitle(mContent->contentConfiguration()->displayName());
            mProtocol->setJobId(currentDownloadJobId());

            // Watermark content if enabled by catalog or EACore.ini override
            bool debugWatermarking = Origin::Services::readSetting(Origin::Services::SETTING_ForceContentWatermarking, Origin::Services::Session::SessionRef());
            if (mContent->contentConfiguration()->watermarkDownload() || debugWatermarking)
            {
                if (debugWatermarking)
                    ORIGIN_LOG_EVENT << "[Fingerprint] Fingerprinting enabled for content: " << mContent->contentConfiguration()->productId();

                QMap<QString,QString> contentTagBlock;

                contentTagBlock["entid"] = QString("%1").arg(mContent->contentConfiguration()->entitlementId());
                contentTagBlock["time"] = QString("%1").arg(QDateTime::currentMSecsSinceEpoch());
                contentTagBlock["mac"] = QString(GetTelemetryInterface()->GetMacAddr());

                mProtocol->SetContentTagBlock(contentTagBlock);
            }

            ContentProtocols::RegisterProtocol(mProtocol);

            ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(stopped()), this, SLOT(onStopped()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Canceled()), this, SLOT(onCanceled()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SLOT(protocolTransferProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(NonTransferProgress(qint64, qint64, qint64, qint64)), this, SLOT(onNonTransferProgressUpdate(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(ContentLength(qint64, qint64)), this, SLOT(setContentSize(qint64, qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(FileDataReceived(QString,QString)), this, SLOT(receiveFileData(QString,QString)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(CannotFindSource(int, int, int, bool)), this, SLOT (promptForDiscChange(int, int, int)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(SourceFound(bool)), this, SLOT(onProtocolSourceFound(bool)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Started()), this, SLOT(onProtocolStarted()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Finished()), this, SLOT(onProtocolFinished()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Resumed()), this, SLOT(onProtocolResumed()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Paused()), this, SLOT(onProtocolPaused()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(CannotFindSource(int, int, int, bool)), this, SLOT(onProtocolCannotFindSource(int, int, int, bool)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicRequiredPortionSizeComputed(qint64)), this, SLOT(onDynamicRequiredPortionSizeComputed(qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkReadyToInstall(int)), this, SLOT(onDynamicChunkReadyToInstall(int)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkInstalled(int)), this, SLOT(onDynamicChunkInstalled(int)), Qt::QueuedConnection);
            if (isUpdate())
            {
                ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Finalized()), this, SLOT(onUpdateFinalized()), Qt::QueuedConnection);
            }
        }
    }

    void ContentInstallFlowDiP::completeInitializeProtocolWithJitUrl()
    {
        if (!makeOnlineCheck("User is offline. Unable to initialize protocol with Jit Url."))
        {
            //The flow will get suspended in IContentInstallFlow::onConnectionStateChanged
            return;
        }

        // Turn off the alternate CDN logic (we want it active once per failure only, if we need it again, it will be re-enabled)
        mUseAlternateCDN = false;

        Engine::UserRef user(mUser.toStrongRef());

        Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());
        ORIGIN_ASSERT(response);
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(completeInitializeProtocolWithJitUrl()));
        response->deleteLater();

        if(response && response->error() == Origin::Services::restErrorSuccess)
        {
            mUrl = response->downloadUrl().mURL;
            mSyncPackageUrl = response->downloadUrl().mSyncURL;

            ORIGIN_LOG_DEBUG << "Retrieved JIT URL " << mUrl << " for product ID " << mContent->contentConfiguration()->productId();

            if (mSyncPackageUrl.isEmpty())
            {
                // Grab the sync package url from the settings, since the JIT service didn't give us one
                mSyncPackageUrl = mContent->contentConfiguration()->overrideSyncPackageUrl();
            }

            // Capture the last CDN in use
            QString urlHostname = QUrl(mUrl).host();
            if (urlHostname.contains("lvlt", Qt::CaseInsensitive))
            {
                mLastCDN = "lvlt";
            }
            else 
            {
                // Default to akamai
                mLastCDN = "akamai";
            }

            if (mProtocol)
            {
                mProtocol->Initialize(mUrl, getDownloadLocation(), false);
            }
        }
        else
        {
            // Error state
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::JitUrlRequestFailed;
            errorInfo.mErrorCode = response == NULL ? 0 : response->error();
            onProtocolError(errorInfo);
        }
    }

    void ContentInstallFlowDiP::onInstallProgressConnected()
    {
        ORIGIN_LOG_DEBUG << "Install Progress Connected!!";
        mInstallProgressConnected = true;
    }

    void ContentInstallFlowDiP::onInstallProgressDisconnected()
    {
        ORIGIN_LOG_DEBUG << "Install Progress Disconnected!!";
        mInstallProgressConnected = false;
    }

    void ContentInstallFlowDiP::onInstallProgressUpdate(float progress, QString description)
    {
        // Generate some dummy values that give us the right progress percentage.
        qint64 bytesTotal;
        
        // If we don't have totalbytes stored for this content, we have to use a dummy 1000 to generate percentages
        // otherwise we end up with %nan in the progress bar.
        if(!mInstallManifest.Get64BitInt("totalbytes", bytesTotal))
        {
            bytesTotal = 1000;
        }

        qint64 bytesProcessed = static_cast<qint64>(progress * bytesTotal);
        
        // There isn't really a way to calculate time remaining or bps so make these fields invalid.
        onProgressUpdate(bytesProcessed, bytesTotal, -1, -1);
    }

    void  ContentInstallFlowDiP::setContentSize(qint64 numBytesInstalled, qint64 numBytesToDownload)
    {
        mInstallArgumentRequest.installedSize = numBytesInstalled;
        mInstallArgumentRequest.downloadSize = numBytesToDownload;
        mInstallManifest.Set64BitInt("totalbytes", numBytesInstalled);
        mInstallManifest.Set64BitInt("totaldownloadbytes", numBytesToDownload);

        emit receivedContentSize(numBytesInstalled, numBytesToDownload);
    }

    void ContentInstallFlowDiP::retrieveDipManifest()
    {
        QString manifestPath(mContent->contentConfiguration()->dipManifestPath());

		if(isUsingLocalSource() && mPendingTransferFiles.contains(manifestPath))
		{
			emit receivedFile(mPendingTransferFiles[manifestPath]);
		}
		else if (makeOnlineCheck("User is offline. Unable to retrieve DiP Manifest"))
		{ 
			mPendingTransferFiles.insert(manifestPath,"");
			mTemporaryFiles.push_back(ContentServices::GetTempFilename(mContent->contentConfiguration()->productId(), "man"));
            if (mTemporaryFiles.back().isEmpty())
            {
                ORIGIN_LOG_ERROR << "Unable to get temp filename. OfferID: " << mContent->contentConfiguration()->productId();
            }
            if(mProtocol != NULL)
			    mProtocol->GetSingleFile(manifestPath, mTemporaryFiles.back());
		}
        //else the flow will get suspended in IContentInstallFlow::onConnectionStateChanged
    }

    void ContentInstallFlowDiP::requestOptionalEulas()
    {
        if (!mProtocol || mIsUpdate || mManifest->GetOptionalCompInfo(currentLanguage()).empty())
        {
            emit finishedOptionalEulas();
        }
        else
        {
            int eulasRequested = 0;
            OptComponentInfoList::const_iterator it =  mManifest->GetOptionalCompInfo(currentLanguage()).begin();
            while (it != mManifest->GetOptionalCompInfo(currentLanguage()).end())
            {
                quint32 fileSignature = 0;
                if (!mProtocol->IsFileInJob(it->mEulaFileName, fileSignature))
                {
                    ++it;
                    continue;
                }

                if (makeOnlineCheck("User is offline. Unable to retrieve optional eula"))
                {
                    QString tempFileName = ContentServices::GetTempFilename(mContent->contentConfiguration()->productId(), "eul");
                    QString cancelMessage(it->mCancelWarning);
                    QString toolTip(it->mToolTip);
                    // The new EULA dialog can read in anything. This tells us whether 
                    // or not we should use our old RTF dialog. By the time it gets to the
                    // UI layer the file is a temp file.
                    const bool isRtfFile = it->mEulaFileName.contains(".rtf"); 
                    Eula eula(it->mEulaName, it->mInstallName, tempFileName, isRtfFile, false, true, cancelMessage, toolTip);
                    eula.IsOptional = true;
                    eula.signature = fileSignature;
                    eula.originalFilename = it->mEulaFileName;
                    mInstallArgumentRequest.optionalEulas.listOfEulas.push_back(eula);

                    mPendingTransferFiles.insert(it->mEulaFileName, "");
                    mTemporaryFiles.push_back(tempFileName);
                    if (mTemporaryFiles.back().isEmpty())
                    {
                        ORIGIN_LOG_ERROR << "Unable to get temp filename. OfferID: " << mContent->contentConfiguration()->productId();
                    }
                    
                    mProtocol->GetSingleFile(it->mEulaFileName, mTemporaryFiles.back());
                    ++eulasRequested;
                }
                //else the flow will get suspended in IContentInstallFlow::onConnectionStateChanged

                ++it;
            }

            if (eulasRequested == 0)
            {
                emit finishedOptionalEulas();
            }
        }
    }

    void ContentInstallFlowDiP::onReceivedOptionalEula()
    {
        for (OptComponentInfoList::const_iterator it =  mManifest->GetOptionalCompInfo(currentLanguage()).begin();
            it != mManifest->GetOptionalCompInfo(currentLanguage()).end(); ++it)
        {
            if (!mPendingTransferFiles.contains(it->mEulaFileName))
            {
                return;
            }
        }

        emit finishedOptionalEulas();
    }

    void ContentInstallFlowDiP::getEulas()
    {
        if (!mProtocol)
        {
            emit finishedEulas();
        }
        else if (!receivedAllEulas())
        {
            const EulaInfoList& eulas = mManifest->GetEulas(currentLanguage());
            if (!mPendingTransferFiles.contains(eulas.front().mFileName)) // Have the eulas already been requested
            {
                if (makeOnlineCheck("User is offline. Unable to retrieve eula"))
                {
                    for (EulaInfoList::const_iterator i = eulas.begin(); i != eulas.end(); i++)
                    {
                        EulaInfo cur = *i;

                        mPendingTransferFiles.insert(cur.mFileName, "");

                        mTemporaryFiles.push_back(ContentServices::GetTempFilename(mContent->contentConfiguration()->productId(), "eul"));
                        if (mTemporaryFiles.back().isEmpty())
                        {
                            ORIGIN_LOG_ERROR << "Unable to get temp filename. OfferID: " << mContent->contentConfiguration()->productId();
                        }

                        mProtocol->GetSingleFile(cur.mFileName, mTemporaryFiles.back());
                    }
                }
                // else the flow will get suspended in IContentInstallFlow::onConnectionStateChanged
            }
        }
        else
        {
            const EulaInfoList& eulas = mManifest->GetEulas(currentLanguage());
            for (EulaInfoList::const_iterator i = eulas.begin(); i != eulas.end(); i++)
            {
                EulaInfo cur = *i;
                Q_ASSERT(mPendingTransferFiles.contains(cur.mFileName) && mPendingTransferFiles[cur.mFileName].length() > 0);

                quint32 fileSignature = 0;
                if (!mProtocol->IsFileInJob(cur.mFileName, fileSignature))
                {
                    continue;
                }

                QString eulaStrName = getCleanEulaID(cur.mFileName);
                quint32 eulaSignature = (quint32)mInstallManifest.Get64BitInt(eulaStrName);

                if (eulaSignature == fileSignature)
                {
                    ORIGIN_LOG_EVENT << "Skipping already accepted EULA: " << cur.mFileName << " (" << eulaSignature << ")";
                    continue;
                }

                // The new EULA dialog can read in anything. This tells us whether 
                // or not we should use our old RTF dialog. By the time it gets to the
                // UI layer the file is a temp file.
                const bool isRtfFile = cur.mFileName.contains(".rtf");
                Eula eula(cur.mName, mPendingTransferFiles[cur.mFileName], isRtfFile, false);
                eula.IsOptional = false;
                eula.signature = fileSignature;
                eula.originalFilename = cur.mFileName;
                mEulaStateRequest.eulas.listOfEulas.push_back(eula);
            }

            emit finishedEulas();
        }
    }

    bool ContentInstallFlowDiP::receivedAllEulas()
    {
        const EulaInfoList& eulas = mManifest->GetEulas(currentLanguage());
        for (EulaInfoList::const_iterator i = eulas.begin(); i != eulas.end(); i++)
        {
            EulaInfo cur = *i;
            QMap<QString, QString>::const_iterator j = mPendingTransferFiles.find(cur.mFileName);
            if (j == mPendingTransferFiles.constEnd() || j.value().isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    void ContentInstallFlowDiP::processDipManifest()
    {
        QString dipManifestPath(mContent->contentConfiguration()->dipManifestPath());

        Q_ASSERT(mPendingTransferFiles.contains(dipManifestPath));

        QString manifestPath = mPendingTransferFiles[dipManifestPath];

        Q_ASSERT(manifestPath.length() != 0);

        mManifest = new DiPManifest();
        if (!mManifest->Load(manifestPath, mContent->contentConfiguration()->isPDLC()))
        {
            ORIGIN_LOG_ERROR << "Unable to parse DiP Manifest: " << manifestPath << " OfferID: " << mContent->contentConfiguration()->productId();

            // telemetry data
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::DiPManifestParseFailure;
            errorInfo.mErrorCode = 0;
            sendCriticalErrorTelemetry(errorInfo);

            // user error dialog
            QString errorMsg(ErrorTranslator::Translate((ContentDownloadError::type)FlowError::DiPManifestParseFailure));
            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorMsg, mContent->contentConfiguration()->productId());

            return;
        }

        mInstallArgumentRequest.showShortCut = mManifest->GetShortcutsEnabled();
        mInstallArgumentRequest.optionalEulas.isConsolidate = mManifest->GetConsolidatedEULAsEnabled();

        // Start with a list of support languages from installdata.xml
        mInstallLanguageRequest.availableLanguages = mContent->localContent()->availableLocales(mManifest->GetSupportedLocales(), mManifest->GetContentIDs(), mManifest->GetManifestVersion().ToStr());
        mInstallLanguageRequest.productId = mContent->contentConfiguration()->productId();

        // Get the locales supported by the content being installed
        QStringList offerLocales = mContent->contentConfiguration()->supportedLocales();


        // Remove unsupported languages, as defined by the entitlement data, for grey market support
        if (mContent->contentConfiguration()->supportsGreyMarket(mManifest->GetManifestVersion().ToStr()))
        {
            // Make sure we have at least one left!
            if (mInstallLanguageRequest.availableLanguages.size() == 0)
            {
                // telemetry data
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = FlowError::LanguageSelectionEmpty;
                errorInfo.mErrorCode = 0;
                sendCriticalErrorTelemetry(errorInfo);

                // client log
                ORIGIN_LOG_ERROR << "Locales: Manifest = (" << mManifest->GetSupportedLocales().join(",") << ") Offer = (" << offerLocales.join(",") << ")";

                // user error dialog
                QString errorMsg(ErrorTranslator::Translate((ContentDownloadError::type)FlowError::LanguageSelectionEmpty));
                emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorMsg, mContent->contentConfiguration()->productId());

                sendGreyMarketLanguageSelectionError(
                    mContent->contentConfiguration()->productId(),
                    offerLocales,
                    mManifest->GetSupportedLocales());

                return;
            }
        }

        else
        {

            // Keep track of content that bypasses grey market
            sendGreyMarketBypassFilterTelemetry(
                mContent->contentConfiguration()->productId(),
                offerLocales,
                mManifest->GetSupportedLocales());
        }

        // Now we have to note which remaining languages require an additional download if we are ITO
        if(isUsingLocalSource())
        {
            foreach(const QString& lang, mInstallLanguageRequest.availableLanguages)
            {
                if(mManifest->CheckIfLocaleRequiresAdditionalDownload(lang))
                {
                    mInstallLanguageRequest.additionalDownloadRequiredLanguages.push_back(lang);
                }
            }
        }

        mEulaStateRequest.eulas.isConsolidate = mManifest->GetConsolidatedEULAsEnabled();

        emit processedManifest();
    }

    void ContentInstallFlowDiP::checkPrerequisites()
    {
        VersionInfo clientVersion(Origin::Services::PlatformService::currentClientVersion());
        VersionInfo requiredClientVersion(mManifest->GetRequiredClientVersion());

        // If the client version does not support this content
        if (requiredClientVersion != VersionInfo(0, 0, 0, 0) && clientVersion < requiredClientVersion)
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::ClientVersionRequirementNotMet;

            // Encode the required/current client versions
            errorInfo.mErrorCode = (requiredClientVersion.GetMajor() << 28)
                                     | (requiredClientVersion.GetMinor() << 24)
                                     | (requiredClientVersion.GetBuild() << 20)
                                     | (requiredClientVersion.GetRev() << 16)
                                     | (clientVersion.GetMajor() << 12)
                                     | (clientVersion.GetMinor() << 8)
                                     | (clientVersion.GetBuild() << 4)
                                     | (clientVersion.GetRev() << 0);

            sendCriticalErrorTelemetry(errorInfo);

            QString msg = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, msg, mContent->contentConfiguration()->productId());
        }

        VersionInfo localOSVersion(EnvUtils::GetOSVersion());
        VersionInfo requiredOSVersion(mManifest->GetMinimumRequiredOS());
        if (localOSVersion < requiredOSVersion)
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::OSRequirementNotMet;

#if defined(ORIGIN_MAC)
            errorInfo.mErrorCode = (requiredOSVersion.GetMajor() << 28)
                                     | (requiredOSVersion.GetMinor() << 24)
                                     | (requiredOSVersion.GetBuild() << 20)
                                     | (requiredOSVersion.GetRev() << 16)
                                     | (localOSVersion.GetMajor() << 12)
                                     | (localOSVersion.GetMinor() << 8)
                                     | (localOSVersion.GetBuild() << 4)
                                     | (localOSVersion.GetRev() << 0);
#else
            // encode the current os in the error code to be sent by the telemetry hook
            errorInfo.mErrorCode = (localOSVersion.GetMajor() << 24)
                                     | (localOSVersion.GetMinor() << 16)
                                     | (localOSVersion.GetBuild() <<  8)
                                     | (localOSVersion.GetRev());
#endif
            sendCriticalErrorTelemetry(errorInfo);

            // encode the required os in the error code to be displaied to the user
            errorInfo.mErrorCode = (requiredOSVersion.GetMajor() << 24)
                                     | (requiredOSVersion.GetMinor() << 16)
                                     | (requiredOSVersion.GetBuild() <<  8)
                                     | (requiredOSVersion.GetRev());

            // DISABLED VERSION CHECK ON MAC FOR THE TIME BEING
#if defined(ORIGIN_MAC)
            if (Origin::Services::PlatformService::isSimCityProductId(mContent->contentConfiguration()->productId()))
            {
                // Allow SimCity to always download
                emit prequisitesMet();
            }
            else
#endif
            {
            QString msg = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, msg, mContent->contentConfiguration()->productId());
        }
        }
        else
        {
            emit prequisitesMet();
        }
    }

    // this is to address the race condition where onInstallInfo() is reached before the InstallerFlow is created which orphans the
    // pendingInstallArguments() signal and gets the installFlow state machine stuck.  So this call was added to make sure the InstallerFlow
    // is created when we emit pendingInstallArguments().  It is called from InstallerFlow::start() after the connection for the signal is set up.
    void ContentInstallFlowDiP::onInstallerFlowCreated()
    {
        if (mWaitingForInstallerFlow && getFlowState() == ContentInstallFlowState::kPendingInstallInfo)
        {
            emit pendingInstallArguments(mInstallArgumentRequest);
        }

        mWaitingForInstallerFlow = false;
        mInstallerFlowCreated = true;
    }

    void ContentInstallFlowDiP::onPendingContentExit()
    {
        emit pendingContentExit();
    }

    void ContentInstallFlowDiP::onInstallInfo()
    {
        if (mIsUpdate)
        {
            emit installArgumentsSet();
        }
        else
        {
            if (contentIsPdlc())
            {
                mInstallArgumentRequest.isPdlc = true;
            }
            else
            {
                mInstallArgumentRequest.isITO = isITO();
                if (mInstallArgumentRequest.isITO)
                    mInstallArgumentRequest.useDefaultInstallOptions = mUseDefaultInstallOptions;
                else
                    mInstallArgumentRequest.useDefaultInstallOptions = false;

                mInstallArgumentRequest.isLocalSource = isUsingLocalSource();
                mInstallArgumentRequest.isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
            }

            mInstallArgumentRequest.installPath = getDownloadLocation();

            if (mInstallerFlowCreated)
                emit pendingInstallArguments(mInstallArgumentRequest);
            else
                mWaitingForInstallerFlow = true;    // we couldn't emit pendingInstallArguments() yet because the installer flow isn't ready
        }
    }

	/*
	This function determines if the user needs to be asked, or not, which language to use in installing this content.
	If not, then it also determines which language will be used.
	*/
    void ContentInstallFlowDiP::onEulaLanguageSelection()
    {
        bool unknownLocale = false;
#ifdef ORIGIN_PC
        // if there is an unknown locale in the list, we MUST ask the user to pick one (aka Turkish support)
		// .. unless they were already asked!
        if (OriginApplication::mCustomerSupportLocales.isEmpty())
        {   // for some reason, this was not initialize yet...
            QString unused;
            OriginApplication::instance().GetEbisuCustomerSupportInBrowser(unused);
        }
        for(QStringList::iterator it = mInstallLanguageRequest.availableLanguages.begin(); it != mInstallLanguageRequest.availableLanguages.end(); ++it)
        {
            if (!OriginApplication::mCustomerSupportLocales.contains(*it))
            {
                unknownLocale = true;
                break;
            }
        }
#endif

        // Determine if we are currently doing a non-DiP install
        bool updatingNonDipGame = isNonDipInstall();
        
        // Make sure we have a valid content configuration first
        if (getContent() && getContent()->contentConfiguration())
        {
            const Origin::Engine::Content::ContentConfigurationRef contentConfiguration = getContent()->contentConfiguration();

            // Set the display name
            mInstallLanguageRequest.displayName = contentConfiguration->displayName();

            // If we're updating a non-DiP game, we should show a warning to make sure users select the same locale that they were previously using
            if (updatingNonDipGame)
            {
                mInstallLanguageRequest.showPreviousInstallLanguageWarning = true;
            }
        }

        // Auto select for the user these languages in order of priority
        // 1) command line (in case we already asked via an installer)
        QString useThisLocale = mEulaLanguageResponse.selectedLanguage;         // HACK: this should really be not reused to transmit the command line info
		bool alreadyAskedUser = (!useThisLocale.isEmpty() && !updatingNonDipGame); // If we are updating a non-DiP game, we haven't already asked the user
        // 2) previously installed language (for updates, repairs, etc.)
        // If this is a full reinstall, and not a repair or update, we don't
        // rely on the previous language because the user might want to
        // reinstall in a different language.
        if (useThisLocale.isEmpty() && (mIsUpdate || mRepair))
            useThisLocale = getContent()->localContent()->installedLocale();
        // 3) Use parent locale (EBIBUGS-27685)
        if (!mContent->parent().isNull() && mContent->parent()->localContent())
            useThisLocale = mContent->parent()->localContent()->installedLocale();
        // 4) client locale
        if (useThisLocale.isEmpty())
            useThisLocale = Origin::Engine::LoginController::instance()->currentUser()->locale();

        // make sure our pre chosen locale is supported!
        bool isSupported = mInstallLanguageRequest.availableLanguages.contains(useThisLocale);
		bool askUser = true;
        
        bool isITOUsingLocalSource = isUsingLocalSource() && isITO();

		if (mIsUpdate && !updatingNonDipGame) // or repair? (assumes previoulsy installed locale is supported!)
			askUser = false;
		if (alreadyAskedUser && isSupported) // don't ask twice if the first choice is valid
			askUser = false;
		if (isSupported && !isITOUsingLocalSource && !unknownLocale) // only downloads (Not ITO from disc) gets to bypass the selection, if we match the client locale (unless we have Turkish)
			askUser = false;

        // suppress eula language dialog for ITO DLC (OFM- )
        if (contentIsPdlc() && isITOUsingLocalSource && !unknownLocale)
            askUser = false;

		ORIGIN_LOG_EVENT << "onEulaLanguageSelection: locale:" << useThisLocale << " askUser:" << askUser;

        // ITO disc base game install should always prompt the user for a language
        // but updates use the language already chosen for the base game.
		// mEulaStateRequest.isLocalSource is not set until AFTER this call!
        if (askUser)
        {
            // This will ask the user for a filtered locale choice
            emit pendingInstallLanguage(mInstallLanguageRequest);
        }
		else
        {
            // This log entry can be removed later - CM (but keep for debugging)
            //ORIGIN_LOG_EVENT << "Locales: Manifest = (" << mManifest->GetSupportedLocales().join(",") << ") Offer = (" << offerLocales.join(",") << ") - " << mEulaLanguageResponse.selectedLanguage;

            // locale is already chosen for the user - no user input required
			Origin::Downloader::EulaLanguageResponse response;
			response.selectedLanguage = useThisLocale;
			setEulaLanguage(response);

            // Send Grey Market "language selection" event
            sendGreyMarketLanguageSelectionTelemetry(
                mContent->contentConfiguration()->supportsGreyMarket(mManifest->GetManifestVersion().ToStr()),
                mContent->contentConfiguration()->productId(),
                useThisLocale,
                mInstallLanguageRequest.availableLanguages,
                mManifest->GetSupportedLocales());
        }
    }

    void ContentInstallFlowDiP::onPendingUpdateEulaState()
    {
        if (mSuppressDialogs)
        {
            // We don't want to show EULAs, we'll do this at the end of the download
            mEulaStateRequest.eulas.listOfEulas.clear();
        }

        if (mEulaStateRequest.eulas.listOfEulas.isEmpty())
        {
            // No EULAs for this update, just skip ahead
            emit pendingEulaComplete();
        }
        else
        {
            // Call the usual slot
            onPendingEulaState();
        }
    }

    void ContentInstallFlowDiP::onPendingEulaState()
    {
        // If the EULAs accepted flag has already been set (e.g. for ITO)
        if (!mIsUpdate && mEulasAccepted)
        {
            // Mark that we have accepted each EULA
            for (QList<Origin::Downloader::Eula>::Iterator it = mEulaStateRequest.eulas.listOfEulas.begin(); it != mEulaStateRequest.eulas.listOfEulas.end(); it++)
            {
                Origin::Downloader::Eula curEula = *it;

                QString eulaStrName = getCleanEulaID(curEula.originalFilename);

                mInstallManifest.Set64BitInt(eulaStrName, (qint64)curEula.signature);
            }

            mInstallManifest.SetBool("eulasaccepted", true);
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // Clear the EULAs
            mEulaStateRequest.eulas.listOfEulas.clear();
        }
        
        if (mEulaStateRequest.eulas.listOfEulas.isEmpty())
        {
			if (mRepair || mIsUpdate)
				startProtocol();
			else
				emit pendingEulaComplete();	// transition to kEnqueued state
        }
        else
        {
            mEulaStateRequest.isLocalSource =  isUsingLocalSource();
            mEulaStateRequest.isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
            mEulaStateRequest.productId = mContent->contentConfiguration()->productId();
            emit pendingEulaState(mEulaStateRequest);
        }
    }

	void ContentInstallFlowDiP::onEnqueued()	// this should only be called when an entitlement is first put on the queue not when restoring a queue (after shutdown/logout)
	{
		// put on queue
		bool put_on_top = mIsUpdate || isRepairing() || isITO() || mTryToMoveToTop;
        if (put_on_top)
        {
			// must protect against user logging out which results in a null queueController
			Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
			if (queueController)
            {
                if (mIsUpdate && (queueController->head() == mContent))
                    startTransferFromQueue();   // if we are an update and we are already in the head position in queue, then we are updating an ITO install so just go
                else
                    queueController->enqueueAndTryToPushToTop(mContent);

                if (!(isRepairing() || mIsUpdate))   // auto download DLC if this is a fresh install
                {
                    QSet<QString> autoStartedPDLC;
                    bool permissionsRequested = true; // This is a throwaway value that basically says "we don't need to ask for UAC upfront since we were already downloading"
                    bool uacFailed = false;
                    mContent->localContent()->downloadAllPDLC(autoStartedPDLC, permissionsRequested, uacFailed);
                }
                else
                if (isRepairing())
                {
                    mContent->localContent()->repairAllPDLC();
                }
            }
        }
        else
        {
			// must protect against user logging out which results in a null queueController
			Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
			if (queueController)
            {
                queueController->enqueue(mContent,false);

                QSet<QString> autoStartedPDLC;
                bool permissionsRequested = true; // This is a throwaway value that basically says "we don't need to ask for UAC upfront since we were already downloading"
                bool uacFailed = false;
                mContent->localContent()->downloadAllPDLC(autoStartedPDLC, permissionsRequested, uacFailed);
            }
        }
	}

    void ContentInstallFlowDiP::onProcessExclusionsAndDeletes()
    {
        if(mProtocol == NULL)
        {
            emit processedExclusionsAndDeletes();
            return;
        }
        if (mManifest->GetLanguageExcludesEnabled())
        {
            mProtocol->SetExcludes(mManifest->GetWildcardExcludes(currentLanguage()));
        }
        if (mManifest->GetLanguageIncludesEnabled())
        {
            mProtocol->SetAutoExcludesFromIncludeList(mManifest->GetAllLanguageIncludes(), mManifest->GetWildcardIncludes(currentLanguage()));
        }
        
        // DiP 2.0 has files that can be ignored during updates
        if (mIsUpdate && mManifest->GetIgnoresEnabled())
        {
            mProtocol->SetExcludes(mManifest->GetWildcardIgnores());
        }

        if (mManifest->GetDeletesEnabled())
        {
            mProtocol->SetDeletes(mManifest->GetWildcardDeletes());
        }

        bool catalogDiffUpdateEnable = true;
        if (mContent->contentConfiguration()->softwareBuildMetadataPresent())
        {
            catalogDiffUpdateEnable = mContent->contentConfiguration()->softwareBuildMetadata().mbEnableDifferentialUpdate;

            if (!catalogDiffUpdateEnable && mManifest->GetEnableDifferentialUpdate())
            {
                ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Differential Updating, but overridden to false by catalog SoftwareBuildMetadata.";
            }
        }

        bool downloaderSafeMode = (Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloaderSafeMode) || mUseDownloaderSafeMode);
        if (downloaderSafeMode && mManifest->GetEnableDifferentialUpdate())
        {
            ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Differential Updating, but overridden to false by Downloader Safe Mode.";
        }

        if (mRepair && mManifest->GetEnableDifferentialUpdate())
        {
            ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Differential Updating, but disabled due to repair.";
        }

        bool usedSubFileDiffPatching = false;
        if (mManifest->GetEnableDifferentialUpdate() && catalogDiffUpdateEnable && !downloaderSafeMode && !mRepair)
        {
            if (isUpdate())
            {
                usedSubFileDiffPatching = true;
            }
            mProtocol->SetDifferentialFlag(true, mSyncPackageUrl);
            mProtocol->SetDiffExcludes(mManifest->GetWildcardExcludeFromDifferential());
        }

        // If PI is enabled and we're not updating or repairing
        bool catalogDDEnable = true;
        if (mContent->contentConfiguration()->softwareBuildMetadataPresent())
        {
            catalogDDEnable = mContent->contentConfiguration()->softwareBuildMetadata().mbDynamicContentSupportEnabled;

            if (!catalogDDEnable && mManifest->GetProgressiveInstallationEnabled())
            {
                ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Dynamic Download, but overridden to false by catalog SoftwareBuildMetadata.";
            }
        }

        bool eaCoreDDEnable = Origin::Services::readSetting(Origin::Services::SETTING_EnableProgressiveInstall, Origin::Services::Session::SessionRef());
        if (!eaCoreDDEnable && mManifest->GetProgressiveInstallationEnabled())
        {
            ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Dynamic Download, but overridden to false by EACore.ini.";
        }

        if (isITO() && mManifest->GetProgressiveInstallationEnabled())
        {
            ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Dynamic Download, but not supported for disc-based installation.";
        }

        if (usedSubFileDiffPatching && mManifest->GetProgressiveInstallationEnabled())
        {
            ORIGIN_LOG_WARNING << "Content " << mContent->contentConfiguration()->productId() << " supports Dynamic Download, but not supported alongside differential updates.  Using differential updating instead.";
        }

        if (mManifest->GetProgressiveInstallationEnabled() && catalogDDEnable && eaCoreDDEnable && !usedSubFileDiffPatching && !isITO() && ((getFlowState() != ContentInstallFlowState::kResuming) || mInstallManifest.GetBool("DynamicDownload")))
        {
            ORIGIN_LOG_EVENT << "Processing content " << mContent->contentConfiguration()->productId() << " as a Dynamic Download.";

            // Enable dynamic downloading for the protocol
            mProtocol->SetDynamicDownloadFlag(true);

            mIsDynamicDownload = true;
            mInstallManifest.SetBool("DynamicDownload", true);

            mChunkState.clear();

            // Clear this out
            mDynamicRequiredPortionSize = 0;
            mInstallManifest.Set64BitInt("DDRequiredPortion", mDynamicRequiredPortionSize);

            bool clearExistingData = !(mContent->localContent()->installed() && mInstallManifest.GetBool("DDinstallAlreadyCompleted"));

            // TODO: Find a more appropriate place for this
            // Tell the Dynamic Download Controller that a Dynamic Download is beginning or resuming
            DynamicDownloadController::GetInstance()->RegisterProtocol(mContent->contentConfiguration()->productId(), getDownloadLocation(), mProtocol, clearExistingData);

            // Define the chunks one-by-one
            Origin::Downloader::tProgressiveInstallChunks chunks = mManifest->GetProgressiveInstallerChunks();
            for (Origin::Downloader::tProgressiveInstallChunks::iterator chunkIter = chunks.begin(); chunkIter != chunks.end(); ++chunkIter)
            {
                int priorityGroupId = chunkIter.key();

                // Track our own metadata
                DynamicDownloadChunkState& chunkState = mChunkState[priorityGroupId];
                chunkState.chunkId = priorityGroupId;
                chunkState.state = DynamicDownloadChunkState::kSubmitted;
                chunkState.requirement = chunkIter.value().GetChunkRequirement();

                mProtocol->SetPriorityGroup(priorityGroupId, chunkIter.value().GetChunkRequirement(), chunkIter.value().GetChunkName(), QStringList(chunkIter.value().GetFilePatterns().toList()));
            }

#ifdef _DEBUG
            if (DynamicDownloadController::GetInstance())
            {
                // Invoke this on the queue so that the DDC has a chance to receive all the data first
                QMetaObject::invokeMethod(DynamicDownloadController::GetInstance(), "DumpDebug", Qt::QueuedConnection, Q_ARG(QString, mContent->contentConfiguration()->productId()), Q_ARG(bool, true));
            }
#endif
        }
        else
        {
            ORIGIN_LOG_EVENT << "Processing content " << mContent->contentConfiguration()->productId() << " as a normal DiP download.  (Clearing any DDC state, if it exists)";

            // If we already previously had any cached dynamic download data
            if (DynamicDownloadController::GetInstance()->IsProgressiveInstallationAvailable(mContent->contentConfiguration()->productId()))
            {
                // Clear any cached dynamic download data for this offer since we are doing a non-dynamic operation on it!
                DynamicDownloadController::GetInstance()->ClearDynamicDownloadData(mContent->contentConfiguration()->productId());
            }

            mIsDynamicDownload = false;
            mInstallManifest.SetBool("DynamicDownload", false);

            mChunkState.clear();
        }

        emit processedExclusionsAndDeletes();
    }

    void ContentInstallFlowDiP::onPendingDiscChange ()
    {

    }

    void ContentInstallFlowDiP::onTransferring()
    {
        mInstallManifest.SetBool("downloading", true);
        mInstallManifest.SetBool("paused", false);

        if (mProtocol)
        {
            // Turn off the safe cancel flag once we've made it to the transferring state
            mProtocol->SetSafeCancelFlag(false);
        }

        bool runInstaller = false;
        if (mIsDynamicDownload)
        {
            if (!isUpdate())
            {
                // Set a flag in the manifest indicating that this is the initial installation, so if the user pauses even after the installer runs
                // we don't treat it as an update
                mInstallManifest.SetBool("DDInitialDownload", true);
            }

            // For updates where no required pieces changed, run the touchup immediately
            if (isUpdate() && checkIfRequiredChunksInstalled() && !mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
            {
                runInstaller = true;
            }
        }

        // Save out the autoresume flag before we save the manifest, this ensures if we crash or get killed, the download will auto-resume
        // This will get un-set during the course of normal operation.  We only do this for the transferring state.
        bool autoresume = mInstallManifest.GetBool("autoresume");
        if (!autoresume) 
            mInstallManifest.SetBool("autoresume", true);

        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        // Restore the autoresume flag to whatever it was previously, so we don't override the normal behavior
        mInstallManifest.SetBool("autoresume", autoresume);

        if (runInstaller)
        {
            ORIGIN_LOG_EVENT << "All required chunks already complete; Installing";

            // This transitions us directly to running the installer (even though the chunks are already installed, we'll call them ready to install)
            emit dynamicDownloadRequiredChunksReadyToInstall();
        }
    }

    void ContentInstallFlowDiP::startProtocol()
    {
        if (makeOnlineCheck("User is offline. Unable to start protocol"))
        {
			if (mProtocol)
            {
				mProtocol->Start();
            }
			else
			{
				ORIGIN_LOG_ERROR << "mProtocol->Start() called when mProtocol = NULL, content = " << mContent->contentConfiguration()->productId();
			}
        }
        // else the flow will get suspended in IContentInstallFlow::onConnectionStateChanged
    }

    QString ContentInstallFlowDiP::getCleanEulaID(const QString& originalFilename)
    {
        QString eulaStrName = QString("eula_%1").arg(originalFilename);
        eulaStrName.replace("/", "_");
        eulaStrName.replace("-", "_");
        eulaStrName.replace(".", "_");

        return eulaStrName;
    }

    void ContentInstallFlowDiP::setEulaState(const EulaStateResponse& response)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::EulaStateResponse, response))

        mEulaStateResponse = response;

        for (QList<Origin::Downloader::Eula>::Iterator it = mEulaStateRequest.eulas.listOfEulas.begin(); it != mEulaStateRequest.eulas.listOfEulas.end(); it++)
        {
            Origin::Downloader::Eula curEula = *it;

            QString eulaStrName = getCleanEulaID(curEula.originalFilename);
            
            mInstallManifest.Set64BitInt(eulaStrName, (qint64)curEula.signature);
        }

        // Clear the EULA list
        mEulaStateRequest.eulas.listOfEulas.clear();

        mInstallManifest.SetBool("eulasaccepted", true);
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        if (!mIsUpdate || !mUpdateFinalizationStateMachine)
        {
			emit pendingEulaComplete();	// transition to kEnqueued state
        }
        else
        {
            emit eulaStateSet();
        }
    }

    void ContentInstallFlowDiP::setInstallArguments(const InstallArgumentResponse& args)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::InstallArgumentResponse, args));

        if (!contentIsPdlc() && !mIsUpdate)
        {
            mInstallArgumentResponse = args;
            mInstallManifest.SetBool("installDesktopShortCut", args.installDesktopShortCut);
            mInstallManifest.SetBool("installStartMenuShortCut", args.installStartMenuShortCut);
            mInstallManifest.Set64BitInt("optionalComponentsToInstall", static_cast<qint64>(args.optionalComponentsToInstall));

            QList<Eula>::const_iterator eula = mInstallArgumentRequest.optionalEulas.listOfEulas.constBegin();
            while (eula != mInstallArgumentRequest.optionalEulas.listOfEulas.constEnd())
            {
                if (mInstallArgumentResponse.optionalComponentsToInstall & 1)
                {
                    mEulaStateRequest.eulas.listOfEulas.push_back(*eula);
                }
                ++eula;
            }

            // When using ITO or download when there isn't enough disc space, the user has the opportunity to re-set the DiP install path during the process, so re-set it here, in case it may have changed.
            // Only save the folder for base game, because all other content is relative to parent content.
            // This is because DLC/etc flows are initialized potentially before the base game is installed, and we wouldn't know the
            // proper path to use yet.
            if (!contentIsPdlc())
            {
                mInstallManifest.SetString("dipInstallPath", mContent->localContent()->dipInstallPath());
            }
            if (mProtocol)
            {
                mProtocol->SetUnpackDirectory(getDownloadLocation());
            }
        }

        emit installArgumentsSet();
    }

    void ContentInstallFlowDiP::setEulaLanguage(const EulaLanguageResponse& response)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::EulaLanguageResponse, response))

        mEulaLanguageResponse = response;
        if (mEulaLanguageResponse.selectedLanguage.size() == 0)
        {
            ORIGIN_LOG_ERROR << "Setting mEulaLanguageResponse.selectedLanguage to an empty string from setEulaLanguage()";
        }

        mInstallManifest.SetString("locale", response.selectedLanguage);

        // We also need to set the LocalContent locale so this info gets saved.
        getContent()->localContent()->setInstalledLocale(response.selectedLanguage);

        StartPreTransferMachine(getState(ContentInstallFlowState::kPendingEulaLangSelection), getState(ContentInstallFlowState::kPendingInstallInfo));

        // Send Grey Market "language selection" event
        sendGreyMarketLanguageSelectionTelemetry(
            mManifest ? mContent->contentConfiguration()->supportsGreyMarket(mManifest->GetManifestVersion().ToStr()) : false,
            mContent->contentConfiguration()->productId(),
            response.selectedLanguage,
            mInstallLanguageRequest.availableLanguages,
            mManifest ? mManifest->GetSupportedLocales() : QStringList());
    }

    void ContentInstallFlowDiP::receiveFileData(const QString& archivePath, const QString& diskPath)
    {
        mPendingTransferFiles[archivePath] = diskPath;

        emit receivedFile(diskPath);
    }

    void ContentInstallFlowDiP::protocolTransferProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimateSecondsRemaining)
    {

        qint64 bytesDownloadedSinceLastSave = bytesDownloaded - mInstallManifest.Get64BitInt("savedBytes");

        // Periodically save out the install manifest with autoresume = true, in case we get killed
        if (bytesDownloadedSinceLastSave > 5242880)
        {
            mInstallManifest.Set64BitInt("savedbytes", bytesDownloaded);

            // Save out the autoresume flag before we save the manifest, this ensures if we crash or get killed, the download will auto-resume
            // This will get un-set during the course of normal operation.  We only do this for the transferring state.
            bool autoresume = mInstallManifest.GetBool("autoresume");
            if (!autoresume) 
                mInstallManifest.SetBool("autoresume", true);

            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // Restore the autoresume flag to whatever it was previously, so we don't override the normal behavior
            mInstallManifest.SetBool("autoresume", autoresume);
        }

        // Only send transfer progress updates during the transferring state
        // This avoids sending duplicate progress events during dynamic downloads, where we can be installing while transferring
        if (getFlowState() == Origin::Downloader::ContentInstallFlowState::kTransferring)
        {
            onProgressUpdate(bytesDownloaded, totalBytes, bytesPerSecond, estimateSecondsRemaining);
        }
    }

    void ContentInstallFlowDiP::onInstall()
    {
        mInstallProgressSignalsConnected = false;

        mInstallManifest.SetBool("downloading", false);
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        QString installerExecutable = getTouchupInstallerExecutable();

        // Installer can be skipped for simple updates if the installer executable hasn't changed 
        // AND there's no "forcetouchupinstallerafterupdate" flag in the DiP Manifest.
        bool bRunInstaller = false;

        // If we are dynamic downloading and we already ran the installer, we'll skip it here
        if (mIsDynamicDownload && mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
        {
            if (mContent->localContent()->installed(true))
            {
                bRunInstaller = false;
                ORIGIN_LOG_EVENT << "Skipping touchup installer because we already ran it earlier in the download.";
            }
            else 
            {
                bRunInstaller = true;
                ORIGIN_LOG_WARNING << "Re-running touchup installer because the content is still not installed.  (Touchup already ran previously)";
            }
        }
        else if (mIsUpdate)
        {
            // For a repair ALWAYS run
            if (mRepair)
            {
                bRunInstaller = true;
            }
            else
            {
                // For non-repair updates run the installer if
                // 1) we've recorded in the installer manifest that the installer had changed
                bRunInstaller = mInstallManifest.GetBool("installerChanged");
            
                // 2) the DiP Manifest has the force flag set
                bRunInstaller |= (mManifest && mManifest->GetForceTouchupInstallerAfterUpdate());

                // However, if no files had changed, skip the installer
                if (mInstallManifest.Get64BitInt("stagedFileCount") == 0)
                    bRunInstaller = false;
            }
        }
        else
        {
            // Not an update, always run installer
            bRunInstaller = true;
        }
        
        // We also do not want to run the touch-up installer for plug-ins since they live in a
        // certain build's runtime dir.
        // TODO: Update EAI to simply not write this field for plug-ins.
        if (mContent->contentConfiguration()->isPlugin())
        {
            bRunInstaller = false;
        }

        if ( bRunInstaller && (!installerExecutable.isEmpty()))
        {
            // TELEMETRY: A game has started installing.
            GetTelemetryInterface()->Metric_GAME_INSTALL_START(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData());

            // In case we were at "Ready To Install" on the Completed queue (due to installer error)
			// must protect against user logging out which results in a null queueController
			Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
			if (queueController)
            {
                queueController->enqueueAndTryToPushToTop(mContent);
            }

#ifdef ORIGIN_PC
            // There is no registry on the Mac.  Content data will be saved to a plist in the game bundle after installation.  See ContentInstallFlowDiP::onComplete().
            bool isHKCU = true;
            ContentServices::setHKeyCurrentUser(mContent, isHKCU);
            ContentServices::ExportStagingKeys();
            ContentServices::setHKeyLocalMachine(mContent, Origin::Engine::LoginController::instance()->currentUser()->locale());
            //if we are not using HKCU as the staging path we must write the staging path reg entry to the staging.reg file
            //first, and have the proxy installer import the reg settings in, otherwise with UAC on we cannot write the entries directly into the registry
            if(!isHKCU)
            {
                ContentServices::setContentHKey(mContent);
            }
#endif
            
            ExecuteInstallerRequest request(installerExecutable, getTouchupInstallerParameters(),
                QFileInfo(installerExecutable).path());

            // start monitoring install progress
            mInstallProgressSignalsConnected = true;
            ORIGIN_VERIFY_CONNECT(mInstallProgress, SIGNAL(Connected()), this, SLOT(onInstallProgressConnected()));
            ORIGIN_VERIFY_CONNECT(mInstallProgress, SIGNAL(ReceivedStatus(float, QString)), this, SLOT(onInstallProgressUpdate(float, QString)));
            ORIGIN_VERIFY_CONNECT(mInstallProgress, SIGNAL(Disconnected()), this, SLOT(onInstallProgressDisconnected()));
            mInstallProgress->StartMonitor();

            ORIGIN_VERIFY_CONNECT(mInstaller, SIGNAL(installFailed(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onInstallerError(Origin::Downloader::DownloadErrorInfo)));
            ORIGIN_VERIFY_CONNECT(mInstaller, SIGNAL(installFinished()), this, SLOT(onInstallFinished()));
            mInstaller->makeInstallRequest(request);
        }
        else
        {
            onInstallFinished();
        }
    }

    void ContentInstallFlowDiP::onInstallFinished()
    {
        // We only want to do this here if we just ran the installer (and so haven't yet set the flag), not if we're looping through again
        // We also need to guard against the fact that the protocol might not exist or not be running (if there were no more non-required data)
        if (mIsDynamicDownload && !mInstallManifest.GetBool("DDinstallAlreadyCompleted") && mProtocol && mProtocol->protocolState() == kContentProtocolRunning)
        {
            // Make sure the protocol releases the non-required chunks to be downloaded
            mProtocol->ActivateNonRequiredDynamicChunks();
        }

        if (mContent->localContent()->installed(true))
        {
            // Mark that we've run the installer
            mInstallManifest.SetBool("DDinstallAlreadyCompleted", true);

            // Save our progress
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // TELEMETRY: A game has been successfully installed.
            GetTelemetryInterface()->Metric_GAME_INSTALL_SUCCESS(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData());

            emit finishedTouchupInstaller();
        }
        else
        {
            ORIGIN_LOG_ERROR << "Touchup installer process reported finished but the game is not ready to play. Content = " << mContent->contentConfiguration()->productId();

            emit pauseInstall();

            emit warn(InstallError::InstallCheckFailed, ErrorTranslator::Translate(
                (ContentDownloadError::type)InstallError::InstallCheckFailed));
        }

    }

    void ContentInstallFlowDiP::onPostTransfer()
    {
        bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);

        // TELEMETRY: Download has completed successfully.
        GetTelemetryInterface()->Metric_DL_SUCCESS(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), isPreload);

        // The download has completed, so reset the counter that keeps track of data corruption-related errors
        mDataCorruptionRetryCount = 0;

        // If we're dynamic downloading, and we have already run the installer
        if (mIsDynamicDownload && mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
        {
            // Skip ahead to the install state (where we'll skip doing anything)
            emit beginInstall();
            return;
        }

        if (mIsUpdate || contentIsPdlc())
        {
            if (!mUpdateFinalizationStateMachine)
            {
                initializeUpdate();
            }
			
			if (mUpdateFinalizationStateMachine->isRunning())
			{
				mUpdateFinalizationStateMachine->stop();	// if it is already running, assume there was an error previously so we need to stop and reset the state machine
	            ORIGIN_LOG_WARNING << "Update state machine already running.  Restarting. Content = " << mContent->contentConfiguration()->productId();
			}
            // EBIBUGS-19793 call queued to ensure that a start() after the stop() above is handled properly
            QMetaObject::invokeMethod(mUpdateFinalizationStateMachine, "start", Qt::QueuedConnection);
        }
        // If we're dynamic downloading
        else if (mIsDynamicDownload)
        {
            if (!checkIfAllChunksInstalled())
            {
                // Install all the chunks we have ready to go
                installAllReadyChunks();
            }
            else
            {
                // Everything is ready, just move to install
                emit beginInstall();
            }
        }
        else
        {
            // This is a regular build, just move to install
            emit beginInstall();
        }
    }

    void ContentInstallFlowDiP::initializeUpdate()
    {
        mUpdateFinalizationStateMachine = new DiPUpdate(this);
        mUpdateFinalizeTransition = getState(ContentInstallFlowState::kPostTransfer)->addTransition(mUpdateFinalizationStateMachine, SIGNAL(finished()), getState(ContentInstallFlowState::kInstalling));
        ORIGIN_VERIFY_CONNECT_EX (this, SIGNAL(canceling()), mUpdateFinalizationStateMachine, SLOT(onCancel()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT (mUpdateFinalizationStateMachine, SIGNAL(pendingUpdatedEulas(const Origin::Downloader::EulaStateRequest&)), this,
            SIGNAL(pendingEulaState(const Origin::Downloader::EulaStateRequest&)));
        ORIGIN_VERIFY_CONNECT_EX (mUpdateFinalizationStateMachine, SIGNAL(finalizeUpdate()), this, SLOT(onUpdateReadyToFinalize()), Qt::QueuedConnection);
        
        // For dynamic downloads, we don't want to pause the protocol since there is still useful work to be done (potentially)
        if (!mIsDynamicDownload)
        {
            ORIGIN_VERIFY_CONNECT(mUpdateFinalizationStateMachine, SIGNAL(pauseUpdate()), this, SIGNAL(protocolPaused()));
        }

        ContentProtocols::RegisterWithInstallFlowThread(mUpdateFinalizationStateMachine);
    }

    void ContentInstallFlowDiP::onInstallerError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        if (mInstallProgressSignalsConnected)
        {
		    // stop monitoring install progress
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Connected()), this, SLOT(onInstallProgressConnected()));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(ReceivedStatus(float, QString)), this, SLOT(onInstallProgressUpdate(float, QString)));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Disconnected()), this, SLOT(onInstallProgressDisconnected()));
		
            // disconnect install session signals
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFailed(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onInstallerError(Origin::Downloader::DownloadErrorInfo)));
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFinished()), this, SLOT(onInstallFinished()));

		    mInstallProgress->StopMonitor();
            mInstallProgressSignalsConnected = false;
        }

        // TELEMETRY: A game has encountered a fatal error during installation.
        GetTelemetryInterface()->Metric_GAME_INSTALL_ERROR(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData(), errorInfo.mErrorType, errorInfo.mErrorCode);

        if (errorInfo.mErrorType == InstallError::InstallerBlocked)
        {
            // Another installer was running at the same time. Try again.
            ORIGIN_LOG_EVENT << "Installer could not start due to another executing installer. Pausing installation.";
            emit pauseInstall();

            emit warn(InstallError::InstallerBlocked, ErrorTranslator::Translate(
                (ContentDownloadError::type)InstallError::InstallerBlocked));
        }
        else if (errorInfo.mErrorType == InstallError::InstallCheckFailed)
        {
            ORIGIN_LOG_EVENT << "Installer reported success but the install check failed for " <<
                mContent->contentConfiguration()->productId() << ". User may have canceled the installer.";
            emit pauseInstall();

            emit warn(InstallError::InstallCheckFailed, ErrorTranslator::Translate(
                (ContentDownloadError::type)InstallError::InstallCheckFailed));
        }
#ifdef ORIGIN_MAC
        else if (errorInfo.mErrorType == InstallError::InstallerExecution)
        {
            // EBIBUGS-25510: This is a hack and should be removed once the root cause is fixed.
            // i.e.,
            // 1) Confirm that EAI preserves the case of the Touchup executable in the installerdata.xml.
            // 2) DGI needs to re-wrap all games that has this case mismatch issue.
            ORIGIN_LOG_WARNING << "Installer failed to start for " << mContent->contentConfiguration()->productId() << ". Do not show an error dialog to user and continue as if the installer succeeded.";
            onInstallFinished();
        }
#endif
        else
        {
            QString errorDescription = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorDescription, mContent->contentConfiguration()->productId());
        }

		emit installDidNotComplete();

		// move to completed so it can be installed later from there by user
		// must protect against user logging out which results in a null queueController
		Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		if (queueController)
        {
            queueController->dequeue(mContent);
        }
    }

    void ContentInstallFlowDiP::onGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField field)
    {
        if (field == Origin::Services::Connection::GLOB_OPR_IS_COMPUTER_AWAKE 
            && Origin::Services::Network::GlobalConnectionStateMonitor::connectionField(field) == Origin::Services::Connection::CS_TRUE)
        {
            switch ( getFlowState() )
            {
            case ContentInstallFlowState::kTransferring:
            case ContentInstallFlowState::kPausing:
                // EBIBUGS-18553: if the computer just came back from sleep, depending on the state of the download, 
                // set the flag that makes the download immediately resume on completed pausing
                ORIGIN_LOG_EVENT << mContent->contentConfiguration()->productId() << " Download in state " << (int) getFlowState() << " when waking up, will resume automatically";
                mResumeOnPaused = true;
                break;
            default:
                // there may be other states that need to be included above, logging here to help determine which ones
                // EBIBUGS-29240: commented out the following log entry, because it was causing massive log spam, and because it's not very important.
                // ORIGIN_LOG_EVENT << mContent->contentConfiguration()->productId() << " Download in state " << (int) getFlowState() << " when waking up, ignored";
                break;
            }
        }
    }

    void ContentInstallFlowDiP::onPaused()
    {
        mInstallManifest.SetBool("paused", true);
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

		emit paused();

        if (mResumeOnPaused)
        {
            mResumeOnPaused = false;
            resume();
        }
        else
        {
            // We aren't immediately resuming (handling an error retry), so we are staying paused.  Reset the error counter so we start fresh next session.
            mDataCorruptionRetryCount = 0;

            mUseDownloaderSafeMode = false; // reset flag
        }
    }


	void ContentInstallFlowDiP::OnPostInstall()
	{
		// If we're dynamic updating, we might have some chunks that aren't yet installed, we should manually finalize because the protocol doesn't do it automatically for updates
		if (mIsDynamicDownload && !mIsDynamicUpdateFinalize && isUpdate() && mProtocol && mProtocol->protocolState() == Origin::Downloader::kContentProtocolFinished)
		{
			mIsDynamicUpdateFinalize = true;

			ORIGIN_LOG_EVENT << "Installing leftover chunks and finalizing protocol.";

			// Tell the protocol to finalize
			mProtocol->Finalize();

			return;
		}

		mIsDynamicUpdateFinalize = false;

		mPendingTransferFiles.clear();        

#ifdef ORIGIN_MAC
        if(!mContent->contentConfiguration()->isPlugin())
        {
            // Create a plist in the game bundle to save content data for EA Access
            ContentServices::CreateAccessPlist (mContent, Origin::Engine::LoginController::instance()->currentUser()->locale(), Services::PlatformService::localDirectoryFromOSPath(mContent->contentConfiguration()->installCheck()));
        }
#endif

        if (mInstallProgressSignalsConnected)
        {
		    // stop monitoring install progress and install session signals
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Connected()), this, SLOT(onInstallProgressConnected()));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(ReceivedStatus(float, QString)), this, SLOT(onInstallProgressUpdate(float, QString)));
		    ORIGIN_VERIFY_DISCONNECT(mInstallProgress, SIGNAL(Disconnected()), this, SLOT(onInstallProgressDisconnected()));
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFailed(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onInstallerError(Origin::Downloader::DownloadErrorInfo)));
		    ORIGIN_VERIFY_DISCONNECT(mInstaller, SIGNAL(installFinished()), this, SLOT(onInstallFinished()));
		    mInstallProgress->StopMonitor();
            mInstallProgressConnected = false;
            mInstallProgressSignalsConnected = false;
        }

		// Destroy the update finalization state machine
		if (mUpdateFinalizationStateMachine)
		{
			getState(ContentInstallFlowState::kPostTransfer)->removeTransition(mUpdateFinalizeTransition);
			mUpdateFinalizeTransition->deleteLater();
			mUpdateFinalizeTransition = NULL;
			mUpdateFinalizationStateMachine->deleteLater();
			mUpdateFinalizationStateMachine = NULL;
		}

		if (mIsDynamicDownload && mProtocol->protocolState() == Origin::Downloader::kContentProtocolRunning)
		{
			ORIGIN_LOG_EVENT << "Protocol still running, returning to transferring state.";

			// Install all chunks that may have completed while we were busy running the installer
			installAllReadyChunks();

			// Resume waiting on the transfer that was already going
			emit resumeExistingTransfer();

			// signal UI that the content is now playable
			emit contentPlayable();
		}
		else
		{
			// Destroy the protocol
			if (mProtocol)
			{
                // Delete the protocol on it's own thread (queue the destructor on the event loop)
				mProtocol->deleteLater();
				mProtocol = NULL;
			}

			// Clear the Dynamic Download flags (even if we might not be a dynamic download, we may need them later)

			// Set a flag in the manifest indicating that this is no longer the initial installation
			mInstallManifest.SetBool("DDInitialDownload", false);

			// Reset the flag which indicates whether we have already run the touchup installer because this job is over
			mInstallManifest.SetBool("DDinstallAlreadyCompleted", false);

			// transition to kCompleted stage 
			emit postInstallToComplete();
		}
	}

    void ContentInstallFlowDiP::onComplete()
    {
        ORIGIN_LOG_EVENT << "Install flow completed for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

		// Reset the main state machine 
		QStateMachine::stop();
		emit finishedState();

        // Clear the download job ID
        clearJobId();
        mInstallManifest.SetString("buildid", "");

		// for non-dynamic downloads
		if (mIsDynamicDownload == false)
        {
            // If this is an ITO we just finished an install and are about to start a mandatory 
            // update - meaning the game won't be playable until after the update is complete
            if(!isITO())
            {
                // signal UI that the content is now playable
                emit contentPlayable();
            }
        }
        else
        {
            // Reset the dynamic download state to false, because the job is complete
            mInstallManifest.SetBool("DynamicDownload", false);
            mIsDynamicDownload = false;
        }
    }

    void ContentInstallFlowDiP::onStopped()
    {
		delete mManifest;
		mManifest = NULL;
        
        if(mDiPManifestFinishedTransition)
        {
            // TODO: StartDiPManifestMachine is currently only called during kPreTransfer, but is set up to potentially be called during any state.
            // We need to make this more elegant.
            getState(ContentInstallFlowState::kPreTransfer)->removeTransition(mDiPManifestFinishedTransition);
            mDiPManifestFinishedTransition->deleteLater();
            mDiPManifestFinishedTransition = NULL;
        }
        
        if(mPreTransferFinishedTransition)
        {
            // TODO: StartPreTransferMachine is currently only called during kPendingEulaLangSelection, but is set up to potentially be called during any state.
            // We need to make this more elegant.
            getState(ContentInstallFlowState::kPendingEulaLangSelection)->removeTransition(mPreTransferFinishedTransition);
            mPreTransferFinishedTransition->deleteLater();
            mPreTransferFinishedTransition = NULL;
        }

        cleanupTemporaryFiles();
        setStartState(ContentInstallFlowState::kReadyToStart);
        QStateMachine::start();
    }

	void ContentInstallFlowDiP::sendDownloadStartTelemetry(const QString& status, qint64 savedBytes)
	{
        // Only send telemetry if this is not an overridden download! We only need to check here because the other download-related 
        // metric calls (i.e. Metric_DL_END, Metric_DL_REDOWNLOAD) will silently fail unless we have called Metric_DL_START first.
        // If the user has an override in EACore.ini but is installing via ITO, or if EACore.ini contains the appropriate lines,
        // we DO want to capture that telemetry.
        const bool enableOverrideTelemetry = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloadOverrideTelemetry, Origin::Services::Session::SessionService::currentSession()).toQVariant().toBool();
        const bool shouldSendTelemetry = !mContent->contentConfiguration()->hasOverride() || isUsingLocalSource() || enableOverrideTelemetry;
        if(shouldSendTelemetry)
        {
            QString cachePath;
            ContentServices::GetContentInstallCachePath(mContent, cachePath, false);
            const bool symlinkDetected = isPathSymlink(cachePath);

            bool autoPatching = mContent->contentController()->autoPatchingEnabled();
            QString typeOfPatch;
            if (mIsUpdate)
            {
                typeOfPatch = "Manual";
                if (autoPatching)
                    typeOfPatch = "Auto";
            }
            else
                typeOfPatch = "NotPatch";

            bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);

            // TELEMETRY: Starting a download.
            GetTelemetryInterface()->Metric_DL_START(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Services::Session::SessionRef()).toString().toUtf8().data(),
                mContent->contentConfiguration()->productId().toLocal8Bit().constData(), 
                status.toLocal8Bit().constData(),
                getContentTypeString().toLocal8Bit().constData(),
                getPackageTypeString().toLocal8Bit().constData(),
                getLogSafeUrl().toLocal8Bit().constData(),
                savedBytes,
                typeOfPatch.toLocal8Bit().constData(),
                getDownloadLocation().toLocal8Bit().constData(),
                symlinkDetected,
                isNonDipInstall(),
                isPreload);
        }
	}

    void ContentInstallFlowDiP::begin()
    {
        ASYNC_INVOKE_GUARD;

        // Just in case there's a UI flow not doing it
        if (!escalationServiceEnabled())
            return;

        ORIGIN_LOG_EVENT << "Begin install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId();

        // We are in a pre-install repair if the UI tried to start a repair while we are in the ReadyToInstall state
        if (getFlowState() == ContentInstallFlowState::kReadyToInstall && isRepairing())
        {
            // WARNING:  This is kind of hacky, and it does generate a Qt warning.  HOWEVER, the forceRepair signal transition (readyToInstall->readyToStart)
            // is crashing QStateMachine for as-yet undetermined reasons.  This sidesteps the need for that transition.

            // Reset the main state machine
            QStateMachine::stop();
            emit finishedState();
            setStartState(ContentInstallFlowState::kReadyToStart);
            QStateMachine::start();

            // We need this later so we know this isn't a normal repair (i.e. treat it like an initial download for the touchup installer)
            mIsPreinstallRepair = true;

            // Let the state machine stabilize and start over again
            QMetaObject::invokeMethod(this, "begin", Qt::QueuedConnection);
            return;
        }

        if (canBegin())
        {
            initializeUpdateFlag();

			setPausedForUnretryableError(false);	// be sure to reset this for each new download
            mUseDownloaderSafeMode = false; // reset flag

            // Reset our saved bytes progress here always, as we are starting a new operation
            // regardless of whether we are installed or a fresh download...
            mInstallManifest.Set64BitInt("savedbytes", 0);

            // We are about to start the flow. If the autostart flag is set we need to
            // unset it so an autostart does not happen again unless the flow encounters
            // conditions were it needs to be set again.
            setAutoStartFlag(false);
            
            sendDownloadStartTelemetry("new",0);

            Origin::Engine::UserRef user = mUser.toStrongRef();
            bool online = user && (Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));
            if (online || !DownloadService::IsNetworkDownload(mUrl))
            {
                //Check to see if an uninstall/reinstall is occurring without shutting origin down.
                //Will need to purge some install parameters from memory
                if (!contentIsPdlc() && !mContent->localContent()->installed(true))
                {
                    mInstallArgumentRequest = InstallArgumentRequest();
                    mInstallLanguageRequest = InstallLanguageRequest();
                    mInstallArgumentResponse = InstallArgumentResponse();
                    mEulaStateResponse = EulaStateResponse();

                    // preserve ito flag because if you downloaded an update last or downloaded the game, uninstalled and then 
					// ran an ITO of the same game, previousState will be kCompleted but the initializeInstallManifest will blow
					// away the isito = true flag which causes problems later. (EBIBUGS-18442)
					bool save_isLocalSource = mInstallManifest.GetBool("isLocalSource");	
                    bool save_isITOFlow = mInstallManifest.GetBool("isITOFlow");

                    QString save_dipInstallPath;
                    if (!contentIsPdlc())
                        save_dipInstallPath = mInstallManifest.GetString("dipInstallPath"); // save dip install path setting

                    //only reset this if it's not ITO, with ITO, the language is passed in and set in initInstallParameters so
                    //clearing it here will result in user being prompted twice (once via EAInstaller and once via Origin)
                    //but we DO want to clear it for non-ITO case to handle the case where user installs with language A, uninstalls, reinstalls in the same session
                    //if we don't clear it, the reinstall will result in language A without user being prompted
                    if (!save_isITOFlow)
                        mEulaLanguageResponse = EulaLanguageResponse();

                    initializeInstallManifest();

					mInstallManifest.SetBool("isLocalSource", save_isLocalSource);	// restore flag
                    mInstallManifest.SetBool("isITOFlow", save_isITOFlow); //restore the flag
                    if (!contentIsPdlc())
                        mInstallManifest.SetString("dipInstallPath", save_dipInstallPath); // restore setting

                    initializeIntstallArgumentRequest();
                }

                // Clear the EULAs
                mEulaStateRequest = EulaStateRequest();
                initializeEulaStateRequest();

                // Create a new Job ID
                generateNewJobId();
                mInstallManifest.SetString("buildid", "");

                // Clear the Dynamic Download install-already-complete flag, in case it might have been set
                mInstallManifest.SetBool("DDinstallAlreadyCompleted", false); // Used by Dynamic Download games... indicates we've already run the installer

                ORIGIN_LOG_EVENT << (isUpdate() ? "Begin Update " : "Begin Download ") << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

				if (mRepair || mIsUpdate)
					emit enqueueRepair();
				else
	                emit beginInitializeProtocol();
            }
            else
            {
                ORIGIN_LOG_WARNING << "Unable to start. Origin is offline.";
            }
        }
        else
        {
            QString warnMsg = QString("Unable to begin download. Download may already be in progress. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowDiP::resume()
    {
        ASYNC_INVOKE_GUARD;

        // Just in case there's a UI flow not doing it
        if (!escalationServiceEnabled())
            return;

        ORIGIN_LOG_EVENT << "Resume install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (canResume())
        {
            initializeUpdateFlag();

			setPausedForUnretryableError(false);	// assumes the user manually is retrying so reset

            // We are about to resume the flow. If the autoresume flag is set we need to
            // unset it so an autoresume does not happen again unless the flow encounters
            // conditions were it needs to be set again.
            mInstallManifest.SetBool("autoresume", false);

            if (getFlowState() == ContentInstallFlowState::kReadyToInstall)
            {
                emit beginInstall();
                return;
            }

            if (!makeOnlineCheck())
            {
                mInstallManifest.SetBool("autoresume", true);
                return;
            }

            switch (getPreviousState())
            {
                case ContentInstallFlowState::kPostTransfer:
                case ContentInstallFlowState::kTransferring:
                case ContentInstallFlowState::kPaused:
                default:
                    {
                        resumeTransfer();
                    }
            }
        }
        else
        {
            QString warnMsg = QString("Unable to resume download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowDiP::resumeTransfer()
    {
        delete mManifest;
        mManifest = NULL;

        QStateMachine* resumeMachine = new QStateMachine();
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), resumeMachine, SLOT(stop()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(canceling()), resumeMachine, SLOT(stop()));

		// this state is being added so we can start the state machine right away but just
		// wait for the protocolInitialized() signal before proceeding.  if we are not active
		// right away, the signal IContentInstallFlow_error signals will be ignored (like disc errors)
		// and the state machine will never get stopped leading to multiple active state machines.
        QState* resumeMachinePreStart = new QState(resumeMachine);
        resumeMachine->setInitialState(resumeMachinePreStart);

        QState* retrieveManifest = new QState(resumeMachine);
        ORIGIN_VERIFY_CONNECT(retrieveManifest, SIGNAL(entered()), this, SLOT(retrieveDipManifest()));

        QState* processManifest = new QState(resumeMachine);
        ORIGIN_VERIFY_CONNECT(processManifest, SIGNAL(entered()), this, SLOT(processDipManifest()));

        QState* exclusions = new QState(resumeMachine);
        ORIGIN_VERIFY_CONNECT(exclusions, SIGNAL(entered()), this, SLOT(onProcessExclusionsAndDeletes()));

        QFinalState* done = new QFinalState(resumeMachine);

		resumeMachinePreStart->addTransition(this, SIGNAL(protocolInitialized()), retrieveManifest);

        retrieveManifest->addTransition(this, SIGNAL(receivedFile(const QString&)), processManifest);
        processManifest->addTransition(this, SIGNAL(processedManifest()), exclusions);
        exclusions->addTransition(this, SIGNAL(processedExclusionsAndDeletes()), done);

        ORIGIN_VERIFY_CONNECT_EX(resumeMachine, SIGNAL(finished()), this, SLOT(resumeProtocol()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(resumeMachine, SIGNAL(finished()), resumeMachine, SLOT(deleteLater()));

        ContentProtocols::RegisterWithInstallFlowThread(resumeMachine);

		resumeMachine->start();	// start it now so we can pick up any error signals during initializing state

        emit resuming();
        initializeProtocol();
    }

    void ContentInstallFlowDiP::resumeProtocol()
    {
        if (makeOnlineCheck("User is offline. Unable to resume protocol"))
        {
            if (mProtocol)
                mProtocol->Resume();
        }
        // else the flow will get suspended in IContentInstallFlow::onConnectionStateChanged
    }

    void ContentInstallFlowDiP::pause(bool autoresume)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, autoresume));

        ORIGIN_LOG_EVENT << "Pause install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (canPause())
        {
            mInstallManifest.SetBool("autoresume", autoresume);
            emit pausing();

            if (mProtocol)
                mProtocol->Pause();
        }
        // If autoresume is set to true, it means that the application and not the user
        // initiated the pause. If the application initiated the pause, and we are not
        // in a pausable state, we want to suspend the flow which will set the flow to
        // ready to install if currently installing and ready to start (with autostart
        // flag cleared) if the transfer state has not been reached yet.
        else if (autoresume)
        {
            suspend(autoresume);
        }
        else
        {
            QString warnMsg = QString("Unable to pause download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    //This should only be called in rare instances, like when Origin is about to shutdown.
    //Flows which have not progressed to the transferring state may get canceled.
    // Sets flows that get paused to auto resume and those that get canceled to auto start.
    void ContentInstallFlowDiP::suspend(bool autostart)
    {
        ContentInstallFlowState currentState = getFlowState();

        switch (currentState)
        {
            case ContentInstallFlowState::kReadyToStart:
            case ContentInstallFlowState::kReadyToInstall:
			case ContentInstallFlowState::kPaused:
			case ContentInstallFlowState::kEnqueued:
            {
                ORIGIN_LOG_EVENT << "No suspend action required, content = " << mContent->contentConfiguration()->productId();

                // do nothing, but just emit that we're done
                emit (suspendComplete(mContent->contentConfiguration()->productId()));
                break;
            }
            case ContentInstallFlowState::kTransferring:
            {
                // pause normally
                pause(autostart);
                break;
            }
            case ContentInstallFlowState::kResuming:
            {
                // We are about to go or are offline so the download session will most likely encounter errors. We want to ignore them.
                ORIGIN_VERIFY_DISCONNECT(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)));

                // intentional fall-through
            }
            case ContentInstallFlowState::kPostTransfer:
            {
                ORIGIN_LOG_WARNING << "Force pause " << mContent->contentConfiguration()->productId();


                if (mUpdateFinalizationStateMachine && mUpdateFinalizationStateMachine->isRunning())
                {
                    mUpdateFinalizationStateMachine->stop();
                }

                mInstallManifest.SetBool("autoresume", autostart);

                if (mProtocol)
                {
                    mProtocol->Pause(true);
                }
                else
                {
                    emit (protocolPaused());
                }
                break;
            }
            case ContentInstallFlowState::kInstalling:
            {
               ORIGIN_LOG_WARNING << "Force pause install, content = " << mContent->contentConfiguration()->productId();

                mInstallManifest.SetBool("autoresume", autostart);
                // go back to ready to install
                emit pauseInstall();

                break;
            }

            case ContentInstallFlowState::kPausing:
                {
                    ORIGIN_LOG_WARNING << "Already in pausing... = " << mContent->contentConfiguration()->productId();
                    //since it's in the middle of pausing, just let it finish, it will emit protocolPaused() when done
                    //and if for some reason, it doens't finish, the timeout in ContentController will just force quit
                    break;
                }
                
            case ContentInstallFlowState::kCanceling:
            {
                ORIGIN_LOG_WARNING << "Already in canceling... = " << mContent->contentConfiguration()->productId();
                //since it's in the middle of canceling, just let it finish, it will rever to kReadyToStart when done
                //and if for some reason, it doens't finish, the timeout in ContentController will just force quit
                break;
            }

            case ContentInstallFlowState::kInitializing:
            case ContentInstallFlowState::kPreTransfer:
            case ContentInstallFlowState::kPendingEulaLangSelection:
            case ContentInstallFlowState::kPendingEula:
            default:
            {
                ORIGIN_LOG_WARNING << "Force cancel, content = " << mContent->contentConfiguration()->productId() <<
                    ", state = " << currentState.toString();

                if (!isRepairing())
                {
					setAutoStartFlag(false); // Clear the autostart flag for flows that have not yet reached the kTransferring state.
                }

                if (mProtocol)
                {
                    // We are about to go or are offline so the download session will most likely encounter errors. We want to ignore them.
                    ORIGIN_VERIFY_DISCONNECT(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)));

                    //in this case, we're manually setting the state back to kReadyToStart so there wouldn't be a signal sent out for state change that
                    //the ContentController is listening for (so that it can remove from the list of pending suspend jobs)
                    //so set a flag to indicate that when onProtocolPaused() is called, it will emit the suspendComplete signal
                    mProcessingSuspend = true;
                    mProtocol->Pause(true);
                }
                else
                {
                    stop();
                }
            }
        }

        ContentServices::SaveInstallManifest(mContent, mInstallManifest);
    }

	void ContentInstallFlowDiP::startTransferFromQueue (bool manual_start)
	{
		ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, manual_start));

		ContentInstallFlowState currentState = getFlowState();

        setEnqueuedAndPaused(false);

		if (currentState == ContentInstallFlowState::kInstalling)
		{
			ORIGIN_LOG_WARNING << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but current state is kInstalling";
			return;
		}

		if ((currentState != ContentInstallFlowState::kEnqueued) && (currentState != ContentInstallFlowState::kPaused))
		{
			ORIGIN_LOG_ERROR << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but we are not in kEnqueued or kPaused state.  Current state is " << currentState.toString();
			return;			
		}

        // if manually starting (from UI) bypass this check - unretryable error flag will be reset in resume()
		if (!manual_start && pausedForUnretryableError())	// an error occurred that we can't retry on so leave it paused as we assume anything below the head if in error state is also in an error state.
		{
			ORIGIN_LOG_WARNING << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but it is paused for a unretryable error";
			return;
		}

		ORIGIN_LOG_EVENT << "Start transferring off queue for " << mContent->contentConfiguration()->productId();

		if ((mRepair || mIsUpdate) && (currentState == ContentInstallFlowState::kEnqueued))
			emit beginInitializeProtocol();
		else if (mProtocol == NULL || canResume())
			resume();
		else
			startProtocol();
    }

    void ContentInstallFlowDiP::cancel(bool forceCancelAndUninstall)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, forceCancelAndUninstall));

        ORIGIN_LOG_EVENT << "Cancel install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (forceCancelAndUninstall || canCancel())
        {
            if(mContent->localContent()->previousInstalledLocale().count())
            {
                mContent->localContent()->setInstalledLocale(mContent->localContent()->previousInstalledLocale());
                mContent->localContent()->setPreviousInstalledLocale("");
            }
			setPausedForUnretryableError(false);	// reset
            mUseDownloaderSafeMode = false; // reset flag

            // TELEMETRY: User has canceled the download.
            bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
            GetTelemetryInterface()->Metric_DL_CANCEL(mContent->contentConfiguration()->productId().toLocal8Bit().constData(),isPreload);

            if (isDynamicDownload() && forceCancelAndUninstall)
            {
                mInstallManifest.SetBool("DDupdateUninstall", true);    // so uninstall will be done after cancel is complete
            }

            emit canceling();
            if (!mProtocol)
            {
                createProtocol();
                initializeUpdateFlag();

                // We don't need to worry about the protocol being in a safe state since we just woke up from a pause, the download is already in progress
                mProtocol->SetSafeCancelFlag(false);

                // If we're a dynamic download, we must inform the protocol so the appropriate files get cleaned up
                if (isDynamicDownload())
                {
                    mProtocol->SetDynamicDownloadFlag(true);
                }
            }

            mProtocol->SetUpdateFlag(mIsUpdate || contentIsPdlc());
            mProtocol->SetRepairFlag(mRepair);

            mProtocol->SetUnpackDirectory(getDownloadLocation());

            // For initial (non-update) dynamic downloads that have already passed the touchup installer phase, we do not want to wipe out the game files
            // The UI will take care of calling the uninstall flow
            if (isDynamicDownload())
            {
                ORIGIN_LOG_EVENT << "Operation canceled; Clear DDC cache data for " << mContent->contentConfiguration()->productId() << ".";

                DynamicDownloadController::GetInstance()->ClearDynamicDownloadData(mContent->contentConfiguration()->productId());

                if (mInstallManifest.GetBool("DDInitialDownload") && mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
                {
                    ORIGIN_LOG_EVENT << "Flow for " << mContent->contentConfiguration()->productId() << " is a dynamic download.  Setting safe cancel flag to prevent cleanup of game files. (We will uninstall.)";

                    // This prevents the protocol from wiping out the game files, which are needed to run the uninstaller
                    mProtocol->SetSafeCancelFlag(true);
                }
            }

            mProtocol->Cancel();
        }
        else
        {
            QString warnMsg = QString("Unable to cancel download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }


    void ContentInstallFlowDiP::retry ()
    {
        ASYNC_INVOKE_GUARD;

        ORIGIN_LOG_EVENT << "Retry install flow for " << mContent->contentConfiguration()->productId();

        if (canRetry())
        {
            if (mProtocol)
                mProtocol->Retry();
        }
        else
        {
            QString warnMsg = QString("Unable to retry download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowDiP::onCanceled()
    {
        emit canceling();

        bool wasUpdate = false;
        bool wasDynamicDownload = false;
        bool wasDynamicDownloadAlreadyInstalled = false;
        
        wasUpdate = isUpdate();
        wasDynamicDownload = isDynamicDownload(); // Cannot use the 'contentPlayable' ref-bool, since it will say 'not playable' for cancelling content (not designed for internal use)
        wasDynamicDownloadAlreadyInstalled = (wasDynamicDownload) ? mInstallManifest.GetBool("DDinstallAlreadyCompleted") : false; // Only look at the manifest value for dynamic downloads

        // Re-initialize requests and the install manifest.
        // For updates, some info must be preserved.
        if (!mIsUpdate)
        {
            mInstallArgumentRequest = InstallArgumentRequest();
            mInstallLanguageRequest = InstallLanguageRequest();
            mEulaStateRequest = EulaStateRequest();
            mInstallArgumentResponse = InstallArgumentResponse();
            mEulaStateResponse = EulaStateResponse();
            mEulaLanguageResponse = EulaLanguageResponse();

            ContentServices::DeleteInstallManifest(mContent);
            initializeInstallManifest();
            initializeIntstallArgumentRequest();
            initializeEulaStateRequest();
        }
        else
        {
            if (mUpdateFinalizationStateMachine)
            {
                getState(ContentInstallFlowState::kPostTransfer)->removeTransition(mUpdateFinalizeTransition);
                mUpdateFinalizeTransition->deleteLater();
                mUpdateFinalizeTransition = NULL;
                mUpdateFinalizationStateMachine->deleteLater();
                mUpdateFinalizationStateMachine = NULL;
            }
        }

        // Clear the job ID
        clearJobId();
        mInstallManifest.SetString("buildid", "");

        // InitializeInstallManifest sets the currentstate to kInvalid. Which is bad because we need to keep what the state is
        mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kCanceling).toString());
        // Reset the main state machine
        setStartState(ContentInstallFlowState::kReadyToStart);
        stop();

        // Destroy the protocol
        if (mProtocol)
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mProtocol->deleteLater();
            mProtocol = NULL;
        }

        removeFromQueue(true);

        // If this was a dynamic download, it is possible that we may have already passed the installation phase (which can occur mid-download)
        // In those cases, we have previously ensured that the protocol would not destroy the game files (safe cancel flag), but we need to call the actual
        // uninstaller in order to complete the 'cancellation'.  (The uninstaller will remove all game files, as well as shortcuts, etc)
        if (wasDynamicDownload)
        {
            bool isUninstalling = mInstallManifest.GetBool("DDupdateUninstall");
            // Reset the dynamic download state to false, because the job is complete
            mInstallManifest.SetBool("DDupdateUninstall", false);
            mInstallManifest.SetBool("DynamicDownload", false);
            mIsDynamicDownload = false;

            if (wasDynamicDownloadAlreadyInstalled)
            {
                if (wasUpdate)
                {
                    if (isUninstalling)
                    {
                        // Silently uninstall the content since we've already run the touchup
                        ORIGIN_LOG_EVENT << "Content [" << mContent->contentConfiguration()->productId() << "] user elected to uninstall Dynamic Download during update after installation.  Content will be uninstalled.";
                        mContent->localContent()->unInstall(true);
                    }
                    else
                    {
                        // We cannot really handle canceling dynamic updates once they are playable cleanly since irreversible changes have already been committed to disk.
                        // The safest thing to do here is leave the content in a broken-but-repairable state, rather than uninstalling the entire game.  (Unlikely the user wants that)
                        // In this situation, canCancel() would have been false to prevent the UI from canceling us, but somehow we got here anyway, and all we can do is log an error and move on.
                        ORIGIN_LOG_ERROR << "Content [" << mContent->contentConfiguration()->productId() << "] Dynamic Update canceled after installation.  Content is in an inconsistent state and must be repaired.";
                    }
                }
                else
                {
                    // Silently uninstall the content since we've already run the touchup
                    ORIGIN_LOG_EVENT << "Content [" << mContent->contentConfiguration()->productId() << "] Dynamic Download canceled after installation.  Content will be uninstalled.";
                    mContent->localContent()->unInstall(true);
                }
            }
        }
    }

    void ContentInstallFlowDiP::cleanupTemporaryFiles()
    {
        while (!mTemporaryFiles.isEmpty())
        {
            QFile tempFile(mTemporaryFiles.back());
            mTemporaryFiles.pop_back();
            if (tempFile.exists())
            {
                tempFile.close();
                tempFile.remove(tempFile.fileName());
            }
        }

		mPendingTransferFiles.clear();
    }

    void ContentInstallFlowDiP::handleError(qint32 errorType, qint32 errorCode , QString errorMessage)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(qint32, errorType), Q_ARG(qint32, errorCode), Q_ARG(QString, errorMessage))
        
        // The transferring, post transfer, installing, and ready to install states are handled in onFlowError().
        // If the main state machine is resuming, it will transition to paused once the protocol emits its
        // paused signal. During all other states, reset the main state machine.
        /*ContentInstallFlowState state = getFlowState();
        if (state != ContentInstallFlowState::kTransferring && state != ContentInstallFlowState::kPostTransfer &&
            state != ContentInstallFlowState::kInstalling && state != ContentInstallFlowState::kResuming &&
            state != ContentInstallFlowState::kReadyToInstall)
        {
            stop();
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
        }*/
    }
    
    void ContentInstallFlowDiP::contentConfigurationUpdated()
    {
        ASYNC_INVOKE_GUARD;

        // Be very careful with what you implement here.  This method will get called anytime a user refreshes
        // their mygames page.   Can happen anytime during the normal download/install flow.

        // If overrides are enabled (an eacore.ini file is present), the download path may be updated by ODT.  Update the 
        // download path - as long as we are in the kReadyToStart state.
        if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
        {
            if (getFlowState() == ContentInstallFlowState::kReadyToStart && mContent)
            {
                initUrl();
            }
        }

        // Check to see if release control downloadability changed if we are repairing or updating.  downloadableStatus returns
        // DS_ReleaseControl if entitlement has past the terminationDate or the useEndDate|releaseDate|downloadDate softwareControlDates on the catalog offer have been past
        if(mContent->localContent()->downloadableStatus() == Origin::Engine::Content::LocalContent::DS_ReleaseControl)
        {
            // Only cancel if we are updating or repairing.
            if((isUpdate() || isRepairing()) && canCancel())
            {
                ORIGIN_LOG_EVENT << "Canceling update or repair due to change of downloadable status of content for [" << mContent->contentConfiguration()->productId() << "]";
                cancel();
            }
        }
    }

    void ContentInstallFlowDiP::onFlowError()
    {
        // We don't want to delete the protocol if the flow is depending on the protocol's Paused signal
        // to transition from kPausing to kPaused. In this case the protocol will be deleted in
        // onProtocolPaused()
        bool deleteProtocol = false;
        ContentInstallFlowState previousState = getPreviousState();

        switch (previousState)
        {
            case ContentInstallFlowState::kReadyToStart:
            {                
                // Special case: if there was already a UAC prompt pending and we had to immediately cancel the download (ie from a kReadyToStart)
                // we wanted to emit an error to force a state change so that LocalContent resets its 'in-use' path - but for that we must go back to the kReadyToStart
                // state
                emit resetForReadyToStart();
            }
            break;

            case ContentInstallFlowState::kTransferring:
            {
                // On errors during transfer, the protocol executes its paused logic. We must enter the pausing state
                // and wait for the protocol to indicate that it has finished pausing by emitting its pause signal.
                emit pausing();

                // For retriable errors that have exceeded the max retry count, the protocol has already
                // emitted its Paused signal during the first failure. We need to re-emit it to put the
                // main state machine into the paused state.
                if (mDownloadRetryCount >= sMaxDownloadRetries)
                {
                    deleteProtocol = true;
                    emit protocolPaused();
                }
                else
                if (isITO() && mProtocol && !mProtocol->IsInitialized())    // for EBIBUGS-27447 - where when waiting for ITO disc change the crc map becomes write-protected
                {                                                           // and install flow gets stuck in kPausing because protocol has been de-initialized in TransferStartJobReady()
                    emit protocolPaused();                                  // which calls ClearRunState() when the crc map cannot be written out.
                }
                break;
            }
            case ContentInstallFlowState::kPostTransfer:
            {
                if (mUpdateFinalizationStateMachine)
                {
                    getState(ContentInstallFlowState::kPostTransfer)->removeTransition(mUpdateFinalizeTransition);
                    mUpdateFinalizeTransition->deleteLater();
                    mUpdateFinalizeTransition = NULL;
                    mUpdateFinalizationStateMachine->deleteLater();
                    mUpdateFinalizationStateMachine = NULL;
                }

                // To handle de-staging error resumption (locked files) (helps fix EBIBUGS-16547)
                mInstallManifest.SetBool("autoresume", true);
                ContentServices::SaveInstallManifest(mContent, mInstallManifest);

                // fall through
            }
			case ContentInstallFlowState::kPaused:
            {
                emit protocolPaused();
                deleteProtocol = true;
                break;
            }
            case ContentInstallFlowState::kReadyToInstall:
            case ContentInstallFlowState::kInstalling:
            {
                emit pauseInstall();
                deleteProtocol = true;
                break;
            }
            default:
            {
                deleteProtocol = true;
                stop();
                ContentServices::SaveInstallManifest(mContent, mInstallManifest);  
            }
        }

        // If we entered this method, we encountered a non retriable error, or we have exceeded the max number
        // of retries. Reset the count for the next retriable error that's encountered after resuming.
        mDownloadRetryCount = 0;

        //After an error, ITO flow needs some data to persist in the protocol so don't delete
        if (deleteProtocol && mProtocol && !isUsingLocalSource())
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mProtocol->deleteLater();
            mProtocol = NULL;
        }

        if (!mErrorPauseQueue)
        {
		    // move down the queue here
		    // must protect against user logging out which results in a null queueController
		    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		    if (queueController)
            {
                queueController->moveToBottom(mContent);
            }
        }
        else
            mErrorPauseQueue = false;   // reset

		setPausedForUnretryableError(true);	// so new enqueued items will skip over this
    }

    void ContentInstallFlowDiP::onProtocolError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        QString errorDescription = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
        ORIGIN_LOG_ERROR << "ContentInstallFlowDiP: errortype = " << errorInfo.mErrorType << ", errorDesc = " << errorDescription << ", errorcode = " << errorInfo.mErrorCode << ", errorctx = " << errorInfo.mErrorContext; 

        // EBIBUGS-18553: on all download errors, ensure that we will try to resume the download. Sometimes when going to 
        // sleep, network errors occur well before (2 seconds was seen) the sleep signal is received.
        mInstallManifest.SetBool("autoresume", true);

        if (!makeOnlineCheck())
        {
            // Do nothing. IContentInstallFlow::onConnectionStateChanged will pause or suspend the flow.
            return;
        }

        if ((errorInfo.mErrorType == FlowError::JitUrlRequestFailed) || (errorInfo.mErrorType == DownloadError::HttpError && errorInfo.mErrorCode == Services::Http403ClientErrorForbidden))
        {
            ++mDownloadRetryCount;
            if (mDownloadRetryCount < sMaxDownloadRetries)
            {
                if(errorInfo.mErrorType == FlowError::JitUrlRequestFailed)
                {
                    QTimer::singleShot(kDelayBeforeJitRetry, this, SLOT(makeJitUrlRequest()));
                }
                else
                {
                    ContentInstallFlowState currentState = getFlowState();
                    if (currentState == ContentInstallFlowState::kInitializing || currentState == ContentInstallFlowState::kResuming)
                    {
                        ORIGIN_LOG_WARNING << "Attempting retry during initialize state.";
                        initializeProtocol();
                    }
                    else if (currentState == ContentInstallFlowState::kTransferring)
                    {
                        ORIGIN_LOG_WARNING << "Attempting retry during tranfer state."; 
                        mResumeOnPaused = true;
                    }
                }
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        // If we ran into a situation where the on-disk state did not match the previously calculated state, we must restart the protocol
        else if (errorInfo.mErrorType == UnpackError::AlreadyCompleted)
        {
            ++mDownloadRetryCount;
            if (mDownloadRetryCount < sMaxDownloadRetries)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "AlreadyCompletedRetry", qPrintable(QString("%1").arg(mDownloadRetryCount)), __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] Protocol reported metadata state mismatch, retrying..."; 
                mResumeOnPaused = true;
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        // If we experience PatchBuilder errors
        else if (errorInfo.mErrorType == PatchBuilderError::FilePosFailure
            ||   errorInfo.mErrorType == PatchBuilderError::FileOpenFailure
            ||   (errorInfo.mErrorType == DownloadError::HttpError && errorInfo.mErrorCode == 416)) // Requested range not satisfiable
        {
            ++mDownloadRetryCount;
            if (mDownloadRetryCount < sMaxDownloadRetries)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "SafeModeFallback", "PatchBuilderError", __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] Patch builder error detected, switching to safe mode.";

                mUseDownloaderSafeMode = true;

                mResumeOnPaused = true;
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        else if (errorInfo.mErrorType == DownloadError::HttpError && errorInfo.mErrorCode == 408) // 408 errors
        {
            ++mDownloadRetryCount;
            if (mDownloadRetryCount < sMaxDownloadRetries)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "SafeModeFallback", "HTTP408", __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] HTTP 408 error detected, switching to safe mode and switching CDNs.";

                mUseDownloaderSafeMode = true;

                // Also switch CDNs
                mUseAlternateCDN = true;

                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "CDNFailoverSwitch", qPrintable(mLastCDN), __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] Switching CDNs.  (Last CDN = " << mLastCDN << ")  Attempting retry during tranfer state.";

                mResumeOnPaused = true;
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        // If we experience data corruption errors, we should try switching CDNs
        else if (errorInfo.mErrorType == UnpackError::StreamStateInvalid 
              || errorInfo.mErrorType == UnpackError::StreamUnpackFailed 
              || errorInfo.mErrorType == UnpackError::CRCFailed)
        {
            ++mDataCorruptionRetryCount;
            if (mDataCorruptionRetryCount < sMaxDataCorruptionRetries)
            {
                // Use safe mode next time
                mUseDownloaderSafeMode = true;

                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "SafeModeFallback", "DataCorruption", __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] Data corruption error detected, switching to safe mode.";

                // Also switch CDNs
                mUseAlternateCDN = true;

                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mContent->contentConfiguration()->productId()), "CDNFailoverSwitch", qPrintable(mLastCDN), __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << "[" << mContent->contentConfiguration()->productId() << "] Switching CDNs.  (Last CDN = " << mLastCDN << ")  Attempting retry during tranfer state."; 
                mResumeOnPaused = true;
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        // If the disk is full, dump HD info to logs
        else if (errorInfo.mErrorType == UnpackError::IOWriteFailed && errorInfo.mErrorCode == Services::PlatformService::ErrorDiskFull)
        {
            mErrorPauseQueue = true;
#ifdef ORIGIN_PC
            Services::PlatformService::PrintDriveInfoToLog();
#endif
#ifdef ORIGIN_MAC
            dumpFileSystemInformation();
#endif
            
        }

        // If ITO disc was ejected
        else if (errorInfo.mErrorType == ProtocolError::DiscEjected)
        {
            if (mProtocol)
                mProtocol->Retry();

            return;
        }
        else if (errorInfo.mErrorType == DownloadError::NotConnectedError)
        {
            // Do nothing. IContentInstallFlow::onConnectionStateChanged will pause or suspend the flow.
            return;
        }

        sendCriticalErrorTelemetry(errorInfo);
        emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
    }

    void ContentInstallFlowDiP::initializeIntstallArgumentRequest()
    {
        mInstallArgumentRequest.contentDisplayName = mContent->contentConfiguration()->displayName();

        mInstallArgumentRequest.optionalEulas.gameProductId = mContent->contentConfiguration()->productId();
        mInstallArgumentRequest.productId = mContent->contentConfiguration()->productId();
        mInstallArgumentRequest.isLocalSource = false;
        mInstallArgumentRequest.isITO = false;
        mInstallArgumentRequest.optionalEulas.gameTitle = mContent->contentConfiguration()->displayName();
        mInstallArgumentRequest.optionalEulas.isAutoPatch = "0";
        mInstallArgumentRequest.isDip = true;

    }

    void ContentInstallFlowDiP::initializeEulaStateRequest()
    {
        mEulaStateRequest.eulas.gameProductId = mContent->contentConfiguration()->productId();
        mEulaStateRequest.eulas.gameTitle = mContent->contentConfiguration()->displayName();
        mEulaStateRequest.eulas.isAutoPatch = "0";
        mInstallArgumentRequest.isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
    }

    bool ContentInstallFlowDiP::validateInstallManifest()
    {
        ContentInstallFlowState currentState = getFlowState();
        ContentInstallFlowState previousState = getPreviousState();

        Engine::Content::EntitlementRef parent = mContent;
        if (contentIsPdlc() && !mContent->parent().isNull())
        {
            parent = mContent->parent();
        }

        if (currentState != ContentInstallFlowState::kReadyToStart && isUsingLocalSource())
        {
            ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() << ". Previous session was ITO and was improperly shutdown.";
            
            initializeInstallManifest();
            setStartState(ContentInstallFlowState::kReadyToStart);
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // We need to remove all "downloaded" data because multi-disc ITO installs will get confused and get stuck.
            // We currently do not support resuming ITO installs.
            EnvUtils::DeleteFolderIfPresent(getDownloadLocation(), true, true);
            return false;
        }

        if (previousState == ContentInstallFlowState::kCompleted && !parent->localContent()->installed())
        {
            ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() << ". Content was uninstalled.";

            initializeInstallManifest();
            setStartState(ContentInstallFlowState::kReadyToStart);
            return false;
        }

        if (currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kPaused && currentState != ContentInstallFlowState::kReadyToInstall)
        {
            ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                ". Install flow was interrupted during install state " << currentState.toString() <<
                ". Previous state was " << previousState.toString();

            switch (currentState)
            {
                case ContentInstallFlowState::kInstalling:
                    {
                        if (!mContent->localContent()->installed())
                        {
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kInstalling).toString());
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToInstall).toString());
                            setStartState(ContentInstallFlowState::kReadyToInstall);
                            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                        }
                        else    // catch situation where origin is killed when installing and installing completed and try to resume (EBIBUGS-29251)
                        if (mInstallManifest.GetBool("dynamicdownload") && !mInstallManifest.GetBool("ddinstallalreadycompleted"))
                        {
                            mInstallManifest.SetBool("ddinstallalreadycompleted", true);    // mark as installed assuming origin was interrupted during install but install completed

                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kTransferring).toString());
                            setStartState(ContentInstallFlowState::kPaused);
                            if (previousState == ContentInstallFlowState::kPostTransfer)
                                mInstallManifest.SetBool("autoresume", true);
                            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                            break;
                        }
                        else
                        {
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToStart).toString());
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kCompleted).toString());
                            setStartState(ContentInstallFlowState::kReadyToStart);
                            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                        }
                        break;
                    }
                case ContentInstallFlowState::kPostTransfer:
                case ContentInstallFlowState::kTransferring:
                case ContentInstallFlowState::kResuming:
                case ContentInstallFlowState::kPausing:
				case ContentInstallFlowState::kEnqueued:
                    {
                        mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                        mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kTransferring).toString());
                        setStartState(ContentInstallFlowState::kPaused);
                        if ((currentState == ContentInstallFlowState::kTransferring) || (currentState == ContentInstallFlowState::kEnqueued))
                            mInstallManifest.SetBool("autoresume", true);
                        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

                        mRepair = mInstallManifest.GetBool("isrepair");
                        break;
                    }
                case ContentInstallFlowState::kError:
                    {
                        if (previousState == ContentInstallFlowState::kPaused || previousState == ContentInstallFlowState::kTransferring || previousState == ContentInstallFlowState::kPostTransfer)
                        {
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kTransferring).toString());
                            setStartState(ContentInstallFlowState::kPaused); 
                            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                            mRepair = mInstallManifest.GetBool("isrepair");
                        }
                        else if (previousState == ContentInstallFlowState::kReadyToInstall || previousState == ContentInstallFlowState::kInstalling)
                        {
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToInstall).toString());
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kInstalling).toString());
                            setStartState(ContentInstallFlowState::kReadyToInstall);
                            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

                        }
                        else
                        {
                            initializeInstallManifest();
                            setStartState(ContentInstallFlowState::kReadyToStart);
                            return false;
                        }
                        break;
                    }
                default:
                    {
                        initializeInstallManifest();
                        setStartState(ContentInstallFlowState::kReadyToStart);
                        return false;
                    }
            }
        }

        // Make sure installer exists if content is ready to install
        if (ContentInstallFlowState(mInstallManifest.GetString("currentstate")) == ContentInstallFlowState::kReadyToInstall)
        {
            if (!loadDipManifestFile())
            {
                ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                    "Install manifest indicates ready to install, but the dip manifest does not exist.";

                initializeInstallManifest();
                setStartState(ContentInstallFlowState::kReadyToStart);
                return false;
            }

            QString installerPath(getTouchupInstallerExecutable());
            
#ifdef ORIGIN_MAC
            if (installerPath.endsWith("/__Installer/touchup.exe", Qt::CaseInsensitive))
            {
                ORIGIN_LOG_EVENT << "Detected PC touchup installer in DiP manifest! " << installerPath;
                
                QString productId = mContent->contentConfiguration()->productId();
                
                if (Origin::Services::PlatformService::isSimCityProductId(productId))
                {
                    ORIGIN_LOG_EVENT << "Detected bad SimCity install! " << installerPath;
                    
                    GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(productId), "SimCityHackFix", qPrintable(installerPath), __FILE__, __LINE__);
                    
                    // Blow away the bad installation by calling cancel on ourselves
                    QMetaObject::invokeMethod(this, "cancel", Qt::QueuedConnection);
                    
                    return true;
                }
            }
#endif
            
            if (!QFile::exists(installerPath))
            {
                ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                    "Install manifest indicates ready to install, but the installer does not exist: " << installerPath;

                delete mManifest;
                mManifest = NULL;

                initializeInstallManifest();
                setStartState(ContentInstallFlowState::kReadyToStart);
                return false;
            }
        }
        
        if (ContentInstallFlowState(mInstallManifest.GetString("currentstate")) == ContentInstallFlowState::kPaused &&
            ContentInstallFlowState(mInstallManifest.GetString("previousstate")) == ContentInstallFlowState::kTransferring)
        {
            sendDownloadStartTelemetry("resume", mInstallManifest.Get64BitInt("savedbytes"));

            mRepair = mInstallManifest.GetBool("isrepair");
        }

        if (!mIsUpdate)
        {
            initiateIntersessionResume();
        }

        return true;
    }

    QString ContentInstallFlowDiP::getTouchupInstallerParameters()
    {
        QString parameters(mManifest->GetInstallerParams());

        initiateIntersessionResume();   // Reads the mInstallArgumentResponse values from mManifest

        // Skip the special repair/update parameters if we are doing a repair of a partial installation
        if (!mIsPreinstallRepair)
        {
            // For DiP 2.2 and up, if a specific update or repair parameter is specified they will override the default ones
            if (mRepair)
            {
                if (!mManifest->GetRepairParams().isEmpty())
                    parameters = mManifest->GetRepairParams();
            }
            else if (mIsUpdate)
            {
                if (!mManifest->GetUpdateParams().isEmpty())
                    parameters = mManifest->GetUpdateParams();
            }
        }

        QString installPath = getDownloadLocation();

        if (!installPath.endsWith('\\'))
        {
            installPath.append("\\");
        }

        parameters.replace("{installLocation}", installPath);

		if (mEulaLanguageResponse.selectedLanguage.size() == 0)
		{
			ORIGIN_LOG_ERROR << "Setting -locale parameter to an empty string when calling touchup installer";
		}
        parameters.replace("{locale}",  mEulaLanguageResponse.selectedLanguage);

        parameters.append(QString(" -startmenuIcon=%1 -desktopIcon=%2").arg(
            static_cast<int>(mInstallArgumentResponse.installStartMenuShortCut)).arg(
            static_cast<int>(mInstallArgumentResponse.installDesktopShortCut)));


        if (mInstallArgumentResponse.optionalComponentsToInstall)
        {
            OptComponentInfoList::const_iterator it =  mManifest->GetOptionalCompInfo(currentLanguage()).begin();
            while ( it != mManifest->GetOptionalCompInfo(currentLanguage()).end() )
            {
                QString sFlagName = it->mFlagName;
                if ((mInstallArgumentResponse.optionalComponentsToInstall & 1) != 0)
                {
                    parameters.append(" ");
                    parameters.append(sFlagName);
                }
                mInstallArgumentResponse.optionalComponentsToInstall >>= 1;
                it++;
            }
        }

        return parameters;
    }

    QString ContentInstallFlowDiP::getTouchupInstallerExecutable()
    {
        QString touchupInstaller;
        QString installerPath = getDownloadLocation();

        if(mManifest)
        {
            QString installerExecutable(mManifest->GetInstallerPath());

            if (!(installerPath.isEmpty() || installerExecutable.isEmpty()))
            {
                installerPath.replace('\\', '/');
                if (installerPath.endsWith('/'))
                {
                    installerPath = installerPath.left(installerPath.size() - 1);
                }

                installerExecutable.replace('\\', '/');
                if (installerExecutable.startsWith('/'))
                {
                    installerExecutable = installerExecutable.right(installerExecutable.size() - 1);
                }

                touchupInstaller = QString("%1/%2").arg(installerPath).arg(installerExecutable);
            }
        }
		else
		{
			ORIGIN_ASSERT(mManifest);
		}

        return touchupInstaller;
    }

    void ContentInstallFlowDiP::promptForDiscChange (int nextDiscNum, int totalDiscNum, int wrongDisc)
    {
        mInstallArgumentRequest.nextDiscNum = nextDiscNum;
        mInstallArgumentRequest.wrongDiscNum = wrongDisc;

        emit pendingDiscChange (mInstallArgumentRequest);
    }

    DiPManifest* ContentInstallFlowDiP::getDiPManifest() const
    {
        return mManifest;
    }

    ContentProtocolPackage* ContentInstallFlowDiP::getProtocol() const
    {
        return mProtocol;
    }

    Parameterizer& ContentInstallFlowDiP::getInstallManifest()
    {
        return mInstallManifest;
    }

    bool ContentInstallFlowDiP::loadDipManifestFile()
    {
        QString installPath = getDownloadLocation();
        if (!installPath.isEmpty())
        {
            installPath.replace("\\", "/");
            if (!installPath.endsWith("/"))
            {
                installPath.append("/");
            }
            QString dipManifestPath = CFilePath::Absolutize(installPath, mContent->contentConfiguration()->dipManifestPath()).ToString();
            
            if (QFile::exists(dipManifestPath))
            {
                delete mManifest;
                mManifest = new DiPManifest();
                if (mManifest->Load(dipManifestPath, mContent->contentConfiguration()->isPDLC()))
                {
                    return true;
                }
                else
                {
                    delete mManifest;
                    mManifest = NULL;
                }
            }
        }

        return false;
    }

    bool ContentInstallFlowDiP::canBegin()
    {
        return getFlowState() == ContentInstallFlowState::kReadyToStart;
    }

    bool ContentInstallFlowDiP::canCancel()
    {
        ContentInstallFlowState currentState = getFlowState();
        bool updateIsFinalizing = mUpdateFinalizationStateMachine && !mUpdateFinalizationStateMachine->CanCancel();
        bool dynamicUpdateAlreadyApplied = isDynamicDownload() && isUpdate() && mInstallManifest.GetBool("DDinstallAlreadyCompleted");
        return currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kPausing &&
            currentState != ContentInstallFlowState::kResuming && currentState != ContentInstallFlowState::kCanceling &&
            currentState != ContentInstallFlowState::kInstalling && !updateIsFinalizing && !dynamicUpdateAlreadyApplied;
    }

    bool ContentInstallFlowDiP::canPause()
    {
        return getFlowState() == ContentInstallFlowState::kTransferring;
    }

    bool ContentInstallFlowDiP::canResume()
    {
        ContentInstallFlowState currentState = getFlowState();
        Engine::UserRef user(mUser.toStrongRef());
        bool isOnlineOrLocalDownload = ((user && (Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()))) || !DownloadService::IsNetworkDownload(mUrl));

        if (!isOnlineOrLocalDownload && currentState == ContentInstallFlowState::kPaused)
        {
            ORIGIN_LOG_WARNING << "Unable to resume download of " << mContent->contentConfiguration()->productId() << " while offline.";
        }

        //if we are online or its a local download and we are paused or ready to install, then we can resume
        return  (isOnlineOrLocalDownload && (currentState == ContentInstallFlowState::kPaused)) || currentState == ContentInstallFlowState::kReadyToInstall;
    }

    bool ContentInstallFlowDiP::canRetry()
    {
        return getFlowState() == ContentInstallFlowState::kPaused;
    }

    bool ContentInstallFlowDiP::isActive()
    {
        ContentInstallFlowState currentState = getFlowState();
        return (currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kCompleted &&
            currentState != ContentInstallFlowState::kInvalid);
    }

    bool ContentInstallFlowDiP::isRepairing() const
    {
        return mRepair;
    }

    bool ContentInstallFlowDiP::isUpdate() const
    {
        return mIsUpdate;
    }

    void ContentInstallFlowDiP::initializeUpdateFlag()
    {
        if (mContent != NULL)
        {
            mIsUpdate = mContent->localContent()->installed(true);

            // If this manifest flag is set, we're finishing a progressive initial download, even though the game might already be installed
            // Don't treat this as an update
            if (mInstallManifest.GetBool("DDInitialDownload"))
            {
                mIsUpdate = false;
            }

            // If the game is installed (we're doing an update), this can't be a pre-install repair anymore.
            if (mIsUpdate)
            {
                mIsPreinstallRepair = false;
            }
        }
    }

    void ContentInstallFlowDiP::onProtocolStarted()
    {
        emit protocolStarted();
    }

    void ContentInstallFlowDiP::onDynamicRequiredPortionSizeComputed(qint64 requiredPortionSize)
    {
        mDynamicRequiredPortionSize = requiredPortionSize;
        mInstallManifest.Set64BitInt("DDRequiredPortion", mDynamicRequiredPortionSize);
    }

    bool ContentInstallFlowDiP::checkIfRequiredChunksReadyToInstall() const
    {
        for (QMap<int, DynamicDownloadChunkState>::const_iterator chunkIter = mChunkState.cbegin(); chunkIter != mChunkState.cend(); ++chunkIter)
        {
            const DynamicDownloadChunkState chunkState = chunkIter.value();

            // Only bother with required chunks
            if (chunkState.requirement != Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired)
                continue;

            // If the chunk isn't already installed or ready to install, we aren't ready
            if (chunkState.state != DynamicDownloadChunkState::kInstalled && chunkState.state != DynamicDownloadChunkState::kReadyToInstall)
                return false;
        }

        return true;
    }

    void ContentInstallFlowDiP::onDynamicChunkReadyToInstall(int chunkId)
    {
        if (mIsDynamicDownload)
        {
            ORIGIN_LOG_EVENT << "Dynamic chunk " << chunkId << " ready to install.";

            DynamicDownloadChunkState& chunkState = mChunkState[chunkId];
            chunkState.chunkId = chunkId; // Capture this in case we didn't already have it (dynamic chunk creation)
            chunkState.state = DynamicDownloadChunkState::kReadyToInstall;

            // If we haven't yet executed the installer, and a required chunk finishes
            if (!mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
            {
                // If it's a required chunk, see if all the required chunks are ready to install
                if (chunkState.requirement == Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired && checkIfRequiredChunksReadyToInstall())
                {
                    emit dynamicDownloadRequiredChunksReadyToInstall();
                }
            }
            else
            {
                // If the installer has executed, we are free to install chunks as they become available because the game watches the SDK

                ORIGIN_LOG_EVENT << "Installing Dynamic chunk " << chunkId;

                // Command the protocol to install it immediately, no need to wait
                chunkState.state = DynamicDownloadChunkState::kInstalling;
                if (mProtocol)
                    mProtocol->InstallDynamicChunk(chunkId);
            }
        }
    }

    void ContentInstallFlowDiP::installAllReadyChunks()
    {
        // What are we doing here then?
        if (!mIsDynamicDownload || !mProtocol)
        {
            ORIGIN_LOG_WARNING << "Can't install chunks; Not dynamic download or protocol not valid";
            return;
        }

        // We can only do this if the protocol is running or finished
        if (mProtocol->protocolState() != Origin::Downloader::kContentProtocolRunning && mProtocol->protocolState() != Origin::Downloader::kContentProtocolFinished)
        {
            ORIGIN_LOG_WARNING << "Can't install chunks; Protocol not running or finished; State = " << IContentProtocol::protocolStateString(mProtocol->protocolState());
            return;
        }

        ORIGIN_LOG_EVENT << "Install all ready chunks.";

        int chunksInstalled = 0;
        for (QMap<int, DynamicDownloadChunkState>::iterator chunkIter = mChunkState.begin(); chunkIter != mChunkState.end(); ++chunkIter)
        {
            DynamicDownloadChunkState& chunkState = chunkIter.value();

            // If the chunk isn't ready to install, skip it
            if (chunkState.state != DynamicDownloadChunkState::kReadyToInstall)
                continue;

            ORIGIN_LOG_EVENT << "Installing Dynamic chunk " << chunkState.chunkId;

            // Command the protocol to install it
            chunkState.state = DynamicDownloadChunkState::kInstalling;
            mProtocol->InstallDynamicChunk(chunkState.chunkId);

            ++chunksInstalled;
        }

        ORIGIN_LOG_EVENT << "Requested installation for " << chunksInstalled << " chunks.";

        // Are the required chunks already installed?
        if (checkIfRequiredChunksInstalled())
        {
            // Just move forward with the download
            emit dynamicDownloadRequiredChunksComplete();
        }
    }

    void ContentInstallFlowDiP::onUpdateReadyToFinalize()
    {
        // Normal case for non-Dynamic Downloads; Just finalize/destage and move on (protocol will emit Finalized())
        if (!mIsDynamicDownload)
        {
			if (getFlowState() == ContentInstallFlowState::kPaused)	// if the update was paused due to the user playing the game when the update finished downloading, resume (EBIBUGS-26938)
				resume();
			else
			{
				ORIGIN_ASSERT(getFlowState() == ContentInstallFlowState::kPostTransfer);
                if (mProtocol)
				    mProtocol->Finalize();
			}
            return;
        }

        // Tell all the ready chunks to install
        installAllReadyChunks();
    }

    void ContentInstallFlowDiP::onUpdateFinalized()
    {
        if (!isUpdate())
            return;

        if (!mIsDynamicDownload)
            return;

        // If we're in the dynamic update finalization step
        if (mIsDynamicUpdateFinalize && getFlowState() == ContentInstallFlowState::kPostInstall)
        {
            // Go back to onPostInstall
            OnPostInstall();
        }
    }

    bool ContentInstallFlowDiP::checkIfRequiredChunksInstalled() const
    {
        for (QMap<int, DynamicDownloadChunkState>::const_iterator chunkIter = mChunkState.cbegin(); chunkIter != mChunkState.cend(); ++chunkIter)
        {
            const DynamicDownloadChunkState chunkState = chunkIter.value();

            // Only bother with required chunks
            if (chunkState.requirement != Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired)
                continue;

            if (chunkState.state != DynamicDownloadChunkState::kInstalled)
                return false;
        }

        return true;
    }

    bool ContentInstallFlowDiP::checkIfAllChunksInstalled() const
    {
        for (QMap<int, DynamicDownloadChunkState>::const_iterator chunkIter = mChunkState.cbegin(); chunkIter != mChunkState.cend(); ++chunkIter)
        {
            const DynamicDownloadChunkState chunkState = chunkIter.value();

            if (chunkState.state != DynamicDownloadChunkState::kInstalled)
                return false;
        }

        return true;
    }

    void ContentInstallFlowDiP::onDynamicChunkInstalled(int chunkId)
    {
        if (mIsDynamicDownload)
        {
            ORIGIN_LOG_EVENT << "Dynamic chunk " << chunkId << " installed.";

            DynamicDownloadChunkState& chunkState = mChunkState[chunkId];
            chunkState.state = DynamicDownloadChunkState::kInstalled;

            if (chunkState.requirement == Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired)
            {
                // Do we have all the required chunks?  Have we already run the installer
                if (mProtocol && checkIfRequiredChunksInstalled())
                {
                    if (!mInstallManifest.GetBool("DDinstallAlreadyCompleted"))
                    {
                        // If this is a dynamic update and the flow isn't yet running (we're probably still waiting for EULAs, etc), that means no required chunks have changed
                        if (isUpdate() && mProtocol->protocolState() == kContentProtocolInitialized)
                        {
                            // No need to do anything
                            ORIGIN_LOG_EVENT << "All required chunks installed, none changed, waiting to run touchup until transfer begins.";
                        }
                        else
                        {
                            ORIGIN_LOG_EVENT << "All required chunks complete; Installing";

                            emit dynamicDownloadRequiredChunksComplete();
                        }
                    }
                    else
                    {
                        // Make sure the protocol releases the non-required chunks to be downloaded
                        mProtocol->ActivateNonRequiredDynamicChunks();
                    }
                }
            }
        }
    }

    void ContentInstallFlowDiP::onProtocolFinished()
    {
        emit protocolFinished();
    }

    void ContentInstallFlowDiP::onProtocolInitialized()
    {
        emit protocolInitialized();
    }

    void ContentInstallFlowDiP::onProtocolResumed()
    {
        emit protocolResumed();
    }

    void ContentInstallFlowDiP::onProtocolPaused()
    {
        // A suspend was called prior to the flow reaching the transferring state. This method gets invoked
        // after the protocol has finished cleaning up from the suspend. The flow will have to resume from
        // the beginning.
        if (mProcessingSuspend)
        {
            mProcessingSuspend = false;
            stop();
        }

        //ITO flows need to persist some data in the protocol so don't delete it.
        if (mProtocol && !isUsingLocalSource())
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mProtocol->deleteLater();
            mProtocol = NULL;
        }

        emit protocolPaused();
    }

    void ContentInstallFlowDiP::onProtocolCannotFindSource(int currentDisc, int numDiscs, int wrongDisc, bool discEjected)
    {
        if( discEjected )
            emit protocolPaused();
        else
            emit protocolCannotFindSource(currentDisc, numDiscs, wrongDisc);
    }

    void ContentInstallFlowDiP::onProtocolSourceFound( bool discEjected )
    {
        if( discEjected )
            resume();

        emit discChanged(); // to hide the 'Insert Disc' dialog
    }

    bool ContentInstallFlowDiP::isUsingLocalSource() const
    {
        return mInstallManifest.GetBool("isLocalSource");
    }

    bool ContentInstallFlowDiP::isNonDipInstall() const
    {
        // Make sure we have a valid content configuration first
        if (getContent() && getContent()->contentConfiguration())
        {
            const Origin::Engine::Content::ContentConfigurationRef contentConfiguration = getContent()->contentConfiguration();

            // Check the catalog flag that enables this behavior
            bool updateNonDipFlag = contentConfiguration->updateNonDipInstall();

            // Check to see if a DiP manifest is cached
            LocalInstallProperties *dipManifest = mContent->localContent()->localInstallProperties();

            // If the update non-DiP flag is set and we have no DiP manifest, then we're upgrading a non-DiP game
            if (updateNonDipFlag && !dipManifest)
            {
                return true;
            }
        }

        // We aren't doing a non-DiP upgrade
        return false;
    }

    bool ContentInstallFlowDiP::isITO() const
    {
        return (mInstallManifest.GetBool("isITOFlow"));
    }

    bool ContentInstallFlowDiP::isDynamicDownload(bool *contentPlayableStatus) const
    {
        // Retrieve the current flow state
        ContentInstallFlowState flowState = getFlowState();

        bool isDD = false;

        // If we're not actively transferring, we may not have our boolean value set, so refer to the manifest
        if (flowState != ContentInstallFlowState::kTransferring)
        {
            isDD = mInstallManifest.GetBool("DynamicDownload");
        }
        else
        {
            isDD = mIsDynamicDownload;
        }

        // If the caller wishes to resolve playable status
        if (contentPlayableStatus != NULL)
        {
            if (isDD)
            {
                // We don't have localcontent available yet
                if (!mContent || !mContent->localContent())
                {
                    *contentPlayableStatus = false;
                }
                // No job is currently pending, so the game is playable if the game is installed
                else if (flowState == ContentInstallFlowState::kReadyToStart)
                {
                    *contentPlayableStatus = mContent->localContent()->installed();
                }
                // There is a pausing/canceling dynamic download, so we can't let the user play
                else if (flowState == ContentInstallFlowState::kPausing || flowState == ContentInstallFlowState::kCanceling)
                {
                    *contentPlayableStatus = false;
                }
                else
                {
                    // We are in some form of paused, starting or running, let them play if its installed
                    *contentPlayableStatus = (mInstallManifest.GetBool("DDinstallAlreadyCompleted") && mContent->localContent()->installed());
                }
            }
            else
            {
                *contentPlayableStatus = false;
            }
        }

        return isDD;
    }

    bool ContentInstallFlowDiP::isInstallProgressConnected() const
    {
        return mInstallProgressConnected;
    }

    bool ContentInstallFlowDiP::contentIsPdlc() const
    {
        return mContent && mContent->contentConfiguration()->isPDLC();
    }

	void ContentInstallFlowDiP::onNonTransferProgressUpdate(qint64 bytesProgressed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
	{
		//we only want to do this if we get this signal while the flow isn't transferring, which means is the crc pass from the repair flow or patch builder init progress
		if ((getFlowState() != ContentInstallFlowState::kTransferring) &&
			(getFlowState() != ContentInstallFlowState::kResuming))
		{
			onProgressUpdate(bytesProgressed, totalBytes, bytesPerSecond, estimatedSecondsRemaining);
		}
	}

} // namespace Downloader
} // namespace Origin
