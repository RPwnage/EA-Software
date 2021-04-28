/*! ************************************************************************************************/
/*!
\file propertyrule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PROPERTYRULE_H
#define BLAZE_GAMEMANAGER_PROPERTYRULE_H

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"

namespace Blaze
{

namespace Search {
class GameSessionSearchSlave;
}

namespace GameManager
{
class MatchmakingCreateGameRequest;
class ReadableRuleConditions;
class MatchmakingCriteriaError;
class MatchmakingAsyncStatus;

namespace Matchmaker
{
    class MatchmakingSession;
    struct MatchmakingSupplementalData;
    struct SessionsEvalInfo;
    struct SessionsMatchInfo;

     class PropertyRule : public Rule
     {
         NON_COPYABLE(PropertyRule);
     public:
 
         PropertyRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
         PropertyRule(const PropertyRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
         ~PropertyRule() override;
 
         bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
         bool initializeInternal(const MatchmakingFilterCriteriaMap& filterCriteriaMap, MatchmakingCriteriaError& err, bool packerSession = false);
         FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
         void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override {}
         FitScore evaluateForMMCreateGameFinalization(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const override { return 0; }

         /*! ************************************************************************************************/
         /*! \brief Clone the current rule copying the rule criteria using the copy ctor.  Used for subsessions.
         ****************************************************************************************************/
         Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

         /*! ************************************************************************************************/
         /*!
         \brief Returns the conditions block

         \return The current search conditions for this rule.
         *************************************************************************************************/
         bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;

         FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override
         {
             return 0;
         }
 
         bool isMatchAny() const override  { return false; }
 
         bool isDedicatedServerEnabled() const override { return false; }

         FitScore getMaxFitScore() const override { return 0; }
 
         bool isDisabled() const override { return false; }
         const PropertyRuleDefinition& getDefinition() const { return static_cast<const PropertyRuleDefinition&>(mRuleDefinition); }

         // From Condition Provider
         const char8_t* getProviderName() const override { return getRuleName(); }
         const uint32_t getProviderSalience() const override { return 0; } //= 0;
         const FitScore getProviderMaxFitScore() const override { return getMaxFitScore(); }

         bool isUsingUnset() const override { return mIsPackerSession; }
     
     protected:
         void updateAsyncStatus() override {}
         int32_t getLogCategory() const;

     private:
         MatchmakingFilterCriteriaMap mPropertyFilterMap;
         bool mIsPackerSession;
     };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PROPERTYRULE_H
