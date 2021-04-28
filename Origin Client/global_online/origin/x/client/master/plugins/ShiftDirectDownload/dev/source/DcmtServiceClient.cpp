//  DcmtServiceClient.cpp
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#include <qdom.h>

#include "services/platform/PlatformService.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/crypto/SimpleEncryption.h"

#include "ShiftDirectDownload/DcmtDownloadManager.h"
#include "ShiftDirectDownload/DcmtDownloadUrlResponse.h"
#include "ShiftDirectDownload/DcmtServiceClient.h"
#include "../widgets/DcmtDownloadViewController.h"
#include "../widgets/ShiftLoginViewController.h"
#include "../widgets/ShiftOfferListViewController.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

QPointer<DcmtServiceClient> DcmtServiceClient::sInstance;

// Required for Shift submissions.
const QString fingerprint("xa37dd45ffe100bfffcc9753aabac325f07cb3fa231144fe2e33ae4783feead2b8a73ff021fac326df0ef9753ab9cdf6573ddff0312fab0b0ff39779eaff312a4f5de65892ffee33a44569bebf21f66d22e54a22347efd375981188743afd99baacc342d88a99321235798725fedcbf43252669dade32415fee89da543bf23d4ex");

const QString PLUGIN_CONFIG_SECTION(OBFUSCATE_STRING("[ShiftDirectDownload]"));
const QUrl DCMT_INTEGRATION_API_BASEURL(OBFUSCATE_STRING("https://dcmt.integration.ea.com/dcmt/rest/external/odt"));
const QUrl DCMT_PRODUCTION_API_BASEURL(OBFUSCATE_STRING("https://dcmt.ea.com/dcmt/rest/external/odt"));

void DcmtServiceClient::init()
{
    if (sInstance.isNull())
    {
        sInstance = new DcmtServiceClient();
        Services::Publishing::DownloadUrlProviderManager::instance()->registerProvider(sInstance);
    }
}


void DcmtServiceClient::release()
{
    if (!sInstance.isNull())
    {
        Services::Publishing::DownloadUrlProviderManager::instance()->unregisterProvider(sInstance);
        delete sInstance.data();
        sInstance = nullptr;
    }
}


DcmtServiceClient* DcmtServiceClient::instance()
{
    ORIGIN_ASSERT(!sInstance.isNull());

    return sInstance.data();
}


DcmtServiceClient::DcmtServiceClient()
    : mRefreshStatusCount(10)
{
    mTokenRefreshTimer.setSingleShot(false);
    ORIGIN_VERIFY_CONNECT(&mTokenRefreshTimer, SIGNAL(timeout()), this, SLOT(refreshToken()));
    reloadOverrides();
}


DcmtServiceClient::~DcmtServiceClient()
{
    logoutSync(false);
}


Services::Publishing::DownloadUrlResponse *DcmtServiceClient::downloadUrl(Services::Session::SessionRef session,
    const QString& productId, const QString& buildId, const QString& buildReleaseVersion, const QString& preferCDN, bool https)
{
    if (isLoggedIn())
    {
        QUrl url(Services::readSetting(Services::SETTING_OverrideDownloadPath.name() + "::" + productId));
        if (handlesScheme(url.scheme()))
        {
            return new DcmtDownloadUrlResponse(productId, url.path(), session);
        }
    }

    return nullptr;
}


bool DcmtServiceClient::handlesScheme(const QString& scheme)
{
    return isLoggedIn() && scheme == "shift";
}


void DcmtServiceClient::loginAsync()
{
    ShiftLoginViewController::instance()->show();
}


void DcmtServiceClient::logoutAsync(bool showLoginDialog)
{
    // Invoke on the next event loop or we will crash here!
    QMetaObject::invokeMethod(ShiftOfferListViewController::instance(), "close", Qt::QueuedConnection);
    DcmtDownloadManager::destroy();
    DcmtDownloadViewController::destroy();

    ExpireTokenResponse *response = expireToken();
    if (response != nullptr)
    {
        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SIGNAL(userLoggedOut()));
        if (showLoginDialog)
        {
            ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(loginAsync()));
        }
    }
    else if (showLoginDialog)
    {
        loginAsync();
    }
}


void DcmtServiceClient::logoutSync(bool showLoginDialog)
{
    // Invoke on the next event loop or we will crash here!
    QMetaObject::invokeMethod(ShiftOfferListViewController::instance(), "close", Qt::QueuedConnection);
    DcmtDownloadManager::destroy();
    DcmtDownloadViewController::destroy();

    ExpireTokenResponse *response = expireToken();
    if (response != nullptr)
    {
        QEventLoop loop;

        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SIGNAL(userLoggedOut()));
        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), &loop, SLOT(quit()));

        loop.exec();
    }

    if (showLoginDialog)
    {
        loginAsync();
    }
}


bool DcmtServiceClient::isLoggedIn()
{
    return !mAuthToken.isEmpty();
}


