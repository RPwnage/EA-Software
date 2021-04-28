//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include "engine/content/BaseGamesController.h"
#include "engine/login/LoginController.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"
#include "services/downloader/Common.h"
#include "services/config/OriginConfigService.h"
#include "services/platform/TrustedClock.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

QThread BaseGamesController::sm_workerThread;

BaseGamesController::BaseGamesController(ContentController *parent)
    : m_baseGamesInitialized(false)
    , m_extraContentInitialized(false)
    , m_entitlementControllerInitialized(false)
    , m_pendingEntitlementRefresh(false)
    , m_queryOfferUpdatedDatesTimer(this)
    , m_dirtyBitsRefreshTimer(this)
    , m_dirtyBitsRefreshMode(CacheOnly)
    , m_failedEntitlementRetryTimer(this)
    , m_state(kInitializing)
{
    sm_workerThread.setObjectName("BaseGamesController Thread");
    sm_workerThread.start();

    moveToThread(&sm_workerThread);

    ORIGIN_VERIFY_CONNECT_EX(LoginController::instance(), &LoginController::userAboutToLogout,
        this, &BaseGamesController::onUserAboutToLogout, Qt::BlockingQueuedConnection);

    setNucleusEntitlementController(parent->nucleusEntitlementController());
    ORIGIN_VERIFY_CONNECT_EX(nucleusEntitlementController(), &NucleusEntitlementController::initialized,
        this, &BaseGamesController::onEntitlementsInitialized, Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(nucleusEntitlementController(), &NucleusEntitlementController::entitlementBestMatchChanged,
        this, &BaseGamesController::onEntitlementBestMatchChanged, Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(nucleusEntitlementController(), &NucleusEntitlementController::entitlementsUpdated,
        this, &BaseGamesController::onEntitlementsUpdated, Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(nucleusEntitlementController(), &NucleusEntitlementController::refreshComplete,
        this, &BaseGamesController::onEntitlementRefreshComplete, Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(nucleusEntitlementController(), &NucleusEntitlementController::extraContentRefreshComplete,
        this, &BaseGamesController::onExtraContentRefreshComplete, Qt::QueuedConnection);

    m_failedEntitlementRetryTimer.setInterval(Services::OriginConfigService::instance()->ecommerceConfig().catalogDefinitionLookupRetryInterval);
    m_failedEntitlementRetryTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_failedEntitlementRetryTimer, &QTimer::timeout,
        this, &BaseGamesController::retryFailedEntitlementRequests);

    m_dirtyBitsRefreshTimer.setInterval(Services::OriginConfigService::instance()->ecommerceConfig().dirtyBitsEntitlementRefreshTimeout);
    m_dirtyBitsRefreshTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_dirtyBitsRefreshTimer, &QTimer::timeout,
        this, &BaseGamesController::onDirtyBitsRefreshTimer);

    m_queryOfferUpdatedDatesTimer.setInterval(OfferUpdatedDateTimerDelay);
    m_queryOfferUpdatedDatesTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_queryOfferUpdatedDatesTimer, &QTimer::timeout,
        this, &BaseGamesController::queryOfferUpdatedDates);
}

BaseGamesController::~BaseGamesController()
{
}

int BaseGamesController::numberOfGames() const
{
    QReadLocker lock(&m_baseGamesLock);

    return m_baseGames.size();
}

void BaseGamesController::refresh(ContentRefreshMode mode)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Engine::Content::ContentRefreshMode, mode));

    if (m_state == kShuttingDown)
    {
        ORIGIN_LOG_EVENT << "Ignoring refresh request due to shut down";
        return;
    }

    if (ContentUpdates == mode && !nucleusEntitlementController()->lastResponseTime().isValid())
    {
        mode = RenewAll;
    }

    //reload the product override settings from eacore.ini, so user doesn't have to restart when modifying EACore.ini
    if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
    {
        Services::SettingsManager::instance()->reloadProductOverrideSettings();
    }

    switch(mode)
    {
    case UserRefresh:
        {
            qint64 throttleInterval = Origin::Services::OriginConfigService::instance()->ecommerceConfig().fullEntitlementRefreshThrottle;
            if (m_lastFullRefresh.isValid() && throttleInterval > 0)
            {
                // If user is spamming "Reload My Games" and we don't want to overload the server, we have a configurable cooldown that doesn't actually make the server call - it just silently
                // ignores the request, presents a spinner and shows the same content.  This logic was moved here from ContentController because we only care about throttling the user's EC2
                // requests--they can hit the CDN as much as they want.
                qint64 msSinceLastFullRefresh = m_lastFullRefresh.msecsTo(Origin::Services::TrustedClock::instance()->nowUTC());
                if (msSinceLastFullRefresh < throttleInterval)
                {
                    ORIGIN_LOG_DEBUG << "User is spamming reload my games, we are silenty ignoring the request because it is within cooldown threshold (" << msSinceLastFullRefresh << " < " << throttleInterval << ")";
                    GetTelemetryInterface()->Metric_ENTITLEMENT_REFRESH_REQUEST_REFUSED((msSinceLastFullRefresh/1000), 1);
                    emit refreshComplete();
                    return;
                }
            }
        }
        // Intentional fall through

    case RenewAll:
        {
            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::ContentRetrieval);
            m_state = kRefreshing;

            nucleusEntitlementController()->refreshUserEntitlements(NucleusEntitlementController::BaseGamesOnly);
            m_pendingEntitlementRefresh = true;

            foreach (const QString& masterTitleId, m_masterTitlesWithExtraContent)
            {
                nucleusEntitlementController()->refreshExtraContentEntitlements(masterTitleId);
                m_pendingExtraContentRefreshes.insert(masterTitleId);
            }

            retryFailedEntitlementRequests();
            break;
        }

    case ContentUpdates:
        {
            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::ContentRetrieval);
            m_state = kRefreshing;

            nucleusEntitlementController()->refreshUserEntitlements(NucleusEntitlementController::ContentUpdates);
            m_pendingEntitlementRefresh = true;
            break;
        }

    case CacheOnly:
        {
            nucleusEntitlementController()->refreshUserEntitlements(NucleusEntitlementController::CacheOnly);
            m_state = kIdle;

            // Reading from cache is a synchronous operation so emit once finished.
            emit refreshComplete();
            break;
        }

    default:
        break;
    }
}

