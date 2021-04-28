// TODO RENAME \global_online\origin\client\services\dev\include\services\entitlement\raw\EntitlementServiceClient.h
// TODO NAMESPACE Origin::Services::Entitlements::Raw

///////////////////////////////////////////////////////////////////////////////
// EntitlementServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ENTITLEMENTSERVICECLIENT_H
#define _ENTITLEMENTSERVICECLIENT_H

#include <limits>
#include <QDateTime>
#include "services/publishing/ECommerceServiceClient.h"
#include "services/publishing/NucleusEntitlementServiceResponse.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

///
/// \brief HTTP service client for the user's entitlements
///
class NucleusEntitlementServiceClient : public ECommerceServiceClient
{
public:
    friend class OriginClientFactory<NucleusEntitlementServiceClient>;

    ///
    /// \brief Returns Entitlements for the user
    ///
    static NucleusEntitlementServiceResponse *entitlementInfo(Session::SessionRef session, quint64 entitlementId)
    {
        return OriginClientFactory<NucleusEntitlementServiceClient>::instance()->entitlementInfoPriv(session, entitlementId);
    }
    
    ///
    /// \brief Returns Entitlements for the user
    ///
    static NucleusEntitlementServiceResponse *baseGames(Session::SessionRef session)
    {
        return OriginClientFactory<NucleusEntitlementServiceClient>::instance()->baseGamesPriv(session);
    }

    ///
    /// \brief Returns Entitlements for the user
    ///
    static NucleusEntitlementServiceResponse *extraContent(Session::SessionRef session, const QString& masterTitleId)
    {
        return OriginClientFactory<NucleusEntitlementServiceClient>::instance()->extraContentPriv(session, masterTitleId);
    }

    ///
    /// \brief Returns Entitlements for the user
    ///
    static NucleusEntitlementServiceResponse *contentUpdates(Session::SessionRef session, const QDateTime& lastModifiedDate = QDateTime())
    {
        return OriginClientFactory<NucleusEntitlementServiceClient>::instance()->contentUpdatesPriv(session, lastModifiedDate);
    }

    ///
    /// \brief Returns Entitlements for the user
    ///
    static NucleusEntitlementServiceResponse *sdkEntitlements(Session::SessionRef session, const QStringList& filterGroups, const QStringList& filterOffers, const QStringList& filterItems, const QStringList& filterEntitlementIds, const bool includeChildGroups)
    {
        return OriginClientFactory<NucleusEntitlementServiceClient>::instance()->sdkEntitlementsPriv(session, filterGroups, filterOffers, filterItems, filterEntitlementIds, includeChildGroups);
    }

private:
    /// 
    /// \brief Creates a new entitlements service client
    ///
    /// \param nam           QNetworkAccessManager instance to send requests through.
    ///
    explicit NucleusEntitlementServiceClient(NetworkAccessManager *nam = NULL);

    ///
    /// \brief Returns Consolidated Entitlements for the user
    ///
    NucleusEntitlementServiceResponse* entitlementInfoPriv(Session::SessionRef session, quint64 entitlementId);

    ///
    /// \brief Returns Base game entitlements for the user
    ///
    NucleusEntitlementServiceResponse* baseGamesPriv(Session::SessionRef session);

    ///
    /// \brief Returns Extra content for the user for the given master title id, both owned and unowned
    ///
    NucleusEntitlementServiceResponse* extraContentPriv(Session::SessionRef session, const QString& masterTitleId);

    ///
    /// \brief Returns Any content that has been updated since lastModifiedDate for the user
    ///
    NucleusEntitlementServiceResponse* contentUpdatesPriv(Session::SessionRef session, const QDateTime& lastModifiedDate = QDateTime());

    ///
    /// \brief Returns SDK-visible entitlements for the user
    ///
    NucleusEntitlementServiceResponse* sdkEntitlementsPriv(Session::SessionRef session, const QStringList& filterGroups, const QStringList& filterOffers, const QStringList& filterItems, const QStringList& filterEntitlementIds, const bool includeChildGroups);
};

}

}

}

#endif //_CHATSERVICECLIENT_H