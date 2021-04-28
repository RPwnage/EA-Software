//    Copyright (c) 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#ifndef CATALOGDEFINITIONCONTROLLER_H
#define CATALOGDEFINITIONCONTROLLER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QMutex>

#include "services/publishing/CatalogDefinitionService.h"
#include "engine/content/UserSpecificContentCache.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

class ORIGIN_PLUGIN_API CatalogDefinitionQuery : public QObject
{
    Q_OBJECT

public:
    CatalogDefinitionQuery(const QString& offerId, Origin::Services::Publishing::CatalogDefinitionLookup* lookup, Origin::Services::Publishing::CatalogDefinitionRef currentDef);

    Q_INVOKABLE void abort();
    const QString& offerId() { return m_offerId; }
    Origin::Services::Publishing::CatalogDefinitionRef definition() { return m_definition; }
    const bool confidential() { return m_confidential; }

signals:
    void catalogDefinition(Origin::Services::Publishing::CatalogDefinitionRef def);
    void catalogDefinitionNotFound(QString offerId);
    void finished();

private slots:
    void onDefLookupUpdated(Origin::Services::Publishing::CatalogDefinitionRef catalogDef);
    void onDefLookupUnchanged();
    void onDefLookupFailed();
    void onDefLookupAborted();
    void onDefLookupConfidential();

private:
    Origin::Services::Publishing::CatalogDefinitionLookup* m_lookup;
    QString m_offerId;
    Origin::Services::Publishing::CatalogDefinitionRef m_definition;
    bool m_confidential;
};


/// \class CatalogDefinitionController
/// \brief TBD.
class ORIGIN_PLUGIN_API CatalogDefinitionController : public UserSpecificContentCache
{
    Q_OBJECT

public:
    enum RefreshType
    {
        RefreshAll,
        RefreshStale
    };

    static void init();
    static CatalogDefinitionController* instance();
    static void release();

#ifdef _DEBUG
    static bool ignoreOfferViaIsolateProductIds(const QString& offerId);
#endif

    void refreshDefinitions(RefreshType refreshType = RefreshStale);
    bool refreshInProgress() const;

    CatalogDefinitionQuery* queryCatalogDefinition(const QString& offerId,
        const QString &qualifyingOfferId = "",
        qint64 batchTime = Services::Publishing::INVALID_BATCH_TIME,
        RefreshType refreshType = RefreshStale,
        bool elevatedPermissions = false);

    bool hasCachedCatalogDefinition(const QString& offerId);
    Origin::Services::Publishing::CatalogDefinitionRef getCachedCatalogDefinition(const QString& offerId);

    Services::Publishing::OfferUpdatedDateQuery *queryOfferUpdatedDates(const QStringList &offerIds);

signals:
    void refreshComplete();

public slots:
    void onDirtyBitsCatalogUpdateNotification(const QByteArray &payload);

private slots:
    void onLookupSucceeded(Origin::Services::Publishing::CatalogDefinitionRef catalogDef);
    void onLookupUnchanged();
    void onLookupFailed();
    void onLookupConfidential();
    void onLookupComplete();
    void onRefreshDefinitionTimerFired();
    void onDirtyBitsTelemetryDelayTimeout();
    void onCatalogDefinitionLookupTelemetryTimeout();
    void onEndSessionComplete(Origin::Services::Session::SessionError error);

protected:
    virtual void serializeCacheData(QDataStream &stream);
    virtual void deserializeCacheData(QDataStream &stream);

private:
    static CatalogDefinitionController* sInstance;

    CatalogDefinitionController();
    ~CatalogDefinitionController();

    void startup();
    void shutdown();

    QString definitionCacheFile(const QString& productId);

    void writeDefinitionCache(const Origin::Services::Publishing::CatalogDefinitionRef &def);

    Services::Publishing::CatalogDefinitionLookup *lookupCatalogDefinition(const QString &offerId,
        const QString &qualifyingOfferId, const QByteArray &eTag, qint64 batchTime, bool elevatedPermissions);
    Origin::Services::Publishing::CatalogDefinitionRef readDefinitionCache(const QString& offerId);

    Services::Publishing::CatalogDefinitionServiceClient m_catalogDefinitionService;

    QMutex m_catalogDefinitionLock;
    QHash<QString, Origin::Services::Publishing::CatalogDefinitionRef> m_catalogDefinitions;
    QSet<QString> m_pendingLookups;
    bool m_refreshRequested;
    QTimer m_catalogDefinitionRefreshTimer;

    QString m_definitionCacheFolder;

    class DirtyBitsCatalogUpdate
    {
    public:
        enum UpdateType
        {
            MasterTitleUpdate,
            OfferDefinitionUpdate,
        };

    public:
        DirtyBitsCatalogUpdate(const QString &id, qint64 changeBatchTimestamp, UpdateType updateType);

        QString id;
        qint64 changeBatchTimestamp;
        UpdateType updateType;
    };

    QList<DirtyBitsCatalogUpdate> m_catalogUpdatesTelemetryData;
    QTimer m_dirtyBitsTelemetryDelayTimer;

    qint64 m_lookupSuccessCount;
    qint64 m_lookupFailureCount;
    qint64 m_lookupConfidentialCount;
    qint64 m_lookupUnchangedCount;
    QTimer m_catalogDefLookupTelemetryTimer;
    QTimer m_writeUserCacheDataTimer;
};

} // namespace Content

} // namespace Engine

} // namespace Origin

#endif
