///////////////////////////////////////////////////////////////////////////////
// StoreServiceClient.cpp
//
// Author: Jonathan Kolb
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QLocale>

#include "services/rest/StoreServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"

#include "services/log/LogService.h"

namespace Origin
{

    namespace Services
    {

        StoreServiceClient::StoreServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginServiceClient(nam)
        {
            setBaseUrl(baseUrl.isEmpty() ? QUrl(readSetting(SETTING_SubscriptionRedemptionURL)) : baseUrl);
        }

        RedeemSubscriptionOfferResponse *StoreServiceClient::redeemSubscriptionOffer(const QString &subscriptionId, const QString &offerId, const QString &transactionId)
        {
            Session::SessionRef session = Session::SessionService::currentSession();
            QString redemptionUrl = QString("/%1/%2/%3").arg(
                                        Session::SessionService::nucleusUser(session), subscriptionId, offerId);

            QUrl url = urlForServicePath(redemptionUrl);
            
            QUrlQuery query(url);
            query.addQueryItem("type", "grant");
            query.addQueryItem("authToken", sessionToken(session));

            url.setQuery(query);

            ORIGIN_LOG_DEBUG << "Redemption URL: " << url.toDisplayString();

            QNetworkRequest request(url);
            request.setRawHeader("skip-client-check", "true");

            return new RedeemSubscriptionOfferResponse(getNonAuth(request), offerId, transactionId);
        }

        RedeemSubscriptionOfferResponse *StoreServiceClient::revokeSubscriptionOffer(const QString &subscriptionId, const QString &offerId, const QString &transactionId)
        {
            Session::SessionRef session = Session::SessionService::currentSession();

            QString redemptionUrl = QString("/%1/%2/%3").arg(
                Session::SessionService::nucleusUser(session), subscriptionId, offerId);

            QUrl url = urlForServicePath(redemptionUrl);
            QUrlQuery query(url);
            query.addQueryItem("type", "revoke");
            query.addQueryItem("authToken", sessionToken(session));

            url.setQuery(query);

            ORIGIN_LOG_DEBUG << "Redemption URL: " << url.toDisplayString();

            QNetworkRequest request(url);
            request.setRawHeader("skip-client-check", "true");

            return new RedeemSubscriptionOfferResponse(getNonAuth(request), offerId, transactionId);
        }

        Services::RedeemSubscriptionOfferResponse * StoreServiceClient::redeemSubscriptionOfferByUrl(Services::RedeemSubscriptionOfferResponse * response)
        {
            return new RedeemSubscriptionOfferResponse(postNonAuth(QNetworkRequest(response->reply()->attribute(QNetworkRequest::RedirectionTargetAttribute).toString()), ""),
                    response->offerId(),
                    response->transactionId());
        }

        Services::RedeemSubscriptionOfferResponse * StoreServiceClient::revokeSubscriptionOfferByUrl(Services::RedeemSubscriptionOfferResponse * response)
        {
            return new RedeemSubscriptionOfferResponse(postNonAuth(QNetworkRequest(response->reply()->attribute(QNetworkRequest::RedirectionTargetAttribute).toString()), ""),
                                                       response->offerId(),
                                                       response->transactionId());
        }

    } // namespace Services

} // namespace Origin
