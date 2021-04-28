///////////////////////////////////////////////////////////////////////////////
// AchievementShareServiceClient.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ACHIEVEMENTSHARESERVICECLIENT_H
#define _ACHIEVEMENTSHARESERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "services/rest/AchievementShareServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// HTTP service client for the friends component of the Friends GCS API.
        ///
        class ORIGIN_PLUGIN_API AchievementShareServiceClient : public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<AchievementShareServiceClient>;

            /// \brief Retrieves the achievements sharing status.
            ///
            /// \param session Current session.
            /// \param userId Nucleus Id.
            static AchievementShareServiceResponses *achievementsSharing(Services::Session::SessionRef session, quint64 userId)
            {
                return OriginClientFactory<AchievementShareServiceClient>::instance()->achievementsSharingPriv(session, userId);
            }

            /// \brief retrieves achievements sharing status
            ///
            /// \param session Current session.
            /// \param userId Nucleus Id.
            /// \param data TBD.
            static AchievementShareServiceResponses *setAchievementsSharing(Services::Session::SessionRef session, quint64 userId, QByteArray& data)
            {
                return OriginClientFactory<AchievementShareServiceClient>::instance()->setAchievementsSharingPriv(session, userId, data);
            }


        private:
            
            /// \brief Creates a new Achievement sharing service client
            ///
            /// \param baseUrl       Base URL for the friends service API
            /// \param nam           QNetworkAccessManager instance to send requests through
            explicit AchievementShareServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            /// \brief retrieves achievements sharing status (private version)
            ///
            /// \param session The current session.
            /// \param userId The Nucleus Id.
            /// \param data TBD.
            AchievementShareServiceResponses *setAchievementsSharingPriv(Services::Session::SessionRef, quint64, QByteArray&);
            
            /// \brief retrieves achievements sharing status (private version)
            ///
            /// \param session The current session.
            /// \param userId The Nucleus Id.
            AchievementShareServiceResponses *achievementsSharingPriv(Services::Session::SessionRef, quint64);

        };
    }
}

#endif
