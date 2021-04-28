/*! ************************************************************************************************/
/*!
    \file teamuedbalancerule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchlist.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucketutil.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteaminfo.h"
#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamUEDBalanceRule::TeamUEDBalanceRule(const TeamUEDBalanceRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(definition, matchmakingAsyncStatus),
          mUEDValue(INVALID_USER_EXTENDED_DATA_VALUE),//only used if rule enabled, so using INVALID_USER_EXTENDED_DATA_VALUE (is a better sentinel than 0)
          mMaxUserUEDValue(INVALID_USER_EXTENDED_DATA_VALUE),
          mMinUserUEDValue(INVALID_USER_EXTENDED_DATA_VALUE),
          mJoiningPlayerCount(0)
    {
    }

    TeamUEDBalanceRule::TeamUEDBalanceRule(const TeamUEDBalanceRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(other, matchmakingAsyncStatus),
          mUEDValue(other.mUEDValue),
          mMaxUserUEDValue(other.mMaxUserUEDValue),
          mMinUserUEDValue(other.mMinUserUEDValue),
          mJoiningPlayerCount(other.mJoiningPlayerCount)
    {
    }

    TeamUEDBalanceRule::~TeamUEDBalanceRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamUEDBalanceRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamUEDBalanceRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
         if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
         {
             // Rule is disabled when searching for servers.
             return true;
         }

        const TeamUEDBalanceRulePrefs& rulePrefs = criteriaData.getTeamUEDBalanceRulePrefs();
        if (rulePrefs.getRangeOffsetListName()[0] == '\0')
        {
            // Rule is disabled (mRangeOffsetList == nullptr)
            return true;
        }

        // This rule is disabled unless the criteria rule name is for it. Note: no need to set the UED for disablers, as other CG enablers simply won't match any disablers.
        // (that's needed due to fact while we allow different possible team UED balance rules configured, they may have different UED sources and/or group formulas, and only one one rule maybe specified by the start MM request)
        if (blaze_stricmp(getRuleName(), rulePrefs.getRuleName()) != 0)
        {
            return true;
        }

        if ((rulePrefs.getRangeOffsetListName()[0] != '\0') && (rulePrefs.getRuleName()[0] == '\0'))
        {
            WARN_LOG("[TeamUEDBalanceRule].initialize No specified rule name in matchmaking criteria, while specifying a range offset list '" << rulePrefs.getRangeOffsetListName() << "'. No rule will be initialized.");
            return true;
        }

        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            mJoiningPlayerCount = (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount;

            UserExtendedDataValue calculatedValue = INVALID_USER_EXTENDED_DATA_VALUE;
            if (!getDefinition().calcUEDValue(calculatedValue, matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getId(), mJoiningPlayerCount, *matchmakingSupplementalData.mMembersUserSessionInfo))
            {
                // Missing key name indicates that there's an issue with the config:
                if (getDefinition().getUEDKey() == INVALID_USER_EXTENDED_DATA_KEY)
                {
                    ERR_LOG("[TeamUEDBalanceRule].initialize: UedRule Name '" << ((getRuleName() != nullptr) ? getRuleName() : "<unavailable>")
                        << "' uses an unknown UED key '" << ((getDefinition().getUEDName() != nullptr) ? getDefinition().getUEDName() : "(missing)")
                        << "', check for typos in your config.  Disabling rule.");
                    return true;
                }

                // Failure to calculate the UED value is a system failure. This is normally caused by the UED being misconfigured.
                // log a warning and disable the rule but don't fail the init.
                TRACE_LOG("[TeamUEDBalanceRule].initialize SYS_ERR UED Value for Session("<<
                    matchmakingSupplementalData.mPrimaryUserInfo->getSessionId() << ") not found, disabling rule.");
                return true;
            }

            mUEDValue = calculatedValue;
            getDefinition().normalizeUEDValue(mUEDValue, true); 

            // also cache highest and lowest user UED values for metrics. Side: error logged, but not considered disabling for metrics
            getDefinition().getSessionMaxOrMinUEDValue(true, mMaxUserUEDValue, matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getId(), mJoiningPlayerCount, *matchmakingSupplementalData.mMembersUserSessionInfo);
            getDefinition().getSessionMaxOrMinUEDValue(false, mMinUserUEDValue, matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getId(), mJoiningPlayerCount, *matchmakingSupplementalData.mMembersUserSessionInfo);
        }
        else
        {
            mJoiningPlayerCount = 0; // MM_AUDIT: need to provide hint for GB so we don't assume a single player joining.
            mUEDValue = 0;
            mMaxUserUEDValue = 0;
            mMinUserUEDValue = 0;
        }


        eastl::string errBuf;
        if (matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size() > 1)
        {
            err.setErrMessage(errBuf.sprintf("TeamUEDBalanceRule does not support joining into multiple teams (%u > 1).", matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size()).c_str());
            return false;
        }

        // validate the listName for the rangeOffsetList. NOTE: empty string is expected when the client disables the rule
        mRangeOffsetList = getDefinition().getRangeOffsetList(rulePrefs.getRangeOffsetListName());
        if (mRangeOffsetList == nullptr)
        {
            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Couldn't find a Range Offset List named \"%s\" in rule definition \"%s\".", rulePrefs.getRangeOffsetListName(), getRuleName());
            err.setErrMessage(msg);
            return false;
        }

        uint32_t nextThreshTemp = 0;
        const Blaze::GameManager::Matchmaker::RangeList::Range* firstThresh = mRangeOffsetList->getRange(0, &nextThreshTemp);
        if (firstThresh == nullptr)
        {
            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Rule Name '\"%s\"' configuration or definition of threshold name '\"%s\"' requires at least one entry in range offset list, disabling rule.", ((getRuleName() != nullptr)? getRuleName() : "<unavailable>"), ((mRangeOffsetList->getName() != nullptr)? mRangeOffsetList->getName() : "<rangeOffsetList name null>"));
            err.setErrMessage(msg);
            return false;
        }

        TRACE_LOG("[TeamUEDBalanceRule].initialize rule '" << getRuleName() << "' initialized for Session(" << matchmakingSupplementalData.mPrimaryUserInfo->getSessionId() << ") with session UED(" << getDefinition().getUEDName() << ") value '" << mUEDValue << "', initial Team UED imbalance tolerance '" << (UserExtendedDataValue)(firstThresh->mMaxValue - firstThresh->mMinValue) << "', session size " << mJoiningPlayerCount << ".");
        return true;
    }


    Rule* TeamUEDBalanceRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamUEDBalanceRule(*this, matchmakingAsyncStatus);
    }

    bool TeamUEDBalanceRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        if (mCurrentRangeOffset->isMatchAny())
        {
            // don't add any wmes if we are match any, the possible values become way too large
            // rely on post rete to just provide the fit score.
            return false;
        }

        const TeamUEDBalanceRuleDefinition &myDefn = getDefinition();

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& capacityOrConditions = conditions.push_back();

        switch (myDefn.getTeamValueFormula())
        {
            case GROUP_VALUE_FORMULA_MAX:
            case GROUP_VALUE_FORMULA_MIN:
                addConditionsMaxOrMin(capacityOrConditions);
                break;
            case GROUP_VALUE_FORMULA_SUM:
                addConditionsSum(capacityOrConditions, getUEDValue());
                break;
            case GROUP_VALUE_FORMULA_AVERAGE:
                addConditionsAvg(capacityOrConditions);
                break;
            case GROUP_VALUE_FORMULA_LEADER:
            default:
                ERR_LOG("[TeamUEDBalanceRule].addConditions unhandled/invalid group value formula " << GameManager::GroupValueFormulaToString(myDefn.getTeamValueFormula()));
                break;
        };

        return true;
    }

    void TeamUEDBalanceRule::addConditionsMaxOrMin(Rete::OrConditionList& capacityOrConditions) const
    {
        // NOTE: this rule's RETE is a broad filter, we rely on post-RETE to precisely filter

        // I might not affect any team's max/min. If game's current imbalance is in our range, count as possible RETE match.
        UserExtendedDataValue myStartRange = 0;
        UserExtendedDataValue myEndRange = getAcceptableTeamUEDImbalance();
        capacityOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, myStartRange, myEndRange, getDefinition()), 
            0, this));

        // I might join and need to compare with the top or else 2nd-to-top max/min-valued team. If that team's and my UED is in range, count as possible RETE match.
        if (!isGameBrowsing())//unneeded if GB
        {
            myStartRange = (getUEDValue() - getAcceptableTeamUEDImbalance());
            myEndRange = (getUEDValue() + getAcceptableTeamUEDImbalance());

            if (myStartRange > myEndRange)
            {
                return;
            }

            capacityOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_TOP_TEAM, myStartRange, myEndRange, getDefinition()), 
                0, this));
            capacityOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_2ND_TOP_TEAM, myStartRange, myEndRange, getDefinition()), 
                0, this));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Add search conditions for sum group formula
        \param[in] mySessionSum Treat this value as my session's UED value. Used by 'avg' calc to override using my actual value.
    *************************************************************************************************/
    void TeamUEDBalanceRule::addConditionsSum(Rete::OrConditionList& capacityOrConditions, UserExtendedDataValue mySessionSum) const
    {
        // Get the range of UED imbalances that could possibly be matched
        // side: do *not* normalize here, since positive and negative ued values *difference* could be outside either positive or negative bounds.
        UserExtendedDataValue myStartingOffset = ((-1) * (getAcceptableTeamUEDImbalance() + mySessionSum));
        UserExtendedDataValue myEndingOffset = (getAcceptableTeamUEDImbalance() - mySessionSum);

        if (myStartingOffset > myEndingOffset)
        {
            return;
        }
        UserExtendedDataValue absStartingOffset = abs(myStartingOffset);

        // WME's are currently limited to positive values.  Difference is inserted by abs value by the definition.
        if (absStartingOffset > myEndingOffset)
        {
            myEndingOffset = absStartingOffset;
            myStartingOffset = 0;
        }
        else
        {
            myStartingOffset = 0;
        }

        FitScore fitScore = 0; // 0 (re-calc at post rete)
        capacityOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, myStartingOffset, myEndingOffset, getDefinition()), 
            fitScore, this));
    }

    /*! ************************************************************************************************/
    /*! \brief Add search conditions for avg group formula
        Note: for simplicity (and efficiency), group formula average will also re-use similar-to-sum algorithm for RETE part (rely on post-RETE to precisely filter).
        We take two extremes' search conditions to ensure we encompass games that can match: either we skew the 
        average of an empty team into being exactly our value, or, we add nothing to the existing team's average.
    *************************************************************************************************/
    void TeamUEDBalanceRule::addConditionsAvg(Rete::OrConditionList& capacityOrConditions) const
    {
        // First take extreme case adding us will increase the avg equal to our value.
        // I.e. do the 'sum' calc treating my value as my value.
        if (!isGameBrowsing())//unneeded if GB
            addConditionsSum(capacityOrConditions, getUEDValue());

        // Second take extreme adding us doesn't modify any team's average at all.
        // I.e. do the 'sum' calc but treat my value as zero.
        addConditionsSum(capacityOrConditions, 0);

        // Note the above *will* get games that we shouldn't actually match also, rely on post-RETE to filter them out.
    }


    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    void TeamUEDBalanceRule::updateAcceptableRange() 
    {
        // Search team requirements, only one for all teams. Center is zero, we use the max of the range as acceptable balance diff.
        mAcceptableRange.setRange(0, *mCurrentRangeOffset, 0, getDefinition().getGlobalMaxTeamUEDImbalance());
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamUEDBalanceRule::updateAsyncStatus()
    {
        TeamUEDBalanceRuleStatus &teamUedBalanceRuleStatus = mMatchmakingAsyncStatus->getTeamUEDBalanceRuleStatus();
        teamUedBalanceRuleStatus.setRuleName(getRuleName());
        teamUedBalanceRuleStatus.setMaxTeamUEDDifferenceAccepted(getAcceptableTeamUEDImbalance());
        teamUedBalanceRuleStatus.setMyUEDValue(getUEDValue());
    }

    void TeamUEDBalanceRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        const TeamUEDBalanceRule& otherTeamUEDBalanceRule = static_cast<const TeamUEDBalanceRule &>(otherRule);

        if (otherTeamUEDBalanceRule.getId() != getId())
        {
            WARN_LOG("[TeamUEDBalanceRule].calcFitPercent: evaluating different TeamUEDBalanceRule against each other, my rule '" << getDefinition().getName() << "' id '" << getId() << 
                "', vs other's rule '" << otherTeamUEDBalanceRule.getDefinition().getName() << "' id '" << otherTeamUEDBalanceRule.getId() << "'. Internal rule indices or rule id is out of sync. No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other team balance rule %u != %u", otherTeamUEDBalanceRule.getId(), getId());

            return;
        }
        if (!otherTeamUEDBalanceRule.getCriteria().hasTeamUEDBalanceRuleEnabled(getId()))
        {
            TRACE_LOG("[TeamUEDBalanceRule].calcFitPercent: other TeamUEDBalanceRule '" << otherTeamUEDBalanceRule.getDefinition().getName() << "' id '" << otherTeamUEDBalanceRule.getId() << "' disabled. No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other team balance rule %u not enabled", getId());

            return;
        }

        // we don't calculate the final fit here, we rely on finalization to sort us out.
        fitPercent = 0.0f;
        isExactMatch = false;
    }

    void TeamUEDBalanceRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        //if matches, so set the match immediately
        if (isFitPercentMatch(evalInfo.mFitPercent))
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight()), 0);
        else
            matchInfo.setNoMatch();
    }

    /*! \brief evaluates whether requirements are met (used post RETE). For matchmaking, match accounts for fact I will join a team.*/
    void TeamUEDBalanceRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        isExactMatch = false;
        const uint16_t teamCount = gameSession.getTeamCount();
        if (teamCount < 2)
        {
            // consider perfect match
            fitPercent = getDefinition().getFitPercent(0, 0);

            debugRuleConditions.writeRuleCondition("Only one team");

            return;
        }
        fitPercent = doCalcFitPercent(gameSession, debugRuleConditions);
    }

    /*! \brief  helper returns the fit percent for game's teams' UED imbalance */
    float TeamUEDBalanceRule::doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        // Post-RETE is a more precise filter. we also check which of the team's we'd actually be joining at the master here.
        // Side: finalization will join the smallest team, regardless of Team UED balance, as player count balancing is always preferred over
        // Team UED balancing. We only need to figure whether the resulting Team UED imbalance falls in range.
        TeamIndex myTeamIndex;
        TeamId myTeamId;
        
        const TeamIdRoleSizeMap* roleSizeMap = getCriteria().getTeamIdRoleSpaceRequirementsFromRule();
        if (roleSizeMap == nullptr || roleSizeMap->size() != 1)
        {
            debugRuleConditions.writeRuleCondition("Unable to determine role requirements. Or multiple teams were used.");

            return FIT_PERCENT_NO_MATCH;
        }

        BlazeRpcError err = gameSession.findOpenTeam(getCriteria().getTeamSelectionCriteriaFromRules(), roleSizeMap->begin(),
            &getDefinition().getGroupUedExpressionList(), myTeamIndex);
        myTeamId = gameSession.getTeamIdByIndex(myTeamIndex);
        if (err != ERR_OK || myTeamIndex == UNSPECIFIED_TEAM_INDEX)
        {
            debugRuleConditions.writeRuleCondition("No joinable team %u", myTeamId);

            return FIT_PERCENT_NO_MATCH;
        }

        // We find the two teams with the largest and smallest team UED values accounting for the team I will join:
        UserExtendedDataValue lowestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue highestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue actualUEDImbal = getDefinition().calcGameImbalance(gameSession, lowestTeamUED, highestTeamUED, this, myTeamIndex, true);
        if (actualUEDImbal == INVALID_USER_EXTENDED_DATA_VALUE)
        {
            debugRuleConditions.writeRuleCondition("Unable to calculate game's UED imbalance.");

            return FIT_PERCENT_NO_MATCH;
        }

        if (actualUEDImbal > getAcceptableTeamUEDImbalance())
        {
            debugRuleConditions.writeRuleCondition("Game UED imbalance %" PRId64 " > %" PRId64 "", actualUEDImbal, getAcceptableTeamUEDImbalance());

            return  FIT_PERCENT_NO_MATCH;
        }
        
        debugRuleConditions.writeRuleCondition("Game UED imbalance %" PRId64 " <= %" PRId64 "", actualUEDImbal, getAcceptableTeamUEDImbalance());
        
        return (getDefinition().getFitPercent(0, actualUEDImbal));
    }

    const char8_t* TeamUEDBalanceRule::makeSessionLogStr(const MatchmakingSession* session, eastl::string& buf)
    {
        if (session == nullptr)
        {
            buf.sprintf("<MMsession nullptr>");
            return buf.c_str();
        }
        buf.sprintf("(MMsessionId=%" PRIu64 ",size=%u", session->getMMSessionId(), session->getPlayerCount());
        if (!session->getCriteria().hasTeamUEDBalanceRuleEnabled())
        {
            buf.append_sprintf(") <TeamUEDBalanceRule disabled>");
        }
        else
        {
            buf.append_sprintf(",teamUed=%" PRIi64 ",maxTeamUedImbal=%" PRIi64 "", session->getCriteria().getTeamUEDContributionValue(), session->getCriteria().getAcceptableTeamUEDImbalance());
        }
        buf.append(")");
        return buf.c_str();
    }

    ////////////// Finalization Helpers
    /*! ************************************************************************************************/
    /*! \brief Return the next team to try to fill for finalization. Returns nullptr if no more teams available to try.
        First priority is smallest size, if teams are equal size, then the team with lower UED value is picked.
    ***************************************************************************************************/
    CreateGameFinalizationTeamInfo* TeamUEDBalanceRule::selectNextTeamForFinalization(
        CreateGameFinalizationSettings& finalizationSettings) const
    {
        CreateGameFinalizationTeamInfo* teamToFill = nullptr;

        size_t teamCapacitiesVectorSize = finalizationSettings.mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            CreateGameFinalizationTeamInfo& nextTeam = finalizationSettings.mCreateGameFinalizationTeamInfoVector[i];
            if ((nextTeam.mTeamSize == nextTeam.mTeamCapacity) ||
                finalizationSettings.wasTeamTriedForCurrentPick(nextTeam.mTeamIndex, finalizationSettings.getTotalSessionsCount()))
            {
                // this team has no possible MM sessions from any buckets that can go into it, move on and try another team.
                continue;
            }

            // priority to smallest sized teams
            if ((teamToFill == nullptr) || (nextTeam.mTeamSize < teamToFill->mTeamSize))
            {
                teamToFill = &nextTeam;
            }
            else if ((teamToFill != nullptr) && (nextTeam.mTeamSize == teamToFill->mTeamSize))
            {
                // size is tied, use weaker ued value, else random, to break the tie.
                if ((nextTeam.mUedValue < teamToFill->mUedValue) && (getDefinition().getTeamValueFormula() != GROUP_VALUE_FORMULA_MIN))
                {
                    teamToFill = &nextTeam;
                }
                if ((nextTeam.mUedValue > teamToFill->mUedValue) && (getDefinition().getTeamValueFormula() == GROUP_VALUE_FORMULA_MIN))
                {
                    // note for group formula MIN, the 'weaker' valued team is actually the higher valued team
                    teamToFill = &nextTeam;
                }
                if ((nextTeam.mUedValue == teamToFill->mUedValue) && (Random::getRandomNumber(2) == 0))//MM_AUDIT: post ILT, potentialy pick a more eff way to 'randomly' choose, e.g. pseudo random
                {
                    teamToFill = &nextTeam;
                }
            }
        }

        TRACE_LOG("[TeamUEDBalanceRule].selectNextTeamForFinalization: found " << ((teamToFill != nullptr)? teamToFill->toLogStr() : "NO vacant/untried teams") << ".");
        return teamToFill;
    }

    /*! ************************************************************************************************/
    /*! \brief Return next matchmaking session (from a sessions bucket), to add to create game finalization list,
        based off team UED balance criteria, the team it would join, and current finalization state.

        \param[in] pullingSession The matchmaking session which owns this rule and is attempting the finalization.
        \param[in] bucket The bucket of matchmaking sessions from which to pick the next session from.
        \param[in] teamToFill The team in the finalization for which the found session would need to join to.
        \param[in] finalizationSettings The current finalization settings and state.
        \return The matchmaking session found to attempt to add. nullptr if none found which could be added.
    ***************************************************************************************************/
     const MatchmakingSessionMatchNode* TeamUEDBalanceRule::selectNextSessionForFinalizationByTeamUEDBalance(const MatchmakingSession& pullingSession,
         const MatchedSessionsBucket& bucket, const CreateGameFinalizationTeamInfo& teamToFill,
         const CreateGameFinalizationSettings& finalizationSettings,
         const MatchmakingSession &autoJoinSession) const
     {
         const UserExtendedDataValue origLowestAcceptableImbal = getLowestAcceptableUEDImbalanceInFinalization(finalizationSettings);
         if (origLowestAcceptableImbal == INVALID_USER_EXTENDED_DATA_VALUE)
             return nullptr;//logged

         const CreateGameFinalizationTeamInfo* bottom = nullptr;
         const CreateGameFinalizationTeamInfo* top = nullptr;
         if (!getHighestAndLowestUEDValuedFinalizingTeams(finalizationSettings, bottom, top))
             return nullptr;//logged

         if (getDefinition().getTeamValueFormula() == GROUP_VALUE_FORMULA_MIN)
         {
             // if group formula MIN the 'top' team is the one with the minimum value.
             const CreateGameFinalizationTeamInfo* tmp = top;
             top = bottom;
             bottom = tmp;
         }

         // try getting the Team UED balancing 'best' session.
         if (finalizationSettings.areAllFinalizingTeamSizesEqual())
         {
             TRACE_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: Pre-existing state: teams currently size-balanced. Now trying to find best session (or pair of sessions) to add to " << teamToFill.toLogStr() << " from " << bucket.toLogStr() << "..");
             eastl::string logBuf;

             // Case 1. we're already size-balanced, try pick a session for which there exists another session it can pair with to
             // maintain balance (NOTE: we only return one here, and assume the next pick will then take that 2nd of the pair).
             // Pre: for performance, large scale, we assume usually there's many matched sessions pairable/'near' each other, so won't need much scanning here.
             const MatchmakingSessionMatchNode* candidateSession = nullptr;
             for (MatchedSessionsBucket::BucketItemVector::const_iterator itr = bucket.mItems.begin(); (itr != bucket.mItems.end()); ++itr)
             {
                 if (!advanceBucketIterToNextJoinableSessionForTeam(itr, bucket, teamToFill, autoJoinSession, pullingSession))
                 {
                     //side: if only found an unpairable session, we'll just return it.
                     SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: for " << teamToFill.toLogStr() << ", no more available sessions to pick from current bucket. Got session: " << 
                         ((candidateSession == nullptr)? "NO candidates(yet)" : TeamUEDBalanceRule::makeSessionLogStr(candidateSession->sMatchedToSession, logBuf)) << "..");
                     break;
                 }

                 // next session non-null, set as candidate, then try find its closest pair-able session below
                 candidateSession = itr->getMatchNode();
                 MatchedSessionsBucket::BucketItemVector::const_iterator itrNext = itr;
                 ++itrNext;
                 if (!advanceBucketIterToNextJoinableSessionForTeam(itrNext, bucket, teamToFill, autoJoinSession, pullingSession))
                 {
                     //only found an unpairable session, we'll just return it.
                     SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: found session to add to " << teamToFill.toLogStr() << " (no available sessions to pair with it)");
                     break;
                 }

                 UserExtendedDataValue newLowestAcceptableImbal = eastl::min(origLowestAcceptableImbal, candidateSession->sMatchedToSession->getCriteria().getAcceptableTeamUEDImbalance());
                 newLowestAcceptableImbal = eastl::min(newLowestAcceptableImbal, itrNext->getMmSession()->getCriteria().getAcceptableTeamUEDImbalance());
                 if ((itr->mSortValue - itrNext->mSortValue) <= newLowestAcceptableImbal)
                 {
                     // found a possible pair that gets acceptable UED balance. break out of loop to add it.
                     SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: found session to add to " << teamToFill.toLogStr() << 
                         ",  the session(" << TeamUEDBalanceRule::makeSessionLogStr(candidateSession->sMatchedToSession, logBuf) << ") - has a possible paired session(" << TeamUEDBalanceRule::makeSessionLogStr(itrNext->getMmSession(), logBuf) << ") for next pick for good balance.");
                     break;
                 }

                 // cannot pair any with the current session, try next
                 SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: Continuing attempt to find best session to add to " << teamToFill.toLogStr() << 
                     ", the current candidate session(" << TeamUEDBalanceRule::makeSessionLogStr(candidateSession->sMatchedToSession, logBuf) << ") did not find a possible session to pair with from the " << bucket.toLogStr() << ".");
             }//for

             // We either found a session that can pair, or found an unpaired session, or found none. Return result.
             return candidateSession;
         }
         else if ((top->mTeamIndex == teamToFill.mTeamIndex) && (getDefinition().getTeamValueFormula() != GROUP_VALUE_FORMULA_MIN))
         {
             TRACE_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: Pre-existing state: teams UED imbalance=" << (top->mUedValue - bottom->mUedValue) << 
                 ". Now trying to find best session to add to highest-UED-valued " << teamToFill.toLogStr() << " from " << bucket.toLogStr() << "..");

             // Case 2. We're adding to highest-UED-valued team. Pick a low-value session that least increases the UED difference, from back of the bucket.
             // Note: we handle GROUP_VALUE_FORMULA_MIN as if its not top-valued, see below.
             MatchedSessionsBucket::BucketItemVector::const_reverse_iterator backItr = bucket.mItems.rbegin();
             if (advanceBucketIterToNextJoinableSessionForTeam(backItr, bucket, teamToFill, autoJoinSession, pullingSession))
             {
                 return backItr->getMatchNode();
             }
             return nullptr;
         }
         else
         {
             TRACE_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: Pre-existing state: teams UED imbalance=" << (top->mUedValue - bottom->mUedValue) << 
                 ". Now trying to find best session to add to smaller-UED-valued " << teamToFill.toLogStr() << " from " << bucket.toLogStr() << "..");
             eastl::string logBuf;

             // Case 3. If here we're adding to not top-valued team. Pick to offset any UED difference.
             // First, calc what would be needed for the session to offset it (side: below works also for group formula min, will get imbalance less than the min)
             UserExtendedDataValue targetUed = (top->mUedValue + origLowestAcceptableImbal);
             UserExtendedDataValue otherNeededUedUpperBnd;
             getDefinition().calcDesiredJoiningMemberUEDValue(otherNeededUedUpperBnd, bucket.mKey, teamToFill.mTeamSize, teamToFill.mUedValue, targetUed);

             // Binary search to approx range. Pre: bucket items sorted descending.
             SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: .. searching for a session with UED value under the upper bound of " << otherNeededUedUpperBnd << "..");
             MatchedSessionsBucket::BucketItemVector::const_iterator foundIt = eastl::lower_bound(bucket.mItems.begin(), bucket.mItems.end(), otherNeededUedUpperBnd, MatchedSessionsBucket::BucketItemComparitor());
             if (foundIt != bucket.mItems.end())
             {
                 // Find a session
                 SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: .. found potential sessions with UED value under the upper bound of " << otherNeededUedUpperBnd << ", continuing to find one of these joinable to team..");
                 if (advanceBucketIterToNextJoinableSessionForTeam(foundIt, bucket, teamToFill, autoJoinSession, pullingSession))
                 {
                     SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: .. found a session with UED value under the upper bound of " << otherNeededUedUpperBnd << ", that's joinable to team index " << teamToFill.mTeamIndex << ", " << makeSessionLogStr(foundIt->getMmSession(), logBuf));
                     return foundIt->getMatchNode();
                 }
             }
             else
             {
                 // happens that our nothing is under our target upper bound. Just take any session
                 SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: .. NO found potential sessions with UED value under the upper bound of " << otherNeededUedUpperBnd << ", continuing to try to find one of these that's joinable to team above this bound..");
                 MatchedSessionsBucket::BucketItemVector::const_reverse_iterator backItr = bucket.mItems.rbegin();
                 if (advanceBucketIterToNextJoinableSessionForTeam(backItr, bucket, teamToFill, autoJoinSession, pullingSession))
                 {
                     SPAM_LOG("[TeamUEDBalanceRule].selectNextSessionForFinalizationByTeamUED: .. found a session with UED value under the upper bound of " << otherNeededUedUpperBnd << ", that's joinable to team index " << teamToFill.mTeamIndex << ", " << makeSessionLogStr(backItr->getMmSession(), logBuf));
                     return backItr->getMatchNode();
                 }
             }

             return nullptr;
         }
     }

    /*! ************************************************************************************************/
    /*! \brief return whether difference between any two teams' UED values falls within the minimum acceptable UED imbalance
        specified by matchmaking sessions in the finalization list.
        \param[out] largestTeamUedDiff stores the largest difference found between 2 teams
    *************************************************************************************************/
    bool TeamUEDBalanceRule::areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings,
        UserExtendedDataValue& largestTeamUedDiff) const
    {
        if (!finalizationSettings.isTeamsEnabled())
            return true;

        const CreateGameFinalizationTeamInfo* lowest = nullptr;
        const CreateGameFinalizationTeamInfo* highest = nullptr;

        // only calculate the UED difference between populated teams (this means a game with only 1 populated team will have a difference of 0.)
        if (!getHighestAndLowestUEDValuedFinalizingTeams(finalizationSettings, lowest, highest, ANY_TEAM_ID, true))
            return false;//logged

        largestTeamUedDiff = (highest->mUedValue - lowest->mUedValue);

        const MatchmakingSession* sessionWithMinAcceptableTeamUedDifference;
        UserExtendedDataValue minAcceptableImbalance = getLowestAcceptableUEDImbalanceInFinalization(finalizationSettings, &sessionWithMinAcceptableTeamUedDifference);

        TRACE_LOG("[TeamUEDBalanceRule].areFinalizingTeamsAcceptableToCreateGame: "
            << " Team UED currently " << ((largestTeamUedDiff > minAcceptableImbalance)? "too unbalanced to finalize" : "acceptably balanced")
            << " for the (" << finalizationSettings.getTotalSessionsCount() << ") MM sessions in the potential match, on Team UED imbalance '" << largestTeamUedDiff << "' with '" << finalizationSettings.mCreateGameFinalizationTeamInfoVector.size() << "' total teams. The MM session with lowest team UED imbalance tolerance(" << minAcceptableImbalance << ") was id='" << ((sessionWithMinAcceptableTeamUedDifference != nullptr)? sessionWithMinAcceptableTeamUedDifference->getMMSessionId() : INVALID_MATCHMAKING_SESSION_ID) << "'.");
        return (largestTeamUedDiff <= minAcceptableImbalance);
    }

    
    /*! ************************************************************************************************/
    /*! \brief get the highest and lowest UED valued teams in the current finalization
    *************************************************************************************************/
    bool TeamUEDBalanceRule::getHighestAndLowestUEDValuedFinalizingTeams(const CreateGameFinalizationSettings& finalizationSettings,
        const CreateGameFinalizationTeamInfo*& lowest,
        const CreateGameFinalizationTeamInfo*& highest, TeamId teamId /*= ANY_TEAM_ID*/, bool ignoreEmptyTeams /*= false*/) const
    {
        lowest = nullptr;
        highest = nullptr;

        if (!finalizationSettings.isTeamsEnabled())
        {
            ERR_LOG("[TeamUEDBalanceRule].getHighestAndLowestUEDValuedTeamsInFinalization teams not enabled.");
            return false;
        }

        size_t teamCapacitiesVectorSize = finalizationSettings.mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            const CreateGameFinalizationTeamInfo& teamInfo = finalizationSettings.mCreateGameFinalizationTeamInfoVector[i];

            bool isTeamIdMatch = ((teamId == ANY_TEAM_ID) || (teamInfo.mTeamId == teamId) || (teamInfo.mTeamId == ANY_TEAM_ID));

            if ((teamInfo.mTeamSize > 0) || !ignoreEmptyTeams || isTeamIdMatch)
            {
                // get the highest valued team for the team Id we have specified
                if (isTeamIdMatch && ((highest == nullptr) || (teamInfo.mUedValue > highest->mUedValue)))
                {
                    highest = &teamInfo;
                }
                // get the lowest valued team for the team Id we have specified
                if (isTeamIdMatch && ((lowest == nullptr) || (teamInfo.mUedValue <= lowest->mUedValue)))
                {
                    lowest = &teamInfo;
                }
            }
        }

        if ((highest == nullptr) || (lowest == nullptr))
        {
            // since we're building teams as part of create-game finalization, we should never hit this.
            // in the worst case, we're assiging the first players to the game meaning at least one team will have members, and be considered both the "lowest" and "highest".
            ERR_LOG("[TeamUEDBalanceRule].getHighestAndLowestUEDValuedTeamsInFinalization: missing teams: highest UED valued team was " << ((highest == nullptr)? "N/A" : "available") << ", lowest UED valued team was " << ((lowest == nullptr)? "N/A" : "available"));
            return false;
        }
        SPAM_LOG("[TeamUEDBalanceRule].getHighestAndLowestUEDValuedTeamsInFinalization: highest UED valued " << highest->toLogStr() << ", lowest UED valued " << lowest->toLogStr());
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Return the lowest acceptable Team UED imbalance of the finalizing group.
         The value is the overall minimum that must be met for the group, currently.
    *************************************************************************************************/
    UserExtendedDataValue TeamUEDBalanceRule::getLowestAcceptableUEDImbalanceInFinalization(
        const CreateGameFinalizationSettings& finalizationSettings,
        const MatchmakingSession** lowestAcceptableTeamImbalanceSession /*= nullptr*/) const
    {
        // Init with my self to start
        const MatchmakingSession* sessionWithMinAcceptableTeamUedDifference = getCriteria().getMatchmakingSession();
        UserExtendedDataValue lowestAcceptableTeamImbalance = getCriteria().getAcceptableTeamUEDImbalance();

        // Pre: all of sessions by finalization here are using the same Team UED rule (and team UED formula etc) to match,
        // (otherwise they'd have failed evaluation earlier). So we can just compare their one TeamUED imbalance member to member below.
        for (MatchmakingSessionList::const_iterator itr = finalizationSettings.getMatchingSessionList().begin(); itr != finalizationSettings.getMatchingSessionList().end(); ++itr)
        {
            UserExtendedDataValue bal = (*itr)->getCriteria().getAcceptableTeamUEDImbalance();
            if (bal < lowestAcceptableTeamImbalance)
            {
                lowestAcceptableTeamImbalance = bal;
                sessionWithMinAcceptableTeamUedDifference = (*itr);
            }
        }
        if (sessionWithMinAcceptableTeamUedDifference == nullptr)
        {
            WARN_LOG("[TeamUEDBalanceRule].getLowestAcceptableUEDImbalanceInFinalization: no session found, the finalization list of sessions to finalize with was empty.");
        }
        if (lowestAcceptableTeamImbalanceSession != nullptr)
            *lowestAcceptableTeamImbalanceSession = sessionWithMinAcceptableTeamUedDifference;
        return lowestAcceptableTeamImbalance;
    }

    // End Finalization Helpers

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
