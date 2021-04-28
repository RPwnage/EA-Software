//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include "content/NucleusEntitlementController.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "engine/content/ContentController.h"

#include "services/config/OriginConfigService.h"
#include "services/publishing/NucleusEntitlementServiceClient.h"
#include "services/session/SessionService.h"
#include "services/downloader/Common.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/config/OriginConfigService.h"
#include "services/platform/TrustedClock.h"

#include "TelemetryAPIDLL.h"

#include <QRunnable>
#include <QReadWriteLock>

namespace Origin
{

namespace Engine
{

class User;

namespace Content
{

const QString EntitlementCacheFolder = "NucleusCache";
const QString EntitlementCacheFileExtension = "ent";

const quint32 EntitlementCacheDataVersion = 2;
const Services::CryptoService::Cipher EntitlementCacheCipher = Services::CryptoService::BLOWFISH;
const Services::CryptoService::CipherMode EntitlementCacheCipherMode = Services::CryptoService::CIPHER_BLOCK_CHAINING;
const QByteArray EntitlementCacheEntropy = "fqc!WJ,)G`Of/`.0@CY@oaK)%f:5BKQa=8BG7-c{FOf~ul.Sb}7eL7p@)q&!~`A|Xj_zPBcXxv<"
    "BWj=*pZ+B|$UEHSn/zO0P9c><\"[%;9:zmlo_x]ni!2]8]O!0^R!";

void logRefreshSuccessTelemetry(Origin::Services::Publishing::NucleusEntitlementServiceResponse *resp, int entitlementDelta)
{
    if(resp->refreshType() == Services::Publishing::EntitlementRefreshTypeExtraContent)
    {
        GetTelemetryInterface()->Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED(resp->extraContentRequestedMasterTitleId().toUtf8(), resp->currentServerTime().toTime_t(), resp->entitlements().count());
    }
    else
    {
        GetTelemetryInterface()->Metric_ENTITLEMENT_REFRESH_COMPLETED(resp->currentServerTime().toTime_t(), resp->entitlements().count(), entitlementDelta);
    }
}

void logRefreshFailureTelemetry(Origin::Services::Publishing::NucleusEntitlementServiceResponse *resp)
{
    // Suppress entitlement network request error telemetry when we're offline to avoid reporting failures 
    // due to network connectivity issues (or the user choosing to manually go offline).
    const bool online = Services::Connection::ConnectionStatesService::isUserOnline(Services::Session::SessionService::currentSession());
    if (online)
    {
        ORIGIN_LOG_ERROR << "Entitlement response error.  Error = " << resp->error() << "; HTTP = " << resp->httpStatus() << 
            "; status = " << resp->refreshStatus() << "; type = " << resp->refreshType() << "; masterTitleId = " << resp->extraContentRequestedMasterTitleId();

        if(resp->refreshType() == Services::Publishing::EntitlementRefreshTypeExtraContent)
        {
            GetTelemetryInterface()->Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR(resp->extraContentRequestedMasterTitleId().toUtf8(), resp->error(), resp->httpStatus(), resp->refreshStatus());
        }
        else
        {
            GetTelemetryInterface()->Metric_ENTITLEMENT_REFRESH_ERROR(resp->error(), resp->httpStatus(), resp->refreshStatus());
        }
    }
}

NucleusEntitlementQuery::NucleusEntitlementQuery(quint64 entitlementId, Origin::Services::Publishing::NucleusEntitlementServiceResponse* lookup, Origin::Services::Publishing::NucleusEntitlementRef currentEnt) :
    m_entitlementId(entitlementId)
    ,m_entitlement(currentEnt)
{
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::NucleusEntitlementServiceResponse::finished,
        this, &NucleusEntitlementQuery::lookupFinished);
}

void NucleusEntitlementQuery::lookupFinished()
{
    Origin::Services::Publishing::NucleusEntitlementServiceResponse* lookup = dynamic_cast<Origin::Services::Publishing::NucleusEntitlementServiceResponse*>(sender());

    if(lookup != NULL)
    {
        m_error = lookup->error();
        m_httpStatus = lookup->httpStatus();

        if(m_error == Origin::Services::restErrorSuccess && 
            !lookup->entitlements().empty())
        {
            logRefreshSuccessTelemetry(lookup, 0);

            // If we already had an entitlement update it in place.  Anyone listening for configuration updates will hear it.
            if(m_entitlement != Origin::Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT)
            {
                m_entitlement->updateEntitlement(lookup->entitlements().at(0));
            }
            else
            {
                m_entitlement = lookup->entitlements().at(0);
            }
        }
        else
        {
            logRefreshFailureTelemetry(lookup);
        }

        lookup->deleteLater();
    }

    emit queryCompleted();
}

/// \brief Creates a new nucleus entitlement controller.
///
/// \param  user  The userId whose entitlements we wish to control. 
/// \returns ContentController Returns a pointer to the newly created nucleus entitlement controller.
NucleusEntitlementController::NucleusEntitlementController()
    : m_initializationSource(UnknownInitializationSource)
{
    const QString &env = Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString();
    QString entitlementCacheFolder = EntitlementCacheFolder;
    if (Services::env::production != env)
    {
        entitlementCacheFolder.append(QString(".%1").arg(env));
    }
    setCacheFolder(entitlementCacheFolder);

    // Only write the private cache when we haven't received a new entitlement for
    // two seconds to avoid thrashing the cache file
    m_writeUserCacheDataTimer.setInterval(2000);
    m_writeUserCacheDataTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_writeUserCacheDataTimer, &QTimer::timeout,
        this, &NucleusEntitlementController::writeUserCacheData);

