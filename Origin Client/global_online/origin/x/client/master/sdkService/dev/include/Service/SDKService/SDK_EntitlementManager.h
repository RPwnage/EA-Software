//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.

#ifndef SDKENTITLEMENTMANAGER_H
#define SDKENTITLEMENTMANAGER_H

#include "lsx.h"
#include "services/publishing/NucleusEntitlement.h"
#include "services/publishing/NucleusEntitlementServiceResponse.h"
#include "services/session/SessionService.h"
#include "engine/content/UserSpecificContentCache.h"
#include "Service/BaseService.h"

#include <QObject>
#include <QStringList>
#include <QHash>

class QNetworkReply;

namespace Origin
{

namespace SDK
{

/// \class SDK_EntitlementManager
/// \brief Manages a superset of entitlements Origin knows about, and entitlements that are visible only to the game through SDK requests.
class SDK_EntitlementManager : public QObject
{
    Q_OBJECT

public:
    SDK_EntitlementManager();
    virtual ~SDK_EntitlementManager();

    template <typename T> struct QueryEntitlementsPayload
    {
        lsx::QueryEntitlementsT request;
        std::vector<lsx::EntitlementT> * entitlements;

        QStringList FilterCategories;
        QStringList FilterOffers;
        QStringList FilterItems;
        QStringList FilterGroups;

        ResponseWrapperEx<T *, SDK_EntitlementManager, int> * gatherWrapper;
    };

    void QueryEntitlements(Lsx::Request& request, Lsx::Response& response);
    void QueryEntitlements(lsx::QueryEntitlementsT r, Lsx::Response& response);

    template <typename T> void QueryEntitlementsImpl(QueryEntitlementsPayload<T> &payload, Lsx::Response &response);
    template <typename T> void QueryCachedEntitlements(QueryEntitlementsPayload<T> &payload, Lsx::Response &response);
    void GetProductEntitlements(uint64_t userId, QString productId, Lsx::Response &response);

    template <typename T> bool QueryEntitlementsProcessResponse(QueryEntitlementsPayload<T> &payload, Origin::Services::Publishing::NucleusEntitlementServiceResponse *serverReply, Lsx::Response &response);
    template <typename T> bool GatherResponse(T *& data, int * jobsPending, Lsx::Response & response);

signals:
    void entitlementGranted(Origin::Services::Publishing::NucleusEntitlementServiceResponse*);

protected:
    template <typename T> QString constructCacheKey(const QueryEntitlementsPayload<T> &payload) const;

private slots:
    void onUserLoggedIn();
    void onEntitlementGranted(const QByteArray payload);
    void GrantEntitlementsProcessResponse();
};

} // namespace SDK

} // namespace Origin

#endif
