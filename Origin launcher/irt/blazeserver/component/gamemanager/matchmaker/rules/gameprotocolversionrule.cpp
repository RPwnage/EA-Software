/*! ************************************************************************************************/
/*!
\file   gameprotocolversionrule.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gameprotocolversionrule.h"
#include "gamemanager/matchmaker/rules/gameprotocolversionruledefinition.h"
#include "gamemanager/matchmaker/matchlist.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "EASTL/functional.h"


namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            uint64_t GameProtocolVersionRule::GAME_PROTOCOL_VERSION_MATCH_ANY_HASH = 0;

           GameProtocolVersionRule::GameProtocolVersionRule(const GameProtocolVersionRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
                : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
                mEvaluateGameProtocolVersionString(false)
            {
            }

            GameProtocolVersionRule::GameProtocolVersionRule(const GameProtocolVersionRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
                : SimpleRule(otherRule, matchmakingAsyncStatus),
                mRequiredGameProtocolVersionString(otherRule.mRequiredGameProtocolVersionString),
                mEvaluateGameProtocolVersionString(otherRule.mEvaluateGameProtocolVersionString),
                mRequiredGameProtocolVersionStringHash(otherRule.mRequiredGameProtocolVersionStringHash)
            {
            }

            GameProtocolVersionRule::~GameProtocolVersionRule() {}

            bool GameProtocolVersionRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
            {
                eastl::hash<const char8_t*> stringHash;

                mRequiredGameProtocolVersionString = matchmakingSupplementalData.mGameProtocolVersionString;
                mEvaluateGameProtocolVersionString = matchmakingSupplementalData.mEvaluateGameProtocolVersionString;
                mRequiredGameProtocolVersionStringHash = stringHash(mRequiredGameProtocolVersionString.c_str());
                TRACE_LOG("[GameProtocolVersionRule].initialize: eval(" << (mEvaluateGameProtocolVersionString ? "true" : "false") << "), RGPVStr(" 
                          << mRequiredGameProtocolVersionString.c_str() << "), RGPVHash(" << mRequiredGameProtocolVersionStringHash << ")");

                return true;
            }

            bool GameProtocolVersionRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
            {
                if (!mEvaluateGameProtocolVersionString || isMatchAny())
                {
                    return false;
                }

                Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
                Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(GameProtocolVersionRuleDefinition::GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME, mRuleDefinition.getWMEAttributeValue(mRequiredGameProtocolVersionString), mRuleDefinition),
                    0, this));
                // We add in match any for any search to ensure we can find games that are match any client
                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(GameProtocolVersionRuleDefinition::GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME, mRuleDefinition.getWMEAttributeValue(GAME_PROTOCOL_VERSION_MATCH_ANY), mRuleDefinition),
                    0, this));

                return true;
            }

            bool GameProtocolVersionRule::preEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const 
            {
                const GameProtocolVersionRule& otherGameProtocolVersionRule = static_cast<const GameProtocolVersionRule &>(otherRule);
                float fitPercent = evaluate( otherGameProtocolVersionRule.getRequiredGameProtocolVersionString(),
                    otherGameProtocolVersionRule.getRequiredGameProtocolVersionStringHash());
                if (fitPercent == FIT_PERCENT_NO_MATCH)
                {
                    matchInfo.sDebugRuleConditions.writeRuleCondition("Mismatch This Rule (id '%" PRIu64 "', GameProtocolVersionString: \"%s(%" PRIu64 ")\"), Other Rule (id '%" PRIu64 "', GameProtocolVersionString: \"%s(%" PRIu64 ")\"",
                        getMMSessionId(), 
                        mRequiredGameProtocolVersionString.c_str(), mRequiredGameProtocolVersionStringHash,
                        otherGameProtocolVersionRule.getMMSessionId(), 
                        otherGameProtocolVersionRule.getRequiredGameProtocolVersionString().c_str(), 
                        otherGameProtocolVersionRule.getRequiredGameProtocolVersionStringHash());
                    matchInfo.setNoMatch();
                    return false;
                }
                return true;
            }

            void GameProtocolVersionRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
            {
                fitPercent = evaluate(gameSession.getGameProtocolVersionString(), gameSession.getGameProtocolVersionHash());

                //This rule is binary, all or nothing so any match is an exact match
                isExactMatch = (fitPercent != FIT_PERCENT_NO_MATCH);

                
                debugRuleConditions.writeRuleCondition("Game gpv '%s' %s my gpv '%s'", gameSession.getGameProtocolVersionString(), isExactMatch?" == ":"!=", mRequiredGameProtocolVersionString.c_str() );
    
            }


            void GameProtocolVersionRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
            {
                const GameProtocolVersionRule& otherGameProtocolVersionRule = static_cast<const GameProtocolVersionRule &>(otherRule);
                fitPercent = evaluate( otherGameProtocolVersionRule.getRequiredGameProtocolVersionString(),
                    otherGameProtocolVersionRule.getRequiredGameProtocolVersionStringHash());

                //This rule is binary, all or nothing so any match is an exact match
                isExactMatch = (fitPercent != FIT_PERCENT_NO_MATCH);

                
                debugRuleConditions.writeRuleCondition("Other gpv '%s' %s my gpv '%s'", otherGameProtocolVersionRule.getRequiredGameProtocolVersionString().c_str(), isExactMatch?" == ":"!=", mRequiredGameProtocolVersionString.c_str() );
    
            }


          
            /*! ************************************************************************************************/
            /*!
            \brief evaluate the supplied gameProtocolVersion against the version this rule requires.  Returns
            a fitScore, or a negative number of the protocols don't match.

            actualGameProtocolVersion is deprecated, but it is left here for backwards compatibility.

            \param[in] actualGameProtocolVersion - the actual GameProtocolVersion to test this rule against
            \param[in] actualGameProtocolVersionString - the actual GameProtocolVersionString to test this rule against
            \return the weighted fitScore for this rule.  The game is not a match if the fitScore is negative.
            ***************************************************************************************************/
            inline float GameProtocolVersionRule::evaluate(const char8_t *actualGameProtocolVersionString, uint64_t actualGameProtocolVersionStringHash) const
            {
                // if we evaluate the game protocol version string,
                // Check if the two versions are the same, or if either of the versions are match any.
                if ((mEvaluateGameProtocolVersionString) &&
                    (mRequiredGameProtocolVersionStringHash != actualGameProtocolVersionStringHash) &&
                    (mRequiredGameProtocolVersionStringHash != static_cast<const GameProtocolVersionRuleDefinition&>(mRuleDefinition).getMatchAnyHash()) &&
                    (actualGameProtocolVersionStringHash != static_cast<const GameProtocolVersionRuleDefinition&>(mRuleDefinition).getMatchAnyHash()))
                {
                    return FIT_PERCENT_NO_MATCH;
                }

                //This rule has no effect on the fitscore.
                return 0.0f;
            }

            /*! ************************************************************************************************/
            /*! \brief Implemented to enable per rule criteria value break down diagnostics
            ****************************************************************************************************/
            void GameProtocolVersionRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
            {
                setupInfos.push_back(RuleDiagnosticSetupInfo("protocolVersion", mRequiredGameProtocolVersionString.c_str()));   
            }

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