bool BaseGamesController::refreshInProgress() const
{
    return (m_state == kInitializing || m_state == kRefreshing);
}

void BaseGamesController::onEntitlementsInitialized()
{
    ASYNC_INVOKE_GUARD;

    ORIGIN_LOG_EVENT << "NucleusEntitlementController is initialized";

    m_entitlementControllerInitialized = true;
}

void BaseGamesController::retrieveOwnedCatalogDefinition(Origin::Services::Publishing::NucleusEntitlementRef ent)
{
    const QString &productId = ent->productId();

#ifdef _DEBUG
    if (CatalogDefinitionController::ignoreOfferViaIsolateProductIds(productId))
    {
        ORIGIN_LOG_DEBUG << "****** Ignoring entitlement for [" << productId << "] due to IsolateProductIds setting.";
        m_initialBaseGameIds.remove(productId);
        checkInitializationComplete();
        return;
    }
#endif

    if (!m_baseGamesInitialized && !m_initialBaseGameIds.contains(productId))
    {
        m_deferredExtraContentLookups[productId] = ent;
    }
    else
    {
        qint64 batchTime = Services::Publishing::INVALID_BATCH_TIME;
        if (ent->updatedDate().isValid())
        {
            batchTime = ent->updatedDate().toTime_t();
        }

        m_pendingEntitlementLookups[productId] = ent;
        m_deferredExtraContentLookups.remove(productId);
        m_failedEntitlementLookups.remove(productId);

        retrieveProductCatalogDefinition(productId, batchTime, true, ent->originPermissions() > Services::Publishing::OriginPermissionsNormal);
    }
}

void BaseGamesController::retrieveUnownedCatalogDefinition(const QString &productId)
{
#ifdef _DEBUG
    if (CatalogDefinitionController::ignoreOfferViaIsolateProductIds(productId))
    {
        ORIGIN_LOG_DEBUG << "****** Ignoring catalog definition for [" << productId << "] due to IsolateProductIds setting.";
        return;
    }
#endif

    if (m_baseGames.contains(productId) || m_extraContent.contains(productId))
    {
        // Do nothing
    }
    else if (!m_baseGamesInitialized)
    {
        if (!m_deferredExtraContentLookups.contains(productId))
        {
            m_deferredExtraContentLookups[productId] = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
        }
    }
    else if (!m_pendingEntitlementLookups.contains(productId) && !m_deferredExtraContentLookups.contains(productId))
    {
        Services::Publishing::NucleusEntitlementRef ent = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
        if (m_failedEntitlementLookups.contains(productId))
        {
            ent = m_failedEntitlementLookups.take(productId);
        }

        m_pendingEntitlementLookups[productId] = ent;

        retrieveProductCatalogDefinition(productId);
    }
}

void BaseGamesController::retrieveProductCatalogDefinition(QString productId, qint64 batchTime,
    bool fetchMissingBatchTime, bool elevatedPermissions)
{
    ORIGIN_ASSERT(QThread::currentThread() == thread());

    // If we have a valid cache, this will eventually lead to Entitlement::create.
    // We don't want to be creating Entitlements during shutdown or we'll crash!
    if (m_state == kShuttingDown || ! LoginController::instance()->isUserLoggedIn())
    {
        ORIGIN_LOG_DEBUG << "Ignored request to look up offer data for " << productId << " due to BaseGamesController shutdown.";
        return;
    }

    if (Services::Publishing::INVALID_BATCH_TIME == batchTime && fetchMissingBatchTime)
    {
        m_offersMissingBatchTimes.append(productId);
        if (MaxOfferUpdatedDateIds == m_offersMissingBatchTimes.size())
        {
            queryOfferUpdatedDates();
        }
        else if (!m_queryOfferUpdatedDatesTimer.isActive())
        {
            m_queryOfferUpdatedDatesTimer.start();
        }

        return;
    }

    QString qualifyingOfferId;
    if (m_pendingEntitlementLookups.contains(productId) && m_pendingEntitlementLookups[productId]->isValid())
    {
        qualifyingOfferId = productId;
    }
    else if (m_privateEligibleOfferIds.contains(productId))
    {
        qualifyingOfferId = m_privateEligibleOfferIds[productId];
    }

    if (!m_extraContentInitialized && CatalogDefinitionController::instance()->hasCachedCatalogDefinition(productId))
    {
        m_deferredCatalogDefinitionUpdates.insert(productId);
        onCatalogDefinitionLoaded(CatalogDefinitionController::instance()->getCachedCatalogDefinition(productId));
    }
    else
    {
        CatalogDefinitionQuery* query = CatalogDefinitionController::instance()->queryCatalogDefinition(productId,
            qualifyingOfferId, batchTime, CatalogDefinitionController::RefreshStale, elevatedPermissions);

        // We already have a cached definition, let's use it directly.  The lookup will propagate an updated signal on the
        // catalog definition if there were any changes on the CDN.
        if (query->definition() != Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
        {
            onCatalogDefinitionLoaded(query->definition());
            m_pendingCatalogDefinitionUpdates.insert(query);
            ORIGIN_VERIFY_CONNECT(query, &CatalogDefinitionQuery::finished, this, &BaseGamesController::onCatalogDefinitionQueryComplete);
        }
        else
        {
            m_pendingCatalogDefinitionUpdates.insert(query);
            ORIGIN_VERIFY_CONNECT(query, &CatalogDefinitionQuery::finished,
                this, &BaseGamesController::onCatalogDefinitionQueryComplete);
        }
    }
}

void BaseGamesController::onEntitlementGranted(Origin::Services::Publishing::NucleusEntitlementRef ent)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Services::Publishing::NucleusEntitlementRef, ent));

    const QString &productId = ent->productId();

    // See if we already know about this offer from a different entitlement..
    if (m_extraContent.contains(productId))
    {
        EntitlementRef extraContent = m_extraContent[productId];
        bool wasOwned = extraContent->isOwned();

        if (!wasOwned)
        {
            if (m_extraContentInitialized)
            {
                processSuppressions();
            }
            addExtraContent(extraContent);
        }
    }
    else if (m_baseGames.contains(productId))
    {
        // Do nothing
    }
    else
    {
        retrieveOwnedCatalogDefinition(ent);
    }
}

