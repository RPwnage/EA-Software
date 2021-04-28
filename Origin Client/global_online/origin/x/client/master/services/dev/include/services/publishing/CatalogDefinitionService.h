//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#ifndef CATALOGDEFINITIONSERVICE_H
#define CATALOGDEFINITIONSERVICE_H

#include "services/publishing/CatalogDefinition.h"
#include "services/rest/OriginAuthServiceClient.h"

#include <QObject>
#include <QString>

namespace Origin
{

namespace Services
{

class OriginAuthServiceResponse;

namespace Publishing
{

class CatalogDefinitionServiceClient;

class ORIGIN_PLUGIN_API CatalogDefinitionLookup : public QObject
{
    friend class CatalogDefinitionServiceClient;
    Q_OBJECT

private:
    CatalogDefinitionLookup(const CatalogDefinitionServiceClient *parent, const QString& offerId,
        const QString &qualifyingOfferId, const QByteArray &lastEtag, qint64 batchTime, bool elevatedPermissions);

public:
    virtual ~CatalogDefinitionLookup();

    const QByteArray &responseBytes() const { return m_responseBytes; }
    const QByteArray &responseSignature() const { return m_responseSignature; }
    DataSource dataSource() const { return m_dataSource; }
    const QString &offerId() const { return m_offerId; }

signals:
    void succeeded(Origin::Services::Publishing::CatalogDefinitionRef catalogDef);
    void unchanged();
    void failed();
    void aborted();
    void confidential();
    void finished();

public slots:
    void start();
    void startPrivateLookup();
    void abort();

protected slots:
    void onLookupFinished();

private:
    void processReply(QNetworkReply *reply);

private:
    const CatalogDefinitionServiceClient *m_parent;
    OriginAuthServiceResponse* m_reply;
    QString m_offerId;
    QString m_qualifyingOfferId;
    QString m_clientLocale;
    QByteArray m_lastEtag;
    qint64 m_batchTime;
    DataSource m_dataSource;
    bool m_elevatedPermissions;
    bool m_aborted;
    Session::SessionRef m_session;
    QByteArray m_responseBytes;
    QByteArray m_responseSignature;
};


class ORIGIN_PLUGIN_API OfferUpdatedDateQuery : public QObject
{
    friend class CatalogDefinitionServiceClient;
    Q_OBJECT

private:
    OfferUpdatedDateQuery(const CatalogDefinitionServiceClient *serviceClient, const QStringList &offerIds);

public:
    virtual ~OfferUpdatedDateQuery();

    const QHash<QString, qint64> &offerUpdatedDates() const { return m_offerUpdatedDates; }

signals:
    void finished();

public slots:
    void start();
    void abort();

protected slots:
    void queryFinished();

private:
    const CatalogDefinitionServiceClient *m_serviceClient;
    OriginAuthServiceResponse* m_reply;
    QStringList m_offerIds;
    QHash<QString, qint64> m_offerUpdatedDates;
};


class ORIGIN_PLUGIN_API CatalogDefinitionServiceClient : public OriginAuthServiceClient
{
public:
    CatalogDefinitionServiceClient();
    virtual ~CatalogDefinitionServiceClient();

    CatalogDefinitionLookup *lookupCatalogDefinition(const QString &offerId, const QString &qualifyingOfferId,
        const QByteArray &lastEtag, qint64 batchTime, bool elevatedPermissions) const;

    OfferUpdatedDateQuery *queryOfferUpdatedDates(const QStringList &offerIds);

protected:
    virtual NetworkAccessManager* networkAccessManager() const;
};

} // namespace Publishing

} // namespace Services

} // namespace Origin 

#endif
