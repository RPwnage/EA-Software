///////////////////////////////////////////////////////////////////////////////
// GamesService.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __GAMES_SERVICE_H__
#define __GAMES_SERVICE_H__

#include "services/rest/OriginAuthServiceClient.h"
#include "services/rest/GamesServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API GamesServiceClient :	public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<GamesServiceClient>;

            static GamesServiceClient *instance();

            ///
            /// \brief Get a list of games that friends own
            ///
            /// \param session  TBD.
            /// \param friendId The identifier of a single friend to be queried. Defaults to 0 and will be ignored. 
            ///
            GamesServiceFriendResponse * commonGames(Session::SessionRef session, QList<quint64> const * const friendId);

            ///
            /// \brief Started playing the content identified by master title ID and multiplayer ID
            ///
            GameStartedResponse * playing(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId);

            ///
            /// \brief Finished playing the content identified by the master title ID and multiplayer ID
            ///
            GameFinishedResponse * finishedPlaying(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId);

            ///
            /// \brief Finished playing the content identified by the master title ID and multiplayer ID
            ///
            GameUsageResponse * playedTime(Session::SessionRef session, const QString &masterTitleId, const QString &multiplayerId);

        private:
            /// 
            /// \brief Creates a new GamesService service client
            /// \param baseUrl base url for the service (QUrl)
            /// \param nam           QNetworkAccessManager instance to send requests through
            ///
            explicit GamesServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
        };
    }
}


#endif // __GAMES_SERVICE_H__