void BaseGamesController::onEntitlementDeleted(const QString& productId)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(const QString&, productId));

    EntitlementRef deletedContent;

    if (m_queuedEntitlementLookups.contains(productId))
    {
        m_queuedEntitlementLookups.remove(productId);
    }

    if (m_initialBaseGameIds.contains(productId))
    {
        m_initialBaseGameIds.remove(productId);
    }

    if (m_pendingEntitlementLookups.contains(productId))
    {
        m_pendingEntitlementLookups.remove(productId);
    }

    if (m_deferredExtraContentLookups.contains(productId))
    {
        m_deferredExtraContentLookups.remove(productId);
    }

    if (m_failedEntitlementLookups.contains(productId))
    {
        m_failedEntitlementLookups.remove(productId);
    }

    if (m_extraContent.contains(productId))
    {
        deletedContent = m_extraContent[productId];
        deletedContent->contentConfiguration()->setNucleusEntitlement(Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT);
        disownExtraContent(deletedContent);

        if (deletedContent->localContent() && deletedContent->localContent()->installFlow())
        {
            deletedContent->localContent()->installFlow()->cancel(true);
        }
    }

    if (m_baseGames.contains(productId))
    {
        BaseGameRef deletedBaseGame = m_baseGames[productId];

        {
            QWriteLocker lock(&m_baseGamesLock);
            m_baseGames.remove(productId);
        }

        const QString &masterTitleId = deletedBaseGame->contentConfiguration()->masterTitleId();
        if (!masterTitleId.isEmpty())
        {
            m_baseGamesByMasterTitleId[masterTitleId].remove(deletedBaseGame);
        }

        deletedBaseGame->contentConfiguration()->setNucleusEntitlement(Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT);

        if (deletedBaseGame->localContent() && deletedBaseGame->localContent()->installFlow())
        {
            deletedBaseGame->localContent()->installFlow()->cancel(true);
        }

        emit baseGameRemoved(deletedBaseGame);

        deletedContent = deletedBaseGame;
    }

    processSuppressions();
}

void BaseGamesController::onEntitlementBestMatchChanged(const QString& offerId, Origin::Services::Publishing::NucleusEntitlementRef newBestMatch)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(const QString&, offerId), Q_ARG(Origin::Services::Publishing::NucleusEntitlementRef, newBestMatch));

    if (!newBestMatch->isValid())
    {
        onEntitlementDeleted(offerId);
    }
    else if (m_extraContent.contains(offerId))
    {
        m_extraContent[offerId]->contentConfiguration()->setNucleusEntitlement(newBestMatch);
    }
    else if (m_baseGames.contains(offerId))
    {
        m_baseGames[offerId]->contentConfiguration()->setNucleusEntitlement(newBestMatch);
    }
    else
    {
        if (!m_entitlementControllerInitialized)
        {
            // Gather all of the ORIGIN_DOWNLOAD and ORIGIN_PREORDER entitlements before any processing, otherwise the
            // baseGamesLoaded signal can be emitted early if we have cached catalog definitions
            if (newBestMatch->tag().contains("ORIGIN_DOWNLOAD", Qt::CaseInsensitive) ||
                newBestMatch->tag().contains("ORIGIN_PREORDER", Qt::CaseInsensitive))
            {
                m_initialBaseGameIds.insert(newBestMatch->productId());
            }
        }

        // We haven't seen this offer before.  Save off the best match entitlement until we get the entitlementsUpdated signal.
        m_queuedEntitlementLookups[offerId] = newBestMatch;
    }
}

void BaseGamesController::onEntitlementsUpdated()
{
    ASYNC_INVOKE_GUARD;

    for (auto ent = m_queuedEntitlementLookups.constBegin(); ent != m_queuedEntitlementLookups.constEnd(); ++ent)
    {
        if (!ent->isNull())
        {
            ORIGIN_VERIFY(QMetaObject::invokeMethod(this, "onEntitlementGranted", Qt::QueuedConnection, Q_ARG(Origin::Services::Publishing::NucleusEntitlementRef, *ent)));
        }
    }

    m_queuedEntitlementLookups.clear();

    ORIGIN_VERIFY(QMetaObject::invokeMethod(this, "checkInitializationComplete", Qt::QueuedConnection));
}

void BaseGamesController::onCatalogDefinitionQueryComplete()
{
    CatalogDefinitionQuery* query = dynamic_cast<CatalogDefinitionQuery*>(sender());
    if(query != NULL)
    {
        // Protect against queries that complete after the user has logged out
        if (m_state != kShuttingDown)
        {
            if(query->definition() == Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
            {
                onCatalogDefinitionNotFound(query->offerId(), query->confidential());
            }
            else
            {
                onCatalogDefinitionLoaded(query->definition());
            }
        }
        else
        {
            m_pendingEntitlementLookups.remove(query->offerId());
        }

        m_pendingCatalogDefinitionUpdates.remove(query);
        query->deleteLater();

        checkRefreshComplete();
    }
}

void BaseGamesController::onEntitlementRefreshComplete(Origin::Services::Publishing::EntitlementRefreshType type)
{
    if(type == Services::Publishing::EntitlementRefreshTypeFull)
    {
        m_lastFullRefresh = Services::TrustedClock::instance()->nowUTC();
    }

    m_pendingEntitlementRefresh = false;
    checkRefreshComplete();
}

void BaseGamesController::onExtraContentRefreshComplete(QString masterTitleId)
{
    m_pendingExtraContentRefreshes.remove(masterTitleId);
    checkInitializationComplete();
    checkRefreshComplete();
}

void BaseGamesController::checkRefreshComplete()
{
    // Refresh is only complete when all of the following are true:
    // 1. Extracontent has been initialized
    // 2. There are no pending /extracontent or /basegames refreshes
    // 3. There are no pending or deferred extracontent offer data lookups (/public, /private)
    // 4. We are not collecting offer IDs toward an /offerUpdatedDates query
    // 5. There are no pending /offerUpdatedDates queries
    bool entitlementRefreshPending =  !m_extraContentInitialized  || !m_pendingExtraContentRefreshes.isEmpty() || m_pendingEntitlementRefresh;
    bool catalogRefreshPending = !m_pendingCatalogDefinitionUpdates.isEmpty() || !m_deferredCatalogDefinitionUpdates.isEmpty() || 
        !m_pendingExtraContentByParentMasterTitleId.isEmpty() || !m_offersMissingBatchTimes.isEmpty() || !m_pendingOfferUpdatedDateQueries.isEmpty();

    if (refreshInProgress() && !entitlementRefreshPending)
    {
        // User is able to interact with his game library at this point, so stop ContentRetrieval timer.
        GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::ContentRetrieval,
            m_baseGames.size() + m_extraContent.size());

        if (!catalogRefreshPending)
        {
            m_state = kIdle;
            emit refreshComplete();
        }
    }
}

