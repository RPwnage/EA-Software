
#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/component/achievementscomponent.h"

#include "BlazeSDK/component/achievements/tdf/achievements.h"

namespace Ignition
{

class Achievements :
    public LocalUserUiBuilder
{
    public:
        

        Achievements(uint32_t userIndex);
        virtual ~Achievements();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();

    private:

        PYRO_ACTION(Achievements, GetAchievements);
        PYRO_ACTION(Achievements, GrantAchievement);
        PYRO_ACTION(Achievements, GrantAchievementFromList);

        PYRO_ACTION(Achievements, GetAchievementsForUser);

        void GetAchievementsCb(const Blaze::Achievements::GetAchievementsResponse *response, Blaze::BlazeError error, Blaze::JobId jobId);
        void GrantAchievementCb(const Blaze::Achievements::AchievementData *response, Blaze::BlazeError error, Blaze::JobId jobId);

        static const char8_t* DEFAULT_ACHIEVEMENTS_PRODUCT_ID;
        static const char8_t* DEFAULT_ACHIEVEMENTS_LOGIN_PASSWORD;
        static const char8_t* DEFAULT_ACHIEVEMENTS_SECRET;

};

}

#endif //ACHIEVEMENTS_H
