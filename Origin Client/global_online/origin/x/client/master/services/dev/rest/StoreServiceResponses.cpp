///////////////////////////////////////////////////////////////////////////////
// StoreServiceResponse.cpp
//
// Author: Jonathan Kolb
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/StoreServiceResponses.h"
#include "services/debug/DebugService.h"

#include <QDomDocument>
#include <NodeDocument.h>
#include <ReaderCommon.h>
#include "serverReader.h"

namespace Origin
{

namespace Services
{

static const int STORE_REQUEST_TIMEOUT_MSEC = 30000;

RedeemSubscriptionOfferResponse::RedeemSubscriptionOfferResponse(QNetworkReply* reply, const QString &offerId, const QString &transactionId)
    : OriginServiceResponse(reply)
    , mOfferId(offerId)
    , mTransactionId(transactionId)
{
    ORIGIN_VERIFY_CONNECT(&mTimeout, &QTimer::timeout, this, &HttpServiceResponse::abort);

    mTimeout.setSingleShot(true);
    mTimeout.start(STORE_REQUEST_TIMEOUT_MSEC);
}

bool RedeemSubscriptionOfferResponse::parseSuccessBody(QIODevice *body)
{
    QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

    mTimeout.stop();

    QByteArray data = body->readAll();
    if (pDoc->Parse(data.data()))
    {
        server::Read(pDoc.data(), mEntitlements);
        return true;
    }

    return false;
}

} // namespace Services

} // namespace Origin
