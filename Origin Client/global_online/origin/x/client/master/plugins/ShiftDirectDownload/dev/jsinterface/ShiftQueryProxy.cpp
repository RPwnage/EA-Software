//  ShiftQueryProxy.cpp
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"

#include "ShiftDirectDownload/DcmtServiceClient.h"
#include "../widgets/ShiftLoginViewController.h"
#include "ShiftQueryProxy.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

namespace JsInterface
{

ShiftQueryProxy::ShiftQueryProxy()
{
    ORIGIN_VERIFY_CONNECT(
        DcmtServiceClient::instance(), SIGNAL(deliveryStatusUpdated(DeliveryStatusResponse *)),
        this, SLOT(onDeliveryStatusUpdated(DeliveryStatusResponse *)));

    ORIGIN_VERIFY_CONNECT(
        DcmtServiceClient::instance(), SIGNAL(deliveryRequestPlaced(DeliveryRequestResponse *)),
        this, SLOT(onDeliveryRequestPlaced(DeliveryRequestResponse *)));
}


ShiftQueryProxy::~ShiftQueryProxy()
{
    ORIGIN_VERIFY_DISCONNECT(
        DcmtServiceClient::instance(), SIGNAL(deliveryStatusUpdated(DeliveryStatusResponse *)),
        this, SLOT(onDeliveryStatusUpdated(DeliveryStatusResponse *)));

    ORIGIN_VERIFY_DISCONNECT(
        DcmtServiceClient::instance(), SIGNAL(deliveryRequestPlaced(DeliveryRequestResponse *)),
        this, SLOT(onDeliveryRequestPlaced(DeliveryRequestResponse *)));
}


QObject* ShiftQueryProxy::authenticate(const QString &userName, const QString &password)
{
    return prepareResponse(DcmtServiceClient::instance()->authenticate(userName, password));
}


QObject* ShiftQueryProxy::getAvailableBuilds(const QString &offerId,
    const QString &buildType, int pageNumber, int pageSize)
{
    return prepareResponse(DcmtServiceClient::instance()->getAvailableBuilds(offerId, buildType, pageNumber, pageSize));
}


QObject* ShiftQueryProxy::placeDeliveryRequest(const QString &offerId, const QString &buildId)
{
    return prepareResponse(DcmtServiceClient::instance()->placeDeliveryRequest(offerId, buildId));
}


QObject* ShiftQueryProxy::getDeliveryStatus(int maxResults)
{
    return prepareResponse(DcmtServiceClient::instance()->getDeliveryStatus(maxResults));
}


QObject* ShiftQueryProxy::generateDownloadUrl(const QString &buildId)
{
    return prepareResponse(DcmtServiceClient::instance()->generateDownloadUrl(buildId));
}


void ShiftQueryProxy::loginAsync()
{
    DcmtServiceClient::instance()->loginAsync();
}


void ShiftQueryProxy::logoutAsync()
{
    DcmtServiceClient::instance()->logoutAsync(true);
}


void ShiftQueryProxy::reloadOverrides()
{
    DcmtServiceClient::instance()->reloadOverrides();
}


void ShiftQueryProxy::closeLoginWindow()
{
    // Invoke on the next event loop or we will crash here!
    QMetaObject::invokeMethod(ShiftLoginViewController::instance(), "close", Qt::QueuedConnection);
}


void ShiftQueryProxy::onDeliveryStatusUpdated(DeliveryStatusResponse *response)
{
    emit deliveryStatusUpdated(response->toVariant());
}


void ShiftQueryProxy::onDeliveryRequestPlaced(DeliveryRequestResponse *response)
{
    emit deliveryRequestPlaced(response->toVariant());
}


QObject *ShiftQueryProxy::prepareResponse(DcmtServiceResponse *resp)
{
    return new ShiftQueryProxyResponse(resp);
}


ShiftQueryProxyResponse::ShiftQueryProxyResponse(DcmtServiceResponse *resp)
    : mDcmtResponse(resp)
{
    ORIGIN_VERIFY_CONNECT(
        resp, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onRequestComplete(QNetworkReply::NetworkError)));
}


QString ShiftQueryProxyResponse::getError()
{
    switch (mDcmtResponse->getErrorCode())
    {
    case DCMT_OK:
        return "OK";

    case DCMT_ERR_NOT_XML:
        return "NOT_XML";

    case DCMT_ERR_WRONG_ROOT:
        return "WRONG_ROOT";

    case DCMT_ERR_DUPLICATE_DELIVERY:
        return "DUPLICATE_DELIVERY";

    case DCMT_ERR_PERMISSION_DENIED:
        return "PERMISSION_DENIED";

    case DCMT_ERR_NOT_FOUND:
        return "NOT_FOUND";

    case DCMT_ERR_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}


void ShiftQueryProxyResponse::onRequestComplete(QNetworkReply::NetworkError status)
{
    DcmtServiceResponse *resp = dynamic_cast<DcmtServiceResponse *>(sender());

    if (resp && QNetworkReply::NoError == status)
    {
        emit done(resp->toVariant());
    }
    else
    {
        emit fail();
    }

    emit always();

    deleteLater();
}

} // namespace JsInterface

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
