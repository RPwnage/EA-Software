///////////////////////////////////////////////////////////////////////////////
// AuthPortalServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AUTHPORTALSERVICECLIENT_H_INCLUDED_
#define _AUTHPORTALSERVICECLIENT_H_INCLUDED_

#include "OriginServiceClient.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class AuthPortalTokensResponse;
        class AuthPortalTokenInfoResponse;
        class AuthPortalIdTokenInfoResponse;
        class AuthPortalAuthorizationCodeResponse;
        class AuthPortalSidCookieRefreshResponse;


        class ORIGIN_PLUGIN_API AuthPortalServiceClient :	public OriginServiceClient
        {

        public:
            friend class OriginClientFactory<AuthPortalServiceClient>;
          
			static AuthPortalTokensResponse* extendTokens(const QString& refreshToken);
            static AuthPortalTokensResponse* retrieveTokens(const QString& authorizationCode, const QString &redirectUri = QString());
            static AuthPortalTokenInfoResponse* retrieveAccessTokenInfo(const QString& accessToken);
            static AuthPortalIdTokenInfoResponse* retrieveIdTokenInfo(const QString& idToken);

            /// \brief Retrieves an authorization code for a specified client id. If the scopes list is empty, the id's default scopes will be used.
            static AuthPortalAuthorizationCodeResponse* retrieveAuthorizationCode(const QString& accessToken, const QString& clientId,
                const QStringList& scopes = QStringList());

            /// \brief Makes a connect/auth call to the server which refreshes the sid cookie
            static AuthPortalSidCookieRefreshResponse* refreshSidCookie(const QString& accessToken);

        private:

            QString mClientId;
            QString mClientSecret;

            explicit AuthPortalServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            AuthPortalTokensResponse* extendTokensPriv(const QString& refreshToken);
            AuthPortalTokensResponse* retrieveTokensPriv(const QString& servicePath, const QString& authorizationCode, const QString &redirectUri);
            AuthPortalTokenInfoResponse* retrieveAccessTokenInfoPriv(const QString& servicePath, const QString& accessToken);
            AuthPortalIdTokenInfoResponse* retrieveIdTokenInfoPriv(const QString& servicePath, const QString& idToken);
            AuthPortalAuthorizationCodeResponse* retrieveAuthorizationCodePriv(const QString& servicePath, const QString& accessToken, const QString& clientId,
                const QStringList& scopes = QStringList());
            AuthPortalSidCookieRefreshResponse* refreshSidCookiePriv(const QString& servicePath, const QString& accessToken);
        };
    }
}

#endif

