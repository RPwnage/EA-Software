///////////////////////////////////////////////////////////////////////////////
// SignInPortalServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/SignInPortalServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {

        SignInPortalEmailVerificationResponse* SignInPortalServiceClient::sendEmailVerificationRequest(Session::SessionRef session, const QString &accessToken)
        {
            return OriginClientFactory<SignInPortalServiceClient>::instance()->sendEmailVerificationRequestPriv(session, accessToken);
        }


        SignInPortalEmailVerificationResponse* SignInPortalServiceClient::sendEmailVerificationRequestPriv(Session::SessionRef session, const QString &accessToken)
        {
            QUrl sendVerificationURL(baseUrl());

            sendVerificationURL.setPath("/p/ajax/user/sendVerificationEmail");

            QUrlQuery urlQuery(sendVerificationURL);
            urlQuery.addQueryItem("accessToken", accessToken);
            sendVerificationURL.setQuery(urlQuery);

            QNetworkRequest request(sendVerificationURL);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            return new SignInPortalEmailVerificationResponse(postNonAuth(request, QByteArray())); 
        }


        SignInPortalServiceClient::SignInPortalServiceClient(const QUrl& baseUrl, NetworkAccessManager* nam)
            : OriginServiceClient(nam) 
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString signInPortalBaseURL = Origin::Services::readSetting(Origin::Services::SETTING_SignInPortalBaseUrl).toString();
                setBaseUrl(QUrl(signInPortalBaseURL));
            }
        }


    }
}