AuthenticateResponse *DcmtServiceClient::authenticate(const QString &userName, const QString &password)
{
    QNetworkRequest request(urlForServicePath(OBFUSCATE_STRING("/authenticate")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, OBFUSCATE_STRING("application/xml"));

    QDomDocument doc;
    QDomNode rootNode = doc.appendChild(doc.createElement("authRequest"));
    QDomNode userNameNode = rootNode.appendChild(doc.createElement("userName"));
    userNameNode.appendChild(doc.createTextNode(userName));
    QDomNode passwordNode = rootNode.appendChild(doc.createElement("password"));
    passwordNode.appendChild(doc.createTextNode(password));
    QDomNode timeStampNode = rootNode.appendChild(doc.createElement("timeStamp"));
    timeStampNode.appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));

    AuthenticateResponse *response = new AuthenticateResponse(postNonAuth(request, doc.toByteArray()));
    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onDcmtLoginComplete()));

    return response;
}


AvailableBuildsResponse *DcmtServiceClient::getAvailableBuilds(const QString &offerId,
    const QString &buildType, int pageNumber, int pageSize)
{
    QUrl url = urlForServicePath(OBFUSCATE_STRING("/gamebuilds"));
    QUrlQuery urlQuery(url);

    urlQuery.addQueryItem("offerId", overrideOfferId(offerId));
    urlQuery.addQueryItem("label", getLabelString());
    if (buildType.length() > 0)
    {
        urlQuery.addQueryItem("buildType", buildType);
    }
    urlQuery.addQueryItem("platform", getPlatformString());
    if (pageNumber > 0)
    {
        urlQuery.addQueryItem("pageNumber", QString::number(pageNumber));
    }
    if (pageSize > 0)
    {
        urlQuery.addQueryItem("pageSize", QString::number(pageSize));
    }
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    addAuthHeaders(request);

    AvailableBuildsResponse *response = new AvailableBuildsResponse(getNonAuth(request));
    ORIGIN_VERIFY_CONNECT(
        response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(checkResponseStatus(QNetworkReply::NetworkError)));

    return response;
}


DeliveryRequestResponse *DcmtServiceClient::placeDeliveryRequest(const QString &offerId, const QString &buildId)
{
    QNetworkRequest request(urlForServicePath(OBFUSCATE_STRING("/deliveryRequests")));
    addAuthHeaders(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, OBFUSCATE_STRING("application/xml"));

    QDomDocument doc;
    QDomNode rootNode = doc.appendChild(doc.createElement("deliveryRequest"));
    QDomNode offerIdNode = rootNode.appendChild(doc.createElement("offerId"));
    offerIdNode.appendChild(doc.createTextNode(overrideOfferId(offerId)));
    QDomNode buildIdNode = rootNode.appendChild(doc.createElement("buildId"));
    buildIdNode.appendChild(doc.createTextNode(buildId));

    DeliveryRequestResponse *response = new DeliveryRequestResponse(postNonAuth(request, doc.toByteArray()));
    ORIGIN_VERIFY_CONNECT(
        response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(checkResponseStatus(QNetworkReply::NetworkError)));

    return response;
}


DeliveryStatusResponse *DcmtServiceClient::getDeliveryStatus(int maxResults)
{
    QUrl url = urlForServicePath(OBFUSCATE_STRING("/deliveryRequests"));
    QUrlQuery urlQuery(url);

    urlQuery.addQueryItem("maxResults", QString::number(maxResults));

    QNetworkRequest request(url);
    addAuthHeaders(request);

    DeliveryStatusResponse *response = new DeliveryStatusResponse(getNonAuth(request));
    ORIGIN_VERIFY_CONNECT(
        response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(checkResponseStatus(QNetworkReply::NetworkError)));

    return response;
}


GenerateUrlResponse *DcmtServiceClient::generateDownloadUrl(const QString &buildId)
{
    QNetworkRequest request(urlForServicePath(OBFUSCATE_STRING("/generateDownloadURL")));
    addAuthHeaders(request);
    request.setRawHeader(OBFUSCATE_STRING("X-Client-Machine-Id"), Services::PlatformService::machineHashAsString().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, OBFUSCATE_STRING("application/xml"));

    QDomDocument doc;
    QDomNode rootNode = doc.appendChild(doc.createElement("deliveryStatusRequest"));
    QDomNode buildIdNode = rootNode.appendChild(doc.createElement("buildId"));
    buildIdNode.appendChild(doc.createTextNode(buildId));

    GenerateUrlResponse *response = new GenerateUrlResponse(postNonAuth(request, doc.toByteArray()));
    ORIGIN_VERIFY_CONNECT(
        response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(checkResponseStatus(QNetworkReply::NetworkError)));

    return response;
}


