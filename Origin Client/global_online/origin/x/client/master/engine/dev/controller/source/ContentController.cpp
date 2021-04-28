//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#include <limits>
#include <QDateTime>
#include <QThreadPool>
#include <QtXml/QDomDocument>
#include <QDir>
#include <QDebug>
#include <QtConcurrentRun>
#include <QTimer>
#include <EAStdC/EARandomDistribution.h>

#if defined(ORIGIN_PC)

#include "services/process/ProcessWin.h"

#endif

#include "engine/content/LocalContent.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/RetrieveDiPManifestHelper.h"
#include "engine/content/LocalInstallProperties.h"
#include "engine/subscription/SubscriptionManager.h"
#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "engine/igo/IGOController.h"
#include "engine/ito/InstallFlowManager.h"
#include "engine/config/SharedNetworkOverrideManager.h"

#include "services/session/SessionService.h"
#include "services/debug/DebugService.h"
#include "services/downloader/Common.h"
#include "services/log/LogService.h"
#include "services/escalation/IEscalationClient.h"
#include "services/config/OriginConfigService.h"
#include "services/platform/TrustedClock.h"
#include "services/publishing/CatalogDefinition.h"
#include "services/downloader/DownloadService.h"
#include "services/trials/TrialServiceClient.h"

#include "TelemetryAPIDLL.h"
#include "engine/igo/IIGOWindowManager.h"
#include "engine/achievements/achievementmanager.h"
#include "engine/dirtybits/DirtyBitsClient.h"

