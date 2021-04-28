/*! ************************************************************************************************/
/*!
\file gameattributerule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEATTRIBUTERULE_H
#define BLAZE_GAMEMANAGER_GAMEATTRIBUTERULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/gameattributeruledefinition.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameAttributeRule : public SimpleRule
    {
        NON_COPYABLE(GameAttributeRule);
    public:

        GameAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        GameAttributeRule(const GameAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~GameAttributeRule() override;


        // Fit Percent Logic:
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& ruleConditions) const override;

        /*! ************************************************************************************************/
        /*!
            \brief compare each desired value against the supplied actual value, returning the highest
                fitTable entry (the highest fitPercent).  If the entity's value is invalid/unknown, return -1.

            This is a N-to-one comparison (many desired values vs a single actual value).

            \param[in]  actualValueIndex the possibleValue index representing another entity's actual value.
            \return the highest fitPercent (0..1), or < 0 if the entity's value is invalid/unknown.
        *************************************************************************************************/
        float getFitPercent(size_t actualValueIndex) const
        {
            float maxFitPercent = -1;
            size_t myIndex = mDesiredValuesBitset.find_first();
            while (myIndex < mDesiredValuesBitset.kSize)
            {
                maxFitPercent = eastl::max_alt(maxFitPercent, getDefinition().getFitPercent(myIndex, actualValueIndex));
                myIndex = mDesiredValuesBitset.find_next(myIndex);
            }

            return maxFitPercent;
        }

        /*! ************************************************************************************************/
        /*!
            \brief compare each desired value against the supplied entity valueBitSet, returning the highest
                fitTable entry (the highest fitPercent).  If the entity's value is invalid/unknown, return -1.

            This is a N-to-M comparison (many desired values vs many desired values).  Used when evaluating
            another matchmaking session for CreateGame.

            \param[in]  entityValueBitset the possibleValues bitset representing another entity's desired values.
            \return the highest fitPercent (0..1), or < 0 if the entity's value is invalid/unknown.
        *************************************************************************************************/
        inline float getFitPercent(const GameAttributeRuleValueBitset &entityValueBitset) const;

        // Voting Logic:
        void voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const override;
        size_t tallyVotes(const MatchmakingSessionList& memberSessions, Voting::VoteTally& voteTally, const GameAttributeRuleValueBitset* ownerWinsPossValues) const;
        size_t tallyPlayersVotes(const uint16_t sessionUsersNum, Voting::VoteTally& voteTally, const GameAttributeRuleValueBitset* ownerWinsPossValues) const;
        const char8_t* voteOwnerWins(const Voting::VoteTally& voteTally) const;
        size_t resolveMultipleDesiredValuesOwnerWins(const Voting::VoteTally& voteTally) const;


        // Rete
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        // Rule Interface
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;
        virtual const GameAttributeRuleDefinition& getDefinition() const { return static_cast<const GameAttributeRuleDefinition&>(mRuleDefinition); }

        void updateAsyncStatus() override;
        bool isDisabled() const override { return mRuleOmitted || SimpleRule::isDisabled(); }
        bool isMatchAny() const override { return (mIsRandomDesired || mIsAbstainDesired || Rule::isMatchAny()); }

        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        FitScore evaluateForMMCreateGameFinalization(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const override;

        size_t getNumDesiredValues() const { return mDesiredValuesBitset.count(); }
        const GameAttributeRuleValueBitset& getDesiredValuesBitset() const { return mDesiredValuesBitset; }
        void buildDesiredValueListString(eastl::string& buffer, const GameAttributeRuleValueBitset &desiredValuesBitset) const;
        void buildDesiredValueListString(eastl::string& buffer, const Collections::AttributeValueList &desiredValues) const;

        const GameAttributeRuleCriteria* getGameAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const;

        bool isExplicitType() const { return getDefinition().getAttributeRuleType() == EXPLICIT_TYPE; }
        bool isArbitraryType() const { return getDefinition().getAttributeRuleType() == ARBITRARY_TYPE; }
    private:

        void enforceRestrictiveOneWayMatchRequirements(const GameAttributeRule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const;

        /*! ************************************************************************************************/
        /*!
            \brief Init this rule's desired rule values bitSet.

            To speed up rule evaluation, we store a rule's desired value list as a bitSet; this allows
                us to do fast comparisons when evaluating against another rule (while pooling players
                for CreateGame sessions).

            The bitset mirrors the ruleDefinition's possibleValueList; if a possibleValue is desired,
            it's index in the bitSet is set.

            For example, if the possibleValues are {"A","B","C"}, and the desired Values are {"C","A"},
            the desiredValueBitset is {1,0,1}.

            \param[in]  desiredValuesList   the list of desired values for this rule.
            \param[out] err the errMessage field is filled out if there's a problem
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initDesiredValuesBitset(const Collections::AttributeValueList& desiredValuesList, MatchmakingCriteriaError &err);

        bool isRandomOrAbstainDesired() const { return mIsRandomDesired || mIsAbstainDesired; }

    // Members
    private:

        GameAttributeRuleStatus mGameAttributeRuleStatus;
        bool mRuleOmitted;
        bool mIsRandomDesired;
        bool mIsAbstainDesired;

        // Note: we use a stl bitset for the general desired values because the size (# of possible values) is
        //   set by the game team, and we must iterate over the list.
        GameAttributeRuleValueBitset mDesiredValuesBitset; // flags indicate which of the rule defn's possible values are desired.
        GameAttributeRuleValueBitset mMatchedValuesBitset; // flags indicate which of the rule defn's possible values currently match.


    private:
        // Arbitrary Attr Params:
        const ArbitraryValueSet& getDesiredValues() const { return mDesiredValues; }
        const Collections::AttributeValueList& getDesiredValueStrings() const { return mDesiredValueStrs; }

        void calcFitPercentInternal(int gameAttribValueIndex, float &fitPercent, bool &isExactMatch) const;
        void calcFitPercentInternal(const GameAttributeRuleValueBitset &otherDesiredValuesBitset, float &fitPercent, bool &isExactMatch) const;
        void calcFitPercentInternal(ArbitraryValue otherValue, float &fitPercent, bool &isExactMatch) const;
        void calcFitPercentInternal(const ArbitraryValueSet &otherValues, float &fitPercent, bool &isExactMatch) const;

        /*! ************************************************************************************************/
        /*!
            \brief Init this rule's desired values.

            Since we are only storing 1 value, we don't need to really optimize for the fact that
                it'll be doing a m:n comparison (unlike the case for PlayerAttributeRule).

            So we can just do a straight up comparison at the same cost basically.  We won't be able
                to benefit from any "union" (bit) operations though.

            \param[in]  desiredValuesList   the list of desired values for this rule.
            \param[out] err the errMessage field is filled out if there's a problem
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initDesiredValues(const Collections::AttributeValueList& desiredValuesList,
            MatchmakingCriteriaError &err);

        ArbitraryValueSet mDesiredValues;
        Collections::AttributeValueList mDesiredValueStrs;


        void createGameAttributeFromVote(const MatchmakingSessionList& memberSessions, Collections::AttributeMap& map) const;

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
        // Note: the base RETE based getDiagnosticGamesVisible() is accurate, given games can only have 1 game attribute value. no need to override it
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMEATTRIBUTERULE_H
