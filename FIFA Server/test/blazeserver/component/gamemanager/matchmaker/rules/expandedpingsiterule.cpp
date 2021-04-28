/*! ************************************************************************************************/
/*!
    \file   expandedpingsiterule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/expandedpingsiterule.h"

#include <EASTL/sort.h>
#include <EASTL/numeric.h>
#include <EAStdC/EAFixedPoint.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    //! Note: you must initialize each PingSiteRule before using it.
    ExpandedPingSiteRule::ExpandedPingSiteRule(const ExpandedPingSiteRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(ruleDefinition, matchmakingAsyncStatus),
          mBestPingSiteAlias(""),
          mSessionMatchCalcMethod(INVALID_SESSION_CALC_METHOD),
          mPingSiteSelectionMethod(INVALID_PING_SITE_SELECTION_METHOD),
          mIsGroupExactMatch(false),
          mAcceptUnknownPingSite(false)
    {
        TRACE_LOG("[ExpandedPingSiteRule].ctor");
    }

    ExpandedPingSiteRule::ExpandedPingSiteRule(const ExpandedPingSiteRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(otherRule, matchmakingAsyncStatus),
          mBestPingSiteAlias(otherRule.mBestPingSiteAlias),
          mSessionMatchCalcMethod(otherRule.mSessionMatchCalcMethod),
          mPingSiteSelectionMethod(otherRule.mPingSiteSelectionMethod),
          mIsGroupExactMatch(otherRule.mIsGroupExactMatch),
          mAcceptUnknownPingSite(otherRule.mAcceptUnknownPingSite)
    {
        otherRule.mPingSiteWhitelist.copyInto(mPingSiteWhitelist);
        otherRule.mComputedLatencyByPingSite.copyInto(mComputedLatencyByPingSite);
        mLatencyByAcceptedPingSiteMap.insert(otherRule.getLatencyByAcceptedPingSiteMap().begin(), otherRule.getLatencyByAcceptedPingSiteMap().end());
    }

    ExpandedPingSiteRule::~ExpandedPingSiteRule() {}

    bool ExpandedPingSiteRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        const ExpandedPingSiteRuleCriteria &ruleCriteria = criteriaData.getExpandedPingSiteRuleCriteria();
        const char8_t* listName = ruleCriteria.getRangeOffsetListName();
        bool dedicatedServerLookup = (blaze_stricmp(listName, ExpandedPingSiteRuleDefinition::DEDICATED_SERVER_MATCH_ANY_THRESHOLD) == 0);

        // special range offset list for findDedicatedServers w/ allowMismatchedPingSites = true; effectively disables this rule
        // (an "INF" range offset list would be too strict because even dedicated servers with unknown/invalid ping sites should match)
        if (dedicatedServerLookup && ruleCriteria.getPingSiteWhitelist().empty())  // Only accept unknown when no whitelist is used.
        {
            mAcceptUnknownPingSite = true;
        }
            

        if (matchmakingSupplementalData.mPreferredPingSite.c_str() != nullptr && matchmakingSupplementalData.mPreferredPingSite.c_str()[0] != '\0')
        {
            mBestPingSiteAlias.set(matchmakingSupplementalData.mPreferredPingSite);
        }
        else
        {
            // Determine the group's best ping site by simple >= 50% majority, falling back to the session Owner's best ping site
            // If we have our simple majority, the resultant ping site will be considered an exact match
            typedef eastl::hash_map<const char8_t*, uint32_t, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > PreferredPingSiteCountMap;
            PreferredPingSiteCountMap preferredPingSiteCountMap;
            float numMembers = (float)matchmakingSupplementalData.mMembersUserSessionInfo->size();
            uint32_t majorityThreshold = (uint32_t)ceil(numMembers / 2);
            UserSessionInfoList::const_iterator userIter = matchmakingSupplementalData.mMembersUserSessionInfo->begin();
            UserSessionInfoList::const_iterator userEnd = matchmakingSupplementalData.mMembersUserSessionInfo->end();
            for (; userIter != userEnd; ++userIter)
            {
                const char8_t* possiblePingSite = (*userIter)->getBestPingSiteAlias();
                PreferredPingSiteCountMap::iterator itr = preferredPingSiteCountMap.insert(eastl::make_pair(possiblePingSite, 0)).first;
                if (++itr->second >= majorityThreshold)
                {
                    mBestPingSiteAlias.set(possiblePingSite);
                    mIsGroupExactMatch = true;
                    break;
                }
            }

            if (userIter == userEnd)
            {
                // We didn't get a majority for a ping site, so use the owner's
                mBestPingSiteAlias.set(matchmakingSupplementalData.mPrimaryUserInfo->getBestPingSiteAlias());
            }
        }

        // Acceptable pingsites are either sent from the request, of use all QoS values:
        if (!ruleCriteria.getPingSiteWhitelist().empty())
        {
            ruleCriteria.getPingSiteWhitelist().copyInto(mPingSiteWhitelist);
        }
        else if (!gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().empty())
        {
            for (auto& pingSiteIter : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
            {
                mPingSiteWhitelist.push_back(pingSiteIter.first);
            }
        }
        else
        {
            // to mitigate if S2S fails, just assume client UED map has the valid ping sites for this request
            for (const auto userItr : *matchmakingSupplementalData.mMembersUserSessionInfo)
            {
                for (auto& pingSiteIter : userItr->getLatencyMap())
                {
                    if (!pingSiteIter.first.empty() && 
                        (blaze_stricmp(pingSiteIter.first.c_str(), UNKNOWN_PINGSITE) != 0) &&
                        (blaze_stricmp(pingSiteIter.first.c_str(), INVALID_PINGSITE) != 0))
                    {
                        mPingSiteWhitelist.push_back(pingSiteIter.first);
                    }
                }
            }
            WARN_LOG("[ExpandedPingSiteRule].initialize: could not populate ping site white list from QoS service. Rule will white list based on (" << mPingSiteWhitelist.size() << ") ping sites found in users' UED instead.");
        }

        bool foundBestAlias = false;
        for (auto& pingSiteIter : mPingSiteWhitelist)
        {
            if (mBestPingSiteAlias == pingSiteIter)
                foundBestAlias = true;
        }
        if (foundBestAlias == false && !mPingSiteWhitelist.empty())
        {
            // If the best alias wasn't in the list, pick the best one that was:
            // (Start with a random site, to avoid stuffing everyone in the first whitelisted pingsite.)
            mBestPingSiteAlias = mPingSiteWhitelist[Random::getRandomNumber(mPingSiteWhitelist.size())];
            int32_t latency = INT_MAX;
            for (auto& pingSiteIter : mPingSiteWhitelist)
            {
                const auto& iter = matchmakingSupplementalData.mPrimaryUserInfo->getLatencyMap().find(pingSiteIter);
                if (iter != matchmakingSupplementalData.mPrimaryUserInfo->getLatencyMap().end() && iter->second < latency && iter->second >= 0)   // Ignore negative latencies (invalid sites)
                {
                    mBestPingSiteAlias = iter->first;
                    latency = iter->second;
                }
            }
        }


        // Check that valid values are used, and if not, set some defaults:
        StringBuilder errorBuf;

        // For gamebrowsing, all valid latency calc methods are equivalent since only the browsing user's latencies are considered.
        // Pick a default method so the user doesn't need to specify one.
        LatencyCalcMethod latencyCalcMethod = matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser() ? LatencyCalcMethod::BEST_LATENCY : ruleCriteria.getLatencyCalcMethod();
        if (latencyCalcMethod == INVALID_LATENCY_CALC_METHOD)
        {
            TRACE_LOG("[ExpandedPingSiteRule].initialize: Missing latencyCalcMethod, using WORST_LATENCY (a pingsite is only as good as the group's slowest-player's ping to it) as a default.");
            latencyCalcMethod = WORST_LATENCY;
        }

        mSessionMatchCalcMethod = ruleCriteria.getSessionMatchCalcMethod();
        if (mSessionMatchCalcMethod == INVALID_SESSION_CALC_METHOD)
        {
            if (!matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser())
                TRACE_LOG("[ExpandedPingSiteRule].initialize: Missing mSessionMatchCalcMethod, using MY_BEST (my session's best pingsite is used when determining fit score) as a default.");
            mSessionMatchCalcMethod = MY_BEST;
        }

        mPingSiteSelectionMethod = ruleCriteria.getPingSiteSelectionMethod();
        if (mPingSiteSelectionMethod == INVALID_PING_SITE_SELECTION_METHOD)
        {
            if (!matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser())
                TRACE_LOG("[ExpandedPingSiteRule].initialize: Missing mPingSiteSelectionMethod, using AVERAGE_FOR_ALL_PLAYERS (use the ping site that has the lowest average latency) as a default.");
            mPingSiteSelectionMethod = AVERAGE_FOR_ALL_PLAYERS;
        }


        GroupLatencyCalculator groupLatencyCalc = getGroupLatencyCalculator(latencyCalcMethod);
        if (groupLatencyCalc == nullptr)
        {
            errorBuf << "[ExpandedPingSiteRule].initialize: Missing getGroupLatencyCalculator for latencyCalcMethod (" << latencyCalcMethod << ").  This value must be set for the expanded pingsite to work.";
            err.setErrMessage(errorBuf.get());
            WARN_LOG(errorBuf.get());
            return false;
        }
        if (getSessionMatchFitPercentCalculator(mSessionMatchCalcMethod) == nullptr)
        {
            errorBuf << "[ExpandedPingSiteRule].initialize: Missing SessionMatchFitPercentCalculator for mSessionMatchCalcMethod (" << mSessionMatchCalcMethod << ").  This value must be set for the expanded pingsite to work.";
            err.setErrMessage(errorBuf.get());
            WARN_LOG(errorBuf.get());
            return false;
        }
        if (getPingSiteOrderCalculator(mPingSiteSelectionMethod) == nullptr)
        {
            errorBuf << "[ExpandedPingSiteRule].initialize: Missing PingSiteOrderCalculator for mPingSiteSelectionMethod (" << mPingSiteSelectionMethod << ").  This value must be set for the expanded pingsite to work.";
            err.setErrMessage(errorBuf.get());
            WARN_LOG(errorBuf.get());
            return false;
        }

        // Calculate the latency for the group using the MM criteria. We'll use this latency for
        // calculating fit scores and choosing which ping site we select in the case of dedicated servers
        if (!dedicatedServerLookup || mAcceptUnknownPingSite)
        {
            groupLatencyCalc(*this, matchmakingSupplementalData);
        }
        else
        {
            // Special case for DS lookup - Since we don't have the full group information at this point, we just use the pingsite information that's provided.
            // The order should be the same as the priority, so we just set the values to 1.  
            // (An alternative would be to pass around the pingsite latencies here, rather than a building an ordered whitelist.)
            int32_t fakePing = 1;
            for (auto& iter : mPingSiteWhitelist)
            {
                const PingSiteAlias& possiblePingSite = iter;
                mComputedLatencyByPingSite[possiblePingSite] = fakePing;
                ++fakePing;
            }
        }


        bool isOmitted = mBestPingSiteAlias.c_str() == nullptr || mBestPingSiteAlias.empty() || mComputedLatencyByPingSite.empty();
        if (!isOmitted)
        {
            // validate the listName for the rangeOffsetList
            // NOTE: nullptr is expected when the client disables the rule
            mRangeOffsetList = getDefinition().getRangeOffsetList(listName);

            // If the rule is specified by the client it must contain a range offset list -
            // unless we're game browsing, in which case a nullptr range offset list will disable the rule but is not an error
            if (mRangeOffsetList == nullptr)
            {
                if (matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser())
                    return true;

                // Only return an error when the threshold was invalid, not just unset.
                if (listName[0] != '\0')
                {
                    errorBuf << "Rule Name '" << ((getRuleName() != nullptr) ? getRuleName() : "<unavailable>") << "' criteria contains invalid range list name '" << ((listName != nullptr) ? listName : "<rangeOffsetList name null>") << "', disabling rule.";
                    err.setErrMessage(errorBuf.get());
                    return false;
                }
            }
        }

        updateAsyncStatus();

        TRACE_LOG("[ExpandedPingSiteRule].initialize: initialized with best ping site(" << mBestPingSiteAlias.c_str() << "), white list size(" << mPingSiteWhitelist.size() << ")");
        return true;
    }

    Rule* ExpandedPingSiteRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW ExpandedPingSiteRule(*this, matchmakingAsyncStatus);
    }

    bool ExpandedPingSiteRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        TRACE_LOG("[ExpandedPingSiteRule].addConditions.");
        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

        // Add the possible ping sites that we match at this threshold
        // This search applies to both the dedicated server and base search.
        //
        // Ex. If the search is for sjc but the current threshold matches sjc and
        // iad, then both sjc and iad should be added at their appropriate fitScores.
        for (auto& iter : mPingSiteWhitelist)
        {
            const PingSiteAlias& possiblePingSite = iter;
            float fitPercent;
            bool isExactMatch;

            calcFitPercentInternal(possiblePingSite, fitPercent, isExactMatch);
            FitScore fitScore = calcWeightedFitScore(isExactMatch, fitPercent);

            if (isBestPingSiteAlias(possiblePingSite))
            {
                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(ExpandedPingSiteRuleDefinition::EXPANDED_PING_SITE_RULE_DEFINITION_RETE_NAME, getDefinition().getWMEAttributeValue(possiblePingSite), getDefinition()), 
                    fitScore, this));
            }
            else if (isFitScoreMatch(fitScore))
            {
                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(ExpandedPingSiteRuleDefinition::EXPANDED_PING_SITE_RULE_DEFINITION_RETE_NAME, getDefinition().getWMEAttributeValue(possiblePingSite), getDefinition()), 
                    fitScore, this));
            }
        }

        if (mAcceptUnknownPingSite)
        {
            FitScore fitScore = 0;
            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(ExpandedPingSiteRuleDefinition::UNKNOWN_PING_SITE_RULE_DEFINITION_RETE_NAME, getDefinition().getWMEBooleanAttributeValue(true), getDefinition()),
                fitScore, this));
        }

        return true;
    }

    void ExpandedPingSiteRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        TRACE_LOG("[ExpandedPingSiteRule].calcFitPercent this pingSiteAlias '" << mBestPingSiteAlias.c_str() << "', game '" 
            << gameSession.getGameId() << "' pingSiteAlias '" << gameSession.getBestPingSiteAlias() << "'.\n");

        const char8_t* gamePingSite = gameSession.getBestPingSiteAlias();
        calcFitPercentInternal(gamePingSite, fitPercent, isExactMatch);
    }

    void ExpandedPingSiteRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const ExpandedPingSiteRule& otherPingSiteRule = static_cast<const ExpandedPingSiteRule&>(otherRule);

        // Calculate and return the best fit percent across all ping sites
        fitPercent = FIT_PERCENT_NO_MATCH;
        isExactMatch = false;
        float tempFitPercent;
        for (PingSiteLatencyByAliasMap::const_iterator itr = mComputedLatencyByPingSite.begin(), end = mComputedLatencyByPingSite.end();
             itr != end;
             ++itr)
        {
            // Other rule must accept this ping site
            if (isPingSiteAcceptable(itr->first, mAcceptableRange, otherPingSiteRule, itr->second))
            {
                if (isBestPingSiteAlias(itr->first))
                    isExactMatch = mIsGroupExactMatch;

                tempFitPercent = getDefinition().getFitPercent(itr->second);
                TRACE_LOG("[ExpandedPingSiteRule].calcFitPercent: My latency (" << itr->second << "ms) generated fitPercent (" << tempFitPercent << ", exactmatch: " << (isExactMatch ? "true" : "false"));
                if (tempFitPercent > fitPercent)
                {
                    fitPercent = tempFitPercent;
                }
            }
        }
    }

    void ExpandedPingSiteRule::calcFitPercentInternal(const PingSiteAlias& pingSite, float& fitPercent, bool& isExactMatch) const
    {
        calcFitPercentInternal(pingSite, mAcceptableRange, fitPercent, isExactMatch);
    }

    void ExpandedPingSiteRule::calcFitPercentInternal(const PingSiteAlias& pingSite, const RangeList::Range& acceptableRange, float& fitPercent, bool& isExactMatch) const
    {
        fitPercent = FIT_PERCENT_NO_MATCH;
        isExactMatch = false;
        uint32_t latency;
        bool found = getLatencyForPingSite(pingSite, latency);
        if (found)
        {
            bool bestPingSiteAlias = isBestPingSiteAlias(pingSite);
            if (bestPingSiteAlias)
                isExactMatch = mIsGroupExactMatch;

            if (bestPingSiteAlias || isInRange(acceptableRange, latency))
                fitPercent = getDefinition().getFitPercent(latency);
        }
        else
        {
            TRACE_LOG("[ExpandedPingSiteRule].calcFitPercentInternal: Failed to look up latency for ping site " << pingSite);

            // If we accept unknown pingsites, then accept it here too:
            if (mAcceptUnknownPingSite)
                fitPercent = 0;
        }
    }

    bool ExpandedPingSiteRule::getLatencyForPingSite(const PingSiteAlias& pingSite, uint32_t& latency) const
    {
        PingSiteLatencyByAliasMap::const_iterator iter = mComputedLatencyByPingSite.find(pingSite);
        if (iter != mComputedLatencyByPingSite.end())
        {
             latency = static_cast<uint32_t>(iter->second);
             return true;
        }

        return false;
    }

    bool ExpandedPingSiteRule::isPingSiteAcceptable(const PingSiteAlias& pingSite, const RangeList::Range& range, const ExpandedPingSiteRule& otherRule, uint32_t myLatency) const
    {
        uint32_t otherLatency = UINT32_MAX;
        if (otherRule.getLatencyForPingSite(pingSite, otherLatency))
        {
            return (isInRange(range, otherLatency) || otherRule.isBestPingSiteAlias(pingSite)) && (isInRange(range, myLatency) || isBestPingSiteAlias(pingSite));
        }

        return false;
    }

    bool ExpandedPingSiteRule::isInRange(const RangeList::Range& range, uint32_t latency) const
    {
        return range.isInRange(latency) || (latency <= getDefinition().getMinLatencyOverride());
    }

    UpdateThresholdResult ExpandedPingSiteRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        TRACE_LOG("[ExpandedPingSiteRule].updateCachedThresholds");
        // If the rule is disabled we don't need to decay.
        if (isDisabled())
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        const RangeList::Range *originalRangeOffset = mCurrentRangeOffset;
        if (mRangeOffsetList != nullptr)
            mCurrentRangeOffset = mRangeOffsetList->getRange(elapsedSeconds, &mNextUpdate);

        if ((originalRangeOffset != mCurrentRangeOffset) || forceUpdate)
        {
            if (mCurrentRangeOffset != nullptr)
            {
                if (originalRangeOffset != nullptr)
                {
                    TRACE_LOG("[ExpandedPingSiteRule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                        << "' elapsedSeconds '" << elapsedSeconds << "' previousRange: " << originalRangeOffset->mMinValue << ".." 
                        << originalRangeOffset->mMaxValue << " currentRange: " << mCurrentRangeOffset->mMinValue << ".." 
                        << mCurrentRangeOffset->mMaxValue);
                }
                else
                {
                    TRACE_LOG("[ExpandedPingSiteRule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                        << "' elapsedSeconds '" << elapsedSeconds << "' currentRange: " << mCurrentRangeOffset->mMinValue << ".." 
                        << mCurrentRangeOffset->mMaxValue);
                }

                updateAcceptableRange();

                // Update our list of acceptable ping sites for our new range
                mLatencyByAcceptedPingSiteMap.clear();
                for (auto& iter : mPingSiteWhitelist)
                {
                    const PingSiteAlias& possiblePingSite = iter;
                    uint32_t latency = UINT32_MAX;
                    bool found = getLatencyForPingSite(possiblePingSite, latency);

                    if (isBestPingSiteAlias(possiblePingSite))
                    {
                        // Always include our best ping site, regardless of latency
                        mLatencyByAcceptedPingSiteMap[possiblePingSite] = latency;
                    }
                    else if (found && isInRange(mAcceptableRange, latency))
                    {
                        mLatencyByAcceptedPingSiteMap[possiblePingSite] = latency;
                    }
                }
            }

            updateAsyncStatus();
            return RULE_CHANGED;
        } 

        return NO_RULE_CHANGE;
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    ****************************************************************************************************/
    void ExpandedPingSiteRule::updateAsyncStatus() 
    {
        TRACE_LOG("[ExpandedPingSiteRule].updateAsyncStatus: this sessions best ping site is [" << mBestPingSiteAlias.c_str() << "].\n");

        if (isDisabled())
            return;

        if (mMatchmakingAsyncStatus == nullptr)
            return;

        ExpandedPingSiteRuleStatus& status = mMatchmakingAsyncStatus->getExpandedPingSiteRuleStatus();
        status.getMatchedValues().clear();

        LatencyByAcceptedPingSiteMap::const_iterator itr = mLatencyByAcceptedPingSiteMap.begin();
        LatencyByAcceptedPingSiteMap::const_iterator end = mLatencyByAcceptedPingSiteMap.end();
        for (; itr != end; ++itr)
        {
            status.getMatchedValues().push_back(itr->first);
        }

        status.setMaxLatency(static_cast<uint32_t>(mAcceptableRange.mMaxValue));
    }

    void ExpandedPingSiteRule::updateAcceptableRange()
    {
        mAcceptableRange.setRange(RANGE_OFFSET_CENTER_VALUE, *mCurrentRangeOffset, 0);
    }

    // Override this from Rule since we use range thresholds instead of fit thresholds
    void ExpandedPingSiteRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        const ExpandedPingSiteRule& otherExpandedPingSiteRule = static_cast<const ExpandedPingSiteRule&>(otherRule);

        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[ExpandedPingSiteRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
            EA_ASSERT(mRangeOffsetList != nullptr);
            return;
        }

        //Check exact match
        if (evalInfo.mIsExactMatch &&
            mCurrentRangeOffset != nullptr &&
            mCurrentRangeOffset->isExactMatchRequired())
        {
            TRACE_LOG("[ExpandedPingSiteRule].calcSessionMatchInfo: Exact match required and found.");
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight()), 0);
            return;
        }

        SessionMatchFitPercentCalculator fitPercentCalculator = getSessionMatchFitPercentCalculator(mSessionMatchCalcMethod);
        if (fitPercentCalculator != nullptr)
        {
            float fitPercent = FIT_PERCENT_NO_MATCH;
            uint32_t matchSeconds = 0;
            fitPercentCalculator(const_cast<ExpandedPingSiteRule&>(*this), otherExpandedPingSiteRule, fitPercent, matchSeconds);

            if (fitPercent != FIT_PERCENT_NO_MATCH)
            {
                FitScore fitScore = static_cast<FitScore>(fitPercent * getDefinition().getWeight());
                matchInfo.setMatch(fitScore, matchSeconds);
                TRACE_LOG("[ExpandedPingSiteRule].calcSessionMatchInfo: Match found at time (" << matchSeconds << "s) with fit percent (" << fitPercent << ").");
                return;
            }
        }

        TRACE_LOG("[ExpandedPingSiteRule].calcSessionMatchInfo: No match found.");
        matchInfo.setNoMatch();
    }

    void ExpandedPingSiteRule::buildAcceptedPingSiteList(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection,
        OrderedPreferredPingSiteList& orderedPreferredPingSites) const
    {
        PingSiteOrderCalculator calculator = getPingSiteOrderCalculator(mPingSiteSelectionMethod);
        if (calculator != nullptr)
        {
            calculator(pingSiteIntersection, orderedPreferredPingSites);
            eastl::quick_sort(orderedPreferredPingSites.begin(), orderedPreferredPingSites.end());
        }
    }

    // Factory class for group latency calculation
    ExpandedPingSiteRule::GroupLatencyCalculator ExpandedPingSiteRule::getGroupLatencyCalculator(LatencyCalcMethod method)
    {
        switch (method)
        {
        case BEST_LATENCY:
            return calcLatencyForGroupBestLatency;
        case WORST_LATENCY:
            return calcLatencyForGroupWorstLatency;
        case AVERAGE_LATENCY:
            return calcLatencyForGroupAverageLatency;
        case MEDIAN_LATENCY:
            return calcLatencyForGroupMedianLatency;
        default:
            return nullptr;
        }
    }

    void ExpandedPingSiteRule::calcLatencyForGroupBestLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData)
    {
        for (auto& iter : owner.mPingSiteWhitelist)
        {
            const PingSiteAlias& possiblePingSite = iter;

            uint32_t groupLatency = UINT32_MAX;
            UserSessionInfoList::const_iterator userIter = matchmakingSupplementalData.mMembersUserSessionInfo->begin();
            UserSessionInfoList::const_iterator userEnd = matchmakingSupplementalData.mMembersUserSessionInfo->end();
            for (; userIter != userEnd; ++userIter)
            {
                PingSiteLatencyByAliasMap::const_iterator latItr = (*userIter)->getLatencyMap().find(possiblePingSite.c_str());
                if (latItr != (*userIter)->getLatencyMap().end())
                {
                    uint32_t latency = latItr->second;
                    if (latency < groupLatency && latItr->second >= 0)  // Ignore negative latencies (invalid sites)
                        groupLatency = latency;
                }
            }
            owner.mComputedLatencyByPingSite[possiblePingSite] = groupLatency;
        }
    }

    void ExpandedPingSiteRule::calcLatencyForGroupWorstLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData)
    {
        for (auto& iter : owner.mPingSiteWhitelist)
        {
            const PingSiteAlias& possiblePingSite = iter;

            uint32_t groupLatency = 0;
            UserSessionInfoList::const_iterator userIter = matchmakingSupplementalData.mMembersUserSessionInfo->begin();
            UserSessionInfoList::const_iterator userEnd = matchmakingSupplementalData.mMembersUserSessionInfo->end();
            for (; userIter != userEnd; ++userIter)
            {
                PingSiteLatencyByAliasMap::const_iterator latItr = (*userIter)->getLatencyMap().find(possiblePingSite.c_str());
                if (latItr != (*userIter)->getLatencyMap().end())
                {
                    uint32_t latency = latItr->second;
                    if (latency > groupLatency && latItr->second >= 0)  // Ignore negative latencies (invalid sites)
                        groupLatency = latency;
                }
                else
                {
                    groupLatency = UINT32_MAX;
                    break;
                }
            }
            owner.mComputedLatencyByPingSite[possiblePingSite] = groupLatency;
        }
    }

    void ExpandedPingSiteRule::calcLatencyForGroupAverageLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData)
    {
        for (auto& iter : owner.mPingSiteWhitelist)
        {
            const PingSiteAlias& possiblePingSite = iter;

            uint32_t latencyCount = 0;
            uint32_t groupLatency = 0;
            UserSessionInfoList::const_iterator userIter = matchmakingSupplementalData.mMembersUserSessionInfo->begin();
            UserSessionInfoList::const_iterator userEnd = matchmakingSupplementalData.mMembersUserSessionInfo->end();
            for (; userIter != userEnd; ++userIter)
            {
                PingSiteLatencyByAliasMap::const_iterator latItr = (*userIter)->getLatencyMap().find(possiblePingSite.c_str());
                if (latItr != (*userIter)->getLatencyMap().end() && latItr->second >= 0)  // Ignore negative latencies (invalid sites)
                    groupLatency += latItr->second;
                else
                    groupLatency += UINT32_MAX;
                ++latencyCount;
            }
            if (latencyCount == 0)
                owner.mComputedLatencyByPingSite[possiblePingSite] = UINT32_MAX;
            else
                owner.mComputedLatencyByPingSite[possiblePingSite] = (groupLatency / latencyCount);
        }
    }

    void ExpandedPingSiteRule::calcLatencyForGroupMedianLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData)
    {
        for (auto& iter : owner.mPingSiteWhitelist)
        {
            const PingSiteAlias& possiblePingSite = iter;

            PingSiteLatencyList list;
            UserSessionInfoList::const_iterator userIter = matchmakingSupplementalData.mMembersUserSessionInfo->begin();
            UserSessionInfoList::const_iterator userEnd = matchmakingSupplementalData.mMembersUserSessionInfo->end();
            for (; userIter != userEnd; ++userIter)
            {
                PingSiteLatencyByAliasMap::const_iterator latItr = (*userIter)->getLatencyMap().find(possiblePingSite.c_str());
                if (latItr != (*userIter)->getLatencyMap().end() && latItr->second >= 0)  // Ignore negative latencies (invalid sites)
                    list.push_back(latItr->second);
                else
                    list.push_back(UINT32_MAX);
            }
            if (!list.empty())
            {
                eastl::quick_sort(list.begin(), list.end()); // Array must be sorted for median
                PingSiteLatencyList::size_type halfListSize = (list.size()/2);
                eastl::nth_element(list.begin(), list.begin() + halfListSize, list.end());
                owner.mComputedLatencyByPingSite[possiblePingSite] = list[halfListSize];
            }
            else
            {
                owner.mComputedLatencyByPingSite[possiblePingSite] = UINT32_MAX;
            }
        }
    }

    // Factory class for session match fit calculation
    ExpandedPingSiteRule::SessionMatchFitPercentCalculator ExpandedPingSiteRule::getSessionMatchFitPercentCalculator(SessionMatchCalcMethod method)
    {
        switch (method)
        {
        case MY_BEST:
            return calcSessionMatchFitPercentMyBest;
        case MUTUAL_BEST:
            return calcSessionMatchFitPercentMutualBest;
        default:
            return nullptr;
        }
    }

    void ExpandedPingSiteRule::calcSessionMatchFitPercentMyBest(ExpandedPingSiteRule& owner, const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds)
    {
        bestFitPercent = FIT_PERCENT_NO_MATCH;

        RangeList::Range acceptableRange;
        const RangeList::RangePairContainer &rangePairContainer = owner.mRangeOffsetList->getContainer();
        RangeList::RangePairContainer::const_iterator rangeIter = rangePairContainer.begin();
        RangeList::RangePairContainer::const_iterator rangeEnd = rangePairContainer.end();
        for (; rangeIter != rangeEnd; ++rangeIter)
        {
            matchSeconds = rangeIter->first;
            const RangeList::Range &rangeOffset = rangeIter->second;
            acceptableRange.setRange(RANGE_OFFSET_CENTER_VALUE, rangeOffset, 0);

            for (auto& iter : owner.mPingSiteWhitelist)
            {
                const PingSiteAlias& possiblePingSite = iter;

                uint32_t myLatency = 0, otherLatency = 0;
                bool found = owner.getLatencyForPingSite(possiblePingSite, myLatency);
                found &= otherExpandedPingSiteRule.getLatencyForPingSite(possiblePingSite, otherLatency);
                if (found)
                {
                    if (owner.isPingSiteAcceptable(possiblePingSite, acceptableRange, otherExpandedPingSiteRule, myLatency))
                    {
                        TRACE_LOG("[MyBestFitPercentCalculator].calcSessionMatchFitPercent: Ping site " << possiblePingSite.c_str() << " in range for both sessions with latency (" <<
                            myLatency << "ms), other latency (" << otherLatency << "ms).")
                        float fitPercent;
                        bool isExactMatch;
                        owner.calcFitPercentInternal(possiblePingSite, acceptableRange, fitPercent, isExactMatch);
                        if (fitPercent > bestFitPercent)
                            bestFitPercent = fitPercent;
                    }
                }
            }

            if (bestFitPercent != FIT_PERCENT_NO_MATCH)
                return;
        }
    }

    void ExpandedPingSiteRule::calcSessionMatchFitPercentMutualBest(ExpandedPingSiteRule& owner, const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds)
    {
        bestFitPercent = FIT_PERCENT_NO_MATCH;

        RangeList::Range acceptableRange;
        const RangeList::RangePairContainer &rangePairContainer = owner.mRangeOffsetList->getContainer();
        RangeList::RangePairContainer::const_iterator rangeIter = rangePairContainer.begin();
        RangeList::RangePairContainer::const_iterator rangeEnd = rangePairContainer.end();
        for (; rangeIter != rangeEnd; ++rangeIter)
        {
            matchSeconds = rangeIter->first;
            const RangeList::Range &rangeOffset = rangeIter->second;
            acceptableRange.setRange(RANGE_OFFSET_CENTER_VALUE, rangeOffset, 0);

            LatenciesByAcceptedPingSiteIntersection intersection;
            for (auto& iter : owner.mPingSiteWhitelist)
            {
                const PingSiteAlias& possiblePingSite = iter;

                uint32_t myLatency = 0, otherLatency = 0;
                bool found = owner.getLatencyForPingSite(possiblePingSite, myLatency);
                found &= otherExpandedPingSiteRule.getLatencyForPingSite(possiblePingSite, otherLatency);
                if (found)
                {
                    if (owner.isPingSiteAcceptable(possiblePingSite, acceptableRange, otherExpandedPingSiteRule, myLatency))
                    {
                        TRACE_LOG("[MutualBestFitPercentCalculator].calcSessionMatchFitPercent: Ping site " << possiblePingSite << " is mutually agreeable for both sessions with latency (" <<
                            myLatency << "ms), other latency (" << otherLatency << "ms).")
                        intersection[possiblePingSite].push_back(myLatency);
                        intersection[possiblePingSite].push_back(otherLatency);
                    }
                }
            }

            OrderedPreferredPingSiteList orderedList;
            owner.buildAcceptedPingSiteList(intersection, orderedList);

            bool isExactMatch;
            if (!orderedList.empty())
            {
                // We have at least one ping site acceptable to both sessions
                // Use the first in the list since it is the ping site we'll during finalization
                TRACE_LOG("[MutualBestFitPercentCalculator].calcSessionMatchFitPercent: Ping site " << orderedList[0].second.c_str() << " is mutually agreeable for both sessions with latency (" <<
                    intersection[orderedList[0].second][0] << "ms) at time (" << matchSeconds << "s).")
                owner.calcFitPercentInternal(orderedList[0].second, acceptableRange, bestFitPercent, isExactMatch);
                if (owner.isBestPingSiteAlias(orderedList[0].second) && bestFitPercent == FIT_PERCENT_NO_MATCH)
                    bestFitPercent = 0.0f;
                return;
            }
        }
    }

    // Factory class for ping site order calculation
    ExpandedPingSiteRule::PingSiteOrderCalculator ExpandedPingSiteRule::getPingSiteOrderCalculator(PingSiteSelectionMethod method)
    {
        switch (method)
        {
        case STANDARD_DEVIATION:
            return calcPingSiteOrderStandardDeviation;
        case BEST_FOR_FASTEST_PLAYER:
            return calcPingSiteOrderBest;
        case BEST_FOR_SLOWEST_PLAYER:
            return calcPingSiteOrderWorst;
        case AVERAGE_FOR_ALL_PLAYERS:
            return calcPingSiteOrderAverage;
        default:
            return nullptr;
        }
    }

    void ExpandedPingSiteRule::calcPingSiteOrderStandardDeviation(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites) 
    {
        for (auto& iter : pingSiteIntersection)
        {
            const eastl::vector<uint32_t>& latencies = iter.second;
            eastl::vector<double> temp;
            double sum = eastl::accumulate(latencies.begin(), latencies.end(), 0.0);
            double mean = sum / latencies.size();

            double accum = 0.0;
            eastl::for_each(latencies.begin(), latencies.end(), [&](const double d) { accum += (d - mean) * (d - mean); });
            double stdev = sqrt(accum / (latencies.size()-1));
            orderedPreferredPingSites.push_back(eastl::make_pair((uint32_t)stdev, iter.first));
        }
    }

    void ExpandedPingSiteRule::calcPingSiteOrderBest(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites) 
    {
        for (auto& iter : pingSiteIntersection)
        {
            uint32_t bestLatency = UINT32_MAX;
            for (auto& latencyIter : iter.second)
            {
                if (latencyIter < bestLatency)
                {
                    bestLatency = latencyIter;
                }
            }
            orderedPreferredPingSites.push_back(eastl::make_pair(bestLatency, iter.first));
        }
    }

    void ExpandedPingSiteRule::calcPingSiteOrderWorst(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites)
    {
        for (auto& iter : pingSiteIntersection)
        {
            uint32_t highestLatency = 0;
            for (auto& latencyIter : iter.second)
            {
                if (latencyIter > highestLatency)
                {
                    highestLatency = latencyIter;
                }
            }
            orderedPreferredPingSites.push_back(eastl::make_pair(highestLatency, iter.first));
        }
    }

    void ExpandedPingSiteRule::calcPingSiteOrderAverage(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites) 
    {
        for (auto& iter : pingSiteIntersection)
        {
            const eastl::vector<uint32_t>& latencies = iter.second;
            double sum = eastl::accumulate(latencies.begin(), latencies.end(), 0.0);
            double mean = sum / latencies.size();

            orderedPreferredPingSites.push_back(eastl::make_pair((uint32_t)mean, iter.first));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics by default
    ****************************************************************************************************/
    void ExpandedPingSiteRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        setupInfos.push_back(RuleDiagnosticSetupInfo("bestPingSite", mBestPingSiteAlias.c_str()));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