void BaseGamesController::addExtraContent(EntitlementRef content)
{
    const ContentConfigurationRef &configuration = content->contentConfiguration();
    ORIGIN_ASSERT(configuration->canBeExtraContent());

    if (content->isSuppressed())
    {
        return;
    }

    // See if we have a base game that cares about this extra content
    const BaseGameSet &baseGames = baseGamesByChild(content);
    for (auto baseGame = baseGames.begin(); baseGame != baseGames.end(); ++baseGame)
    {
        if ((*baseGame)->addExtraContent(content))
        {
            emit baseGameExtraContentUpdated(*baseGame);
        }
    }
}

void BaseGamesController::disownExtraContent(EntitlementRef content)
{
    const ContentConfigurationRef &configuration = content->contentConfiguration();
    ORIGIN_ASSERT(configuration->canBeExtraContent());

    QStringList parentMasterTitleIds;

    parentMasterTitleIds << configuration->masterTitleId();
    parentMasterTitleIds.append(configuration->alternateMasterTitleIds());

    foreach (const QString &masterTitleId, parentMasterTitleIds)
    {
        // See if we have a base game that cares about this extra content
        if (m_baseGamesByMasterTitleId.contains(masterTitleId))
        {
            foreach (BaseGameRef baseGame, m_baseGamesByMasterTitleId[masterTitleId])
            {
                if (baseGame->contentConfiguration()->addonsAvailable() &&
                    baseGame->disownExtraContent(content))
                {
                    emit baseGameExtraContentUpdated(baseGame);
                }
            }
        }
    }
}

void BaseGamesController::deleteExtraContent(EntitlementRef content)
{
    const ContentConfigurationRef &configuration = content->contentConfiguration();
    ORIGIN_ASSERT(configuration->canBeExtraContent());

    QStringList parentMasterTitleIds;

    parentMasterTitleIds << configuration->masterTitleId();
    parentMasterTitleIds.append(configuration->alternateMasterTitleIds());

    foreach (const QString &masterTitleId, parentMasterTitleIds)
    {
        // See if we have a base game that cares about this extra content
        if (m_baseGamesByMasterTitleId.contains(masterTitleId))
        {
            foreach (BaseGameRef baseGame, m_baseGamesByMasterTitleId[masterTitleId])
            {
                if (baseGame->contentConfiguration()->addonsAvailable() &&
                    baseGame->deleteExtraContent(content))
                {
                    emit baseGameExtraContentUpdated(baseGame);
                }
            }
        }
    }
}

void BaseGamesController::onCatalogDefinitionLoaded(Origin::Services::Publishing::CatalogDefinitionRef def)
{
    const QString &productId = def->productId();
    if (!m_pendingEntitlementLookups.contains(productId))
    {
        ORIGIN_LOG_DEBUG_IF(Services::readSetting(Services::SETTING_LogCatalog).toQVariant().toBool()) << "Unexpected catalog definition loaded for " << productId;
        return;
    }

    BaseGameRef newBaseGame;

    removePendingExtraContent(productId);

    Services::Publishing::NucleusEntitlementRef ent = m_pendingEntitlementLookups.take(productId);

    if (def->isBaseGame())
    {
        BaseGameRef baseGame;

        if (m_baseGames.contains(productId))
        {
            baseGame = m_baseGames[productId];
        }
        else
        {
            if (m_extraContent.contains(productId))
            {
                baseGame = qSharedPointerDynamicCast<BaseGame>(m_extraContent[productId]);
            }
            else
            {
                baseGame = BaseGame::create(def, ent);
                ORIGIN_VERIFY_CONNECT(baseGame->contentConfiguration().data(), &ContentConfiguration::configurationChanged,
                    this, &BaseGamesController::onContentConfigurationChanged);
            }

            if (baseGame->isOwned())
            {
                newBaseGame = baseGame;

                const QString &masterTitleId = def->masterTitleId();
                if (!masterTitleId.isEmpty())
                {
                    bool elevatedPermissions = baseGame->contentConfiguration()->originPermissions() > Services::Publishing::OriginPermissionsNormal;

                    m_baseGamesByMasterTitleId[masterTitleId].insert(baseGame);

                    // If we have extra content, query for it now...
                    if (def->addonsAvailable())
                    {
                        m_masterTitlesWithExtraContent.insert(masterTitleId);

                        foreach (const QString &extraContentOfferId, def->availableExtraContent())
                        {
                            if (!m_extraContent.contains(extraContentOfferId))
                            {
                                m_pendingExtraContentByParentMasterTitleId[masterTitleId].insert(extraContentOfferId);
                            }

                            if (elevatedPermissions)
                            {
                                m_privateEligibleOfferIds[extraContentOfferId] = productId;
                            }
                        }

                        if (!m_pendingExtraContentRefreshes.contains(masterTitleId))
                        {
                            m_pendingExtraContentRefreshes.insert(masterTitleId);
                            if (m_baseGamesInitialized)
                            {
                                nucleusEntitlementController()->refreshExtraContentEntitlements(masterTitleId);
                            }
                        }

                        foreach (EntitlementRef extraContent, m_extraContentByParentMasterTitleId[masterTitleId])
                        {
                            if (!extraContent->isSuppressed())
                            {
                                baseGame->addExtraContent(extraContent);
                            }
                        }
                    }
                }

                {
                    QWriteLocker lock(&m_baseGamesLock);
                    m_baseGames.insert(productId, baseGame);
                }

                if (!masterTitleId.isEmpty() && def->addonsAvailable())
                {
                    foreach (const QString &extraContentOfferId, def->availableExtraContent())
                    {
                        retrieveUnownedCatalogDefinition(extraContentOfferId);
                    }
                }
            }
        }

        // In the case of Sims expansions, they are both base games and extra content.
        if (def->canBeExtraContent())
        {
            QWriteLocker lock(&m_extraContentLock);

            if (!m_extraContent.contains(productId))
            {
                const QString &masterTitleId = baseGame->contentConfiguration()->masterTitleId();

                m_extraContent.insert(productId, baseGame);

                m_extraContentByParentMasterTitleId[masterTitleId].insert(baseGame);
                foreach (const QString &alternateMasterTitleId, baseGame->contentConfiguration()->alternateMasterTitleIds())
                {
                    m_extraContentByParentMasterTitleId[alternateMasterTitleId].insert(baseGame);
                }
            }

            addExtraContent(baseGame);
        }
    }
    else if (def->canBeExtraContent())
    {
        EntitlementRef extraContent;

        if (m_extraContent.contains(productId))
        {
            extraContent = m_extraContent[productId];
        }
        else
        {
            extraContent = Entitlement::create(def, ent);
            ORIGIN_VERIFY_CONNECT(extraContent->contentConfiguration().data(), &ContentConfiguration::configurationChanged,
                this, &BaseGamesController::onContentConfigurationChanged);

            QWriteLocker lock(&m_extraContentLock);
            m_extraContent.insert(extraContent->contentConfiguration()->productId(), extraContent);

            const QString &masterTitleId = extraContent->contentConfiguration()->masterTitleId();
            m_extraContentByParentMasterTitleId[masterTitleId].insert(extraContent);
            foreach (const QString& alternateMasterTitleId, extraContent->contentConfiguration()->alternateMasterTitleIds())
            {
                m_extraContentByParentMasterTitleId[alternateMasterTitleId].insert(extraContent);
            }
        }

        addExtraContent(extraContent);
    }

    if (m_baseGamesInitialized)
    {
        if (m_extraContentInitialized)
        {
            processSuppressions();
        }

        if (!newBaseGame.isNull() && !newBaseGame->isSuppressed())
        {
            emit baseGameAdded(newBaseGame);
        }
    }
    else
    {
        m_initialBaseGameIds.remove(productId);
    }

    checkInitializationComplete();
}

