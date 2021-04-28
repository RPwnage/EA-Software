#include <limits>
#include <QDateTime>
#include <QSettings>
#include <QFile>
#include <QDesktopServices>
#include <QtConcurrent>
#include <QMetaObject>
#include <time.h>
#include <algorithm>

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

#include "IGOSharedStructs.h"

#include "ContentProtocols.h"

#include "engine/content/NucleusEntitlementController.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/GamesController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/PlayFlow.h"
#include "services/common/DiskUtil.h"
#include "engine/downloader/DynamicDownloadController.h"
#include "engine/downloader/ContentServices.h"
#include "engine/igo/IGOController.h"
#include "engine/ito/InstallFlowManager.h"
#include "engine/login/User.h"
#include "engine/plugin/PluginManager.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/trials/TrialServiceResponse.h"
#include "services/common/VersionInfo.h"
#include "services/debug/DebugService.h"
#include "services/downloader/CRCMap.h"
#include "services/downloader/StringHelpers.h"
#include "services/escalation/IEscalationClient.h"
#include "services/log/LogService.h"
#include "services/platform/EnvUtils.h"
#include "services/platform/PlatformService.h"
#include "services/platform/TrustedClock.h"
#include "services/process/IProcess.h"
#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"
#include "services/settings/MigrationUtils.h"
#include "services/settings/SettingsManager.h"
#include "services/qt/QtUtil.h"

#ifdef ORIGIN_MAC
#include "services/process/ProcessOSX.h"
#include "services/escalation/IEscalationClient.h"
#include "services/escalation/IEscalationService.h"
#endif

#include "LocalInstallProperties.h"
#include "TelemetryAPIDLL.h"



using namespace Origin::Services;

namespace
{
    // How long we cache our "installed" flag for
    const unsigned int InstallCheckCacheMs = 5 * 60 * 1000;

    // These game need to run elevated for Windows 7

    const QStringList RunElevatedWhitelistedContentIds = (QStringList() <<
        //C&C:
        "70342" <<
        "70376" <<
        "70460" <<
        "71466" <<
        "71455" <<
        "1001095" <<
        "1001217" <<
        "1001277" <<
        "1001278" <<
        "1001287" <<
        "1001288" <<
        "1001289" <<
        "1001290" <<
        "1001291" <<
        "1001293" <<
        "1001294" <<
        "1001297" <<
        "1001298" <<
        "1001301" <<
        "1001302" <<
        "1001303" <<
        "1001304" <<
        "1001307" <<
        "1001308" <<
        "1001985" <<
        "1001986" <<
        "1001987" <<
        "1001988" <<
        //C&C Ultimate Collection:
        "1002272" <<
        "1002272_ww" <<
        "1002273" <<
        //C&C 3:
        "1007635" <<
        "1007873" <<
        "70299" <<
        "cnc3" <<
        "cnc3_ep1" <<
        "70354" <<
        //C&C 4:
        "cnc4_dd" <<
        "cnc4_ap" <<
        "cnc4_eu1" <<
        "cnc4_eu2" <<
        "cnc4_eu3" <<
        "cnc4_eu4" <<
        "cnc4_eu5" <<
        "70518" <<
        // All for SWTOR:
        "71065" <<
        "71236" <<
        "71185" <<
        "71074" <<
        "71199" <<
        "71235" <<
        "71198" <<
        "16952" <<
        "16954" <<
        "73058" <<
        "36654");

    const QString ChildManualDownloadsEmpty = "CHILD_MANUAL_DOWNLOAD_EMPTY";

    /// \brief Checks overrides to see if the given product ID has a Twitch blacklist override enabled.
    ///
    /// The syntax for enabling Twitch functionailty for a blacklisted title (in the EACore.ini) is:
    ///
    /// [QA]
    /// DisableTwitchBlacklist::OFB-EAST:49753=true
    ///
    /// \return True if the product ID has a blacklist override, false otherwise.
    bool isTwitchBlacklistOverride(const QString& productId)
    {
        QString productOverrideString(Origin::Services::SETTING_DisableTwitchBlacklist.name() + "::" + productId);

        return Origin::Services::readSetting(productOverrideString);
    }
}

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            void uninstallDLCUsingCRCMap(QString cacheFolderPath, QString gameInstallFolder, QString dlcInstallCheck);

            const unsigned int LocalContentSerializationMagic = 0xb25a145e;
            const QDataStream::Version LocalContentSerializationVersion = QDataStream::Qt_4_8;

            void LauncherDescription::Clear()
            {
                mLanguageToCaptionMap.clear();
                msParameters.clear();
                msPath.clear();
                mbExecuteElevated = false;
                mbRequires64BitOS = false;
                mbIsTimedTrial = false;
            }

            LocalContent::LocalContent(UserRef user, EntitlementRef entitlement)
                :mInstallFlow(NULL)
                ,mPlayFlow(NULL)
                ,mRunning(NULL)
                ,mInstalledLocale("")
                ,mPreviousInstalledLocale("")
                ,mInstalledVersion("-1")
                ,mFreeTrialStarted(false)
                ,mHasUnownedContent(false)
                ,mAddonInstallCheck("")
                ,mReleaseControlState(RC_Undefined)
                ,mReleaseControlStateChangeTimer(NULL)
                ,mReleaseControlExpiredTimer(NULL)
                ,mFreeTrialStartedCheckTimer(this)
                ,m_initialTrialDuration(-1)
                ,m_remainingTrialDuration(-1)
                ,m_currentTrialTimeSlice(-1)
                ,m_reconnectingTrialTimeLeft(-1)
                ,mNonMonitoredInstallInProgress(false)
                ,mPlayingOverride(false)
                ,mPlayableOverride(false)
                ,mUninstallCheckTimer(this)
                ,mInstalled(false)
                ,mNeedsRepair(false)
                ,mUser(user)
                ,m_Entitlement(entitlement)
                ,mLocalInstallProperties(NULL)
                ,mAuthorizationCode("")
                ,mAuthCodeRetrievalTimer(NULL)
                ,mUninstalling(false)
                ,mInitialInstalledTimestamp(0)
            {
                mFreeTrialStartedCheckTimer.setSingleShot(true);
                ORIGIN_VERIFY_CONNECT_EX(&mFreeTrialStartedCheckTimer, &QTimer::timeout,
                    this, &LocalContent::onFreeTrialStartedCheckTimeOut, Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(ContentOperationQueueController::currentUserContentOperationQueueController(), SIGNAL(headChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)),
                    this, SLOT(onHeadChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
            }

            UserRef LocalContent::user() const 
            { 
                return mUser.toStrongRef(); 
            }

            LocalContent::~LocalContent()
            {
                if (mAuthCodeRetrievalTimer)
                {
                    mAuthCodeRetrievalTimer->stop();
                    delete mAuthCodeRetrievalTimer;
                    mAuthCodeRetrievalTimer = NULL;
                }

                if (mInstallFlow)
                    mInstallFlow->deleteLater();

                delete mLocalInstallProperties;
            }

            void LocalContent::deleteInstallFlow()
            {
                if (!mInstallFlow)
                {
                    emit installFlowDeleted();
                    return;
                }

                Origin::Services::ThreadObjectReaper* reaper = new Origin::Services::ThreadObjectReaper(mInstallFlow, "deleteAsync");
                ORIGIN_VERIFY_CONNECT(reaper, SIGNAL(objectsDeleted()), this, SIGNAL(installFlowDeleted()));
                Downloader::ContentProtocols::RegisterWithInstallFlowThread(reaper);
                reaper->start();

                mInstallFlow = NULL;
            }

            LocalContent::States LocalContent::state() const
            {
                if (!contentConfiguration()->owned() && contentConfiguration()->purchasable() && !contentConfiguration()->previewContent())
                {
                    return NotPurchased;
                }

                // If mPlayingOverride or mPlayingOverride is set, the content is
                // content not wrapped in RtP.
                if (mPlayingOverride)
                {
                    return Playing;
                }
                if (mPlayableOverride)
                {
                    return ReadyToPlay;
                }

                if(mRunning)
                {
                    if(playing())
                    {
                        return Playing;
                    }
                    else
                    {
                        return Busy;
                    }
                }
                
                if(!contentConfiguration()->monitorInstall() && mNonMonitoredInstallInProgress)
                    return ReadyToPlay;

#ifdef ORIGIN_MAC
                // On Mac, we may encounter a race condition where we think the game is installed while the game
                // is actually installing - not sure what I would break if I were to move the switch(mInstallFlow)
                // block before the if (installed()), so I'm just going to check for this specific condition - since
                // our real problem is not to allow exiting the Client while running the touchup installer
                if (mInstallFlow)
                {
                    Downloader::ContentInstallFlowState flowState = mInstallFlow->getFlowState();
                    if (flowState == Downloader::ContentInstallFlowState::kInstalling)
                    {
                        return Installing;
                    }
                }
#endif
                    
                if(installed())
                {
                    //we want the true state and not take ORC into account
                    if(playable(false))
                    {
                        return ReadyToPlay;
                    }
                    else
                    {
                        if( mInstallFlow && mInstallFlow->isInstallProgressConnected() )
                        {
                            return Installing;
                        }
                        else
                        {
                            return Installed;
                        }
                    }
                }

                if (mInstallFlow)
                {
                    switch (mInstallFlow->getFlowState())
                    {
                    case Downloader::ContentInstallFlowState::kError:
                    case Downloader::ContentInstallFlowState::kReadyToStart:
                        {
                            if(releaseControlState() == RC_Unreleased)
                            {
                                return Unreleased;
                            }

                            //we want the true state and not take ORC into account, or online into account
                            if(inDownloadState())
                            {
                                return ReadyToDownload;
                            }
                        }
                    case Downloader::ContentInstallFlowState::kInitializing:
                    case Downloader::ContentInstallFlowState::kPreTransfer:
                    case Downloader::ContentInstallFlowState::kPendingInstallInfo:
                    case Downloader::ContentInstallFlowState::kPendingEulaLangSelection:
                    case Downloader::ContentInstallFlowState::kPendingEula:
                    case Downloader::ContentInstallFlowState::kResuming:
                        {
                            return Preparing;
                        }
                    case Downloader::ContentInstallFlowState::kPendingDiscChange:
                    case Downloader::ContentInstallFlowState::kPaused:
                    case Downloader::ContentInstallFlowState::kPausing:
                        {
                            return Paused;
                        }
                    case Downloader::ContentInstallFlowState::kTransferring:
                        {
                            return Downloading;
                        }
                    case Downloader::ContentInstallFlowState::kPostTransfer:
                        {
                            return FinalizingDownload;
                        }
                    case Downloader::ContentInstallFlowState::kUnpacking:
                        {
                            return Unpacking;
                        }
                    case Downloader::ContentInstallFlowState::kDecrypting:
                        {
                            return Decrypting;
                        }
                    case Downloader::ContentInstallFlowState::kPreInstall:
                    case Downloader::ContentInstallFlowState::kReadyToInstall:
                        {
                            return ReadyToInstall;
                        }
                    case Downloader::ContentInstallFlowState::kInstalling:
                        {
                            return Installing;
                        }

                    case Downloader::ContentInstallFlowState::kCompleted:
                        {
                            return ReadyToPlay;
                        }
                    default:
                        return Undefined;

                    }
                }
                else 
                {
                    // We have no install flow; likely the content isn't configured to be downloadable yet
                    return Unreleased;
                }
            }

            QSharedPointer<PlayFlow> LocalContent::playFlow()
            {
                return mPlayFlow;
            }

            Downloader::IContentInstallFlow * LocalContent::installFlow()
            {
                return mInstallFlow;
            }

            bool LocalContent::inDownloadState() const
            {
                //this function is just used by the state() function to help determine state
                //this should only used internally
                //the outward facing function is "downloadable"

                if(installed(true))
                {
                    return false;
                }

                if (!mInstallFlow)
                {
                    // Need an installFlow to download
                    return false;
                }

                if (mInstallFlow->isActive())
                {
                    return false;
                }
                
                return true;
            }

            LocalContent::DownloadableStatus LocalContent::downloadableStatus() const
            {
                UserRef user = mUser.toStrongRef();
                if (!user) 
                    return DS_NotDownloadable;

                bool online = Origin::Services::Connection::ConnectionStatesService::isUserOnline(user->getSession());
                if (!online)
                    return DS_NotOnline;

                const qint64 timeLeft = trialTimeRemaining(true);
                if (contentConfiguration()->isFreeTrial() && timeLeft <= 0 && timeLeft != TrialError_LimitedTrialNotStarted)
                    return DS_ReleaseControl;

                if (!contentConfiguration()->owned() && !contentConfiguration()->previewContent())
                    return DS_NotOwned;

                if (!contentConfiguration()->downloadable())
                    return DS_NotDownloadable;

                if(!contentConfiguration()->platformEnabled())
                    return DS_WrongPlatform;

                if (!contentConfiguration()->isSupportedByThisMachine())
                    return DS_WrongArchitecture;

                if (!contentConfiguration()->monitorInstall() && mNonMonitoredInstallInProgress)
                    return DS_Busy;

                if(contentConfiguration()->isEntitledFromSubscription())
                {
                    // Has the user's subscription offline play expired?
                    if(Engine::Subscription::SubscriptionManager::instance()->offlinePlayRemaining() <= 0)
                        return DS_SubscriptionControl;

                    // Has the user's subscription membership expired?
                    if( !Engine::Subscription::SubscriptionManager::instance()->isActive())
                        return DS_SubscriptionControl;

                    // Has the entitlement retired from the subscriptions?
                    if (Services::TrustedClock::instance()->nowUTC().secsTo(contentConfiguration()->originSubscriptionUseEndDate()) <= 0)
                        return DS_SubscriptionControl;
                }

                // Permissions and preview content trump Origin RC. 1102 and 1103 can both install
                // although 1102 doesn't bypass the download date - it only allows "downloading" via .ini download path override.
                // Allow download of preview content with a local override, regardless of permissions.  This is necessary because
                // 1102 implies that the user has an entitlement, but game teams will need to test preview DLC download without one.
                if(mReleaseControlState == RC_Unreleased && 
                   contentConfiguration()->originPermissions() <= Origin::Services::Publishing::OriginPermissionsNormal &&
                   !(contentConfiguration()->previewContent() && contentConfiguration()->hasOverride()))
                {
                    return DS_ReleaseControl;
                }

                if(mReleaseControlState == RC_Expired)
                {
                    return DS_ReleaseControl;
                }

                if(!inDownloadState())
                {
                    return DS_Busy;
                }

                // For normal games, we just blindly ask the server for the live build.
                // For plug-ins, we need this information up front to determine compatibility,
                // which means we also need to calculate which build is live (see CreatePluginSoftwareBuildMap)
                // If we didn't find a live build and have no override, we should disallow downloading.
                if(contentConfiguration()->isPlugin() && contentConfiguration()->liveBuildId().isEmpty() && 
                    !contentConfiguration()->hasOverride() && contentConfiguration()->buildIdentifierOverride().isEmpty())
                {
                    return DS_NotDownloadable;
                }

                // PDLC can parent to multiple base games.  If one just complete the others may not have updated their installState cache yet
                // so we'll need check all parents, or force the update check.
                if(contentConfiguration()->isPDLC() && !isAnyParentInstalled())
                {
                    return DS_BaseGameNotInstalled;
                }

                return DS_Downloadable;
            }

            bool LocalContent::isAnyParentInstalled(bool forceUpdate, bool parentInQueueOK) const
            {
                if (!entitlement().isNull())
                {
                    foreach(Origin::Engine::Content::EntitlementRef parent, entitlement()->parentList())
                    {
                        if (parent->localContent() && (parent->localContent()->installed(forceUpdate) ||
                            (parentInQueueOK && parent->localContent()->installInProgressQueued())))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            bool LocalContent::isAnyParentPlayable() const
            {
                if (!entitlement().isNull())
                {
                    foreach(Origin::Engine::Content::EntitlementRef parent, entitlement()->parentList())
                    {
                        if(parent->localContent())
                        {
                            LocalContent::PlayableStatus status = parent->localContent()->playableStatus();
                            if(status == PS_Playable || status == PS_Playing)
                            {
                                return true;
                            }
                        }
                    }
                }

                return false;
            }

            bool LocalContent::download(QString installLocale /*en_US */, int download_flags , QString srcFile /*""*/)
            {
                bool isITO = download_flags & kDownloadFlag_isITO;

                // OFM-4779: Some DLC can only be download when the base game is up to date.
                // Is DLC && Needs parent to be updated && parent has an update && parent isn't currently updating
                if(entitlementRef()->parent()
                   && contentConfiguration()
                   && (contentConfiguration()->originDisplayType() == Services::Publishing::OriginDisplayTypeAddon || contentConfiguration()->originDisplayType() == Services::Publishing::OriginDisplayTypeExpansion)
                   && contentConfiguration()->softwareBuildMetadata().mbDLCRequiresParentUpToDate
                   && entitlementRef()->parent()->localContent()
                   && entitlementRef()->parent()->localContent()->updateAvailable()
                   && entitlementRef()->parent()->localContent()->installFlow()
                   && entitlementRef()->parent()->localContent()->installFlow()->operationType() != Downloader::kOperation_Update)
                {
                    emit error(entitlementRef(), ErrorParentNeedsUpdate);
                    return false;
                }

                // Check the content config to make sure this piece of content is available for this platform, and that the OS will support it
                if (!contentConfiguration()->isSupportedByThisMachine())
                {
                    return false;
                }

                // Do not allow downloading if entitlement has 1102 permission but no download override
                // NOTE: Installing from media (i.e., ITO) counts as a download override

                if( releaseControlState() != RC_Released && 
                    releaseControlState() != RC_Preload &&
                    contentConfiguration()->originPermissions() == Origin::Services::Publishing::OriginPermissions1102 &&
                    !contentConfiguration()->hasOverride() &&
                    !isITO )
                {
                    emit error(entitlementRef(), ErrorForbidden);
                    return false;
                }

                deleteCacheFolderIfUninstalled();
                
                //since this is a new download lets reset the value
                mInstalledVersion = QString("-1");
                
                Downloader::IContentInstallFlow::ContentInstallFlags flags = Downloader::IContentInstallFlow::kNone;
                if(isITO)
                {
                    flags = static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags | Downloader::IContentInstallFlow::kITO);
                    
                    if (!srcFile.isEmpty())
                        flags = static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags | Downloader::IContentInstallFlow::kLocalSource);

                    if (download_flags & kDownloadFlag_eulas_accepted)
                        flags = static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags | Downloader::IContentInstallFlow::kEulasAccepted);
                    if (download_flags & kDownloadFlag_default_install)
                        flags = static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags | Downloader::IContentInstallFlow::kDefaultInstall);
                }

                if (download_flags & kDownloadFlag_try_to_move_to_top)
                    flags = static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags | Downloader::IContentInstallFlow::kTryToMoveToTop);

                bool downloadStartSuccess = beginInstallFlow(installLocale, srcFile, flags);

                if(downloadStartSuccess)
                {
                    mInstalledLocale = installLocale;
                    if (contentConfiguration()->dip())
                    {
                        mInstalledFolder = mInstallFlow->getDownloadLocation();
                    }
                    else
                    {
                        mInstalledFolder = nonDipInstallerPath();
                    }
                }

                return downloadStartSuccess;
            }

            bool LocalContent::resumeDownload(bool silent /*= false*/)
            {
                bool downloadResumed = false;

				// must protect against user logging out which results in a null queueController
				Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();

                if (mInstallFlow && mInstallFlow->canResume() && queueController && queueController->resume(entitlementRef()))
                {
                    bool canResume = true;

                    ContentController* contentController = ContentController::currentUserContentController();

                    // Make sure the content folders are valid before allowing any resumption
                    if(!contentController->validateContentFolder(contentConfiguration()->dip()))
                    {
                        return false;
                    }

                    //if this is a pdlc download and the parent is installed then we can resume as well since its assumed for now that
                    //all pdlc are downloaded into separate subfolders (we already do a similar check when starting a download)
                    EntitlementRef e = entitlement();
                    bool isPdlcAndParentInstalled = contentConfiguration()->isPDLC() &&
                        e && !e->parent().isNull() && e->parent()->localContent()->installed();

                    if (contentConfiguration()->dip() && !isPdlcAndParentInstalled)
                    {
                        QString folderPath(mInstallFlow->getDownloadLocation());

                        QList<EntitlementRef> entitlementsAccessingFolder = contentController->getEntitlementsAccessingFolder(folderPath);

                        // Make sure other entitlements are not active in the install folder.
                        for (QList<EntitlementRef>::const_iterator entitlement = entitlementsAccessingFolder.begin();
                            entitlement != entitlementsAccessingFolder.end() && canResume; ++entitlement)
                        {
                            Downloader::IContentInstallFlow* flow = (*entitlement)->localContent()->installFlow();
                            if (flow)
                            {
                                // We don't care if we are the only ones accessing the folder
                                bool thisIsMe = (*entitlement)->contentConfiguration()->productId().compare(
                                    contentConfiguration()->productId(), Qt::CaseInsensitive) == 0;

                                // flow->isActive() returns true for states kPaused and kReadyToInstall, but content
                                // in those states are not currently accessing the folder
                                Downloader::ContentInstallFlowState state = flow->getFlowState();
                                bool activelyAccessingFolder = flow->isActive() && 
									!(state == Downloader::ContentInstallFlowState::kPaused ||
									  state == Downloader::ContentInstallFlowState::kEnqueued ||
                                      state == Downloader::ContentInstallFlowState::kReadyToInstall);

                                canResume = !activelyAccessingFolder || thisIsMe;
                                if (!canResume)
                                {
                                    ORIGIN_LOG_EVENT << "(" << contentConfiguration()->productId() << ") Cannot resume download, folder=(" << folderPath << ") already in use by (" << (*entitlement)->contentConfiguration()->productId();
                                    break;
                                }
                            }
                        }

                        if (!canResume && !silent)
                        {
                            emit error(entitlementRef(), ErrorFolderInUse);
                        }
                    }

                    if (canResume)
                    {
                        // Mark this flow active so that the appropriate error handlers get hooked up
                        emit flowIsActive(contentConfiguration()->productId(), true);

                        // Tell the flow whether or not we're silent (auto resumes, etc, are silent)
                        mInstallFlow->setSuppressDialogs(silent);

                        mInstallFlow->resume();
                        downloadResumed = true;
                    }
                      
                }
                return downloadResumed;
            }

            void LocalContent::install()
            {
                if(mInstallFlow)
                {
                    if(mInstallFlow->isActive())
                    {
                        if(state() != ReadyToInstall)
                            ORIGIN_LOG_WARNING << "InstallFlow not in the ready to install state";

                        mInstallFlow->resume();
                    }
                    else
                    {
                        
                        //clear this flag if we're starting a new flow to get to the install state
                        if(!contentConfiguration()->monitorInstall())
                        {
                            mNonMonitoredInstallInProgress = false;
                        }

                        QString srcFile;
                        beginInstallFlow(mInstalledLocale, srcFile, Downloader::IContentInstallFlow::kNone);
                    }
                }
            }

            bool LocalContent::installable() const
            {
                return (state() == ReadyToInstall);
            }

            bool LocalContent::installed(bool forceUpdate /* = false*/) const
            {
                return (installState(forceUpdate) == ContentInstalled);
            }

            bool LocalContent::installInProgressQueued() const
            {
                // if this entitlement has an active install flow and is on the queue, then consider this an installation in progress.
                // this function is basically used to determine if a PDLC can be added to the queue before the parent is completely installed.
                if (mInstallFlow && mInstallFlow->isActive())
                {
                    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
                    if (queueController)
                    {
                        int index = queueController->indexInQueue(entitlementRef());
                        ORIGIN_LOG_EVENT << "installInProgressQueued checking: " << index;
                        if (index > -1)
                            return true;   
                    }
                }

                return false;
            }

            LocalContent::InstallState LocalContent::installState(bool forceUpdate /* = false*/) const
            {
                if (contentConfiguration()->isNonOriginGame())
                {
                    mInstalled = QFile::exists(contentConfiguration()->installCheck());
                    mInstalled |= (contentConfiguration()->protocolUsedAsExecPath() == true);
                    return mInstalled ? ContentInstalled : ContentNotInstalled;
                }
                
                // If content is not wrapped in RtP, we need to fake
                // install check success to get the the proper UI indications
                // when the content is externally started.
                if (mPlayingOverride || mPlayableOverride || contentConfiguration()->isBrowserGame())
                {
                    return ContentInstalled;
                }

                if (!mAddonInstallCheck.isEmpty() && contentConfiguration()->isAddonOrBonusContent())
                {
                    return QFile::exists(mAddonInstallCheck) ? ContentInstalled : ContentNotInstalled;
                }

                if(!contentConfiguration()->installCheck().isEmpty())
                {
                    // Scope mutex locker so it's only locked while refreshing manifest 
                    QMutexLocker locker(&mInstallStateLock);

                    // Can we use our cached value?
                    const bool canUseCached = !forceUpdate &&
                        mLastInstallCheck.isValid() &&
                        !mLastInstallCheck.hasExpired(InstallCheckCacheMs);

                    if (!canUseCached)
                    {
                        //ORIGIN_LOG_DEBUG << "Refreshing install state for ProductID: " << contentConfiguration()->productId() << " forceUpdate:" << forceUpdate << " mLastInstallCheck.isValid():" << mLastInstallCheck.isValid() << "InstallCheck:" << contentConfiguration()->installCheck();

                        QString path;

                        path = PlatformService::localFilePathFromOSPath(contentConfiguration()->installCheck());

                        bool currentlyInstalled = QFile::exists(path);

                        mExecutablePathIsExistingFolderPath = false;
                        if(!currentlyInstalled)
                        {
                            //if the executable path is a folder, lets just see if that folder exists, if so we deem it as installed
                            currentlyInstalled = isPathANormalFolder(executablePath());

                            //lets cache this value since the "path exist" call is expensive
                            mExecutablePathIsExistingFolderPath = currentlyInstalled;
                        }
                        mLastInstallCheck.restart();

                        // Unlock so we don't deadlock on handleProgressChanged() below.
                        locker.unlock();

                        if (mInstalled != currentlyInstalled)
                        {
                            mInstalled = currentlyInstalled;

                            // This is a bit noisy as its possible we just hadn't set mInstalled before
                            // Emitting extra stateChanged() signals should be harmless 
                            const_cast<LocalContent*>(this)->stateChanged(entitlementRef());
                        }
                    }

                    return mInstalled ? ContentInstalled : ContentNotInstalled;
                }

                return NoInstallCheck;
            }

            bool LocalContent::isNonMonitoredInstalling()
            {
                return mNonMonitoredInstallInProgress;
            }

            void LocalContent::setAddonInstallCheck(const QString& installCheck)
            {
                if (!installCheck.isEmpty())
                {
                    mAddonInstallCheck = installCheck;
                    mLastInstallCheck.invalidate();
                }
                else
                {
                    mAddonInstallCheck.clear();
                }
            }
                
            bool LocalContent::unInstallable() const
            {
                const ContentConfigurationRef config = contentConfiguration();
                bool success = installed() && mInstallFlow && mInstallFlow->operationType() == Downloader::kOperation_None && !config->isNonOriginGame() && !config->isBrowserGame() && !config->isPlugin();

                // for case where game is playable and a download is paused
                if (!success && mInstallFlow && mInstallFlow->isDynamicDownload() && mInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kPaused && playableStatus() == PS_Playable)
                {
                    success = true;
                }
                
#ifdef ORIGIN_MAC
                // For Mac, we only want to uninstall DiP games as we have no idea what an installer for any other downloadable content could do.
                success &= config->dip();
                success &= !config->isPDLC();
#endif
                return success;
            }

            // Keep the spaces between quotes. Because file names may have spaces, we cannot use ' ' as a seperator.
            QStringList parseCommandLineWithQuotes(QString commandLine)
            {
                int     outsideQuotes = 1;  // start off being outsde a pair of quotes
                for (QChar *data = commandLine.data(); *data != 0; data++)
                {
                    if (*data == '"')
                        outsideQuotes++;    // toggle low bit to keep track of being inside or outside a pair of quotes
                    else if ((*data == ' ') && (outsideQuotes & 1))
                        *data = 0x1F;    // convert the space to 0x1F (unit seperator character) if not inside quotes
                }
                QStringList argv = commandLine.split(0x1F, QString::SkipEmptyParts); // now we can use 0x1F to seperate the args
                return argv;
            }


            bool LocalContent::hasUninstaller() const
            {
                if (mLocalInstallProperties)
                {
                    // If there is an explicit uninstaller, it is uninstallable
                    if (!mLocalInstallProperties->GetUninstallPath().isEmpty())
                        return true;

                    // If this is PDLC, we need to see if the catalog is preventing us from uninstalling
                    if (contentConfiguration()->isPDLC())
                    {
                        return contentConfiguration()->enableDLCUninstall();
                    }

                    return false;
                }
                
                return false;
            }

            bool LocalContent::unInstall(bool silent)
            {
                bool success = false;
                bool usedGameUninstall = false;
#ifdef ORIGIN_PC
                // for case where game is playable and a download is paused
                if (mInstallFlow && mInstallFlow->isDynamicDownload() && mInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kPaused && playableStatus() == PS_Playable)
                {
                    mInstallFlow->cancel(true);
                }
                else
                {
                    if ( mLocalInstallProperties )
                    {
                        QString sUninstallCommandLine;
                        QString sUninstall = mLocalInstallProperties->GetUninstallPath();
                        if (!sUninstall.isEmpty())
                        {
                            ORIGIN_LOG_EVENT << "Found Uninstall Configuration: " << sUninstall;

                            // It's possible that the DiP Manifest specified an uninstall command directly, not one referenced in the registry.  If so, try and use that.
                            int rBracket = -1;
                            if (sUninstall.left(1) == "[" && ((rBracket = sUninstall.indexOf("]")) != -1))
                            {
                                QString sSystemPathPortion = sUninstall.left(rBracket+1);
                                QString sArgsPortion = sUninstall.right(sUninstall.length()-(rBracket+1));

                                ORIGIN_LOG_EVENT << "Path: " << sSystemPathPortion << " Args: " << sArgsPortion;

#ifdef Q_OS_WIN
                                sUninstallCommandLine = PlatformService::dirFromRegistry(sSystemPathPortion, true);    // true = use the 64 bit path

                                // If it's still empty, try the 32 bit registry location
                                if (sUninstallCommandLine.isEmpty())
                                    sUninstallCommandLine = PlatformService::dirFromRegistry(sSystemPathPortion);    // true = use the 64 bit path
#endif
#ifdef ORIGIN_MAC
                                sUinstallCommandLine = PlatformService::dirFromBundleCatalogFormat(sSystemPathPortion);
#endif
                                // Append the args if any were present (stuff after the registry brackets)
                                if (!sArgsPortion.isEmpty())
                                {
                                    sUninstallCommandLine.append(sArgsPortion);
                                }
                            }
                            else
                                sUninstallCommandLine = sUninstall;
                        }
                        else if (!mAlternateUninstallPath.isEmpty())
                        {
                            // Using alternate legacy DLC-style uninstall
                            sUninstallCommandLine = QString("\"%1\" uninstall_pdlc -autologging").arg(mAlternateUninstallPath);
                        }

                        QString sCommand;
                        QString sParameters;
                        if (!sUninstallCommandLine.isEmpty())
                        {
                            ORIGIN_LOG_EVENT << "Uninstall command: \"" << sUninstallCommandLine << "\"";

                            // Parse out the executable and the parameters
                            QStringList argv = parseCommandLineWithQuotes(sUninstallCommandLine);
                            sCommand = argv[0];
                            // Now grab the rest of the parameters since that's what shellexecute wants
                            for (int i = 1; i < argv.size(); i++)
                                sParameters += argv[i] + " ";

                            // If silent (or the content is DLC, which we always prompt for)
                            if (silent || contentConfiguration()->isPDLC())
                            {
                                sParameters += "-quiet";
                            }

                            success = true;

                            usedGameUninstall = true;
                        }

                        // If we're using the 'alternate' DLC uninstall method, we need to finish up by going over the CRC map if it exists (this will run later)
                        bool doManualDlcUninstall = false;
                        if (mLocalInstallProperties && mLocalInstallProperties->GetUninstallPath().isEmpty() && contentConfiguration()->dip() && contentConfiguration()->isPDLC())
                        {
                            doManualDlcUninstall = true;

                            success = true;

                            usedGameUninstall = true;
                        }

                        if (usedGameUninstall)
                        {
                            UninstallTask *uninstallTask = new UninstallTask(contentConfiguration()->productId(), sCommand, sParameters);
                            ORIGIN_VERIFY_CONNECT_EX(uninstallTask, SIGNAL(finished()), this, SLOT(uninstallFinished()), Qt::QueuedConnection);
                            uninstallTask->setAutoDelete(true);
                            mUninstalling = true;

                            // If we need to do the manual DLC uninstall step after, add the args
                            if (doManualDlcUninstall)
                            {
                                // Get game install folder (DLC CRC map entries are relative to the base game root folder)
                                QString gameInstallFolder = dipInstallPath();
                                QString pdlcInstallDirectory = contentConfiguration()->installationDirectory();  // This is only used by relatively obsolete DLC!  (This shouldn't be used for new DLC)
                                Engine::Content::EntitlementRef parent = entitlement()->parent();
                                if (!parent.isNull())
                                {
                                    gameInstallFolder = PlatformService::localDirectoryFromOSPath(parent->contentConfiguration()->installCheck());
                                }

                                // Append the PDLC install directory (obsolete DLC use this)
                                if (!pdlcInstallDirectory.isEmpty())
                                {
                                    gameInstallFolder.append(pdlcInstallDirectory);
                                }

                                uninstallTask->setManualDLCUninstallArgs(cacheFolderPath(), gameInstallFolder, contentConfiguration()->installCheck());
                            }

                            QThreadPool::globalInstance()->start(uninstallTask);
                        }
                    }

                    if (!usedGameUninstall && !silent)
                        Services::OpenAddOrRemovePrograms();
                }
                
#elif defined(ORIGIN_MAC)
                
                if (mLocalInstallProperties)
                {
                    QString sUninstall = mLocalInstallProperties->GetUninstallPath();
                    if (!sUninstall.isEmpty())
                    {
                        // TODO: implement once we have an example of the format used in that field!
                        usedGameUninstall = true;
                        ORIGIN_LOG_WARNING << "FOUND AN UNINSTALL PATH FOR MAC - NEED IMPL";
                    }
                }

                if (mLocalInstallProperties && mLocalInstallProperties->GetUninstallPath().isEmpty() && contentConfiguration()->dip() && contentConfiguration()->isPDLC())
                {
                    // Empty command and parameters (not using an uninstaller exe)
                    UninstallTask *uninstallTask = new UninstallTask(contentConfiguration()->productId(), "", "");
                    ORIGIN_VERIFY_CONNECT_EX(uninstallTask, SIGNAL(finished()), this, SLOT(uninstallFinished()), Qt::QueuedConnection);
                    uninstallTask->setAutoDelete(true);
                    mUninstalling = true;

                    // Get game install folder (DLC CRC map entries are relative to the base game root folder)
                    QString gameInstallFolder = dipInstallPath();
                    Engine::Content::EntitlementRef parent = entitlement()->parent();
                    if (!parent.isNull())
                    {
                        gameInstallFolder = PlatformService::localDirectoryFromOSPath(parent->contentConfiguration()->installCheck());
                    }

                    uninstallTask->setManualDLCUninstallArgs(cacheFolderPath(), gameInstallFolder, contentConfiguration()->installCheck());

                    QThreadPool::globalInstance()->start(uninstallTask);

                    // Skip the rest
                    usedGameUninstall = true;
                }
                
                if (!usedGameUninstall)
                {
                    QString escalationReasonStr = "uninstall game";
                    int escalationError = 0;
                    QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                    if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                        return false;
                    
                    // Is it missing from the cache?
                    QString tmpFolder;
                    if(mInstalledFolder.isEmpty())
                        tmpFolder = Origin::Services::PlatformService::localDirectoryFromOSPath(contentConfiguration()->installCheck());
                    
                    else
                        tmpFolder = mInstalledFolder;
                    
                    // Make sure we're working with a bundle
                    QString installFolder = Origin::Services::PlatformService::getContainerBundle(tmpFolder);
                    if (installFolder.isEmpty())
                    {
                        ORIGIN_LOG_ERROR << "Unable for find valid bundle for uninstall (" << tmpFolder << ")";
                        return false;
                    }
                    
                    int errorCode = escalationClient->deleteDirectory(installFolder);
                    if (Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "deleteDirectory", errorCode, escalationClient->getSystemError()))
                    {
                        success = true;
                        ORIGIN_LOG_DEBUG << "Uninstall(deletion) of folder '" << installFolder << "' successful";
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "Failed to uninstall(delete) folder '" << installFolder << "' (" << errorCode << ")";
                    }
                }