    setCacheDataVersion(EntitlementCacheDataVersion);
    setCacheFileExtension(EntitlementCacheFileExtension);
    setCacheCipher(EntitlementCacheCipher);
    setCacheCipherMode(EntitlementCacheCipherMode);
    setCacheEntropy(EntitlementCacheEntropy);
}

/// \brief The NucleusEntitlementController destructor; releases the resources used by the NucleusEntitlementController.
NucleusEntitlementController::~NucleusEntitlementController()
{
    // LARS3_TODO: Close the database
}

void NucleusEntitlementController::init()
{
    qRegisterMetaType<EntitlementRefreshMode>("Origin::Engine::Content::NucleusEntitlementController::EntitlementRefreshMode");
}

void NucleusEntitlementController::initUserEntitlements()
{
    ASYNC_INVOKE_GUARD;
    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::ContentRetrieval);
    readUserCacheData();
    refreshUserEntitlements(BaseGamesOnly);
}

int NucleusEntitlementController::numberOfEntitlements() const
{
    QReadLocker lock(&m_entitlementLock);
    int count = 0;
    foreach(const Services::Publishing::NucleusEntitlementList& entList, m_entitlements)
    {
        count += entList.size();
    }
    return count;
}

QDateTime NucleusEntitlementController::lastResponseTime() const
{
    return m_lastResponseTime;
}

Services::Publishing::NucleusEntitlementList NucleusEntitlementController::entitlements() const
{
    QReadLocker lock(&m_entitlementLock);
    Services::Publishing::NucleusEntitlementList retVal;
    foreach(const Services::Publishing::NucleusEntitlementList& entList, m_entitlements)
    {
        retVal += entList;
    }
    return retVal;
}

Services::Publishing::NucleusEntitlementList NucleusEntitlementController::entitlements(const QString& offerId) const
{
    QReadLocker lock(&m_entitlementLock);
    Services::Publishing::NucleusEntitlementList retVal;
    if (m_entitlements.contains(offerId))
    {
        retVal += m_entitlements[offerId];
    }
    return retVal;
}