BaseGameSet BaseGamesController::baseGames() const
{
    QReadLocker lock(&m_baseGamesLock);

    BaseGameSet validBaseGames;

    foreach (BaseGameRef baseGame, m_baseGames)
    {
        if(!baseGame->isSuppressed())
        {
            validBaseGames.insert(baseGame);
        }
    }

    return validBaseGames;
}

BaseGameSet BaseGamesController::baseGamesByChild(EntitlementRef child) const
{
    QStringList parentMasterTitleIds;
    BaseGameSet validBaseGames;

    parentMasterTitleIds << child->contentConfiguration()->masterTitleId();
    parentMasterTitleIds.append(child->contentConfiguration()->alternateMasterTitleIds());

    QReadLocker lock(&m_baseGamesLock);
    for (auto masterTitleIdIter = parentMasterTitleIds.begin(); masterTitleIdIter != parentMasterTitleIds.end(); ++masterTitleIdIter)
    {
        const auto baseGames = m_baseGamesByMasterTitleId.find(*masterTitleIdIter);

        if (baseGames != m_baseGamesByMasterTitleId.end())
        {
            for (auto baseGameIter = baseGames->begin(); baseGameIter != baseGames->end(); ++baseGameIter)
            {
                const BaseGameRef &baseGame = *baseGameIter;

                if (!baseGame->isSuppressed() &&
                    baseGame->contentConfiguration()->addonsAvailable() &&
                    (Services::Publishing::OriginDisplayTypeFullGamePlusExpansion != child->contentConfiguration()->originDisplayType() ||
                    Services::Publishing::OriginDisplayTypeFullGamePlusExpansion != baseGame->contentConfiguration()->originDisplayType()))
                {
                    validBaseGames.insert(baseGame);
                }
            }
        }
    }

    return validBaseGames;
}

void BaseGamesController::onContentConfigurationChanged()
{
    ASYNC_INVOKE_GUARD;

    // Determine if any addons have been added or removed from the definition
    ContentConfiguration *configuration = dynamic_cast<ContentConfiguration *>(sender());
    if (configuration && configuration->addonsAvailable())
    {
        bool elevatedPermissions = configuration->originPermissions() > Services::Publishing::OriginPermissionsNormal;
        const QString &masterTitleId = configuration->masterTitleId();
        const QSet<QString> &availableExtraContent = configuration->availableExtraContent().toSet();
        foreach (const QString &offerId, availableExtraContent)
        {
            if (m_extraContent.contains(offerId))
            {
                addExtraContent(m_extraContent[offerId]);
            }
            else
            {
                m_pendingExtraContentByParentMasterTitleId[masterTitleId].insert(offerId);

                if (elevatedPermissions)
                {
                    m_privateEligibleOfferIds[offerId] = configuration->productId();
                }

                retrieveUnownedCatalogDefinition(offerId);
            }
        }

        EntitlementRefSet knownOffers = m_extraContentByParentMasterTitleId[masterTitleId];
        foreach (const EntitlementRef &extraContent, knownOffers)
        {
            // Only delete the unavailable extra content if it's not owned and is not used by another master title.  
            // Some offers will never be "available" but users may still own them.  For example, pre-order bonuses 
            // and special content that is bundled with a "deluxe" edition (see OFB-MASS:46484 - ME3 Collector's 
            // Edition Materials).  Other titles may be part of multiple master titles.  For example, Sims 3 expansions
            // (see Origin.OFR.50.0000124 - The Sims 3 Movie Stuff).
            if (!availableExtraContent.contains(extraContent->contentConfiguration()->productId()) && !extraContent->isOwned())
            {
                m_extraContentByParentMasterTitleId[masterTitleId].remove(extraContent);

                EntitlementRefSet alternateExtraContent;
                foreach (const QString& alternateMasterTitleId, configuration->alternateMasterTitleIds())
                {
                    alternateExtraContent.unite(m_extraContentByParentMasterTitleId[alternateMasterTitleId]);
                }

                if (!alternateExtraContent.contains(extraContent))
                {
                    deleteExtraContent(extraContent);
                }
            }
        }
    }

    if (m_extraContentInitialized)
    {
        // We have to reprocess suppressions anytime a catalog definition is changed...
        processSuppressions();
    }
}

