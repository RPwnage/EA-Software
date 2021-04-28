//  DcmtServiceResponses.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef DCMTSERVICERESPONSES_H
#define DCMTSERVICERESPONSES_H

#include <qdom.h>

#include "services/rest/HttpServiceResponse.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

struct GameBuildInfo
{
    QString buildId;
    QString version;
    QString milestone;
    QStringList buildTypes;
    QString lifeCycleStatus;
    QString status;
    QStringList approvalStates;
};

typedef QList<GameBuildInfo> GameBuildInfoList;


struct DeliveryStatusInfo
{
    QString buildId;
    QString downloadUrl;
    QString status;
};

typedef QList<DeliveryStatusInfo> DeliveryStatusInfoList;

enum DcmtResponseStatus
{
    DCMT_OK = 0,

    DCMT_ERR_NOT_XML = 1,
    DCMT_ERR_WRONG_ROOT = 2,
    DCMT_ERR_NETWORK = 2,
    DCMT_ERR_DUPLICATE_DELIVERY = 3,
    DCMT_ERR_PERMISSION_DENIED = 4,
    DCMT_ERR_NOT_FOUND = 5,

    DCMT_ERR_UNKNOWN = 999
};


class DcmtServiceResponse : public Services::HttpServiceResponse
{
    Q_OBJECT

protected:
    DcmtServiceResponse(QNetworkReply *reply);
    virtual ~DcmtServiceResponse();

    QDomElement getResponseRoot(const QString &tag);
    bool parseDeliveryStatus(DeliveryStatusInfo &deliveryStatus, const QDomElement &node);

public:
    const QDateTime &getServerTime();
    DcmtResponseStatus getErrorCode() { return mErrorCode; }
    const QString &getErrorCause() { return mErrorCause; }
    const QString &getErrorDescription() { return mErrorDescription; }
    virtual QVariant toVariant() const = 0;

signals:
    void complete(QNetworkReply::NetworkError status);

public slots:
    void ignoreSslErrors(const QList<QSslError> &errors);

private:
    QDateTime mServerTime;
    DcmtResponseStatus mErrorCode;
    QString mErrorCause;
    QString mErrorDescription;
};


class AuthenticateResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    AuthenticateResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

    const QString &getAuthToken() { return mAuthToken; }
    const QDateTime &getTokenExpiration() { return mTokenExpiration; }

protected:
    virtual void processReply();

private:
    QString mAuthToken;
    QDateTime mTokenExpiration;
};


class ExpireTokenResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    ExpireTokenResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

protected:
    virtual void processReply();
};


class AvailableBuildsResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    AvailableBuildsResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

    const GameBuildInfoList &getGameBuilds() const { return mGameBuilds; }

protected:
    virtual void processReply();

private:
    GameBuildInfoList mGameBuilds;
};


class DeliveryRequestResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    DeliveryRequestResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

    const DeliveryStatusInfo &getDeliveryStatusInfo() const { return mDeliveryStatusInfo; }

protected:
    virtual void processReply();

private:
    DeliveryStatusInfo mDeliveryStatusInfo;
};


class DeliveryStatusResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    DeliveryStatusResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

    const DeliveryStatusInfoList &getDeliveryStatusList() { return mDeliveryStatusList; }

protected:
    virtual void processReply();

private:
    DeliveryStatusInfoList mDeliveryStatusList;
};


class GenerateUrlResponse : public DcmtServiceResponse
{
    Q_OBJECT

public:
    GenerateUrlResponse(QNetworkReply *reply) : DcmtServiceResponse(reply) { }

    virtual QVariant toVariant() const;

    const DeliveryStatusInfo &getDeliveryStatusInfo() const { return mDeliveryStatusInfo; }

protected:
    virtual void processReply();

private:
    DeliveryStatusInfo mDeliveryStatusInfo;
};


} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // DCMTSERVICERESPONSES_H
