/*! ************************************************************************************************/
/*!
    \file teamuedpositionparityrule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucketutil.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteaminfo.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamUEDPositionParityRule::TeamUEDPositionParityRule(const TeamUEDPositionParityRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(definition, matchmakingAsyncStatus),
          mMaxAcceptableTopImbalance(0),
          mMaxAcceptableBottomImbalance(0)
    {
    }

    TeamUEDPositionParityRule::TeamUEDPositionParityRule(const TeamUEDPositionParityRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(other, matchmakingAsyncStatus),
          mMaxAcceptableTopImbalance(other.mMaxAcceptableTopImbalance),
          mMaxAcceptableBottomImbalance(other.mMaxAcceptableBottomImbalance)
    {
        mUEDValueList.assign(other.mUEDValueList.begin(), other.mUEDValueList.end());
    }

    TeamUEDPositionParityRule::~TeamUEDPositionParityRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamUEDPositionParityRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamUEDPositionParityRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
        const TeamUEDPositionParityRulePrefs& rulePrefs = criteriaData.getTeamUEDPositionParityRulePrefs();
        if (rulePrefs.getRangeOffsetListName()[0] == '\0')
        {
            // Rule is disabled (mRangeOffsetList == nullptr)
            return true;
        }

        // This rule is disabled unless the criteria rule name is for it. Note: no need to set the UED for disablers, as other CG enablers simply won't match any disablers.
        // (that's needed due to fact while we allow different possible team UED balance rules configured, they may have different UED sources and/or requirements, and only one one rule may be specified by the start MM request)
        if (blaze_stricmp(getRuleName(), rulePrefs.getRuleName()) != 0)
        {
            return true;
        }

        // Missing key name indicates that there's an issue with the config:
        if (getDefinition().getUEDKey() == INVALID_USER_EXTENDED_DATA_KEY)
        {
            ERR_LOG("[TeamUEDPosRule].initialize: UedRule Name '" << ((getRuleName() != nullptr) ? getRuleName() : "<unavailable>")
                << "' uses an unknown UED key '" << ((getDefinition().getUEDName() != nullptr) ? getDefinition().getUEDName() : "(missing)")
                << "', check for typos in your config.  Disabling rule.");
            return true;
        }

        if ((rulePrefs.getRangeOffsetListName()[0] != '\0') && (rulePrefs.getRuleName()[0] == '\0'))
        {
            WARN_LOG("[TeamUEDPositionParityRule].initialize No specified rule name in matchmaking criteria, while specifying a range offset list '" << rulePrefs.getRangeOffsetListName() << "'. No rule will be initialized.");
            return true;
        }
        
        // if we're not comparing any positions, the rule is a no-op, treat as disabled
        if ((getDefinition().getTopPlayerCount() == 0) && (getDefinition().getBottomPlayerCount() == 0))
        {
            TRACE_LOG("[TeamUEDPositionParityRule].initialize '" << getRuleName() << "'. Rule is comparing no positions, treated as disabled.");
            return true;
        }

        eastl::string errBuf;
        if (matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size() > 1)
        {
            err.setErrMessage(errBuf.sprintf("TeamUEDPositionParityRule does not support joining into multiple teams (%u > 1).", matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size()).c_str());
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

        if ((matchmakingSupplementalData.mMembersUserSessionInfo == nullptr) || matchmakingSupplementalData.mMembersUserSessionInfo->empty())
        {
            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Rule Name '\"%s\"' matchmakingSupplementalData.mMembersUserSessionInfo was '\"%s\"', cannot initialize rule.", ((getRuleName() != nullptr)? getRuleName() : "<unavailable>"), ((matchmakingSupplementalData.mMembersUserSessionInfo == nullptr)? "nullptr" : "empty"));
            err.setErrMessage(msg);
            return false;
        }

        // put the ued values in the rule's UED list
        UserExtendedDataKey uedKey = getDefinition().getUEDKey();
        mUEDValueList.reserve(matchmakingSupplementalData.mJoiningPlayerCount);
        for (GameManager::UserSessionInfoList::const_iterator itr = matchmakingSupplementalData.mMembersUserSessionInfo->begin(), end = matchmakingSupplementalData.mMembersUserSessionInfo->end(); itr != end; ++itr)
        {
            UserExtendedDataValue dataValue = 0;
            if (MatchmakingUtil::getLocalUserData(**itr, uedKey, dataValue))
            {
                mUEDValueList.push_back(dataValue);
            }
        }
        // sorting this can make some FG comparisons easier later on.
        eastl::sort(mUEDValueList.begin(), mUEDValueList.end(), eastl::greater<UserExtendedDataValue>());

        return true;
    }


    Rule* TeamUEDPositionParityRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamUEDPositionParityRule(*this, matchmakingAsyncStatus);
    }

    
    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    void TeamUEDPositionParityRule::updateAcceptableRange()
    {
        // "minimum" difference is always 0, so just ensure we clamp the current range offset to the rule's configured max range.
        mMaxAcceptableTopImbalance = mCurrentRangeOffset->mMaxValue > getDefinition().getMaxRange() ? getDefinition().getMaxRange() : mCurrentRangeOffset->mMaxValue;
        mMaxAcceptableBottomImbalance = mCurrentRangeOffset->mMinValue > getDefinition().getMaxRange() ? getDefinition().getMaxRange() : mCurrentRangeOffset->mMinValue;
        if (mMaxAcceptableTopImbalance == RangeList::INFINITY_RANGE_VALUE)
        {
            mMaxAcceptableTopImbalance = (getDefinition().getMaxRange() - getDefinition().getMinRange());
        }
        if (mMaxAcceptableBottomImbalance == RangeList::INFINITY_RANGE_VALUE)
        {
            mMaxAcceptableBottomImbalance = (getDefinition().getMaxRange() - getDefinition().getMinRange());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamUEDPositionParityRule::updateAsyncStatus()
    {
        TeamUEDPositionParityRuleStatus &teamUedPositionParityRuleStatus = mMatchmakingAsyncStatus->getTeamUEDPositionParityRuleStatus();
        teamUedPositionParityRuleStatus.setRuleName(getRuleName());
        teamUedPositionParityRuleStatus.setMaxUEDDifferenceAcceptedTopPlayers(getAcceptableTopUEDImbalance());
        teamUedPositionParityRuleStatus.setTopPlayersCounted(getDefinition().getTopPlayerCount());
        teamUedPositionParityRuleStatus.setMaxUEDDifferenceAcceptedBottomPlayers(getAcceptableBottomUEDImbalance());
        teamUedPositionParityRuleStatus.setBottomPlayersCounted(getDefinition().getBottomPlayerCount());
    }

    void TeamUEDPositionParityRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        const TeamUEDPositionParityRule& otherTeamUEDPositionParityRule = static_cast<const TeamUEDPositionParityRule &>(otherRule);

        if (otherTeamUEDPositionParityRule.getId() != getId())
        {
            WARN_LOG("[TeamUEDBalanceRule].calcFitPercent: evaluating different TeamUEDPositionParityRules against each other, my rule '" << getDefinition().getName() << "' id '" << getId() << 
                "', vs other's rule '" << otherTeamUEDPositionParityRule.getDefinition().getName() << "' id '" << otherTeamUEDPositionParityRule.getId() << "'. Internal rule indices or rule ids out of sync. No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other team UED position parity rule %u != %u", otherTeamUEDPositionParityRule.getId(), getId());

            return;
        }
        if (!otherTeamUEDPositionParityRule.getCriteria().hasTeamUEDPositionParityRuleEnabled(getId()))
        {
            WARN_LOG("[TeamUEDBalanceRule].calcFitPercent: evaluating different TeamUEDPositionParityRules against each other, my rule '" << getDefinition().getName() << "' id '" << getId() << 
                "', vs other's rule '" << otherTeamUEDPositionParityRule.getDefinition().getName() << "' id '" << otherTeamUEDPositionParityRule.getId() << "'. Internal rule indices or rule ids out of sync. No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other team UED position parity rule %u not enabled", getId());

            return;
        }

        // we don't calculate the final fit here, we rely on finalization to sort us out.
        fitPercent = 0.0f;
        isExactMatch = false;
    }

    void TeamUEDPositionParityRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
      // no-op
    }

    /*! \brief evaluates whether requirements are met (used post RETE). For matchmaking, match accounts for fact I will join a team.*/
    void TeamUEDPositionParityRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
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

    /*! \brief  helper returns the fit percent for game's teams' UED position parity. */
    float TeamUEDPositionParityRule::doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {

        // Post-RETE is a more precise filter. we also check which of the team's we'd actually be joining at the master here.
        // Side: finalization will join the smallest team, regardless of Team position parity, as player count balancing AND team UED balance is always preferred over
        // Team position parity. We only need to figure whether the resulting top/bottom N differences fall in range.
        TeamIndex myTeamIndex;
        TeamId myTeamId;
        
        const TeamIdRoleSizeMap* roleSizeMap = getCriteria().getTeamIdRoleSpaceRequirementsFromRule();
        if (roleSizeMap == nullptr || roleSizeMap->size() != 1)
        {
            debugRuleConditions.writeRuleCondition("Unable to determine role requirements. Or multiple teams were used.");

            return FIT_PERCENT_NO_MATCH;
        }

        BlazeRpcError err = gameSession.findOpenTeam(getCriteria().getTeamSelectionCriteriaFromRules(), roleSizeMap->begin(), 
            getCriteria().getTeamUEDBalanceRule() ? &(getCriteria().getTeamUEDBalanceRule()->getDefinition().getGroupUedExpressionList()) : nullptr, myTeamIndex);
        myTeamId = gameSession.getTeamIdByIndex(myTeamIndex);
        if (err != ERR_OK || myTeamIndex == UNSPECIFIED_TEAM_INDEX)
        {
            debugRuleConditions.writeRuleCondition("No joinable team %u.", myTeamId);

            return FIT_PERCENT_NO_MATCH;
        }


        // determine if I (or my group) will displace any ranks we care about, and make a temp UED list
        const MatchmakingGameInfoCache::TeamIndividualUEDValues *teamUedValues = gameSession.getMatchmakingGameInfoCache()->getCachedTeamIndivdualUEDVectors(getDefinition());
        if (teamUedValues == nullptr)
        {
            ERR_LOG("[TeamUEDBalanceRule].calcFitPercent: evaluating game '" << gameSession.getGameId() << "' had no cached TeamIndivdualUEDVectors to evaluate.");

            debugRuleConditions.writeRuleCondition("No team individual UED values.");

            return FIT_PERCENT_NO_MATCH;
        }

        // make a copy of my target team's UED list, insert my UED and sort
        if (teamUedValues->size() < myTeamIndex)
        {
            // shouldn't happen, since we just picked this team!
            ERR_LOG("[TeamUEDBalanceRule].calcFitPercent: evaluating game '" << gameSession.getGameId() << "' had no team at index '" << myTeamIndex << "'.");

            debugRuleConditions.writeRuleCondition("Team individual UED values had no list for team at index %u.", myTeamIndex);

            return FIT_PERCENT_NO_MATCH;
        }

        // tempTeamUEDVector will be used in place of the real target team during our eval step.
        TeamUEDVector tempTeamUEDVector;
        const TeamUEDVector& targetTeamUEDVector = teamUedValues->at(myTeamIndex);
        tempTeamUEDVector.reserve(targetTeamUEDVector.size() + mUEDValueList.size());
        tempTeamUEDVector.insert(tempTeamUEDVector.end(), targetTeamUEDVector.begin(), targetTeamUEDVector.end());
        tempTeamUEDVector.insert(tempTeamUEDVector.end(), mUEDValueList.begin(), mUEDValueList.end());
        eastl::sort(tempTeamUEDVector.begin(), tempTeamUEDVector.end(), eastl::greater<UserExtendedDataValue>());

        // now see if the game is in-range for my requirements after accounting for my join
        return getDefinition().calcFitPercent(*teamUedValues, gameSession.getTeamCapacity(), myTeamIndex, tempTeamUEDVector, getAcceptableTopUEDImbalance(), getAcceptableBottomUEDImbalance(), debugRuleConditions);

    }


    const char8_t* TeamUEDPositionParityRule::makeSessionLogStr(const MatchmakingSession* session, eastl::string& buf)
    {
        if (session == nullptr)
        {
            buf.sprintf("<MMsession nullptr>");
            return buf.c_str();
        }
        buf.sprintf("(MMsessionId=%" PRIu64 ",size=%u", session->getMMSessionId(), session->getPlayerCount());
        if (!session->getCriteria().hasTeamUEDPositionParityRuleEnabled())
        {
            buf.append_sprintf(") <TeamUEDPositionParityRule disabled>");
        }
        else
        {
            buf.append_sprintf("(");
        }
        buf.append(")");
        return buf.c_str();
    }

    ////////////// Finalization Helpers


    /*! ************************************************************************************************/
    /*! \brief return whether difference between any two teams' UED values falls within the minimum acceptable UED imbalance
        specified by matchmaking sessions in the finalization list.
    *************************************************************************************************/
    bool TeamUEDPositionParityRule::areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings) const
    {
        if (!finalizationSettings.isTeamsEnabled())
        {
            return true;
        }

        // iterate over the top N players to find the lowest/highest for each position
        uint16_t topPositonsToCheck = getDefinition().getTopPlayerCount();
        if (topPositonsToCheck != 0)
        {
            const MatchmakingSession* sessionWithMinAcceptableTeamUedDifferenceForTopPlayers;
            // need to get the minimum acceptable difference for the top end.
            UserExtendedDataValue minAcceptableImbalanceForTopPlayers = getLowestAcceptableTopUEDImbalanceInFinalization(finalizationSettings, &sessionWithMinAcceptableTeamUedDifferenceForTopPlayers);

            for (uint16_t i = 0; i < topPositonsToCheck; ++i )
            {
                const CreateGameFinalizationTeamInfo* lowest = nullptr;
                const CreateGameFinalizationTeamInfo* highest = nullptr;
                // this method needs to take the rank we care about as a parameter
                if (!getHighestAndLowestUEDValueAtRankFinalizingTeams(i, finalizationSettings, lowest, highest))
                    return false; // we've logged this, something was wrong with the team vectors

                UserExtendedDataValue highValueAtRank = highest->getRankedUedValue(i);
                UserExtendedDataValue lowValueAtRank = lowest->getRankedUedValue(i);

                // skip the eval if no users at this position in the teams; note we do eval if at least two teams have a user in this position
                // see impl of getHighestAndLowestTopUEDValuedFinalizingTeams()
                // otherwise, this rule could become a de-facto team size balance rule.
                if ((highValueAtRank != INVALID_USER_EXTENDED_DATA_VALUE) && (lowValueAtRank != INVALID_USER_EXTENDED_DATA_VALUE))
                {
                    UserExtendedDataValue largestTeamUedDiff = abs( highValueAtRank - lowValueAtRank );

                    TRACE_LOG("[TeamUEDPositionParityRule].areFinalizingTeamsAcceptableToCreateGame: "
                        << " UED difference at position (" << i << ") from top currently " << ((largestTeamUedDiff > minAcceptableImbalanceForTopPlayers)? "too unbalanced to finalize" : "acceptably balanced")
                        << " for the (" << finalizationSettings.getTotalSessionsCount() << ") MM sessions in the potential match, on Team UED imbalance '" << largestTeamUedDiff << "' with lowest top UED of '" << lowValueAtRank
                        << "' and highest of '" << highValueAtRank  << "' across '" << finalizationSettings.mCreateGameFinalizationTeamInfoVector.size() << "' total teams. The MM session with lowest team UED imbalance tolerance(" 
                        << minAcceptableImbalanceForTopPlayers << ") was id='" << ((sessionWithMinAcceptableTeamUedDifferenceForTopPlayers != nullptr)? sessionWithMinAcceptableTeamUedDifferenceForTopPlayers->getMMSessionId() : INVALID_MATCHMAKING_SESSION_ID) << "'.");

                    if (largestTeamUedDiff > minAcceptableImbalanceForTopPlayers)
                        return false;
                }
                else
                {
                    TRACE_LOG("[TeamUEDPositionParityRule].areFinalizingTeamsAcceptableToCreateGame: "
                        << " UED difference at position (" << i << ") from top incalcuable, not enough players to make comparison, teams are acceptable.");
                    return true; // couldn't find two teams with occupied position, move on. We can short-circuit eval if we can't fill out the top-N
                }
            }
        }

        // now test the bottom N players
        uint16_t bottomPositionsToCheck = getDefinition().getBottomPlayerCount();
        if (bottomPositionsToCheck != 0)
        {
            const MatchmakingSession* sessionWithMinAcceptableTeamUedDifferenceForBottomPlayers;
            // need to get the minimum acceptable difference for the bottom end.
            UserExtendedDataValue minAcceptableImbalanceForBottomPlayers = getLowestAcceptableBottomUEDImbalanceInFinalization(finalizationSettings, &sessionWithMinAcceptableTeamUedDifferenceForBottomPlayers);

            uint16_t teamCapacity = getCriteria().getTeamCapacity();


            if (teamCapacity < bottomPositionsToCheck)
            {
                // something is wrong, we are trying to eval more bottom positions than the game has capacity for
                ERR_LOG("[TeamUEDPositionParityRule].areFinalizingTeamsAcceptableToCreateGame: "
                    << " trying to evaluate (" << bottomPositionsToCheck << ") from bottom, with team capacity of (" << teamCapacity << ").");
                return false;
            }

            for (uint16_t i = 0; i < bottomPositionsToCheck; ++i )
            {
                // magic number to keep bottom index in bounds
                uint16_t bottomIndex = teamCapacity - i - 1;

                const CreateGameFinalizationTeamInfo* lowest = nullptr;
                const CreateGameFinalizationTeamInfo* highest = nullptr;
                // this method needs to take the rank we care about as a parameter
                if (!getHighestAndLowestUEDValueAtRankFinalizingTeams(bottomIndex, finalizationSettings, lowest, highest))
                    return false;//logged

                UserExtendedDataValue highValueAtRank = highest->getRankedUedValue(bottomIndex);
                UserExtendedDataValue lowValueAtRank = lowest->getRankedUedValue(bottomIndex);

                // skip the eval if no users at this position in the teams; note we do eval if at least two teams have a user in this position
                // see impl of getHighestAndLowestTopUEDValuedFinalizingTeams()
                // otherwise, this rule could become a de-facto team size balance rule.
                if ((highValueAtRank != INVALID_USER_EXTENDED_DATA_VALUE) && (lowValueAtRank != INVALID_USER_EXTENDED_DATA_VALUE))
                {
                    UserExtendedDataValue largestTeamUedDiff = abs( highValueAtRank - lowValueAtRank );

                    TRACE_LOG("[TeamUEDPositionParityRule].areFinalizingTeamsAcceptableToCreateGame: "
                        << " UED difference at position (" << i << ") from bottom currently " << ((largestTeamUedDiff > minAcceptableImbalanceForBottomPlayers)? "too unbalanced to finalize" : "acceptably balanced")
                        << " for the (" << finalizationSettings.getTotalSessionsCount() << ") MM sessions in the potential match, on Team UED imbalance , on Team UED imbalance '" << largestTeamUedDiff << "' with lowest bottom UED of '" << lowValueAtRank
                        << "' and highest of '" << highValueAtRank  << "' across '" << finalizationSettings.mCreateGameFinalizationTeamInfoVector.size() << "' total teams. The MM session with lowest team UED imbalance tolerance(" 
                        << minAcceptableImbalanceForBottomPlayers << ") was id='" << ((sessionWithMinAcceptableTeamUedDifferenceForBottomPlayers != nullptr)? sessionWithMinAcceptableTeamUedDifferenceForBottomPlayers->getMMSessionId() : INVALID_MATCHMAKING_SESSION_ID) << "'.");

                    if (largestTeamUedDiff > minAcceptableImbalanceForBottomPlayers)
                        return false;
                }
                else
                {
                    // can't early out, since we work up from bottom
                    TRACE_LOG("[TeamUEDPositionParityRule].areFinalizingTeamsAcceptableToCreateGame: "
                        << " UED difference at position (" << i << ") from bottom incalcuable, not enough players to make comparison, teams are acceptable at this rank.");
                }
            }
        }

        return true;
    }

    bool TeamUEDPositionParityRule::getHighestAndLowestUEDValueAtRankFinalizingTeams(uint16_t rankIndex, const CreateGameFinalizationSettings& finalizationSettings, const CreateGameFinalizationTeamInfo*& lowest, const CreateGameFinalizationTeamInfo*& highest, TeamId teamId) const
    {
        if (!finalizationSettings.isTeamsEnabled())
        {
            ERR_LOG("[TeamUEDPositionParityRule].getHighestAndLowestUEDValueAtRankFinalizingTeams: teams not enabled.");
            return false;
        }

        size_t teamCapacitiesVectorSize = finalizationSettings.mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            const CreateGameFinalizationTeamInfo& teamInfo = finalizationSettings. mCreateGameFinalizationTeamInfoVector[i];

            bool isTeamIdMatch = ((teamId == ANY_TEAM_ID) || (teamInfo.mTeamId == teamId) || (teamInfo.mTeamId == ANY_TEAM_ID));

            UserExtendedDataValue uedValue = teamInfo.getRankedUedValue(rankIndex);


            // the getTopRankedUEDValue method returns INVALID_USER_EXTENDED_DATA_VALUE if a team has no member at the target rank.
            // we don't want to fail a match because of player count; there are other rules that do that
            // this means that if a team has too few players, we ignore it for comparison purposes,
            // unless every team is missing a player at the given rank from the top, and we'll only compare *populated* positions.

            // get the highest valued team for the team Id we have specified
            if (isTeamIdMatch && ((highest == nullptr) || 
                (highest->getRankedUedValue(rankIndex) == INVALID_USER_EXTENDED_DATA_VALUE) || 
                ((uedValue != INVALID_USER_EXTENDED_DATA_VALUE) && (uedValue > highest->getRankedUedValue(rankIndex)))))
            {
                highest = &teamInfo;
            }
            // get the lowest valued team for the team Id we have specified
            if (isTeamIdMatch && ((lowest == nullptr) || 
                (lowest->getRankedUedValue(rankIndex) == INVALID_USER_EXTENDED_DATA_VALUE) || 
                ((uedValue != INVALID_USER_EXTENDED_DATA_VALUE) && (uedValue <= lowest->getRankedUedValue(rankIndex)))))
            {
                lowest = &teamInfo;
            }
        }

        if ((highest == nullptr) || (lowest == nullptr))
        {
            ERR_LOG("[TeamUEDPositionParityRule].getHighestAndLowestUEDValueAtRankFinalizingTeams: missing teams: highest UED at rank (" << rankIndex << ") team was " << ((highest == nullptr)? "N/A" : "available") << ", lowest UED valued team was " << ((lowest == nullptr)? "N/A" : "available"));
            return false;
        }
        SPAM_LOG("[TeamUEDPositionParityRule].getHighestAndLowestUEDValueAtRankFinalizingTeams: at rank (" << rankIndex << ") from top, highest UED valued " << highest->toLogStr() << ", lowest UED valued " << lowest->toLogStr());
        return true;
    }

    UserExtendedDataValue TeamUEDPositionParityRule::getLowestAcceptableTopUEDImbalanceInFinalization(const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession** lowestAcceptableTopUEDImbalanceSession) const
    {
        // Init with my self to start
        const MatchmakingSession* sessionWithMinAcceptableTopUEDDifference = getCriteria().getMatchmakingSession();
        UserExtendedDataValue lowestAcceptableTopUEDImbalance = getAcceptableTopUEDImbalance();

        // Pre: all of sessions by finalization here are using the same Team UED rule (and team UED formula etc) to match,
        // (otherwise they'd have failed evaluation earlier). So we can just compare their one TeamUED imbalance member to member below.
        for (MatchmakingSessionList::const_iterator itr = finalizationSettings.getMatchingSessionList().begin(); itr != finalizationSettings.getMatchingSessionList().end(); ++itr)
        {
            UserExtendedDataValue bal = (*itr)->getCriteria().getTeamUEDPositionParityRule()->getAcceptableTopUEDImbalance();
            if (bal < lowestAcceptableTopUEDImbalance)
            {
                lowestAcceptableTopUEDImbalance = bal;
                sessionWithMinAcceptableTopUEDDifference = (*itr);
            }
        }
        if (sessionWithMinAcceptableTopUEDDifference == nullptr)
        {
            WARN_LOG("[TeamUEDPositionParityRule].getLowestAcceptableTopUEDImbalanceInFinalization: no session found, the finalization list of sessions to finalize with was empty.");
        }
        if (lowestAcceptableTopUEDImbalanceSession != nullptr)
            *lowestAcceptableTopUEDImbalanceSession = sessionWithMinAcceptableTopUEDDifference;
        return lowestAcceptableTopUEDImbalance;
    }

    UserExtendedDataValue TeamUEDPositionParityRule::getLowestAcceptableBottomUEDImbalanceInFinalization(const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession** lowestAcceptableBottomUEDImbalanceSession) const
    {
        // Init with my self to start
        const MatchmakingSession* sessionWithMinAcceptableBottomUEDDifference = getCriteria().getMatchmakingSession();
        UserExtendedDataValue lowestAcceptableBottomUEDImbalance = getAcceptableBottomUEDImbalance();

        // Pre: all of sessions by finalization here are using the same Team UED rule (and team UED formula etc) to match,
        // (otherwise they'd have failed evaluation earlier). So we can just compare their one TeamUED imbalance member to member below.
        for (MatchmakingSessionList::const_iterator itr = finalizationSettings.getMatchingSessionList().begin(); itr != finalizationSettings.getMatchingSessionList().end(); ++itr)
        {
            UserExtendedDataValue bal = (*itr)->getCriteria().getTeamUEDPositionParityRule()->getAcceptableBottomUEDImbalance();
            if (bal < lowestAcceptableBottomUEDImbalance)
            {
                lowestAcceptableBottomUEDImbalance = bal;
                sessionWithMinAcceptableBottomUEDDifference = (*itr);
            }
        }
        if (sessionWithMinAcceptableBottomUEDDifference == nullptr)
        {
            WARN_LOG("[TeamUEDPositionParityRule].getLowestAcceptableBottomUEDImbalanceInFinalization: no session found, the finalization list of sessions to finalize with was empty.");
        }
        if (lowestAcceptableBottomUEDImbalanceSession != nullptr)
            *lowestAcceptableBottomUEDImbalanceSession = sessionWithMinAcceptableBottomUEDDifference;
        return lowestAcceptableBottomUEDImbalance;
    }

    bool TeamUEDPositionParityRule::removeSessionFromTeam(TeamUEDVector& teamUedVector) const
    {
        // remove my mm session member's UED values from the teamUEDVector
        for (size_t i = 0; i < mUEDValueList.size(); ++i)
        {
            bool foundUEDValue = false;
            // team UED vector is probably sorted, so have to iterate
            TeamUEDVector::iterator iter = teamUedVector.begin();
            TeamUEDVector::iterator end = teamUedVector.end();
            for (; iter != end; ++iter)
            {
                if (*iter == mUEDValueList[i])
                {
                    // found a match, erase and bail
                    teamUedVector.erase(iter);
                    foundUEDValue = true;
                    break;
                }
            }

            if (!foundUEDValue)
            {
                  ERR_LOG("[TeamUEDPositionParityRule].removeSessionFromTeam failed to find expected UED value '" << mUEDValueList[i] << "' to remove from the member UED list, on removal of member session from the team/group. The cached off list of UED values appear to be out of sync with actual members in the team/group.");
                  return false;
            }
        }

        return true;
    }

    // End Finalization Helpers

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
