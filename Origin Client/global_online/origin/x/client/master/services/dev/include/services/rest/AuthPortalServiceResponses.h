///////////////////////////////////////////////////////////////////////////////
// AuthPortalServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AUTHPORTALSERVICERESPONSES_H_INCLUDED_
#define _AUTHPORTALSERVICERESPONSES_H_INCLUDED_

#include "OriginServiceResponse.h"
#include "AuthenticationData.h"

#include <QMap>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API AuthPortalSidCookieRefreshResponse : public OriginServiceResponse
        {
           public:
                explicit AuthPortalSidCookieRefreshResponse(QNetworkReply* reply);

            private:

                bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API AuthPortalTokensResponse : public OriginServiceResponse
        {
            public:
                explicit AuthPortalTokensResponse(QNetworkReply* reply);
                const struct TokensNucleus2& tokens() const { return mTokens; }

            protected:
                struct TokensNucleus2 mTokens;

                bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API AuthPortalTokenInfoResponse : public OriginServiceResponse
        {
            public:

                explicit AuthPortalTokenInfoResponse(QNetworkReply* reply);
                const QMap<QString, QString>& tokenInfo() const { return mTokenInfo; }

                QString clientId() const;
                QString expiresIn() const;
                QString personaId() const;     
                QString pidId() const;
                QString pidType() const;
                QString scope() const;
                QString userId() const;
            
            protected:

                QString token(const QString& token_name) const;

                QMap<QString, QString> mTokenInfo;

                bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API AuthPortalIdTokenInfoResponse : public OriginServiceResponse
        {
            public:

                explicit AuthPortalIdTokenInfoResponse(QNetworkReply* reply);
                const QMap<QString, QString>& tokenInfo() const { return mTokenInfo; }

                QString cid() const;

            protected:

                QString token(const QString& token_name) const;

                QMap<QString, QString> mTokenInfo;

                bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API AuthPortalAuthorizationCodeResponse : public OriginServiceResponse
        {
            public:

                explicit AuthPortalAuthorizationCodeResponse(QNetworkReply* reply);
                QString authorizationCode() const;
                QString authCodeError() const;
                QString authCodeErrorCode() const;

            protected:

                QString mAuthorizationCode;
                QString mAuthCodeError;
                QString mAuthCodeErrorCode;

                bool parseSuccessBody(QIODevice* body);
        };
    }
}

#endif


