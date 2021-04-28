/*! ************************************************************************************************/
/*!
    \file   playercountrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_DESIRED_PLAYER_COUNT
#define BLAZE_MATCHMAKING_DESIRED_PLAYER_COUNT

#include "gamemanager/matchmaker/rules/playercountruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class PlayerCountRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
    \brief Blaze Matchmaking predefined rule.  The Player Count Rule evaluates matches based on the game
           size. By default this rule tries to match players based off of their rule preferences
           (desired player count) and requirements (min, max player count). For create game mode this means
           trying to pool people together to create a game of a desired count. For find game mode this
           means trying to find an existing game that has the desired number of players and joining it.
           The fit percent is evaluated based off of the desired player count.

    SINGLE GROUP MATCH

        Sometimes clients want to match only another game group. In this case they should set the Single
        Group Match flag to 1. When the Single Group Match flag is set this session will only match
        another single game group. This means it will evaluate all of the potential matches but will select
        the best match during finalization and stop looking. It will also take into account the groups
        size when determining the fit percent. The fit percent is calculated based off the sum of the
        evaluating sessions group size and the other group size vs. the desired game size clamping around
        the min/max game size requirements. This flag is meaningful only in create game mode since in find
        game we don't know if the other players in the game are a group.

        The PlayerCountRule prioritizes matches via a fit percent using a fit formula function (e.g. Gaussian) defined in the rule
        configuration.  E.g. For Gaussian, the bell curve's width is specified in the definition.  The curve is centered at
        the desiredPlayerCount of the game browser request or matchmaking session being evaluated.
        For example a game that matches your desiredPlayerCount will have a fit percent of
        100% (1.0)).  From there the fit is determined by a function of the difference between the two
        sizes on the bell curve.

        The curve's width/slope is determined by the definition, and clamped by min/max player count requirements.

        See PlayerCountRuleDefinition, and the Dev Guide for Blaze Matchmaking for more information.
    *************************************************************************************************/
    class PlayerCountRule : public SimpleRangeRule
    {
    public:
        PlayerCountRule(const PlayerCountRuleDefinition &definition, MatchmakingAsyncStatus* status);
        PlayerCountRule(const PlayerCountRule &other, MatchmakingAsyncStatus* status);

        ~PlayerCountRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW PlayerCountRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's PlayerCountRuleCriteria
                (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

            \param[in]  criteriaData - data required to initialize the matchmaking rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        /*! ************************************************************************************************/
        /*!
            \brief returns true if the supplied participantCount satisfies the rule's current rangeOffsetList
                value.

                Used to see if a createGame matchmaking session can be finalized; if the number of matched
                participant is high enough to create a game with.
        
            \param[in] potentialParticipantCount The pool size (number of matched participants + yourself) you have to draw
                participants from (when creating a game)
            \return true if the potentialParticipantCount is sufficient to create a game
        ***************************************************************************************************/
        bool isParticipantCountSufficientToCreateGame(size_t potentialPlayerCount) const;
        
        /*! ************************************************************************************************/
        /*!
            \brief return the absolute minimum participant capacity for a createGame matchmaking session. (May not match what is acceptable)
                (minimum game size)
        ***************************************************************************************************/
        uint16_t getMinParticipantCount() const { return mMinParticipantCount; }

        /*! ************************************************************************************************/
        /*!
            \brief return the minimum acceptable participant capacity for a createGame matchmaking session.
                (minimum game size), based on the minimum value passed to the rule AND the current fit threshold.
        ***************************************************************************************************/
        uint16_t calcMinPlayerCountAccepted() const;
        /*! ************************************************************************************************/
        /*!
            \brief return the maximum acceptable participant capacity for a createGame matchmaking session.
                (minimum game size), based on the maximum value passed to the rule AND the current fit threshold.
        ***************************************************************************************************/
        uint16_t calcMaxPlayerCountAccepted() const;

        /*! ************************************************************************************************/
        /*!
            \brief return if a group is playing with only one another group. 
            \return true if it is a single group match.
        ***************************************************************************************************/
        bool isSingleGroupMatch() const { return mIsSingleGroupMatch; }

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDedicatedServerEnabled() const override { return false; }

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        const PlayerCountRuleDefinition& getDefinition() const { return static_cast<const PlayerCountRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;


    private:
        void calcFitPercentInternal(uint16_t otherDesiredPlayerCount, float &fitPercent, bool &isExactMatch) const;

    private:
        PlayerCountRule(const PlayerCountRule& rhs);
        PlayerCountRule &operator=(const PlayerCountRule &);

        uint16_t mDesiredParticipantCount;
        uint16_t mMinParticipantCount;
        uint16_t mMaxParticipantCount;
        uint16_t mMyGroupParticipantCount;
        bool  mIsSingleGroupMatch;
        UserGroupId mUserGroupId;
        bool mIsSimulatingRosterSizeRule;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_DESIRED_PLAYER_COUNT
