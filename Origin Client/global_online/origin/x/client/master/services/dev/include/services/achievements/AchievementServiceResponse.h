///////////////////////////////////////////////////////////////////////////////
// AchievementServiceResponse.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __ACHIEVEMENTSERVICERESPONSE_H__
#define __ACHIEVEMENTSERVICERESPONSE_H__

#include "services/rest/OriginAuthServiceResponse.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/plugin/PluginAPI.h"

#include <QVector>
#include <QHash>
#include <server.h>

namespace Origin
{
    namespace Services
    {
        namespace Achievements
        {
            typedef QHash<int, QString> IconMap;

            struct AchievementT
            {
                long long u;
                long long e;
                int p;
                int t;
                int cnt;
                QString img;
                QString name;
                QString desc;
                QString howto;
                int xp;
                QString xp_t;
                int rp;
                QString rp_t;
                IconMap icons;

                bool hasExp;
                QString expId;
                QString expName;
            };

            struct ExpansionT
            {
                QString expansionId;
                QString expansionName;
            };

            typedef QHash<QString, AchievementT> AchievementMap;
            typedef QList<ExpansionT> ExpansionList;

            struct AchievementSetT
            {
                QString setId;
                QString name;
                QString platform;
                AchievementMap achievements;
                ExpansionList expansions;
            };

            typedef QList<AchievementSetT> AchievementSetList;

			struct AchievementSetReleaseInfoT
			{
				QString achievementSetId;
				QString masterTitleId;
				QString displayName;
				QString platform;
			};
			
			typedef QList<AchievementSetReleaseInfoT> AchievementSetReleaseInfoList;

            class ORIGIN_PLUGIN_API AchievementSetQueryResponse : public OriginAuthServiceResponse
            {
            public:
                explicit AchievementSetQueryResponse(const QString &personaId, const QString &achievementSet, AuthNetworkRequest);
                const AchievementSetT &achievementSet() const { return mSet; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                AchievementSetT mSet;
            };



            class ORIGIN_PLUGIN_API AchievementSetsQueryResponse : public OriginAuthServiceResponse
            {
            public:
                explicit AchievementSetsQueryResponse(const QString &personaId, AuthNetworkRequest);

                const AchievementSetList &achievementSets(){ return mAchievementSets; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                AchievementSetList mAchievementSets;
            };


			class AchievementSetReleaseInfoResponse : public OriginAuthServiceResponse
			{
			public:
				explicit AchievementSetReleaseInfoResponse(AuthNetworkRequest);

				const AchievementSetReleaseInfoList &achievementSetReleaseInfo(){return mAchievementSetReleaseInfo;}

			protected:
				bool parseSuccessBody(QIODevice*);

			private:
				AchievementSetReleaseInfoList mAchievementSetReleaseInfo;
			};
												

            class ORIGIN_PLUGIN_API AchievementProgressQueryResponse : public OriginAuthServiceResponse
            {
            public:
                explicit AchievementProgressQueryResponse(AuthNetworkRequest);
            protected:
                bool parseSuccessBody(QIODevice*);

            public:
                int xp() const { return mProgress.experience; }
                int rp() const { return mProgress.rewardPoints; }
                int achieved() const { return mProgress.achievements; }

            private:
                server::ProgressT mProgress;
            };

            class ORIGIN_PLUGIN_API AchievementGrantResponse : public OriginAuthServiceResponse
            {
            public:
                explicit AchievementGrantResponse(AuthNetworkRequest);
            protected:
                bool parseSuccessBody(QIODevice*);
            };

            class ORIGIN_PLUGIN_API PostEventResponse : public OriginAuthServiceResponse
            {
            public:
                explicit PostEventResponse(AuthNetworkRequest);
            protected:
                bool parseSuccessBody(QIODevice*);
            };
        }
    }
}

#endif //__ACHIEVEMENTSERVICERESPONSE_H__
