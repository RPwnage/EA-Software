//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#ifndef NUCLEUSENTITLEMENTCONTROLLER_H
#define NUCLEUSENTITLEMENTCONTROLLER_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QSharedPointer>
#include <QReadWriteLock>

#include "services/publishing/NucleusEntitlement.h"
#include "services/publishing/NucleusEntitlementServiceResponse.h"
#include "services/session/SessionService.h"
#include "UserSpecificContentCache.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

class NucleusEntitlementServiceResponse;

}

}

namespace Engine
{

namespace Content
{

class NucleusEntitlementQuery : public QObject
{
    Q_OBJECT

public:
    NucleusEntitlementQuery(quint64 entitlementId, Origin::Services::Publishing::NucleusEntitlementServiceResponse* lookup, Origin::Services::Publishing::NucleusEntitlementRef currentEnt);

    const quint64 entitlementId() { return m_entitlementId; }
    Origin::Services::Publishing::NucleusEntitlementRef entitlement() { return m_entitlement; }

    Origin::Services::restError error() const { return m_error; }
    Origin::Services::HttpStatusCode httpStatus() const { return m_httpStatus; }

signals:
    void queryCompleted();

private slots:
    void lookupFinished();

private:
    quint64 m_entitlementId;
    Origin::Services::Publishing::NucleusEntitlementRef m_entitlement;
    Origin::Services::restError m_error;
    Origin::Services::HttpStatusCode m_httpStatus;
};

/// \class NucleusEntitlementController
/// \brief TBD.
class NucleusEntitlementController : public UserSpecificContentCache
{
    Q_OBJECT

public:
    enum InitializationSource { UnknownInitializationSource, FromCache, FromServer, ServerError };

public:
    /// \brief Creates a new nucleus entitlement controller.
    ///
    /// \param  user  The userId whose entitlements we wish to control. 
    /// \returns ContentController Returns a pointer to the newly created nucleus entitlement controller.
    NucleusEntitlementController();

    /// \brief The NucleusEntitlementController destructor; releases the resources used by the NucleusEntitlementController.
    virtual ~NucleusEntitlementController();

    static void init();

    InitializationSource initializationSource() const { return m_initializationSource; }
    int numberOfEntitlements() const;

    QDateTime lastResponseTime() const;

    NucleusEntitlementQuery* refreshSpecificEntitlement(quint64 entitlementId);

    Origin::Services::Publishing::NucleusEntitlementRef entitlementById(quint64 entitlementId);
    Origin::Services::Publishing::NucleusEntitlementRef entitlementById(quint64 entitlementId, const QString& offerId);

    Services::Publishing::NucleusEntitlementList entitlements() const;
    Services::Publishing::NucleusEntitlementList entitlements(const QString& offerId) const;

    Q_INVOKABLE void deleteEntitlement(quint64 entitlementId);

signals:
    void initialized();

    void refreshComplete(Origin::Services::Publishing::EntitlementRefreshType type);
    void extraContentRefreshComplete(QString masterTitleId);

    /// \brief Emits a signal when entitlement signatures fail to verify.
    void entitlementSignatureVerificationFailed();

    void entitlementGranted(Origin::Services::Publishing::NucleusEntitlementRef ent);
    void entitlementDeleted(Origin::Services::Publishing::NucleusEntitlementRef ent);

    void entitlementBestMatchChanged(const QString& offerId, Origin::Services::Publishing::NucleusEntitlementRef newBestMatch);
    void entitlementsUpdated();

private slots:
    void onEntitlementRequestSuccess();
    void onEntitlementRequestError();

protected:
    virtual void serializeCacheData(QDataStream &stream);
    virtual void deserializeCacheData(QDataStream &stream);

private:
    /// \enum EntitlementRefreshMode
    enum EntitlementRefreshMode
    {
        ContentUpdates, ///< Only entitlements modified since the last updated time will be fetched.
        BaseGamesOnly,  ///< Clear local cache and refetch entitlements.
        CacheOnly       ///< Re-read all entitlement data available in the offline caches, no network activity necessary.
    };

    Q_INVOKABLE void initUserEntitlements();
    Q_INVOKABLE void refreshUserEntitlements(Origin::Engine::Content::NucleusEntitlementController::EntitlementRefreshMode mode);

    // Because nucleus entitlement controller doesn't really know which entitlements are base game entitlements, it is up to
    // content controller to tell it what to query for after it loads the base game definitions from the CatalogDefinitionController.
    Q_INVOKABLE void refreshExtraContentEntitlements(const QString& masterTitleId);

    InitializationSource m_initializationSource;

    void updateEntitlements(const QList<Origin::Services::Publishing::NucleusEntitlementRef>& newEntitlements, InitializationSource initializationSource);
    void sortEntitlements(const QString& offerId);

    mutable QReadWriteLock m_entitlementLock;

    QDateTime m_lastResponseTime;

    // All user entitlements
    QHash<QString, Services::Publishing::NucleusEntitlementList> m_entitlements;
    QHash<QString, QSet<quint64> > m_entitlementIdsByParentMasterTitleId;
    QTimer m_writeUserCacheDataTimer;

    friend class ContentController;
    friend class BaseGamesController;
};

} // namespace Content

} // namespace Engine

} // namespace Origin

#endif