void DcmtServiceClient::onDcmtLoginComplete()
{
    AuthenticateResponse *response = dynamic_cast<AuthenticateResponse*>(sender());

    mAuthToken = response->getAuthToken().toUtf8();
    if (mAuthToken.size() > 0)
    {
        qint64 tokenExpiration = response->getTokenExpiration().toMSecsSinceEpoch();
        qint64 currentTime = response->getServerTime().toMSecsSinceEpoch();
        qint64 refreshTimerPeriod = tokenExpiration - currentTime - TOKEN_REFRESH_MARGIN;

        if (refreshTimerPeriod > MAX_TOKEN_REFRESH_TIME)
        {
            refreshTimerPeriod = MAX_TOKEN_REFRESH_TIME;
        }

        mTokenRefreshTimer.setInterval(refreshTimerPeriod);
        mTokenRefreshTimer.start();

        emit userLoggedIn();

        ShiftLoginViewController::instance()->close();
        ShiftOfferListViewController::instance()->show();
    }
}


void DcmtServiceClient::refreshToken()
{
    DcmtServiceResponse *response = getDeliveryStatus(mRefreshStatusCount);

    ORIGIN_VERIFY_CONNECT(
        response, SIGNAL(complete(QNetworkReply::NetworkError)),
        this, SLOT(onDeliveryStatusUpdated(QNetworkReply::NetworkError)));
}


void DcmtServiceClient::onDeliveryStatusUpdated(QNetworkReply::NetworkError status)
{
    DeliveryStatusResponse *response = dynamic_cast<DeliveryStatusResponse *>(sender());
    if (nullptr == response)
    {
        // EA_TODO("jonkolb", "2014/03/24", "Log an error, this should never happen")
        return;
    }

    // EA_TODO("jonkolb", "2014/03/24", "Keep around a copy of the old delivery status list and only send out new status")
    emit deliveryStatusUpdated(response);
}


void DcmtServiceClient::checkResponseStatus(QNetworkReply::NetworkError status)
{
    if (QNetworkReply::AuthenticationRequiredError == status)
    {
        loginAsync();
    }
    else
    {
        DeliveryRequestResponse *response = dynamic_cast<DeliveryRequestResponse *>(sender());
        if (response != nullptr && DCMT_OK == response->getErrorCode())
        {
            emit deliveryRequestPlaced(response);
        }
    }
}


const char *DcmtServiceClient::getLabelString() const
{
    QString envname = Services::readSetting(Services::SETTING_EnvironmentName);

    if (Services::SETTING_ENV_production == envname)
    {
        return "PROD";
    }
    else
    {
        return "INT";
    }
}


const char *DcmtServiceClient::getPlatformString() const
{
    switch (Services::PlatformService::runningPlatform())
    {
    case Services::PlatformService::PlatformWindows:
        return "PCWIN";

    case Services::PlatformService::PlatformMacOS:
        return "MAC";

    default:
        ORIGIN_LOG_WARNING << "Unknown platform detected...";
        return "UNKNOWN";
    }
}


QString DcmtServiceClient::overrideOfferId(const QString &offerId) const
{
    QString newOfferId = Engine::Config::ConfigFile::currentConfigFile().getOverride(PLUGIN_CONFIG_SECTION, OBFUSCATE_STRING("DcmtOfferIdOverride"), offerId);

    if (newOfferId.isEmpty())
    {
        newOfferId = Engine::Config::ConfigFile::currentConfigFile().getOverride(PLUGIN_CONFIG_SECTION, OBFUSCATE_STRING("DcmtOfferIdOverride"), "*");
    }

    if (newOfferId.isEmpty())
    {
        return offerId;
    }
    else
    {
        return newOfferId;
    }
}


ExpireTokenResponse *DcmtServiceClient::expireToken()
{
    ExpireTokenResponse *response = nullptr;

    mTokenRefreshTimer.stop();

    if (mAuthToken.length() > 0)
    {
        QNetworkRequest request(urlForServicePath(OBFUSCATE_STRING("/expireToken")));
        addAuthHeaders(request);
        request.setHeader(QNetworkRequest::ContentTypeHeader, OBFUSCATE_STRING("application/xml"));

        response = new ExpireTokenResponse(postNonAuth(request, QByteArray()));

        mAuthToken.clear();
    }

    return response;
}

void DcmtServiceClient::addAuthHeaders(QNetworkRequest &request)
{
    request.setRawHeader(OBFUSCATE_STRING("X-Access-Token"), mAuthToken);
    request.setRawHeader(OBFUSCATE_STRING("Authorization"), OBFUSCATE_STRING("Basic b3JpZ2luZGV2Om9yaWdpbjEyMw=="));
}


void DcmtServiceClient::reloadOverrides()
{
    Engine::Config::ConfigFile::reloadCurrentConfigFile();

    QString env = Engine::Config::ConfigFile::currentConfigFile().getOverride(PLUGIN_CONFIG_SECTION, OBFUSCATE_STRING("DcmtEnvironmentName"));
    if (Services::env::integration != env)
    {
        setBaseUrl(DCMT_PRODUCTION_API_BASEURL);
    }
    else
    {
        setBaseUrl(DCMT_INTEGRATION_API_BASEURL);
    }
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
