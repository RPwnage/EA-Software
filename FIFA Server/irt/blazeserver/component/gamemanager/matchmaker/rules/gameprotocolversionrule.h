/*! ************************************************************************************************/
/*!
\file   gameprotocolversionrule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE
#define BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/tdf/gamemanager.h" // for GameProtocolVersionString
#include "gamemanager/matchmaker/rules/gameprotocolversionruledefinition.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {

            /*! ************************************************************************************************/
            /*!
                \brief A simple predefined rule that filters out games or sessions that don't match the rule's
                required gameProtocolVersion.

                This rule allows a title to segment matchmaking sessions/games.  You'll only match games (or other players)
                that have the same gameProtocolVersion as you do.
            ***************************************************************************************************/
            class GameProtocolVersionRule : public SimpleRule
            {
            public:
                static uint64_t GAME_PROTOCOL_VERSION_MATCH_ANY_HASH;

                              

                /*! ************************************************************************************************/
                /*! \brief Rule constructor.

                    \param[in] ruleDefinition - the definition object associated with this particular rule
                    \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
                *************************************************************************************************/
                GameProtocolVersionRule(const GameProtocolVersionRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

                /*! ************************************************************************************************/
                /*! \brief Rule constructor.

                    \param[in] ruleDefinition - the definition object associated with this particular rule
                    \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
                *************************************************************************************************/
                GameProtocolVersionRule(const GameProtocolVersionRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
                
                /*! ************************************************************************************************/
                /*! \brief Rule destructor.  Cleanup any allocated memory
                *************************************************************************************************/
                ~GameProtocolVersionRule() override;

                Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW GameProtocolVersionRule(*this, matchmakingAsyncStatus); }

                /*! ************************************************************************************************/
                /*!
                    \brief Initialize the required game protocol version
                    \param[in]     criteriaData data required to initialize the matchmaking rule.
                    \param[in]     matchmakingSupplementalData supplemental data about matchmaking not passed in through the request and criteriaData.
                    \param[in/out] err error message returned on initialization failure.
                *************************************************************************************************/
                bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;
    
                //nothing to do for this one.
                void updateAsyncStatus() override {}

                bool isDisabled() const override { return false; }

                bool preEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;

                /*! ************************************************************************************************/
                /*! \brief Calculate the fit percent match of this rule against a game session. Used for calculating
                    a match during a matchmaking find game, or a gamebrowser list search.

                    \param[in] gameSession - the game you are evaluating against
                    \param[in/out] fitPercent - The fit percent match your calculation yields.
                    \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
                *************************************************************************************************/
                void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

                /*! ************************************************************************************************/
                /*! \brief Calculate the fit percent match of this rule against another rule.  Used for calculating
                    a match during a matchmaking create game.

                    \param[in] otherRule - The other sessions rule to calculate against
                    \param[in/out] fitPercent - The fit percent match your calculation yields.
                    \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
                *************************************************************************************************/
                void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

                        
                /*! ************************************************************************************************/
                /*!
                  \brief Return the GameProtocolVersionString required by this rule
                  \return the GameProtocolVersionString required by this rule
                ***************************************************************************************************/
                const GameProtocolVersionString& getRequiredGameProtocolVersionString() const { return mRequiredGameProtocolVersionString; }
                uint64_t getRequiredGameProtocolVersionStringHash() const { return mRequiredGameProtocolVersionStringHash; }


                bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;

                FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

                bool isMatchAny() const override { return mRequiredGameProtocolVersionStringHash == static_cast<const GameProtocolVersionRuleDefinition&>(mRuleDefinition).getMatchAnyHash(); }

                bool isDedicatedServerEnabled() const override { return true; }

                /*! *********************************************************************************************/
                /*! \brief Single Group Match Rule doesn't contribute to the fit score because it is a filtering rule.
                
                    \return 0
                *************************************************************************************************/
                FitScore getMaxFitScore() const override { return 0; }
            private:

                /*! ************************************************************************************************/
                /*!
                    \brief evaluate the supplied gameProtocolVersion against the version this rule requires.  Returns
                    a fitScore, or a negative number of the protocols don't match.

                    \param[in] actualGameProtocolVersion - the actual GameProtocolVersion to test this rule against
                    \param[in] actualGameProtocolVersionString - the actual GameProtocolVersionString to test this rule against
                    \return the weighted fitScore for this rule.  The game is not a match if the fitScore is negative.
                ***************************************************************************************************/
                inline float evaluate(const char8_t *actualGameProtocolVersionString, uint64_t actualGameProtocolVersionStringHash) const;

                void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

            private:
                GameProtocolVersionString mRequiredGameProtocolVersionString;
                bool mEvaluateGameProtocolVersionString;
                uint64_t mRequiredGameProtocolVersionStringHash;
            };

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE
