//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#ifndef BASEGAMESCONTROLLER_H
#define BASEGAMESCONTROLLER_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QSharedPointer>
#include <QReadWriteLock>
#include <QVector>

#include "engine/content/Entitlement.h"
#include "engine/content/NucleusEntitlementController.h"
#include "engine/content/CatalogDefinitionController.h"
#include "services/common/Accessor.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

/// \enum ContentRefreshMode
/// \brief TBD.
enum ContentRefreshMode
{
    ContentUpdates, ///< Only entitlements modified since the last updated time will be fetched.
    RenewAll,       ///< Clear local cache and refetch entitlements.
    UserRefresh,    ///< Same as RenewAll but has throttling applied.
    CacheOnly,      ///< Re-read all entitlement data available in the offline caches, no network activity necessary.
};

/// \class BaseGamesController
/// \brief TBD.
class BaseGamesController : public QObject
{
    Q_OBJECT

public:
    /// \brief Creates a new base game controller.
    ///
    /// \param  entController  Nucleus entitlement controller to listen to for base game entitlement updates 
    /// \returns BaseGamesController Returns a pointer to the newly created base games controller.
    BaseGamesController(ContentController *parent);
    virtual ~BaseGamesController();

    int numberOfGames() const;
    BaseGameSet baseGames() const;
    BaseGameSet baseGamesByChild(EntitlementRef child) const;

    bool refreshInProgress() const;

    const QDateTime& lastFullRefresh() const { return m_lastFullRefresh; }

public slots:
    /// \brief Refreshes the user's games.
    void refresh(Origin::Engine::Content::ContentRefreshMode mode);
    void onDirtyBitsEntitlementUpdate(const QByteArray &dbPayload);
    void processDeferredExtraContent();

signals:
    /// \brief Signals that the initial loading of all base game definitions is complete
    void baseGamesLoaded();

    /// \brief Signals that the requested entitlement refresh is complete
    void refreshComplete();

    void baseGameAdded(Origin::Engine::Content::BaseGameRef baseGame);
    void baseGameRemoved(Origin::Engine::Content::BaseGameRef baseGame);
    void baseGameExtraContentUpdated(Origin::Engine::Content::BaseGameRef baseGame);

private slots:
    void onEntitlementsInitialized();

    void onEntitlementBestMatchChanged(const QString& offerId, Origin::Services::Publishing::NucleusEntitlementRef newBestMatch);
    void onEntitlementsUpdated();

    void onEntitlementRefreshComplete(Origin::Services::Publishing::EntitlementRefreshType type);
    void onExtraContentRefreshComplete(QString masterTitleId);

    void onCatalogDefinitionQueryComplete();
    void onContentConfigurationChanged();

    void retryFailedEntitlementRequests();

    void onDirtyBitsRefreshTimer();

    void queryOfferUpdatedDates();
    void onOfferUpdatedDateQueryComplete();

    void onUserAboutToLogout();

private:
    enum { MaxOfferUpdatedDateIds = 20 };
    enum { OfferUpdatedDateTimerDelay = 1000 };
    enum State
    {
        kInitializing = 0,
        kRefreshing,
        kIdle,
        kShuttingDown
    };

    void retrieveOwnedCatalogDefinition(Origin::Services::Publishing::NucleusEntitlementRef ent);
    void retrieveUnownedCatalogDefinition(const QString &productId);
    Q_INVOKABLE void retrieveProductCatalogDefinition(QString productId,
        qint64 batchTime = Services::Publishing::INVALID_BATCH_TIME, bool fetchMissingBatchTime = true, bool elevatedPermissions = false);
    void onCatalogDefinitionLoaded(Origin::Services::Publishing::CatalogDefinitionRef def);
    void onCatalogDefinitionNotFound(QString catalogOfferId, bool confidential);
    Q_INVOKABLE void checkInitializationComplete();
    void checkRefreshComplete();
    void processSuppressions();
    void addExtraContent(EntitlementRef content);
    void disownExtraContent(EntitlementRef content);
    void deleteExtraContent(EntitlementRef content);
    void removePendingExtraContent(const QString &offerId);

    Q_INVOKABLE void onEntitlementGranted(Origin::Services::Publishing::NucleusEntitlementRef ent);
    void onEntitlementDeleted(const QString& productId);

    bool m_baseGamesInitialized;
    bool m_extraContentInitialized;
    bool m_entitlementControllerInitialized;

    ACCESSOR(NucleusEntitlementController, nucleusEntitlementController);

    typedef QHash<QString, Services::Publishing::NucleusEntitlementRef> NucleusEntitlementHash;
    NucleusEntitlementHash m_queuedEntitlementLookups;
    NucleusEntitlementHash m_pendingEntitlementLookups;
    NucleusEntitlementHash m_deferredExtraContentLookups;
    QSet<QString> m_deferredCatalogDefinitionUpdates;
    QSet<CatalogDefinitionQuery*> m_pendingCatalogDefinitionUpdates;
    QSet<Services::Publishing::OfferUpdatedDateQuery*> m_pendingOfferUpdatedDateQueries;
    NucleusEntitlementHash m_failedEntitlementLookups;
    QSet<QString> m_initialBaseGameIds;

    mutable QReadWriteLock m_baseGamesLock;
    bool m_pendingEntitlementRefresh;
    QHash<QString, BaseGameRef> m_baseGames;
    QHash<QString, BaseGameSet> m_baseGamesByMasterTitleId;

    mutable QReadWriteLock m_extraContentLock;
    QSet<QString> m_pendingExtraContentRefreshes;
    QSet<QString> m_deferredExtraContentRefreshes;
    QHash<QString, EntitlementRef> m_extraContent;
    QHash<QString, EntitlementRefSet> m_extraContentByParentMasterTitleId;
    QHash<QString, QSet<QString>> m_pendingExtraContentByParentMasterTitleId;
    QHash<QString, QString> m_privateEligibleOfferIds;
    QSet<QString> m_masterTitlesWithExtraContent;

    QStringList m_offersMissingBatchTimes;
    QTimer m_queryOfferUpdatedDatesTimer;

    QTimer m_dirtyBitsRefreshTimer;
    ContentRefreshMode m_dirtyBitsRefreshMode;

    QSet<QString> m_suppressedContent;
    QTimer m_failedEntitlementRetryTimer;
    QDateTime m_lastFullRefresh;

    volatile State m_state;

    static QThread sm_workerThread;
};

} // namespace Content

} // namespace Engine

} // namespace Origin

#endif
