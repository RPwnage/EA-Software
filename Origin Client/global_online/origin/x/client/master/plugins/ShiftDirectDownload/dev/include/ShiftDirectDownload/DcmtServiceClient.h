//  DcmtServiceClient.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef DCMTSERVICECLIENT_H
#define DCMTSERVICECLIENT_H

#include "engine/config/ConfigFile.h"
#include "engine/login/LoginController.h"
#include "services/publishing/DownloadUrlProviderManager.h"
#include "services/rest/HttpServiceClient.h"
#include "DcmtFileDownloader.h"
#include "DcmtServiceResponses.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

class DcmtServiceClient : public QObject, public Services::HttpServiceClient, public Services::Publishing::DownloadUrlProvider
{
    Q_OBJECT

public:

    /// \brief Initializes the session service.
    static void init();

    /// \brief De-initializes the session service.
    static void release();

    /// \brief Returns a pointer to the session services.
    static DcmtServiceClient* instance();

    /// \brief Returns a download url object for the given offer if it's been overridden
    virtual Services::Publishing::DownloadUrlResponse *downloadUrl(Services::Session::SessionRef session, const QString& productId, const QString& buildId, const QString& buildReleaseVersion, const QString& preferCDN, bool https);

    virtual bool handlesScheme(const QString &productId);
    virtual bool overridesUseJitService() { return true; }

public slots:

    /// \brief Initiates a new login.
    void loginAsync();

public:

    /// \brief Initiates a logout.
    void logoutAsync(bool showLoginDialog);

    /// \brief Initiates a logout (synchronous).
    void logoutSync(bool showLoginDialog);

    /// \brief Returns true if there is a currently valid session.
    bool isLoggedIn();

    /// \brief Initiates a request to authenticate the user with the provided credentials.
    AuthenticateResponse *authenticate(const QString &userName, const QString &password);

    /// \brief Initiates a request for the given offer's available builds.
    AvailableBuildsResponse *getAvailableBuilds(const QString &offerId,
        const QString &buildType = QString(), int pageNumber = 0, int pageSize = 0);

    /// \brief Initiates a request for the given offer's available builds.
    DeliveryRequestResponse *placeDeliveryRequest(const QString &offerId, const QString &buildId);

    /// \brief Initiates a request for the given offer's available builds.
    DeliveryStatusResponse *getDeliveryStatus(int maxResults = 10);

    /// \brief Initiates a request for the given offer's available builds.
    GenerateUrlResponse *generateDownloadUrl(const QString &buildId);

    /// \brief Reloads EACore.ini from disk.
    void reloadOverrides();

    /// \brief Increments the number of builds whose status is requested
    void addDownload() { ++mRefreshStatusCount; }

    /// \brief Decrements the number of builds whose status is requested
    void removeDownload() { --mRefreshStatusCount; }

signals:

    /// \brief Signal emitted when loginAsync has completed.
    void userLoggedIn();

    /// \brief Signal emitted when logoutAsync has completed.
    void userLoggedOut();

    /// \brief Signal emitted when build info is updated.
    void deliveryStatusUpdated(DeliveryStatusResponse *);

    /// \brief Signal emitted when a delivery request is successfully placed
    void deliveryRequestPlaced(DeliveryRequestResponse *);

protected slots:

    void onDcmtLoginComplete();

    void refreshToken();
    void onDeliveryStatusUpdated(QNetworkReply::NetworkError);
    void checkResponseStatus(QNetworkReply::NetworkError status);

private:
    DcmtServiceClient();
    DcmtServiceClient(DcmtServiceClient const& from);
    DcmtServiceClient& operator=(DcmtServiceClient const& from);
    virtual ~DcmtServiceClient();

    const char *getLabelString() const;
    const char *getPlatformString() const;
    QString overrideOfferId(const QString &offerId) const;
    ExpireTokenResponse *expireToken();
    void addAuthHeaders(QNetworkRequest &request);

private:
    static QPointer<DcmtServiceClient> sInstance;

    enum
    {
        TOKEN_REFRESH_MARGIN = 60 * 1000,
        MAX_TOKEN_REFRESH_TIME = 5 * 60 * 1000,
    };

    QByteArray mAuthToken;
    QTimer mTokenRefreshTimer;
    int mRefreshStatusCount;
};

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // DCMTSERVICECLIENT_H
