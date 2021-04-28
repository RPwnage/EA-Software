#include "services/debug/DebugService.h"

#include "ShiftDirectDownload/DcmtDownloadUrlResponse.h"
#include "ShiftDirectDownload/DcmtServiceClient.h"
#include "ShiftDirectDownload/DcmtServiceResponses.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

DcmtDownloadUrlResponse::DcmtDownloadUrlResponse(const QString &offerId,
        const QString &shiftBuildId, Services::Session::SessionRef session)
    : Services::Publishing::DownloadUrlResponse(Services::AuthNetworkRequest(nullptr, session), false)
    , mOfferId(offerId)
    , mShiftBuildId(shiftBuildId)
    , mDisconnectDeliveryStatus(false)
{
    connectDeliveryStatus();
    generateDownloadUrl();
}


DcmtDownloadUrlResponse::~DcmtDownloadUrlResponse()
{
    disconnectDeliveryStatus();
}


void DcmtDownloadUrlResponse::generateDownloadUrl()
{
    GenerateUrlResponse *next = DcmtServiceClient::instance()->generateDownloadUrl(mShiftBuildId);
    ORIGIN_VERIFY_CONNECT(next, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onDownloadUrl(QNetworkReply::NetworkError)));
}


void DcmtDownloadUrlResponse::onDownloadUrl(QNetworkReply::NetworkError status)
{
    GenerateUrlResponse *response = dynamic_cast<GenerateUrlResponse *>(sender());

    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onDownloadUrl(QNetworkReply::NetworkError)));

    if (QNetworkReply::NoError == status)
    {
        const DeliveryStatusInfo &info = response->getDeliveryStatusInfo();

        if (info.downloadUrl.length() > 0)
        {
            // EA_TODO("jonkolb", "2014/03/24", "Will we ever have sync url data from server?")

            m_downloadUrl.mOfferId = mOfferId;
            m_downloadUrl.mURL = info.downloadUrl;
            m_downloadUrl.mSyncURL = "";
            m_downloadUrl.mValidStartTime = response->getServerTime();
            m_downloadUrl.mValidEndTime = m_downloadUrl.mValidStartTime.addSecs(URL_VALID_SEC);

            finish(Services::restErrorSuccess);
        }
        else if ("INPROGRESS" != info.status)
        {
            placeDeliveryRequest();
        }
    }
    else if (QNetworkReply::ContentNotFoundError == status)
    {
        placeDeliveryRequest();
    }
    else
    {
        // EA_TODO: Log an error, this shouldn't ever happen
        finish(Services::restErrorUnknown);
    }

    response->deleteLater();
}


void DcmtDownloadUrlResponse::onDeliveryRequest(QNetworkReply::NetworkError status)
{
    DeliveryRequestResponse *response = dynamic_cast<DeliveryRequestResponse *>(sender());
    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onDeliveryRequest(QNetworkReply::NetworkError)));

    if (DCMT_OK != response->getErrorCode() && DCMT_ERR_DUPLICATE_DELIVERY != response->getErrorCode())
    {
        finish(Services::restErrorUnknown);
    }

    response->deleteLater();
}


void DcmtDownloadUrlResponse::onDeliveryStatusUpdated(DeliveryStatusResponse *response)
{
    foreach (const DeliveryStatusInfo &status, response->getDeliveryStatusList())
    {
        if (status.buildId == mShiftBuildId)
        {
            if ("COMPLETED" == status.status)
            {
                generateDownloadUrl();
            }
            break;
        }
    }
}


void DcmtDownloadUrlResponse::placeDeliveryRequest()
{
    DeliveryRequestResponse *next = DcmtServiceClient::instance()->placeDeliveryRequest(mOfferId, mShiftBuildId);
    ORIGIN_VERIFY_CONNECT(next, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onDeliveryRequest(QNetworkReply::NetworkError)));

    emit requestedDelivery();
}


void DcmtDownloadUrlResponse::finish(Services::restError status)
{
    disconnectDeliveryStatus();
    setError(status);
    emit finished();
}


void DcmtDownloadUrlResponse::connectDeliveryStatus()
{
    if (!mDisconnectDeliveryStatus)
    {
        DcmtServiceClient::instance()->addDownload();
        ORIGIN_VERIFY_CONNECT(
            DcmtServiceClient::instance(), SIGNAL(deliveryStatusUpdated(DeliveryStatusResponse *)),
            this, SLOT(onDeliveryStatusUpdated(DeliveryStatusResponse *)));
        mDisconnectDeliveryStatus = true;
    }
}


void DcmtDownloadUrlResponse::disconnectDeliveryStatus()
{
    if (mDisconnectDeliveryStatus)
    {
        DcmtServiceClient::instance()->removeDownload();
        ORIGIN_VERIFY_DISCONNECT(
            DcmtServiceClient::instance(), SIGNAL(deliveryStatusUpdated(DeliveryStatusResponse *)),
            this, SLOT(onDeliveryStatusUpdated(DeliveryStatusResponse *)));
        mDisconnectDeliveryStatus = false;
    }
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
