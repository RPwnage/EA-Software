/*************************************************************************************************/
/*!
    \file   achievementsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_ACHIEVEMENTSUTIL
#define BLAZE_GAMEREPORTING_ACHIEVEMENTSUTIL

#ifdef TARGET_achievements
#include "achievements/achievementsslaveimpl.h"
#include "achievements/tdf/achievements.h"

using namespace Blaze::Achievements;

namespace Blaze
{
namespace GameReporting
{
    class AchievementsUtil
    {
    public:
        AchievementsUtil() :
            mInitialized(false), mComponent(nullptr)
        {
        }

        ~AchievementsUtil()
        {
            mInitialized = false;
            mComponent = nullptr;
        }

        bool getAchievements(User user, ProductId productId, bool includeMetadata, LanguageCode language, FilterType filterType, AchievementList& result);
        bool getAchievements(const GetAchievementsRequest& request, AchievementList& result);

        // retrieve a single active achievement. This will be used by achievement access via gamereporting config
        bool getAchievement(User user, ProductId productId, bool includeMetadata, LanguageCode language, FilterType filterType, AchievementId achieveId, AchievementData& result);
        bool getAchievement(const GetAchievementsRequest& request, AchievementId achieveId, AchievementData& result);

        bool grantAchievement(User user, ProductId productId, AchievementId achieveId, Points points, Points current, LanguageCode language, AchievementData& result);
        bool grantAchievement(const GrantAchievementRequest& request, AchievementData& result);

        bool postEvents(User user, ProductId productId, EventsPayload& payload);
        bool postEvents(const PostEventsRequest& request);

    private:
        bool initialize();

        bool mInitialized;
        Achievements::AchievementsSlave* mComponent;
    };

} //namespace GameReporting
} //namespace Blaze
#endif

#endif
