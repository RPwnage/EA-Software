///////////////////////////////////////////////////////////////////////////////
// AuthPortalServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"
#include "services/settings/SettingsManager.h"

namespace
{
	const static QString contentTypeHeader("application/x-www-form-urlencoded");
}

namespace Origin
{
    namespace Services
    {

		AuthPortalTokensResponse* AuthPortalServiceClient::extendTokens(const QString& refreshToken)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->extendTokensPriv(refreshToken);
        }

        AuthPortalTokensResponse* AuthPortalServiceClient::retrieveTokens(const QString& authorizationCode, const QString& redirectUri)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->retrieveTokensPriv("/connect/token",  authorizationCode, redirectUri);
        }

        AuthPortalTokenInfoResponse* AuthPortalServiceClient::retrieveAccessTokenInfo(const QString& accessToken)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->retrieveAccessTokenInfoPriv("/connect/tokeninfo", accessToken);
        }

        AuthPortalIdTokenInfoResponse* AuthPortalServiceClient::retrieveIdTokenInfo(const QString& idToken)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->retrieveIdTokenInfoPriv("/connect/idtokeninfo", idToken);
        }

        AuthPortalAuthorizationCodeResponse* AuthPortalServiceClient::retrieveAuthorizationCode(const QString& accessToken, const QString& clientId, const QStringList& scopes /* = QStringList()*/)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->retrieveAuthorizationCodePriv("/connect/auth", accessToken, clientId, scopes);
        }
 
        AuthPortalSidCookieRefreshResponse* AuthPortalServiceClient::refreshSidCookie(const QString& accessToken)
        {
            return OriginClientFactory<AuthPortalServiceClient>::instance()->refreshSidCookiePriv("/connect/auth", accessToken);
        }

        AuthPortalServiceClient::AuthPortalServiceClient(const QUrl& baseUrl, NetworkAccessManager* nam)
            : OriginServiceClient(nam) 
        {

            mClientId = Origin::Services::readSetting(Origin::Services::SETTING_ClientId).toString();
            mClientSecret = Origin::Services::readSetting(Origin::Services::SETTING_ClientSecret).toString();

            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString connectPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_ConnectPortalBaseUrl).toString();
                setBaseUrl(connectPortalBaseUrl);
            }
        }

        AuthPortalTokensResponse* AuthPortalServiceClient::extendTokensPriv(const QString& refreshToken)
		{
            QUrlQuery postBody;

            postBody.addQueryItem("client_id", mClientId.toLatin1());
            postBody.addQueryItem("grant_type", "refresh_token");
			postBody.addQueryItem("refresh_token", refreshToken.toLatin1());
			postBody.addQueryItem("client_secret", mClientSecret.toLatin1());

            QNetworkRequest req(baseUrl().toString() + "/connect/token");
            req.setHeader(QNetworkRequest::ContentTypeHeader, contentTypeHeader.toLatin1());
            return new AuthPortalTokensResponse(postNonAuthOffline(req, postBody.query(QUrl::FullyEncoded).toUtf8()));
        }

        AuthPortalTokensResponse* AuthPortalServiceClient::retrieveTokensPriv(const QString& servicePath, const QString& authorizationCode, const QString &redirectUri)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QNetworkRequest request(serviceUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader,contentTypeHeader);

            QByteArray data;
            QUrlQuery params;
            params.addQueryItem("grant_type", "authorization_code");
            params.addQueryItem("code", authorizationCode.toLatin1());
            params.addQueryItem("client_id", mClientId.toLatin1());
            params.addQueryItem("client_secret", mClientSecret.toLatin1());

            if(!redirectUri.isEmpty())
                 params.addQueryItem("redirect_uri", redirectUri.toLatin1());
            else
            {
                QString redirectUrl = Origin::Services::readSetting(Origin::Services::SETTING_LoginSuccessRedirectUrl).toString();
                params.addQueryItem("redirect_uri", redirectUrl.toLatin1());
            }
            data.append(params.toString(QUrl::FullyEncoded));
            
            return new AuthPortalTokensResponse(postNonAuth(request, data));
        }

        AuthPortalTokenInfoResponse* AuthPortalServiceClient::retrieveAccessTokenInfoPriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("access_token", accessToken.toLatin1());
            serviceUrlQuery.addQueryItem("client_id", mClientId.toLatin1());
            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            return new AuthPortalTokenInfoResponse(getNonAuth(request));
        }

        AuthPortalIdTokenInfoResponse* AuthPortalServiceClient::retrieveIdTokenInfoPriv(const QString& servicePath, const QString& idToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("client_id", mClientId.toUtf8());
            serviceUrlQuery.addQueryItem("id_token", idToken.toUtf8());
            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            return new AuthPortalIdTokenInfoResponse(getNonAuth(request));
        }

        AuthPortalAuthorizationCodeResponse* AuthPortalServiceClient::retrieveAuthorizationCodePriv(const QString& servicePath, const QString& accessToken,
            const QString& clientId, const QStringList& scopes /* = QStringList()*/)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);
            if (!scopes.isEmpty())
            {
                serviceUrlQuery.addQueryItem("scope", scopes.join(" ").toUtf8());
            }

            serviceUrlQuery.addQueryItem("access_token", accessToken.toUtf8());
            serviceUrlQuery.addQueryItem("client_id", clientId.toUtf8());
            serviceUrlQuery.addQueryItem("response_type", "code");
            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            return new AuthPortalAuthorizationCodeResponse(getNonAuth(request));
        }

        AuthPortalSidCookieRefreshResponse* AuthPortalServiceClient::refreshSidCookiePriv(const QString& servicePath, const QString& accessToken)
        {
            QUrl serviceUrl(baseUrl().toString() + servicePath);
            QUrlQuery serviceUrlQuery(serviceUrl);

            serviceUrlQuery.addQueryItem("access_token", accessToken.toUtf8());
            serviceUrlQuery.addQueryItem("client_id", mClientId.toUtf8());
            serviceUrlQuery.addQueryItem("response_type", "code");
            serviceUrlQuery.addQueryItem("prompt", "none");
            serviceUrl.setQuery(serviceUrlQuery);

            QNetworkRequest request(serviceUrl);

            return new AuthPortalSidCookieRefreshResponse(getNonAuth(request));
        }
    }
}
