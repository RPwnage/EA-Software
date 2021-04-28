//  DcmtDownloadUrlResponse.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef DCMTDOWNLOADURLRESPONSE_H
#define DCMTDOWNLOADURLRESPONSE_H

#include "services/publishing/DownloadUrlProviderManager.h"
#include "ShiftDirectDownload/DcmtServiceResponses.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

class DcmtDownloadUrlResponse : public Services::Publishing::DownloadUrlResponse
{
    Q_OBJECT

public:
    DcmtDownloadUrlResponse(const QString &offerId, const QString &shiftBuildId, Services::Session::SessionRef session);
    virtual ~DcmtDownloadUrlResponse();

signals:
    void requestedDelivery();

public slots:
    void generateDownloadUrl();
    void onDownloadUrl(QNetworkReply::NetworkError);
    void onDeliveryRequest(QNetworkReply::NetworkError);
    void onDeliveryStatusUpdated(DeliveryStatusResponse *);

private:
    void placeDeliveryRequest();
    void finish(Services::restError status);
    void connectDeliveryStatus();
    void disconnectDeliveryStatus();

private:
    enum { URL_VALID_SEC = 5 * 60 };

    QString mOfferId;
    QString mShiftBuildId;
    bool mDisconnectDeliveryStatus;
};

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // DCMTDOWNLOADURLRESPONSE_H