void NucleusEntitlementController::refreshUserEntitlements(EntitlementRefreshMode mode)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Engine::Content::NucleusEntitlementController::EntitlementRefreshMode, mode));

    using namespace Origin::Services::Publishing;

    // Start online query, load db from memory, emit what base game entitlements we have
    Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
    if (session.isNull())
    {
        return;
    }

    switch(mode)
    {
    case BaseGamesOnly:
        {
            NucleusEntitlementServiceResponse *resp = NucleusEntitlementServiceClient::baseGames(session);

            ORIGIN_VERIFY_CONNECT(resp, &NucleusEntitlementServiceResponse::success,
                this, &NucleusEntitlementController::onEntitlementRequestSuccess);
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)),
                this, SLOT(onEntitlementRequestError()));
            break;
        }

    case ContentUpdates:
        {
            NucleusEntitlementServiceResponse *resp = NucleusEntitlementServiceClient::contentUpdates(session, m_lastResponseTime);

            ORIGIN_VERIFY_CONNECT(resp, &NucleusEntitlementServiceResponse::success,
                this, &NucleusEntitlementController::onEntitlementRequestSuccess);
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)),
                this, SLOT(onEntitlementRequestError()));
            break;
        }

    case CacheOnly:
        readUserCacheData();
        break;

    default:
        break;
    }
}

NucleusEntitlementQuery* NucleusEntitlementController::refreshSpecificEntitlement(quint64 entitlementId)
{
    using namespace Origin::Services::Publishing;

    NucleusEntitlementServiceResponse* resp = NucleusEntitlementServiceClient::entitlementInfo(Origin::Services::Session::SessionService::currentSession(), entitlementId);

    NucleusEntitlementRef existingEnt = entitlementById(entitlementId);

    return new NucleusEntitlementQuery(entitlementId, resp, existingEnt);
}

void NucleusEntitlementController::refreshExtraContentEntitlements(const QString& masterTitleId)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(const QString&, masterTitleId));

    using namespace Origin::Services::Publishing;
    NucleusEntitlementServiceResponse* resp = NucleusEntitlementServiceClient::extraContent(Origin::Services::Session::SessionService::currentSession(), masterTitleId);

    ORIGIN_VERIFY_CONNECT(resp, &NucleusEntitlementServiceResponse::success,
        this, &NucleusEntitlementController::onEntitlementRequestSuccess);
    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)),
        this, SLOT(onEntitlementRequestError()));
}

void NucleusEntitlementController::onEntitlementRequestSuccess()
{
    using namespace Origin::Services::Publishing;

    NucleusEntitlementServiceResponse* resp = dynamic_cast<NucleusEntitlementServiceResponse*>(sender());
    if (NULL == resp)
    {
        return;
    }

    int entitlementDelta = (UnknownInitializationSource == initializationSource()) ? -1 : m_entitlements.count();
    updateEntitlements(resp->entitlements(), FromServer);

    QSet<quint64> entitlementIds;
    for (auto entitlementIter = resp->entitlements().begin(); entitlementIter != resp->entitlements().end(); ++entitlementIter)
    {
        entitlementIds.insert((*entitlementIter)->id());
    }

    const QString &parentMasterTitleId = resp->extraContentRequestedMasterTitleId();
    if (Services::Publishing::EntitlementRefreshTypeIncremental == resp->refreshType())
    {
        // Incremental refreshes can only add new entitlements, not delete them
        m_entitlementIdsByParentMasterTitleId[parentMasterTitleId].unite(entitlementIds);
    }
    else
    {
        // Figure out if any entitlements were missing from the response and delete them
        QSet<quint64> deletedEntitlements = m_entitlementIdsByParentMasterTitleId[parentMasterTitleId] - entitlementIds;

        m_entitlementIdsByParentMasterTitleId[parentMasterTitleId] = entitlementIds;
        for (auto entitlementIter = deletedEntitlements.begin(); entitlementIter != deletedEntitlements.end(); ++entitlementIter)
        {
            deleteEntitlement(*entitlementIter);
        }
    }
    emit entitlementsUpdated();

    entitlementDelta = (-1 == entitlementDelta) ? 0 : (m_entitlements.count() - entitlementDelta);
    logRefreshSuccessTelemetry(resp, entitlementDelta);
    m_writeUserCacheDataTimer.start();
    m_lastResponseTime = resp->currentServerTime();

    // Update the trusted clock with the server time.
    Origin::Services::TrustedClock::instance()->initialize(m_lastResponseTime);

    if (resp->refreshType() == Services::Publishing::EntitlementRefreshTypeExtraContent)
    {
        emit extraContentRefreshComplete(resp->extraContentRequestedMasterTitleId());
    }
    else
    {
        emit refreshComplete(resp->refreshType());
    }

    resp->deleteLater();
}

