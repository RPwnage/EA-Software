//  ShiftQueryProxy.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef _SHIFT_QUERY_PROXY_H_
#define _SHIFT_QUERY_PROXY_H_

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QString>

#include "ShiftDirectDownload/DcmtServiceResponses.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

namespace JsInterface
{

class ShiftQueryProxyResponse : public QObject
{
    Q_OBJECT

public:
    ShiftQueryProxyResponse(DcmtServiceResponse *resp);

    Q_INVOKABLE QString getError();
    Q_INVOKABLE QString getErrorCause() { return mDcmtResponse->getErrorCause(); }
    Q_INVOKABLE QString getErrorDescription() { return mDcmtResponse->getErrorDescription(); }

signals:
    void done(const QVariant &);
    void fail();
    void always();

protected slots:
    void onRequestComplete(QNetworkReply::NetworkError status);

protected:
    DcmtServiceResponse *mDcmtResponse;
};

class ShiftQueryProxy : public QObject
{
    Q_OBJECT

public:
    ShiftQueryProxy();
    virtual ~ShiftQueryProxy();

    Q_INVOKABLE void loginAsync();
    Q_INVOKABLE void logoutAsync();
    Q_INVOKABLE QObject* authenticate(const QString &userName, const QString &password);
    Q_INVOKABLE QObject* getAvailableBuilds(const QString &offerId,
        const QString &buildType = QString(), int pageNumber = 0, int pageSize = 0);
    Q_INVOKABLE QObject* placeDeliveryRequest(const QString &offerId, const QString &buildId);
    Q_INVOKABLE QObject* getDeliveryStatus(int maxResults);
    Q_INVOKABLE QObject* generateDownloadUrl(const QString &buildId);

    Q_INVOKABLE void reloadOverrides();

    Q_INVOKABLE void closeLoginWindow();

signals:
    void deliveryStatusUpdated(const QVariant &);
    void deliveryRequestPlaced(const QVariant &);

private slots:
    void onDeliveryStatusUpdated(DeliveryStatusResponse *);
    void onDeliveryRequestPlaced(DeliveryRequestResponse *);

private:
    QObject *prepareResponse(DcmtServiceResponse *resp);
};

} // namespace JsInterface

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // _SHIFT_QUERY_PROXY_H_