// Wait time before searching for and initiating automatic repairs
#define BROKEN_CONTENT_DELAY 30*1000
#define BROKEN_CONTENT_DELAY_ITO_RTP 30*60*1000

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            ContentController::ContentController(Origin::Engine::UserRef user) 
                : m_User(user)
                , m_InitialEntitlementRefreshed(false)
				, m_InitialFullEntitlementRefreshed(false)
                , m_refreshInProgress(false)
                , m_refreshTimeout(10000)
                , mIsLoadingEntitlementsSlow(false)
                , m_autostartPermissionsRequested(false)
                , m_autostartUACFailed(false)
				, m_autoStartItemsOK(false)	// set to true in onImportComplete
                , m_autostartUACPending(false)
                , m_autostartAllowOnExtraDownloads(false)
            {
                ORIGIN_VERIFY_CONNECT(
                    Origin::Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)),
                    this, SLOT (onUserLoggedOut(Origin::Engine::UserRef)));

                setContentFolders(new ContentFolders(this));

                setCustomBoxartController(new CustomBoxartController(this));
                setNonOriginContentController(new NonOriginContentController(this));
                setNucleusEntitlementController(new NucleusEntitlementController());
                setBaseGamesController(new BaseGamesController(this));

                ORIGIN_VERIFY_CONNECT(baseGamesController(), SIGNAL(baseGamesLoaded()), this, SLOT(onBaseGamesLoaded()));
                ORIGIN_VERIFY_CONNECT_EX(baseGamesController(), SIGNAL(refreshComplete()), this, SLOT(onDelayedUpdateFinished()), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(CatalogDefinitionController::instance(), SIGNAL(refreshComplete()), this, SLOT(onDelayedUpdateFinished()), Qt::QueuedConnection);

                m_downloadUtilizationRestoreTimer.setInterval(Downloader::DownloadService::GetGameStartDownloadSuppressionTimeout());
                m_downloadUtilizationRestoreTimer.setSingleShot(true);
                ORIGIN_VERIFY_CONNECT(&m_downloadUtilizationRestoreTimer, SIGNAL(timeout()), this, SLOT(onRestoreDownloadUtilizationTimout()));

                ORIGIN_VERIFY_CONNECT(nucleusEntitlementController(), &NucleusEntitlementController::entitlementSignatureVerificationFailed,
                    this, &ContentController::entitlementSignatureVerificationFailed);

                m_sharedNetworkOverrideManager = new Shared::SharedNetworkOverrideManager(this);
            }

            ContentController::~ContentController()
            {
                delete nonOriginController();
                delete boxartController();
                delete nucleusEntitlementController();
                baseGamesController()->deleteLater();

                //it appears that we are still receiving signals even after destruction so this is supposed to disconnect EVERYTHING
                ORIGIN_VERIFY_DISCONNECT (this, NULL, NULL, NULL);
            }

            ContentController * ContentController::create(Origin::Engine::UserRef user)
            {
                return new ContentController(user);
            }

            void ContentController::refreshAchievements(EntitlementRefList refreshEntitlements, bool refreshUserPoints)
            {
                if(Achievements::AchievementManager::instance() == NULL)
                    return;
                Achievements::AchievementManager::instance()->initializeEventHandler();

                QStringList achievementSetIds;
                EntitlementRefList entitlementsList;

                // TODO: Make this a little more general
                //achievementSetIds.append("origin");
                //entitlementsList.append(EntitlementRef());

                foreach(Origin::Engine::Content::EntitlementRef entitlement, refreshEntitlements)
                {
                    //TODO (Hans): Make sure that when we are supporting more than two platforms this code gets modified to take that into account.
                    QString achievementSetId = entitlement->contentConfiguration()->achievementSet();

                    if(achievementSetId.isEmpty())
                    {
                        achievementSetId = entitlement->contentConfiguration()->achievementSet(Origin::Services::PlatformService::PlatformWindows);
                    }

                    if(!achievementSetId.isEmpty() && !achievementSetIds.contains(achievementSetId))
                    {
                        achievementSetIds.append(achievementSetId);
                        entitlementsList.append(entitlement);
                    }
                }

                // TODO: we may want to refresh all achievements here, not only the users.
                if(achievementSetIds.size()>0)
                {
                    if(Origin::Engine::Achievements::AchievementManager::instance()->currentUsersPortfolio() == NULL)
                        return;
                    Origin::Engine::Achievements::AchievementManager::instance()->currentUsersPortfolio()->updateAchievementSets(achievementSetIds, entitlementsList);
                }

                if(refreshUserPoints)
                {
                    Origin::Engine::Achievements::AchievementPortfolioRef portfolio = Origin::Engine::Achievements::AchievementManager::instance()->currentUsersPortfolio();
                    if (!portfolio.isNull())
                    {
                        portfolio->refreshPoints();
                    }

					Origin::Engine::Achievements::AchievementManager::instance()->updateAchievementSetReleaseInfo();
                }
				
                Origin::Engine::Achievements::AchievementManager::instance()->grantQueuedAchievements();
                Origin::Engine::Achievements::AchievementManager::instance()->postQueuedEvents();
            }

            ContentController * ContentController::currentUserContentController()
            {
                // it is possible for the user to be gone so check first to avoid a crash.
                Origin::Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                if (user.isNull())
                    return NULL;

                return user->contentControllerInstance();
            }

            UserRef ContentController::user() const
            {
                return m_User.toStrongRef();
            }

            Services::Session::SessionRef ContentController::session()
            {
                // use local shared pointer to keep the user instance alive
                UserRef usr = user();
                ORIGIN_ASSERT(usr);
                if ( usr ) 
                    return usr->getSession();
                else
                    return Services::Session::SessionRef();
            }

            int ContentController::numberOfNonOriginEntitlements()
            {
                return static_cast<int>(nonOriginController()->nonOriginGames().size());
            }

            QStringList ContentController::nonOriginEntitlementIds()
            {
                QStringList ids;

                foreach (EntitlementRef e, nonOriginController()->nonOriginGames())
                {
                    ids.append(e->contentConfiguration()->productId());
                }

                return ids;
            }

            EntitlementRefList ContentController::entitlements(EntitlementFilter filter /*= OwnedContent*/) const 
            { 
                // Use a set to prevent duplicates.
                EntitlementRefSet allContent;

                foreach (const EntitlementRef baseGame, baseGamesController()->baseGames())
                {
                    if (filter & OwnedContent)
                    {
                        allContent.insert(baseGame);
                    }

                    foreach (const EntitlementRef child, baseGame->children(filter))
                    {
                        allContent.insert(child);
                    }
                }

                if (filter & OwnedContent)
                {
                    allContent += nonOriginController()->nonOriginGames();
                }

                return allContent.toList();
            }

            EntitlementRefList ContentController::entitlementByState(LocalContent::States any, LocalContent::States all, LocalContent::States none, EntitlementFilter filter) const
            {
                EntitlementRefList ret;

                EntitlementRefList allContent = entitlements(filter);
                foreach(EntitlementRef e, allContent)
                {
                    LocalContent::States states = e->localContent()->state();
                    bool _any  = (any  == 0) || (any  & states) != 0;
                    bool _all  = (all  == 0) || (all  & states) == all;
                    bool _none = (none == 0) || (none & states) == 0;
                    if ( _any && _all && _none )
                        ret.push_back(e);
                }
                return ret;
            }

            void ContentController::checkIfOfferVersionIsUpToDate(Origin::Engine::Content::EntitlementRef entitlement)
            {
                const QString& offerId = entitlement->contentConfiguration()->productId();
                const QString& installedVersion = entitlement->localContent()->installedVersion();

                ORIGIN_LOG_ACTION << "Determining if product build version definition for [" << offerId << "] is up to date.  Installed version: " << installedVersion;

                if (!entitlement->contentConfiguration()->hasOverride() &&
                    Origin::Services::Connection::ConnectionStatesService::isUserOnline (LoginController::instance()->currentUser()->getSession()))
                {
                    ORIGIN_LOG_ACTION << "Starting server version update check for [" << offerId << "].";

                    GetTelemetryInterface()->Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK(offerId.toLocal8Bit().constData(), installedVersion.toLocal8Bit().constData());
                    CatalogDefinitionQuery* query = CatalogDefinitionController::instance()->queryCatalogDefinition(offerId,
                        entitlement->contentConfiguration()->qualifyingOfferId(), Services::Publishing::INVALID_BATCH_TIME, CatalogDefinitionController::RefreshAll,
                        entitlement->contentConfiguration()->originPermissions() > Services::Publishing::OriginPermissionsNormal);
                    ORIGIN_VERIFY_CONNECT(query, SIGNAL(finished()), this, SLOT(onOfferVersionUpdateCheckComplete()));
                    return;
                }

                // If we're using an override or are offline, there is no point in trying to check the CDN catalog data
                emit offerVersionUpdateCheckComplete(offerId, !entitlement->localContent()->updateAvailable());
            }

            void ContentController::onOfferVersionUpdateCheckComplete()
            {
                CatalogDefinitionQuery* query = dynamic_cast<CatalogDefinitionQuery*>(sender());
                if(query != NULL)
                {    
                    bool upToDate = true;
                    EntitlementRef entitlement = entitlementByProductId(query->offerId());
                    if(!entitlement.isNull())
                    {
                        QString localVersion = entitlement->localContent()->installedVersion();
                        QString liveServerVersion = entitlement->contentConfiguration()->version();

                        GetTelemetryInterface()->Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED(query->offerId().toLocal8Bit().constData(), 
                            localVersion.toLocal8Bit().constData(), liveServerVersion.toLocal8Bit().constData());

                        VersionInfo localVersionInfo(localVersion);
                        VersionInfo liveServerVersionInfo(liveServerVersion);

                        if(localVersionInfo != liveServerVersionInfo)
                        {
                            ORIGIN_LOG_EVENT << "Detected live version change for product [" << query->offerId() << "] from installed version [" << localVersion << "] to live version [" <<
                                liveServerVersion << "].";

                            // The version has changed - update our local representation with the server version, and trigger a full offer refresh in background just in
                            // case anything else regarding suppression has changed
                            upToDate = false;
                        }
                    }

                    emit offerVersionUpdateCheckComplete(query->offerId(), upToDate);

                    query->deleteLater();
                }
            }

            void ContentController::refreshUserGameLibrary(Origin::Engine::Content::ContentRefreshMode mode)
            {
                ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Engine::Content::ContentRefreshMode, mode));

                if (!m_refreshInProgress)
                {
                    m_refreshInProgress = true;
                    emit updateStarted();

                    setupSharedNetworkOverrides();

                    baseGamesController()->refresh(mode);

                    if(mode != CacheOnly)
                    {
                        CatalogDefinitionController::instance()->refreshDefinitions(CatalogDefinitionController::RefreshAll);
                    }
                }
            }

            void ContentController::initUserGameLibrary() 
            {
                if (!m_refreshInProgress)
                {
                    m_refreshInProgress = true;
                    emit updateStarted();

                    //reload the product override settings from eacore.ini, so user doesn't have to restart when modifying EACore.ini
                    if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
                    {
                        Services::SettingsManager::instance()->reloadProductOverrideSettings();
                    }

                    CatalogDefinitionController::instance()->readUserCacheData();
                    nucleusEntitlementController()->initUserEntitlements();
                }
            }

            void ContentController::setupSharedNetworkOverrides()
            {
                // In addition to creating new shared network overrides, we need to delete any existing ones that are no longer required.
                // We create a list of the offerId's of all existing overrides, remove ones that we want to keep, and then finally delete the remaining ones.
                QStringList overridesToDelete = m_sharedNetworkOverrideManager->overrideList();
                
                EntitlementRefList ents = entitlements();

                foreach(EntitlementRef ent, ents)
                {
                    const QString &overrideSharedNetworkFolder = ent->contentConfiguration()->getProductOverrideValue(Services::SETTING_OverrideSNOFolder.name());

                    if (!overrideSharedNetworkFolder.isEmpty())
                    {
                        m_sharedNetworkOverrideManager->createSharedNetworkOverride(ent);
                        overridesToDelete.removeAll(ent->contentConfiguration()->productId());
                    }
                }

                foreach(QString overrideToDelete, overridesToDelete)
                {
                    m_sharedNetworkOverrideManager->deleteSharedNetworkOverride(overrideToDelete);
                }
            }

            void ContentController::onBaseGamesLoaded()
            {
                if (m_User.isNull())
                {
                    emit updateFinished(EntitlementRefList(), EntitlementRefList());
                    return;
                }

                TIME_BEGIN("ContentController::onBaseGamesLoaded")

                // Get all the base games...
                foreach (BaseGameRef baseGame, baseGamesController()->baseGames())
                {
                    addOwnedContent(baseGame);
                }

                // Go ahead and load up NOGs...
                nonOriginController()->init();
                foreach (EntitlementRef nog, nonOriginController()->nonOriginGames())
                {
                    addOwnedContent(nog);
                }

                ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
                    this, SLOT(onConnectionStateChanged(bool)));

                const EntitlementRefList &ownedContent = entitlements(OwnedContent);

                // Some ODT specific stuff that happens after initial game load...
                if (Origin::Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled) &&
                    Origin::Services::readSetting(Services::SETTING_PurgeAccessLicenses).toQVariant().toBool())
                {
                    purgeLicenses(ownedContent);
                }

                // Process extra content just this once explicitly.  All others come in through extra content updated signal..
                foreach (BaseGameRef baseGame, baseGamesController()->baseGames())
                {
                    if (baseGame->contentConfiguration()->addonsAvailable())
                    {
                        EntitlementRefList throwAwayList;
                        processExtraContent(baseGame, throwAwayList);
                    }
                }

                // Reload overrides in case there have been any changes that we're unaware of.
                foreach (BaseGameRef baseGame, baseGamesController()->baseGames())
                {
                    baseGame->contentConfiguration()->reloadOverrides();

                    if (baseGame->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access
                        || baseGame->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                    {
                        auto resp = Origin::Services::Trials::TrialServiceClient::checkTime(baseGame->contentConfiguration()->contentId());
                        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), baseGame->localContent(), SLOT(updateTrialTime()));
                    }
                }
                setupSharedNetworkOverrides();

                emit updateFinished(ownedContent, EntitlementRefList());

                // Now we got our entitlement lets refresh the achievements.  We only refresh newly added achievements on incremental refreshes
                // We always refresh the user points on each refresh.
                refreshAchievements(ownedContent, true);

                {
                    QMutexLocker locker(&m_entitlementInitialCheckLock);
                    m_InitialEntitlementRefreshed = true;
                }

                emit entitlementsInitialized();

                TIME_END("ContentController::onBaseGamesLoaded");

                ORIGIN_VERIFY_CONNECT(nonOriginController(), SIGNAL(nonOriginGameAdded(Origin::Engine::Content::EntitlementRef)), this, SLOT(onNonOriginGameAdded(Origin::Engine::Content::EntitlementRef)));
                ORIGIN_VERIFY_CONNECT(nonOriginController(), SIGNAL(nonOriginGameRemoved(Origin::Engine::Content::EntitlementRef)), this, SLOT(onNonOriginGameRemoved(Origin::Engine::Content::EntitlementRef))); 
                ORIGIN_VERIFY_CONNECT(baseGamesController(), SIGNAL(baseGameAdded(Origin::Engine::Content::BaseGameRef)), this, SLOT(onBaseGameAdded(Origin::Engine::Content::BaseGameRef)));
                ORIGIN_VERIFY_CONNECT(baseGamesController(), SIGNAL(baseGameRemoved(Origin::Engine::Content::BaseGameRef)), this, SLOT(onBaseGameRemoved(Origin::Engine::Content::BaseGameRef)));
                ORIGIN_VERIFY_CONNECT(baseGamesController(), SIGNAL(baseGameExtraContentUpdated(Origin::Engine::Content::BaseGameRef)), this, SLOT(onBaseGameExtraContentUpdated(Origin::Engine::Content::BaseGameRef)));
            }

            void ContentController::processExtraContent(BaseGameRef baseGame, EntitlementRefList& addedEntitlementList)
            {
                EntitlementRefList newExtraContent;
                QSet<QString> previouslyStartedPDLC;

                foreach (EntitlementRef extraContent, baseGame->children())
                {
                    const QString &productId = extraContent->contentConfiguration()->productId();
                    if (m_processedExtraContent.contains(productId))
                    {
                        previouslyStartedPDLC.insert(productId);
                    }
                    else
                    {
                        m_processedExtraContent.insert(productId);
                        addOwnedContent(extraContent);
                        newExtraContent.push_back(extraContent);

                        // check to see if this DLC is one that was just purchased and needs to start downloading even if auto-updates are off (EBIBUGS-28701)
                        if (mPendingPurchasedDLCDownloadList.contains(productId))
                        {
                            baseGame->localContent()->appendPendingPurchasedDLCDownloads(productId);
                            mPendingPurchasedDLCDownloadList.remove(productId);
                        }
                    }
                }

                if(!newExtraContent.isEmpty())
                {
                    addedEntitlementList.append(newExtraContent);
                }
            }

            void ContentController::onBaseGameExtraContentUpdated(Origin::Engine::Content::BaseGameRef baseGame) 
            {
                EntitlementRefList newExtraContent;
                processExtraContent(baseGame, newExtraContent);

                if(!newExtraContent.isEmpty())
                {
                    refreshAchievements(newExtraContent, false);
                    emit updateFinished(newExtraContent, EntitlementRefList());
                }
            }

            EntitlementRef ContentController::addOwnedContent(EntitlementRef ownedContent)
            {
                ORIGIN_VERIFY_CONNECT(ownedContent->localContent(), SIGNAL(flowIsActive(const QString &, const bool &)),
                    this, SLOT(onFlowActiveStateCheck(const QString &, const bool &)));

                ORIGIN_VERIFY_CONNECT(ownedContent->localContent(), SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)),
                    this, SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)));
                ORIGIN_VERIFY_CONNECT(ownedContent->localContent(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)),
                    this, SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)));

                ORIGIN_VERIFY_CONNECT(ownedContent->localContent(), SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)),
                    this, SLOT(onPlayStarted(Origin::Engine::Content::EntitlementRef)));
                ORIGIN_VERIFY_CONNECT(ownedContent->localContent(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)),
                    this, SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)));

                return ownedContent;
            }

            void ContentController::onNonOriginGameAdded(Origin::Engine::Content::EntitlementRef nog)
            {
                addOwnedContent(nog);
                emit updateFinished(EntitlementRefList() << nog, EntitlementRefList());
            }

            void ContentController::onNonOriginGameRemoved(Origin::Engine::Content::EntitlementRef nog)
            {
                emit updateFinished(EntitlementRefList(), EntitlementRefList() << nog);
            }

            void ContentController::onBaseGameAdded(Origin::Engine::Content::BaseGameRef baseGame)
            {
                EntitlementRefList addedEntitlements;

                addOwnedContent(baseGame);

                addedEntitlements.push_back(baseGame);
                if(baseGame->contentConfiguration()->addonsAvailable())
                {
                    processExtraContent(baseGame, addedEntitlements);
                }

                if (baseGame->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access
                    || baseGame->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                {
                    auto resp = Origin::Services::Trials::TrialServiceClient::checkTime(baseGame->contentConfiguration()->contentId());
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), baseGame->localContent(), SLOT(updateTrialTime()));
                }

                refreshAchievements(addedEntitlements, true);

                emit updateFinished(addedEntitlements, EntitlementRefList());
            }

            void ContentController::onBaseGameRemoved(Origin::Engine::Content::BaseGameRef baseGame)
            {
                emit updateFinished(EntitlementRefList(), EntitlementRefList() << baseGame);
            }

            void ContentController::onDelayedUpdateFinished()
            {
                if(!baseGamesController()->refreshInProgress() && !CatalogDefinitionController::instance()->refreshInProgress())
                {
                    m_refreshInProgress = false;

                    // Reload overrides in case there have been any changes that we're unaware of.
                    const EntitlementRefList& allContent = entitlements(AllContent);
                    for (auto citer = allContent.constBegin(); citer != allContent.end(); ++citer)
                    {
                        (*citer)->contentConfiguration()->reloadOverrides();
                    }

                    if (!Engine::LoginController::instance()->isUserLoggedIn())
                    {
                        ORIGIN_LOG_EVENT << "Detected user logged out while processing overrides post-refresh.";
                        return;
                    }

                    setupSharedNetworkOverrides();

                    if(contentFolders()->cacheFolderValid() && contentFolders()->downloadInPlaceFolderValid())
                    {
                        // Kick off auto-updates, etc
                        processDownloaderAutostartItems();

                        // must protect against user logging out which results in a null queueController
                        Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
                        if (queueController)
                            queueController->postFullRefresh();
                    }
                    else
                    {
                        ORIGIN_VERIFY_CONNECT(contentFolders(), SIGNAL(contentFoldersValid()), this, SLOT(onContentFoldersValidForAutoDownloads()));
                        emit(invalidContentFolders());
                    }

                    // Clear the dip manifest cache so that refreshing entitlements is enough to trigger update if user updated the DiP package version
                    ORIGIN_LOG_DEBUG << "Clearing DiP Manifest Cache.";
                    Origin::Downloader::RetrieveDiPManifestHelper::clearManifestCache();


                    // We have to keep our PromoBrowserOfferCache in sync at all times...
                    // If user is configured for promo manager v2, we need to cache the set of owned offer ids.  If user owns no entitlements,
                    // "opm_none" is expected.
                    QStringList promoOfferIds;

                    const EntitlementRefList& entList = entitlements();
                    foreach(const EntitlementRef ent, entList)
                    {
                        const QString& productId = ent->contentConfiguration()->productId();
                        if (!productId.isEmpty())
                        {
                            const Services::Publishing::NucleusEntitlementList& nucleusEntList = nucleusEntitlementController()->entitlements(productId);
                            foreach(const Services::Publishing::NucleusEntitlementRef nucleusEnt, nucleusEntList)
                            {
                                if(ent->contentConfiguration()->isEntitledFromSubscription())
                                {
                                    promoOfferIds.push_back(QString("%1_vault").arg(productId));
                                }
                                else if (0 == nucleusEnt->source().compare("Ebisu-Platform", Qt::CaseInsensitive) && 
                                    ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeNormalGame)
                                {
                                    promoOfferIds.push_back(QString("%1_OTH").arg(productId));
                                    break;
                                }
                            }

                            promoOfferIds.push_back(productId);
                        }
                    }

                    if(promoOfferIds.empty())
                    {
                        promoOfferIds.push_back(Origin::Services::PROMOBROWSEROFFERCACHE_NO_ENTITLEMENTS_SENTINEL);
                    }

                    Services::writeSetting(Services::SETTING_PromoBrowserOfferCache, promoOfferIds.join(","), Services::Session::SessionService::currentSession());

                    emit updateFinished(EntitlementRefList(), EntitlementRefList());

                    if (!m_InitialFullEntitlementRefreshed)
                    {
                        m_InitialFullEntitlementRefreshed = true;
                        emit initialFullUpdateCompleted();
                    }
                }
            }

            void ContentController::onContentFoldersValidForAutoDownloads()
            {
                ORIGIN_VERIFY_DISCONNECT(contentFolders(), SIGNAL(contentFoldersValid()), this, SLOT(onContentFoldersValidForAutoDownloads()));

                // Kick off auto-updates, etc
                processDownloaderAutostartItems();
            }

            bool ContentController::validateContentFolder(bool validateDipFolder)
            {
                bool result = true;

                if(validateDipFolder)
                {
#if defined(ORIGIN_PC)
                    result = contentFolders()->downloadInPlaceFolderValid() && QDir(contentFolders()->downloadInPlaceLocation()).exists();
#elif defined(ORIGIN_MAC)
                    // verifyMountPoint() is necessary for Mac in case the path is set for a network drive that has just been disconnected, EBIBUGS-18578
                    result = contentFolders()->downloadInPlaceFolderValid() && contentFolders()->verifyMountPoint(contentFolders()->downloadInPlaceLocation());
#endif
                    if (!result)
                    {
                        emit(folderError(FolderErrors::DIP_INVALID));
                    }
                }
                else
                {
#if defined(ORIGIN_PC)
                    result = contentFolders()->cacheFolderValid() && QDir(contentFolders()->installerCacheLocation()).exists();
#elif defined(ORIGIN_MAC)
                    // verifyMountPoint() is necessary for Mac in case the path is set for a network drive that has just been disconnected, EBIBUGS-18578
                    result = contentFolders()->cacheFolderValid() && contentFolders()->verifyMountPoint(contentFolders()->installerCacheLocation());
#endif
                    if (!result)
                    {
                        emit(folderError(FolderErrors::CACHE_INVALID));
                    }
                }

                if(!result)
                {
                    emit(invalidContentFolders());
                }

                return result;
            }

            EntitlementRef ContentController::entitlementById(const QString &id, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRef ret = entitlementByContentId(id, filter);
                if(!ret.isNull())
                {
                    return ret;
                }

                EntitlementRefList entitlementsbyMasterTitleId = entitlementsBaseByMasterTitleId(id);
                if (entitlementsbyMasterTitleId.isEmpty() == false)
                {
                    ret = entitlementsbyMasterTitleId.first();
                }
                if(!ret.isNull())
                {
                    return ret;
                }

                return entitlementByProductId(id, filter);
            }


            EntitlementRefList  ContentController::entitlementsById(const QString &id, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRefList matched_entitlements;
                matched_entitlements = entitlementsByContentId(id, filter);

                if(matched_entitlements.size() > 0)
                {
                    return matched_entitlements;
                }

                //we won't have multiple entitlements with the same productId, so just look for the one
                EntitlementRef ret = entitlementByProductId(id, filter);
                if (!ret.isNull())
                {
                    matched_entitlements.push_back(ret);
                }

                return matched_entitlements;
            }

            EntitlementRefList ContentController::entitlementsByTimedTrialAssociation(const QString &contentId, EntitlementFilter filter)
            {
                EntitlementRefList matched_entitlements;

                if(!contentId.isEmpty())
                {
                    foreach(const EntitlementRef ent, entitlements(filter))
                    {
                        if(ent->contentConfiguration()->isBaseGame() &&
                               (
                                   ent->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access ||
                                   ent->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime
                               ))
                        {
                            // Go through all entitlements and find ones that have the id in their contentId list in the manifest.
                            Downloader::LocalInstallProperties * localInstallProperties = ent->localContent()->localInstallProperties();

                            if(localInstallProperties != NULL)
                            {
                                foreach(auto cid, localInstallProperties->GetContentIDs())
                                {
                                    if(cid == contentId)
                                    {
                                        matched_entitlements.push_back(ent);
                                    }
                                }
                            }
                        }
                    }
                }

                return matched_entitlements;
            }

            EntitlementRef ContentController::entitlementByContentId(const QString &contentId, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRefList entitlements = entitlementsByContentId(contentId, filter);

                if (!entitlements.isEmpty())
                {
                    return entitlements.front();
                }
                else
                {
                    return EntitlementRef();
                }
            }

            QList<Origin::Engine::Content::EntitlementRef> ContentController::entitlementsByContentId(const QString &contentId, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRefList matched_entitlements;

                if (!contentId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        if (0 == ent->contentConfiguration()->contentId().compare(contentId, Qt::CaseInsensitive))
                        {
                            matched_entitlements.push_back(ent);
                        }
                    }
                }

                return matched_entitlements;
            }

            EntitlementRefList ContentController::entitlementsBaseByMasterTitleId(const QString& masterTitleID)
            {
                EntitlementRefList matched_entitlements;

                if (!masterTitleID.isEmpty())
                {
                    foreach(EntitlementRef e, baseGamesController()->baseGames())
                    {
                        if (e->contentConfiguration()->masterTitleId().compare(masterTitleID, Qt::CaseInsensitive) == 0)
                        {
                            matched_entitlements.push_back(e);
                        }
                    }
                }

                return matched_entitlements;
            }

            EntitlementRefList ContentController::entitlementsBaseByMasterTitleId_ExcludeSubscription(const QString& masterTitleID)
            {
                EntitlementRefList matched_entitlements;
                if (!masterTitleID.isEmpty())
                {
                    foreach(EntitlementRef e, baseGamesController()->baseGames())
                    {
                        if (e->contentConfiguration()->isEntitledFromSubscription() == false
                            && e->contentConfiguration()->masterTitleId().compare(masterTitleID, Qt::CaseInsensitive) == 0)
                        {
                            matched_entitlements.push_back(e);
                        }
                    }
                }
                return matched_entitlements;
            }

            EntitlementRefList ContentController::entitlementsByMasterTitleId(const QString &masterTitleId, bool includeAlternateMatches, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRefList matched_entitlements;

                if (!masterTitleId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        ContentConfigurationRef config = ent->contentConfiguration();
                        if (config->masterTitleId() == masterTitleId ||
                            (includeAlternateMatches && config->alternateMasterTitleIds().contains(masterTitleId)))
                        {
                            matched_entitlements.push_back(ent);
                        }
                    }
                }

                return matched_entitlements;
            }

            EntitlementRef ContentController::entitlementByMultiplayerId(const QString &multiplayerId, EntitlementFilter filter /*= OwnedContent*/)
            {
                if (!multiplayerId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        ContentConfigurationRef config = ent->contentConfiguration();
                        if (multiplayerId == config->multiplayerId(Services::PlatformService::PlatformThis) ||
                            multiplayerId == config->multiplayerId(Services::PlatformService::PlatformOther))
                        {
                            return ent;
                        }
                    }
                }

                return EntitlementRef();
            }

            EntitlementRef ContentController::entitlementByProductId(const QString &productId, EntitlementFilter filter /*= OwnedContent*/)
            {
                if (!productId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        if (0 == ent->contentConfiguration()->productId().compare(productId, Qt::CaseInsensitive))
                        {
                            return ent;
                        }
                    }
                }

                return EntitlementRef();
            }

            EntitlementRef ContentController::entitlementBySoftwareId(const QString &softwareId, EntitlementFilter filter /*= OwnedContent*/)
            {
                // :TODO: should this return a list of entitlements? Is it possible to have multiple
                // entitlements that have the same software ID?
                if (!softwareId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        if (0 == ent->contentConfiguration()->softwareId().compare(softwareId, Qt::CaseInsensitive))
                        {
                            return ent;
                        }
                    }
                }

                return EntitlementRef();
            }
            
            EntitlementRef ContentController::bestMatchedEntitlementByIds(const QStringList &ids, EntitlementLookupType lookupType, bool baseGamesOnly, EntitlementFilter filter /*= OwnedContent*/)
            {
                EntitlementRefList matchingEnts;
                EntitlementRef firstInstalledEnt;
                EntitlementRef firstFoundEnt;

                //we're going to loop through the list of ids passed to us and attempt to find 3 entitlements
            
                //1. The first entitlement found (regardless of install state)
                //2. The first playable entitlement
                //3. The first installed entitlement (I think 99% of the time the playable and installed entitlement will be the same)

                foreach(QString id, ids)
                {
                    switch(lookupType)
                    {
                        case EntByOfferId:
                            matchingEnts += entitlementByProductId(id, filter);
                            break;
                        case EntByContentId:
                            matchingEnts += entitlementsByContentId(id, filter);
                            break;
                        case EntByMasterTitleId:
                            matchingEnts += entitlementsByMasterTitleId(id, filter);
                            break;
                        case EntById:
                        default:
                            matchingEnts += entitlementByProductId(id, filter);
                            matchingEnts += entitlementsByContentId(id, filter);
                            matchingEnts += entitlementsByMasterTitleId(id, filter);
                            break;
                    }
                }

                foreach(EntitlementRef entRef, matchingEnts)
                {
                    if(entRef && (!baseGamesOnly || entRef->contentConfiguration()->isBaseGame()))
                    {
                        //if we didn't already set our first found entitlement, save it off
                        if (!firstFoundEnt)
                        {
                            firstFoundEnt = entRef;
                        }

                        //if this entitlement is installed and we haven't found our first installed, save it off
                        if (!firstInstalledEnt)
                        {
                            if(entRef->localContent()->installed(true))
                                firstInstalledEnt = entRef;
                        }

                        //if this entitlement is playable, there is no sense in continuing--just return immediately.
                        if (entRef->localContent()->playable())
                        {
                            return entRef;
                        }
                    }
                }

                //if no playable check for installed
                if(firstInstalledEnt)
                    return firstInstalledEnt;

                //if no playable or installed, return the first found
                return firstFoundEnt;
            }


            EntitlementRef ContentController::entitlementOrChildrenByProductId(const QString &productId, EntitlementFilter filter /*= OwnedContent*/)
            {
                if (!productId.isEmpty())
                {
                    foreach (const EntitlementRef ent, entitlements(filter))
                    {
                        if (ent->contentConfiguration()->productId() == productId)
                        {
                            return ent;
                        }
                    }
                }

                return EntitlementRef();
            }


            void ContentController::onFlowActiveStateCheck(const QString &productId, const bool &active)
            {
                EntitlementRef entitlement = entitlementByProductId(productId, AllContent);

                if(entitlement)
                {
                    QString hashKeyProductId = entitlement->contentConfiguration()->productId();

                    if(!m_productIdToEntitlementRefHash.contains(hashKeyProductId, entitlement))
                    {
                        //if were not in the active list and we are active add it
                        if(active)
                        {
                            m_productIdToEntitlementRefHash.insert(hashKeyProductId, entitlement);
                            emit(activeFlowsChanged(productId, active));
                        }
                    }
                    else
                    {
                        //if we are in the active list and we are not active remove it
                        if(!active)
                        {
                            m_productIdToEntitlementRefHash.remove(hashKeyProductId, entitlement);
                            emit(activeFlowsChanged(productId, active));
                        }
                    }

                    // Maintain a separate accounting of install folder usage for base games only (DLC can co-exist)
                    if (!entitlement->contentConfiguration()->isPDLC())
                    {
                        QString installFolder = entitlement->localContent()->cacheFolderPath();
                        if (entitlement->contentConfiguration()->dip())
                        {
                            installFolder = entitlement->localContent()->dipInstallPath();
                        }
                        installFolder.replace("\\", "/");
                        if (!installFolder.endsWith("/"))
                        {
                            installFolder.append("/");
                        }

                        if(!m_installFolderToEntitlementRefHash.contains(installFolder, entitlement))
                        {
                            if (active)
                            {
                                m_installFolderToEntitlementRefHash.insert(installFolder, entitlement);
                            }
                        }
                        else
                        {
                            if (!active)
                            {
                                m_installFolderToEntitlementRefHash.remove(installFolder, entitlement);
                            }
                        }
                    }
                }
            }


            bool ContentController::removeInstallPathToEntitlement(const QString& productId, QString& oldDownloadPath)
            {
                EntitlementRef entitlement = entitlementByProductId(productId, AllContent);
                bool success = false;
                if(entitlement)
                {
                    oldDownloadPath.replace("\\", "/");
                    if (!oldDownloadPath.endsWith("/"))
                    {
                        oldDownloadPath.append("/");
                    }

                    if(m_installFolderToEntitlementRefHash.contains(oldDownloadPath, entitlement))
                    {
                        success = m_installFolderToEntitlementRefHash.remove(oldDownloadPath, entitlement);
                    }
                }
                return success;
            }

            bool ContentController::hasEntitlementUpgraded(EntitlementRef entitlement, EntitlementRef* upgradedEntitlement)
            {
                if(entitlement &&
                   entitlement->contentConfiguration()->owned() &&                                              // We bought the game
                   entitlement->contentConfiguration()->isBaseGame() &&                                         // It is a basegame
                   !entitlement->contentConfiguration()->isEntitledFromSubscription() &&                        // We didn't gotten it from the subscription
                   !entitlement->contentConfiguration()->suppressVaultUpgrade() &&                              // The game is not marked as not upgradeable 
                   !Subscription::SubscriptionManager::instance()->isAvailableFromSubscription(entitlement->contentConfiguration()->productId()))  // The game is not contained in the subscription
                {
                    foreach (const EntitlementRef& subEntitlement, entitlementsByMasterTitleId(entitlement->contentConfiguration()->masterTitleId()))
                    {
                        if (subEntitlement->contentConfiguration()->owned() &&                                   // We bought the game
                            subEntitlement->contentConfiguration()->isBaseGame() &&                              // It is a basegame
                            subEntitlement->contentConfiguration()->isEntitledFromSubscription() &&              // It is part of the subscription
                            subEntitlement->contentConfiguration()->rank() > entitlement->contentConfiguration()->rank())               // It is more 'colorful' than the one we bought.
                        {
                            if(upgradedEntitlement)
                                *upgradedEntitlement = subEntitlement;
                            return true;
                        }
                    }
                }
                return false;
            }

            bool ContentController::isEntitlementSuppressed(EntitlementRef entitlement, EntitlementRef * suppressingEntitlement)
            {
                // If the current entitlement is not an subscription entitlement, and there is a subscription entitlement with the same masterTitleId
                // --> suppress.

                if (Engine::Subscription::SubscriptionManager::instance()->isActive())
                {
#if defined(ORIGIN_MAC)
                    {
                        // OFM-10723: Disable subs for mac. Don't suppress owned titles
                        ORIGIN_LOG_EVENT << "Suppressed: false - Mac";
                        return false;
                    }
#endif
                    if(Engine::Subscription::SubscriptionManager::instance()->offlinePlayRemaining() <= 0)
                    {
                        ORIGIN_LOG_EVENT << "Suppressed: false - no offline play remaining";
                        return false;
                    }
                    else if(hasEntitlementUpgraded(entitlement, suppressingEntitlement))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ContentController::folderPathInUse(const QString &folderPath)
            {
                bool inUse = false;
                QString key(folderPath);

                key.replace("\\", "/");
                if (!key.endsWith("/"))
                {
                    key.append("/");
                }

                if(m_installFolderToEntitlementRefHash.contains(key))
                    inUse = true;

                return inUse;
            }

            EntitlementRefList ContentController::getEntitlementsAccessingFolder(const QString &folderPath)
            {
                QString key(folderPath);
                key.replace("\\", "/");
                if (!key.endsWith("/"))
                {
                    key.append("/");
                }

                return m_installFolderToEntitlementRefHash.values(key);
            }

            bool ContentController::isNonDipFlowActive()
            {
                bool running = false;
                for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                    it != m_productIdToEntitlementRefHash.constEnd();
                    it++)
                {
                    if(!it->isNull() && !(*it)->contentConfiguration()->dip())
                    {
                        running = true;
                        break;
                    }
                }
                return running;

            }

            bool ContentController::isDipFlowActive()
            {
                bool running = false;
                for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                    it != m_productIdToEntitlementRefHash.constEnd();
                    it++)
                {
                    if(!it->isNull() && (*it)->contentConfiguration()->dip())
                    {
                        running = true;
                        break;
                    }
                }
                return running;
            }

            //similar to DipFlowActive but does a special case check for ITO and if it hasn't started transferring yet, then it's ok to change
            //this is to allow the user to change the directory via the ITO Prepare Download dialog
            bool ContentController::canChangeDipInstallLocation()
            {
                // This code is disabled because we no longer want to prevent the user from changing download directories when flows are active.
                // Leaving this code in place in case we need this functionality for something else later

                //bool canChange = true;
                //for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                //    it != m_productIdToEntitlementRefHash.constEnd();
                //    it++)
                //{
                //    if(!it->isNull() && (*it)->contentConfiguration()->dip())
                //    {
                //        //check if it's ITO and if the current downloading state is kPendingInstallInfo
                //        Origin::Downloader::IContentInstallFlow *iFlow = (*it)->localContent()->installFlow();
                //        if (iFlow && iFlow->isUsingLocalSource() && iFlow->getFlowState() == Origin::Downloader::ContentInstallFlowState::kPendingInstallInfo)
                //            continue;

                //        canChange = false;
                //        break;
                //    }
                //}
                //return canChange;

                return true;
            }


            void ContentController::stopAllDipFlowsWithNonChangeableFolders()
            {
                // This code is disabled because we no longer want to prevent the user from changing download directories when flows are active.
                // Leaving this code in place in case we need this functionality for something else later

                //for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                //    it != m_productIdToEntitlementRefHash.constEnd();
                //    it++)
                //{
                //    if(!it->isNull() && (*it)->contentConfiguration()->dip())
                //    {
                //        //if it's ITO AND it's currently on the Prepare Download dialog, DON'T cancel the installflow
                //        Origin::Downloader::IContentInstallFlow *iFlow = (*it)->localContent()->installFlow();
                //        if (iFlow && iFlow->isUsingLocalSource() && iFlow->getFlowState() == Origin::Downloader::ContentInstallFlowState::kPendingInstallInfo)
                //            continue;
                //        
                //        (*it)->localContent()->installFlow()->cancel();
                //    }
                //}

            }

            void ContentController::stopAllNonDipFlows()
            {
                for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                    it != m_productIdToEntitlementRefHash.constEnd();
                    it++)
                {
                    if(!it->isNull() && !(*it)->contentConfiguration()->dip())
                    {
                        if ((*it)->localContent()->installFlow())
                            (*it)->localContent()->installFlow()->cancel();
                    }
                }
            }
            
            void ContentController::onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState state)
            {
                Origin::Downloader::IContentInstallFlow* flow = dynamic_cast<Origin::Downloader::IContentInstallFlow*>(sender());
                if(flow && m_flowsWaitingToPause.contains(flow))
                {
                    if(state == Origin::Downloader::ContentInstallFlowState::kPaused || state == Origin::Downloader::ContentInstallFlowState::kReadyToInstall || state == Origin::Downloader::ContentInstallFlowState::kReadyToStart)
                    {
                        ORIGIN_VERIFY_DISCONNECT(flow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState)));
                        ORIGIN_VERIFY_DISCONNECT(flow, SIGNAL(suspendComplete (const QString&)), this, SLOT(onInstallFlowSuspendComplete(const QString&)));
                        m_flowsWaitingToPause.removeAll(flow);
                    
                        if(m_flowsWaitingToPause.isEmpty())
                        {
                            ORIGIN_LOG_EVENT << "All active flows have been successfully paused or suspended.";
                            emit lastActiveFlowPaused(true);
                        }
                    }
                }
            }

            void ContentController::onInstallFlowSuspendComplete (const QString& productId)
            {
                EntitlementRef entRef = entitlementByProductId(productId, AllContent);
                if (!entRef.isNull())
                {
                    Origin::Downloader::IContentInstallFlow* flow = entRef->localContent()->installFlow();
                    if(flow && m_flowsWaitingToPause.contains(flow))
                    {
                        ORIGIN_VERIFY_DISCONNECT(flow, SIGNAL(suspendComplete (const QString&)), this, SLOT(onInstallFlowSuspendComplete(const QString&)));
                        ORIGIN_VERIFY_DISCONNECT(flow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState)));
                        m_flowsWaitingToPause.removeAll(flow);
                    
                        if(m_flowsWaitingToPause.isEmpty())
                        {
                            ORIGIN_LOG_EVENT << "All active flows have been successfully paused or suspended.";
                            emit lastActiveFlowPaused(true);
                        }
                    }
                }
            }

            void ContentController::suspendAllActiveFlows(bool autoresume, int timeout /*= 0*/)
            {
                for(QMultiHash<QString, EntitlementRef >::const_iterator it = m_productIdToEntitlementRefHash.constBegin();
                    it != m_productIdToEntitlementRefHash.constEnd();
                    it++)
                {
                    if (!it->isNull())
                    {
                        Downloader::IContentInstallFlow * flow = (*it)->localContent()->installFlow();
                        if (flow)
                        {
                            // Wait for downloads to pause or to be suspended
                            if(timeout > 0 )
                            {
                                // Track all pausing downloads. Logout will not complete until the flows have finished pausing to
                                // allow an orderly cleanup. This will prevent having to redownload files on resuming.
                                // The pause method will cause flows that are in a pausable state to pause. Flows that are not
                                // in a pausable state will not be able to salvage everything and can do their cleanup in the background.
                                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState)));
                                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(suspendComplete(const QString&)), this, SLOT(onInstallFlowSuspendComplete(const QString&)));
                                m_flowsWaitingToPause.append(flow);
                            }

                            // TELEMETRY: Flow is being suspended due to user logging out or exiting.
                            GetTelemetryInterface()->Metric_DL_PAUSE((*it)->contentConfiguration()->productId().toLocal8Bit().constData(), "logout_exit");
                            flow->pause(autoresume);
                        }
                    } 
                }
                
                if(timeout > 0)
                {
                    // Don't bother with the timer if we aren't waiting on anything to pause
                    if(m_flowsWaitingToPause.isEmpty())
                    {
                        ORIGIN_LOG_EVENT << "Found no downloads that could be paused.";
                        emit lastActiveFlowPaused(true);
                    }
                    else
                    {
                        ORIGIN_LOG_EVENT << "Starting Pause All timer (" << timeout << " msec).  Waiting on " << m_flowsWaitingToPause.count() << " install flows to pause.";
                        QTimer::singleShot(timeout, this, SLOT(onPauseAllTimeout()));
                    }
                }
            }
            
            void ContentController::onPauseAllTimeout()
            {
                if(m_flowsWaitingToPause.isEmpty())
                    return;

                for(QList<Origin::Downloader::IContentInstallFlow*>::const_iterator it = m_flowsWaitingToPause.constBegin();
                    it != m_flowsWaitingToPause.constEnd();
                    it++)
                {
                    if (*it != NULL)
                    {
                        ORIGIN_VERIFY_DISCONNECT(*it, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState)));
                    }
                }

                ORIGIN_LOG_WARNING << "Pause All timed out with " << m_flowsWaitingToPause.count() << " install flows still active.";
                m_flowsWaitingToPause.clear();
                emit lastActiveFlowPaused(false);
            }

            void ContentController::deleteAllContentInstallFlows()
            {
                foreach (const EntitlementRef &e, entitlements(OwnedContent))
                {
                    if (e)
                    {
                        Engine::Content::LocalContent* content = e->localContent();
                        ORIGIN_VERIFY_CONNECT_EX(content, SIGNAL(installFlowDeleted()), this, SLOT(onInstallFlowDeleted()), Qt::QueuedConnection);
                        m_contentFlowsToDelete.append(content);
                        content->deleteInstallFlow();
                    }
                }

                if(m_contentFlowsToDelete.isEmpty())
                {
                    ORIGIN_LOG_EVENT << "Found no content install flow to clean up.";
                    emit lastContentInstallFlowDeleted();
                }
            }

            void ContentController::onInstallFlowDeleted()
            {
                Engine::Content::LocalContent* content = dynamic_cast<Engine::Content::LocalContent*>(sender());
                if (content)
                {
                    ORIGIN_VERIFY_DISCONNECT(content, SIGNAL(installFlowDeleted()), this, SLOT(onInstallFlowDeleted()));
                    int idx = m_contentFlowsToDelete.indexOf(content);
                    if (idx >= 0)
                    {
                        m_contentFlowsToDelete.removeAt(idx);
                        if (m_contentFlowsToDelete.isEmpty())
                        {
                            ORIGIN_LOG_EVENT << "All content install flows deleted";
                            emit lastContentInstallFlowDeleted();
                        }
                    }
                }
            }

            void ContentController::onRefreshGamesTimedOut()
            {
                ORIGIN_LOG_ERROR << "Timeout while retrieving user entitlement list.";
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("ContentRetrievalTimeout", "");

                m_refreshGamesTimeout->stop();
                ORIGIN_VERIFY_DISCONNECT(m_refreshGamesTimeout, SIGNAL(timeout()), this, SLOT(onRefreshGamesTimedOut()));

                if(updateInProgress() == false)
                    return;

                // trigger an error message
                mIsLoadingEntitlementsSlow = true;
                emit loadingGamesTooSlow();

                return;
            }

            bool ContentController::isLoadingEntitlementsSlow()
            {
                return mIsLoadingEntitlementsSlow;
            }

            bool ContentController::downloadById(const QString& id, const QString& installLocale /*= ""*/)
            {
                EntitlementRef eRef = entitlementById(id);
                if (!eRef.isNull() && eRef->localContent()->downloadable())
                {
                    QString locale = installLocale;
                    if (locale.isEmpty() && user())
                    {
                        locale = user()->locale();
                    }

                    return eRef->localContent()->download(locale);
                }

                return false;
            }

            void ContentController::autoStartDownloadOnNextRefresh(const QString& id)
            {
                if (!id.isEmpty())
                {
                    mAutoStartOnRefreshList.insert(id);
                }
            }

            // this is like autoStartDownloadOnNextRefresh() above but it is for the case where DLC is purchased but
            // auto-update setting is OFF which prevents auto-downloading. (EBIBUGS-28701)
            void ContentController::forceDownloadPurchasedDLCOnNextRefresh(const QString& id)
            {
                if (!id.isEmpty())
                {
                    mPendingPurchasedDLCDownloadList.insert(id);
                }
            }

            bool ContentController::autoPatchingEnabled() const
            {
                if(user() == NULL)
                    return false;
                return Origin::Services::readSetting(Origin::Services::SETTING_AUTOPATCH, user()->getSession());
            }

            QString ContentController::getDisplayNameFromId(const QString &id)
            {
                EntitlementRef e = entitlementById(id);
                if (e)
                {
                    ContentConfigurationRef c = e->contentConfiguration();
                    if (c)
                    {
                        if(c->monitorPlay())
                        {
                            QString displayName(c->displayName());
                            return displayName;
                        }
                    }
                }
                //if content id isn't found, return empty string
                return "";
            }

            void ContentController::processDownloaderAutostartItems()
            {
                // Make sure the user has allowed the previous UAC popup
                if (m_autostartUACPending)
                    return;

                // Stop any auto-downloads of PDLC/patches from onExtraContentAvailable() callback
                m_autostartAllowOnExtraDownloads = false;

                // Reset the UAC flags for each refresh attempt
                // These are handled as member variables because PDLC comes in asynchronously and we don't want to re-request UAC for those if the user told us no already
                m_autostartPermissionsRequested = false;
                m_autostartUACFailed = false;
                m_autostartUACPending = false;
                
                QStringList pendingAutoPatches;
                QSet<QString> pendingAutoStartedPDLC;

                //kick off auto patching
                pendingAutoPatches = collectAutoPatchInstalledContent();

                // Only emit if empty, otherwise we will emit this in the processAutoPatchInstalledContent...
                if (pendingAutoPatches.isEmpty())
                    emit (autoUpdateCheckCompleted(!pendingAutoPatches.isEmpty(), pendingAutoPatches));

                // Check whether we have PDLCs we could load (on initial run-through, the entitlements may not have their children parsed yet, but this is handled later on)
                pendingAutoStartedPDLC = collectAutoStartedPDLC();

                // Schedule a check for broken content
//                QTimer::singleShot(BROKEN_CONTENT_DELAY, this, SLOT(onCollectBrokenContentItemsCallback()));

                if (!pendingAutoPatches.isEmpty() || !pendingAutoStartedPDLC.isEmpty())
                {
                    ORIGIN_LOG_EVENT << "Requesting UAC for autopatching/autostart";

                    m_autostartUACPending = true;

                    QString uacReasonCode = "processDownloaderAutostartItems";
                    Origin::Escalation::EscalationWorker* worker = Origin::Escalation::IEscalationClient::quickEscalateASync(uacReasonCode);
                    ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished(bool)), this, SLOT(onProcessDownloaderAutoStartItemsCallback(bool)));
                    worker->start();
                }

                else
                {
                    // Re-enable auto-downloads of PDLC/patches from onExtraContentAvailable() callback
                    m_autostartAllowOnExtraDownloads = true;

                    ORIGIN_LOG_EVENT << "No pending autopatching/autostart";
                }
            }

            void ContentController::onProcessDownloaderAutoStartItemsCallback(bool uacSuccess)
            {
                m_autostartUACFailed = !uacSuccess;
                m_autostartUACPending = false;
                m_autostartPermissionsRequested = true;

                // Kick off pending patches
                // Re-calculate pendingAutoPatches because new extracontent may have
                // come in while waiting on UAC.
                QStringList pendingAutoPatches = collectAutoPatchInstalledContent();
                if (!pendingAutoPatches.isEmpty())
                {
                    processAutoPatchInstalledContent(pendingAutoPatches, m_autostartUACFailed);
                }

                // Kick off pending PDLC downloads
                // Don't need to collect PDLC here because downloadAllPDLC will do it for us.
                QSet<QString> pendingAutoStartedPDLC;
                processAutoStartedPDLC(pendingAutoStartedPDLC, m_autostartUACFailed);

                // Re-enable auto-downloads of PDLC/patches from onExtraContentAvailable() callback
                m_autostartAllowOnExtraDownloads = true;

                if (m_autostartUACFailed)
                {
                    ORIGIN_LOG_WARNING << "Aborted processing of downloader autostart items.  UAC failed or was denied.";

                    emit autoDownloadUACFailed();
                    return;
                }

            }

            void ContentController::onCollectBrokenContentItemsCallback()
            {   
                return;

                bool deferCheck = false;

                // If ITO is running we want to avoid throwing up the auto-repair dialogs
                if (InstallFlowManager::Exists())
                {
                    QString contentIdsITO = InstallFlowManager::GetInstance().GetVariable("ContentIds").toString();
                    if (!contentIdsITO.isEmpty())
                    {
                        // ITO is probably running
                        deferCheck = true;

                        ORIGIN_LOG_EVENT << "[AUTO-REPAIR] ITO active.  Deferring broken content check.";
                    }
                }

                // For now, we'll let any game being played stand-in for RTP
                if (isPlaying())
                {
                    deferCheck = true;

                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Game playing.  Deferring broken content check.";
                }

                if (deferCheck)
                {
                    // Schedule a check for broken content
                    QTimer::singleShot(BROKEN_CONTENT_DELAY_ITO_RTP, this, SLOT(onCollectBrokenContentItemsCallback()));

                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Deferring broken content check due to ITO or RTP.  Delay time: " << (BROKEN_CONTENT_DELAY_ITO_RTP/(1000*60)) << " minutes.";

                    return;
                }

                ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Running broken content check.";

                // Collect the content which is broken and may require repair
                QStringList pendingAutoRepairs = collectBrokenInstalledContent(true);

                if (!pendingAutoRepairs.isEmpty())
                {
                    // Ask the user for permission first
                    emit brokenContentDetected();
                    return;
                }
            }

            void ContentController::processBrokenContentItems()
            {
                return;

                //  TELEMETRY: User has accepted the broken content fixing task
                GetTelemetryInterface()->Metric_AUTOREPAIR_TASK_ACCEPTED();

                if (!m_autostartPermissionsRequested)
                {
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Requesting UAC for autorepair";

                    m_autostartUACPending = true;

                    QString uacReasonCode = "processBrokenContentItems";
                    Origin::Escalation::EscalationWorker* worker = Origin::Escalation::IEscalationClient::quickEscalateASync(uacReasonCode);
                    ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished(bool)), this, SLOT(onProcessBrokenContentItemsCallback(bool)));
                    worker->start();
                }
                else
                {
                    // We already requested UAC, no need to do it again
                    onProcessBrokenContentItemsCallback(true);
                }
            }

            void ContentController::processBrokenContentItemsRejected()
            {
                ORIGIN_LOG_EVENT << "[AUTO-REPAIR] User declined auto-repairs.";

                //  TELEMETRY: User has declined the broken content fixing task
                GetTelemetryInterface()->Metric_AUTOREPAIR_TASK_DECLINED();
            }

            void ContentController::onProcessBrokenContentItemsCallback(bool uacSuccess)
            {
                ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Processing auto-repairs";

                m_autostartUACFailed = !uacSuccess;
                m_autostartUACPending = false;
                m_autostartPermissionsRequested = true;

                // Collect the content which is broken and may require repair
                QStringList pendingAutoRepairs = collectBrokenInstalledContent(false);
                if (!pendingAutoRepairs.isEmpty())
                {
                    processBrokenInstalledContent(pendingAutoRepairs, m_autostartUACFailed);
                }
            }

            void ContentController::onPlayStarted(Origin::Engine::Content::EntitlementRef e)
            {
                // Do not have the Origin SDK Test App control the download utilization on startup.
                if(e->contentConfiguration()->contentId() != "71314")
                {
                    // Reduce the download utilization during game start.
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultGameStartingDownloadUtilization());

                    // Make sure we resume at some point in the future.
                    m_downloadUtilizationRestoreTimer.start();
                }
            }

            void ContentController::onPlayFinished(Origin::Engine::Content::EntitlementRef e)
            {
                m_downloadUtilizationRestoreTimer.stop();

                if(isPlaying())
                {
                    // We are still playing set normal gameplay download utilization.
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultPlayingDownloadUtilization());
                }
                else
                {
                    // We are no longer playing, go full throttle.
                    Downloader::DownloadService::SetGameRequestedDownloadUtilization(Downloader::DownloadService::GetDefaultGameRequestedDownloadUtilization());
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultDownloadUtilization());
                }
            }


            void ContentController::onRestoreDownloadUtilizationTimout()
            {
                if(isPlaying())
                {
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultPlayingDownloadUtilization());
                }
                else
                {
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultDownloadUtilization());
            }
            }


            //  Enumerates through all the content in the set to check if a newer version
            //  exists on the server.
            QStringList ContentController::collectAutoPatchInstalledContent()
            {
                TIME_BEGIN("ContentController::collectAutoPatchInstalledContent")

                QStringList autoUpdatingList;

                //  Check if auto-patching in enabled (global, i.e. settings)
                ORIGIN_LOG_EVENT << "Getting auto_patch flag = " << (autoPatchingEnabled() ? "true" : "false");

                // We still have to iterate right now to check for a forced repair.  If we remove force repair code below we can once again
                // short circuit here if autoPatch is false
                /*
                if (autoPatch == false)
                {
                    emit (autoUpdateCheckCompleted(false, autoUpdatingList));
                    return;
                }
                */
                foreach (const EntitlementRef e, entitlements(AllContent))
                {
                    bool startDownload = false;
                    bool permissionsRequested = true;
                    bool uacFailed = false;
                    autoPatchInstalledContent(startDownload, permissionsRequested, uacFailed, e, autoUpdatingList);
                }

                TIME_END("ContentController::collectAutoPatchAllInstalledContent")

                return autoUpdatingList;
            }

            void ContentController::processAutoPatchInstalledContent(const QStringList& pendingDownloads, bool uacFailed)
            {
                TIME_BEGIN("ContentController::processAutoPatchInstalledContent")

                QStringList autoUpdatingList;
                foreach (const EntitlementRef e, entitlements(AllContent))
                {
                    QString productId = e->contentConfiguration()->productId();
                    if (pendingDownloads.contains(productId))
                    {
                        bool startDownload = true;
                        bool permissionsRequested = true;
                        autoPatchInstalledContent(startDownload, permissionsRequested, uacFailed, e, autoUpdatingList);
                    }
                }

                emit (autoUpdateCheckCompleted(!autoUpdatingList.isEmpty(), autoUpdatingList));
                TIME_END("ContentController::processAutoPatchInstalledContent")
            }

            QSet<QString> ContentController::collectAutoStartedPDLC()
            {
                QSet<QString> pendingDownloads;
                foreach (const EntitlementRef e, baseGamesController()->baseGames())
                {
                    // Only try for entitlements that have children, as well as being installed
                    const bool addonsAvailable = e->contentConfiguration()->addonsAvailable();
                    const bool installed = e->localContent() && (e->localContent()->installed() || e->localContent()->installInProgressQueued());

                    if (addonsAvailable && installed)
                    {
                        e->localContent()->collectAllPendingPDLC(pendingDownloads);
                    }
                }

                return pendingDownloads;
            }

            void ContentController::processAutoStartedPDLC(QSet<QString>& pendingDownloads, bool uacFailed)
            {
                // Attempt to kick off PDLC (on initial run-through, the entitlements may not have their children parsed yet, but this is handled later on)
                bool permissionsRequested = true;
                foreach (const EntitlementRef e, baseGamesController()->baseGames())
                {
                    const bool addonsAvailable = e->contentConfiguration()->addonsAvailable();
                    const bool installed = e->localContent() && (e->localContent()->installed() || e->localContent()->installInProgressQueued());

                    if (addonsAvailable && installed)
                    {
                        e->localContent()->downloadAllPDLC(pendingDownloads, permissionsRequested, uacFailed);
                    }
                }
            }

            QStringList ContentController::collectBrokenInstalledContent(bool sendTelemetry)
            {
                TIME_BEGIN("ContentController::collectBrokenInstalledContent")

                QStringList autoRepairList;

                foreach (const EntitlementRef e, entitlements(AllContent))
                {
                    bool startRepair = false;
                    bool permissionsRequested = true;
                    bool uacFailed = false;
                    autoRepairInstalledContent(startRepair, permissionsRequested, uacFailed, e, autoRepairList, sendTelemetry);
                }

                TIME_END("ContentController::collectBrokenInstalledContent")

                return autoRepairList;
            }

            void ContentController::processBrokenInstalledContent(const QStringList& pendingBrokenContent, bool uacFailed)
            {
                TIME_BEGIN("ContentController::processBrokenInstalledContent")

                QStringList autoRepairList;
                foreach (const EntitlementRef e, entitlements(AllContent))
                {
                    QString productId = e->contentConfiguration()->productId();
                    if (pendingBrokenContent.contains(productId))
                    {
                        bool startRepair = true;
                        bool permissionsRequested = true;
                        autoRepairInstalledContent(startRepair, permissionsRequested, uacFailed, e, autoRepairList, false);
                    }
                }

                // TODO: FIX
                emit (autoUpdateCheckCompleted(!autoRepairList.isEmpty(), autoRepairList));
                TIME_END("ContentController::processBrokenInstalledContent")
            }

            bool ContentController::autoPatchInstalledContent(bool startDownload, bool &permissionsRequested, bool &uacFailed, EntitlementRef entitlement, QStringList& autoUpdatingList)
            {
                bool retVal = false;

                //  Skip version check if content is not installed on machine
                bool contentInstalled = entitlement->localContent()->installed();
                if (contentInstalled == false)
                    return retVal;

                // Skip if not owned and not preview content
                if (!entitlement->contentConfiguration()->owned() && !entitlement->contentConfiguration()->previewContent())
                {
                    return retVal;
                }

                // Skip if this is a non-origin game
                if (entitlement->contentConfiguration()->isNonOriginGame())
                {
                    ORIGIN_LOG_EVENT << "(" << entitlement->contentConfiguration()->productId() << ") Skip update, non-origin game";
                    return retVal;
                }
                    
                //check to make sure that installFlow() exists (EBIBUGS-17602)
                if (!entitlement->localContent()->installFlow())
                {
                    ORIGIN_LOG_ERROR << "install flow NULL for (" << entitlement->contentConfiguration()->productId() << ") package type= [" << entitlement->contentConfiguration()->packageType() << "]";
                    return retVal;
                }

                //  Skip version check if this content has a download job in progress
                if (entitlement->localContent()->installFlow()->isActive())
                {
                    ORIGIN_LOG_EVENT << "(" << entitlement->contentConfiguration()->productId() << ") Skip update, install flow already active";
                    return retVal;
                }

                // If the user does not have "keep my games up to date" then we bail here.
                if(autoPatchingEnabled() == false)
                    return retVal;

                bool autopatchfromManifest;
                autopatchfromManifest = entitlement->localContent()->updatable();
                if (!autopatchfromManifest)
                {
                    ORIGIN_LOG_EVENT << "(" << entitlement->contentConfiguration()->productId()<< ") Skip update, not updatable";
                    ORIGIN_LOG_EVENT << "Game Title: " << entitlement->contentConfiguration()->displayName();
                    return retVal;
                }

                // ORPUB-1378: Skip patching if this entitlement has a shared network override with a pending download override.  This prevents a title from
                // immediately "updating" to the cdn build when a normal download override is replaced by a SNO, and we are awaiting confirmation from the user
                // to set a new download override.
                const QString pendingSharedNetworkOverride = m_sharedNetworkOverrideManager->pendingOverrideForOfferId(entitlement->contentConfiguration()->productId());
                if (!pendingSharedNetworkOverride.isEmpty())
                {
                    return retVal;
                }

                //  Check if the server version is different than installed version.
                //  This check allows us to downgrade the installation if necessary.
                if(entitlement->localContent()->updateAvailable())
                {
                    if (uacFailed)
                    {
                        ORIGIN_LOG_WARNING << "Skipping auto-update of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to UAC failure.";
                        return retVal;
                    }

                    // If the user hasn't already been asked for permissions, we'll go ahead and ask before we begin
                    if (!permissionsRequested)
                    {
                        // We don't do auto-update processing for local overrides that don't have a version specified via "OverrideVersion" or the DiP manifest version
                        // See EBICHANGE-1388
                        if(!entitlement->localContent()->contentConfiguration()->hasValidVersion())
                        {
                            ORIGIN_LOG_WARNING << "Skipping auto-update of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to no explicit version specified via override or dip manifest.";
                            return retVal;
                        }

                        QString uacReasonCode = "autoPatchInstalledContent (" + entitlement->contentConfiguration()->productId() + ")";

                        // If we can't get escalation for some reason, we bail out of the entire loop
                        if (!Origin::Escalation::IEscalationClient::quickEscalate(uacReasonCode))
                        {
                            ORIGIN_LOG_WARNING << "Skipping auto-update of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to UAC failure.";
                            uacFailed = true;
                            return retVal;
                        }
                    }

                    retVal = true;
                    if (startDownload)
                    {
                        //  Request download update job
                        if(entitlement->localContent()->update(true /*silent, i.e. suppress dialogs*/))
                        {
                            //  Log:  Record installed version and version we are upgrading to.
                            ORIGIN_LOG_EVENT << "(" << entitlement->contentConfiguration()->productId()<< ") Start update, Installed version=(" << entitlement->localContent()->installedVersion() << "), Server version=(" << entitlement->contentConfiguration()->version() << "), Installcheck=(" << entitlement->contentConfiguration()->installCheck() << "), path=(" << Origin::Services::PlatformService::localFilePathFromOSPath(entitlement->contentConfiguration()->installCheck()) << ")";

                            //  TELEMETRY: Version mismatch detected, automatically patch content
                            GetTelemetryInterface()->Metric_AUTOPATCH_DOWNLOAD_START(
                                entitlement->contentConfiguration()->productId().toUtf8().constData(),
                                entitlement->localContent()->installedVersion().toUtf8().constData(),
                                entitlement->contentConfiguration()->version().toUtf8().constData());

                            // Go ahead and add the folder path to the set and emit activeFlowsChanged to avoid
                            // race condition on onFlowActiveStateCheck getting triggered before this loop comes 
                            // through to another entitlement that shares the install folder
                            QString installFolder = entitlement->localContent()->dipInstallPath();
                            installFolder.replace("\\", "/");
                            if (!installFolder.endsWith("/"))
                            {
                                installFolder.append("/");
                            }

                            if (!m_installFolderToEntitlementRefHash.contains(installFolder, entitlement))
                            {
                                m_installFolderToEntitlementRefHash.insert(installFolder, entitlement);
                            }

                            emit(activeFlowsChanged(entitlement->contentConfiguration()->productId(), true));

                            autoUpdatingList.append (entitlement->contentConfiguration()->productId());
                        }
                        else
                        {
                            //  Log:  Record installed version and version we are upgrading to.
                            ORIGIN_LOG_EVENT << "(" << entitlement->contentConfiguration()->productId()<< ") Could not start update, Installed version=(" << entitlement->localContent()->installedVersion() << "), Server version=(" << entitlement->contentConfiguration()->version() << ")";
                        }
                    }

                    else
                    {
                        autoUpdatingList.append (entitlement->contentConfiguration()->productId());
                    }
                }

                return retVal;
            }

            bool ContentController::autoRepairInstalledContent(bool startRepair, bool &permissionsRequested, bool &uacFailed, EntitlementRef entitlement, QStringList& autoRepairList, bool sendNeedRepairTelemetry)
            {
                bool retVal = false;

                //  Skip version check if content is not installed on machine
                bool contentInstalled = entitlement->localContent()->installed();
                if (contentInstalled == false)
                    return retVal;

                // We only bother with titles that need repair
                if (!entitlement->localContent()->needsRepair())
                    return retVal;

                // Skip if not owned and not preview content
                if (!entitlement->contentConfiguration()->owned() && !entitlement->contentConfiguration()->previewContent())
                {
                    return retVal;
                }

                // Skip if this is a non-origin game
                if (entitlement->contentConfiguration()->isNonOriginGame())
                {
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] (" << entitlement->contentConfiguration()->productId() << ") Skip repair, non-origin game";
                    return retVal;
                }
                    
                //check to make sure that installFlow() exists (EBIBUGS-17602)
                if (!entitlement->localContent()->installFlow())
                {
                    ORIGIN_LOG_ERROR << "[AUTO-REPAIR] install flow NULL for (" << entitlement->contentConfiguration()->productId() << ") package type= [" << entitlement->contentConfiguration()->packageType() << "]";
                    return retVal;
                }

                //  Skip version check if this content has a download job in progress
                if (entitlement->localContent()->installFlow()->isActive())
                {
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] (" << entitlement->contentConfiguration()->productId() << ") Skip repair, install flow already active";
                    return retVal;
                }

                if (!entitlement->localContent()->repairable())
                {
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] (" << entitlement->contentConfiguration()->productId()<< ") Skip repair, not repairable";
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Game Title: " << entitlement->contentConfiguration()->displayName();
                    return retVal;
                }

                if (uacFailed)
                {
                    ORIGIN_LOG_WARNING << "[AUTO-REPAIR] Skipping auto-repair of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to UAC failure.";
                    return retVal;
                }

                // We don't do auto-update processing for local overrides that don't have a version specified via "OverrideVersion" or the DiP manifest version
                // See EBICHANGE-1388
                if(!entitlement->localContent()->contentConfiguration()->hasValidVersion())
                {
                    ORIGIN_LOG_WARNING << "[AUTO-REPAIR] Skipping auto-repair of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to no explicit version specified via override or dip manifest.";
                    return retVal;
                }

                // If the user hasn't already been asked for permissions, we'll go ahead and ask before we begin
                if (!permissionsRequested)
                {
                    QString uacReasonCode = "autoRepairInstalledContent (" + entitlement->contentConfiguration()->productId() + ")";

                    // If we can't get escalation for some reason, we bail out of the entire loop
                    if (!Origin::Escalation::IEscalationClient::quickEscalate(uacReasonCode))
                    {
                        ORIGIN_LOG_WARNING << "[AUTO-REPAIR] Skipping auto-repair of (" << entitlement->contentConfiguration()->productId()<< ") " << entitlement->contentConfiguration()->displayName() << " due to UAC failure.";
                        uacFailed = true;
                        return retVal;
                    }
                }

                retVal = true;
                if (startRepair)
                {
                    //  Request repair job
                    if(entitlement->localContent()->repair(true /*silent, i.e. suppress dialogs*/))
                    {
                        //  Log:  Record installed version and version we are upgrading to.
                        ORIGIN_LOG_EVENT << "[AUTO-REPAIR] (" << entitlement->contentConfiguration()->productId()<< ") Start auto-repair, Installed version=(" << entitlement->localContent()->installedVersion() << "), Server version=(" << entitlement->contentConfiguration()->version() << "), Installcheck=(" << entitlement->contentConfiguration()->installCheck() << "), path=(" << Origin::Services::PlatformService::localFilePathFromOSPath(entitlement->contentConfiguration()->installCheck()) << ")";

                        //  TELEMETRY: Automatic repair start
                        GetTelemetryInterface()->Metric_AUTOREPAIR_DOWNLOAD_START(
                            entitlement->contentConfiguration()->productId().toUtf8().constData(),
                            entitlement->localContent()->installedVersion().toUtf8().constData(),
                            entitlement->contentConfiguration()->version().toUtf8().constData());

                        // Go ahead and add the folder path to the set and emit activeFlowsChanged to avoid
                        // race condition on onFlowActiveStateCheck getting triggered before this loop comes 
                        // through to another entitlement that shares the install folder
                        QString installFolder = entitlement->localContent()->dipInstallPath();
                        installFolder.replace("\\", "/");
                        if (!installFolder.endsWith("/"))
                        {
                            installFolder.append("/");
                        }

                        if (!m_installFolderToEntitlementRefHash.contains(installFolder, entitlement))
                        {
                            m_installFolderToEntitlementRefHash.insert(installFolder, entitlement);
                        }

                        emit(activeFlowsChanged(entitlement->contentConfiguration()->productId(), true));

                        autoRepairList.append (entitlement->contentConfiguration()->productId());
                    }
                    else
                    {
                        //  Log:  Record installed version and version we are upgrading to.
                        ORIGIN_LOG_EVENT << "[AUTO-REPAIR] (" << entitlement->contentConfiguration()->productId()<< ") Could not start auto-repair, Installed version=(" << entitlement->localContent()->installedVersion() << "), Server version=(" << entitlement->contentConfiguration()->version() << ")";
                    }
                }
                else
                {
                    ORIGIN_LOG_EVENT << "[AUTO-REPAIR] Content needs repair: " << entitlement->contentConfiguration()->productId() << ".";

                    if (sendNeedRepairTelemetry)
                    {
                        //  TELEMETRY: Content needs repair
                        GetTelemetryInterface()->Metric_AUTOREPAIR_NEEDS_REPAIR(
                            entitlement->contentConfiguration()->productId().toUtf8().constData(),
                            entitlement->localContent()->installedVersion().toUtf8().constData(),
                            entitlement->contentConfiguration()->version().toUtf8().constData());
                    }
                    autoRepairList.append (entitlement->contentConfiguration()->productId());
                }

                return retVal;
            }

            void ContentController::deleteAllInstallerCacheFiles()
            {
                TIME_BEGIN("ContentController::deleteAllInstallerCacheFiles")

                foreach (const EntitlementRef ent, entitlements(OwnedContent))
                {
                    // No installer cache to delete for DiPs, plus deleteTempInstallerFiles cancels active install flows
                    // but this user operation is allowed as long as there are no non-dip install flows active.
                    if (!ent->contentConfiguration()->dip())
                    {
                        ent->localContent()->deleteTempInstallerFiles();
                    }
                }
                TIME_END("ContentController::deleteAllInstallerCacheFiles")
            }

            bool ContentController::initialEntitlementRefreshCompleted() const
            {
                //we use this for ITO, must complete before we can kick off ITO
                QMutexLocker locker(&m_entitlementInitialCheckLock);
                return m_InitialEntitlementRefreshed;
            }

            bool ContentController::hasExtraContent()
            {
                foreach (const EntitlementRef e, entitlements(OwnedContent))
                {
                    if (e->contentConfiguration()->addonsAvailable())
                        return true;
                }

                return false;
            }

            bool ContentController::updateInProgress() const
            {
                return m_refreshInProgress;
            }

            bool ContentController::isPlaying() const
            {
                foreach(EntitlementRef e, entitlements(OwnedContent))
                {
                    if((e->localContent()->state() & Origin::Engine::Content::LocalContent::Playing) == Origin::Engine::Content::LocalContent::Playing)
                    {
                        return true;
                    }
                }
                return false;
            }

            void ContentController::prepareIGO(const Origin::Engine::Content::ContentConfigurationRef config)
            {
                // Don't have the test case in use for the old fix, so kept the behavior the way it was, even if it's quite ugly
                static bool oldBadHackSetup = true;

                if(config == Origin::Engine::Content::ContentConfiguration::INVALID_CONTENT_CONFIGURATION)  // we have an external app(RTP) that uses the SDK, but is not installed, so always enable the IGO!
                {                   // EBIDEVSUP-1147
                    if (oldBadHackSetup)
                        IGOController::instance()->setIGOEnable(true);
                    
                    oldBadHackSetup = false;
                    return;
                }

                oldBadHackSetup = false;
                bool bEnabled = config->isIGOEnabled();
                
                // We need to notify the game itself through IGOWindowManager.
                // This is to help with the SDK games which always load igo32.dll -> we check that flag when setting up OIG in the game itself.
                IGOController::instance()->setIGOEnable(bEnabled);
            }


            // Set externally started content not wrapped in RtP to a playing state
            void ContentController::sdkConnectionEstablished(const QString &productId, const QString& name)
            {
                ASYNC_INVOKE_GUARD_ARGS(Q_ARG(const QString &, productId), Q_ARG(const QString &, name));
                

                ORIGIN_LOG_EVENT << "[Content Controller] Connection established: " << productId;

                EntitlementRef entRef = entitlementByProductId(productId);

                if (entRef.isNull())
                {
                    entRef = entitlementByContentId(productId);
                }

#if defined(ORIGIN_PC)
                uint32_t processID = 0;

                if (!entRef.isNull())
                {
                    QString exePath = entRef->localContent()->executablePath();
                    processID = Origin::Services::ProcessList::getPIDFromSocket(exePath);
                }

                // send IGO a notification that it was loaded by the SDK in order to attach it's hooks, if that has not been done yet
                IGOController::instance()->signalIGO(processID); 
#endif
                // Ignore the SDK Test App
                if (productId != "71314" || productId != "OFB-EAST:45210")
                {
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultPlayingDownloadUtilization());

                    m_downloadUtilizationRestoreTimer.stop();
                }

                if (!entRef.isNull())
                {
                    emit sdkConnected(entRef);

                    // If the content is not already in a playing state, set it as playing.
                    if (!entRef->localContent()->playing())
                    {
                        prepareIGO(entRef->localContent()->contentConfiguration());
                            
                        entRef->localContent()->playNonRtp();
                        return;
                    }
                }
                prepareIGO(Origin::Engine::Content::ContentConfiguration::INVALID_CONTENT_CONFIGURATION);
            }

            // Set content not wrapped in RtP to a ready to play state after shut down externally.
            void ContentController::sdkConnectionLost(const QString &productId)
            {
                ASYNC_INVOKE_GUARD_ARGS(Q_ARG(const QString &, productId));

                ORIGIN_LOG_EVENT << "[Content Controller] Connection lost: " << productId;

                if (!isPlaying())
                {
                    // Restore the game-specified download setting
                    Downloader::DownloadService::SetGameRequestedDownloadUtilization(Downloader::DownloadService::GetDefaultGameRequestedDownloadUtilization());

                    // Restore the client download setting
                    Downloader::DownloadService::SetDownloadUtilization(Downloader::DownloadService::GetDefaultDownloadUtilization());
                }

                EntitlementRef entRef = entitlementByProductId(productId); 

                if(entRef.isNull())
                {
                    // In case the game had a hard coded contentId, or was started outside of Origin.
                    entRef = entitlementByContentId(productId); 
                }

                if (!entRef.isNull())
                {
                    entRef->localContent()->stopNonRtpPlay();
                }
            }

            void ContentController::onConnectionStateChanged(bool onlineAuth)
            {
                if(onlineAuth)
                {
                    // EBIBUGS-27009: Need to perform a full refresh when coming back online because the
                    // incremental refresh doesn't include catalog updates
                    refreshUserGameLibrary(RenewAll);
                }
            }

            void ContentController::onUserLoggedOut (Origin::Engine::UserRef user)
            {
                if (user.data() && user.data() == m_User.data())
                {
                    //disconnect all the signals just to be safe
                    ORIGIN_VERIFY_DISCONNECT (this, NULL, NULL, NULL);
                    ORIGIN_LOG_EVENT << "User logged out, disconnecting all signals";
                }
            }

            void ContentController::onConnectedToSDK(QString productId)
            {
                mEntitlementsConnectedToSDK.append(productId);
            }

            void ContentController::onHungupFomSDK(const QString& productId)
            {

                QStringList::iterator it = mEntitlementsConnectedToSDK.begin();
                for (; it != mEntitlementsConnectedToSDK.end(); ++it)
                {
                    if ((*it) == productId)
                    {
                        break;
                    }
                }

                if (it != mEntitlementsConnectedToSDK.end())
                {
                    // remove QString contentId from List
                    mEntitlementsConnectedToSDK.erase(it);
                }
            }

            void ContentController::purgeLicenses(const EntitlementRefList &entitlementList)
            {
#ifdef ORIGIN_PC
                //on PC, the licenses are all stored in the same folder: typically, C:\ProgramData\Electronic Arts\EA Services\License.
                //on PC, commonAppDataPath() returns, for example, C:\ProgramData\Origin.
                QDir licensesDir(Services::PlatformService::commonAppDataPath() + "\\..\\Electronic Arts\\EA Services\\License");
                if (licensesDir.exists())
                {
                    //delete all the license files in the license directory
                    QStringList licenseFiles(licensesDir.entryList(QStringList("*.dlf"), QDir::Files));
                    foreach(QString license, licenseFiles)
                    {
                        if (!licensesDir.remove(license))
                        {
                            ORIGIN_LOG_ERROR << "Unable to delete Access license: " << licensesDir.absoluteFilePath(license);
                        }
                    }
                    //delete the license directory
                    if (!licensesDir.rmdir(licensesDir.absolutePath()))
                    {
                        ORIGIN_LOG_ERROR << "Unable to delete Access license folder: " << licensesDir.absolutePath();
                    }
                }
#elif defined(ORIGIN_MAC)
                //Mac Access licenses are located in the game bundles.  Escalation may be required to delete them.
                QString escalationReasonStr = "purgeLicenses";
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                    return;
                
                foreach (EntitlementRef ent, entitlementList)
                {
                    QString executePath = ent->contentConfiguration()->executePath();
                    if (!executePath.isEmpty())
                    {
                        QString bundlePath;
                        //The execute path may need to be translated by the OS
                        if (executePath.startsWith("["))
                        {
                            bundlePath = Services::PlatformService::localDirectoryFromOSPath(executePath);
                        }
                        else
                        {
                            bundlePath = executePath;
                        }
                        if (!bundlePath.isEmpty())
                        {
                            //Before calling the escalation service to delete the license, ensure that the license folder
                            //exists in the bundle, and that it contains at least one license file
                            QDir licenseDir(bundlePath + "/__Installer/OOA/License");
                            if (licenseDir.exists())
                            {
                                QStringList fileList(licenseDir.entryList(QStringList("*.dlf"), QDir::Files));
                                if (!fileList.isEmpty())
                                {
                                    escalationReasonStr = "purgeLicenses (" + ent->contentConfiguration()->displayName() + ")";

                                    if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "deleteBundleLicense", escalationClient->deleteBundleLicense(bundlePath), escalationClient->getSystemError()))
                                    {
                                        ORIGIN_LOG_ERROR << "Unable to delete bundle license for: " << ent->contentConfiguration()->displayName();
                                    }
                                }
                            }
                        }
                    }
                }
#else
#error "Requires platform implementation"
#endif
            }

            void ContentController::doDownloadPreflighting(Origin::Engine::Content::LocalContent const& content, Origin::Engine::Content::ContentController::DownloadPreflightingResult& result)
            {
                // Forward the preflighting request to any interested parties.
                emit preflightDownload(content, result);
            }
        } //namespace Content
    } //namespace Engine
}//namespace Origin

