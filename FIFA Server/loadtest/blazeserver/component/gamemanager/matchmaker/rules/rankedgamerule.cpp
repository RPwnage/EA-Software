/*! ************************************************************************************************/
/*!
    \file   rankedgamerule.cpp

    http://online.ea.com/confluence/display/tech/RankedGameRule+details

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/rankedgamerule.h"
#include "gamemanager/matchmaker/rules/rankedgameruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "framework/util/random.h"

#include "gamemanager/matchmaker/voting.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    RankedGameRule::RankedGameRule(const RankedGameRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mDesiredValue(RankedGameRulePrefs::ABSTAIN)
    {
    }

    RankedGameRule::RankedGameRule(const RankedGameRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus), mDesiredValue(other.mDesiredValue)
    {

    }

    //////////////////////////////////////////////////////////////////////////
    // from ReteRule grandparent
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief
    ***************************************************************************************************/
    bool RankedGameRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        // Desired Any evaluation does not need to be part of the RETE tree, since no matter what value comes in
        // for a game, we wish to match it.
        if (isMatchAny())
        {
            // Can only have random or abstain, don't need to evaluate further.
            return false;
        }
        // set these below
        bool isExactMatch = false;
        float fitPercent = -1;
        FitScore fitScore;
 
        // side: resettable dedicated servers always perfect match, omit from tree
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& orConditions = conditions.push_back();

        // For each possible value, determine the best fitscore against all of the desired values.
        // If the fitscore is a match at the current threshold then add it to the search conditions, else skip it.
        // NOTE: this does not include the random or abstain values.
        RankedGameRule::calcFitPercentInternal(RankedGameRulePrefs::RANKED, fitPercent, isExactMatch);
        fitScore = calcWeightedFitScore(isExactMatch, fitPercent);
        if (isFitScoreMatch(fitScore))
        {
            const char8_t* value = RankedGameRulePrefs::RankedGameDesiredValueToString(RankedGameRulePrefs::RANKED);
            orConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(RankedGameRuleDefinition::RANKEDRULE_RETE_KEY, mRuleDefinition.getWMEAttributeValue(value), mRuleDefinition), 
                fitScore, this));
        }

        calcFitPercentInternal(RankedGameRulePrefs::UNRANKED, fitPercent, isExactMatch);
        fitScore = calcWeightedFitScore(isExactMatch, fitPercent);
        if (isFitScoreMatch(fitScore))
        {
            const char8_t* value = RankedGameRulePrefs::RankedGameDesiredValueToString(RankedGameRulePrefs::UNRANKED);
            orConditions.push_back(Rete::ConditionInfo(Rete::Condition(RankedGameRuleDefinition::RANKEDRULE_RETE_KEY, mRuleDefinition.getWMEAttributeValue(value), mRuleDefinition), 
                fitScore, this));
        }
        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    // from Rule
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*!
        \brief initialize a rankedGameRule instance for use in a matchmaking session. Return true on success.
    
        \param[in]  criteriaData - Criteria data used to initialize the rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    ***************************************************************************************************/
    bool RankedGameRule::initialize(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for resettable servers.
            return true;
        }

        const RankedGameRulePrefs& rankedGameRulePrefs = criteriaData.getRankedGameRulePrefs();

        // init the desired value
        mDesiredValue = rankedGameRulePrefs.getDesiredRankedGameValue();

        // lookup the minFitThreshold list we want to use
        const char8_t* listName = rankedGameRulePrefs.getMinFitThresholdName();

        const bool minFitIni = Rule::initMinFitThreshold( listName, mRuleDefinition, err );
        if (minFitIni)
        {
            updateAsyncStatus();
        }

        return minFitIni;
    }

    /*! ************************************************************************************************/
    /*! \brief vote on predefined rules
        \param[in] votingSessionList list of all the matchmaking sessions that will be voting.
    *************************************************************************************************/
    void RankedGameRule::voteOnGameValues(const MatchmakingSessionList& votingSessionList,
        GameManager::CreateGameRequest& createGameRequest) const
    {
        if(getRankedGameValueFromVote(votingSessionList))
        {
            createGameRequest.getGameCreationData().getGameSettings().setRanked();
        }
    }

    Rule* RankedGameRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW RankedGameRule(*this, matchmakingAsyncStatus);
    }

    //////////////////////////////////////////////////////////////////////////
    // from SimpleRule
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief From SimpleRule. Calculate the fitpercent and exact match for the Game Session.  This method is called
        for and evaluateForMMFindGame to determine the fitpercent
        for a game. For Game Browser and MM Find Game.

        \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
        \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
        \param[out] isExactMatch If the match to this Game Session is an exact match.
    *************************************************************************************************/
    void RankedGameRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const RankedGameRulePrefs::RankedGameDesiredValue actual = gameSession.getGameSettings().getRanked()?
            RankedGameRulePrefs::RANKED : RankedGameRulePrefs::UNRANKED;
        RankedGameRule::calcFitPercentInternal(actual, fitPercent, isExactMatch);

        if (debugRuleConditions.isEnabled())
        {
            debugRuleConditions.writeRuleCondition("game is %s %s %s", RankedGameRulePrefs::RankedGameDesiredValueToString(actual) , isExactMatch?"==":"!=", RankedGameRulePrefs::RankedGameDesiredValueToString(mDesiredValue) );
        }
    }
    /*! ************************************************************************************************/
    /*! \brief From SimpleRule. Calculate the fitpercent and exact match for the Game Session.  This method is called
        for evaluateForMMFindGame to determine the fitpercent
        for a game. For Game Browser and MM Find Game.

        \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
        \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
        \param[out] isExactMatch If the match to this Game Session is an exact match.
    *************************************************************************************************/
    void RankedGameRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const RankedGameRule& otherRankedGameRule = static_cast<const RankedGameRule&>(otherRule);
        RankedGameRule::calcFitPercentInternal(otherRankedGameRule.getDesiredValue(), fitPercent, isExactMatch);

        if (debugRuleConditions.isEnabled())
        { 
            debugRuleConditions.writeRuleCondition("other is %s %s %s", RankedGameRulePrefs::RankedGameDesiredValueToString(otherRankedGameRule.getDesiredValue()) , isExactMatch?"==":"!=", RankedGameRulePrefs::RankedGameDesiredValueToString(mDesiredValue) );
        }
    }


    /*! ************************************************************************************************/
    /*! \brief from SimpleRule. Side(blaze 3.03): SimpleRule::evaluateForMMCreateGame already handled the handleDisabledRuleEvaluation, fixupMatchTimesForRequirements, so all we need is a call to enforceRestrictiveOneWayMatchRequirements
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void RankedGameRule::postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        const RankedGameRule& otherRankedGameRule = static_cast<const RankedGameRule&>(otherRule);
        RankedGameRule::enforceRestrictiveOneWayMatchRequirements(otherRankedGameRule, matchInfo);
    }

    void RankedGameRule::enforceRestrictiveOneWayMatchRequirements(const RankedGameRule &otherRule, MatchInfo &myMatchInfo) const
    {
        // MM_AUDIT, if the other person is not exact match, do we need to enforce this?

        // Don't allow abstain || random to pull in another session that desires ranked or unranked.
        if (((mDesiredValue == RankedGameRulePrefs::RANDOM) || (mDesiredValue == RankedGameRulePrefs::ABSTAIN)) && 
            ((otherRule.mDesiredValue == RankedGameRulePrefs::RANKED) || otherRule.mDesiredValue == RankedGameRulePrefs::UNRANKED))
        {
            myMatchInfo.setNoMatch();
        }
    }



    // local helper
    // get the fit score of me vs other
    void RankedGameRule::calcFitPercentInternal(RankedGameRulePrefs::RankedGameDesiredValue otherValue, float &fitPercent, bool &isExactMatch) const
    {
        // see http://online.ea.com/confluence/display/tech/RankedGameRule+details one way restrictive matches are enforced later.
        if ( (mDesiredValue == otherValue)
            || (otherValue == RankedGameRulePrefs::ABSTAIN) || (otherValue == RankedGameRulePrefs::RANDOM)
            || (mDesiredValue ==RankedGameRulePrefs::ABSTAIN) || (mDesiredValue == RankedGameRulePrefs::RANDOM))
        {
            isExactMatch = true;
            fitPercent = RankedGameRule::getDefinition().getMatchingFitPercent();
        }
        else
        {
            isExactMatch = false;
            fitPercent = RankedGameRule::getDefinition().getMismatchFitPercent();
        }
       
    }

  
    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    ****************************************************************************************************/
    void  RankedGameRule::updateAsyncStatus()
    {
        uint8_t matchedRankFlags = 0;

        if (isDisabled())
        {
            // rule is disabled, we'll match anything/anyone
            matchedRankFlags = (RankedGameRulePrefs::RANKED | RankedGameRulePrefs::UNRANKED | RankedGameRulePrefs::RANDOM | RankedGameRulePrefs::ABSTAIN);
        }
        else
        {
            // Note: this is a slightly inefficient way to determine what we match. However, this function
            //  is called infrequently and an efficient impl would require duplicated logic between eval & getStatus.
            //  I think the minor inefficiency is well worth the ease of maintenance.

            float fitPercent;
            bool isExactMatch;
            
            calcFitPercentInternal(RankedGameRulePrefs::RANKED, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedRankFlags |= RankedGameRulePrefs::RANKED;

            calcFitPercentInternal(RankedGameRulePrefs::UNRANKED, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedRankFlags |= RankedGameRulePrefs::UNRANKED;

            calcFitPercentInternal(RankedGameRulePrefs::RANDOM, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedRankFlags |= RankedGameRulePrefs::RANDOM;

            calcFitPercentInternal(RankedGameRulePrefs::ABSTAIN, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedRankFlags |= RankedGameRulePrefs::ABSTAIN;
        }

        Rule::mMatchmakingAsyncStatus->getRankRuleStatus().setMatchedRankFlags(matchedRankFlags);
    }

    /*! ************************************************************************************************/
    /*!
        \brief determine a single value for the rankedGame setting, using this rule's votingMethod and the supplied
            list of game members.  (ie: vote on a rankedGame setting to use)
    
        \param[in] memberSessions - member session list including owner session and all it's member session
        \return true if the game should be ranked, false otherwise.
    ***************************************************************************************************/
    bool RankedGameRule::getRankedGameValueFromVote(const MatchmakingSessionList& memberSessions) const
    {

        // we get an unranked game if the rule is disabled
        if (isDisabled())
        {
            return false;
        }

        RuleVotingMethod votingMethod = getDefinition().getVotingMethod();
        if (MinFitThresholdList::isThresholdExactMatchRequired(mCurrentMinFitThreshold))
        {
            votingMethod = OWNER_WINS;
        }

        //////////////////////////////////////////////////////////////////////////
        // New Voting
        //////////////////////////////////////////////////////////////////////////


        // tally the votes
        Voting::VoteTally voteTally;
        RankedGameRulePrefs::RankedGameDesiredValue winningValue = RankedGameRulePrefs::RANDOM;
        size_t totalVoteCount = tallyVotes(memberSessions, voteTally);

        if (totalVoteCount == 0)
        {
            // everyone abstained
            winningValue = RankedGameRulePrefs::ABSTAIN;
        }
        else
        {
            if (votingMethod == OWNER_WINS)
            {
                winningValue = mDesiredValue;
            }
            else
            {
                winningValue = getRankedGameDesiredValue((PossibleRankedVoteValue)getDefinition().vote(voteTally, totalVoteCount, votingMethod));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Old voting
        //////////////////////////////////////////////////////////////////////////

        // translate the winning DesiredValue (which can contain abstain/random) into a bool rankedGame setting
        switch (winningValue)
        {
        case RankedGameRulePrefs::RANKED:
            return true;
        case RankedGameRulePrefs::UNRANKED:
            return false;
        default:
            // random or abstain won
            // Note: abstain is only possible if OWNER_WINS was set and the owner abstained
            // or if it the voting method is VOTE_LOTTERY and all votes are abstain.
            return (Random::getRandomNumber(2) == 1); // randomly return true or false (ranked or unranked)
        }
    }

    /*! ************************************************************************************************/
    /*! \brief given a list of game members, tally each member's vote for the final rankedGame setting.
    
        \param[in] memberSessions - member session list including owner session and all it's member session
        \param[out] numRankedVotes - number of votes for a ranked game
        \param[out] numUnrankedVotes - number of votes for an unranked game
        \param[out] numRandomVotes - number of votes for a random ranked game setting
    ***************************************************************************************************/
    size_t RankedGameRule::tallyVotes(const MatchmakingSessionList& memberSessions, Voting::VoteTally& voteTally) const
    {
        size_t totalVoteCount = 0;
        // zero out the tally
        voteTally.clear();
        voteTally.resize(MAX_POSSIBLE_RANKED_VOTE_VALUES);

        // count votes (one vote for each user in the matchmaking session)
        MatchmakingSessionList::const_iterator gameMemberIter = memberSessions.begin();
        MatchmakingSessionList::const_iterator end = memberSessions.end();
        for ( ; gameMemberIter != end; ++gameMemberIter)
        {
            const RankedGameRule* otherRule = static_cast<const RankedGameRule*>(
                (*gameMemberIter)->getCriteria().getRuleByIndex(getRuleIndex()));
            if ((otherRule != nullptr) && (otherRule->mDesiredValue != RankedGameRulePrefs::ABSTAIN))
            {
                totalVoteCount++;
                PossibleRankedVoteValue voteValue = getPossibleRankedVoteValue(otherRule->mDesiredValue);
                voteTally[voteValue] = voteTally[voteValue] + (*gameMemberIter)->getPlayerCount(); //Note: avoiding uin16_t += conversion warning
            }
        }

        return totalVoteCount;
    }

    // TODO: make this generic and move to the Voting System.
    RankedGameRule::PossibleRankedVoteValue RankedGameRule::getPossibleRankedVoteValue(RankedGameRulePrefs::RankedGameDesiredValue rankedGameDesiredValue) const
    {
        EA_ASSERT(rankedGameDesiredValue == RankedGameRulePrefs::RANKED
            || rankedGameDesiredValue == RankedGameRulePrefs::UNRANKED
            || rankedGameDesiredValue == RankedGameRulePrefs::RANDOM); // ABSTAIN does not vote.

        switch (rankedGameDesiredValue)
        {
        case RankedGameRulePrefs::RANKED:
            return VOTE_VALUE_RANKED;
        case RankedGameRulePrefs::UNRANKED:
            return VOTE_VALUE_UNRANKED;
        case RankedGameRulePrefs::RANDOM:
            return VOTE_VALUE_RANDOM;
        default:
            break;
        }

        // Should never get here.
        ERR_LOG("[RankedGameRule].getPossibleRankedVoteValue ERROR invalid desired Value '" << (RankedGameRulePrefs::RankedGameDesiredValueToString(rankedGameDesiredValue)) << "'");
        return VOTE_VALUE_RANKED;
    }

    RankedGameRulePrefs::RankedGameDesiredValue RankedGameRule::getRankedGameDesiredValue(PossibleRankedVoteValue possibleRankedVoteValue) const
    {
        EA_ASSERT(possibleRankedVoteValue == VOTE_VALUE_RANKED
            || possibleRankedVoteValue == VOTE_VALUE_UNRANKED
            || possibleRankedVoteValue == VOTE_VALUE_RANDOM); // MAX_POSSIBLE_RANKED_VOTE_VALUES is not a valid value.

        switch (possibleRankedVoteValue)
        {
        case VOTE_VALUE_RANKED:
            return RankedGameRulePrefs::RANKED;
        case VOTE_VALUE_UNRANKED:
            return RankedGameRulePrefs::UNRANKED;
        case VOTE_VALUE_RANDOM:
            return RankedGameRulePrefs::RANDOM;
        default:
            break;
        }

        // Should never get here.
        ERR_LOG("[RankedGameRule].getRankedGameDesiredValue ERROR invalid vote Value '" << possibleRankedVoteValue << "'");
        return RankedGameRulePrefs::ABSTAIN;
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics
    ****************************************************************************************************/
    void RankedGameRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        setupInfos.push_back(RuleDiagnosticSetupInfo("desiredRankedSetting", RankedGameRulePrefs::RankedGameDesiredValueToString(mDesiredValue)));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

