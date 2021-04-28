/*************************************************************************************************/
/*!
    \file   achievementsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "achievementsutil.h"

namespace Blaze
{
namespace GameReporting
{
#ifdef TARGET_achievements
    bool AchievementsUtil::initialize()
    {
        // check if already initialized
        if (mInitialized)
        {
            return true;
        }

        mComponent = static_cast<Achievements::AchievementsSlave*>(gController->getComponent(Achievements::AchievementsSlave::COMPONENT_ID, false));    
        if (mComponent == nullptr)
        {
            return false;
        }

        mInitialized = true;
        return true;
    }

    bool AchievementsUtil::getAchievements(User user, ProductId productId, bool includeMetadata, LanguageCode language, FilterType filterType, AchievementList& result)
    {
        GetAchievementsRequest request;
        user.copyInto(request.getUser());
        request.setProductId(productId);
        request.setIncludeMetadata(includeMetadata);
        request.setLanguage(language);
        request.setLimit(filterType);

        return getAchievements(request, result);
    }

    bool AchievementsUtil::getAchievements(const GetAchievementsRequest& request, AchievementList& result)
    {
        if (initialize() == false)
        {
            return false;
        }

        GetAchievementsResponse response;

        if (mComponent->getAchievements(request, response) != Blaze::ERR_OK)
        {
            return false;
        }

        response.getAchievements().copyInto(result);
        return true;
    }

    bool AchievementsUtil::getAchievement(User user, ProductId productId, bool includeMetadata, LanguageCode language, FilterType filterType, AchievementId achieveId, AchievementData& result)
    {
        GetAchievementsRequest request;
        user.copyInto(request.getUser());
        request.setProductId(productId);
        request.setIncludeMetadata(includeMetadata);
        request.setLanguage(language);
        request.setLimit(filterType);

        return getAchievement(request, achieveId, result);
    }

    bool AchievementsUtil::getAchievement(const GetAchievementsRequest& request, AchievementId achieveId, AchievementData& result)
    {
        AchievementList achievementList;
        if (getAchievements(request, achievementList) == false)
            return false;

        AchievementList::const_iterator iter = achievementList.find(achieveId);
        if (iter == achievementList.end())
        {
            return false;
        }

        iter->second->copyInto(result);
        return true;
    }

    bool AchievementsUtil::grantAchievement(User user, ProductId productId, AchievementId achieveId, Points points, Points current, LanguageCode language, AchievementData& result)
    {
        GrantAchievementRequest request;
        user.copyInto(request.getUser());
        request.setProductId(productId);
        request.setAchieveId(achieveId);
        request.getProgress().setPoints(points);
        request.getProgress().setCurrent(current);
        request.setLanguage(language);
        
        return grantAchievement(request, result);
    }

    bool AchievementsUtil::grantAchievement(const GrantAchievementRequest& request, AchievementData& result)
    {
        if (initialize() == false)
        {
            return false;
        }

        if (mComponent->grantAchievement(request, result) != Blaze::ERR_OK)
        {
            return false;
        }

        return true;
    }

    bool AchievementsUtil::postEvents(User user, ProductId productId, EventsPayload& payload)
    {
        PostEventsRequest request;
        user.copyInto(request.getUser());
        request.setProductId(productId);
        payload.copyInto(request.getPayload());

        return postEvents(request);
    }

    bool AchievementsUtil::postEvents(const PostEventsRequest& request)
    {
        if (initialize() == false)
        {
            return false;
        }

        Blaze::Achievements::GetAchievementsResponse response;
        if (mComponent->postEvents(request, response) != Blaze::ERR_OK)
        {
            return false;
        }

        return true;
    }
#endif

} //namespace GameReporting
} //namespace Blaze
