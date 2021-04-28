/*! ************************************************************************************************/
/*!
    \file   platformrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/platformrule.h"
#include "gamemanager/matchmaker/rules/platformruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    PlatformRule::PlatformRule(const PlatformRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mCrossplayMustMatch(false),
        mIgnoreCallerPlatform(false)
    {
    }

    PlatformRule::PlatformRule(const PlatformRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRule(other, status),
        mCrossplayMustMatch(other.mCrossplayMustMatch),
        mIgnoreCallerPlatform(other.mIgnoreCallerPlatform),
        mPlatformsInUse(other.mPlatformsInUse),
        mPlatformListOverride(other.mPlatformListOverride)
    {
    }

    bool PlatformRule::isDisabled() const
    {
        return mPlatformsInUse.empty() && mPlatformListOverride.empty();
    }

    bool PlatformRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        const PlatformRuleDefinition &myDefn = getDefinition();

        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        for (auto& curPlatform : gController->getHostedPlatforms())
        {
            bool platformInUse = (mPlatformsInUse.find(curPlatform) != mPlatformsInUse.end());
            

            // first add conditions for the platforms present in the request
            //  otherwise we need to filter games that allow our platforms
            if (platformInUse && curPlatform != common)
            {
                Rete::OrConditionList& baseOrConditions = baseConditions.push_back();
                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(myDefn.getPlatformAllowedAttrName(curPlatform),
                        myDefn.getWMEBooleanAttributeValue(platformInUse), mRuleDefinition),
                    0, this
                ));
            }
            // now add conditions for platforms in the override list
            // If MustMatch is set, then the overrides we're using have to match the Game exactly,
            else if (mCrossplayMustMatch)
            {
                bool platformInOverrideList = (mPlatformListOverride.findAsSet(curPlatform) != mPlatformListOverride.end());
                Rete::OrConditionList& baseOrConditions = baseConditions.push_back();
                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition( myDefn.getPlatformOverrideAttrName(curPlatform),
                        myDefn.getWMEBooleanAttributeValue(platformInOverrideList), mRuleDefinition),
                    0, this
                ));
            }
        }
        // if we don't require the platform override list, add OR conditions for the platforms that aren't required (the group's platforms)
        // only do this if the override list has more entries than there are platforms for the group.
        // (Rule init guarantees that if the override is non-empty, there's no platform in-use that's not in the override list.)
        if (!mCrossplayMustMatch && (mPlatformListOverride.size() > mPlatformsInUse.size()))
        {
            Rete::OrConditionList& optionalPlatformOrConditions = baseConditions.push_back();
            for (auto curPlatform : mPlatformListOverride)
            {
                // we add an OR condition for every entry in the override list, even ones added in the previous loop
                // otherwise the rule condition becomes "AND at least one of the 'optional' extra platforms"
                optionalPlatformOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(myDefn.getPlatformOverrideAttrName(curPlatform),
                        myDefn.getWMEBooleanAttributeValue(true), mRuleDefinition),
                    0, this
                ));
            }
        }

        return true;
    }

    bool PlatformRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        const PlatformRuleCriteria &ruleCriteria = criteriaData.getPlatformRuleCriteria();

        // Overrides:
        ruleCriteria.getClientPlatformListOverride().copyInto(mPlatformListOverride);
        mCrossplayMustMatch = ruleCriteria.getCrossplayMustMatch();
        mIgnoreCallerPlatform = matchmakingSupplementalData.mIgnoreCallerPlatform;

        // Cleanup the request and remove any possible duplicates from the Override list (since we care about list size being accurate):
        mPlatformListOverride.eraseDuplicatesAsSet();

        bool usingPlatfromOverride = (!mPlatformListOverride.empty());

        // If an override list was provided, we use that exclusively: 
        if (!usingPlatfromOverride && mCrossplayMustMatch)
        {
            err.setErrMessage("Crossplay must match, but no overrides were provided.  This is not supported.");
            return false;
        }

        if (mIgnoreCallerPlatform && matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser())
        {
            return true; // we're just going to search based on the platform list override
        }

        ClientPlatformSet groupPlatforms;
        bool gotGroupCrossplayValue = false;
        bool groupCrossplayEnabled = true;
        
        // Use the host info if it's provided: 
        ClientPlatformType hostPlatform = INVALID;
        if (matchmakingSupplementalData.mPrimaryUserInfo)
        {
            hostPlatform = matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getPlatformInfo().getClientPlatform();
            groupCrossplayEnabled = UserSessionManager::isCrossplayEnabled(matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo());
            gotGroupCrossplayValue = true;
        }
        // Primary user info always set when searching by gamebrowser.  See GameList

        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // For DS Searches, we treat all override platforms as required, since they indicate a choice by the user when they created the Game.
            // Also, we don't have the group member platforms or we'd just use those to figure out what's required.
            for (auto curPlatformOverride : mPlatformListOverride)
                groupPlatforms.insert(curPlatformOverride);
        }

        if ((hostPlatform != INVALID) && (hostPlatform != common))
            groupPlatforms.insert(hostPlatform);

        // Add in settings from group members:
        for (auto userInfo : *matchmakingSupplementalData.mMembersUserSessionInfo)
        {
            groupPlatforms.insert(userInfo->getUserInfo().getPlatformInfo().getClientPlatform());
            groupCrossplayEnabled = groupCrossplayEnabled && UserSessionManager::isCrossplayEnabled(userInfo->getUserInfo());
            gotGroupCrossplayValue = true;
        }

        if (gotGroupCrossplayValue && mCrossplayMustMatch && (groupCrossplayEnabled == false))
        {
            if(mPlatformListOverride.size() > 1)
            {
                const ClientPlatformSet &nonCrossplayPlatformSet = gUserSessionManager->getNonCrossplayPlatformSet(*mPlatformListOverride.begin());
                for (auto curPlatform : mPlatformListOverride)
                {
                    if (nonCrossplayPlatformSet.find(curPlatform) == nonCrossplayPlatformSet.end())
                    {
                        err.setErrMessage("Crossplay must match and CrossplayEnabled platforms are set for the Rule, but the Members are not crossplay enabled.");
                        return false;
                    }
                }
            }
        }

        if (usingPlatfromOverride)
        {
            // Check for invalid platforms:
            for (auto curPlatform : groupPlatforms)
            {
                if (curPlatform == INVALID || curPlatform == common)
                    continue;

                bool found = false;
                for (auto curPlatformOverride : mPlatformListOverride)
                {
                    if (curPlatformOverride == curPlatform)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    err.setErrMessage("Platform override was in use, but players in group were not all in the allowed platforms. (Or common/INVALID)");
                    return false;
                }
            }
        }

        mPlatformsInUse = groupPlatforms;

        if (mPlatformsInUse.empty())
        {
            err.setErrMessage("Missing valid platforms for Platform rule.");
            return false;
        }
        
        return true;
    }

    void PlatformRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        if (mCrossplayMustMatch == false)
        {
            bool foundInvalidPlatform = false;
            for (auto curPlatform : mPlatformsInUse)
            {
                if (gameSession.getCurrentlyAcceptedPlatformList().findAsSet(curPlatform) == gameSession.getCurrentlyAcceptedPlatformList().end())
                {
                    foundInvalidPlatform = true;
                    break;
                }
            }

            if (foundInvalidPlatform)
            {
                debugRuleConditions.writeRuleCondition("crossplay platforms can't play together");
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
            else
            {
                debugRuleConditions.writeRuleCondition("crossplay settings match");
                fitPercent = 0;
                isExactMatch = true;
            }
        }
        else
        {
            // Iterate over all the platforms, and make sure they're in both lists:
            bool foundInvalidPlatform = false;
            if (gameSession.getBasePlatformList().size() == mPlatformListOverride.size())
            {
                for (auto curGamePlatform : gameSession.getBasePlatformList())
                {
                    bool foundMatchingPlat = false;
                    for (auto curRulePlatform : mPlatformListOverride)
                    {
                        if (curGamePlatform == curRulePlatform)
                        {
                            foundMatchingPlat = true;
                            break;
                        }
                    }

                    if (!foundMatchingPlat)
                    {
                        foundInvalidPlatform = true;
                        break;
                    }
                }
            }

            if (foundInvalidPlatform)
            {
                debugRuleConditions.writeRuleCondition("crossplay platforms can't play together");
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
            else
            {
                debugRuleConditions.writeRuleCondition("crossplay settings match");
                fitPercent = 0;
                isExactMatch = true;
            }
        }
    }

    void PlatformRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const PlatformRule& otherTopRule = (const PlatformRule&)otherRule;
        // MustMatch settings have to be the same, since they're going to be creating the Game from these rules:
        if (mCrossplayMustMatch != otherTopRule.mCrossplayMustMatch)
        {
            debugRuleConditions.writeRuleCondition("crossplay must match settings don't match");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
        else if (mCrossplayMustMatch == false)
        {
            // Iterate over all the platforms and verify that they are supported bi-directionally:
            bool foundMismatch = false;
            for (ClientPlatformType curPlatform : mPlatformsInUse)
            {
                // (Don't have to check Override list, since that's already the same)
                auto unrestrictedPlats = gUserSessionManager->getUnrestrictedPlatforms(curPlatform);
                for (ClientPlatformType otherCurPlatform : otherTopRule.mPlatformsInUse)
                    if (unrestrictedPlats.find(otherCurPlatform) == unrestrictedPlats.end())
                        foundMismatch = true;
            }
            for (ClientPlatformType otherCurPlatform : otherTopRule.mPlatformsInUse)
            {
                // (Don't have to check Override list, since that's already the same)
                auto unrestrictedPlats = gUserSessionManager->getUnrestrictedPlatforms(otherCurPlatform);
                for (ClientPlatformType curPlatform : mPlatformsInUse)
                    if (unrestrictedPlats.find(curPlatform) == unrestrictedPlats.end())
                        foundMismatch = true;
            }

            if (foundMismatch == false)
            {
                debugRuleConditions.writeRuleCondition("crossplay settings match");
                fitPercent = 0;
                isExactMatch = true;
            }
            else
            {
                debugRuleConditions.writeRuleCondition("crossplay platforms can't play together");
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
        }
        else
        {
            bool foundInvalidPlatform = false;
            if (otherTopRule.mPlatformListOverride.size() == mPlatformListOverride.size())
            {
                for (auto otherRulePlatform : otherTopRule.mPlatformListOverride)
                {
                    bool foundMatchingPlat = false;
                    for (auto curRulePlatform : mPlatformListOverride)
                    {
                        if (otherRulePlatform == curRulePlatform)
                        {
                            foundMatchingPlat = true;
                            break;
                        }
                    }

                    if (!foundMatchingPlat)
                    {
                        foundInvalidPlatform = true;
                        break;
                    }
                }
            }

            if (foundInvalidPlatform)
            {
                debugRuleConditions.writeRuleCondition("crossplay platforms can't play together");
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
            else
            {
                debugRuleConditions.writeRuleCondition("crossplay settings match");
                fitPercent = 0;
                isExactMatch = true;
            }

        }

    }

    // Override this for games
    FitScore PlatformRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        return FitScore(fitPercent);
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