void NucleusEntitlementController::onEntitlementRequestError()
{
    Origin::Services::Publishing::NucleusEntitlementServiceResponse* resp = dynamic_cast<Origin::Services::Publishing::NucleusEntitlementServiceResponse*>(sender());
    if (NULL == resp)
    {
        return;
    }

    logRefreshFailureTelemetry(resp);

    if (UnknownInitializationSource == m_initializationSource)
    {
        m_initializationSource = ServerError;
        emit initialized();
    }

    if (Services::Publishing::EntitlementRefreshStatusSignatureInvalid == resp->refreshStatus())
    {
        emit entitlementSignatureVerificationFailed();
    }

    if (resp->refreshType() == Services::Publishing::EntitlementRefreshTypeExtraContent)
    {
        emit extraContentRefreshComplete(resp->extraContentRequestedMasterTitleId());
    }
    else
    {
        emit refreshComplete(resp->refreshType());
    }

    resp->deleteLater();
}

Origin::Services::Publishing::NucleusEntitlementRef NucleusEntitlementController::entitlementById(quint64 entitlementId)
{
    QReadLocker lock(&m_entitlementLock);

    for (auto it = m_entitlements.begin(); it != m_entitlements.end(); ++it)
    {
        Services::Publishing::NucleusEntitlementList& entList = it.value();
        foreach(Services::Publishing::NucleusEntitlementRef ent, entList)
        {
            if (ent->id() == entitlementId)
            {
                return ent;
            }
        }
    }

    // Return an invalid entitlement
    return Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
}

Origin::Services::Publishing::NucleusEntitlementRef NucleusEntitlementController::entitlementById(quint64 entitlementId, const QString& offerId)
{
    QReadLocker lock(&m_entitlementLock);

    if (m_entitlements.contains(offerId))
    {
        Services::Publishing::NucleusEntitlementList& entList = m_entitlements[offerId];
        foreach(Services::Publishing::NucleusEntitlementRef ent, entList)
        {
            if (ent->id() == entitlementId)
            {
                return ent;
            }
        }
    }

    // Return an invalid entitlement
    return Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
}

void NucleusEntitlementController::deleteEntitlement(quint64 entId)
{
    Services::Publishing::NucleusEntitlementRef oldBestEntitlement = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
    Services::Publishing::NucleusEntitlementRef newBestEntitlement = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;

    Services::Publishing::NucleusEntitlementRef deletedEntitlement = entitlementById(entId);
    if (deletedEntitlement != Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT)
    {
        QWriteLocker lock(&m_entitlementLock);

        Services::Publishing::NucleusEntitlementList& entList = m_entitlements[deletedEntitlement->productId()];
        oldBestEntitlement = entList.front();
        entList.removeAll(deletedEntitlement);

        if (entList.isEmpty())
        {
            m_entitlements.remove(deletedEntitlement->productId());
        }
        else
        {
            newBestEntitlement = entList.front();
        }

        ORIGIN_LOG_EVENT << "Detected that an entitlement was deleted (entitlementId: " << deletedEntitlement->id() << " offer: " << deletedEntitlement->productId() << ")";

        emit entitlementDeleted(deletedEntitlement);
    }

    if (newBestEntitlement != oldBestEntitlement)
    {
        emit entitlementBestMatchChanged(deletedEntitlement->productId(), newBestEntitlement);
    }
}

void NucleusEntitlementController::sortEntitlements(const QString& offerId)
{
    QWriteLocker lock(&m_entitlementLock);
    Services::Publishing::NucleusEntitlementList& entList = m_entitlements[offerId];
    qStableSort(entList.begin(), entList.end(), Services::Publishing::NucleusEntitlement::CompareEntitlements);
}

