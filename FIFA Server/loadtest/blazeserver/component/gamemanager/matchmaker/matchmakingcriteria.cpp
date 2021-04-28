/*! ************************************************************************************************/
/*!
    \file   matchmakingcriteria.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/tdf/attributes.h"
#include "gamemanager/gamesession.h" // for GameSession
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmaker/matchlist.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/rules/gamestaterule.h"
#include "gamemanager/matchmaker/rules/gamesettingsrule.h"
#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/playercountruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"
#include "gamemanager/tdf/gamemanager_server.h" // for TeamSelectionCriteria
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in evaluateGame
#include "EASTL/sort.h"
#include "EASTL/algorithm.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    struct RuleIdComparator
    {
        bool operator()(const Rule* a, const Rule* b) const
        {
            return (a->getId() < b->getId());
        }
        bool operator()(RuleDefinitionId a, const Rule* b) const
        {
            return (a < b->getId());
        }
        bool operator()(const Rule* a, RuleDefinitionId b) const
        {
            return (a->getId() < b);
        }
    };
    
    //! \brief Note: you must initialize the matchmaking criteria & update the cachedInfo before evaluating it.
    MatchmakingCriteria::MatchmakingCriteria(RuleDefinitionCollection& ruleDefinitionCollection,
                                             MatchmakingAsyncStatus& matchmakingAsyncStatus, RuleDefinitionId useRuleDefinitionId)
        : mMatchmakingSession(nullptr),
          mRuleDefinitionCollection(ruleDefinitionCollection),
          mMatchmakingAsyncStatus(matchmakingAsyncStatus),
          mNextUpdate(UINT32_MAX),
          mMaxPossibleFitScore(0),
          mMaxPossiblePostReteFitScore(0),
          mMaxPossibleNonReteFitScore(0),
          mTeamUEDBalanceRuleId(RuleDefinition::INVALID_DEFINITION_ID),
          mTeamUEDPositionParityRuleId(RuleDefinition::INVALID_DEFINITION_ID),
          mTeamCompositionRuleId(RuleDefinition::INVALID_DEFINITION_ID)
    {
        EA_ASSERT(mRuleContainer.empty());
        const RuleDefinitionList& ruleDefinitionList = mRuleDefinitionCollection.getRuleDefinitionList();

        RuleDefinitionList::const_iterator itr = ruleDefinitionList.begin();
        RuleDefinitionList::const_iterator end = ruleDefinitionList.end();

        for (; itr != end; ++itr)
        {
            const RuleDefinition& ruleDefinition = **itr;

            if ((useRuleDefinitionId != RuleDefinition::INVALID_DEFINITION_ID) && 
                (ruleDefinition.getID() != useRuleDefinitionId))
                continue; // single rule specified, skip all definitions that don't match


            if (!ruleDefinition.isDisabled())
            {
                if (ruleDefinition.isDisabledForRuleCreation())
                {
                    // game size rule deprecated in place of using other rules, specially handled at criteria setup.
                    continue;
                }

                Rule* rule = ruleDefinition.createRule(&matchmakingAsyncStatus);  // This allocates a new rule owned by the MM Criteria.  Must be cleaned up later.

                if (rule != nullptr)
                {
                    TRACE_LOG("[MatchmakingCriteria].createRules Created rule for definition " << ruleDefinition.getName() << "(" << ruleDefinition.getID() << ")");
                    rule->setCriteria(this);
                    mRuleContainer.push_back(rule);
                }
                else
                {
                    ERR_LOG("[MatchmakingCriteria].createRules Failed to create rule from definition " << ruleDefinition.getName() << "(" << ruleDefinition.getID() << ")");
                }
            }
            else
            {
                TRACE_LOG("[MatchmakingCriteria].createRules Skipping rule for disabled definition " << ruleDefinition.getName() << "(" << ruleDefinition.getID() << ")");
            }
        }

        if (useRuleDefinitionId != RuleDefinition::INVALID_DEFINITION_ID)
        {
            if (mRuleContainer.empty())
            {
                ASSERT_LOG("[MatchmakingCriteria].ctor: Use single rule(" << useRuleDefinitionId << "), collection does not contain expected rule!");
            }
            else
            {
                TRACE_LOG("[MatchmakingCriteria].ctor: Use single rule(" << useRuleDefinitionId << ")");
            }
        }

        sortRules();
    }

    MatchmakingCriteria::MatchmakingCriteria(const MatchmakingCriteria &other, MatchmakingAsyncStatus& matchmakingAsyncStatus)
        : mMatchmakingSession(other.mMatchmakingSession),
          mRuleDefinitionCollection(other.mRuleDefinitionCollection),
          mMatchmakingAsyncStatus(matchmakingAsyncStatus),
          mNextUpdate(other.mNextUpdate),
          mMaxPossibleFitScore(other.mMaxPossibleFitScore),
          mCachedTeamSelectionCriteria(other.mCachedTeamSelectionCriteria),
          mTeamUEDBalanceRuleId(other.mTeamUEDBalanceRuleId),
          mTeamUEDPositionParityRuleId(other.mTeamUEDPositionParityRuleId),
          mTeamCompositionRuleId(other.mTeamCompositionRuleId)
    {
        RuleContainer::const_iterator ruleItr = other.mRuleContainer.begin();
        RuleContainer::const_iterator ruleEnd = other.mRuleContainer.end();

        for (; ruleItr != ruleEnd; ++ruleItr)
        {
            Rule* rule = *ruleItr;
            mRuleContainer.push_back(rule->clone(&matchmakingAsyncStatus));
        }
        sortRules();
    }

    MatchmakingCriteria::~MatchmakingCriteria()
    {
        //  Catch to guard against implicit destruction for criteria rules by owning objects.  Owning objects should call cleanup() explicitly.
        EA_ASSERT(mRuleContainer.empty());

        cleanup();
    }

    /*! ************************************************************************************************/
    /*!
        \brief explicitly destroys the rule objects and maps created by initialize.

        The destructor implicitly does this as well, but if it's requried to explicitly cleanup 
        MatchmakingCritieria, the owning object should invoke this method instead.
    *************************************************************************************************/
    void MatchmakingCriteria::cleanup()
    {
        // Free any custom Rules (all rules eventually will be here)
        RuleContainer::const_iterator ruleItr = mRuleContainer.begin();
        RuleContainer::const_iterator ruleEnd = mRuleContainer.end();

        for (; ruleItr != ruleEnd; ++ruleItr)
        {
            delete *ruleItr;
        }
        mRuleContainer.clear();
    }

    /*! ************************************************************************************************/
    /*!
        \brief initialize a MatchmakingCriteria - get it ready for evaluation (as part of starting a matchmaking session).

        Given a MatchmakingCriteriaData, lookup and initialize each rule from the matchmaker;
        You cannot evaluate a criteria until it's been successfully initialized.

        \param[in]  criteriaData - the set of rules defining the criteria (from StartMatchmakingSession).
        \param[in]  matchmakingSupplementalData - used to lookup the rule definitions
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool MatchmakingCriteria::initialize(const MatchmakingCriteriaData &criteriaData,
                                         const MatchmakingSupplementalData &matchmakingSupplementalData,
                                         MatchmakingCriteriaError &err)
    {
        eastl::string buf;
        if (! MatchmakingCriteria::validateEnabledGameSizeRulesForRequest(criteriaData, matchmakingSupplementalData, buf))
        {
            ERR_LOG("[MatchmakingCriteria].initialize failed validation: " << buf.c_str());
            err.setErrMessage(buf.c_str());
            return false;
        }
        if (! MatchmakingCriteria::validateEnabledTeamSizeRulesForRequest(criteriaData, matchmakingSupplementalData, buf))
        {
            ERR_LOG("[MatchmakingCriteria].initialize failed validation: " << buf.c_str());
            err.setErrMessage(buf.c_str());
            return false;
        }

        if (!initRules(criteriaData, matchmakingSupplementalData, err))
        {
            return false;
        }

        //criteria can use one of different possible team UED rules. Cache id from name if available.
        const TeamUEDBalanceRuleDefinition* teamUEDBalanceRuleDef = mRuleDefinitionCollection.getRuleDefinition<TeamUEDBalanceRuleDefinition>(
            criteriaData.getTeamUEDBalanceRulePrefs().getRuleName());
        const TeamUEDPositionParityRuleDefinition* teamUEDPostionParityRuleDef = mRuleDefinitionCollection.getRuleDefinition<TeamUEDPositionParityRuleDefinition>(
            criteriaData.getTeamUEDPositionParityRulePrefs().getRuleName());
        const TeamCompositionRuleDefinition* teamCompositionRuleDef = mRuleDefinitionCollection.getRuleDefinition<TeamCompositionRuleDefinition>(
            criteriaData.getTeamCompositionRulePrefs().getRuleName());
        mTeamUEDBalanceRuleId = teamUEDBalanceRuleDef ? teamUEDBalanceRuleDef->getID() : RuleDefinition::INVALID_DEFINITION_ID;
        mTeamUEDPositionParityRuleId = teamUEDPostionParityRuleDef ? teamUEDPostionParityRuleDef->getID() : RuleDefinition::INVALID_DEFINITION_ID;
        mTeamCompositionRuleId = teamCompositionRuleDef ? teamCompositionRuleDef->getID() : RuleDefinition::INVALID_DEFINITION_ID;

        updateCachedThresholds(0, true);

        // TEMPORARY required until all of the rules are converted.
        initReteRules();

        //keep cached team selection criteria for team join flows
        cacheTeamSelectionCriteria((uint16_t)matchmakingSupplementalData.mJoiningPlayerCount);

        return true; // no error
    }

    void MatchmakingCriteria::initReteRules()
    {
        for (RuleContainer::iterator itr = mRuleContainer.begin(), end = mRuleContainer.end();
            itr != end; ++itr)
        {
            if ((*itr)->isReteCapable())
            {
                mReteRuleContainer.push_back(*itr);
                // subset of rete rules that need their fit score calculated outside of RETE.
                if ((*itr)->calculateFitScoreAfterRete())
                {
                    mPostReteFitScoreRuleContainer.push_back(*itr);
                }
            }
            // non-rete rules, and rules that evaluate both non-rete and rete.
            if ((*itr)->callEvaluateForMMFindGameAfterRete())
            {
                mReteNoncapableRuleContainer.push_back(*itr);
            }
        }
    }
    
    void MatchmakingCriteria::sortRules()
    {
        // since the array is filled in once we can sort it to improve search time
        eastl::sort(mRuleContainer.begin(), mRuleContainer.end(), RuleIdComparator());
        uint32_t idx = 0;
        for (RuleContainer::iterator i = mRuleContainer.begin(), e = mRuleContainer.end(); i != e; ++i, ++idx)
        {
            (*i)->setRuleIndex(idx);
        }
    }

    void MatchmakingCriteria::setMatchmakingSession(const MatchmakingSession* matchmakingSession)
    {
        mMatchmakingSession = matchmakingSession;
    }

    bool MatchmakingCriteria::initRules(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        mMaxPossibleFitScore = 0;
        mMaxPossibleNonReteFitScore = 0;
        mMaxPossiblePostReteFitScore = 0;
        RuleContainer::const_iterator itr = mRuleContainer.begin();
        RuleContainer::const_iterator end = mRuleContainer.end();

        bool result = true;
        // Set the matchmaking session for all of the rules.
        for (; itr != end; ++itr)
        {
            Rule* rule = *itr;
            if (!rule->initialize(criteriaData, matchmakingSupplementalData, err))
            {
                result = false;
                break;
            }
            if (!rule->isDisabled())
                mRuleDefinitionCollection.incRuleDefinitionTotalUseCount(rule->mRuleDefinition.getName());

            FitScore ruleFitScore = rule->getMaxFitScore();

            mMaxPossibleFitScore += ruleFitScore;
            if (rule->calculateFitScoreAfterRete())
            {
                mMaxPossiblePostReteFitScore += ruleFitScore;
            }
            if (rule->callEvaluateForMMFindGameAfterRete())
            {
                mMaxPossibleNonReteFitScore += ruleFitScore;
            }
            TRACE_LOG("[MatchmakingCriteria] Rule '" << rule->getRuleName() << "' max fitscore '" << ruleFitScore << "'");
        }

        if (EA_UNLIKELY(!result))
        {
            // NOTE: In the unlikely event of a failed rule initialization
            // we need to tick down the total counts of rule references
            end = itr;
            itr = mRuleContainer.begin();
            for (; itr != end; ++itr)
            {
                Rule* rule = *itr;
                if (!rule->isDisabled())
                    mRuleDefinitionCollection.decRuleDefinitionTotalUseCount(rule->mRuleDefinition.getName());
            }
        }

        return result;
    }

    const Rule* MatchmakingCriteria::getRuleById(RuleDefinitionId ruleDefinitionId) const
    {
        const Rule* rule = nullptr;
        if (ruleDefinitionId != RuleDefinition::INVALID_DEFINITION_ID)
        {
            RuleContainer::const_iterator it = 
                eastl::binary_search_i(mRuleContainer.begin(), mRuleContainer.end(), ruleDefinitionId, RuleIdComparator());
            if (it != mRuleContainer.end())
            {
                rule = *it;
            }
        }
        return rule;
    }
    
    const Rule* MatchmakingCriteria::getRuleByIndex(uint32_t index) const
    {
        const Rule* rule = nullptr;
        if (index < mRuleContainer.size())
        {
            rule = mRuleContainer[index];
        }
        return rule;
    }

    const TeamIdRoleSizeMap* MatchmakingCriteria::getTeamIdRoleSpaceRequirementsFromRule() const
    {
        if (!isRuleEnabled<RolesRuleDefinition>())
        {
            // This really shouldn't ever happen
            return nullptr;
        }
        else
            return &getRolesRule()->getTeamIdRoleSpaceRequirements();
    }

    bool MatchmakingCriteria::getDuplicateTeamsAllowed() const 
    { 
        // TeamChoiceRule should be always enabled
        if (!isRuleEnabled<TeamChoiceRuleDefinition>())
            return false;
        else
            return getTeamChoiceRule()->getDuplicateTeamsAllowed(); 
    }
    uint16_t MatchmakingCriteria::getUnspecifiedTeamCount() const 
    { 
        if (!isRuleEnabled<TeamChoiceRuleDefinition>())
            return 0;
        else
            return getTeamChoiceRule()->getUnspecifiedTeamCount(); 
    }
    const TeamIdVector* MatchmakingCriteria::getTeamIds() const 
    { 
        if (!isRuleEnabled<TeamChoiceRuleDefinition>())
        {
            // This really shouldn't ever happen
            return nullptr;
        }
        else
            return &getTeamChoiceRule()->getTeamIds(); 
    }


    const TeamIdSizeList* MatchmakingCriteria::getTeamIdSizeListFromRule() const 
    { 
        if (!isRuleEnabled<TeamChoiceRuleDefinition>())
            return nullptr;                 
        else
            return &getTeamChoiceRule()->getTeamIdSizeList();
    }

    uint16_t MatchmakingCriteria::getTeamCountFromRule() const 
    { 
        if (!isRuleEnabled<TeamCountRuleDefinition>())
        {
            const TeamIdVector* teamIds = getTeamIds();
            if (teamIds != nullptr)
                return teamIds->size();

            const TeamIdSizeList* teamIdList = getTeamIdSizeListFromRule();
            if (teamIdList != nullptr)
                return teamIdList->size();

            return 1;                           // default team size is 1
        }
        else
            return getTeamCountRule()->getTeamCount();
    }
        
    // This varies, so we can't just get the value directly, we have to use the interpolated one.
    uint16_t MatchmakingCriteria::getAcceptableTeamMinSize() const 
    { 
        if (!isRuleEnabled<TeamMinSizeRuleDefinition>())
            return 0;                                   // Accept any min size.
        else
            return getTeamMinSizeRule()->getAcceptableTeamMinSize();
    }
    uint16_t MatchmakingCriteria::getAcceptableTeamBalance() const
    { 
        if (!isRuleEnabled<TeamBalanceRuleDefinition>())
            return getTeamCapacity();                           // Accept a game that's got a max sized team, and an empty team.
        else
            return getTeamBalanceRule()->getAcceptableTeamBalance();
    }

    // pre: all of sessions in this finalization are using the same rule, UED, and team UED formula to calc and match
    UserExtendedDataValue MatchmakingCriteria::getAcceptableTeamUEDImbalance() const
    { 
        const TeamUEDBalanceRule* rule = getTeamUEDBalanceRule();
        if ((rule == nullptr) || rule->isDisabled())
            return INT64_MAX;                                  // Accept a game that's got a max 'skilled' team, and an empty team.
        else
            return rule->getAcceptableTeamUEDImbalance();
    }
    UserExtendedDataValue MatchmakingCriteria::getTeamUEDContributionValue() const
    { 
        const TeamUEDBalanceRule* rule = getTeamUEDBalanceRule();
        UserExtendedDataValue uedValue = ((rule != nullptr)? rule->getUEDValue() : INVALID_USER_EXTENDED_DATA_VALUE);
        return ((uedValue == INVALID_USER_EXTENDED_DATA_VALUE)? 0 : uedValue); // rule may have been disabled or UED was missing. Count as adding 0.
    }

    uint16_t MatchmakingCriteria::getParticipantCapacity() const 
    {
        if (isTotalSlotsRuleEnabled())
        {
            return getTotalPlayerSlotsRule()->calcCreateGameParticipantCapacity();
        }
        else if (hasTeamCompositionRuleEnabled())
        {
            return (uint16_t)(getTeamCompositionRule()->getTeamCapacityForRule() * getTeamCountFromRule());
        }
        else if(mRuleDefinitionCollection.isRuleDefinitionInUse<PlayerCountRuleDefinition>())
        {
            // I would like to just grab the server config directly here, unfortunately, the config can reside on one of 3 objects (search slave, matchmaking slave, game manager), depending on how this function is reached.
            // So, just grab the max value from a definition that stored it (using player count rule because that should always be available)
            return mRuleDefinitionCollection.getRuleDefinition<PlayerCountRuleDefinition>()->getGlobalMaxTotalPlayerSlotsInGame();    
        }
        else
        {
            ERR_LOG("[MatchmakingCriteria].getParticipantCapacity: Attempting to get capacity when TeamCompositionRule and TotalSlotRule are disabled, and no PlayerCountRuleDefinition exists.");
            return 0;
        }
    }

    uint16_t MatchmakingCriteria::getMaxPossibleParticipantCapacity() const 
    {
        if (isTotalSlotsRuleEnabled())
        {
            return getTotalPlayerSlotsRule()->getMaxTotalParticipantSlots();
        }
        else if (hasTeamCompositionRuleEnabled())
        {
            return (uint16_t)(getTeamCompositionRule()->getTeamCapacityForRule() * getTeamCountFromRule());
        }
        else if(mRuleDefinitionCollection.isRuleDefinitionInUse<PlayerCountRuleDefinition>())
        {
            // I would like to just grab the server config directly here, unfortunately, the config can reside on one of 3 objects (search slave, matchmaking slave, game manager), depending on how this function is reached.
            // So, just grab the max value from a definition that stored it (using player count rule because that should always be available)
            return mRuleDefinitionCollection.getRuleDefinition<PlayerCountRuleDefinition>()->getGlobalMaxTotalPlayerSlotsInGame();    
        }
        else
        {
            ERR_LOG("[MatchmakingCriteria].getMaxPossibleParticipantCapacity: Attempting to get capacity when TeamCompositionRule and TotalSlotRule are disabled, and no PlayerCountRuleDefinition exists.");
            return 0;
        }
    }

    uint16_t MatchmakingCriteria::getTeamCapacity() const 
    {
        // Team count should always be 1 or greater
        uint16_t teamCount = getTeamCountFromRule();
        uint16_t playerCapacity = getParticipantCapacity();
        return playerCapacity / teamCount;
    }

    uint16_t MatchmakingCriteria::getMaxPossibleTeamCapacity() const
    {
        // Team count should always be 1 or greater
        uint16_t teamCount = getTeamCountFromRule();
        uint16_t playerCapacity = getMaxPossibleParticipantCapacity();
        return playerCapacity / teamCount;
    }

    /*! ************************************************************************************************/
    /*!
        \brief update each rule's cached fit/count Thresholds to the given time.

            Instead of scanning through the minFitThresholdList every time we evaluate a rule, we can cache the value
            and do one lookup per session idle (since elapsed seconds doesn't change during a session's idle loop).

        \param[in]  elapsedSeconds - the 'age' of the matchmaking session
    *************************************************************************************************/
    UpdateThresholdStatus MatchmakingCriteria::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate /* = false */)
    {
        UpdateThresholdStatus overallResult;
        overallResult.nonReteRulesResult = NO_RULE_CHANGE;
        overallResult.reteRulesResult = NO_RULE_CHANGE;

        mNextUpdate = UINT32_MAX;

        // Update the cached thresholds for rules
        RuleContainer::const_iterator ruleItr = mRuleContainer.begin();
        RuleContainer::const_iterator ruleEnd = mRuleContainer.end();

        for (; ruleItr != ruleEnd; ++ruleItr)
        {
            Rule* rule = *ruleItr;

            uint32_t nextElapsedSecondsUpdate = rule->getNextElapsedSecondsUpdate();
            if (nextElapsedSecondsUpdate <= elapsedSeconds || forceUpdate)
            {
                UpdateThresholdResult ruleResult = rule->updateCachedThresholds(elapsedSeconds, forceUpdate);
                if (rule->isReteCapable())
                {
                    if (ruleResult > overallResult.reteRulesResult)
                    {
                        overallResult.reteRulesResult = ruleResult;
                    }
                }
                if (rule->callEvaluateForMMFindGameAfterRete())
                {
                    if (ruleResult > overallResult.nonReteRulesResult)
                    {
                        overallResult.nonReteRulesResult = ruleResult;
                    }
                }
            }

            if (rule->getNextElapsedSecondsUpdate() < mNextUpdate)
            {
                mNextUpdate = rule->getNextElapsedSecondsUpdate();
            }
        }

        return overallResult;
    }


    FitScore MatchmakingCriteria::evaluateGameReteFitScore(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const
    {
        return evaluateGameInternal(mPostReteFitScoreRuleContainer, game, debugResultMap, useDebug);
    }

    FitScore MatchmakingCriteria::evaluateGame(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const
    {
        return evaluateGameInternal(mReteNoncapableRuleContainer, game, debugResultMap, useDebug);
    }

    FitScore MatchmakingCriteria::evaluateGameAllRules(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const
    {
        return evaluateGameInternal(mRuleContainer, game, debugResultMap, useDebug);
    }

    /*! ************************************************************************************************/
    /*!
        \brief evaluate the criteria against a Game(either a found game or new created game by a session.
            If the game is a 'match', we return the total weighted fit score (>= 0); otherwise we return a
            negative number.

        If this criteria doesn't match the supplied game, we return a log-able error message via
        matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the
        MatchFailureMessage object.

        \param[in]  game                The game to evaluate the criteria against.
        \param[out] debugResultMap     Debugging information about rule evaluations.
        

        \return the total weighted fitScore for this rule.  The game is not a match if the fitScore is negative.
    *************************************************************************************************/
    FitScore MatchmakingCriteria::evaluateGameInternal(const RuleContainer& ruleContainer, const Search::GameSessionSearchSlave &game,
                                            DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const
    {
        // eval predefined rules
        FitScore totalFitScore = 0;

        // evaluate rules for game
        if (!mRuleContainer.empty())
        {
            RuleContainer::const_iterator ruleItr = ruleContainer.begin();
            RuleContainer::const_iterator ruleEnd = ruleContainer.end();

            for (; ruleItr != ruleEnd; ++ruleItr)
            {
                Rule* rule = *ruleItr;
                if (rule->isDisabled() || (!rule->isDedicatedServerEnabled() && game.isResetable()))
                    continue;
                
                ReadableRuleConditions debugRuleConditions;

                if (useDebug)
                {
                    debugRuleConditions.setIsDebugSession();
                }

                FitScore ruleFitScore = rule->evaluateForMMFindGame(game, debugRuleConditions);

                if (useDebug)
                {
                    DebugRuleResultMap::iterator iter = debugResultMap.find(rule->getRuleName());
                    if (iter != debugResultMap.end())
                    {
                        // add our conditions and fitScore to an existing result for the rule.
                        // Can happen when rete rules specify multiple and conditions, or a rule uses rete and non rete together.
                        DebugRuleResult *existingResult = iter->second;
                        if (!isFitScoreMatch(ruleFitScore))
                        {
                            existingResult->setFitScore(0);
                        }
                        else
                        {
                            existingResult->setFitScore(existingResult->getFitScore() + ruleFitScore);
                        }

                        existingResult->setResult( ((existingResult->getResult() == MATCH) && isFitScoreMatch(ruleFitScore)) ? MATCH : NO_MATCH );
                        existingResult->getConditions().insert(existingResult->getConditions().end(), debugRuleConditions.getRuleConditions().begin(), debugRuleConditions.getRuleConditions().end());
                    }
                    else
                    {
                        DebugRuleResult debugResult;
                        // mask our uint32_t max "no match" fit score here with 0.
                        if (ruleFitScore == FIT_SCORE_NO_MATCH)
                        {
                            debugResult.setFitScore(0);
                        }
                        else
                        {
                            debugResult.setFitScore(ruleFitScore);
                        }
                        
                        debugResult.setMaxFitScore(rule->getMaxFitScore());
                        debugResult.setResult( isFitScoreMatch(ruleFitScore) ? MATCH : NO_MATCH );
                        debugRuleConditions.getRuleConditions().copyInto(debugResult.getConditions());
                        debugResultMap.insert(eastl::make_pair(rule->getRuleName(), debugResult.clone()));
                    }
                }

                if (!isFitScoreMatch(ruleFitScore))
                {
                    TRACE_LOG("[MatchmakingEvaluation] Game '" << game.getGameId() << "' Type 'PostRETE' Rule '" << rule->getRuleName()
                        << "' NO_MATCH fitScore '" << ruleFitScore << "'/'" << rule->getMaxFitScore() << "'.");
                    ReadableRuleConditionList::const_iterator iter = debugRuleConditions.getRuleConditions().begin();
                    for (; iter != debugRuleConditions.getRuleConditions().end(); ++iter)
                    {
                        TRACE_LOG("[MatchmakingEvaluation] Rule evaluated '" << *iter << "'.");
                    }
                    return FIT_SCORE_NO_MATCH; // failed to pass custom rules
                }
                TRACE_LOG("[MatchmakingEvaluation] Game '" << game.getGameId() << "' Type 'PostRETE' Rule '" << rule->getRuleName()
                    << "'  fitScore '" << ruleFitScore << "'");
                totalFitScore += ruleFitScore;
            }
        }

        return totalFitScore;
    }

    /*! ************************************************************************************************/
    /*!
        \brief evaluate the criteria against another matchmaking session (for CreateGame).  If the game is a 'match',
        we return the total weighted fit score (>= 0); otherwise we return a negative number.

        If this criteria doesn't match the supplied game, we return a log-able error message via
        matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the
        MatchFailureMessage object.

        \param[in]  session The session to evaluate the criteria against.
        \param[in/out] aggregateSessionMatchInfo aggregateMatchInfo for all of the rules.
    *************************************************************************************************/
    void MatchmakingCriteria::evaluateCreateGame(MatchmakingSession &mySession, MatchmakingSession &otherSession, 
            AggregateSessionMatchInfo& aggregateSessionMatchInfo) const
    {
 
        SessionsEvalInfo sessionsEvalInfo(mySession, otherSession);

        bool debugOutput = (mySession.getDebugCheckOnly() || IS_LOGGING_ENABLED(Logging::TRACE));
       

        // evaluate rules for session
        RuleContainer::const_iterator ruleItr = mRuleContainer.begin();
        RuleContainer::const_iterator ruleEnd = mRuleContainer.end();

        for (; ruleItr != ruleEnd; ++ruleItr)
        {
            Rule* rule = *ruleItr;

            SessionsMatchInfo ruleMatchInfo;
            ruleMatchInfo.sMyMatchInfo.setMatch(0, 0);
            ruleMatchInfo.sOtherMatchInfo.setMatch(0, 0);
            if (debugOutput)
            {
                ruleMatchInfo.sMyMatchInfo.sDebugRuleConditions.setIsDebugSession();
                ruleMatchInfo.sOtherMatchInfo.sDebugRuleConditions.setIsDebugSession();
            }

            rule->evaluateForMMCreateGame(sessionsEvalInfo, otherSession, ruleMatchInfo);

            // update diagnostics
            if (mySession.getMatchmaker().getMatchmakingConfig().getServerConfig().getTrackDiagnostics())
            {
                rule->tallyRuleDiagnosticsCreateGame(mySession.getDiagnostics(), &ruleMatchInfo.sMyMatchInfo);
                const Rule* otherRule = (otherSession.getCriteria().getRuleByIndex(rule->getRuleIndex()));
                if (otherRule != nullptr)
                {
                    otherRule->tallyRuleDiagnosticsCreateGame(otherSession.getDiagnostics(), &ruleMatchInfo.sOtherMatchInfo);
                }
            }

            TRACE_LOG("[MatchmakingCriteria] " << (rule->isDisabled()?"Disabled ":"") << "Rule '" << rule->getRuleName() << "' my session " 
                << mySession.getMMSessionId() << " evaluated other session " << otherSession.getMMSessionId() << " result " 
                << ((ruleMatchInfo.sMyMatchInfo.sFitScore == FIT_SCORE_NO_MATCH) ? "NO_MATCH" : "MATCH") << " (" << ruleMatchInfo.sMyMatchInfo.sFitScore 
                << "/" << rule->getMaxFitScore() << " fit in " << ruleMatchInfo.sMyMatchInfo.sMatchTimeSeconds << " sec)");
            
            if ( !rule->isDisabled() && debugOutput )
            {
                DebugRuleResult debugResult;
                ruleMatchInfo.sMyMatchInfo.sDebugRuleConditions.getRuleConditions().copyInto(debugResult.getConditions());
                debugResult.setFitScore(ruleMatchInfo.sMyMatchInfo.sFitScore);
                debugResult.setMaxFitScore(rule->getMaxFitScore());
                debugResult.setResult( ((ruleMatchInfo.sMyMatchInfo.sFitScore == FIT_SCORE_NO_MATCH) ? NO_MATCH : MATCH) );
                aggregateSessionMatchInfo.sRuleResultMap.insert(eastl::make_pair(rule->getRuleName(), debugResult.clone()));

                // Evaluations of my session to other session
                ReadableRuleConditionList::const_iterator condIter = ruleMatchInfo.sMyMatchInfo.sDebugRuleConditions.getRuleConditions().begin();
                for (; condIter != ruleMatchInfo.sMyMatchInfo.sDebugRuleConditions.getRuleConditions().end(); ++condIter)
                {
                    TRACE_LOG("[MatchmakingCriteria] Rule evaluation '" << *condIter << "'.");
                }
            }

            TRACE_LOG("[MatchmakingCriteria] " << (rule->isDisabled()?"Disabled ":"") << "Rule '" << rule->getRuleName() << "' other session " 
                << otherSession.getMMSessionId() << " evaluated my session " << mySession.getMMSessionId() << " result " 
                << ((ruleMatchInfo.sOtherMatchInfo.sFitScore == FIT_SCORE_NO_MATCH) ? "NO_MATCH" : "MATCH") << " (" << ruleMatchInfo.sOtherMatchInfo.sFitScore 
                << "/" << rule->getMaxFitScore() << " fit in " << ruleMatchInfo.sOtherMatchInfo.sMatchTimeSeconds << " sec)");

            if ( !rule->isDisabled() && debugOutput )
            {
                // Evaluations of other session to my session
                ReadableRuleConditionList::const_iterator condIter = ruleMatchInfo.sOtherMatchInfo.sDebugRuleConditions.getRuleConditions().begin();
                for (; condIter != ruleMatchInfo.sOtherMatchInfo.sDebugRuleConditions.getRuleConditions().end(); ++condIter)
                {
                    TRACE_LOG("[MatchmakingCriteria] Rule evaluation '" << *condIter << "'.");
                }
            }

            if (!aggregateSessionMatchInfo.aggregate(ruleMatchInfo))
            {
               return; // match failure
            }
        }

    }

    /*! ************************************************************************************************/
        /*!
            \brief evaluate the criteria against a finalized create game request

            If this criteria doesn't match the supplied create game request, we return a log-able error message via
            matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the 
            MatchFailureMessage object.

            \param[in] request - The create requests to evaluate the criteria against.
            \param[out] ruleResults - the debug output from matching
            \param[in] useDebug - tells the criteria if the session needs debug output or not.
            \return the total weighted fitScore for this criteria.  The game is not a match if the fitScore is negative.
        *************************************************************************************************/
    FitScore MatchmakingCriteria::evaluateCreateGameRequest(const MatchmakingCreateGameRequest request,  DebugRuleResultMap& ruleResults, bool useDebug /*=false*/) const
    {
        // eval predefined rules
        FitScore totalFitScore = 0;

        // evaluate rules for game
        if (!mRuleContainer.empty())
        {
            RuleContainer::const_iterator ruleItr = mRuleContainer.begin();
            RuleContainer::const_iterator ruleEnd = mRuleContainer.end();

            ReadableRuleConditions debugRuleConditions;

            if (useDebug)
            {
                debugRuleConditions.setIsDebugSession();
            }

            for (; ruleItr != ruleEnd; ++ruleItr)
            {
                Rule* rule = *ruleItr;
                FitScore ruleFitScore = rule->evaluateForMMCreateGameFinalization(request, debugRuleConditions);

                TRACE_LOG("[MatchmakingCriteria] Session " << getMatchmakingSession()->getMMSessionId() << "  Rule '" << rule->getRuleName() << "' evaluated create game request with fitscore '" << ruleFitScore << "'.");

                if (useDebug)
                {
                    DebugRuleResult debugResult;
                    // mask our uint32_t max "no match" fit score here with 0.
                    if (ruleFitScore == FIT_SCORE_NO_MATCH)
                    {
                        debugResult.setFitScore(0);
                    }
                    else
                    {
                        debugResult.setFitScore(ruleFitScore);
                    }

                    debugResult.setMaxFitScore(rule->getMaxFitScore());
                    debugResult.setResult( isFitScoreMatch(ruleFitScore) ? MATCH : NO_MATCH );
                    debugRuleConditions.getRuleConditions().copyInto(debugResult.getConditions());
                    ruleResults.insert(eastl::make_pair(rule->getRuleName(), debugResult.clone()));

                    if ( !rule->isDisabled() )
                    {
                        // Evaluations of other session to my session
                        ReadableRuleConditionList::const_iterator condIter = debugRuleConditions.getRuleConditions().begin();
                        for (; condIter != debugRuleConditions.getRuleConditions().end(); ++condIter)
                        {
                            TRACE_LOG("[MatchmakingCriteria] Rule evaluation '" << *condIter << "'.");
                        }
                    }
                }

                if (!isFitScoreMatch(ruleFitScore))
                {
                    WARN_LOG("[MatchmakingCriteria].evaluateCreateGameRequest: Rule '" << rule->getRuleName() 
                        << "' evaluated create game request with fitscore '" << ruleFitScore  
                        << "' rejecting the request.");
                    return FIT_SCORE_NO_MATCH; // no match
                }
                totalFitScore += ruleFitScore;
            }
        }

        return totalFitScore;

    }

    void MatchmakingCriteria::setRuleEvaluationMode(RuleEvaluationMode ruleMode)
    {
        // set rule evaluation mode for custom rules
        RuleContainer::const_iterator ruleItr = mRuleContainer.begin();
        RuleContainer::const_iterator ruleEnd = mRuleContainer.end();

        for (; ruleItr != ruleEnd; ++ruleItr)
        {
            Rule* rule = *ruleItr;
            rule->setRuleEvaluationMode(ruleMode);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief vote on various game values (settings/attributes), and set them in the supplied createGameRequest.

        \param[in] votingSessionList list of all the matchmaking sessions that will be voting.
        \param[in,out] createGameRequest the various voted values are set in the createGameRequest object.
    ***************************************************************************************************/
    void MatchmakingCriteria::voteOnGameValues(const MatchmakingSessionList& votingSessionList, CreateGameRequest &createGameRequest) const
    {
        RuleContainer::const_iterator i = mRuleContainer.begin();
        RuleContainer::const_iterator e = mRuleContainer.end();
        for (; i != e; ++i)
        {
            if ((*i)->isDisabled())
            {
                continue;
            }
            (*i)->voteOnGameValues(votingSessionList, createGameRequest);
        }
    }

    /*! ************************************************************************************************
        \brief return the max game total player slots the session will accept, based on all request criteria.
    ************************************************************************************************/
    uint16_t MatchmakingCriteria::calcMaxPossPlayerSlotsFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
        uint16_t globalMaxBound) const
    {
        // find enabled rule to return the value from.
        // Pre: below order unimportant/ok, as conflicts in rule values already checked.
        const TotalPlayerSlotsRuleCriteria& totalSlotRule = criteriaData.getTotalPlayerSlotsRuleCriteria();
        if (totalSlotRule.getRangeOffsetListName()[0] != '\0')
        {
            return totalSlotRule.getMaxTotalPlayerSlots();
        }

        const TeamCompositionRulePrefs& teamCompositionRulePrefs = criteriaData.getTeamCompositionRulePrefs();
        if (teamCompositionRulePrefs.getMinFitThresholdName()[0] != '\0')
        {
            const TeamCompositionRuleDefinition* tcRuleDefn = mRuleDefinitionCollection.getRuleDefinition<TeamCompositionRuleDefinition>(teamCompositionRulePrefs.getRuleName());
            if ((tcRuleDefn != nullptr) && !tcRuleDefn->isDisabled())
            {
                return (tcRuleDefn->getTeamCapacityForRule() * calcTeamCountFromRules(criteriaData, matchmakingSupplementalData));
            }
        }
        return globalMaxBound;
    }

    /*! ************************************************************************************************
        \brief return the min game player count the session will accept, based on all request criteria.
    ************************************************************************************************/
    uint16_t MatchmakingCriteria::calcMinPossPlayerCountFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
        uint16_t globalMinBound) const
    {
        // find enabled rule to return the value from
        const PlayerCountRuleCriteria& playerCountRule = criteriaData.getPlayerCountRuleCriteria();
        if (playerCountRule.getRangeOffsetListName()[0] != '\0')
        {
            return playerCountRule.getMinPlayerCount();
        }

        return globalMinBound;
    }

    /*! ************************************************************************************************
        \brief return the team count based on all request criteria.
        \param[out] teamCountIsDefault if non null store whether the team count returned is the default
            (not from an explicit rule criteria setting).
    ************************************************************************************************/
    uint16_t MatchmakingCriteria::calcTeamCountFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, bool* teamCountIsDefault /*= nullptr*/) const
    {
        if (teamCountIsDefault != nullptr)
            *teamCountIsDefault = false;

        // find enabled rule to return the value from.
        // Pre: below order unimportant/ok, as conflicts in rule values already checked.
        const TeamCountRulePrefs& teamCountRulePrefs = criteriaData.getTeamCountRulePrefs();
        if (teamCountRulePrefs.getTeamCount() != 0)
        {
            return teamCountRulePrefs.getTeamCount();
        }
        const TeamCompositionRulePrefs& teamCompositionRulePrefs = criteriaData.getTeamCompositionRulePrefs();
        if (teamCompositionRulePrefs.getMinFitThresholdName()[0] != '\0')
        {
            const TeamCompositionRuleDefinition* tcRuleDefn = mRuleDefinitionCollection.getRuleDefinition<TeamCompositionRuleDefinition>(teamCompositionRulePrefs.getRuleName());
            if ((tcRuleDefn != nullptr) && !tcRuleDefn->isDisabled())
            {
                return tcRuleDefn->getTeamCount();
            }
        }
        if (!matchmakingSupplementalData.mTeamIds.empty())   // TeamIds used for CG MM and duplicate support
        {
            return matchmakingSupplementalData.mTeamIds.size();
        }

        // If we explicitly requested to join into different teams, then the game must have that many teams:
        // Note: This means that if you want to join 2 teams into a 3 team game you have to set the TeamCountPrefs or you won't find the game.
        //       There is no way to say 'I want to join my 2 teams into a game with 2 or more teams', like you can with 1 team.
        if (matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size() > 1)
        {
            return matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.size();
        }

        if (teamCountIsDefault != nullptr)
            *teamCountIsDefault = true;
        return 1;
    }

    /*! ************************************************************************************************/
    /*! \brief validate the set of enabled game size related rules.
        \param[in] criteriaData the matchmaking request data to validate for.
    *************************************************************************************************/
    bool MatchmakingCriteria::validateEnabledGameSizeRulesForRequest(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, eastl::string &err)
    {
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief validate the set of enabled team size related rules have no conflicts up front.
        \param[in] criteriaData the matchmaking request data to validate for.
    *************************************************************************************************/
    bool MatchmakingCriteria::validateEnabledTeamSizeRulesForRequest(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, eastl::string &err)
    {
        const TeamCountRulePrefs& teamCountRulePrefs = criteriaData.getTeamCountRulePrefs();
        const TeamCompositionRulePrefs& teamCompositionRulePrefs = criteriaData.getTeamCompositionRulePrefs();
        const TotalPlayerSlotsRuleCriteria& totalSlotRule = criteriaData.getTotalPlayerSlotsRuleCriteria();
        const TeamCompositionRuleDefinition* tcmpnRuleDefn = ((teamCompositionRulePrefs.getMinFitThresholdName()[0] == '\0')? nullptr :
            mRuleDefinitionCollection.getRuleDefinition<TeamCompositionRuleDefinition>(teamCompositionRulePrefs.getRuleName()));

        // Check for old/new team rules before the criteria is set (because we'll enable both sets of team rules at that point)
        bool teamCountRuleEnabled = (teamCountRulePrefs.getTeamCount() != 0);
        bool teamCompositionRuleEnabled = ((teamCompositionRulePrefs.getMinFitThresholdName()[0] != '\0') && (tcmpnRuleDefn != nullptr) && !tcmpnRuleDefn->isDisabled());

        // ensure team counts don't conflict.
        if (teamCompositionRuleEnabled && teamCountRuleEnabled &&
            (tcmpnRuleDefn->getTeamCount() != teamCountRulePrefs.getTeamCount()))
        {
            err.sprintf("TeamCountRule's specified team count %u conflicts with the TeamCompositionRule(%s)'s team count %u", teamCountRulePrefs.getTeamCount(), teamCompositionRulePrefs.getRuleName(), tcmpnRuleDefn->getTeamCount());
            return false;
        }

        // ensure team capacities don't conflict
        if (teamCompositionRuleEnabled && (totalSlotRule.getRangeOffsetListName()[0] != '\0'))
        {
            // (team composition vs total player slots rule)
            uint16_t initialTotalSlots = UINT16_MAX, finalTotalSlots = UINT16_MAX;
            if (TotalPlayerSlotsRule::calcParticipantCapacityAtInitialAndFinalTimes(totalSlotRule, mRuleDefinitionCollection, initialTotalSlots, finalTotalSlots))
            {
                const uint16_t teamCount = calcTeamCountFromRules(criteriaData, matchmakingSupplementalData);// note: can get actual team count now that we know we have a valid value from above team count validation.
                const uint16_t teamCapacityFromTotalSlotsRule = ((teamCount == 0)? 0 : initialTotalSlots / teamCount);
                if (tcmpnRuleDefn->getTeamCapacityForRule() != teamCapacityFromTotalSlotsRule)
                {
                    err.sprintf("TotalPlayerSlotsRule's overall capacity %u for %u teams gives team capacity of %u conflicts with TeamCompositionRule(%s)'s team capacity %u", initialTotalSlots, teamCount, teamCapacityFromTotalSlotsRule, teamCompositionRulePrefs.getRuleName(), tcmpnRuleDefn->getTeamCapacityForRule());
                    return false;
                }
                if (initialTotalSlots != finalTotalSlots)
                {
                    WARN_LOG("[MatchmakingCriteria].validateEnabledTeamSizeRulesForRequest: TotalPlayerSlotsRule's range offset list " << totalSlotRule.getRangeOffsetListName() << " relaxes to eventually match on a team capacity, e.g. final decay's is " << ((teamCount == 0)? 0 : finalTotalSlots / teamCount) << ", which is not equal to TeamCompositionRule(" << teamCompositionRulePrefs.getRuleName() << ")'s required team capacity of " << tcmpnRuleDefn->getTeamCapacityForRule() << ". Client request settings may need to be corrected, as this may prevent matches after decay.");
                }
            }
        }

        // side: player count and player slot utilization rules check things in their initialize.
        return true;
    }

    uint16_t MatchmakingCriteria::getMaxFinalizationPickSequenceRetries() const 
    { 
        uint16_t teamUEDBalanceRuleRetries = (hasTeamUEDBalanceRuleEnabled()? getTeamUEDBalanceRule()->getDefinition().getMaxFinalizationPickSequenceRetries() : 0);
        uint16_t teamUEDPositionParityRuleRetries = (hasTeamUEDPositionParityRuleEnabled()? getTeamUEDPositionParityRule()->getDefinition().getMaxFinalizationPickSequenceRetries() : 0);
        return teamUEDBalanceRuleRetries + teamUEDPositionParityRuleRetries; 
    }

    /*! \brief cache off values used to select the team to join (used by FG). Pre: rules initialized */
    void MatchmakingCriteria::cacheTeamSelectionCriteria(uint16_t joiningPlayerCount)
    {
        mCachedTeamSelectionCriteria.setRequiredSlotCount(joiningPlayerCount);
        mCachedTeamSelectionCriteria.setTeamUEDFormula(getTeamUEDFormulaFromRule());
        mCachedTeamSelectionCriteria.setTeamUEDKey(getTeamUEDKeyFromRule());
        mCachedTeamSelectionCriteria.setTeamUEDJoiningValue(getTeamUEDContributionValue());
        // note team composition rule criteria is updated separately via the rule

        // Add all criteria in default order.  WE should look at how prioritizing these in a configurable fashion at some point.
        const EA::TDF::TdfEnumMap& enumMap = GetTeamSelectionCriteriaPriorityEnumMap();
        for (int32_t i = 0; enumMap.exists(i); ++i)
        {
            mCachedTeamSelectionCriteria.getCriteriaPriorityList().push_back((TeamSelectionCriteriaPriority)i);
        }
    }


    const TeamUEDBalanceRule* MatchmakingCriteria::getTeamUEDBalanceRule() const { return getRule<TeamUEDBalanceRuleDefinition>(mTeamUEDBalanceRuleId); }

    UserExtendedDataKey MatchmakingCriteria::getTeamUEDKeyFromRule() const 
    {
        const TeamUEDBalanceRule* teamUedRule = getTeamUEDBalanceRule();
        return ((teamUedRule != nullptr && !teamUedRule->isDisabled())? teamUedRule->getDefinition().getUEDKey() : INVALID_USER_EXTENDED_DATA_KEY);
    }

    GroupValueFormula MatchmakingCriteria::getTeamUEDFormulaFromRule() const
    {
        const TeamUEDBalanceRule* teamUedRule = getTeamUEDBalanceRule();//if disabled return arbitrary
        return ((teamUedRule != nullptr && !teamUedRule->isDisabled())? teamUedRule->getDefinition().getTeamValueFormula() : GROUP_VALUE_FORMULA_SUM);
    }

    UserExtendedDataValue MatchmakingCriteria::getTeamUEDMinValueFromRule() const
    {
        const TeamUEDBalanceRule* teamUedRule = getTeamUEDBalanceRule();
        return ((teamUedRule != nullptr && !teamUedRule->isDisabled())? teamUedRule->getDefinition().getMinRange() : 0);
    }

    /*! \param[in] id if specified (not INVALID_DEFINITION_ID) checks whether this *specific* TeamUEDBalanceRule is enabled */
    bool MatchmakingCriteria::hasTeamUEDBalanceRuleEnabled(RuleDefinitionId id /*= RuleDefinition::INVALID_DEFINITION_ID*/) const 
    {
        const TeamUEDBalanceRule* rule = ((mTeamUEDBalanceRuleId != RuleDefinition::INVALID_DEFINITION_ID)? getTeamUEDBalanceRule() : nullptr);
        if ((rule == nullptr) || rule->isDisabled())
            return false;
        return ((id == RuleDefinition::INVALID_DEFINITION_ID)? true : (rule->getId() == id));
    }

    bool MatchmakingCriteria::hasTeamUEDPositionParityRuleEnabled(RuleDefinitionId id /*= RuleDefinition::INVALID_DEFINITION_ID*/) const
    {
        const TeamUEDPositionParityRule* rule = ((mTeamUEDPositionParityRuleId != RuleDefinition::INVALID_DEFINITION_ID)? getTeamUEDPositionParityRule() : nullptr);
        if ((rule == nullptr) || rule->isDisabled())
            return false;
        return ((id == RuleDefinition::INVALID_DEFINITION_ID)? true : (rule->getId() == id));
    }

    const TeamUEDPositionParityRule* MatchmakingCriteria::getTeamUEDPositionParityRule() const
    {
        return getRule<TeamUEDPositionParityRuleDefinition>(mTeamUEDPositionParityRuleId);
    }

    UserExtendedDataKey MatchmakingCriteria::getPositionParityUEDKeyFromRule() const
    {
        const TeamUEDPositionParityRule* teamUedRule = getTeamUEDPositionParityRule();
        if ((teamUedRule != nullptr && !teamUedRule->isDisabled()))
        {
            return teamUedRule->getDefinition().getUEDKey();
        }
        else 
        {
            return INVALID_USER_EXTENDED_DATA_KEY;
        }
    }


    uint16_t MatchmakingCriteria::getMaxFinalizationGameTeamCompositionsAttempted() const
    {
        return (hasTeamCompositionRuleEnabled()? getTeamCompositionRule()->getDefinition().getMaxFinalizationGameTeamCompositionsAttempted() : 1);
    }

    const GameTeamCompositionsInfoVector* MatchmakingCriteria::getAcceptableGameTeamCompositionsInfoVectorFromRule() const
    {
        const TeamCompositionRule* teamCompRule = getTeamCompositionRule();
        if ((teamCompRule != nullptr) && !teamCompRule->isDisabled())
        {
            return &teamCompRule->getAcceptableGameTeamCompositionsInfoVector();
        }
        return nullptr;
    }

    /*! \param[in] id if specified (not INVALID_DEFINITION_ID) checks whether this *specific* TeamCompositionRule is enabled */
    bool MatchmakingCriteria::hasTeamCompositionRuleEnabled(RuleDefinitionId id /*= RuleDefinition::INVALID_DEFINITION_ID*/) const
    {
        const TeamCompositionRule* rule = ((mTeamCompositionRuleId != RuleDefinition::INVALID_DEFINITION_ID)? getTeamCompositionRule() : nullptr);
        if ((rule == nullptr) || rule->isDisabled())
            return false;
        return ((id == RuleDefinition::INVALID_DEFINITION_ID)? true : (rule->getId() == id));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
