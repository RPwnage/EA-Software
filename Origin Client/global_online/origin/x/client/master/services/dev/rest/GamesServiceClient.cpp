///////////////////////////////////////////////////////////////////////////////
// GamesServiceClient.cpp
// These are private helper implementations to build and send the requests to each of the RESTs
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/rest/GamesServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        GamesServiceClient *GamesServiceClient::instance()
        {
            return OriginClientFactory<GamesServiceClient>::instance();
        }

        GamesServiceClient::GamesServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_GamesServiceURL);
                setBaseUrl(strUrl);
            }
        }

        GamesServiceFriendResponse * GamesServiceClient::commonGames(Session::SessionRef session, QList<quint64> const * const friendIds)
        {
            QString list;
            QUrl serviceUrl(urlForServicePath("/users/" + escapedNucleusId(session) + "/commonGames"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            // The ?userId= parameter is our own Nucleus ID, the Authentication Token is used by the server to validate this.
            
            // Build the list of friendIds, the server will verify that these are indeed our friends.
            foreach ( quint64 friendId, *friendIds  )
            {
                list += QString::number(friendId) + ",";
            }
            list.resize(list.size()-1);
            serviceUrlQuery.addQueryItem("friendIds", list);
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new GamesServiceFriendResponse(getAuth(session, request));
        }	
        
        // POST, but not body is sent, no returned content.
        GameStartedResponse * GamesServiceClient::playing(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId)
        {
            QString servicePath = QString("/users/%1/games/%2/start").arg(escapedNucleusId(session)).arg(masterTitleId);

            QUrl serviceUrl(urlForServicePath(servicePath));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            if (!multiplayerId.isEmpty())
            {
                request.setRawHeader("MultiplayerId", multiplayerId.toUtf8());
            }
            return new GameStartedResponse(postAuth(session, request, QByteArray()));
        }	

        // POST, but not body is sent, no returned content.
        GameFinishedResponse * GamesServiceClient::finishedPlaying(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId)
        {
            QString servicePath = QString("/users/%1/games/%2/finish").arg(escapedNucleusId(session)).arg(masterTitleId);

            QUrl serviceUrl(urlForServicePath(servicePath));           
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            if (!multiplayerId.isEmpty())
            {
                request.setRawHeader("MultiplayerId", multiplayerId.toUtf8());
            }
            return new GameFinishedResponse(postAuth(session, request, QByteArray()));
        }	

        GameUsageResponse * GamesServiceClient::playedTime(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId)
        {
            QString servicePath = QString("/users/%1/games/%2/usage").arg(escapedNucleusId(session)).arg(masterTitleId);

            QUrl serviceUrl(urlForServicePath(servicePath));
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            if (!multiplayerId.isEmpty())
            {
                request.setRawHeader("MultiplayerId", multiplayerId.toUtf8());
            }
            return new GameUsageResponse(getAuth(session, request));
        }
    }
}