void NucleusEntitlementController::updateEntitlements(const QList<Origin::Services::Publishing::NucleusEntitlementRef>& newEntitlements, InitializationSource initializationSource)
{
    foreach(const Origin::Services::Publishing::NucleusEntitlementRef newEnt, newEntitlements)
    {
        const QString& offerId = newEnt->productId();

        if (newEnt->status() != Origin::Services::Publishing::DELETED)
        {
            Services::Publishing::NucleusEntitlementRef existingEnt = entitlementById(newEnt->id(), offerId);
            if (existingEnt == Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT)
            {
                QWriteLocker lock(&m_entitlementLock);
                m_entitlements[offerId] += newEnt;
                emit entitlementGranted(newEnt);
            }
            else
            {
                QWriteLocker lock(&m_entitlementLock);

                // Entitlement emits updated signal if the newEnt has newer lastModifiedDate than current
                existingEnt->updateEntitlement(newEnt);
            }

            sortEntitlements(offerId);

            if (newEnt == m_entitlements[offerId].front())
            {
                emit entitlementBestMatchChanged(offerId, newEnt);
            }
        }
        else
        {
            deleteEntitlement(newEnt->id());
        }

        // OFM-8565: Check for special Addon Preview offer which allows the user to see unreleased extracontent.
        const QString& addonPreviewId = Services::OriginConfigService::instance()->ecommerceConfig().addonPreviewProductId;
        if (!addonPreviewId.isEmpty() && addonPreviewId.compare(offerId, Qt::CaseInsensitive) == 0)
        {
            Services::writeSetting(Services::SETTING_AddonPreviewAllowed, newEnt->status() != Origin::Services::Publishing::DELETED, Services::Session::SessionService::currentSession());
        }
    }

    // We only want to emit this once after the first batch of entitlements come through
    // and we don't care if it's cached or live at this point.  The rest of the system will
    // only get updates if the live entitlement has a newer last modified date than the cached
    // entitlement.
    if (UnknownInitializationSource == m_initializationSource)
    {
        m_initializationSource = initializationSource;
        emit initialized();
    }
    else if (ServerError == m_initializationSource)
    {
        m_initializationSource = initializationSource;
    }
}

void NucleusEntitlementController::serializeCacheData(QDataStream &cacheContentStream) 
{
    QReadLocker lock(&m_entitlementLock);

    cacheContentStream << m_entitlementIdsByParentMasterTitleId;

    foreach (const Services::Publishing::NucleusEntitlementList entList, m_entitlements)
    {
        foreach (const Services::Publishing::NucleusEntitlementRef ref, entList)
        {
            if (!Services::Publishing::NucleusEntitlement::serialize(cacheContentStream, ref))
            {
                ORIGIN_LOG_ERROR << "Failed to persist entitlement to cache [id: " << ((ref && ref->isValid()) ? QString::number(ref->id()) : "INVALID_ENTITLEMENT") << "]";
            }
        }
    }
}

void NucleusEntitlementController::deserializeCacheData(QDataStream &cacheContentStream) 
{
    TIME_BEGIN("NucleusEntitlementController::deserializeCacheData")

    Services::Publishing::NucleusEntitlementList cachedEntitlements;

    if (!cacheContentStream.atEnd())
    {
        cacheContentStream >> m_entitlementIdsByParentMasterTitleId;
    }

    while (!cacheContentStream.atEnd())
    {
        Services::Publishing::NucleusEntitlementRef ref;
        if (Services::Publishing::NucleusEntitlement::deserialize(cacheContentStream, ref))
        {
            cachedEntitlements.push_back(ref);
        }
    }

    if (!cachedEntitlements.empty())
    {
        updateEntitlements(cachedEntitlements, FromCache);
        emit entitlementsUpdated();
    }
    
    TIME_END("NucleusEntitlementController::deserializeCacheData")
}

} // namespace Content

} // namespace Engine

} // namespace Origin
