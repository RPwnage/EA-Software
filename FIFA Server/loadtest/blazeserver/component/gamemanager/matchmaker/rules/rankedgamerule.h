/*! ************************************************************************************************/
/*!
    \file   rankedgamerule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_RANKED_GAME_RULE
#define BLAZE_MATCHMAKING_RANKED_GAME_RULE

#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/matchmaker/rules/rankedgameruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class MatchmakingCriteriaError;
namespace Matchmaker
{
    class Matchmaker;
    struct MatchmakingSupplementalData;
    class VoteTally;

    /*! ************************************************************************************************/
    /*!
        \brief Predefined matchmaking rule dealing with desired and actual values for a game's 'ranked' setting.

        Games are either ranked or unranked, and a matchmaking session can specify ranked, unranked, random, or abstain
        as the desired values for the ranked setting.  (See RankedGameRulePrefs::RankedGameDesiredValue).

        The rankedGameRule can be disabled (for a session) if the client sends up an empty string as the name
        of the minFitThresholdList to use.  If disabled, the rule will consider any ranked value a perfect match.
        Disable this rule if your title doesn't support ranked games.

        Note: This rule will always result in a perfect match against dedicated servers that are available for reset.

        See RankedGameRuleDefinition for info about the server-side configuration and settings for this rule.
    ***************************************************************************************************/
    class RankedGameRule : public SimpleRule
    {
    public:
        enum PossibleRankedVoteValue
        {
            VOTE_VALUE_RANKED,
            VOTE_VALUE_UNRANKED,
            VOTE_VALUE_RANDOM,
            MAX_POSSIBLE_RANKED_VOTE_VALUES
        };
        PossibleRankedVoteValue getPossibleRankedVoteValue(RankedGameRulePrefs::RankedGameDesiredValue rankedGameDesiredValue) const;
        RankedGameRulePrefs::RankedGameDesiredValue getRankedGameDesiredValue(PossibleRankedVoteValue possibleRankedVoteValue) const;

    public:
        //! \brief You must initialize the rule before using it.
        RankedGameRule(const RankedGameRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        RankedGameRule(const RankedGameRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        //! \brief Destructor
        ~RankedGameRule() override {}

        //////////////////////////////////////////////////////////////////////////
        // from ReteRule grandparent
        //////////////////////////////////////////////////////////////////////////
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        bool isMatchAny() const override { return ((mDesiredValue & RankedGameRulePrefs::RANDOM) != 0) || ((mDesiredValue & RankedGameRulePrefs::ABSTAIN) != 0) || SimpleRule::isMatchAny(); }
        
        bool isDedicatedServerEnabled() const override { return false; }
        void voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const override;
        
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
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;
      
        //////////////////////////////////////////////////////////////////////////
        // from SimpleRule
        //////////////////////////////////////////////////////////////////////////
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        //////////////////////////////////
        // RankedGameRule specific
        //////////////////////////////////

        /*! ************************************************************************************************/
        /*!
            \brief determine a single value for the rankedGame setting, using this rule's votingMethod and the supplied
                list of game members.  (ie: vote on a rankedGame setting to use)
        
            \param[in] memberSessions - member session list including owner session and all it's member session
            \return true if the game should be ranked, false otherwise.
        ***************************************************************************************************/
        bool getRankedGameValueFromVote(const MatchmakingSessionList& memberSessions) const;

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // from SimpleRule
        //////////////////////////////////////////////////////////////////////////
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;

    private:
        RankedGameRule(const RankedGameRule& rhs);
        RankedGameRule &operator=(const RankedGameRule &);

        void enforceRestrictiveOneWayMatchRequirements(const RankedGameRule &otherRule, MatchInfo &myMatchInfo) const;

        void calcFitPercentInternal(RankedGameRulePrefs::RankedGameDesiredValue otherValue, float &fitPercent, bool &isExactMatch) const;

        /*! ************************************************************************************************/
        /*! \brief given a list of game members, tally each member's vote for the final rankedGame setting.
        
            \param[in] memberSessions - member session list including owner session and all it's member session
            \param[out] numRankedVotes - number of votes for a ranked game
            \param[out] numUnrankedVotes - number of votes for an unranked game
            \param[out] numRandomVotes - number of votes for a random ranked game setting
        ***************************************************************************************************/
        size_t tallyVotes(const MatchmakingSessionList& memberSessions, Voting::VoteTally& voteTally) const;

        const RankedGameRuleDefinition& getDefinition() const { return static_cast<const RankedGameRuleDefinition&>(mRuleDefinition); }
        RankedGameRulePrefs::RankedGameDesiredValue getDesiredValue() const { return mDesiredValue; } //used by calcFitPercent

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

    private:

        RankedGameRulePrefs::RankedGameDesiredValue mDesiredValue;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_RANKED_GAME_RULE