void BaseGamesController::retryFailedEntitlementRequests()
{
    ASYNC_INVOKE_GUARD;

    m_failedEntitlementRetryTimer.stop();

    NucleusEntitlementHash failedLookups = m_failedEntitlementLookups;
    m_failedEntitlementLookups.clear();

    QHashIterator<QString, Services::Publishing::NucleusEntitlementRef> failedLookup(failedLookups);
    while (failedLookup.hasNext())
    {
        failedLookup.next();
        if (failedLookup.value()->isValid())
        {
            retrieveUnownedCatalogDefinition(failedLookup.key());
        }
        else
        {
            onEntitlementGranted(failedLookup.value());
        }
    }
}

void BaseGamesController::onCatalogDefinitionNotFound(QString catalogOfferId, bool confidential)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, catalogOfferId), Q_ARG(bool, confidential));

    removePendingExtraContent(catalogOfferId);

    ORIGIN_LOG_ERROR << "Pending entitlement could not find matching catalog offer definition for defined offer id: " << catalogOfferId <<
        ", confidential=" << (confidential ? "true" : "false"); 
    if (m_pendingEntitlementLookups.contains(catalogOfferId))
    {
        Services::Publishing::NucleusEntitlementRef ent = m_pendingEntitlementLookups.take(catalogOfferId);

        // EBIBUGS-29222 : Avoid retrying for confidential offers
        if (!confidential)
        {
            m_failedEntitlementLookups[catalogOfferId] = ent;

            // EBIBUGS-29240 : Avoid retrying if we are offline.  When the client goes back online, BaseGamesController::refresh(RenewAll) is called, and definition look-ups resume.
            const bool online = Services::Connection::ConnectionStatesService::isUserOnline(LoginController::instance()->currentUser()->getSession());
            if (!m_failedEntitlementRetryTimer.isActive() && online)
            {
                m_failedEntitlementRetryTimer.start();
            }
        }
    }
    m_initialBaseGameIds.remove(catalogOfferId);

    checkInitializationComplete();
}

void BaseGamesController::checkInitializationComplete()
{
    if (!m_baseGamesInitialized && m_initialBaseGameIds.isEmpty())
    {
        ORIGIN_LOG_EVENT << "Initialized base games controllers, processing catalog suppressions.";
        processSuppressions();
        m_baseGamesInitialized = true;

        const bool cached = nucleusEntitlementController()->initializationSource() == NucleusEntitlementController::FromCache;
        GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::ContentRetrieval,
            m_baseGames.size() + m_extraContent.size(), cached ? 1 : 0);
        GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_CLEAR(EbisuTelemetryAPI::ContentRetrieval);

        // If we initialized our entitlements from the cache, defer fetching extra content entitlements
        // until we're completely initialized
        if (nucleusEntitlementController()->initializationSource() == NucleusEntitlementController::FromCache)
        {
            m_deferredExtraContentRefreshes = m_pendingExtraContentRefreshes;
            m_pendingExtraContentRefreshes.clear();
        }

        for (auto it = m_pendingExtraContentRefreshes.begin(); it != m_pendingExtraContentRefreshes.end(); ++it)
        {
            nucleusEntitlementController()->refreshExtraContentEntitlements(*it);
        }

        // EBIBUGS-29109: Set lastFullRefresh if we successfully loaded entitlements so telemetry is properly sent
        if (!m_lastFullRefresh.isValid() &&
            NucleusEntitlementController::ServerError != nucleusEntitlementController()->initializationSource())
        {
            m_lastFullRefresh = Services::TrustedClock::instance()->nowUTC();
        }

        emit baseGamesLoaded();

        // Now that all of our base games are loaded, let's fetch extracontent data.
        processDeferredExtraContent();
    }

    if (m_baseGamesInitialized &&
        !m_extraContentInitialized &&
        m_pendingExtraContentRefreshes.isEmpty() &&
        m_deferredExtraContentLookups.isEmpty() &&
        m_pendingEntitlementLookups.isEmpty())
    {
        ORIGIN_LOG_DEBUG << "Initialized extra content, processing catalog suppressions.";
        processSuppressions();
        m_extraContentInitialized = true;

        for (auto it = m_deferredExtraContentRefreshes.begin(); it != m_deferredExtraContentRefreshes.end(); ++it)
        {
            nucleusEntitlementController()->refreshExtraContentEntitlements(*it);
            m_pendingExtraContentRefreshes.insert(*it);
        }
        m_deferredExtraContentRefreshes.clear();

        for (auto it = m_deferredCatalogDefinitionUpdates.begin(); it != m_deferredCatalogDefinitionUpdates.end(); ++it)
        {
            const QString &offerId = *it;
            EntitlementRef ent;

            if (m_extraContent.contains(offerId))
            {
                ent = m_extraContent[offerId];
            }
            else if (m_baseGames.contains(offerId))
            {
                ent = m_baseGames[offerId];
            }
            else
            {
                ORIGIN_LOG_ERROR_IF(Services::readSetting(Services::SETTING_LogCatalog).toQVariant().toBool()) << "Unable to find entitlement for deferred catalog definition update " << offerId;
                continue;
            }

            QString qualifyingOfferId;
            qint64 batchTime = Services::Publishing::INVALID_BATCH_TIME;

            if (ent->isOwned())
            {
                qualifyingOfferId = offerId;
                if (ent->primaryNucleusEntitlement()->updatedDate().isValid())
                {
                    batchTime = ent->primaryNucleusEntitlement()->updatedDate().toTime_t();
                }
            }
            else if (m_privateEligibleOfferIds.contains(offerId))
            {
                qualifyingOfferId = m_privateEligibleOfferIds[offerId];
            }

            CatalogDefinitionQuery* query = CatalogDefinitionController::instance()->queryCatalogDefinition(offerId,
                qualifyingOfferId, batchTime, CatalogDefinitionController::RefreshStale,
                ent->contentConfiguration()->originPermissions() > Services::Publishing::OriginPermissionsNormal);

            m_pendingCatalogDefinitionUpdates.insert(query);
            ORIGIN_VERIFY_CONNECT(query, &CatalogDefinitionQuery::finished, this, &BaseGamesController::onCatalogDefinitionQueryComplete);
        }

        m_deferredCatalogDefinitionUpdates.clear();
    }
}