#endif
                
                mLastInstallCheck.invalidate();
                
                return success;
            }

            QString LocalContent::getLaunchAdditionalEnvironmentVars ()
            {
                QString sAddlEnvVars;
                
                // NOTE: These env vars needs to match the env var set in awc.dll, Launch2
                const QString CORE_AUTH_TOKEN_ENV_VARIABLE ( "EALaunchUserAuthToken" );
                const QString NUCLEUS_AUTH_TOKEN_ENV_VARIABLE ( "EAGenericAuthToken" );
                const QString AUTH_CODE_ENV_VARIABLE ("EAAuthCode");        //ID2 authorization code retrieved with game's client_id
                const QString LAUNCHCODE_ENV_VARIABLE ( "EALaunchCode" );
                const QString RTP_ENV_VARIABLE( "EARtPLaunchCode" );
                const QString ORIGIN_OFFLINEMODE_ENV_VARIABLE ( "EALaunchOfflineMode" );
                const QString ORIGIN_ENVIRONMENT_NAME_ENV_VARIABLE ( "EALaunchEnv" );
                const QString ORIGIN_USER_NAME_ENV_VARIABLE ( "EALaunchEAID" );
                const QString ORIGIN_FREETRIAL_ENV_VARIABLE ( "EAFreeTrialGame" );
                const QString ORIGIN_LICENSE_TOKEN ("EALicenseToken");
                const QString OIG_TELEMETRY_TIMESTAMP(OIG_TELEMETRY_TIMESTAMP_ENV_NAME_A);
                const QString OIG_TELEMETRY_PRODUCTID(OIG_TELEMETRY_PRODUCTID_ENV_NAME_A);
                const QString OIG_LOG_PATH("IGOLogPath");

                //if Origin was launched from Battlelog via Firefox or Chrome, it seems to be passing back into Origin
                //the EALaunch variables that we passed to Battlelog
                //make sure those are removed first if it exists
                Origin::Services::IProcess::removeEnvironmentVar (CORE_AUTH_TOKEN_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (NUCLEUS_AUTH_TOKEN_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (AUTH_CODE_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (LAUNCHCODE_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (RTP_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (ORIGIN_OFFLINEMODE_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (ORIGIN_ENVIRONMENT_NAME_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (ORIGIN_USER_NAME_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (ORIGIN_FREETRIAL_ENV_VARIABLE);
                Origin::Services::IProcess::removeEnvironmentVar (ORIGIN_LICENSE_TOKEN);
                Origin::Services::IProcess::removeEnvironmentVar (OIG_TELEMETRY_TIMESTAMP);
                Origin::Services::IProcess::removeEnvironmentVar (OIG_TELEMETRY_PRODUCTID);
#ifndef ORIGIN_MAC
                Origin::Services::IProcess::removeEnvironmentVar (OIG_LOG_PATH);
#endif

                // This is an attempt the prevention of users adding Entitlements as NOG's
                // This added check fixes EBIBUGS-21126 and helps prevent EBIBUGS-27562
                if (!contentConfiguration()->isNonOriginGame())
                {
                    QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());

                    sAddlEnvVars.append( QString("%1=%2").arg(CORE_AUTH_TOKEN_ENV_VARIABLE).arg(accessToken) );

                    //QChar(0xFF) is the separator between each environment variable
                    if(!accessToken.isEmpty())
                        sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(NUCLEUS_AUTH_TOKEN_ENV_VARIABLE).arg(accessToken));

                    if (!mAuthorizationCode.isEmpty())
                        sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(AUTH_CODE_ENV_VARIABLE).arg(mAuthorizationCode));

                    // Game serial/cd key
                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(LAUNCHCODE_ENV_VARIABLE).arg(contentConfiguration()->cdKey()));

                    // RTP
                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(RTP_ENV_VARIABLE).arg(getRTPHandshakeCode()));

                    // Is Origin in Offline Mode?
                    UserRef u = mUser.toStrongRef();
                    bool online = u && Origin::Services::Connection::ConnectionStatesService::isUserOnline (u->getSession());
                    QString sOriginOfflineModeValue = online ? "false" : "true";
                    QString sOriginIsFreeTrialValue = contentConfiguration()->isFreeTrial() ? "true" : "false";

                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(ORIGIN_FREETRIAL_ENV_VARIABLE).arg(sOriginIsFreeTrialValue));
                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(ORIGIN_OFFLINEMODE_ENV_VARIABLE).arg(sOriginOfflineModeValue));
                    sAddlEnvVars.append(QString("%1ContentId=%2").arg(QChar(0xFF)).arg(contentConfiguration()->contentId()));
                    sAddlEnvVars.append(QString("%1EAConnectionId=%2").arg(QChar(0xFF)).arg(contentConfiguration()->productId()));
                    sAddlEnvVars.append(QString("%1OriginSessionKey=%2").arg(QChar(0xFF)).arg(Origin::Services::readSetting(Origin::Services::SETTING_OriginSessionKey).toString()));
                    sAddlEnvVars.append(QString("%1EAGameLocale=%2").arg(QChar(0xFF)).arg(installedLocale()));

                    // Pass along current server environment for Access game usage
                    QString sEnvironmentName = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Origin::Services::Session::SessionRef());
                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(ORIGIN_ENVIRONMENT_NAME_ENV_VARIABLE).arg(sEnvironmentName));

                    if (user())
                        sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(ORIGIN_USER_NAME_ENV_VARIABLE).arg(user()->eaid()));

                    sAddlEnvVars.append(QString("%1%2=%3,%4").arg(QChar(0xFF)).arg(ORIGIN_LICENSE_TOKEN).arg(accessToken).arg(entitlement()->contentConfiguration()->productId()));

                    //  For the Microsoft Surface Pro we need to set environment variable to disable the Virtual PC check.
                    if (GetTelemetryInterface()->IsComputerSurfacePro() == true)
                        sAddlEnvVars.append(QString("%1%2").arg(QChar(0xff)).arg(QString("EAAccess_AllowVMs=true")));
                }


                // Identifiers used by OIG telemetry
                time_t timestamp = time(NULL);
                sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(OIG_TELEMETRY_TIMESTAMP).arg(timestamp));
                sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(OIG_TELEMETRY_PRODUCTID).arg(entitlement()->contentConfiguration()->productId()));
                
#ifndef ORIGIN_MAC
                // set environment variable for IGO logging (can't call SHGetFolderPath from inside injected dll)
                WCHAR commonDataPath[MAX_PATH];
                if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, commonDataPath)))
                {
                    sAddlEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(OIG_LOG_PATH).arg(QString::fromWCharArray(commonDataPath)));
                }
