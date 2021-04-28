///////////////////////////////////////////////////////////////////////////////
// UserInfoServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/UserInfoServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        UserInfoResponse* UserInfoServiceClient::userInfo(Session::SessionRef session, const QString &pid)
        {
            return OriginClientFactory<UserInfoServiceClient>::instance()->userInfoPrivate(session, pid); 
        }
    
        UserInfoResponse* UserInfoServiceClient::userInfoPrivate(Session::SessionRef session, const QString &pid)
        {
            QUrl serviceUrl("www.google.com"); //(urlForServicePath("what/is/this/users?pid=" + pid));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new UserInfoResponse(postAuth(session, request, QByteArray()));
        }

        UserInfoServiceClient::UserInfoServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam) 
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString strUrl = "www.google.com"; //Origin::Services::readSetting(Origin::Services::SETTING_UserInfoURLTODO); 
                setBaseUrl(QUrl(strUrl + "/something/better/TODO/"));
            }
        }
    }
}