void BaseGamesController::processSuppressions()
{
    QSet<QString> suppressedContent;
    BaseGameSet updatedBaseGames;

    // Build up a set of all suppressed content
    foreach (EntitlementRef baseGame, m_baseGames)
    {
        if (Services::Publishing::NucleusEntitlement::ExternalTypeSubscription != baseGame->primaryNucleusEntitlement()->externalType())
        {
            suppressedContent.unite(baseGame->contentConfiguration()->suppressedOfferIds().toSet());
        }
    }

    QSet<QString> ownedItemIds;
    foreach (EntitlementRef extraContent, m_extraContent)
    {
        if (extraContent->isOwned())
        {
            ownedItemIds.insert(extraContent->contentConfiguration()->itemId());
            if (Services::Publishing::NucleusEntitlement::ExternalTypeSubscription != extraContent->primaryNucleusEntitlement()->externalType())
            {
                suppressedContent.unite(extraContent->contentConfiguration()->suppressedOfferIds().toSet());
            }
        }
    }

    foreach (EntitlementRef extraContent, m_extraContent)
    {
        if (!extraContent->isOwned() && ownedItemIds.contains(extraContent->contentConfiguration()->itemId()))
        {
            suppressedContent.insert(extraContent->contentConfiguration()->productId());
            ORIGIN_LOG_DEBUG_IF(Services::readSetting(Services::SETTING_LogCatalog).toQVariant().toBool() && !extraContent->isSuppressed()) << 
                "Suppressing unowned extra content [" << extraContent->contentConfiguration()->productId() << "] due to owned item id [" <<
                extraContent->contentConfiguration()->itemId() << "]";
        }
    }

    const QSet<QString> &newlySuppressedOffers = suppressedContent - m_suppressedContent;
    foreach (const QString &offerId, newlySuppressedOffers)
    {
        if (m_baseGames.contains(offerId))
        {
            BaseGameRef baseGame = m_baseGames[offerId];

            baseGame->suppress(true);
            if (m_baseGamesInitialized)
            {
                emit baseGameRemoved(baseGame);
            }
        }

        if (m_extraContent.contains(offerId))
        {
            EntitlementRef content = m_extraContent[offerId];
            const QString &masterTitleId = content->contentConfiguration()->masterTitleId();

            content->suppress(true);
            deleteExtraContent(content);
        }
    }

    const QSet<QString> &unsuppressedOffers = m_suppressedContent - suppressedContent;
    foreach (const QString &offerId, unsuppressedOffers)
    {
        if (m_baseGames.contains(offerId))
        {
            BaseGameRef baseGame = m_baseGames[offerId];

            baseGame->suppress(false);
            if (m_baseGamesInitialized)
            {
                emit baseGameAdded(baseGame);
            }
        }

        if (m_extraContent.contains(offerId))
        {
            EntitlementRef content = m_extraContent[offerId];
            const QString &masterTitleId = content->contentConfiguration()->masterTitleId();

            content->suppress(false);
            if (m_baseGamesByMasterTitleId.contains(masterTitleId))
            {
                foreach (BaseGameRef baseGame, m_baseGamesByMasterTitleId[masterTitleId])
                {
                    baseGame->addExtraContent(content);
                    updatedBaseGames.insert(baseGame);
                }
            }
        }
    }

    if (m_baseGamesInitialized)
    {
        foreach (BaseGameRef baseGame, updatedBaseGames)
        {
            if (!baseGame->isSuppressed())
            {
                emit baseGameExtraContentUpdated(baseGame);
            }
        }
    }

    m_suppressedContent = suppressedContent;
}

void BaseGamesController::removePendingExtraContent(const QString &offerId)
{
    QHash<QString, QSet<QString>>::iterator it = m_pendingExtraContentByParentMasterTitleId.begin();
    while (it != m_pendingExtraContentByParentMasterTitleId.end())
    {
        it.value().remove(offerId);
        if (it.value().isEmpty())
        {
            it = m_pendingExtraContentByParentMasterTitleId.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void BaseGamesController::onDirtyBitsEntitlementUpdate(const QByteArray &dbPayload)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QByteArray, dbPayload));

    // Should be okay to send this without a delay since we're responding to a change to a single user's account,
    // not something global like a catalog offer update.
    GetTelemetryInterface()->Metric_ENTITLEMENT_DIRTYBIT_NOTIFICATION();

    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(dbPayload, &jsonResult);

    if (jsonResult.error == QJsonParseError::NoError)
    {
        const QJsonObject dirtyBitNotification = jsonDoc.object();
        const QString verb = dirtyBitNotification.value("verb").toString().toLower();
        const QJsonObject data = dirtyBitNotification.value("data").toObject();
        const QString operation = data.value("status").toString().toLower();

        if ("upd" == verb && "deleted" == operation)
        {
            const quint64 entId = static_cast<quint64>(data.value("entId").toDouble());
            if (nucleusEntitlementController()->entitlementById(entId) != Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT)
            {
                // Don't need to refresh when deleting a known entitlement, just remove it.
                nucleusEntitlementController()->deleteEntitlement(entId);
                return;
            }
            else
            {
                // Unknown entitlement id, do a full refresh
                m_dirtyBitsRefreshMode = RenewAll;
            }
        }
        else if ("add" == verb || "upd" == verb)
        {
            const QString& status = dirtyBitNotification.value("data").toObject().value("status").toString().toUpper();
            if (status != "ACTIVE")
            {
                // This entitlement was DISABLED or DELETED so we must do a full refresh to fix suppressions.
                m_dirtyBitsRefreshMode = RenewAll;
            }
            else if (Content::CacheOnly == m_dirtyBitsRefreshMode)
            {
                // Entitlement status update or new entitlement granted
                m_dirtyBitsRefreshMode = ContentUpdates;
            }
        }
        else
        {
            // Not sure what verb this is, but assume the worst and refresh all
            m_dirtyBitsRefreshMode = RenewAll;
        }
    }
    else
    {
        // If we failed to parse the payload for any reason, just refresh everything
        m_dirtyBitsRefreshMode = RenewAll;
    }

    ORIGIN_ASSERT(Content::CacheOnly != m_dirtyBitsRefreshMode);
    m_dirtyBitsRefreshTimer.start();
}