#endif

                return sAddlEnvVars;
            }

            // Content not wrapped in RtP will call this method through
            // LSX when externally started to force the UI to show it in
            // a playing state.
            void LocalContent::playNonRtp()
            { 
                mPlayingOverride = true;
                mPlayableOverride = false;

                // We've started
                emit playStarted(entitlementRef(), true);

                // forces the ui to update its display of this content
                emit stateChanged(entitlementRef());
            }

            // Externally started content not wrapped in RtP will call this method through
            // LSX when shutdown to force the UI to show it in
            // a ready to play state.
            void LocalContent::stopNonRtpPlay()
            {
                // Only handle this when the game is not started through Rtp otherwise let process monitoring take care of this.
                if(mPlayingOverride)
                {
                    mPlayingOverride = false;

                    //If the install check for this non RtP content is valid, we don't need to set
                    //the playable override flag since the client will already show the content
                    // in the ready to play state.
                    if(installed(true))
                    {
                        mPlayableOverride = false;
                    }
                    else
                    {
                        mPlayableOverride = true;
                    }

                    // We've stopped
                    emit playFinished(entitlementRef());

                    // forces the ui to update its display of this content
                    emit stateChanged(entitlementRef());
                }
            }


            void LocalContent::alternateGameLaunched()
            {
                //this is to force the update on the game tile and for it to be undimmed when the alternate game (e.g. battlelog
                mPlayableOverride = true;
                //emit stateChanged will trigger an update to the UI (game tile), which will use mPlayableOverride to return playable=true
                //that will allow the tile to get undimmed.  
                emit stateChanged(entitlementRef());

                //the connection of EntitlementProxy to stateChanged needs to be a direct connection
                //otherwise, resetting mPlayableOverride here will cause playable=false to be returned and the tile won't be undimmed
                mPlayableOverride = false;
            }

            void LocalContent::play()
            {
                ASYNC_INVOKE_GUARD;

                if (playing() && !allowMultipleInstances())
                {
                    emit (launched(PlayResult::PLAY_SUCCESS));  //this was previouly returning as true when play() itself used to return the bool so preserve the same behavior
                    return;
                }

                // Check to see if an update is mandatory
                if (treatUpdatesAsMandatory())
                {
                    // Update is mandatory
                    if (updateAvailable())
                    {
                        ORIGIN_LOG_EVENT << L"Detected mandatory update... canceling play";
                        emit (launched(PlayResult::PLAY_FAILED_REQUIRED_UPDATE));
                        return;
                    }
                }

                QString launchParams = mPlayFlow->launchParameters();

                QStringList childEntitlements = entitlementRef()->childEntitlementProductIds();
                QString childrenEntitlementsString = childEntitlements.join(",");

                bool nonOriginGame = contentConfiguration()->isNonOriginGame();
                bool bRunProtocol = contentConfiguration()->protocolUsedAsExecPath();

                if(isPathANormalFolder(executablePath()))
                {
                    QString path = QDir::toNativeSeparators(executablePath()); 
                    QDesktopServices::openUrl(QUrl("file:///" + path)); 
                    emit (launched(PlayResult::PLAY_SUCCESS));
                    return;
                }

#ifdef ORIGIN_MAC
                bool start = bRunProtocol;
#else
                bool start = (bRunProtocol && !nonOriginGame);
#endif
                if (start)
                {
                    sendGameSessionStartTelemetry("prot", childrenEntitlementsString, nonOriginGame);
                    Origin::Services::IProcess::runProtocol(contentConfiguration()->executePath(), QString(), QString());
                    finishLaunch(PlayResult::PLAY_SUCCESS);
                }
                else if (contentConfiguration()->isPlugin())
                {
                    // Success/failure telemetry captured inside Origin::Engine::Plugin class.
                    if(PluginManager::instance()->runPlugin(contentConfiguration()->productId()))
                    {
                        finishLaunch(PlayResult::PLAY_SUCCESS);
                        emit stateChanged(entitlementRef());
                    }
                    else
                    {
                        finishLaunch(PlayResult::PLAY_FAILED_PLUGIN_INCOMPATIBLE);
                        emit stateChanged(entitlementRef());
                    }

                }
                else if (!mAddonInstallCheck.isEmpty() && contentConfiguration()->isAddonOrBonusContent())
                {
                    QFileInfo fileInfo(mAddonInstallCheck);
                    if (fileInfo.suffix().compare("msi", Qt::CaseInsensitive) == 0)
                    {
                        sendGameSessionStartTelemetry("exeAdd", childrenEntitlementsString, nonOriginGame);
                        runEscalated(mAddonInstallCheck);
                    }
                    else
                    {
                        sendGameSessionStartTelemetry("protAdd", childrenEntitlementsString, nonOriginGame);
                        Origin::Services::IProcess::runProtocol(mAddonInstallCheck, QString(), QString());
                    }
                    finishLaunch(PlayResult::PLAY_SUCCESS);
                }
                else
                {
                    //we need to retrieve the ID2 authorization code for the games that need it
                    //so split out play() into:
                    //1. initiate request to retrieve authCode and wait for it
                    //2. prepare actual launch & launch
                    //3. finish up post-launch stuff and emit signal launched(true)

                    mAuthorizationCode.clear();

                    QString client_id = contentConfiguration()->identityClientIdOverride();
                    if (client_id.isEmpty())
                    {
                        launch ();
                    }
                    else
                    {
                        const int NETWORK_TIMEOUT_MSECS = 30000;

                        ORIGIN_LOG_EVENT << "LocalContent: retrieving authorization code";

                        QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                        Origin::Services::AuthPortalAuthorizationCodeResponse *resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, client_id);
                        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onAuthorizationCodeRetrieved()));

                        if (mAuthCodeRetrievalTimer)
                        {
                            delete mAuthCodeRetrievalTimer;
                            mAuthCodeRetrievalTimer = NULL;
                        }

                        mAuthCodeRetrievalTimer = new QTimer (this);
                        ORIGIN_VERIFY_CONNECT(mAuthCodeRetrievalTimer, SIGNAL(timeout()), resp, SLOT(abort()));

                        mAuthCodeRetrievalTimer->setSingleShot(true);
                        mAuthCodeRetrievalTimer->setInterval (NETWORK_TIMEOUT_MSECS);
                        mAuthCodeRetrievalTimer->start();

                        resp->setDeleteOnFinish(true);
                    }
                }
            }

            Origin::Services::IProcess* LocalContent::playRunningInstance(uint32_t pid, const QString& executablePath)
            {
                // We are re-attaching to a running game (ie the Origin client was closed/reopened + we know it's coming from an IGO connection)
                QMutexLocker locker(&mProcessLaunchLock);
                
                if (!mRunning)
                {
                    mRunning = Origin::Services::IProcess::createNewProcess(executablePath, "", "", "", true, false, false, false, true, false, false, this);
                    if (mRunning)
                    {
                        ORIGIN_VERIFY_CONNECT_EX(mRunning, SIGNAL(stateChanged(Origin::Services::IProcess::ProcessState)), this, SLOT(onProcessStateChanged(Origin::Services::IProcess::ProcessState)), Qt::QueuedConnection);
                        ORIGIN_VERIFY_CONNECT_EX(mRunning, SIGNAL(finished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), this, SLOT(onProcessFinished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), Qt::QueuedConnection);

                        if (mRunning->attach(pid))
                        {   
                            // This won't change how the game is currently setup (since right now we grab the info when starting the
                            // game/start a separate thread to watch the time), but it will properly show a timer in the UI
                            initFreeTrialStartedCheck();

                            emit playStarted(entitlementRef(), false);
                            trackTimePlayedStart();

                            finishLaunch(PlayResult::PLAY_SUCCESS);
                        }
                        
                        else
                        {
                            delete mRunning;
                            mRunning = NULL;
                        }
                    }
                }

                return mRunning;
            }

            void LocalContent::onAuthorizationCodeRetrieved()
            {
                if (mAuthCodeRetrievalTimer)
                {
                    mAuthCodeRetrievalTimer->stop();
                    delete mAuthCodeRetrievalTimer;
                    mAuthCodeRetrievalTimer = NULL;
                }

                AuthPortalAuthorizationCodeResponse* response = dynamic_cast<AuthPortalAuthorizationCodeResponse*>(sender());
                ORIGIN_ASSERT(response);
                if ( !response )
                {
                    //the fact that the response was deleted shouldn't really happen, but in the event that it does, just cancel the launch
                    ORIGIN_LOG_ERROR << "LocalContent: launch auth code retrieval failed, response = NULL";
                    emit (launched (PlayResult::PLAY_FAILED_AUTH_CODE_RETRIEVAL));
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onAuthorizationCodeRetrieved()));
                }
                
                bool success = ((response->error() == restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));
                if (!success)   //we need to log it
                {
                    ORIGIN_LOG_ERROR << "LocalContent: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
                }

                //it's ok if mAuthCode is actually empty, just save it off
                mAuthorizationCode = response->authorizationCode();
                launch ();
            }

            void LocalContent::launch()
            {
                QString launchParams = mPlayFlow->launchParameters();

                QStringList childEntitlements = entitlementRef()->childEntitlementProductIds();
                QString childrenEntitlementsString = childEntitlements.join(",");

                bool nonOriginGame = contentConfiguration()->isNonOriginGame();
                bool bRunProtocol = contentConfiguration()->protocolUsedAsExecPath();

                QString executableImage = executablePath();

                bool isNetworkPath = executableImage.startsWith("\\\\");
                executableImage = executableImage.replace("\\\\", "\\");
                executableImage = executableImage.replace("\"", "");

#ifdef ORIGIN_MAC
                QString bundlePath;
                QString executableFolder;
                QFileInfo currentImage( executableImage );
                    
                bool setupDone = false;
                    
                if (!setupDone)
                {
                    executableFolder = currentImage.absolutePath();

                    QString bundleExt = ".app";
                    int bundleExtIdx = executableImage.indexOf(bundleExt);
                    if (bundleExtIdx > 0)
                        bundlePath = executableImage.left(bundleExtIdx + bundleExt.length());
                    else
                        bundlePath = executableFolder; // not cool!
                }
                
                // check to verify that the bundle min os version <= current os version
                QString required( (Origin::Services::PlatformService::getRequiredOSVersionFor(bundlePath)) );
                if (!required.isEmpty())
                {
                    QString current( (EnvUtils::GetOSVersion()) );
                    Origin::VersionInfo requiredOS(required);
                    Origin::VersionInfo currentOS(current);
                    if (currentOS < requiredOS)
                    {
                        finishLaunch(PlayResult(PlayResult::PLAY_FAILED_MIN_OS, required));
                        return;
                    }
                }

#else
                    
                QString executableFolder = executableImage.left(executableImage.lastIndexOf("\\"));
#endif // ORIGIN_MAC
                QString executeParams = contentConfiguration()->executeParameters();
                if (!launchParams.isEmpty())
                {
                    executeParams.append(" ");
                    executeParams.append (launchParams);
                }

                QString gameArgs(GamesController::currentUserGamesController()->gameCommandLineArguments(contentConfiguration()->productId()));
                if (!gameArgs.isEmpty())
                {
                    executeParams.append(" ");
                    executeParams.append(gameArgs);
                }

                if (contentConfiguration()->isFreeTrial())
                {
                    executeParams.append(" -OriginFreeTrial");
                }

                QString environmentVars = getLaunchAdditionalEnvironmentVars ();

                ORIGIN_LOG_DEBUG << "sAddlEnvVars = " << environmentVars;

                QMutexLocker locker(&mProcessLaunchLock);

                //Previous code replaced all \\ with \. If network path, re-add a \ to the beginning
                if (isNetworkPath)
                {
                    executableImage.prepend("\\");
                    executableFolder.prepend("\\");
                }

                // 3 ways this game could require elevation
                QString tmp=this->contentConfiguration()->contentId();
                bool bRunElevated = RunElevatedWhitelistedContentIds.contains(tmp);   // 1) known list of IDs
                bRunElevated |= (mLocalInstallProperties && mLocalInstallProperties->GetExecuteElevated());                   // 2) feature flag (deprecated but needs to be supported)
                bRunElevated |= mPlayFlow->launchElevated();                                                                // 3) flag in the launcher specs in DiP Manifest 2.2
                    
                //this flag is set when we want the game to terminate if Origin/OriginClientService terminates
                bool bKillGameWhenOriginStops = contentConfiguration()->isFreeTrial();

                mRunning = Origin::Services::IProcess::createNewProcess(executableImage, executeParams, executableFolder, environmentVars, contentConfiguration()->monitorPlay(), false, bRunElevated, bRunProtocol, true, false, bKillGameWhenOriginStops, this);

                // mRunning might be NULL, if an error occurred!!!
                if (mRunning)
                {
                    // We want to prevent monitored play if the content is already playing
                    // and is configured to allow multiple instances to be launched. We
                    // assume that the second instance doesn't need to be monitored.
                    const bool playingAndAllowsMultipleInstances = playing() && allowMultipleInstances();

                    //if we do not monitor play (3PDD) do not inject IGO
                    if (contentConfiguration()->monitorPlay() && !playingAndAllowsMultipleInstances)
                    {
                        ORIGIN_VERIFY_CONNECT_EX(mRunning, SIGNAL(stateChanged(Origin::Services::IProcess::ProcessState)), this, SLOT(onProcessStateChanged(Origin::Services::IProcess::ProcessState)), Qt::QueuedConnection);
                        ORIGIN_VERIFY_CONNECT_EX(mRunning, SIGNAL(finished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), this, SLOT(onProcessFinished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), Qt::QueuedConnection);

                        // Initialize/Release the IGOInstance depending on whether IGO is needed or not.
                        Origin::Engine::Content::ContentController::currentUserContentController()->prepareIGO(contentConfiguration());

                        // enable IGO
                        bool IGOEnabled = contentConfiguration()->isIGOEnabled();

                        ORIGIN_LOG_EVENT << "IGO " << (IGOEnabled ? "enabled" : "disabled") << " for " << contentConfiguration()->productId();
                        
                        if (IGOEnabled)
                        {
                            if (Origin::Engine::IGOController::instance())
                            {
                                const Origin::Engine::Content::ContentConfigurationRef config = contentConfiguration();
                                bool forceKillAtOwnershipExpiry = config->isFreeTrial() && config->forceKillAtOwnershipExpiry();

                                // This flag specifies that we should hide the game in the IGO
                                // i.e., embargo mode: do not broadcast and do not show the game name
                                bool hideTheGame = shouldHide();

                                QString displayName;
                                QString masterTitle;

                                if (hideTheGame)
                                {
                                    displayName = tr("ebisu_client_notranslate_embargomode_title");
                                    masterTitle = tr("ebisu_client_notranslate_embargomode_title");
                                }
                                else
                                {
                                    displayName = config->displayName();
                                    QString dir = config->installationDirectory();
                                    if (dir == "" || dir == "\\" || dir == "/")
                                        masterTitle = config->masterTitle();
                                    else
                                        masterTitle = dir;
                                }
#ifdef ORIGIN_MAC
                                // We really need the bundle folder instead of the actual executable folder to watch the game-related processes
                                QString path = bundlePath;
#else
                                QString path = executableFolder;
#endif
                                QString defaultURL = Services::readSetting(SETTING_OverrideCatalogIGODefaultURL);
                                if (defaultURL.isEmpty())
                                    defaultURL = config->IGOBrowserDefaultURL();

                                Origin::Engine::IGOController::instance()->startProcessWithIGO(mRunning, config->productId(),
                                    displayName, masterTitle, path, mInstalledLocale, executableImage, forceKillAtOwnershipExpiry, 
                                    hideTheGame, trialTimeRemaining(), config->achievementSet(), defaultURL);

                            // do not use mRunning->start() here !!!!
                            }
                            else
                            {
                                ORIGIN_LOG_WARNING << "IGOEnabled is true but there is no IGOController available, starting entitlement normally.";
                                mRunning->start();
                            }
                        }
                        else
                        {
                            mRunning->start();
                        }
                        sendGameSessionStartTelemetry("exe", childrenEntitlementsString, nonOriginGame);
                    }
                    else
                    {
                        // Suppress game start telemetry for multiple instances
                        if (!playingAndAllowsMultipleInstances)
                        {
                            sendGameSessionStartTelemetry("exe", childrenEntitlementsString, nonOriginGame);
                        }
                        else
                        {
                            GetTelemetryInterface()->Metric_GAME_SESSION_UNMONITORED(contentConfiguration()->productId().toUtf8().data(), "exe", childrenEntitlementsString.toUtf8().data(), installedVersion().toUtf8().constData());
                        }
                        mRunning->startUnmonitored();
                    }

                    mRunning->resume();

                    // Only init free trial once the game is setup properly
                    initFreeTrialStartedCheck();

                    emit playStarted(entitlementRef(), false);
                    trackTimePlayedStart();
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Failed to start playing entitlement " << contentConfiguration()->productId() << " createNewProcess returned NULL.";
                    // Something went wrong; invalidate our cached installed state
                    mLastInstallCheck.invalidate();
                }
                finishLaunch(PlayResult::PLAY_SUCCESS);
            }

            void LocalContent::finishLaunch (LocalContent::PlayResult result)
            {
                if (!contentConfiguration()->monitorInstall())
                {
                    mNonMonitoredInstallInProgress = false;
                    bool keepInstallers = Origin::Services::readSetting(Origin::Services::SETTING_KEEPINSTALLERS, user()->getSession());
                    if(!keepInstallers)
                        deleteTempInstallerFiles();
                }
                emit (launched(result));
            }

            void LocalContent::runEscalated(const QString& executable) const
            {
                QString escalationReasonStr = "BonusContent runEscalated (" + executable + ")";
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                    return;

                ulong pid = escalationClient->executeProcess(executable, "", "", "", "");
                if (!pid)
                {
                    ORIGIN_LOG_ERROR << "Escalation Client error " << escalationClient->getError() << " (OS: " << 
                        escalationClient->getSystemError() << "). Opening " << executable;
                }
            }

            LocalContent::PlayableStatus LocalContent::playableStatus(bool checkORC) const
            {
                if(!contentConfiguration()->platformEnabled())
                    return PS_WrongPlatform;

                if (!contentConfiguration()->isSupportedByThisMachine())
                    return PS_WrongArchitecture;

                if (!contentConfiguration()->owned() && !contentConfiguration()->previewContent())
                    return PS_NotOwned;

                // If content is not wrapped in RtP, we need to fake
                // playability to get the the proper UI indications
                // when the content is externally started and then shutdown.
                if (mPlayableOverride)
                    return PS_Playable;

                UserRef user = mUser.toStrongRef();
                ORIGIN_ASSERT(user);
                if ( !user )
                    return PS_NotPlayable;

                bool online = Services::Connection::ConnectionStatesService::isUserOnline(user->getSession());
                if (contentConfiguration()->isFreeTrial())
                {
                    const qint64 timeLeft = trialTimeRemaining(true);
                    if (!online)
                    {
                        return PS_TrialNotOnline;
                    }
                    else if (timeLeft <= 0 && timeLeft != TrialError_LimitedTrialNotStarted)
                    {
                        return PS_ReleaseControl;
                    }
                }

                if(contentConfiguration()->isBrowserGame() && (releaseControlState() == RC_Released))
                    return PS_Playable;

                if (!contentConfiguration()->monitorInstall() && mNonMonitoredInstallInProgress)
                    return PS_Playable;

                if (!mAddonInstallCheck.isEmpty() && contentConfiguration()->isAddonOrBonusContent())
                {
                    if (installed())
                        return PS_Playable;
                    else
                        return PS_NotPlayable;
                }

                if (contentConfiguration()->isNonOriginGame())
                {
                    // We don't want to check if the executable exists because the play flow will give
                    // the user the option to change its location if it does not.
                    if (playing())
                        return PS_Playing;
                    else
                        return PS_Playable;
                }

                if (!installed())
                    return PS_NotInstalled;
 
                // Permissions trump Origin RC. 1102 and 1103 can both play
                if (checkORC && (releaseControlState() != RC_Released) && contentConfiguration()->originPermissions() <= Origin::Services::Publishing::OriginPermissionsNormal)
                    return PS_ReleaseControl;

                if(mReleaseControlState == RC_Expired)
                    return PS_ReleaseControl;

                if(contentConfiguration()->isEntitledFromSubscription())
                {
                    // Has the user's subscription offline play expired?
                    if(Engine::Subscription::SubscriptionManager::instance()->offlinePlayRemaining() <= 0)
                        return PS_SubscriptionControl;

                    // Has the user's subscription membership expired?
                    if( !Engine::Subscription::SubscriptionManager::instance()->isActive())
                        return PS_SubscriptionControl;

                    // Has the entitlement retired from the subscriptions?
                    if(Services::TrustedClock::instance()->nowUTC().secsTo(contentConfiguration()->originSubscriptionUseEndDate()) <= 0)
                        return PS_SubscriptionControl;
                }

                if (mInstallFlow && mInstallFlow->isInstallProgressConnected())
                    return PS_Busy;

                bool dynamicContentPlayableStatus = false;
                if (mInstallFlow && mInstallFlow->isDynamicDownload(&dynamicContentPlayableStatus))
                {
                    if (!dynamicContentPlayableStatus)
                        return PS_Busy;
                }

                // Update is destaging files
                if (mInstallFlow && mInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kPostTransfer)
                    return PS_Busy;

                if (repairing())
                    return PS_Busy;

                if (updating())
                {
                    if(treatUpdatesAsMandatory() && !dynamicContentPlayableStatus)
                        return PS_Busy;
                }

                if (playing())
                    return PS_Playing;

                if (mPlayFlow && mPlayFlow->isRunning())
                    return PS_Busy;

                // OFM-10995: PDLC can parent to multiple base games. Only let a DLC play if the parent is available too
                if(contentConfiguration()->isPDLC() && !isAnyParentPlayable())
                {
                    return PS_BaseGameNotPlayable;
                }

                if (!contentConfiguration()->executePath().isEmpty())
                {
                    QString path = executablePath();
                    if(mInstallFlow && mInstallFlow->operationType() == Downloader::kOperation_Install)
                        return PS_NotInstalled;
                    else if (QFile::exists(path) || mExecutablePathIsExistingFolderPath || contentConfiguration()->protocolUsedAsExecPath())
                        return PS_Playable;
                    else
                        return PS_NotInstalled;
                }

                return PS_NotPlayable;
            }

            bool LocalContent::isAllowedToPlay() const
            {
                // Permissions trump Origin RC. 1102 and 1103 can both play
                if ((releaseControlState() != RC_Released) && contentConfiguration()->originPermissions() <= Services::Publishing::OriginPermissionsNormal)
                    return false;

                if(releaseControlState() == RC_Expired)
                    return false;

                return true;
            }

            bool LocalContent::playing() const
            {
                // If content is not wrapped in RtP, we need to fake
                // play to get the the proper UI indications
                // when the content is externally started (mPlayingOverride).

                return (mRunning && (mRunningState == Origin::Services::IProcess::Running)) || mPlayingOverride;
            }

            bool LocalContent::inTrash() const
            {
                QString path = executablePath();
                if (path.contains("/.Trash"))
                    return true;
                else return false;
            }

            bool LocalContent::restoreFromTrash()
            {
                bool success = false;
#ifdef ORIGIN_MAC
                // Example oldPath = /Users/richardspitler/.Trash/Dragon Age/DragonAgeOrigins.app/Contents/MacOS/DragonAgeLauncher
                // Example completed newPath = /Applications/Dragon Age
                // Example completed appPath = /Applications/Dragon Age/DragonAgeOrigins.app/
                QString oldPath = executablePath();
                QString newPath = oldPath;
                const QString trash = ".Trash/"; // If App was removed on the base drive folder is .Trash
                const QString trashes = ".Trashes/"; // If App was removed on a separate partition folder is .Trashes
                const QString divider = "/";
                const QString applications = "/Applications/";
                const QString app = ".app/";
                // Grab index of the Trash folder to find the root app folder
                int trashIndex = (newPath.indexOf(trash) + trash.length());
                // Check to see if the entitlement was stored on a different volume
                if (trashIndex < 7)
                {
                    trashIndex = (newPath.indexOf(trashes) + trashes.length());
                }
                // Remove everything to the left of the root folder
                newPath.remove(0, trashIndex);
             
                const QString firstFolder = newPath.section('/', 0, 0);
                if (firstFolder.data()->isNumber())
                {
                    newPath.remove(0, newPath.indexOf(divider) + divider.length());
                }
                // Start a new string for the application path, it might be different that the root directory
                QString appPath = newPath;
                // Find index of next "/" this tell us where the root folder ends
                int appIndex = newPath.indexOf(divider);
                // Remove everything after root to ensure we are moving the root not files within
                newPath.remove(appIndex, newPath.size());
                // Find index of root in oldPath
                int oldAppIndex = oldPath.indexOf(newPath);
                // Remove everything after the root to ensure we are working with the same directory
                oldPath.remove(oldAppIndex + newPath.size(), oldPath.size());
                // Alter new path so it is within the Applications folder
                newPath.prepend(applications);
                // Find index of .app bundle
                int temp = appPath.indexOf(app);
                //remove everything after .app/
                appPath.remove(temp + 5, appPath.size());
                // Alter new path so it is within the Applications folder
                appPath.prepend(applications);
                
                // Try to move the Application
                success = Origin::Services::PlatformService::restoreApplicationFromTrash(oldPath, newPath, appPath);
#endif
                return success;
            }

            bool LocalContent::repair(bool silent /*= false*/)
            {
                QString path;
                // Doing a repair on a base game carries the expectation that
                // the user wants the game to be full and complete, so we
                // should reset all PDLC to auto-download.
                clearChildManualOnlyDownloads();

                int flags = Downloader::IContentInstallFlow::kRepair;

                if (silent) //suppresses dialogs
                {
                    flags |= Downloader::IContentInstallFlow::kSuppressDialogs;
                }

                return beginInstallFlow(mInstalledLocale, path, static_cast<Downloader::IContentInstallFlow::ContentInstallFlags>(flags));
            }

            bool LocalContent::repairable() const
            {
                UserRef u = mUser.toStrongRef();
                bool online = u && Origin::Services::Connection::ConnectionStatesService::isUserOnline (u->getSession());
                if(!online)
                    return false;

                if(!contentConfiguration()->dip())
                    return false;

                if (!contentConfiguration()->owned() && !contentConfiguration()->previewContent())
                    return false;

                if (!contentConfiguration()->downloadable())
                    return false;

                if (!contentConfiguration()->platformEnabled())
                    return false;

                if (!contentConfiguration()->isSupportedByThisMachine())
                    return false;

                if (!contentConfiguration()->monitorInstall() && mNonMonitoredInstallInProgress)
                    return false;

                // Permissions and preview content trump Origin RC. 1102 and 1103 can both install
                // although 1102 doesn't bypass the download date - it only allows "downloading" via .ini download path override.
                // Allow download of preview content with a local override, regardless of permissions.  This is necessary because
                // 1102 implies that the user has an entitlement, but game teams will need to test preview DLC download without one.
                if (mReleaseControlState == RC_Unreleased &&
                    contentConfiguration()->originPermissions() <= Origin::Services::Publishing::OriginPermissionsNormal &&
                    !(contentConfiguration()->previewContent() && contentConfiguration()->hasOverride()))
                {
                    return false;
                }

                if (mReleaseControlState == RC_Expired)
                {
                    return false;
                }

                // For normal games, we just blindly ask the server for the live build.
                // For plug-ins, we need this information up front to determine compatibility,
                // which means we also need to calculate which build is live (see CreatePluginSoftwareBuildMap)
                // If we didn't find a live build and have no override, we should disallow downloading.
                if (contentConfiguration()->isPlugin() && contentConfiguration()->liveBuildId().isEmpty() &&
                    !contentConfiguration()->hasOverride() && contentConfiguration()->buildIdentifierOverride().isEmpty())
                {
                    return false;
                }

                // if installerdata.xml is not loaded, treat this as not repairable.
                // this to address the issue where entitlements are installed via disc (non-dip) but the catalog lists it as DIP (Sims 3 + expansions) 
                // which can result in auto-repairs or manual repairs attempting to uninstall expansions and full game (ORPLAT-1110)
                if (mLocalInstallProperties == NULL)
                {
                    return false;
                }

                if(contentConfiguration()->isPDLC())
                {
                    return true;
                }
                else
                {
                    return mLocalInstallProperties->GetAutoUpdateEnabled();
                }
            }

            bool LocalContent::update(bool silent)
            {
                QString path;
                Downloader::IContentInstallFlow::ContentInstallFlags flags = Downloader::IContentInstallFlow::kNone;

                if (silent) //suppresses dialogs
                    flags = Downloader::IContentInstallFlow::kSuppressDialogs;
                else
                {
                    //  TELEMETRY: Patch triggered manually if silent flag is false
                    GetTelemetryInterface()->Metric_MANUALPATCH_DOWNLOAD_START(
                        contentConfiguration()->productId().toUtf8().constData(),
                        installedVersion().toUtf8().constData(),
                        contentConfiguration()->version().toUtf8().constData()); 
                }
                

                return beginInstallFlow(mInstalledLocale, path, flags);
            }

            bool LocalContent::updating() const
            {
                if(mInstallFlow && mInstallFlow->isActive() && mInstallFlow->isUpdate())
                    return true;
                else
                    return false;
            }

            bool LocalContent::repairing() const
            {
                if(mInstallFlow && mInstallFlow->isActive() && mInstallFlow->isRepairing())
                    return true;
                else
                    return false;
            }

            bool LocalContent::needsRepair() const
            {
                return mNeedsRepair;
            }

            bool LocalContent::updatable() const
            {
                UserRef user = mUser.toStrongRef();
                bool online = user && Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession());
                if(!online)
                    return false;

                if(!contentConfiguration()->dip())
                    return false;

                bool isExpired = (mReleaseControlState == RC_Expired);
                bool isUnreleased = (mReleaseControlState == RC_Unreleased);
                Origin::Services::Publishing::OriginPermissions permissions = contentConfiguration()->originPermissions();
                // 1103 codes can bypass the download dates, so if this is an 1103 code, we'll allow updating
                // 1102 codes can also update but only from a local override
                bool isPermissionsNormal = (permissions == Origin::Services::Publishing::OriginPermissionsNormal);
                bool isCode1102 = (permissions == Origin::Services::Publishing::OriginPermissions1102);
                if(isExpired || (isUnreleased && isPermissionsNormal) || (isUnreleased && isCode1102 && !contentConfiguration()->hasOverride()))
                    return false;

                //when the version number from OFB is zero we don't patch
                if(contentConfiguration()->version().compare("0") == 0)
                    return false;

                //PDLC always returns true, we don't check the local install properties because they
                //do not get loaded for PDLC.
                if(contentConfiguration()->isPDLC())
                    return true;

                //Plugins always return true, we may not have a local install properties if the plugin was 
                //distributed as part of a client build (if a nant build is started with the -D:embedPlugins=true
                //option, the resulting client build will contain all plug-in binaries)
                if(contentConfiguration()->isPlugin())
                    return true;
                
                // If the catalog/override contains software build metadata, use that value first, before the installed value
                if (contentConfiguration()->softwareBuildMetadataPresent())
                {
                    return contentConfiguration()->softwareBuildMetadata().mbAutoUpdateEnabled;
                }
                else if (mLocalInstallProperties)
                {
                    if (!mLocalInstallProperties->GetAutoUpdateEnabled() && contentConfiguration()->updateNonDipInstall())
                    {
                        // If we're using DiP < 2.2, we don't have feature flags, so we'll say we're updatable
                        VersionInfo manifestVersion = mLocalInstallProperties->GetManifestVersion();
                        if (manifestVersion < VersionInfo(2,2))
                        {
                            return true;
                        }
                    }
                        
                    // Default to whatever the manifest says, which for DiP >= 2.2 might actually disable updating
                    // We assume that if you are using DiP 2.2+ and you set this flag, you know what you are doing...
                    return mLocalInstallProperties->GetAutoUpdateEnabled();
                }
                else
                {
                    if (contentConfiguration() && contentConfiguration()->updateNonDipInstall())
                    {
                        // For 'update non-DiP' titles, we always update, even if we can't find a DiP manifest
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            bool LocalContent::updateAvailable()
            {
                bool available = false;

                if(installed() && updatable())
                {
                    // If we have no DiP manifest, we might want to update anyway if this flag is set
                    if (contentConfiguration()->updateNonDipInstall())
                    {
                        if (!mLocalInstallProperties)
                        {
                            return true;   
                        }
                    }

                    //check to see if we want to use the game version from the dip manifest as the version check
                    //if not, we use the version number stored in the local content cache
                    if(mLocalInstallProperties && mLocalInstallProperties->GetUseGameVersionFromManifestEnabled())
                    {
                        VersionInfo serverVersionInfo(contentConfiguration()->version());
                        VersionInfo localVersion = mLocalInstallProperties->GetGameVersion();

                        if(localVersion != serverVersionInfo)
                        {
                            available = true;
                        }
                    }
                    else
                    if(mInstalledVersion != "-1")
                    {
                        //mInstalledVersion with a -1 means this is the initial install. We can run into a case where the installed() returns true, but the flow
                        //is not complete. An example is the touchup installer is complete, but then it kicks off something like punkbuster. We don't want to show an update
                        //available at this point
                        available = (installed() && mInstalledVersion != contentConfiguration()->version());
                    }

                }

                return available;
            }

            QDateTime LocalContent::lastPlayDate() const
            {
                return GamesController::currentUserGamesController()->getCachedLastTimePlayed(entitlementRef());
            }

            qint64 LocalContent::totalPlaySeconds() const
            {
                return GamesController::currentUserGamesController()->getCachedTimePlayed(entitlementRef());
            }

            QString LocalContent::dipInstallPath() const
            {
                QString basePath;
                if (contentConfiguration()->isPDLC())
                {
                    if (entitlement() != NULL)
                    {
                        //since it is possible for PDLC to have multiple parents, we want to retrieve the list of parents
                        //and then choose the first one that's installed.
                        //this is to handle the issue where the rtp vs. non-rtp parents were configured with differing installchecks
                        //so we want to be sure to grab the information from the parent that's actually installed

                        QList<EntitlementRef> mainContentList = entitlement()->parentList();
                        if (mainContentList.size())
                        {
                            EntitlementRef mainContent;

                            //find the one that's installed
                            for(QList<EntitlementRef>::const_iterator it = mainContentList.constBegin();
                                it != mainContentList.constEnd();
                                it++)
                            {
                               EntitlementRef ent = (*it);
                               if (ent->localContent()->installed())
                               {
                                   mainContent = ent;
                                   break;
                               }
                            }
                            
                            // If none are installed, find one that is supported by this machine.
                            if(mainContent.isNull())
                            {
                                for(QList<EntitlementRef>::const_iterator it = mainContentList.constBegin();
                                    it != mainContentList.constEnd();
                                    it++)
                                {
                                    EntitlementRef ent = (*it);
                                    if (ent->contentConfiguration()->isSupportedByThisMachine())
                                    {
                                        mainContent = ent;
                                        break;
                                    }
                                }
                            }

                            //if couldn't find one that's installed, then just use the first parent found
                            if (mainContent.isNull())
                            {
                                mainContent = mainContentList.front();
                            }

                            if (!mainContent->localContent()->installed())
                            {
                                basePath = mainContent->localContent()->dipInstallPath();
                            }
                            else
                            {
                                basePath = PlatformService::localDirectoryFromOSPath(mainContent->contentConfiguration()->installCheck());
                            }
                            
                            basePath.replace("\\", "/");
                            if (basePath.endsWith("/"))
                            {
                                basePath = basePath.left(basePath.size()-1);
                            }

                            //Some pdlc get installed in subdirectories of the main content. Entitlement xml uses the
                            //installFolderName key but the EntitlementXmlImport class sets the value to the
                            //installationDirectory keyword
                            QString pdlcSubfolder = contentConfiguration()->installationDirectory();
                            pdlcSubfolder.replace("\\", "/");
                            if (!pdlcSubfolder.startsWith("/"))
                            {
                                pdlcSubfolder.prepend("/");
                            }

                            return basePath + pdlcSubfolder;

                        }
                        else
                        {
                            ORIGIN_LOG_ERROR << "Unable to build install path for pdlc without parent content. Content = " <<
                                contentConfiguration()->productId();
                            return "";
                        }
                    }
                }

                if(contentConfiguration()->dip())
                {
                    if(contentConfiguration()->isPlugin())
                    {
                        basePath = Origin::Services::PlatformService::pluginsDirectory();
                    }
                    else
                    {
                        //if a game is installed just return the installed directory from the registry
                        //updates for example
                        if(installed())
                        {
                            QString installFolder = PlatformService::localDirectoryFromOSPath(contentConfiguration()->installCheck());
                        
                            if(!installFolder.isEmpty())
                                return installFolder;
                        }
                        basePath = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR).toString();
                    }

                }
                else
                {
                    //this function should be only used for DIP as non dip titles choose their own install path
                    ORIGIN_ASSERT(0);
                    return QString();
                }

                if (basePath.right(1) != (QDir::separator()))
                    basePath += QDir::separator();

                return basePath + installFolderName();
            }

            bool LocalContent::isPathANormalFolder(QString path) const
            {
                QFileInfo info(path);
                return(info.isDir() && info.exists() && !info.isBundle());
            }

            QString LocalContent::executablePath() const
            {
                QString path;

                // For multi-launcher supported titles, the launcher will be in the mPlayFlow object
                QString launcher;
                if (mPlayFlow)
                    launcher = mPlayFlow->launcher();

                if (!launcher.isEmpty())
                {
                    path = PlatformService::localFilePathFromOSPath(launcher);
                }
                else if (!contentConfiguration()->isNonOriginGame())
                {
                    path = PlatformService::localFilePathFromOSPath(contentConfiguration()->executePath());
                }
                else
                {
                    path = contentConfiguration()->executePath();
                }

                return path;
            }

            QString LocalContent::nonDipInstallerPath(bool createIfNotExists /*= false*/)
            {
                QString originCachePath(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR));

                if (!originCachePath.isEmpty())
                {
                    originCachePath.replace("\\", "/");

                    if (!originCachePath.endsWith("/"))
                    {
                        originCachePath.append("/");
                    }

                    QString gameFolderName;

                    if(contentConfiguration()->originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeFullGame && !contentConfiguration()->contentId().isEmpty())
                    {
                        gameFolderName = contentConfiguration()->contentId();
                    }
                    else if(!contentConfiguration()->productId().isEmpty())
                    {
                        //we have to hash it since some product Id's have non valid characters like ":" if used in a directory name
                        gameFolderName = QString("%1").arg(qHash(contentConfiguration()->productId()));
                    }

                    if(gameFolderName.isEmpty())
                    {
                        return QString("");
                    }

                    QDir contentInstallCacheDir(originCachePath);
                    if (createIfNotExists && !contentInstallCacheDir.exists())
                    {
                        createElevatedFolder(originCachePath);
                    }

                    originCachePath += gameFolderName;

                    contentInstallCacheDir.setPath(originCachePath);

                    if (createIfNotExists && !contentInstallCacheDir.exists())
                    {
                        if(!createElevatedFolder(originCachePath))
                            return QString("");
                    }

                    return originCachePath;
                }

                return QString("");
            }

            void LocalContent::migrateOldFileNames()
            {
                //we need this function to migrate existing save files in ProgramData\Origin\LocalContent to use the productId in the file name as opposed
                //to a hash of the productId or contentId (for the nucleus migration case)
                if(!contentConfiguration()->isNonOriginGame())
                {
                    TIME_BEGIN("LocalContent::migrateOldFileNames")
                    QString newFileName = contentConfiguration()->productId();
                    Origin::Downloader::StringHelpers::StripReservedCharactersFromFilename(newFileName);
                    QString legacyFilePath(cacheFolderPath());
                    legacyFilePath.replace("\\", "/");
                    if (!legacyFilePath.endsWith("/"))
                    {
                        legacyFilePath.append("/");
                    }

                    //check and see if we have file with content Id's as file name
                    if(!contentConfiguration()->contentId().isEmpty())
                    {
                        QFile oldMsftContentIdFile(legacyFilePath + contentConfiguration()->contentId() + ".mfst");
                        QFile oldPkgContentIdFile(legacyFilePath + contentConfiguration()->contentId() + ".pkg");
                        QFile oldDatContentIdFile(legacyFilePath + contentConfiguration()->contentId()  + ".dat");

                        //try to rename it to the stripped product Id name, will fail if it can't rename file
                        oldMsftContentIdFile.rename(legacyFilePath + newFileName + ".mfst");
                        oldPkgContentIdFile.rename(legacyFilePath + newFileName + ".pkg");
                        oldDatContentIdFile.rename(legacyFilePath + newFileName + ".dat");
                    }

                    QString oldHashId = QString("%1").arg(qHash(contentConfiguration()->productId()));
                    QFile oldMsftProdHashFile(legacyFilePath + oldHashId + ".mfst");
                    QFile oldPkgProdHashFile(legacyFilePath + oldHashId + ".pkg");
                    QFile oldDatProdHashFile(legacyFilePath + oldHashId + ".dat");

                    //try to rename it to the stripped product Id name, will fail if it can't rename file
                    oldMsftProdHashFile.rename(legacyFilePath + newFileName + ".mfst");
                    oldPkgProdHashFile.rename(legacyFilePath + newFileName + ".pkg");
                    oldDatProdHashFile.rename(legacyFilePath + newFileName + ".dat");

                    TIME_END("LocalContent::migrateOldFileNames")
                }

                // Some titles have their installation directory configured as "/" instead of an empty string which
                // was causing their install manifest, .dat file, and crc map to be stored in the same LocalContent folder.
                // See cacheFolderPath() and installFolderName().
                if (contentConfiguration()->installationDirectory() == "/" || contentConfiguration()->installationDirectory() == "\\")
                {
                    QString newCachePath(cacheFolderPath());
                    QString oldCachePath(PlatformService::machineStoragePath() + "LocalContent");

                    QString newInstallManifestPath;
                    if (Downloader::ContentServices::GetInstallManifestPath(entitlement(), newInstallManifestPath))
                    {
                        // Move the install manifest
                        QFileInfo newInstallManifestFileInfo(newInstallManifestPath);
                        QString oldInstallManifestPath(oldCachePath + QDir::separator() + newInstallManifestFileInfo.fileName());
                        if (QFile::exists(oldInstallManifestPath) && !QFile::exists(newInstallManifestPath))
                        {
                            QDir dir;
                            dir.mkpath(newCachePath);

                            QFile oldInstallManifestFile(oldInstallManifestPath);
                            oldInstallManifestFile.copy(newInstallManifestPath);
                            oldInstallManifestFile.remove();

                            // Move the .dat file
                            QString newSerializationPath;
                            if (buildSerializationPath(newSerializationPath))
                            {
                                QFileInfo newSerializationFileInfo(newSerializationPath);
                                QString oldSerializationPath(oldCachePath + QDir::separator() + newSerializationFileInfo.fileName());
                                if (QFile::exists(oldSerializationPath) && !QFile::exists(newSerializationPath))
                                {
                                    QFile fileToMove(oldSerializationPath);
                                    fileToMove.copy(newSerializationPath);
                                    fileToMove.remove();
                                }
                            }

                            // Copy the crc map
                            QString newCrcMapPath(newCachePath + QDir::separator() + "map.crc");
                            QString oldCrcMapPath(oldCachePath + QDir::separator() + "map.crc");
                            if (QFile::exists(oldCrcMapPath) && !QFile::exists(newCrcMapPath))
                            {
                                QFile fileToCopy(oldCrcMapPath);
                                fileToCopy.copy(newCrcMapPath);
                                // Don't remove it. It is being shared by other entitlements which also need migration.
                            }
                        }
                    }
                }
            }

            void LocalContent::migrateLegacyInformation(const QString& cacheFolderPath)
            {
                // We call loadNonSpecificUserData here in case the child manual only download setting may have been written out when PDLC was canceled with different
                // parent base game (think BF3 and BF3 Limited Edition).  We need to read it in so we don't overwrite it with an empty value on the saveNonUserSpecificData below.
                loadNonUserSpecificData();

                // Attempt to migrate existing settings from previous version of Origin
                if(Origin::Services::MigrationUtils::getLegacyOriginIniContentData(this->contentConfiguration()->contentId(), mInstalledLocale, mInstalledVersion, mInstalledFolder))
                {
                    if(mInstalledVersion.isEmpty())
                    {
                        mInstalledVersion = contentConfiguration()->version();
                    }
                    if(mInstalledFolder.isEmpty())
                    {
                        mInstalledFolder = QFileInfo(PlatformService::localFilePathFromOSPath(contentConfiguration()->installCheck())).absolutePath();
                    }
                            
                    ORIGIN_LOG_DEBUG << "Successfully migrated settings for product id [" << this->contentConfiguration()->productId() << 
                        "] from legacy ORIGIN.INI: [installedLocale=" << mInstalledLocale << ", installedVersion=" << mInstalledVersion <<
                        ", installedFolder=" << mInstalledFolder << "]";

                    saveNonUserSpecificData();
                }

                //Attempt to mirgrate old crc map. In 8.X, main content and pdlc had separate crc maps. 9.0 uses
                //a single crc map. Legacy maps get combined here.
                if (contentConfiguration()->dip())
                {
                    // Construct path to the old crc file
                    QString oldCrcMapPath(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR));
                    oldCrcMapPath.replace("\\", "/");
                    if (!oldCrcMapPath.endsWith("/"))
                    {
                        oldCrcMapPath.append("/");
                    }
                    // Construct the old crc file name. Main content used the content id. PDLC used the
                    // product id with ':' replaced by '-'.
                    QString oldCrcFilename(contentConfiguration()->contentId());
                    if (contentConfiguration()->isPDLC())
                    {
                        oldCrcFilename = contentConfiguration()->productId();
                        oldCrcFilename.replace(':', '-');
                    }
                    oldCrcMapPath = QString("%1%2%3").arg(oldCrcMapPath).arg(oldCrcFilename).arg(".file_crcs");

                    // Build the new crc map path
                    QString newCrcMapPath(cacheFolderPath);
                    newCrcMapPath.replace("\\", "/");
                    if (!newCrcMapPath.endsWith("/"))
                    {
                        newCrcMapPath.append("/");
                    }
                    newCrcMapPath.append("map.crc");

                    // Create the crc map object. If the new crc map path already exists, load it.
                    // The map may already exist if pdlc is currently being migrated and the main
                    // content has already been migrated or vice versa.
                    Downloader::CRCMap *map = Downloader::CRCMapFactory::instance()->getCRCMap(newCrcMapPath);
                    if (QFile(newCrcMapPath).exists())
                    {
                        map->Load();
                    }

                    // Since main content and pdlc files are stored in the same map, an id has to be
                    // assigned to each map entry so update and repair jobs know which files to check.
                    QString id = QString("%1").arg(qHash(contentConfiguration()->installCheck()));
                    if (id.isEmpty())
                    {
                        ORIGIN_LOG_WARNING << "CRC map key is an empty string. Content = " << contentConfiguration()->productId();
                    }

                    if (QFile(oldCrcMapPath).exists())
                    {
                        if (Downloader::CRCMap::ConvertLegacy(oldCrcMapPath, dipInstallPath(), id, *map))
                        {
                            if (!map->Save())
                            {
                                ORIGIN_LOG_ERROR << "Unable to save CRC map: " << newCrcMapPath;
                            }

                            // Make sure to release the map (de-ref) when we finish
                            Downloader::CRCMapFactory::instance()->releaseCRCMap(map);
                            return;
                        }
                    }

                    // Make sure to release the map (de-ref) when we finish
                    Downloader::CRCMapFactory::instance()->releaseCRCMap(map);

                    ORIGIN_LOG_EVENT << "Unable to migrate crc map for " << contentConfiguration()->productId() <<
                        ". A new one will have to be generated on the first update.";
                }
            }

            QString LocalContent::cacheFolderPath(bool createIfNotExists /*= false*/, bool migrateLegacy /*= false*/, int *elevation_result /*= NULL */, int *elevation_error /*= NULL */)
            {
                const QString basePath = PlatformService::machineStoragePath() + "LocalContent";
                QString path = basePath + QDir::separator() + installFolderName();
                
#if defined(ORIGIN_MAC)
                path.remove(".app");
#endif
                if (elevation_result)
                    *elevation_result = Origin::Escalation::kCommandErrorNone;

                if (elevation_error)
                    *elevation_error = 0;

                if(createIfNotExists)
                {
                    QDir dir(path);

                    if(!dir.exists())
                    {
                        bool success = createElevatedFolder(path, elevation_result, elevation_error);  

                        if(!success)
                        {
                            ORIGIN_LOG_WARNING << "Unable to create local content cache folder path";
                        }
                    }
                }

                if(migrateLegacy)
                {
                    // Attempt to migrate content information from 8.x locations
                    migrateLegacyInformation(path);
                }

                return path;
            }

            void LocalContent::checkForUninstall()
            {
                // If the flow is inactive, and the game is not installed, try to kill the cache folder, if it exists
                QDir cacheDir(cacheFolderPath());
                if (mInstallFlow && mInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kReadyToStart && !installed(true) && cacheDir.exists())
                {
                    deleteCacheFolderIfUninstalled();
                }
            }

            void LocalContent::uninstallFinished()
            {
                mUninstalling = false;
                // Force an install check refresh
                bool uninstalled = !installed(true);
                
                if (!uninstalled)
                    return;
                
                ORIGIN_LOG_EVENT << "Uninstall complete!";

                // See if we need to delete the cache folder
                checkForUninstall();
            }

            void LocalContent::onHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead)
            {
                if (oldHead && oldHead->contentConfiguration() && oldHead->contentConfiguration()->productId() == contentConfiguration()->productId())
                {
                    // When the head is changed, the state is still paused. However, we need to update the UI to say that it's in the queue.
                    emit stateChanged(entitlementRef());
                }
            }

            void LocalContent::deleteCacheFolderIfUninstalled()
            {
                // We need a valid install flow
                if (!mInstallFlow)
                    return;

                // We need a valid content controller
                ContentController* contentController = ContentController::currentUserContentController();
                if (!contentController)
                    return;

                // If this content is pdlc, the main content may still be installed. Do not
                // delete the cache folder. If the main content is uninstalled, the cache folder
                // will be deleted when its associated LocalContent runs this method.
                //if (entitlement()->contentConfiguration()->isPDLC())
                //    return;

                // If the content is installed, we don't want to do this
                if (installed())
                    return;

                // Previously installed content that was uninstalled will be in the ready to
                // start state but will fail the install check. If the downloader isn't ready to start, the content might just be paused
                if (mInstallFlow->getFlowState() != Downloader::ContentInstallFlowState::kReadyToStart)
                    return;

                // Some content flows are set to auto start and in the ready to start state.
                // We don't want to delete the cache in that case.
                if (mInstallFlow->isAutoStartSet())
                    return;

                // Get the game folder path
                // For DiP titles, do one more failsafe check to make sure the DiP manifest is gone
                QString folderPath(cacheFolderPath());
                if (contentConfiguration()->dip())
                {
                    // Get game install folder
                    folderPath = (mInstallFlow) ? mInstallFlow->getDownloadLocation() : dipInstallPath();

                    // If the DiP Manifest is present, we know the game is definitely not uninstalled.  This is a 'failsafe' check only
                    const QString manifestPath = folderPath + QDir::separator() + contentConfiguration()->dipManifestPath();
                    QFile dipManifestFile(manifestPath);
                    if(dipManifestFile.exists())
                    {
                        // We found a DiP manifest, obviously the content has not been removed
                        ORIGIN_LOG_DEBUG << "Content not installed or active, but DiP manifest still present. Offer: " << contentConfiguration()->productId() << " Location: " << manifestPath;
                        return;
                    }
                }

                // Because we're seeing users now install game content on removable drives, it's possible for game content to appear/disappear.
                // And so we can no longer consider a game uninstalled simply because the install check fails.
                // If the drive on which the game was installed is not available we skip deleting the LocalContent metadata
                if (!EnvUtils::IsDriveAvailable(mInstallFlow->getDownloadLocation()))
                {
                    // The drive where the game was installed is not currently available
                    ORIGIN_LOG_DEBUG << "Installed content not found and the drive where it was previously installed is not available (possibly removed or drive letter changed). Offer: " << contentConfiguration()->productId() << " Location: " << mInstallFlow->getDownloadLocation();
                    return;
                }

                // If the game folder is in use, we can't delete the metadata
                if (contentController->folderPathInUse(folderPath))
                    return;

                // if the game was uninstalled, clear list that might prevent DLC from auto-downloading if the game is re-installed this session
				clearChildManualOnlyDownloads(false);

                //this assumes no sub folders
                QDir dir(cacheFolderPath());
                if (dir.exists())
                {
                    //ORIGIN_LOG_EVENT << "[Offer: " << contentConfiguration()->productId() << "] Cleaning up cache data at: " << dir.absolutePath();

                    // Cache file names
                    QString cacheFileExtensions = "%1.dat;%1.mfst;%1.pkg;%1.ddc";
                    QString cacheFileBasename = contentConfiguration()->productId();

                    // Clean the base filename (offerIDs typically contain ':' characters, etc)
                    Downloader::StringHelpers::StripReservedCharactersFromFilename(cacheFileBasename);

                    bool clearedFiles = false;
                    QStringList cacheFileTemplates = cacheFileExtensions.split(';');
                    for (int i = 0; i < cacheFileTemplates.count(); ++i)
                    {
                        QString cacheFilename = cacheFileTemplates.at(i).arg(cacheFileBasename);

                        if (dir.exists(cacheFilename))
                        {
                            QString absFilename = dir.absoluteFilePath(cacheFilename);
                            if (dir.remove(cacheFilename))
                            {
                                clearedFiles = true;

                                ORIGIN_LOG_EVENT << "[Offer: " << contentConfiguration()->productId() << "] Removed cache data for uninstalled game: "  << absFilename;
                            }
                            else
                            {
                                ORIGIN_LOG_ERROR << "[Offer: " << contentConfiguration()->productId() << "] Unable to remove cache data: "  << absFilename;
                            }
                        }
                    }

                    QStringList fileList = dir.entryList();

                    // Cycle through and see if any files are left
                    bool empty = true;
                    for(int i = 0; i < fileList.count(); ++i)
                    {
                        QString entry = fileList.at(i);

                        if (entry == "." || entry == "..")
                            continue;

                        empty = false;
                    }

                    // TODO: Eventually determine a safe way to delete the CRC map here, or maybe we will eventually have a CRC service

                    // Delete directory if there are no more files
                    if (empty)
                    {
                        ORIGIN_LOG_EVENT << "[Offer: " << contentConfiguration()->productId() << "] Removing empty cache folder: "  << dir.absolutePath();
                        dir.rmdir(cacheFolderPath());
                    }

                    // For games supporting dynamic content, we need to clear the DDC cache (in memory)
                    if (contentConfiguration()->softwareBuildMetadataPresent() && contentConfiguration()->softwareBuildMetadata().mbDynamicContentSupportEnabled && clearedFiles)
                    {
                        Downloader::DynamicDownloadController::GetInstance()->ClearDynamicDownloadData(contentConfiguration()->productId());
                    }
                }
            }

            QString LocalContent::installFolderName() const
            {
                EntitlementRef e = entitlement();
                if (e && e->contentConfiguration()->isPDLC() &&
                    !e->parent().isNull())
                {
                    e = e->parent();
                }

                // build the path
                QString gameFolderName = e->contentConfiguration()->installationDirectory();

                if (gameFolderName.isEmpty() || gameFolderName == "\\" || gameFolderName == "/")    // fallback to old functionality
                {
                    gameFolderName = e->contentConfiguration()->displayName();

                    // strip out any offending characters
                    Origin::Downloader::StringHelpers::StripReservedCharactersFromFilename(gameFolderName);

                    // Remove unicode and other garbage from the name
                    gameFolderName = Origin::Downloader::StringHelpers::stripMultiByte(gameFolderName);
                    
                    // Remove leading and trailing whitespace.
                    gameFolderName = gameFolderName.trimmed();

                    if (gameFolderName.isEmpty()) // If everything was stripped out
                        gameFolderName = e->contentConfiguration()->contentId();    // use the contentID
                }

#ifdef ORIGIN_MAC
                // Plugins need to go in .plugin folders.  All other DiP applications need to go in .app folders
                if (e && e->contentConfiguration()->dip())
                {
                    if (e->contentConfiguration()->isPlugin())
                    {
                        if (!gameFolderName.endsWith(".plugin", Qt::CaseInsensitive))
                        {
                            gameFolderName.append(".plugin");
                        }
                    }
                    else
                    {
                        if (!gameFolderName.endsWith(".app", Qt::CaseInsensitive))
                        {
                            gameFolderName.append(".app");
                        }
                    }
                }
#endif

                return gameFolderName;

            }

            const ContentConfigurationRef LocalContent::contentConfiguration() const
            {
                if(entitlement())
                    return entitlement()->contentConfiguration();
                else 
                    return ContentConfiguration::INVALID_CONTENT_CONFIGURATION;
            }

            ContentConfigurationRef LocalContent::contentConfiguration()
            {
                if(entitlement())
                    return entitlement()->contentConfiguration();
                else 
                    return ContentConfiguration::INVALID_CONTENT_CONFIGURATION;
            }

            void LocalContent::init()
            {
                migrateOldFileNames();

                mInstallFlow = Downloader::ContentServices::CreateInstallFlow(entitlement(), user());
                if (mInstallFlow)
                {
                    ORIGIN_VERIFY_CONNECT_EX(mInstallFlow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onStateChanged(Origin::Downloader::ContentInstallFlowState)), Qt::DirectConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mInstallFlow, SIGNAL(progressUpdate ( Origin::Downloader::ContentInstallFlowState, const Origin::Downloader::InstallProgressState&)), this, SLOT(onProgressUpdate ( Origin::Downloader::ContentInstallFlowState, const Origin::Downloader::InstallProgressState&)), Qt::DirectConnection);
                }

                setCloudContent(new CloudContent(user(), this));

                //loading persisted data if the game is installed
                if(installed())
                {
                    //load the local dip manifest usually here at <install location>\__Installer\InstallerData.xml
                    loadLocalInstallProperties();
                }

                if(!loadNonUserSpecificData())
                {
                    //if we can't load the file successfully and the game is installed, some thing is wrong
                    //in that case lets set the mInstalledVersion to 0 so the user can update
                    mInstalledVersion = QString("0");
                }

                // Set up the uninstall checking timer
                ORIGIN_VERIFY_CONNECT(&mUninstallCheckTimer, SIGNAL(timeout()), this, SLOT(checkForUninstall()));
                int randMsecDelay = qrand() % 1000; // Add a little random delay so that these don't all happen at the same time
                mUninstallCheckTimer.start((5 * 60 * 1000) + randMsecDelay); // Every ~5 minutes

                // Check now
                checkForUninstall();

                //free trials set up if any -- free trial subtypes, or if it's timed trial.
                if (contentConfiguration()->isFreeTrial())
                {
                    refreshReleaseControlExpiredTimer( TrustedClock::instance()->nowUTC(), contentConfiguration()->terminationDate() );
                }

                //ORC Setup
                refreshReleaseControlStatesAndTimers();

                //lets populate our total time played -- for base games only
                if(contentConfiguration()->isBaseGame() && GamesController::currentUserGamesController() != NULL)
                {
                    if (!contentConfiguration()->isNonOriginGame())
                    {
                        const QString &masterTitleId = contentConfiguration()->masterTitleId();
                        const QString &multiplayerId = contentConfiguration()->multiplayerId();

                        GamesController::currentUserGamesController()->getPlayedGameTime(masterTitleId, multiplayerId);
                    }
                    else
                    {
                        GamesController::currentUserGamesController()->getNonOriginPlayedGameTime(contentConfiguration()->productId());
                    }
                }

                mPlayFlow = QSharedPointer<PlayFlow>(new PlayFlow(entitlement()));
                mDownloadElapsedTimer.invalidate();

                ORIGIN_VERIFY_CONNECT_EX(contentConfiguration().data(), &ContentConfiguration::configurationChanged,
                    this, &LocalContent::refreshReleaseControlStatesAndTimers, Qt::QueuedConnection);
            }

            bool LocalContent::buildSerializationPath(QString& path, bool createIfNotExists /*= false*/)
            {
                if (contentConfiguration()->isPDLC())
                {
                    if (!entitlement() || entitlement()->parent().isNull())
                    {
                        return false;
                    }
                }

                QString fileNameSafeId(contentConfiguration()->productId());
                Origin::Downloader::StringHelpers::StripReservedCharactersFromFilename(fileNameSafeId);

                path = (QString("%1") + QDir::separator() + QString("%2.dat")).arg(cacheFolderPath(createIfNotExists)).arg(fileNameSafeId);

                return true;
            }

            void LocalContent::saveNonUserSpecificData()
            {
                QString serializationFilePath;
                buildSerializationPath(serializationFilePath, true);
                QFile serializationFile(serializationFilePath);

                if (!serializationFile.open(QIODevice::WriteOnly))
                {
                    ORIGIN_LOG_ERROR << "Failed to save local content configuration for [" << contentConfiguration()->productId() << "]";
                    return;
                }

                QMutexLocker locker(&mDataSerializationLock);

                ORIGIN_LOG_DEBUG << "Saving local content configuration for [" << contentConfiguration()->productId() << "]";

                QDataStream stream(&serializationFile);
                stream.setVersion(LocalContentSerializationVersion);
                stream << LocalContentSerializationMagic;

                stream << mInstalledVersion;
                stream << mInstalledLocale;
                stream << mInstalledFolder;

                // mFreeTrialStarted flag is no longer used, but we are keeping it in the code (and .dat) for now to avoid invalidating old .dat files
                stream << mFreeTrialStarted;

                //populate with an place holder string if there are no manual downloads for DLC
                QString childManualDownloads = ChildManualDownloadsEmpty;
                if(mChildManualOnlyDownloads.size())
                {
                    childManualDownloads = mChildManualOnlyDownloads.join(",");
                }

                ORIGIN_LOG_DEBUG << "Saved child downloads [" << contentConfiguration()->productId() << "]: " << childManualDownloads;

                stream << childManualDownloads;

                stream << mInitialInstalledTimestamp;
            }

            bool LocalContent::loadNonUserSpecificData()
            {
                QMutexLocker locker(&mDataSerializationLock);

                ORIGIN_LOG_DEBUG << "Loading non-user specific data for [" << contentConfiguration()->productId() << "]";

                QString serializationFilePath;
                buildSerializationPath(serializationFilePath);
                QFile serializationFile(serializationFilePath);

                if (!serializationFile.open(QIODevice::ReadOnly))
                {
                    return false;
                }

                QDataStream stream(&serializationFile);
                stream.setVersion(LocalContentSerializationVersion);

                unsigned int magic;
                stream >> magic;

                if ((stream.status() != QDataStream::Ok) ||
                    (magic != LocalContentSerializationMagic))
                {
                    return false;
                }

                stream >> mInstalledVersion;
                stream >> mInstalledLocale;
                stream >> mInstalledFolder;

                // mFreeTrialStarted flag is no longer used, but we are keeping it in the code (and .dat) for now to avoid invalidating old .dat files
                stream >> mFreeTrialStarted;

                // add case where game is installed but the installed version is -1 due to Origin terminating before touchup installer is done (EBIBUGS-26808)
                if (mInstalledVersion.isEmpty() || (mInstalledVersion == "-1" && mInstalled))   
                {
                    // The DownloadVersion was read wrongly from the consolidated entitlements in 9.0 resulting in the InstalledVersion to be an empty string
                    // this invalid InstalledVersion saved by 9.0 prevents automatic updates in 9.1. By setting the installed version to "0" Origin 
                    // will check the content version, and fix the configuration
                    mInstalledVersion = "0";
                }

                // Use the local install properties to override the installed version if necessary
                if(mInstalled && mLocalInstallProperties && mLocalInstallProperties->GetUseGameVersionFromManifestEnabled())
                {
                    mInstalledVersion = mLocalInstallProperties->GetGameVersion().ToStr();
                }
                
                // Use the version in the binary/plist for plug-ins.  This is necessary because there can be multiple plug-in installations per machine
                // (DL, RL, installed live client) and a single install manifest will not cut it.
                if(mInstalled && contentConfiguration()->isPlugin())
                {
                    VersionInfo pluginVersion, compatibleClientVersion;
                    Services::PlatformService::getPluginVersionInfo(executablePath(), pluginVersion, compatibleClientVersion);
                    mInstalledVersion = pluginVersion.ToStr();
                }

                QString childManualDownloads;
                stream >> childManualDownloads;
                
                ORIGIN_LOG_DEBUG << "Loaded child downloads [" << contentConfiguration()->productId() << "]: " << childManualDownloads;

                if(childManualDownloads.isEmpty() || (childManualDownloads == ChildManualDownloadsEmpty))
                {
                    mChildManualOnlyDownloads.clear();
                }
                else
                {
                    mChildManualOnlyDownloads = childManualDownloads.split(",");
                }

                stream >> mInitialInstalledTimestamp;

                return true;
            }

            void LocalContent::trackTimePlayedStart()
            {
                //let the server know were starting to play a game so it can start to track time on the server side
                //only for base games and not PDLC
                if(contentConfiguration()->isBaseGame())
                {
                    if (!contentConfiguration()->isNonOriginGame())
                    {
                        GamesController::currentUserGamesController()->postPlayingGame(entitlementRef());
                    }
                    else
                    {
                        GamesController::currentUserGamesController()->postPlayingNonOriginGame(entitlementRef());
                    }
                    mPlaySessionStartTime = Services::TrustedClock::instance()->nowUTC();
                }
            }

            void LocalContent::trackTimePlayedStop()
            {
                //let the server know we stopped playing so it knows to stop the timer
                //only for base games and not PDLC
                if (contentConfiguration()->isBaseGame()
                    && GamesController::currentUserGamesController() != NULL
                    && GamesController::currentUserGamesController()->user() != NULL)
                {
                    if (!contentConfiguration()->isNonOriginGame())
                    {
                        GamesController::currentUserGamesController()->postFinishedPlayingGame(entitlementRef());
                    }
                    else
                    {
                        GamesController::currentUserGamesController()->postFinishedPlayingNonOriginGame(entitlementRef());
                    }
                    mPlaySessionStartTime = QDateTime();
                }
            }

            void LocalContent::handleProgressChanged()
            {
                // Do very simple event compression
                if (mStateChangeSignalPending.testAndSetRelaxed(0, 1))
                {
                    QMetaObject::invokeMethod(this, "emitPendingProgressChangedSignal");
                }
            }

            void LocalContent::onProcessFinished(uint32_t pid, int exitCode, enum Origin::Services::IProcess::ExitStatus /*exitStatus*/)
            {
                // ORPUB-1615: It's possible that the user has logged out by the time we detect the process has ended.
                // It's possible that the GamesController, ContentConfiguration, or SubscriptionManager has been destroyed by now.
                if (GamesController::currentUserGamesController() && contentConfiguration() && Subscription::SubscriptionManager::instance())
                {
                    const QDateTime lastPlayDT = lastPlayDate();
                    const QString lastPlayed = lastPlayDT.toString() == "" ? "0" : lastPlayDT.toString();

                    GetTelemetryInterface()->Metric_GAME_SESSION_END(contentConfiguration()->productId().toUtf8().data(), exitCode, contentConfiguration()->isNonOriginGame(),
                        lastPlayed.toUtf8().data(), Subscription::SubscriptionManager::instance()->firstSignupDate().toString().toUtf8().data(),
                        Subscription::SubscriptionManager::instance()->startDate().toString().toUtf8().data(), contentConfiguration()->isEntitledFromSubscription());
                }
            } 
            
            void LocalContent::cleanupRunningProcessObject()
            {
                QMutexLocker locker(&mProcessLaunchLock);
                mRunning->deleteLater();
                mRunning = NULL;
            }

            void LocalContent::onProcessStateChanged(Origin::Services::IProcess::ProcessState state)
            {
                mRunningState = state;

                if(state == Origin::Services::IProcess::NotRunning)
                {
                    if(mRunning)
                    {
                        // We still want to run the free trial check even though the full time hasn't elapsed
                        // https://developer.origin.com/support/browse/OFM-7914
                        if (mFreeTrialStartedCheckTimer.isActive())
                        {
                            mFreeTrialStartedCheckTimer.stop();
                            onFreeTrialStartedCheckTimeOut();
                        }

                        // ORSUBS-1779: If the timed trial timer has started, stop it so we aren't expiring 
                        // the user's trial while he's not playing.
                        if ((contentConfiguration()->itemSubType() == Publishing::ItemSubTypeTimedTrial_Access ||
                            contentConfiguration()->itemSubType() == Publishing::ItemSubTypeTimedTrial_GameTime) &&
                            mReleaseControlExpiredTimer && mReleaseControlExpiredTimer->isActive())
                        {
                            mReleaseControlExpiredTimer->stop();
                        }

                        //if we were running and now we're not, update the last played time
                        trackTimePlayedStop();

                        // See if we're configured with a remote sync delegate factory
                        if (cloudContent()->shouldCloudSync())
                        {
                            cloudContent()->pushToCloud();
                        }
                        
                        emit playFinished(entitlementRef());
                        // we do not want to block the signal (would casue a hang), but we have to make sure, we delete the mRunning only if we are outside of LocalContent::play
                        // so we destroy it in a mutexed thread!
                        QtConcurrent::run(this, &LocalContent::cleanupRunningProcessObject);
                    }
                }
                emit stateChanged(entitlementRef());
            }


            void LocalContent::onProgressUpdate ( Origin::Downloader::ContentInstallFlowState, const Origin::Downloader::InstallProgressState& progressState )
            {
                //notifies the js that something changed;
                handleProgressChanged();
            }

            void LocalContent::onStateChanged(Origin::Downloader::ContentInstallFlowState newState)
            {
                if(mInstallFlow)
                {
                    emit flowIsActive(contentConfiguration()->productId(), mInstallFlow->isActive());

                    if(state() == Installing)
                        emit downloadingFinished();
                }

                switch (newState)
                {
                case Downloader::ContentInstallFlowState::kReadyToStart:
                    {
                        // Do nothing for now
                        break;
                    }
                case Downloader::ContentInstallFlowState::kCanceling:
                    {
                        mDownloadElapsedTimer.invalidate();

                        //if someone cancels a pdlc mark it so that it doesn't auto download
                        setChildManualOnlyDownloadOnAllParents();

                        //we check for the canceling state, since there is no canceled state and there is no way that the job
                        //does not cancel once hitting this state
                        emit installFlowCanceled();
                        break;
                    }
                case Downloader::ContentInstallFlowState::kPostTransfer:
                    {
                        //write this out to save off the installed path after download so that if we run the installer and the user has changed the downloadpath
                        //we can still use the old persisted path
                        saveNonUserSpecificData();
                        break;
                    }
                case Downloader::ContentInstallFlowState::kPostInstall:
                    {
                        bool keepInstallers = Origin::Services::readSetting(Origin::Services::SETTING_KEEPINSTALLERS, user()->getSession());
                        if(!keepInstallers && contentConfiguration()->monitorInstall() && !contentConfiguration()->dip())
                        {
                            deleteTempInstallerFiles();
                        }
                    }
                case Downloader::ContentInstallFlowState::kCompleted:
                    {
                        //set and save off the version after everything has been installed

                        //we only want to set the timestamp for when the initial install is completed
                        //if a game is deleted and redownloaded, we will update the time stamp
                        if(!mInstallFlow->isRepairing() && !mInstallFlow->isUpdate())
                        {
                            //get the current time and convert it to seconds since 1970-01-01T00:00:00 
                            mInitialInstalledTimestamp = QDateTime::currentDateTimeUtc().toTime_t();
                        }

                        mInstalledVersion = mDownloadingVersion;
                        saveNonUserSpecificData();
                        loadLocalInstallProperties();

                        if(mLocalInstallProperties && mLocalInstallProperties->GetUseGameVersionFromManifestEnabled())
                        {
                            // Use the local install properties to determine installed version if UseGameVersionFromManifestEnabled
                            mInstalledVersion = mLocalInstallProperties->GetGameVersion().ToStr();

                            // Because "Check for updates" just updates directly without checking for availability, 
                            // if the overridden DiP 2.2 had its manifest version changed after we loaded our entitlements,
                            // the installed version can be out of sync with what is in the ContentConfiguration.  Update
                            // the overridden version in contentConfiguration to reflect the new installed DiP manifest version.
                            if(contentConfiguration()->dip() && contentConfiguration()->hasOverride())
                            {
                                contentConfiguration()->setOverrideVersion(mLocalInstallProperties->GetGameVersion().ToStr());
                            }
                        }

                        if(!contentConfiguration()->monitorInstall())
                        {
                            mNonMonitoredInstallInProgress = true;
                        }
                        else
                        {
                            if(installed(true))
                            {
                                qint64 elapsedTime = 0;

                                if(mDownloadElapsedTimer.isValid())
                                {
                                    elapsedTime  = mDownloadElapsedTimer.elapsed();
                                    GetTelemetryInterface()->Metric_DL_ELAPSEDTIME_TO_READYTOPLAY(contentConfiguration()->productId().toLocal8Bit().constData(), elapsedTime,mInstallFlow->isITO(), mInstallFlow->isUsingLocalSource(), mInstallFlow->isUpdate(), mInstallFlow->isRepairing());
                                }
                            }
                        }

                        // when completed, if it is ITO and an update is going to follow, do not dequeue the item as it will mess up the UI if there are DLC in the mini-queue
                        // instead, let it stay and when it updates later in the ITO flow, it will trigger it to start the update from the head position.
						if ((newState == Downloader::ContentInstallFlowState::kCompleted) && (!installFlow()->isITO() || !updatable()))
						{
							// must protect against user logging out which results in a null queueController
							Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
							if (queueController)
								queueController->dequeue(entitlementRef());
						}

                        if(installFlow() && installFlow()->isUpdate() && installFlow()->lastFlowAction().flowState <= Downloader::ContentInstallFlowState::kPendingEula)
                        {
							if (!installFlow()->suppressDialogs())
							{
								emit error(entitlementRef(), ErrorNothingUpdated);
                                GetTelemetryInterface()->Metric_GAME_ZERO_LENGTH_UPDATE(contentConfiguration()->productId().toLocal8Bit().constData());
							}
                        }

                        //if someone completes a pdlc mark it so that it doesn't auto download if they uninstall
                        setChildManualOnlyDownloadOnAllParents();

                        mDownloadElapsedTimer.invalidate();

                        if (contentConfiguration()->isBaseGame())
                        {
                            const QString &productId = contentConfiguration()->productId();
                            const QString &masterTitleId = contentConfiguration()->masterTitleId();
                            const QList<EntitlementRef> &entitlements = entitlement()->contentController()->entitlementsByMasterTitleId(masterTitleId);

                            for (auto iter = entitlements.begin(); iter != entitlements.end(); ++iter)
                            {
                                EntitlementRef ent = *iter;
                                if (ent &&
                                    ent->localContent() &&
                                    ent->contentConfiguration() &&
                                    ent->contentConfiguration()->isBaseGame() &&
                                    ent->contentConfiguration()->productId() != productId)
                                {
                                    ent->localContent()->reloadInstallProperties();
                                }
                            }
                        }

                        break;
                    }
                default:
                    {

                    }
                }
                emit stateChanged(entitlementRef());

            }

            void LocalContent::reloadInstallProperties()
            {
                loadLocalInstallProperties();
                loadNonUserSpecificData();

                emit stateChanged(entitlementRef());
            }

            void LocalContent::onParentStateChanged(Origin::Downloader::ContentInstallFlowState newState)
            {
                switch (newState)
                {
                case Downloader::ContentInstallFlowState::kInitializing:
                    {
                        if(contentConfiguration()
                            && contentConfiguration()->softwareBuildMetadata().mbDLCRequiresParentUpToDate
                            && entitlementRef()->parent()->localContent()->updateAvailable()
                            && entitlementRef()->parent()->localContent()->updating())
                        {
                            stopListeningForParentStateChange();
                            download("");
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            void LocalContent::emitPendingProgressChangedSignal()
            {
                // Is there a state changed signal pending?
                if (mStateChangeSignalPending.testAndSetRelaxed(1, 0))
                {
                    emit progressChanged(entitlementRef());
                }
            }

            void LocalContent::onReleaseControlExpiredTimeOut()
            {
                if(mExpiredTimerIntervals.size())
                {

                    ExpiredIntervalData data = mExpiredTimerIntervals.pop();
                    ORIGIN_LOG_DEBUG << contentConfiguration()->productId() << ":" <<data.timeRemaining << " minutes left in before expire "<< QDateTime::currentDateTime().toString();
                    
                    if (contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                        || contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                    {
                        emit timedTrialExpireNotify(data.timeRemaining);
                    }
                    else
                    {
                        emit expireNotify(data.timeRemaining);
                    }

                    if (mExpiredTimerIntervals.count())
                    {
                        ORIGIN_LOG_ACTION << "Trial toast - Will notify time left in: (" << (data.timeTilNextTimeOut / 60000) << " minutes || " << (data.timeTilNextTimeOut / 1000) << " secs || " << data.timeTilNextTimeOut << " mil)";
                    }
                    else
                    {
                        ORIGIN_LOG_ACTION << "Trial toast - last toast";
                    }
                    mReleaseControlExpiredTimer->setInterval(data.timeTilNextTimeOut);
                    mReleaseControlExpiredTimer->start();
                }
                else
                {
                    ORIGIN_LOG_DEBUG << contentConfiguration()->productId() << ": EXPIRED " << QDateTime::currentDateTime().toString();
                    ORIGIN_LOG_ACTION << "Trial toast - No more to show";

                    //clean up release control timer
                    if(mReleaseControlExpiredTimer)
                    {
                        delete mReleaseControlExpiredTimer;
                        mReleaseControlExpiredTimer = NULL;
                    }

                    mReleaseControlState = RC_Expired;

                    //tell the UI to refresh itself
                    emit stateChanged(entitlementRef());
                }
            }

            void LocalContent::onReleaseControlChangeTimeOut()
            {
                //clean up release control timer
                if(mReleaseControlStateChangeTimer)
                {
                    delete mReleaseControlStateChangeTimer;
                    mReleaseControlStateChangeTimer = NULL;
                }

                ORIGIN_LOG_ACTION << "[" << contentConfiguration()->productId() << "] Refreshing ORC states and timers due to state change trigger.";

                // Update our release control state
                refreshReleaseControlStatesAndTimers();
            }

            void LocalContent::onFreeTrialStartedCheckTimeOut()
            {
                UserRef userRef = mUser.toStrongRef();
                Origin::Services::Session::SessionRef session = userRef->getSession();

                ContentController *contentController = ContentController::currentUserContentController();

                if(session && contentController)
                {
                    NucleusEntitlementQuery* entitlementQuery = contentController->nucleusEntitlementController()->refreshSpecificEntitlement(contentConfiguration()->entitlementId());
                    ORIGIN_VERIFY_CONNECT(entitlementQuery, SIGNAL(queryCompleted()), this, SLOT(onEntitlementQueryFinished()));
                }
            }

            qint64 LocalContent::trialTimeRemaining(bool bActualTime /*= false*/, const QDateTime& overrideTerminationDate /*= QDateTime()*/) const
            {
				/* 	The bActualTime boolean allows for this function to return the actual time left on the trial instead of the 'disaplay' time left on the trial
					When the user is not playing the game we don't what to be showing the trial time counting down, although in fact it is still progressing on the 
					active time splice. 
					The early trial will be requesting timesplices through the /burntime API, and each timeslice will be 5-10 minutes long. When the user
					quits the game there may still be 9 minutes left on the last requested time slice. This time is allocated and cannot be given back to the system.
					Now in the case where the user is on the last time slice before expiration, the user technically can play when he still has time left on the time slice,
					but the UI is currently using the users visible time for the enable logic--> hence the trial is expired. 
					By basing the disable logic on the actual time remaining bActualTime == true. the user will still be able to start the game.
					
					Another option is to have the trial time count down while the user is not playing...
				*/
				
                if(contentConfiguration()->isFreeTrial())
                {
                    const QDateTime now = TrustedClock::instance()->nowUTC();
                    if (contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access 
                        || contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                    {
                        // In timed trial, useEndDate in the past means that the Admin disabled this trial
                        if(now.msecsTo(contentConfiguration()->useEndDate()) < 0)
                        {
                            return TrialError_DisabledByAdmin;
                        }
                        else if(m_currentTrialTimeSlice == 0)
                        {
                            return m_remainingTrialDuration * 1000;
                        }
                        else if(m_lastTrialUpdate.isValid())
                        {
                            const qint64 lapsed = (TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() - m_lastTrialUpdate.toMSecsSinceEpoch()) / 1000;
                            // seconds to milliseconds
                            return (m_remainingTrialDuration + m_currentTrialTimeSlice - ((playing() || bActualTime) ? std::min(lapsed, m_currentTrialTimeSlice) : m_currentTrialTimeSlice)) * 1000;
                        }
                    }
                    else
                    {
                        qint64 ret = 0;
                        const QDateTime termination = overrideTerminationDate.isValid() ? overrideTerminationDate : contentConfiguration()->terminationDate();
                        if(termination.isValid())
                        {
                            ret = now.msecsTo(termination);
                            return ret > 0 ? ret : 0;
                        }
                        else
                        {
                            // With limited trials the termination date isn't set until the game is started.
                            return TrialError_LimitedTrialNotStarted;
                        }
                    }
                }
                return TrialError_NotTrial;
            }

            void LocalContent::setTrialTimeRemaining(const int& initial, const int& remaining, const int& slice, const QDateTime& timeStamp)
            {
                m_initialTrialDuration = initial;
                m_remainingTrialDuration = remaining;
                m_currentTrialTimeSlice = slice;
                const QDateTime timeStamp2 = timeStamp.isValid() ? timeStamp : TrustedClock::instance()->nowUTC();
                m_lastTrialUpdate = timeStamp2;
                refreshReleaseControlExpiredTimer(TrustedClock::instance()->nowUTC(), QDateTime());
                updateReleaseControlState();

                // We may want to fire this only when the m_remainingTrialDuration == 0
                emit stateChanged(entitlement());
            }

            bool LocalContent::hasEarlyTrialStarted() const
            {
                return !(m_initialTrialDuration == m_remainingTrialDuration && m_currentTrialTimeSlice == 0);
            }

            void LocalContent::updateTrialTime()
            {
                auto resp = dynamic_cast<Origin::Services::Trials::TrialCheckTimeResponse *>(sender());
                auto respGrant = dynamic_cast<Origin::Services::Trials::TrialGrantTimeResponse *>(sender());

                if (resp && resp->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == Origin::Services::Http200Ok)
                {
                    setTrialTimeRemaining(resp->timeInitial(), resp->timeRemaining(), 0, resp->timeStamp());
                }
                else if (respGrant && respGrant->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == Origin::Services::Http200Ok)
                {
                    setTrialTimeRemaining(respGrant->timeInitial(), respGrant->timeRemaining(), 0, respGrant->timeStamp());
                }
                else
                {
                    // TODO: Need retry code.
                    setTrialTimeRemaining(-1, -1, 0, QDateTime::currentDateTimeUtc());
                }
            }


            int LocalContent::trialReconnectingTimeLeft() const
            {
                if(m_reconnectingTrialTimeLeft == -1)
                {
                    return m_reconnectingTrialTimeLeft;
                }
                else
                {
                    qint64 timePast = TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() - m_reconnectingTrialTimeStart.toMSecsSinceEpoch();
                    qint64 timeleft = m_reconnectingTrialTimeLeft - timePast;

                    return timeleft < 0 ? 0 : timeleft;
                }
            }

            void LocalContent::setTrailReconnectingTimeLeft(int timeLeft)
            {
                m_reconnectingTrialTimeStart = Origin::Services::TrustedClock::instance()->nowUTC();
                m_reconnectingTrialTimeLeft = timeLeft;
            }


            int m_reconnectingTrialTimeLeft;
            QDateTime m_reconnectingTrialTimeStart;


            void LocalContent::onEntitlementQueryFinished()
            {
                NucleusEntitlementQuery* entitlementQuery = dynamic_cast<NucleusEntitlementQuery*>(sender());
                ORIGIN_ASSERT(entitlementQuery);

                if (!entitlementQuery)
                {
                    return;
                }
                
                // Get the termination date
                QDateTime terminationDate;
                QDateTime grantDate;

                if(entitlementQuery->error() == Origin::Services::restErrorSuccess && 
                   entitlementQuery->entitlement()->id() == contentConfiguration()->entitlementId())
                {
                    terminationDate = entitlementQuery->entitlement()->terminationDate();
                }

                const qint64 timeTilExpire = trialTimeRemaining(false, terminationDate);
                if (timeTilExpire > 0)
                {
                    // set up expiration timer for product
                    refreshReleaseControlExpiredTimer( TrustedClock::instance()->nowUTC(), terminationDate );
                    updateReleaseControlState();
                }
                else
                {
                    // assume that if the entitlement isn't found or the entitlement hasn't expired,
                    // then the issue must be that someone else already used Game Time on this machine
                    int gameTimeError = 0;
                    if (contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                        || contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                    {
                        gameTimeError = isTerminationDatePassed() ? ErrorTimedTrialAccount : ErrorTimedTrialSystem;
                    }
                    else
                    {
                        gameTimeError = isTerminationDatePassed() ? ErrorGameTimeAccount : ErrorGameTimeSystem;
                    }
                    emit error(entitlementRef(), gameTimeError);

                    //let IGO know to kill the game, if Origin is killed before reaching this point, the game will kill itself after a minute
                    IGOController::instance()->gameTracker()->setFreeTrialTimerInfo(contentConfiguration()->productId(), 0, &freeTrialElapsedTimer());

                    // Record error telemetry
                    GetTelemetryInterface()->Metric_ENTITLEMENT_FREE_TRIAL_ERROR(
                        entitlementQuery->error(),
                        entitlementQuery->httpStatus());

                }

                entitlementQuery->deleteLater();

                emit stateChanged(entitlementRef());
            }

            void LocalContent::prepareForRestart()
            {
                // This is a dialed down version of what happens in onProcessStateChanged(NotRunning), because we do not need to execute cloud sync code or display
                // the promo browser during this restart.  All that will happen when the new restarted process exits.  
                if(mRunning)
                {
                    // Detach from the currently running process
                    ORIGIN_VERIFY_DISCONNECT(mRunning, SIGNAL(stateChanged(Origin::Services::IProcess::ProcessState)), this, SLOT(onProcessStateChanged(Origin::Services::IProcess::ProcessState)));
                    ORIGIN_VERIFY_DISCONNECT(mRunning, SIGNAL(finished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), this, SLOT(onProcessFinished(uint32_t, int, Origin::Services::IProcess::ExitStatus)));
                   
                    mRunning->close();
                    mRunningState = Origin::Services::IProcess::NotRunning;

                    // Cleanup process so we can start a new one
                    cleanupRunningProcessObject();

                    // Cleanup some items that get recreated on the restart
                    mFreeTrialStartedCheckTimer.stop();
 
                    //if we were running and now we're not, update the last played time
                    trackTimePlayedStop();
                }
            }

            LocalContent::RCState LocalContent::releaseControlState() const
            {
                return mReleaseControlState;
            }

            bool LocalContent::beginInstallFlow(QString &locale, QString &path, const Downloader::IContentInstallFlow::ContentInstallFlags &flags)
            {
                ContentController *contentController = ContentController::currentUserContentController();
                bool suppressErrorDialog = flags & Downloader::IContentInstallFlow::kSuppressDialogs;

                if(shouldRestrictUnderageUser())
                {
                    if(!suppressErrorDialog)
                    {
                       notifyCOPPAError(true);
                    }
                    return false;

                }

                // Make sure the content folders are valid before allowing any install flow to begin
                if(!contentController->validateContentFolder(contentConfiguration()->dip()))
                {
                    return false;
                }

                if (mInstallFlow)
                {
                    //if it's ITO then set the downloading version to 0 (mInstalledVersion will get set to mDownloadingVersion after the installation is complete)
                    //in normal ITO flow, after the base game install, the update will occur and THAT will update the mInstalledVersion correctly
                    //and in the case where they are installing the base game offline, and thus unable to update, mInstalledVersion will get set to 0 so that next
                    //time they login/refresh, it will check for auto-update correctly.
                    if (path.isEmpty()) //not ITO
                        mDownloadingVersion = contentConfiguration()->version();
                    else
                        mDownloadingVersion = QString("0");

                    mInstallFlow->initInstallParameters (locale, path, flags);

                    QString folderPath(cacheFolderPath());
                    if (contentConfiguration()->dip())
                    {
                        folderPath = mInstallFlow->getDownloadLocation();
                    }

                    bool isPdlcAndParentInstalled = false;
                    
                    //MY: another case where we can't assume there's only one parent
                    //need to check and see if any parent is installed
                    if (contentConfiguration()->isPDLC() && entitlement())
                    {
                        QList<EntitlementRef> parentList = entitlement()->parentList();
                        if (parentList.size())
                        {
                            //find the one that's installed
                            for(QList<EntitlementRef>::const_iterator it = parentList.constBegin();
                                it != parentList.constEnd();
                                it++)
                            {
                               EntitlementRef ent = (*it);
                               if (ent->localContent()->installed() || (ent->localContent()->installFlow() && ent->localContent()->installFlow()->isActive()))
                               {
                                   //found an installed parent
                                   isPdlcAndParentInstalled = true;
                                   break;
                               }
                            }
                        }
                    }

                    bool isReadyToInstall = (mInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kReadyToInstall);

                    //Multiple pdlc content is allowed to simultaneously download to the same main folder, as is content that is ready_to_install
                    bool canBegin = false;

                    if(!contentController->folderPathInUse(folderPath) || isPdlcAndParentInstalled || isReadyToInstall)
                    {
                        canBegin = true;
                    }
                    else
                    {   // means not PDLC and folder is said to be in use - so check to see if other entitlement is truly using it as 
                        // opposed to the flow just being active (fixes EBIBUGS-28534)
                        QString folderPath(mInstallFlow->getDownloadLocation());

                        QList<EntitlementRef> entitlementsAccessingFolder = contentController->getEntitlementsAccessingFolder(folderPath);

                        // Make sure other entitlements are not active in the install folder.
                        for (QList<EntitlementRef>::const_iterator entitlement = entitlementsAccessingFolder.begin();
                            entitlement != entitlementsAccessingFolder.end(); ++entitlement)
                        {
                            Downloader::IContentInstallFlow* flow = (*entitlement)->localContent()->installFlow();
                            if (flow)
                            {
                                // We don't care if we are the only ones accessing the folder
                                bool thisIsMe = (*entitlement)->contentConfiguration()->productId().compare(
                                    contentConfiguration()->productId(), Qt::CaseInsensitive) == 0;

                                // flow->isActive() returns true for states kPaused and kReadyToInstall, but content
                                // in those states are not currently accessing the folder
                                Downloader::ContentInstallFlowState state = flow->getFlowState();
                                bool activelyAccessingFolder = flow->isActive() && 
                                    !(state == Downloader::ContentInstallFlowState::kPaused ||
                                    state == Downloader::ContentInstallFlowState::kEnqueued ||
                                    state == Downloader::ContentInstallFlowState::kReadyToInstall);

                                canBegin = !activelyAccessingFolder || thisIsMe;
                                if (!canBegin)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (canBegin)
                    {
                        // Mark this flow active so that the appropriate error handlers get hooked up
                        emit flowIsActive(contentConfiguration()->productId(), true);

                        mDownloadElapsedTimer.restart();
                        mInstallFlow->begin();
                    }
                    else if(!contentConfiguration()->isPDLC()) // we don't need to surface this error for PDLC since folder restrictions don't apply
                    {

                        ORIGIN_LOG_EVENT << "(" << contentConfiguration()->productId() << ") Cannot begin install, folder=(" << folderPath << ") already in use";

                        if(!suppressErrorDialog)
                        {
                            emit error(entitlementRef(), ErrorFolderInUse);
                        }
                        return false;
                    }
                    return true;
                }
                return false;
            }

            QStringList LocalContent::availableLocales(const QStringList& manifestLocales, const QStringList& manifestContentIds, const QString& manifestVersion)
            {
                QStringList retLocales = manifestLocales;

                // Get the locales supported by the content being installed
                QStringList offerLocales = contentConfiguration()->supportedLocales();

                // If the manifest supports other content IDs, go through them all. If
                // the user has an entitlement for the content, add any available
                // languages to the offerLocales list. This means a user can install
                // from any entitlement for this content and always be offered the
                // maximal list of languages to which they are entitled.
                QStringListIterator ci(manifestContentIds);
                const QString &requestingProductID = contentConfiguration()->productId();

                while (ci.hasNext())
                {
                    QString contentID = ci.next();

                    const QList<Engine::Content::EntitlementRef> &entList = Engine::Content::ContentController::currentUserContentController()->entitlementsByContentId(contentID);
                    foreach (const Engine::Content::EntitlementRef &entRef, entList)
                    {
                        // If this is the entitlement we're being asked to install we
                        // already have its locales in the list.
                        if (entRef && entRef->contentConfiguration()->productId().compare(requestingProductID, Qt::CaseInsensitive) != 0)
                        {
                            // We don't check for duplicate locales. It's the absence
                            // of a locale from this list that affects whether we
                            // remove it from the available languages below, so it
                            // behaves the same whether the locale appears once or
                            // ten times.
                            offerLocales.append(entRef->contentConfiguration()->supportedLocales());
                        }
                    }
                }

                // Remove unsupported languages, as defined by the entitlement data, for grey market support
                if (contentConfiguration()->supportsGreyMarket(manifestVersion))
                {
                    QMutableListIterator<QString>   i(retLocales);
                    while (i.hasNext())
                        if (!offerLocales.contains(i.next(), Qt::CaseInsensitive))
                            i.remove();
                }

                return retLocales;
            }

            void LocalContent::deleteTempInstallerFiles()
            {
                ORIGIN_LOG_DEBUG << L"LocalContent::DeleteTempFiles - " << contentConfiguration()->productId();

                // Delete exec path file
                
                // Cache path for related user
                QString sCachePath = ContentController::currentUserContentController()->contentFolders()->installerCacheLocation();
                QString sPathName = contentConfiguration()->executePath();
                QString sDebugMsg;

                if (!sPathName.isEmpty())
                {
                    QString sTestPath;

                    sTestPath = sPathName.left(sCachePath.size());
                    // ONLY if exec path is contained under the cache folder and isn't the cache folder itself
                    if (sTestPath.compare(sCachePath, Qt::CaseInsensitive) == 0 && sPathName.size() > sCachePath.size())
                    {
#ifdef _DEBUG
                        sDebugMsg = QString("Deleting exec file: %1\r\n").arg(sPathName);
                        ORIGIN_LOG_ERROR << sDebugMsg;
#endif
                        Origin::Util::DiskUtil::DeleteFileWithRetry(sPathName);
                    }
                }

                sPathName = contentConfiguration()->InstallerPath();
                QString sFileLink = contentConfiguration()->fileLink();

                // If we downloaded a dmg then sPathName is actually pointing at the installation program
                // housed inside that. In this case we really want to delete the DMG instead, so update
                // sPathName to point at that.
                //
                // (The first place I found a reference to the DMG was in fileLink, there may be a better
                // place to look.)
                
                // Side-effect warning: it seems like if you call fileLink() before InstallerPath() you get back
                // an empty string (or fileLink() is randomly empty...)
                //
                // Either way it scary.

                QStringList split_file_link = contentConfiguration()->fileLink().split('.');

                bool is_disk_image = false;
                if (split_file_link.size() > 1)
                {
                    QString extension = split_file_link.back();

                    is_disk_image = extension.compare("dmg", Qt::CaseInsensitive) == 0 ||
                                    extension.compare("iso", Qt::CaseInsensitive) == 0;
                }

                if (!sPathName.isEmpty() && !is_disk_image)
                {
                    QString sTestPath;

                    sTestPath = sPathName.left(sCachePath.size());
                    // ONLY if exec path is contained under the cache folder and isn't the cache folder itself
                    if (sTestPath.compare(sCachePath, Qt::CaseInsensitive) == 0 && sPathName.size() > sCachePath.size())
                    {
#ifdef _DEBUG
                        sDebugMsg = QString("Deleting installer file: %1\r\n").arg(sPathName);
                        ORIGIN_LOG_ERROR << sDebugMsg;
#endif
                        Origin::Util::DiskUtil::DeleteFileWithRetry(sPathName);
                    }
                }

                if (!contentConfiguration()->dip())
                {
                    // Delete unpack folder w/contents
                    sPathName = nonDipInstallerPath();
                    if (!sPathName.isEmpty())
                    {
                        QString sTestPath;

#ifdef __APPLE__
                        // On Mac, leave the slashes as they are

#else
                        // On Windows, swap slashes
                        sPathName.replace("/", "\\");
#endif

                        sTestPath = sPathName.left(sCachePath.size());

                        // ONLY if installer path is contained under the cache folder and isn't the cache folder itself
                        if (sTestPath.compare(sCachePath, Qt::CaseInsensitive) == 0 && sPathName.size() > sCachePath.size())
                        {
    #ifdef _DEBUG
                            sDebugMsg = QString("Deleting unpack folder: %1\r\n").arg(sPathName);
                            ORIGIN_LOG_ERROR << sDebugMsg;
    #endif
                            
                            EnvUtils::DeleteFolderIfPresent(sPathName, true, true);
                        }
                    }
                }
                
                // Since we just deleted the installer, we are not "ready to install" anything.  So if we are in the ReadyToInstall state,
                // then we should cancel the install flow.  This situation can occur if the user downloads a non-dip game, but does 
                // not install it, and then chooses to "delete all installers" in the settings.  This is a valid use case.  Cancelling
                // the install flow will update the state so that the entitlement mouse-over action becomes "Download" instead of "Install".  
                // EBIBUGS-21061.
                if (state() == ReadyToInstall)
                {
                    mInstallFlow->cancel();
                }

                emit stateChanged(entitlementRef());
            }
            void LocalContent::refreshReleaseControlExpiredTimer( const QDateTime& serverTime, const QDateTime& terminationDate )
            {
                qint64 timeTilExpire = trialTimeRemaining(false, terminationDate);

                // If this is a timed trial, don't start the shutting down notification timers until we are playing the game
                if ((contentConfiguration()->itemSubType() == Publishing::ItemSubTypeTimedTrial_Access || contentConfiguration()->itemSubType() == Publishing::ItemSubTypeTimedTrial_GameTime) && playing() == false)
                {
                    return;
                }

                if ((timeTilExpire > 0) && (timeTilExpire <= INT_MAX))
                {
                    //because we want to notify the user before the timer expires and we are using the lastServerUTCDateTime and terminationDate to setup a count down timer,
                    //we need to do some calculations to figure how when to fire the first timer and when to fire subsequent timers

                    //number of notifications we will send as we count down
                    const int maxIntervals = 7;

                    //minute marks at which we will send out notifications
                    const int notificationIntervals[maxIntervals] = { 0, 1, 3, 5, 10, 15, 30 };

                    mExpiredTimerIntervals.clear();

                    //set up the intervals between each notification so we can setup the next time when the current one times out
                    for (int i = 1; i < maxIntervals; i++)
                    {
                        const int notificationTimeMili = notificationIntervals[i] * 60000;
                        // Don't add time intervals that have already passed
                        if (timeTilExpire < notificationTimeMili)
                        {
                            continue;
                        }
                        const int timeTillNextTimeOut = (notificationIntervals[i] - notificationIntervals[i - 1]) * 60000;
                        ExpiredIntervalData intervalData;
                        intervalData.timeRemaining = notificationIntervals[i];
                        intervalData.timeTilNextTimeOut = timeTillNextTimeOut;
                        mExpiredTimerIntervals.push(intervalData);

                        ORIGIN_LOG_DEBUG << contentConfiguration()->productId() << ":intervalData --> timeRemain: " << intervalData.timeRemaining << " timeTilNextTimeOut " << intervalData.timeTilNextTimeOut;
                    }

                    //figure out when we want to send our first notification
                    if (mExpiredTimerIntervals.size())
                    {
                        const qint64 topTimeRemainingMiliseconds = mExpiredTimerIntervals.top().timeRemaining * 60000;
                        timeTilExpire -= topTimeRemainingMiliseconds;
                    }

                    ORIGIN_ASSERT(timeTilExpire > 0);

                    if (!mReleaseControlExpiredTimer)
                    {
                        mReleaseControlExpiredTimer = new QTimer(this);
                        mReleaseControlExpiredTimer->setSingleShot(true);
                        ORIGIN_VERIFY_CONNECT_EX(mReleaseControlExpiredTimer, SIGNAL(timeout()), this, SLOT(onReleaseControlExpiredTimeOut()), Qt::QueuedConnection);
                    }
                    else
                    {
                        mReleaseControlExpiredTimer->stop();
                    }

                    //setup first timer
                    //keep track of elapsed time for free trial
                    initFreeTrialElapsedTimer();
                    ORIGIN_LOG_ACTION << "Trial toast - Will notify time left in: (" << (timeTilExpire / 60000) << " minutes || " << (timeTilExpire / 1000) << " secs || " << timeTilExpire << " mil)";
                    mReleaseControlExpiredTimer->setInterval(timeTilExpire);
                    mReleaseControlExpiredTimer->start();

                    // make sure termination date in IGO is updated
                    IGOController::instance()->gameTracker()->setFreeTrialTimerInfo(contentConfiguration()->productId(), trialTimeRemaining(), &freeTrialElapsedTimer());
                }
            }

            void LocalContent::refreshReleaseControlStatesAndTimers()
            {
                if(mReleaseControlStateChangeTimer)
                {
                    mReleaseControlStateChangeTimer->stop();
                }
                
                qint64 timeUntilDownloadStartDate =  TrustedClock::instance()->nowUTC().msecsTo(contentConfiguration()->downloadStartDate());
                qint64 timeUntilReleaseDate = TrustedClock::instance()->nowUTC().msecsTo(contentConfiguration()->releaseDate());
                qint64 timeUntilUseEndDate = TrustedClock::instance()->nowUTC().msecsTo(contentConfiguration()->useEndDate());

                // Find the soonest valid state change 
                qint64 timeUntilNextStateChange = (timeUntilDownloadStartDate > 0 ? timeUntilDownloadStartDate : INT_MAX);
                timeUntilNextStateChange = (timeUntilReleaseDate > 0 ? std::min(timeUntilNextStateChange, timeUntilReleaseDate) : timeUntilNextStateChange);
                timeUntilNextStateChange = (timeUntilUseEndDate > 0 ? std::min(timeUntilNextStateChange, timeUntilUseEndDate) : timeUntilNextStateChange);

                // Setup a timer to refresh if it's less than INT_MAX (limitation for QTimer setInterval).  There's a separate ORC timer in ContentController
                // that refreshes user content every INT_MAX to work around this limitation.
                if ((timeUntilNextStateChange > 0) && (timeUntilNextStateChange < INT_MAX))
                {
                    //set a timer that will expire once we reach the next release control state
                    if(!mReleaseControlStateChangeTimer)
                    {
                        mReleaseControlStateChangeTimer = new QTimer(this); 
                        mReleaseControlStateChangeTimer->setSingleShot(true);
                    }

                    ORIGIN_LOG_ACTION << "[" << contentConfiguration()->productId() << "] Adding trigger for next ORC state change in " << timeUntilNextStateChange << " ms.";

                    mReleaseControlStateChangeTimer->setInterval(timeUntilNextStateChange);
                    mReleaseControlStateChangeTimer->start();
                    ORIGIN_VERIFY_CONNECT_EX(mReleaseControlStateChangeTimer, SIGNAL(timeout()), this, SLOT(onReleaseControlChangeTimeOut()),Qt::QueuedConnection);
                }

                updateReleaseControlState();
            }

            void LocalContent::updateReleaseControlState()
            {
                RCState previousState = mReleaseControlState;

                mReleaseControlState = RC_Released;

                if (contentConfiguration()->isPreorder())
                {
                    mReleaseControlState = RC_Unreleased;
                }
                else if (contentConfiguration()->isDownloadExpired() && contentConfiguration()->itemSubType() != Publishing::ItemSubTypeLimitedWeekendTrial)
                {
                    // If the download has expired then the release control state is set to expired, unless this is a limited weekend free trial (relies
                    // on termination date instead, because the trial duration can extend beyond - or stop short of - the download expiration date, 
                    // depending on when the user activates the trial).
                    mReleaseControlState = RC_Expired;
                }
                else if (isTerminationDatePassed())
                {
                    mReleaseControlState = RC_Expired;
                }
                else if(contentConfiguration()->isUnreleased())
                {
                    //if its a weburl path or is PULC, there is no preload state and we should mark it as unreleased
                    if(!contentConfiguration()->isBrowserGame() && contentConfiguration()->isPreload())
                        mReleaseControlState = RC_Preload;
                    else
                        mReleaseControlState = RC_Unreleased;
                }

                //if the release control state really changed and is not coming from the the initialization, then we want to notify the UI
                //we don't want to fire the signal for everything on init as the UI will refresh with the proxy gets created
                if(previousState != RC_Undefined && previousState != mReleaseControlState)
                {
                    ORIGIN_LOG_EVENT << "[" << contentConfiguration()->productId() << "] ORC state transition from [" << previousState << "] to [" << mReleaseControlState << "]";
                    if(installFlow() != NULL)
                    {
                        installFlow()->contentConfigurationUpdated();
                    }

                    emit stateChanged(entitlementRef());
                }
            }

            bool LocalContent::autoStartMainContent(bool startDownload, bool uacFailed)
            {
                bool retVal = false;
                if (mInstallFlow && !contentConfiguration()->isPDLC())
                {
                    Downloader::ContentInstallFlowState currentFlowState = mInstallFlow->getFlowState();
                    if (currentFlowState == Downloader::ContentInstallFlowState::kPaused)
                    {
                        // If downloadable state changed for any reason, we will not auto-resume until it is downloadable
                        if (mInstallFlow->isAutoResumeSet() && (downloadableStatus() == DS_Downloadable || downloadableStatus() == DS_Busy)) // Paused titles are DS_Busy
                        {
                            if (uacFailed)
                            {
                                ORIGIN_LOG_WARNING << "Skipping auto resume of " << contentConfiguration()->productId() << " due to UAC failure.";
                                
                                // For auto-resume flows, they may be 'active', but not running.  (e.g. Paused)  Mark them as active before giving up
                                emit flowIsActive(this->contentConfiguration()->productId(), true);
                            }

                            else
                            {
                                retVal = true;
                                if (startDownload)
                                {
                                    ORIGIN_LOG_EVENT << "Auto resuming " << contentConfiguration()->productId();
                                    resumeDownload(true);
                                }
                            }
                        }
                        else if((mInstallFlow->isUpdate() || mInstallFlow->isRepairing()) && downloadableStatus() == DS_ReleaseControl)
                        {
                            // downloadableStatus only returns DS_ReleaseControl if the content is unreleased [Catalog downloadDate in future] 
                            // or expired [Catalog useEndDate or Entitlement terminationDate passed]
                            // If content becomes non-downloadable for these reasons, we cancel any ongoing repair or update
                            ORIGIN_LOG_EVENT << "Canceling update or repair due to change of downloadable status of content for [" << contentConfiguration()->productId() << "]";
                            mInstallFlow->cancel();
                        }
                        else
                        {    // Even though we are not resuming at this time, mark this folder as active (prevents changing the download folder)
                            emit flowIsActive(this->contentConfiguration()->productId(), true);
                        }
                    }

                    else 
                    if (currentFlowState == Downloader::ContentInstallFlowState::kReadyToStart && mInstallFlow->isAutoStartSet())
                    {
                        if (uacFailed)
                        {
                            ORIGIN_LOG_WARNING << "Skipping auto start of " << contentConfiguration()->productId() << " due to UAC failure.";
                        }

                        else
                        {
                            retVal = true;
                            if (startDownload)
                            {
                                ORIGIN_LOG_EVENT << "Auto starting " << contentConfiguration()->productId();
                                download(mInstalledLocale);
                            }
                        }
                    }
                }

                return retVal;
            }

            bool LocalContent::isPDLCReadyForDownload(EntitlementRef entitlement)
            {
                // OFM-1856: Do not add unreleased 1102 content, without a download override, to pending PDLC.  The auto-download would fail.
                if( entitlement->localContent()->releaseControlState() != RC_Released && 
                    entitlement->localContent()->releaseControlState() != RC_Preload &&
                    entitlement->contentConfiguration()->originPermissions() == Origin::Services::Publishing::OriginPermissions1102 &&
                    !entitlement->contentConfiguration()->hasOverride() )
                {
                    return false;
                }

                Downloader::IContentInstallFlow* installFlow = entitlement->localContent()->installFlow();
                bool isResume = installFlow && installFlow->getFlowState() == Downloader::ContentInstallFlowState::kPaused;
                bool isAutoResume = isResume && installFlow->isAutoResumeSet();
                bool isPDLC = entitlement->contentConfiguration()->isPDLC();
                bool isDownloadable = entitlement->localContent()->downloadable();
                bool pdlcIsManualOnlyDownload = isChildManualOnlyDownload(entitlement->contentConfiguration()->productId());
                bool isOrWillBeInstalled = installed() || installInProgressQueued();

                if(isPDLC && // is current child pdlc
                    (isDownloadable || isAutoResume) && // downloadable() returns false for paused downloads, if auto resume flag is set we still want to resume the download
                    (!pdlcIsManualOnlyDownload || isAutoResume) && // not auto-downloaded or paused
                    isOrWillBeInstalled) // is main content installed             
                {
                    return true;
                }

                return false;
            }

            void LocalContent::collectAllPendingPDLC(QSet<QString>& pendingPDLC)
            {
                if (ContentController::currentUserContentController()->autoPatchingEnabled() == false)
                {
                    // If the user does not have "keep my games up to date" then we need to check for just purchased DLC as it is expected to auto-download
                    pendingPDLC.unite(mPendingPurchasedDLCDownloadList);
                    return;
                }

                typedef QList<Origin::Engine::Content::EntitlementRef> Children;
                Children children = entitlement()->children();

                for(Children::const_iterator it = children.constBegin(); it != children.constEnd(); it++)
                {
                    QString productId = (*it)->contentConfiguration()->productId();

                    // This PDLC was already auto-started by shared parent...
                    if(pendingPDLC.contains(productId))
                        continue;

                    if (isPDLCReadyForDownload((*it)))
                    {
                        pendingPDLC.insert(productId);
                    }
                }
            }

            void LocalContent::downloadAllPDLC(QSet<QString>& autoStartedPDLC, bool& permissionsRequested, bool &uacFailed)
            {
                // Grab the set of pending PDLC
                QSet<QString> pendingPDLC;

                ContentController* contentController = ContentController::currentUserContentController();
                if (!contentController) // crash protection
                    return;

                collectAllPendingPDLC(pendingPDLC);

                bool usePendingDownloadDLC = false;
                // If the user does not have "keep my games up to date" then we need to check for just purchased DLC as it is expected to auto-download
                if (contentController->autoPatchingEnabled() == false)
                {
                    usePendingDownloadDLC = true;
                }

                if (pendingPDLC.count() == 0)
                    return;

                typedef QList<Origin::Engine::Content::EntitlementRef> Children;
                Children children = entitlement()->children();

                for(Children::const_iterator it = children.constBegin(); it != children.constEnd(); it++)
                {
                    if (!pendingPDLC.contains((*it)->contentConfiguration()->productId()))
                        continue;

                    if (usePendingDownloadDLC)
                    {
                        if (!isPDLCReadyForDownload((*it))) // test it to make sure it is available for download since we got it from a list of strings
                            continue;
                        mPendingPurchasedDLCDownloadList.remove((*it)->contentConfiguration()->productId());
                    }

                    downloadPDLC((*it), autoStartedPDLC, permissionsRequested, uacFailed);
                }
            }

            void LocalContent::downloadPDLC(Origin::Engine::Content::EntitlementRef child, QSet<QString>& autoStartedPDLC, bool& permissionsRequested, bool &uacFailed)
            {
                QString productId = child->contentConfiguration()->productId();

                // This PDLC was already auto-started by shared parent...
                if(autoStartedPDLC.contains(productId))
                    return;

                // if ITO is running and it just redeemed a code, it might trigger a downloadAllPDLC in the refresh so we
                // want to avoid it trying to download since we are installing from disc
                if (InstallFlowManager::Exists())
                {
                    QString contentIdsITO = InstallFlowManager::GetInstance().GetVariable("ContentIds").toString();
                    if (!contentIdsITO.isEmpty())
                    {
                        QStringList contentIdsList = contentIdsITO.split(",");
                        QString contentId = child->contentConfiguration()->contentId();
                        if (contentIdsList.contains(contentId))
                        {
                            ORIGIN_LOG_WARNING << "DLC [" << contentId << "] not auto-downloaded ITO for this content in progress";
                            return;
                        }
                    }
                }

                Downloader::IContentInstallFlow* installFlow = child->localContent()->installFlow();
                bool isResume = installFlow && installFlow->getFlowState() == Downloader::ContentInstallFlowState::kPaused;
                bool isAutoResume = isResume && installFlow->isAutoResumeSet();
                {
                    QString uacReasonCode = "downloadAllPDLC (" + productId + ")";
                    if (isAutoResume)
                    {
                        // downloadableStatus only returns DS_ReleaseControl if the content is unreleased [Catalog downloadDate in future] 
                        // or expired [Catalog useEndDate or Entitlement terminationDate passed]
                        // If content becomes non-downloadable for these reasons, we cancel any ongoing repair or update
                        if(child->localContent()->downloadableStatus() == DS_ReleaseControl && 
                            (installFlow->isUpdate() || installFlow->isRepairing()))
                        {
                            ORIGIN_LOG_EVENT << "Canceling update or repair due to change of downloadable status of content for [" << productId << "]";
                            installFlow->cancel();
                            return;
                        }
                        else
                        {
                            if (uacFailed)
                            {
                                ORIGIN_LOG_WARNING << "Skipping auto-resume of " << productId << " due to UAC failure.";
                                // An auto-resume flow is 'active' even if it isn't currently running, so emit the signal to make sure its hooked up to the appropriate UI events
                                emit flowIsActive(productId, true);
                                return;
                            }

                            ORIGIN_LOG_EVENT << "Auto resuming " << productId;
                            autoStartedPDLC.insert(productId);

                            // If the user hasn't already been asked for permissions, we'll go ahead and ask before we begin
                            if (!permissionsRequested)
                            {
                                permissionsRequested = true;

                                // If we can't get escalation for some reason, we bail out of the entire loop
                                if (!Origin::Escalation::IEscalationClient::quickEscalate(uacReasonCode))
                                {
                                    ORIGIN_LOG_WARNING << "Skipping auto-resume of " << productId << " due to UAC failure.";
                                    uacFailed = true;

                                    // An auto-resume flow is 'active' even if it isn't currently running, so emit the signal to make sure its hooked up to the appropriate UI events
                                    emit flowIsActive(productId, true);
                                    return;
                                }
                            }

                            if (child->localContent()->resumeDownload(true))
                            {
                                //we want to add this to the list of "active" downloads so that it will be handled correctly on suspend when logging out
                                //previously we were waiting for the statechange to add it, but it seems like if you log out really quickly, the state
                                //change signal arrive after logout
                                emit flowIsActive(productId, true);
                            }
                            //else
                            //{
                            //    ORIGIN_LOG_EVENT << "Unable to autoresume " << child->contentConfiguration()->key().toString();
                            //}
                        }
                    }
                    else
                    {
                        if (uacFailed)
                        {
                            ORIGIN_LOG_WARNING << "Skipping auto-start of " << productId << " due to UAC failure.";
                            return;
                        }

                        ORIGIN_LOG_EVENT << "Auto starting " << productId;
                        autoStartedPDLC.insert(productId);

                        // If the user hasn't already been asked for permissions, we'll go ahead and ask before we begin
                        if (!permissionsRequested)
                        {
                            permissionsRequested = true;

                            // If we can't get escalation for some reason, we bail out of the entire loop
                            if (!Origin::Escalation::IEscalationClient::quickEscalate(uacReasonCode))
                            {
                                ORIGIN_LOG_WARNING << "Skipping auto-start of " << productId << " due to UAC failure.";
                                uacFailed = true;
                                return;
                            }
                        }

                        if (child->localContent()->download(mInstalledLocale))
                        {
                            //we want to add this to the list of "active" downloads so that it will be handled correctly on suspend when logging out
                            //previously we were waiting for the statechange to add it, but it seems like if you log out really quickly, the state
                            //change signal arrive after logout
                            emit flowIsActive(productId, true);
                        }
                    }
                }
            }

            // this is like autoStartDownloadOnNextRefresh() above but it is for the case where DLC is purchased but
            // auto-update setting is OFF which prevents auto-downloading. (EBIBUGS-28701)
            void LocalContent::appendPendingPurchasedDLCDownloads(const QString id)
            {
                if (!id.isEmpty())
                {
                    mPendingPurchasedDLCDownloadList.insert(id);
                }
            }


            void LocalContent::repairAllPDLC()
            {
                typedef QList<Origin::Engine::Content::EntitlementRef> Children;
                Children children = entitlement()->children();

                for (Children::const_iterator it = children.constBegin(); it != children.constEnd(); it++)
                {
                    QString productId = (*it)->contentConfiguration()->productId();

                    bool isPDLC = (*it)->contentConfiguration()->isPDLC();
                    bool isInstalled = (*it)->localContent()->installed();
                    bool isRepairable = (*it)->localContent()->repairable();

                    // Is it of interest?
                    if (!isPDLC || !isInstalled || !isRepairable)
                        continue;

                    ORIGIN_LOG_EVENT << "Auto repairing DLC " << productId;

                    (*it)->localContent()->repair();
                }
            }

            void LocalContent::invalidateCaches()
            {
                mLastInstallCheck.invalidate();

                // Refresh manifest properties
                loadLocalInstallProperties();
            }

            bool LocalContent::createElevatedFolder(const QString & folder, int *elevation_result, int *elevation_error)
            {
                return Services::PlatformService::createFolderElevated(folder, "D:(A;OICI;GA;;;WD)", &mUACExpired, elevation_result, elevation_error);
            }

            bool LocalContent::isTerminationDatePassed() const
            {
                bool retval = false;
                QDateTime terminationDate = contentConfiguration()->terminationDate();

                // For a trial entitlement a missing date is the same as a
                // date in the past - both mean the trial is expired.
                // For a non-trial entitlement we give the benefit of the doubt
                // to a missing termination date, so that we don't accidentally
                // lock users from playing their full games.
                if (contentConfiguration()->isFreeTrial())
                {
                    switch (contentConfiguration()->itemSubType())
                    {
                        //if its a limited trial that has not yet been started, a termination date is not set until we actually launch the game
                    case Publishing::ItemSubTypeLimitedTrial:
                        if (!terminationDate.isValid())
                        {
                            retval = false;
                        }
                        else if (terminationDate < TrustedClock::instance()->nowUTC())
                        {
                            retval = true;
                        }
                        break;
                    case Publishing::ItemSubTypeTimedTrial_Access:
                    case Publishing::ItemSubTypeTimedTrial_GameTime:
                        if (trialTimeRemaining(true)<=0)
                        {
                            retval = true;
                        }
                        break;
                    default:
                        if (!terminationDate.isValid() || terminationDate < TrustedClock::instance()->nowUTC())
                        {
                            retval = true;
                        }
                        break;
                    }
                }
                else
                {
                    if (terminationDate.isValid() && terminationDate < TrustedClock::instance()->nowUTC())
                    {
                        retval = true;
                    }
                }

                return retval;
            }

            bool LocalContent::isOnlineRequiredForFreeTrial()
            {
                //for now all free trial launches will require the user to be online
                //previously we were planning on only enforcing this on first launch

                bool required = false;
                UserRef userRef = mUser.toStrongRef();

                //if its a free trial and the user is offline
                if(contentConfiguration()->isFreeTrial() && !(userRef && Origin::Services::Connection::ConnectionStatesService::isUserOnline (userRef->getSession())))
                {
                    required = true;
                }

                return required;
            }

            void LocalContent::initFreeTrialStartedCheck()
            {
                if( contentConfiguration()->isFreeTrial() )
                {
                    mFreeTrialElapsedTimer.invalidate(); // reset everytime a free trial is started;

                    const int TIME_UNTIL_CHECK = 15 * 1000; // 15 seconds wait to make sure 'Get License' call to update terminationDate is completed
                    mFreeTrialStartedCheckTimer.start(TIME_UNTIL_CHECK);
                }
            }

            void LocalContent::initFreeTrialElapsedTimer()
            {
                mFreeTrialElapsedTimer.restart();
            }

            bool LocalContent::setChildManualOnlyDownload(const QString& productId)
            {
                bool retval = false;

                // XXX: Check if we have an entitlement with this ID.

                // Keep the list unique, including not having copies which
                // differ only by upper/lower case.
                if (!isChildManualOnlyDownload(productId))
                {
                    mChildManualOnlyDownloads.append(productId);
                    retval = true;
                }
                saveNonUserSpecificData();
                return retval;
            }

            bool LocalContent::isChildManualOnlyDownload(const QString& productId) const
            {
                return mChildManualOnlyDownloads.contains(productId, Qt::CaseInsensitive);
            }

            void LocalContent::clearChildManualOnlyDownloads(bool saveToDisk)
            {
                mChildManualOnlyDownloads.clear();

                if (!saveToDisk)
                    return;

                // We don't need to save the data unless the cache path exists (it may not for uninstalled games)
                QDir dir(cacheFolderPath());
                if (dir.exists())
                {
                    saveNonUserSpecificData();
                }
            }
            
            void LocalContent::setChildManualOnlyDownloadOnAllParents()
            {
                if(!contentConfiguration()->isPDLC() || entitlement().isNull())
                {
                    return;
                }

                foreach(Origin::Engine::Content::EntitlementRef parent, entitlement()->parentList())
                {
                    if(parent->localContent())
                    {
                        parent->localContent()->setChildManualOnlyDownload(contentConfiguration()->productId());
                    }
                }
            }

            void LocalContent::loadLocalInstallProperties()
            {
                // Clear the alternate uninstall path
                mAlternateUninstallPath = "";

                if(!contentConfiguration()->dip() || !installed(true))
                    return;

                QString installFolder = PlatformService::localDirectoryFromOSPath(contentConfiguration()->installCheck());
                if (contentConfiguration()->isPDLC())
                {
                    QString pdlcInstallDirectory = contentConfiguration()->installationDirectory();  // This is only used by relatively obsolete DLC!  (This shouldn't be used for new DLC)
                    Engine::Content::EntitlementRef parent = entitlement()->parent();
                    if (!parent.isNull())
                    {
                        installFolder = PlatformService::localDirectoryFromOSPath(parent->contentConfiguration()->installCheck());
                    }

                    // Append the PDLC install directory (obsolete DLC use this)
                    if (!pdlcInstallDirectory.isEmpty())
                    {
                        installFolder.append(pdlcInstallDirectory);
                    }
                }
                const QString manifestPath = installFolder + QDir::separator() + contentConfiguration()->dipManifestPath();
                QFile dipManifestFile(manifestPath);
                if(dipManifestFile.exists())
                {
                    if (!mLocalInstallProperties)
                        mLocalInstallProperties = new Origin::Downloader::LocalInstallProperties();

                    mLocalInstallProperties->Load(manifestPath, contentConfiguration()->isPDLC());
                }
                else
                {

                    delete mLocalInstallProperties;
                    mLocalInstallProperties = NULL;
                }

                if (mLocalInstallProperties && contentConfiguration()->isPDLC() && mLocalInstallProperties->GetUninstallPath().isEmpty())
                {
                    QString uninstallPath = manifestPath;

                    uninstallPath.replace("installerdata.xml", "Cleanup.exe", Qt::CaseInsensitive);

                    QFile cleanupExeFile(uninstallPath);
                    if (cleanupExeFile.exists())
                    {
                        ORIGIN_LOG_EVENT << "Found alternate install path for DLC (Cleanup.exe) - " << uninstallPath;

                        mAlternateUninstallPath = uninstallPath;
                    }
                }

                if (!contentConfiguration()->isPDLC())
                {
                    // See if the CRC map exists
                    QString crcMapPath = cacheFolderPath() + QDir::separator() + "map.crc";
                    QFileInfo crcMapFile(crcMapPath);
                    if (crcMapFile.exists())
                    {
                        mNeedsRepair = false;
                    }
                    else
                    {
                        if (repairable())
                        {
                            ORIGIN_LOG_WARNING << "[" << contentConfiguration()->productId() << "] CRC Map missing!  Content needs repair.";
                            mNeedsRepair = true;
                        }
                        else
                        {
                            ORIGIN_LOG_WARNING << "[" << contentConfiguration()->productId() << "] CRC Map missing but content not repairable.";
                            mNeedsRepair = false;
                        }
                    }
                }
            }

            void LocalContent::sendGameSessionStartTelemetry(const QString& launchType, const QString& childrenContent, const bool& isNonOriginGame)
            {
                const QDateTime lastPlayDT = lastPlayDate();
                const QString lastPlayed = lastPlayDT.toString() == "" ? "0" : lastPlayDT.toString();

                GetTelemetryInterface()->Metric_GAME_SESSION_START(contentConfiguration()->productId().toUtf8().data(), launchType.toUtf8().data(), childrenContent.toUtf8().data(), 
                    installedVersion().toUtf8().constData(), isNonOriginGame, lastPlayed.toUtf8().data(), Subscription::SubscriptionManager::instance()->firstSignupDate().toString().toUtf8().data(), 
                    Subscription::SubscriptionManager::instance()->startDate().toString().toUtf8().data(), contentConfiguration()->isEntitledFromSubscription());
            }

            bool LocalContent::hasMinimumVersionRequiredForOnlineUse() const
            {
                // Read this from build metadata, which handles catalog and overrides
                if (contentConfiguration()->softwareBuildMetadataPresent())
                {
                    VersionInfo minimumRequiredVersionForOnlineUse = contentConfiguration()->softwareBuildMetadata().mMinimumRequiredVersionForOnlineUse;
                    VersionInfo installedVersion = VersionInfo(mInstalledVersion);

                    return (installedVersion >= minimumRequiredVersionForOnlineUse);
                }
                else
                {
                    return true;
                }
            }

            bool LocalContent::treatUpdatesAsMandatory() const
            {
                if(contentConfiguration()->treatUpdatesAsMandatoryAlternate())
                    return true;

                if (contentConfiguration()->softwareBuildMetadataPresent())
                    return contentConfiguration()->softwareBuildMetadata().mbTreatUpdatesAsMandatory;

                return false;
            }

            bool LocalContent::allowMultipleInstances() const
            {
                Downloader::LocalInstallProperties* pProperties = localInstallProperties();

                // :TODO: Not sure we need an "allow multiple instances alternate" similar
                // to what "treat updates as mandatory" has? Looks like the "alternate"
                // was added to support SDK game restart, not sure why though.
                if (pProperties)
                    return pProperties->GetAllowMultipleInstances();

                return false;
            }

            // Returns the first non-user selected launcher that matches the 32/64 bit of the local OS
            QString LocalContent::defaultLauncher()
            {
                bool bIs64BitOS = PlatformService::queryThisMachineCanExecute(PlatformService::ProcessorArchitecture64bit);
                bool bIsTimedTrial = contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access || contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime;

                // As far as launchers are concerned, if this is XP or earlier we only support 32 bit
                if (EnvUtils::GetISWindowsXPOrOlder())
                    bIs64BitOS = false;

                QStringList launchTitleList;
                if (mLocalInstallProperties)
                {
                    Engine::Content::tLaunchers launchers = mLocalInstallProperties->GetLaunchers();
                    if (launchers.size() > 0)
                    {
                        Engine::Content::LauncherDescription* pLauncherToUse = NULL;
                        Engine::Content::LauncherDescription* pBackupLauncher = NULL;

                        for (Engine::Content::tLaunchers::iterator it = launchers.begin(); it != launchers.end(); it++)
                        {
                            Engine::Content::LauncherDescription* pLauncher = &(*it);

                            const bool compatibleBitness = (bIs64BitOS || !pLauncher->mbRequires64BitOS);
                            const bool matchesBitness = (pLauncher->mbRequires64BitOS && bIs64BitOS) || (!pLauncher->mbRequires64BitOS && !bIs64BitOS);
                            const bool matchesTrialSetting = (pLauncher->mbIsTimedTrial == bIsTimedTrial);

                            // If this launcher matches our OS bitness and the offer's trial setting, use it
                            // If a matching launcher was not found, fall back on any launcher that is compatible
                            if (matchesTrialSetting)
                            {
                                if (matchesBitness)
                                {
                                    pLauncherToUse = pLauncher;
                                    break;
                                }
                                else if (compatibleBitness)
                                {
                                    pBackupLauncher = pLauncher;
                                }
                            }
                        }

                        if (!pLauncherToUse)
                            pLauncherToUse = pBackupLauncher;

                        // If there was no launcher matching the bitness of the OS, just use the first one
                        if (!pLauncherToUse)
                            pLauncherToUse = &(*launchers.begin());

                        // Now get the title
                        const QString locale = mInstalledLocale;
                        if (QFile::exists( Services::PlatformService::localFilePathFromOSPath(pLauncherToUse->msPath) ))
                        {
                            QString localeTitle;
                            if (pLauncherToUse->mLanguageToCaptionMap.contains(locale))
                            {
                                localeTitle = pLauncherToUse->mLanguageToCaptionMap[locale];
                            }

                            // If the localeTitle doesn't match our current locale - use English (US or GB).
                            // If we don't have a English title... Tell the game team to include one. - Alex P.
                            if(localeTitle.isEmpty())
                            {
                                ORIGIN_LOG_EVENT << "Looking for EN";
                                // I wish there was a way of just returning the first value of "en_" but there isn't.
                                // The key has to be exact.
                                if(pLauncherToUse->mLanguageToCaptionMap.contains("en_US"))
                                    localeTitle = pLauncherToUse->mLanguageToCaptionMap["en_US"];
                                else if(pLauncherToUse->mLanguageToCaptionMap.contains("en_GB"))
                                    localeTitle = pLauncherToUse->mLanguageToCaptionMap["en_GB"];

                                //means it doesn't includ en_US or en_GB either; just use the first one
                                if (localeTitle.isEmpty())
                                {
                                    ORIGIN_LOG_EVENT << "EN not found, use first one";
                                    localeTitle = pLauncherToUse->mLanguageToCaptionMap.begin().value();
                                }
                            }

                            return localeTitle;
                        }
                    }

                ORIGIN_LOG_ERROR << "No launcher found matching the 32/64 bit depth of OS... b64BitOS:" << bIs64BitOS;
                }

                return "";  // No launcher found that matches the bit depth of OS
            }

            bool LocalContent::shouldHide()
            {
                bool shouldHide = true;
                Origin::Engine::Content::ContentConfigurationRef config = contentConfiguration();

                // These five variable affect whether we hide the game
                // See R&D Mode - Phase 2 (https://developer.origin.com/documentation/pages/viewpage.action?pageId=69142927)
                bool isUnreleased = (releaseControlState() == RC_Unreleased) || (config->testCode() != Engine::Content::ContentConfiguration::NoTestCode);
                bool isBlackListed = config->twitchClientBlacklist();
                bool isNOG = config->isNonOriginGame();
                bool isOverridesEnabled = Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool();
                bool isEmbargoModeDisabled = isOverridesEnabled && Services::readSetting(Services::SETTING_DisableEmbargoMode);

                if( (isOverridesEnabled && !isUnreleased && !isBlackListed && !isNOG) ||    // EACore.ini and regular game
                    (isOverridesEnabled && isEmbargoModeDisabled) ||                        // EACore.ini with Override
                    (!isOverridesEnabled && !isUnreleased && !isBlackListed))               // No EACore.ini, Not Unreleased, Not Blacklisted 
                    shouldHide = false;
 
                // check the "DisableTwitchBlacklist" override for this offer ID
                if (isTwitchBlacklistOverride(config->productId()))
                    shouldHide = false;

                return shouldHide;
            }

            QStringList LocalContent::launchTitleList()
            {
                bool bIs64BitOS = PlatformService::queryThisMachineCanExecute(PlatformService::ProcessorArchitecture64bit);
                bool bIsTimedTrial = contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access || contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime;

                // As far as launchers are concerned, if this is XP or earlier we only support 32 bit
                if (EnvUtils::GetISWindowsXPOrOlder())
                    bIs64BitOS = false;

                QStringList launchTitleList;
                if (mLocalInstallProperties)
                {
                    Engine::Content::tLaunchers launchers = mLocalInstallProperties->GetLaunchers();
                    if (launchers.size() > 0)
                    {
                        const QString locale = mInstalledLocale;
                        for (Engine::Content::tLaunchers::iterator it = launchers.begin(); it != launchers.end(); it++)
                        {
                            Engine::Content::LauncherDescription& launcherDescription = *it;

                            // Only add the file to the list if it exists.
                            if (QFile::exists( Services::PlatformService::localFilePathFromOSPath(launcherDescription.msPath) ))
                            {
                                // If this launcher requires a 64 bit OS, only add it if we're on a 64 bit OS
                                if (launcherDescription.mbRequires64BitOS && !bIs64BitOS)
                                {
                                    ORIGIN_LOG_DEBUG << "Launcher requires 64 bit OS but current OS is not 64 bit.";
                                    continue;
                                }

                                // If this launcher requires a trial offer, only add it if this offer is a trial offer (and vice versa)
                                if (launcherDescription.mbIsTimedTrial != bIsTimedTrial)
                                {
                                    ORIGIN_LOG_DEBUG << "Launcher trial setting does not match offer trial setting. Launcher requires trial setting to match";
                                    continue;
                                }

                                QString localeTitle;
                                if (launcherDescription.mLanguageToCaptionMap.contains(locale))
                                {
                                    localeTitle = launcherDescription.mLanguageToCaptionMap[locale];
                                }

                                // If the localeTitle doesn't match our current locale - use English (US or GB).
                                // If we don't have a English title... Tell the game team to include one. - Alex P.
                                if(localeTitle.isEmpty())
                                {
                                    // I wish there was a way of just returning the first value of "en_" but there isn't.
                                    // The key has to be exact.
                                    ORIGIN_LOG_EVENT << "Looking for EN";
                                    if(launcherDescription.mLanguageToCaptionMap.contains("en_US"))
                                        localeTitle = launcherDescription.mLanguageToCaptionMap["en_US"];
                                    else if(launcherDescription.mLanguageToCaptionMap.contains("en_GB"))
                                        localeTitle = launcherDescription.mLanguageToCaptionMap["en_GB"];

                                    //means they don't have en_US or en_GB defined either, so just use the first one 
                                    if (localeTitle.isEmpty())
                                    {
                                        ORIGIN_LOG_EVENT << "EN not found, use first one";
                                        localeTitle = launcherDescription.mLanguageToCaptionMap.begin().value();
                                    }
                                }
                                launchTitleList.push_back(localeTitle);
                            }
                            else
                            {
                                ORIGIN_LOG_DEBUG << "Launcher specified but not present on system: " << launcherDescription.msPath;
                            }
                        }
                    }
                }
                return launchTitleList;
            }

            const Engine::Content::LauncherDescription* LocalContent::findLauncherByTitle (QString& launchTitle)
            {
                if (mLocalInstallProperties)
                {
                    const QString locale = readSetting(Services::SETTING_LOCALE);
                    const Engine::Content::tLaunchers launchers = mLocalInstallProperties->GetLaunchers();
                    for (int i = 0; i < launchers.size(); i++)
                    {
                        const Engine::Content::LauncherDescription& launcherDescription = launchers[i];

                        QString localeTitle;
                        if (launcherDescription.mLanguageToCaptionMap.contains(locale))
                        {
                            localeTitle = launcherDescription.mLanguageToCaptionMap[locale];
                        }

                        // If the localeTitle doesn't match our current locale - use English (US or GB).
                        // If we don't have a English title... Tell the game team to include one. - Alex P.
                        if(localeTitle.isEmpty())
                        {
                            // I wish there was a way of just returning the first value of "en_" but there isn't.
                            // The key has to be exact.
                            ORIGIN_LOG_EVENT << "Looing for EN";
                            if(launcherDescription.mLanguageToCaptionMap.contains("en_US"))
                                localeTitle = launcherDescription.mLanguageToCaptionMap["en_US"];
                            else if(launcherDescription.mLanguageToCaptionMap.contains("en_GB"))
                                localeTitle = launcherDescription.mLanguageToCaptionMap["en_GB"];

                            //means they don't have en_US or en_GB defined either, so just use the first one 
                            if (localeTitle.isEmpty())
                            {
                                ORIGIN_LOG_EVENT << "EN not found, use first one";
                                localeTitle = launcherDescription.mLanguageToCaptionMap.begin().value();
                            }
                        }
                        if (!localeTitle.isEmpty())
                        {
                            if (launchTitle.compare (localeTitle, Qt::CaseInsensitive) == 0)
                            {
                                //found the launcher that matches
                                return &(launchers[i]);
                            }
                        }
                    }
                }
                return NULL;

            }

            //this validates whether the currently set launcher will work on current OS
            //or if launcher associated with launchTitle is not found, it will also return false
            bool LocalContent::validateLauncher (QString& launchTitle)
            {
                bool valid = true;

                const Engine::Content::LauncherDescription* launcherDesc = findLauncherByTitle (launchTitle);
                if (launcherDesc != NULL)
                {
                    const bool bIsTimedTrial = contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access || contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime;

                    // Launcher must be compatible with OS bitness and trial settings need to match
                    if ((launcherDesc->mbRequires64BitOS && !EnvUtils::GetIS64BitOS()) ||
                        (launcherDesc->mbIsTimedTrial != bIsTimedTrial))
                    {
                        valid = false;
                    }
                }
                else    //couldn't find the expected launcher
                {
                    valid = false;
                }
                return valid;
            }



            // This is based on the handshake described here:
            // http://confluence.ea.com/display/EATech/Origin-Game+RtP+Handshake+mechansim
            // current DateTime must be calculated from UTC
            // Hash Code Generation:
            // quint32 year = YYYY;
            // quint32 month = MM; // January = 1
            // quint32 day = DD; 
            // quint32 hour = hh;
            // quint32 minute = mm;

            // const qint32 prime_10000th = 104729;
            // const qint32 prime_20000th = 224737;
            // const qint32 prime_30000th = 350377;

            // quint32 temp_value = (prime_10000th * year) ^ (month * prime_20000th) ^ (day * prime_30000th);
            // quint32 hashed_timestamp = temp_value ^ (temp_value<<16) ^ (temp_value>>16);
            
            // This should be passed up in the environment variable "EARtPLaunchCode"

            quint32 LocalContent::getRTPHandshakeCode()
            {
                QDateTime dateTime(QDateTime::currentDateTimeUtc());

                quint32 year = dateTime.date().year();
                quint32 month = dateTime.date().month();
                quint32 day = dateTime.date().day();

                const qint32 prime_10000th = 104729;
                const qint32 prime_20000th = 224737;
                const qint32 prime_30000th = 350377;

                quint32 temp_value = (prime_10000th * year) ^ (month * prime_20000th) ^ (day * prime_30000th);
                quint32 hashed_timestamp = temp_value ^ (temp_value<<16) ^ (temp_value>>16);

                return hashed_timestamp;
            }

            bool LocalContent::shouldRestrictUnderageUser()
            {
                bool shouldBlock = false;
                if(contentConfiguration()->blockUnderageUsers() && user()->isUnderAge())
                    shouldBlock = true;

                return shouldBlock;
            }

            void LocalContent::listenForParentStateChange()
            {
                if(!entitlementRef()->parent().isNull() && entitlementRef()->parent()->localContent() && entitlementRef()->parent()->localContent()->installFlow())
                {
                    // This might seem a little strange. However, this prevents us from having duplicate calls if someone calls this multiple times.
                    stopListeningForParentStateChange();
                    ORIGIN_VERIFY_CONNECT(entitlementRef()->parent()->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onParentStateChanged(Origin::Downloader::ContentInstallFlowState)));
                }
            }

            void LocalContent::stopListeningForParentStateChange()
            {
                if(!entitlementRef()->parent().isNull() && entitlementRef()->parent()->localContent() && entitlementRef()->parent()->localContent()->installFlow())
                {
                    ORIGIN_VERIFY_DISCONNECT(entitlementRef()->parent()->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onParentStateChanged(Origin::Downloader::ContentInstallFlowState)));
                }
            }

            void LocalContent::notifyCOPPAError(bool isDownload)
            {
                Error errorType = ErrorCOPPAPlay;
                if(isDownload)
                {
                    errorType = ErrorCOPPADownload;
                }
                emit error(entitlementRef(), errorType);
            }

            void uninstallDLCUsingCRCMap(QString cacheFolderPath, QString gameInstallFolder, QString dlcInstallCheck)
            {
                QString crcMapPath = cacheFolderPath + QDir::separator() + "map.crc";
                QFileInfo crcMapFile(crcMapPath);
                if (crcMapFile.exists())
                {
                    // Get the CRC map key for this particular piece of content (this is cryptic for whatever reason)
                    QString crcMapKey = QString("%1").arg(qHash(dlcInstallCheck));

                    // Get the CRC map and load it
                    Downloader::CRCMap* crcMap = Downloader::CRCMapFactory::instance()->getCRCMap(crcMapPath);
                    crcMap->Load();

                    // Iterate over all the entries
                    int filesRemoved = 0;
                    QList<QString> crcMapKeys = crcMap->GetAllKeys();
                    for (int i = 0; i < crcMapKeys.count(); ++i)
                    {
                        QString curKey = crcMapKeys[i];

                        const Downloader::CRCMapData crcData = crcMap->GetCRC(curKey);

                        // If this file is associated with this content
                        if (crcData.id == crcMapKey)
                        {
                            QString gameFilePath = gameInstallFolder + QDir::separator() + curKey;
                            QFile gameFile(gameFilePath);
                            if (gameFile.exists())
                            {
                                ++filesRemoved;

                                // Delete it from disk
                                gameFile.remove();

                                crcMap->Remove(curKey);
                            }
                        }
                    }

                    // If we deleted any entries from the CRC map, save it
                    if (filesRemoved > 0)
                    {
                        ORIGIN_LOG_EVENT << "Removed " << filesRemoved << " DLC files.";
                        crcMap->Save();
                    }

                    // Release the CRC map
                    Downloader::CRCMapFactory::instance()->releaseCRCMap(crcMap);
                }
            }

            UninstallTask::UninstallTask(const QString& productId, const QString& command, const QString& arguments) : 
                mProductId(productId),
                mCommand(command), 
                mArguments(arguments) 
            {

            }

            void UninstallTask::setManualDLCUninstallArgs(const QString& cacheFolderPath, const QString& gameInstallFolder, const QString& dlcInstallCheck)
            {
                mCacheFolderPath = cacheFolderPath;
                mGameInstallFolder = gameInstallFolder;
                mDlcInstallCheck = dlcInstallCheck;
            }

            void UninstallTask::run()
            {
                if (!mCommand.isEmpty())
                {
                    Origin::Services::IProcess* process = Origin::Services::IProcess::createNewProcess(mCommand, mArguments, QString(), QString(), true, true, true);
                    ORIGIN_LOG_EVENT << "[" << mProductId << "] Calling uninstaller: " << mCommand << " Args: " << mArguments;
                    process->start();
                    bool success = (process->getError() == Origin::Services::IProcess::NoError);
                    if (success)
                    {
                        ORIGIN_LOG_EVENT << "[" << mProductId << "] Uninstall job completed successfully.";
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "[" << mProductId << "] Uninstall job failed, code = " << (int)process->getError();
                    }
                    delete process;
                }
                if (!mCacheFolderPath.isEmpty())
                {
                    ORIGIN_LOG_EVENT << "[" << mProductId << "] Running manual DLC uninstall cleanup.";

                    uninstallDLCUsingCRCMap(mCacheFolderPath, mGameInstallFolder, mDlcInstallCheck);
                }

                emit finished();
            }
        } //namespace Content
    } //namespace Engine
}//namespace Origin



