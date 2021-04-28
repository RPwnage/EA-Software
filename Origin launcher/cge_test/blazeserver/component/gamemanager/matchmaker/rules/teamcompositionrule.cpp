/*! ************************************************************************************************/
/*!
    \file teamcompositionrule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucketutil.h" // for getHighestSessionsBucketInMap() in selectNextBucketForFinalization()
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamCompositionRule::TeamCompositionRule(const TeamCompositionRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(definition, matchmakingAsyncStatus),
          mJoiningPlayerCount(0),
          mCachedTeamSelectionCriteria(nullptr),
          mIsFindMode(false),
          mGroupAndFitThresholdMatchInfoCache(nullptr)
    {
    }

    TeamCompositionRule::TeamCompositionRule(const TeamCompositionRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus),
          mJoiningPlayerCount(other.mJoiningPlayerCount),
          mCachedTeamSelectionCriteria(other.mCachedTeamSelectionCriteria),//side: set for code consistency though this member only needed for FG which doesn't have subsessions
          mIsFindMode(other.mIsFindMode),
          mGroupAndFitThresholdMatchInfoCache(other.mGroupAndFitThresholdMatchInfoCache)
    {
    }

    TeamCompositionRule::~TeamCompositionRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamCompositionRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamCompositionRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
        // This rule only applies to Find Game and Create Game matchmaking
        if (!matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo() || 
            matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            return true;
        }
        mIsFindMode = (matchmakingSupplementalData.mMatchmakingContext.isSearchingGames());

        const TeamCompositionRulePrefs& rulePrefs = criteriaData.getTeamCompositionRulePrefs();
        if (rulePrefs.getMinFitThresholdName()[0] == '\0')
        {
            // Rule is disabled (mMinFitThresholdName == nullptr)
            return true;
        }
        // This rule is disabled unless the criteria rule name is for it.
        if (blaze_stricmp(getRuleName(), rulePrefs.getRuleName()) != 0)
        {
            return true;
        }

        const char8_t* listName = rulePrefs.getMinFitThresholdName();
        if ((listName[0] != '\0') && (rulePrefs.getRuleName()[0] == '\0'))
        {
            WARN_LOG("[TeamCompositionRule].initialize No specified rule name in matchmaking criteria, while specifying a min fit threshold list '" << rulePrefs.getMinFitThresholdName() << "'. No rule will be initialized.");
            return true;
        }

        if (!initMinFitThreshold(listName, mRuleDefinition, err))
        {
            return false;
        }

        eastl::string errBuf;
        if (matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size() > 1)
        {
            err.setErrMessage(errBuf.sprintf("TeamCompositionRule does not support joining into multiple teams (%u > 1).", matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size()).c_str());
            return false;
        }

        // rule's poss composition's team capacity should be no more than max team capacity derived from other rules
        const uint16_t rulesMaxPossSlots = getCriteria().calcMaxPossPlayerSlotsFromRules(criteriaData, matchmakingSupplementalData,
            getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
        if (getDefinition().getTeamCapacityForRule() > (rulesMaxPossSlots / getDefinition().getTeamCount()))
        {
            err.setErrMessage(errBuf.sprintf("TeamCompositionRule's configured teamCapacity(%u) must be <= max possible total slots per team based on other total player slots rule's criteria (%u).", getDefinition().getTeamCapacityForRule(), rulesMaxPossSlots).c_str());
            return false;
        }
        // games are expected full for CG
        if (matchmakingSupplementalData.mMatchmakingContext.isSearchingPlayers())
        {
            uint16_t rulesMinPossParticipants = getCriteria().calcMinPossPlayerCountFromRules(criteriaData, matchmakingSupplementalData,
                getDefinition().getGlobalMinPlayerCountInGame());
            if(getDefinition().getTeamCapacityForRule() < (rulesMinPossParticipants / getDefinition().getTeamCount()))
            {
                err.setErrMessage(errBuf.sprintf("TeamCompositionRule's configured teamCapacity(%u) must be >= min possible participant count per team based on other participant count rule's criteria (%u).", getDefinition().getTeamCapacityForRule(), rulesMinPossParticipants).c_str());
                return false;
            }
        }

        // side: don't need to check if we're doing GB here (and set joining count 0), since this rule is n/a for GB.
        mJoiningPlayerCount = (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount;

        // set ref to the team selection criteria, which rule will automatically update for the session
        mCachedTeamSelectionCriteria = matchmakingSupplementalData.mCachedTeamSelectionCriteria;

        // validate session's group size can ever be matched from the start
        mGroupAndFitThresholdMatchInfoCache = getDefinition().getMatchInfoCacheByGroupSizeAndMinFit(
            (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount, mMinFitThresholdList->getMinFitThreshold(0));
        if ((mGroupAndFitThresholdMatchInfoCache == nullptr) ||
            mGroupAndFitThresholdMatchInfoCache->mAcceptableGameTeamCompositionsVector.empty())
        {
            err.setErrMessage(errBuf.sprintf("matchmaking session has a group size(%u) that can never be matched based on configured fit table and/or min fit threshold list(%s, initial fit %1.2f) for rule definition \"%s\".", (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount, rulePrefs.getMinFitThresholdName(), mMinFitThresholdList->getMinFitThreshold(0), getRuleName()).c_str());
            return false;
        }        

        TRACE_LOG("[TeamCompositionRule].initialize rule '" << getRuleName() << "' initialized for Session(" << getSessionLogStr() << "). Initial matchable game team compositions are " << getCurrentAcceptableGtcVectorInfoCacheLogStr() << ".");
        return true;
    }


    Rule* TeamCompositionRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamCompositionRule(*this, matchmakingAsyncStatus);
    }

    /*! ************************************************************************************************/
    /*! \brief add RETE conditions. Note: final full filtering is completed post-RETE (see).
    *************************************************************************************************/
    bool TeamCompositionRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        FitScore fitScore = 0; // 0 (re-calc at post rete)

        // 1. search cond: games have team space that fits at least my size
        Rete::OrConditionList& teamSpaceOrConditions = conditions.push_back();

        // range condition - match all acceptable space that will fit who i am bringing with
        teamSpaceOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(
            TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_SPACE, getJoiningPlayerCount(), getTeamCapacityForRule(), getDefinition()), 
            fitScore, this));


        // 2. search cond: negate games with group sizes that violate my team compositions
        if (mGroupAndFitThresholdMatchInfoCache == nullptr)
        {
            ERR_LOG("[TeamCompositionRule].addConditions: internal error: no cached info on acceptable group sizes available. RETE may not properly filter out games by their violating group sizes.");
            return true;
        }
        for (uint16_t i = getTeamCapacityForRule(); i >= 1 ; --i)
        {
            // We will only match games whose group sizes are ALL in this set of acceptable group sizes.
            // pre: this set only includes my size if a matchable game's team compositions included more than one instance of my size.
            if (mGroupAndFitThresholdMatchInfoCache->isOtherGroupSizeAcceptable(i))
                continue;

            // Note we 'AND' the (negated) conditions here by adding new item to the condition block
            Rete::OrConditionList& presentGroupSizeConditions = conditions.push_back();
            
             const bool negate = true;
             eastl::string buf;
             buf.append_sprintf("%s_%u", TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_GROUP_SIZE, i);
             presentGroupSizeConditions.push_back(Rete::ConditionInfo(Rete::Condition(
                 buf.c_str(), getDefinition().getWMEBooleanAttributeValue(true), getDefinition(), negate), 
                 fitScore, this));
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief override to additionally cache off data about the current acceptable game team compositions
    *************************************************************************************************/
    UpdateThresholdResult TeamCompositionRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // If the rule is disabled or if the MinFitThresholdList is null we don't decay.
        if (isDisabled() || (mMinFitThresholdList == nullptr))
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        float originalThreshold = mCurrentMinFitThreshold;
        mCurrentMinFitThreshold = mMinFitThresholdList->getMinFitThreshold(elapsedSeconds, &mNextUpdate);

        if (forceUpdate || originalThreshold != mCurrentMinFitThreshold)
        {
            // refresh the cached acceptable team compositions, for the current min fit
            mGroupAndFitThresholdMatchInfoCache = getDefinition().getMatchInfoCacheByGroupSizeAndMinFit(
                getJoiningPlayerCount(), mCurrentMinFitThreshold);

            if ((mGroupAndFitThresholdMatchInfoCache == nullptr) || getAcceptableGameTeamCompositionsInfoVector().empty())
            {
                ERR_LOG("[TeamCompositionRule].updateCachedThresholds: at " << elapsedSeconds << "s, my session(" << getSessionLogStr() << ") found NO acceptable game team compostions for rule(" << getRuleName() << "). No matches will be possible. The matchmaking configuration and/or inputs invalidly disallow matchability.");
            }

            TRACE_LOG("[TeamCompositionRule].updateCachedThresholds: my session(" << getSessionLogStr() << ") rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                << "' elapsedSeconds '" << elapsedSeconds << "' originalThreshold '" << originalThreshold << "' currentThreshold '" 
                << mCurrentMinFitThreshold << "', has acceptable game team compostions for rule(" << getRuleName() << ") " << getCurrentAcceptableGtcVectorInfoCacheLogStr());

            updateCachedTeamSelectionCriteria();
            updateAsyncStatus();
            return RULE_CHANGED;
        }
        return NO_RULE_CHANGE;
    }

    /*! ************************************************************************************************/
    /*! \brief (for FG) to refresh the cached criteria team selection crit, and maintain the sorted order.
        While at it update the game sizes cache
    *************************************************************************************************/
    void TeamCompositionRule::updateCachedTeamSelectionCriteria()
    {
        //only FG needs team selection criteria
        if (mIsFindMode)
        {
            // pre: order of acceptable game team compositions info vector is by non-asc preference
            // reserve clears any existing contents
            mCachedTeamSelectionCriteria->getAcceptableGameTeamCompositionsList().reserve(getAcceptableGameTeamCompositionsInfoVector().size());
            for (size_t i = 0; i < getAcceptableGameTeamCompositionsInfoVector().size(); ++i)
            {
                const GameTeamCompositionsInfo& gtcInfo = getAcceptableGameTeamCompositionsInfoVector()[i];
                if (gtcInfo.getCachedGroupSizeCountsByTeam() != nullptr)
                    gtcInfo.getCachedGroupSizeCountsByTeam()->copyInto(*mCachedTeamSelectionCriteria->getAcceptableGameTeamCompositionsList().pull_back());
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamCompositionRule::updateAsyncStatus()
    {
        // Side: TeamCompositionRule's async statuses are a bit more work to build, so we cache them built out up front
        // for each fit-threshold/group-size. Pre: updateCachedThresholds updated mGroupAndFitThresholdMatchInfoCache.
        if (mGroupAndFitThresholdMatchInfoCache == nullptr)
        {
            ERR_LOG("[TeamCompositionRule].updateAsyncStatus: internal error: no cached info on acceptable game team compositions. Unable to update async status info.");
            return;
        }
        mGroupAndFitThresholdMatchInfoCache->mTeamCompositionRuleStatus.copyInto(mMatchmakingAsyncStatus->getTeamCompositionRuleStatus());
    }

    /*! ************************************************************************************************/
    /*! \brief Return the current list of acceptable game team compositions infos.
    *************************************************************************************************/
    const GameTeamCompositionsInfoVector& TeamCompositionRule::getAcceptableGameTeamCompositionsInfoVector() const 
    {
        if ((mGroupAndFitThresholdMatchInfoCache == nullptr) || mGroupAndFitThresholdMatchInfoCache->mAcceptableGameTeamCompositionsVector.empty())
        {
            ERR_LOG("[TeamCompositionRule].getAcceptableGameTeamCompositionsInfoVector: internal error: (" << getSessionLogStr() << ") unable to process any potential game team compositions. Cached acceptable game teams compositions info was " << ((mGroupAndFitThresholdMatchInfoCache == nullptr)? "not initialized":"empty") << ", for this rule(" << getRuleName() << ").");
            return mEmptyAcceptableGameTeamCompositionsInfoVector;
        }
        return mGroupAndFitThresholdMatchInfoCache->mAcceptableGameTeamCompositionsVector;
    }

    /*! ************************************************************************************************/
    /*! \brief eval vs another rule, return FIT_PERCENT_NO_MATCH if no match, else return 0 fit pct. See helper called.
    *************************************************************************************************/
    void TeamCompositionRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const TeamCompositionRule& otherTeamCompositionRule = static_cast<const TeamCompositionRule &>(otherRule);
        

        if (otherTeamCompositionRule.getId() != getId())
        {
            WARN_LOG("[TeamCompositionRule].doCalcFitPercent: (" << getSessionLogStr() << ") evaluating different TeamCompositionRule against each other, my rule(" << getSessionLogStr() << ") '" << getDefinition().getName() << "' id '" << getId() << "', vs other's rule(sessionId=" << otherTeamCompositionRule.getMMSessionId() << ") '" << otherTeamCompositionRule.getDefinition().getName() << "' id '" << otherTeamCompositionRule.getId() << "'. Internal rule indices or rule id's out of sync. No match.");
            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;

            debugRuleConditions.writeRuleCondition("My team composition rule %u != %u", getId(), otherTeamCompositionRule.getId());

            return;
        }
        if (!otherTeamCompositionRule.getCriteria().hasTeamCompositionRuleEnabled(getId()))
        {
            WARN_LOG("[TeamCompositionRule].doCalcFitPercent: (" << getSessionLogStr() << ") evaluating different TeamCompositionRule against each other, my rule(" << getSessionLogStr() << ") '" << getDefinition().getName() << "' id '" << getId() << "', vs other's rule(sessionId=" << otherTeamCompositionRule.getMMSessionId() << ") '" << otherTeamCompositionRule.getDefinition().getName() << "' id '" << otherTeamCompositionRule.getId() << "'. Internal rule indices or rule id's out of sync. No match.");
            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;

            debugRuleConditions.writeRuleCondition("Other team composition rule %u not enabled", otherTeamCompositionRule.getId());

            return;
        }
        fitPercent = doCalcFitPercent(otherTeamCompositionRule, isExactMatch, debugRuleConditions);
    }

    /*! ************************************************************************************************/
    /*! \brief eval vs another rule, return FIT_PERCENT_NO_MATCH if no match, else return 0 fit pct.

        This helper filters out sessions, checking whether my other's group sizes and rule criteria's are
        potentially compatible for a match.
        Note: We rely on finalization stage session picking (based on this and other rules) later to
        determine whether the other session actually gets added to the finalized game.

    *************************************************************************************************/
    float TeamCompositionRule::doCalcFitPercent(const TeamCompositionRule& otherTeamCompositionRule, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        isExactMatch = false;
        
        // sanity check rules' team counts and capacities match
        if ((otherTeamCompositionRule.getTeamCapacityForRule() != getTeamCapacityForRule()) || (otherTeamCompositionRule.getTeamCount() != getTeamCount()))
        {
            debugRuleConditions.writeRuleCondition("Team capacities and team counts must be equal for all possible game team compositions. Other rule(%s)'s team capacity(%u) or team count(%u), does not match my rule(%s)'s team capacity(%u) or team count(%u).]",
                otherTeamCompositionRule.getSessionLogStr(), otherTeamCompositionRule.getTeamCapacityForRule(), otherTeamCompositionRule.getTeamCount(), getSessionLogStr(), getTeamCapacityForRule(), getTeamCount());
            return FIT_PERCENT_NO_MATCH;
        }

        // first see if we have an 'exact match' possible
        if (mMinFitThresholdList->isExactMatchRequired())//side: this is whether *has* an exact-match-required in thresholds list
        {
            // pre: already cached off our acceptable other session group sizes for our current fit threshold.
            const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCache* gtcsForExactMatch =
                getDefinition().getMatchInfoCacheByGroupSizeAndMinFit(getJoiningPlayerCount(), MinFitThresholdList::EXACT_MATCH_REQUIRED);
            if ((gtcsForExactMatch != nullptr) && gtcsForExactMatch->isOtherGroupSizeAcceptable(otherTeamCompositionRule.getJoiningPlayerCount()))
            {
                TRACE_LOG("[TeamCompositionRule].doCalcFitPercent my session(" << getSessionLogStr() << ") found for exact-match, session(" << otherTeamCompositionRule.getSessionLogStr() << ") to attempt finalizion with, once/given my fit threshold (currently '" << mCurrentMinFitThreshold << "') is >= '" << gtcsForExactMatch->mMinFitThreshold << "'. My acceptable list " << gtcsForExactMatch->toLogStr() << ".");
                isExactMatch = gtcsForExactMatch->mAllExactMatches;
                return gtcsForExactMatch->mFitPercentMin;
            }
        }

        // Find acceptable game team compositions. Pre: map ordered non-ascending fit threshold
        // Note: we need to actually return the earliest fit percent back to caller (can't just return any arbitrary fit pct)
        // in order for decay times to be calculated properly by SimpleRule interface.
        const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCacheByMinFitMap* acceptableGtcsByMinFitMap =
            getDefinition().getMatchInfoCacheByMinFitMap(getJoiningPlayerCount());
        if ((acceptableGtcsByMinFitMap != nullptr) && !acceptableGtcsByMinFitMap->empty())
        {
            TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCacheByMinFitMap::const_iterator itr;
            for (itr = acceptableGtcsByMinFitMap->begin(); itr != acceptableGtcsByMinFitMap->end(); ++itr)
            {
                // exact-match one, already checked first above (skippable)
                if (itr->first == MinFitThresholdList::EXACT_MATCH_REQUIRED)
                {
                    continue;
                }

                // filter by group size
                const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCache& acceptableGtcsInfo = itr->second;
                if (!acceptableGtcsInfo.isOtherGroupSizeAcceptable(otherTeamCompositionRule.getJoiningPlayerCount()))
                {
                    SPAM_LOG("[TeamCompositionRule].doCalcFitPercent my session(" << getSessionLogStr() << ") other session(" << otherTeamCompositionRule.getSessionLogStr() << ") incompatible with my possible acceptable " << acceptableGtcsInfo.toLogStr() << ". My acceptable list " << acceptableGtcsInfo.toLogStr() << ".");
                    continue;
                }
                // Match found. Approximate the score between the two sessions as the worst case fit for the time threshold (even though finalization determines actual compositions).
                // Note: as described above, this is required in order for correct decay using SimpleRule interface.
                TRACE_LOG("[TeamCompositionRule].doCalcFitPercent my session(" << getSessionLogStr() << ") found session(" << otherTeamCompositionRule.getSessionLogStr() << ") to attempt finalizion with, once/given my fit threshold (currently '" << mCurrentMinFitThreshold << "') is >= '" << acceptableGtcsInfo.mMinFitThreshold << "'. My acceptable list " << acceptableGtcsInfo.toLogStr() << ".");
                isExactMatch = acceptableGtcsInfo.mAllExactMatches;//note: We treat as non-exact (pref-only) if any gtc isn't an exact match in the list of acceptables here.
                return acceptableGtcsInfo.mFitPercentMin;
            }
        }
        // Found no acceptable game team compositions, compatible with me and other session
        debugRuleConditions.writeRuleCondition("Other rule(%s)'s size(%u), is not an acceptable size for any of my rule(%s)'s acceptable game team compositions, at any time. Currently acceptable %s.]",
            otherTeamCompositionRule.getSessionLogStr(), otherTeamCompositionRule.getJoiningPlayerCount(), getSessionLogStr(), getCurrentAcceptableGtcVectorInfoCacheLogStr());
        return FIT_PERCENT_NO_MATCH;
    }


    /*! ************************************************************************************************/
    /*! \brief evaluates whether requirements are met (used post RETE). See called helper.
    *************************************************************************************************/
    void TeamCompositionRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        if (getAcceptableGameTeamCompositionsInfoVector().empty())
        {
            WARN_LOG("[TeamCompositionRule].calcFitPercent: (" << getSessionLogStr() << ") rule '" << getRuleName() << "' has NO acceptable game team compositions currently matchable.");
            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;

            debugRuleConditions.writeRuleCondition("No team compositions matchable");

            return;
        }
        fitPercent = doCalcFitPercent(gameSession, isExactMatch, debugRuleConditions);
    }
    
    /*! ************************************************************************************************/
    /*! \brief For FG, determines which team will be chosen to join, and returns based on that, the fit percent.
        If my joining the team in game would not cause game to be full, figures out what are the game's
        potential future-filled game team compositions to determine the fit score/match-ability.

        Note: This Post-RETE is the final precise filter
        (RETE serves as a broad filter, but does not accurately account for the team you'll join nor cases of non full games)

        \return The worst-case fit percent for the match as described above, or FIT_PERCENT_NO_MATCH if no match can be made.
    *************************************************************************************************/
    float TeamCompositionRule::doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        isExactMatch = false;

        // early out if poss
        if (gameSession.getTeamCount() != getDefinition().getTeamCount())
        {
            debugRuleConditions.writeRuleCondition("Game team count %u != %u", gameSession.getTeamCount(), getDefinition().getTeamCount());

            return FIT_PERCENT_NO_MATCH;
        }
        if (gameSession.getTeamCapacity() != getTeamCapacityForRule())
        {
            debugRuleConditions.writeRuleCondition("Game team capacity %u != %u", gameSession.getTeamCapacity(), getTeamCapacityForRule());

            return FIT_PERCENT_NO_MATCH;
        }
        if ((gameSession.getPlayerRoster()->getParticipantCount() + getJoiningPlayerCount()) > gameSession.getTotalParticipantCapacity())
        {
            debugRuleConditions.writeRuleCondition("%u resulting players > %u game's capacity", gameSession.getPlayerRoster()->getParticipantCount() + getJoiningPlayerCount(), gameSession.getTotalParticipantCapacity());

            return FIT_PERCENT_NO_MATCH;
        }

        // 1. find the 'best' team (possibly joinable) based on all team selection criteria in the game.
        // 2. check which acceptable game team compositions won't be violated, by joining that team. If none, no match.
        // 3. If my join won't lead to full game, check that *all* poss future outcomes of other joins must also full-fill some acceptable game team compositions.

        // 1. Get the team we'd actually be joining (at the master) based on all my MM criteria.
        TeamIndex myTeamIndex = UNSPECIFIED_TEAM_INDEX;
        TeamId myTeamId = INVALID_TEAM_ID;
        const TeamIdRoleSizeMap* roleSizeMap = getCriteria().getTeamIdRoleSpaceRequirementsFromRule();
        if (roleSizeMap == nullptr || roleSizeMap->size() != 1)
            return FIT_PERCENT_NO_MATCH;
        
        BlazeRpcError err = gameSession.findOpenTeam(getCriteria().getTeamSelectionCriteriaFromRules(),
            roleSizeMap->begin(), nullptr, myTeamIndex);
        myTeamId = gameSession.getTeamIdByIndex(myTeamIndex);
        if ((err != ERR_OK) || (myTeamIndex == UNSPECIFIED_TEAM_INDEX))
        {
            debugRuleConditions.writeRuleCondition("Game doesn't have joinable team in %s", gameSession.getPlayerRoster()->getTeamAttributesLogStr());

            return FIT_PERCENT_NO_MATCH;
        }

        // 2. Now knowing my team, get all acceptable game team compositions ('GTC's) that can be full-filled,
        // with my join, and/or additional joins after me (if not full).
        GameTeamCompositionsInfoPtrVector acceptableGtcsAfterKnowMyTeam;
        for (size_t i = 0; i < getAcceptableGameTeamCompositionsInfoVector().size(); ++i)
        {
            const GameTeamCompositionsInfo& gtcInfo = getAcceptableGameTeamCompositionsInfoVector()[i];
            if ((gtcInfo.getCachedGroupSizeCountsByTeam() != nullptr) &&
                !gameSession.doesGameViolateGameTeamCompositions(*gtcInfo.getCachedGroupSizeCountsByTeam(), getJoiningPlayerCount(), myTeamIndex))
            {
                acceptableGtcsAfterKnowMyTeam.push_back(&gtcInfo);
            }
        }

        // check if found any GTCs
        if (acceptableGtcsAfterKnowMyTeam.empty())
        {
            debugRuleConditions.writeRuleCondition("No compatible game team compositions for joining %u players to teamIndex %u", getJoiningPlayerCount(), myTeamIndex);

            return FIT_PERCENT_NO_MATCH;
        }
        else
        {
            //found compatible GTCs
            if (IS_LOGGING_ENABLED(Logging::TRACE))
            {
                eastl::string grpLogBuf;
                SPAM_LOG("[TeamCompositionRule].doCalcFitPercent: Potentially compatible game team compositions' group-size cached data: " << gameTeamCompositionsInfoPtrVectorToLogStr(acceptableGtcsAfterKnowMyTeam, grpLogBuf) << ".");
            }

            // Pre: the vector already sorted non-ascending fit above
            const GameTeamCompositionsInfo* lowestFitGtcInfo = acceptableGtcsAfterKnowMyTeam.back();

            // 3a. see if full upon my join. If so its a sure match
            if ((gameSession.getPlayerRoster()->getParticipantCount() + getJoiningPlayerCount()) >= gameSession.getTotalParticipantCapacity())
            {
                // exact match if filling game and all teams' compositions equal
                isExactMatch = areAllTeamCompositionsEqual(*lowestFitGtcInfo);

                TRACE_LOG("[TeamCompositionRule].doCalcFitPercent: Session '" << getSessionLogStr() << "' (exact) MATCH Game '" << gameSession.getGameId() << "', found for rule '" << getDefinition().getName() <<
                    "' for join to team(index=" << myTeamIndex << "). After I join there'd be '" << acceptableGtcsAfterKnowMyTeam.size() << "' total possible game team compositions, and the worst case one was: " << 
                    ((lowestFitGtcInfo != nullptr)? lowestFitGtcInfo->toFullLogStr() : "<internal error info n/a>") << ". Current acceptable: " << getCurrentAcceptableGtcVectorInfoCacheLogStr() << 
                    ". Game's pre-existing teams: " << gameSession.getPlayerRoster()->getTeamAttributesLogStr() << ".");

                debugRuleConditions.writeRuleCondition("%s team composition fills game after joining %u players to team %u", lowestFitGtcInfo->toCompositionsLogStr(), getJoiningPlayerCount(), myTeamId);
    
                return lowestFitGtcInfo->mCachedFitPercent;
            }

            // 3b. if here, joining me will not get the game full:
            // * If future joiners could *potentially* cause some game composition we don't currently accept, no match.
            // * Else its guaranteed regardless of who else joins game matches one of my acceptable game compositions, match.
            // Figure out possible future game composition outcomes.
            size_t totalMatchematicallyPossGtcs = 1;//know at least 1
            for (TeamIndex teamIndex = 0; teamIndex < gameSession.getTeamCount(); ++teamIndex)
            {
                const int32_t spaceOnTeam = (gameSession.getTeamCapacity() - gameSession.getPlayerRoster()->getTeamSize(teamIndex) -
                    ((myTeamIndex == teamIndex)? getJoiningPlayerCount() : 0));
                if (spaceOnTeam <= 0)
                    continue;

                // get total possible outcomes based on the empty space on team, and multiply to current overall tally
                size_t possOutcomesForTeam = getDefinition().getMathematicallyPossibleCompositionsForSlotcount((uint16_t)spaceOnTeam).size();
                if (possOutcomesForTeam != 0)
                    totalMatchematicallyPossGtcs *= possOutcomesForTeam;
            }

            // to check if all future joins can match, check all possible outcomes vs all possible *acceptable* outcomes.
            if (totalMatchematicallyPossGtcs != acceptableGtcsAfterKnowMyTeam.size())// check these 2 counts equal
            {
                debugRuleConditions.writeRuleCondition("Joining %u players to team %u will not fill game and leads to unacceptable future compositions.", getJoiningPlayerCount(), myTeamId);
    
                return FIT_PERCENT_NO_MATCH;
            }
            else
            {
                TRACE_LOG("[TeamCompositionRule].doCalcFitPercent: Session '" << getSessionLogStr() << "' MATCH Game '" << gameSession.getGameId() << "', found for rule '" << getDefinition().getName() << 
                    "' for join to team(index=" << myTeamIndex << "). After I join there'd be '" << acceptableGtcsAfterKnowMyTeam.size() << "' total possible game team compositions, and the worst case one (for fit score) was: " << 
                    ((lowestFitGtcInfo != nullptr)? lowestFitGtcInfo->toFullLogStr() : "<internal error info n/a>") << ". Current acceptable: " << getCurrentAcceptableGtcVectorInfoCacheLogStr() << 
                    ". Game's pre-existing teams: " << gameSession.getPlayerRoster()->getTeamAttributesLogStr() << ".");

                debugRuleConditions.writeRuleCondition("Joining %u players to team %u results in worse case %s", getJoiningPlayerCount(), myTeamId, ((lowestFitGtcInfo != nullptr)? lowestFitGtcInfo->toFullLogStr() : "<n/a>"));
    
                return lowestFitGtcInfo->mCachedFitPercent;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief calculate a weighted fit score
    ***************************************************************************************************/
    FitScore TeamCompositionRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        return ((fitPercent == FIT_PERCENT_NO_MATCH)? FIT_SCORE_NO_MATCH : static_cast<FitScore>(fitPercent * mRuleDefinition.getWeight()));
    }


    ////////////// Finalization Helpers

    /*! ************************************************************************************************/
    /*! \brief set up data on game team compositions to attempt as needed. Stores to finalization info.

            PRE: all acceptable GameTeamCompositions include my group size. I.e. we don't have to worry about
            selecting something impossible for my group size.
    ***************************************************************************************************/
    const GameTeamCompositionsInfo* TeamCompositionRule::selectNextGameTeamCompositionsForFinalization(const CreateGameFinalizationSettings& finalizationSettings) const
    {
        // pre: acceptable game team compositions already sorted in preferred order
        const GameTeamCompositionsInfo* gameTeamCompositionsInfo = nullptr;
        for (size_t i = 0; i < getAcceptableGameTeamCompositionsInfoVector().size(); ++i)
        {
            const GameTeamCompositionsInfo& gtcInfo = getAcceptableGameTeamCompositionsInfoVector()[i];
            if (finalizationSettings.wasGameTeamCompositionsIdTried(gtcInfo.mGameTeamCompositionsId))
            {
                SPAM_LOG("[TeamCompositionRule].selectNextGameTeamCompositionsForFinalization: (" << getSessionLogStr() << ") skipping game team compositions '" << gtcInfo.toFullLogStr() << "', was already tried.");
                continue;
            }
            else
            {
                TRACE_LOG("[TeamCompositionRule].selectNextGameTeamCompositionsForFinalization: (" << getSessionLogStr() << ") found untried game team compositions to try finalizing with: '" << gtcInfo.toFullLogStr() << "'.");
                gameTeamCompositionsInfo = &gtcInfo;
                return gameTeamCompositionsInfo;
            }
        }
        TRACE_LOG("[TeamCompositionRule].selectNextGameTeamCompositionsForFinalization: (" << getSessionLogStr() << ") no (more) found game team compositions to try finalizing with.");
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Return the next matched sessions bucket to try pick from for the next mm session in the finalization.
        Returns nullptr if no more sessions available to try.
    ***************************************************************************************************/
    const MatchedSessionsBucket* TeamCompositionRule::selectNextBucketForFinalization(
        const CreateGameFinalizationTeamInfo& teamToFill, const CreateGameFinalizationSettings& finalizationSettings,
        const MatchedSessionsBucketMap& bucketMap) const
    {
        TeamCompositionFilledInfo* teamCompositionToFill = teamToFill.getTeamCompositionToFill();
        if (teamCompositionToFill == nullptr)
        {
            ERR_LOG("[TeamCompositionRule].selectNextBucketForFinalization: internal error: (" << getSessionLogStr() << ") team(index=" << teamToFill.mTeamIndex << ") did not have an assigned team composition to use. Failing trying to fill team composition of '" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "', with team '" << teamToFill.toLogStr() << "'.");
            return nullptr;
        }

        uint16_t nextGroupSizeToFill = teamCompositionToFill->getNextUnfilledGroupSize();
        if (nextGroupSizeToFill == UINT16_MAX)//already full
        {
            return nullptr;
        }
        if (teamToFill.wasBucketTriedForCurrentPick(nextGroupSizeToFill, finalizationSettings.getTotalSessionsCount()))
        {
            SPAM_LOG("[TeamCompositionRule].selectNextBucketForFinalization: (" << getSessionLogStr() << ") bucket was already all tried for group size '" << nextGroupSizeToFill << "', unable to fill that group size in team composition '" << teamCompositionToFill->toLogStr() << "' in ''" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "', for team '" << teamToFill.toLogStr() << "'.");
            return nullptr;
        }

        // get the bucket for the group size
        const MatchedSessionsBucket* foundBucket = getHighestSessionsBucketInMap(nextGroupSizeToFill, nextGroupSizeToFill, bucketMap);
        if (foundBucket == nullptr)
        {
            SPAM_LOG("[TeamCompositionRule].selectNextBucketForFinalization: (" << getSessionLogStr() << ") found no available bucket to fill group size '" << nextGroupSizeToFill << "' in team composition '" << teamCompositionToFill->toLogStr() << "' in ''" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "', for team '" << teamToFill.toLogStr() << "'.");
            return nullptr;
        }

        TRACE_LOG("[TeamCompositionRule].selectNextBucketForFinalization: (" << getSessionLogStr() << ") found candidate bucket that can fill group size '" << nextGroupSizeToFill << "' in team composition '" << teamCompositionToFill->toLogStr() << "' in '" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "', for team '" << teamToFill.toLogStr() << "'. The bucket: '" << foundBucket->toLogStr() << "'.");
        return foundBucket;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether all the compositions in the current attempted game team composition, are full-filled.
    *************************************************************************************************/
    bool TeamCompositionRule::areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings) const
    {
        if (!finalizationSettings.isTeamsEnabled())
            return true;

        bool isFullfilled = finalizationSettings.getGameTeamCompositionsInfo().isFullfilled();

        TRACE_LOG("[TeamCompositionRule].areFinalizingTeamsAcceptableToCreateGame: (" << getSessionLogStr() << ")"
            << " game team compositions " << (isFullfilled? "" : "NOT")
            << " successfully full-filled, for the (" << finalizationSettings.getTotalSessionsCount() << ") MM sessions in the potential match, on '" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "'.");
        return isFullfilled;
    }

    /*! \brief logging helper, appends to buffer
    *************************************************************************************************/
    const char8_t* TeamCompositionRule::gameTeamCompositionsInfoPtrVectorToLogStr(const GameTeamCompositionsInfoPtrVector& infoVector, eastl::string& buf, bool newlinePerItem /*= false*/) const
    {
        buf.append_sprintf("(total=%" PRIsize ") ", infoVector.size());
        for (size_t i = 0; i < infoVector.size(); ++i)
        {
            buf.append_sprintf("%s[%s]%s", (newlinePerItem? "\n\t":""), infoVector[i]->toFullLogStr(), ((i+1 < infoVector.size())? ",":""));
        }
        return buf.c_str();
    }

    // End Finalization Helpers

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