void BaseGamesController::processDeferredExtraContent()
{
    ASYNC_INVOKE_GUARD;

    // Bail if we're shutting down.
    if (m_state == kShuttingDown)
    {
        return;
    }
    
    for (auto it = m_deferredExtraContentLookups.begin(); it != m_deferredExtraContentLookups.end(); ++it)
    {
        const QString &offerId = it.key();
        const Services::Publishing::NucleusEntitlementRef ent = it.value();

        qint64 batchTime = Services::Publishing::INVALID_BATCH_TIME;
        bool elevatedPermissions = false;
        if (!ent.isNull() && ent->isValid())
        {
            elevatedPermissions = ent->originPermissions() > Services::Publishing::OriginPermissionsNormal;
            if (ent->updatedDate().isValid())
            {
                batchTime = ent->updatedDate().toTime_t();
            }
        }

        ORIGIN_ASSERT(!m_pendingEntitlementLookups.contains(offerId));
        m_pendingEntitlementLookups[offerId] = ent;

        // Asynchronously queue up calls to retrieveProductCatalogDefinition(offerId, batchTime);
        ORIGIN_VERIFY(QMetaObject::invokeMethod(this, "retrieveProductCatalogDefinition", Qt::QueuedConnection, Q_ARG(QString, offerId), Q_ARG(qint64, batchTime),
            Q_ARG(bool, true), Q_ARG(bool, elevatedPermissions)));
    }

    m_deferredExtraContentLookups.clear();
}

void BaseGamesController::onDirtyBitsRefreshTimer()
{
    ORIGIN_LOG_EVENT << "Triggering entitlement refresh due to dirty bit entitlement notification.";
    GetTelemetryInterface()->Metric_ENTITLEMENT_DIRTYBIT_REFRESH();

    refresh(m_dirtyBitsRefreshMode);

    m_dirtyBitsRefreshMode = CacheOnly;
}

void BaseGamesController::queryOfferUpdatedDates()
{
    using Services::Publishing::OfferUpdatedDateQuery;

    m_queryOfferUpdatedDatesTimer.stop();

    OfferUpdatedDateQuery *query = CatalogDefinitionController::instance()->queryOfferUpdatedDates(m_offersMissingBatchTimes);

    m_pendingOfferUpdatedDateQueries.insert(query);

    ORIGIN_VERIFY_CONNECT(query, &OfferUpdatedDateQuery::finished, this, &BaseGamesController::onOfferUpdatedDateQueryComplete);

    m_offersMissingBatchTimes.clear();
}

void BaseGamesController::onOfferUpdatedDateQueryComplete()
{
    using Services::Publishing::OfferUpdatedDateQuery;

    OfferUpdatedDateQuery *reply = dynamic_cast<OfferUpdatedDateQuery *>(sender());
    ORIGIN_ASSERT(reply);

    const QHash<QString, qint64> &offerUpdatedDates = reply->offerUpdatedDates();
    for (auto offer = offerUpdatedDates.begin(); offer != offerUpdatedDates.end(); ++offer)
    {
        retrieveProductCatalogDefinition(offer.key(), offer.value(), false);
    }

    m_pendingOfferUpdatedDateQueries.remove(reply);
    reply->deleteLater();

    checkRefreshComplete();
}

void BaseGamesController::onUserAboutToLogout()
{
    ORIGIN_LOG_DEBUG << "BaseGamesController initiating shutdown...";
    m_state = kShuttingDown;

    ORIGIN_LOG_DEBUG << "BaseGamesController shutting down with " << m_pendingCatalogDefinitionUpdates.size() << " lookups pending.";
    for (auto it = m_pendingCatalogDefinitionUpdates.constBegin(); it != m_pendingCatalogDefinitionUpdates.constEnd(); ++it)
    {
        CatalogDefinitionQuery *query = *it;
        if (query)
        {
            ORIGIN_VERIFY_DISCONNECT(query, 0, this, 0);
            query->abort();
            query->deleteLater();
        }
    }
    m_pendingCatalogDefinitionUpdates.clear();

    ORIGIN_LOG_DEBUG << "BaseGamesController shutting down with " << m_pendingOfferUpdatedDateQueries.size() << " offer date queries pending.";
    for (auto it = m_pendingOfferUpdatedDateQueries.constBegin(); it != m_pendingOfferUpdatedDateQueries.constEnd(); ++it)
    {
        Services::Publishing::OfferUpdatedDateQuery *query = *it;
        if (query)
        {
            ORIGIN_VERIFY_DISCONNECT(query, 0, this, 0);
            query->abort();
            query->deleteLater();
        }
    }
    m_pendingOfferUpdatedDateQueries.clear();

    for (auto it = m_baseGames.constBegin(); it != m_baseGames.constEnd(); ++it)
    {
        Engine::Content::BaseGameRef game = *it;
        if (!game.isNull())
        {
            ORIGIN_VERIFY_DISCONNECT(game->contentConfiguration().data(), 0, this, 0);
        }
    }

    for (auto it = m_extraContent.constBegin(); it != m_extraContent.constEnd(); ++it)
    {
        Engine::Content::EntitlementRef ent = *it;
        if (!ent.isNull())
        {
            ORIGIN_VERIFY_DISCONNECT(ent->contentConfiguration().data(), 0, this, 0);
        }
    }

    ORIGIN_VERIFY_DISCONNECT(nucleusEntitlementController(), 0, this, 0);

    ORIGIN_VERIFY_DISCONNECT(&m_failedEntitlementRetryTimer, 0, this, 0);
    ORIGIN_VERIFY_DISCONNECT(&m_dirtyBitsRefreshTimer, 0, this, 0);
    ORIGIN_VERIFY_DISCONNECT(&m_queryOfferUpdatedDatesTimer, 0, this, 0);

    m_failedEntitlementRetryTimer.stop();
    m_dirtyBitsRefreshTimer.stop();
    m_queryOfferUpdatedDatesTimer.stop();
}

} // namespace Content

} // namespace Engine

} // namespace Origin
