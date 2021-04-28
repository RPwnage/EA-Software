///////////////////////////////////////////////////////////////////////////////
// DownloadUrlServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

void DownloadUrlServiceClient::init()
{
    DownloadUrlProviderManager::instance()->registerProvider(instance());
}

void DownloadUrlServiceClient::release()
{
    DownloadUrlProviderManager::instance()->unregisterProvider(instance());
}

DownloadUrlServiceClient *DownloadUrlServiceClient::instance()
{
    return OriginClientFactory<DownloadUrlServiceClient>::instance();
}

DownloadUrlServiceClient::DownloadUrlServiceClient(NetworkAccessManager *nam)
    : ECommerceServiceClient(nam)
{
}	

DownloadUrlResponse* DownloadUrlServiceClient::downloadUrl(Session::SessionRef session, const QString& productId, const QString& buildId, const QString& buildReleaseVersion, const QString& preferCDN, bool https)
{
    QString myUrl("downloadURL");
    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    QString cdnOverride = Origin::Services::readSetting(Origin::Services::SETTING_CdnOverride.name());

    serviceUrlQuery.addQueryItem("productId", productId);
    serviceUrlQuery.addQueryItem("userId", nucleusId(session));

    if(!cdnOverride.isEmpty())
    {
        cdnOverride = cdnOverride.trimmed();
        serviceUrlQuery.addQueryItem("cdnOverride", cdnOverride);
    }
    else if (!preferCDN.isEmpty())
    {
        cdnOverride = preferCDN.trimmed();
        serviceUrlQuery.addQueryItem("cdnOverride", cdnOverride);
        serviceUrlQuery.addQueryItem("useAlternateCdn", "true");
    }

    if(!buildId.isEmpty())
    {
        serviceUrlQuery.addQueryItem("buildLiveDate", buildId);
    }

    if(!buildReleaseVersion.isEmpty())
    {
        serviceUrlQuery.addQueryItem("buildReleaseVersion", buildReleaseVersion);
    }

    // If the client has requested an SSL/TLS URL
    if (https)
    {
        serviceUrlQuery.addQueryItem("https", "true");
    }

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);
    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    bool specificBuildRequested = !buildId.isEmpty() || !buildReleaseVersion.isEmpty();
    return new DownloadUrlResponse(getAuth(session,request), specificBuildRequested);
}

bool DownloadUrlServiceClient::handlesScheme(const QString &scheme)
{
    return scheme == "file" || scheme == "http" || scheme == "https" || scheme == "ftp";
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
