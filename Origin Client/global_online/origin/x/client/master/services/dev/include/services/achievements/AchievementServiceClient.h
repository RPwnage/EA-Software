///////////////////////////////////////////////////////////////////////////////
// AchievementServiceClient.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ACHIEVEMENTSERVICECLIENT_H
#define _ACHIEVEMENTSERVICECLIENT_H

#include "services/rest/OriginAuthServiceClient.h"
#include "services/achievements/AchievementServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Achievements
        {

            ///
            /// \brief HTTP service client for the communication with the achievement service.
            ///
            class ORIGIN_PLUGIN_API AchievementServiceClient : public OriginAuthServiceClient
            {
            public:
                friend class OriginClientFactory<AchievementServiceClient>;


                ///
                /// \brief Returns achievements for the user
                ///
                static AchievementSetQueryResponse *achievements(const QString &personaId, const QString& achievementSet, const QString &language, bool bMetadata)
                {
                    return OriginClientFactory<AchievementServiceClient>::instance()->achievementsPriv(personaId, achievementSet, language, bMetadata);
                }

                ///
                /// \brief Returns all achievement sets for the user
                ///
                static AchievementSetsQueryResponse *achievementSets(const QString &personaId, const QString &language, bool bMetadata)
                {
                    return OriginClientFactory<AchievementServiceClient>::instance()->achievementSetsPriv(personaId, language, bMetadata);
                }

				///
				/// \brief Returns all achievement sets release info configured
				///
				static AchievementSetReleaseInfoResponse *achievementSetReleaseInfo(const QString &language)
				{
					return OriginClientFactory<AchievementServiceClient>::instance()->achievementSetReleaseInfoPriv(language);
				}

				///
                /// \brief Returns the users points information.
                ///
                static AchievementProgressQueryResponse *points(const QString &personaId)
                {
                    return OriginClientFactory<AchievementServiceClient>::instance()->pointsPriv(personaId);
                }

                ///
                /// \brief grants the achievement.
                ///
                static AchievementGrantResponse *grantAchievement(const QString &personaId, const QString &appId, const QString &achievementSetId, const QString &achievementId, const QString &achievementCode, int progress)
                {
                    return OriginClientFactory<AchievementServiceClient>::instance()->grantPriv(personaId, appId, achievementSetId, achievementId, achievementCode, progress);
                }

                ///
                /// \brief post events.
                ///
                static PostEventResponse * postEvent(QString personaId, QString appId, QString achievementSetId, QString events)
                {
                    return OriginClientFactory<AchievementServiceClient>::instance()->postEventPriv(personaId, appId, achievementSetId, events);
                }

            private:

                /// \brief The actual implementation of the achievements query.
                /// \param personaId The EAID persona, or the game personaId.
                /// \param achievementSet The game product identifier for the achievements.
                /// \param language The language of the string resources. If empty no string resources are returned. When specified check whether string resources are returned, else default to en_US.
                /// \param bMetadata When this flag is try additional achievement data is returned. Eg. Xp points/Rp points.
                AchievementSetQueryResponse *achievementsPriv(const QString &personaId, const QString& achievementSet, const QString &language, bool bMetadata);

                /// \brief The actual implementation of the achievement sets query.
                /// \param personaId The EAID persona, or the game personaId.
                /// \param language The language of the string resources. If empty no string resources are returned. When specified check whether string resources are returned, else default to en_US.
                /// \param bMetadata When this flag is try additional achievement data is returned. Eg. Xp points/Rp points.
                AchievementSetsQueryResponse *achievementSetsPriv(const QString &personaId, const QString &language, bool bMetadata);

				/// \brief The actual implementation of the achievement sets release info query.
				/// \param language The language of the string resources. If empty no string resources are returned. When specified check whether string resources are returned, else default to en_US.
				AchievementSetReleaseInfoResponse *achievementSetReleaseInfoPriv(const QString &language);

                /// \brief Implementation of the query for getting the users points progress.
                /// \param personaId The EAID persona, or the game personaId.
                AchievementProgressQueryResponse *pointsPriv(const QString &personaId);

                /// \brief Grants the specified achievement.
                /// \param personaId The EAID persona, or the game personaId.
				/// \param appId TBD.
                /// \param achievementSetId The achievementSet of the achievement to grant.
                /// \param achievementId The id of the achievement to award.
                /// \param achievementCode The secret associated with the achievement.
                /// \param progress The amount of progress to add.
                AchievementGrantResponse *grantPriv(const QString &personaId, const QString &appId, const QString &achievementSetId, const QString &achievementId, const QString &achievementCode, int progress);

                /// \brief Grants the specified achievement.
                /// \param personaId The EAID persona, or the game personaId.
                /// \param appId TBD.
                /// \param achievementSetId The achievementSet of the achievement to grant.
                /// \param events The events to post
                PostEventResponse *postEventPriv(const QString &personaId, const QString &appId, const QString &achievementSetId, const QString &events);

                /// 
                /// \brief Creates a new achievement service client
                ///
                /// \param baseUrl       Base URL for the achievement service API.
                /// \param nam           QNetworkAccessManager instance to send requests through.
                ///
                explicit AchievementServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
            };
        }
    }
}

#endif //_ACHIEVEMENTSERVICECLIENT_H