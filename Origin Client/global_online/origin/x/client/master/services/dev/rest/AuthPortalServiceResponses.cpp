///////////////////////////////////////////////////////////////////////////////
// AuthPortalServiceResponses.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/common/JsonUtil.h"
#include "services/rest/AuthPortalServiceResponses.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "NodeDocument.h"
#include "ReaderCommon.h"


namespace
{
    QString safe_map_get(const QMap<QString, QString> &map, const QString &key)
    {
        QString value = "";

        if (map.contains(key))
        {
            value = map[key];
        }

        return value;
    }
}

namespace Origin
{
    namespace Services
    {
        AuthPortalSidCookieRefreshResponse::AuthPortalSidCookieRefreshResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        bool AuthPortalSidCookieRefreshResponse::parseSuccessBody(QIODevice* body )
        {
            return true;
        }

        AuthPortalTokensResponse::AuthPortalTokensResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {
            //normally when a REST call fails as a result of some authentication, we trigger a signal to initiate 
            //a call to renewAccessToken (see OriginServiceResponse::processReply()),
            //but in this case, since this response is as a result of calling renewAccessToken, we don't want to trigger the signal since
            //that will just cause an infinite loop
            mFireAuthenticationErrorSignal = false;
        }

        bool AuthPortalTokensResponse::parseSuccessBody(QIODevice* body )
        {
            bool success = false;
            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            QByteArray payload(body->readAll());

            if (json && json->Parse(payload.data()))
            {
                success = ReadAttribute(json.data(), "access_token", mTokens.mAccessToken);
                success |= ReadAttribute(json.data(), "token_type", mTokens.mTokenType);
                success |= ReadAttribute(json.data(), "expires_in", mTokens.mExpiresIn);
                success |= ReadAttribute(json.data(), "refresh_token", mTokens.mRefreshToken);
                success |= ReadAttribute(json.data(), "id_token", mTokens.mIdToken);
            }

            return success;
        }

        AuthPortalTokenInfoResponse::AuthPortalTokenInfoResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        bool AuthPortalTokenInfoResponse::parseSuccessBody(QIODevice *body)
        {
            /* Example JSON

                {
                    "client_id":"client1",
                    "scope":"basic.identity openid",
                    "expires_in":3599,
                    "pid_id":4684250331,
                    "pid_type":"Nucleus",
                    "user_id":4684250331,
                    "persona_id":null
                }

            */

            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            QByteArray payload(body->readAll());

            if (json && json->Parse(payload.data()))
            {
                if (json->FirstAttribute())
                {
                    do
                    {
                        QString key = json->GetAttributeName();
                        QString value = json->GetAttributeValue();
                        mTokenInfo.insert(key, value);
                    }
                    while (json->NextAttribute());

                    return true;
                }
            }

            return false;
        }

        QString AuthPortalTokenInfoResponse::token(const QString& token_name) const
        {
            return safe_map_get(mTokenInfo, token_name);
        }

        QString AuthPortalTokenInfoResponse::clientId() const
        {
            return token("client_id");
        }
    
        QString AuthPortalTokenInfoResponse::expiresIn() const
        {
            return token("expires_in");
        }

        QString AuthPortalTokenInfoResponse::pidId() const
        {
            return token("pid_id");
        }

        QString AuthPortalTokenInfoResponse::pidType() const
        {
            return token("pid_type");
        }

        QString AuthPortalTokenInfoResponse::scope() const
        {
            return token("scope");
        }

        QString AuthPortalTokenInfoResponse::userId() const
        {
            return token("user_id");
        }

        QString AuthPortalTokenInfoResponse::personaId() const
        {
            return token("persona_id");
        }

        AuthPortalIdTokenInfoResponse::AuthPortalIdTokenInfoResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        QString AuthPortalIdTokenInfoResponse::cid() const
        {
            return token("cid");
        }

        QString AuthPortalIdTokenInfoResponse::token(const QString& token_name) const
        {
            return safe_map_get(mTokenInfo, token_name);
        }

        /* Example JSON
        {
            "aud": "client1",
            "iss": "account.ea.com",
            "iat": 1359030208,
            "exp": 1359033808,
            "nonce": "123123",
            "pid_id": 4684250331,
            "user_id": 4684250331,
            "persona_id": null,
            "pid_type": "NUCLEUS",
            "from_rememberme": false,
            "login_as_invisible": false,
            "cid": "9BbET5QdxGMWiwMnZ5nu4QBkN6f9IrUu",
            "auth_time": 1359030208,
            "at_hash": null,
            "c_hash": null
        }
        */

        bool AuthPortalIdTokenInfoResponse::parseSuccessBody(QIODevice* body)
        {
            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            QByteArray payload(body->readAll());

            if (json && json->Parse(payload.data()))
            {
                if (json->FirstAttribute())
                {
                    do
                    {
                        QString key = json->GetAttributeName();
                        QString value = json->GetAttributeValue();
                        mTokenInfo.insert(key, value);
                    }
                    while (json->NextAttribute());

                    return true;
                }
            }

            return false;
        }

        AuthPortalAuthorizationCodeResponse::AuthPortalAuthorizationCodeResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        QString AuthPortalAuthorizationCodeResponse::authorizationCode() const
        {
            return mAuthorizationCode;
        }

        QString AuthPortalAuthorizationCodeResponse::authCodeError() const
        {
            return mAuthCodeError;
        }

        QString AuthPortalAuthorizationCodeResponse::authCodeErrorCode() const
        {
            return mAuthCodeErrorCode;
        }

        bool AuthPortalAuthorizationCodeResponse::parseSuccessBody(QIODevice* body)
        {
            if (reply())
            {
                QString locationHeader = reply()->header(QNetworkRequest::LocationHeader).toString();
                QUrl locationUrl (locationHeader);
                //in order for the query key/value pairs to get parsed correctly, we need to pass in only the query portion of the QUrl.
                //otherwise something like http://localhost/?code=ACJNy88IlsUuX01be1rfHFr54pkBRKxwjRMQe6vOtQ
                //will get parsed as key = http://localhost/?code, value = ACJNy88IlsUuX01be1rfHFr54pkBRKxwjRMQe6vOtQ
                //and then the queryItemValue below will end up returning an empty string
                QUrlQuery redirectUrlQuery(locationUrl.query());
                mAuthorizationCode = redirectUrlQuery.queryItemValue("code");
                mAuthCodeError = redirectUrlQuery.queryItemValue("error");
                mAuthCodeErrorCode = redirectUrlQuery.queryItemValue("error_code");
            }

            return !mAuthorizationCode.isEmpty();
        }

    }
}
