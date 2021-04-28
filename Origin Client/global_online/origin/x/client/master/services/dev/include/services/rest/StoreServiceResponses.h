///////////////////////////////////////////////////////////////////////////////
// StoreServiceResponse.h
//
// Author: Jonathan Kolb
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __STORESERVICERESPONSE_H__
#define __STORESERVICERESPONSE_H__

#include "services/rest/OriginServiceResponse.h"
#include "services/plugin/PluginAPI.h"
#include "server.h"

namespace Origin
{

namespace Services
{

class ORIGIN_PLUGIN_API RedeemSubscriptionOfferResponse : public OriginServiceResponse
{
    Q_OBJECT

public:
    explicit RedeemSubscriptionOfferResponse(QNetworkReply *reply, const QString &offerId, const QString &transactionId);
    const QString &offerId() const { return mOfferId; }
    const QString &transactionId() const { return mTransactionId; }
    const server::RedemptionResponseT &entilements(){ return mEntitlements; }

protected:
    virtual bool parseSuccessBody(QIODevice *);

private:
    QString mOfferId;
    QString mTransactionId;
    server::RedemptionResponseT mEntitlements;
    QTimer mTimeout;
};

} // namespace Services

} // namespace Origin

#endif // __STORESERVICERESPONSE_H__
