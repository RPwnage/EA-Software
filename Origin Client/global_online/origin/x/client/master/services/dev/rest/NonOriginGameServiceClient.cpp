///////////////////////////////////////////////////////////////////////////////
// NonOriginGameServiceClient.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/NonOriginGameServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {

        AppStartedResponse* NonOriginGameServiceClient::playing(Session::SessionRef session, const QString& appId)
        {
            return OriginClientFactory<NonOriginGameServiceClient>::instance()->playingPrivate(session, appId);
        }

        AppFinishedResponse* NonOriginGameServiceClient::finishedPlaying(Session::SessionRef session, const QString& appId)
        {
            return OriginClientFactory<NonOriginGameServiceClient>::instance()->finishedPlayingPrivate(session, appId);
        }

        NonOriginGameFriendResponse* NonOriginGameServiceClient::friendsWithSameContent(Session::SessionRef session, const QString& appId)
        {
            return OriginClientFactory<NonOriginGameServiceClient>::instance()->friendsWithSameContentPrivate(session, appId);
        }

        AppUsageResponse* NonOriginGameServiceClient::usageTime(Session::SessionRef session, const QString& appId)
        {
            return OriginClientFactory<NonOriginGameServiceClient>::instance()->usageTimePrivate(session, appId);
        }

        NonOriginGameServiceClient::NonOriginGameServiceClient(const QUrl& baseUrl, NetworkAccessManager* nam)
            : OriginAuthServiceClient(nam) 
        {
            if (!baseUrl.isEmpty())
            {
                setBaseUrl(baseUrl);
            }
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_GamesServiceURL);
                setBaseUrl(strUrl);
            }
        }

        AppStartedResponse* NonOriginGameServiceClient::playingPrivate(Session::SessionRef session, const QString& appId)
        {
            QString servicePath = QString("/users/%1/apps/%2/start").arg(escapedNucleusId(session)).arg(appId);

            QUrl serviceUrl(urlForServicePath(servicePath));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AppStartedResponse(postAuth(session, request, QByteArray()));
        }

        AppFinishedResponse* NonOriginGameServiceClient::finishedPlayingPrivate(Session::SessionRef session, const QString& appId)
        {
            QString servicePath = QString("/users/%1/apps/%2/finish").arg(escapedNucleusId(session)).arg(appId);

            QUrl serviceUrl(urlForServicePath(servicePath));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AppFinishedResponse(postAuth(session, request, QByteArray()));
        }

        NonOriginGameFriendResponse* NonOriginGameServiceClient::friendsWithSameContentPrivate(Session::SessionRef session, const QString& appId)
        {
            QUrl serviceUrl(urlForServicePath("/users/" + escapedNucleusId(session) + "/apps/" + appId + "/friends"));            
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new NonOriginGameFriendResponse(getAuth(session, request), appId);      
        }

        AppUsageResponse* NonOriginGameServiceClient::usageTimePrivate(Session::SessionRef session, const QString& appId)
        {
            QString servicePath = QString("/users/%1/apps/%2/usage").arg(escapedNucleusId(session)).arg(appId);

            QUrl serviceUrl(urlForServicePath(servicePath));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new  AppUsageResponse(getAuth(session, request));   
        }
    }
}