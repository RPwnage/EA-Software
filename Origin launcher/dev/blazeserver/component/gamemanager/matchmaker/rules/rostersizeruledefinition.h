/*! ************************************************************************************************/
/*!
    \file   rostersizeruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_ROSTER_SIZE_RULE_DEFINITION
#define BLAZE_MATCHMAKING_ROSTER_SIZE_RULE_DEFINITION


#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \brief Note: RosterSizeRule's has no 'Rule', instead its rule processing is implemented via PlayerCountRule
        (see for details). RosterSizeRuleDefinition class merely serves as a container
        for the original configuration values for populating matchmaking config response objects for clients,
        and for logging purposes.
    ***************************************************************************************************/
    class RosterSizeRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(RosterSizeRuleDefinition);
    public:

        static const char8_t* ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME;

        RosterSizeRuleDefinition();

        ~RosterSizeRuleDefinition() override {};

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        /*! ************************************************************************************************/
        /*! \brief write the configuration information for this rule into a MatchmakingRuleInfo TDF object
            \param[in\out] ruleConfig - rule configuration object to pack with data for this rule
        *************************************************************************************************/
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        // this rule is disabled from rete and is implemented via PlayerCountRule.
        bool isReteCapable() const override { return false; }
        // always enabled in the definition.  The rule can only be not used by setting max value to max uint
        bool isDisabled() const override { return false; }
        bool isDisabledForRuleCreation() const override { return true; }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        // Special Case Exception code below, because RosterSizeRule no longer exists (now implemented via PlayerCountRule) and
        // we cannot use DEFINE_RULE_DEF_H directly here
        Rule* createRule(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;
        typedef void* mRuleType; 
        static RuleDefinitionId getRuleDefinitionId() {return mRuleDefinitionId;} /* Not valid with Multi-type rules */ 
        static const char* getConfigKey() {return mRuleDefConfigKey;} 
        static RuleDefinitionType getRuleDefType() {return mRuleDefType; } 
        const char8_t* getType() const override { return getConfigKey(); } 
        const char8_t* getName() const override { return (getRuleDefType() != RULE_DEFINITION_TYPE_MULTI) ? getConfigKey() : mName; } 
        void setID(RuleDefinitionId id) override 
        { 
            RuleDefinition::setID(id); 
            if (getRuleDefType() != RULE_DEFINITION_TYPE_MULTI) mRuleDefinitionId = id; 
        }
    private:
        // Special Case Exception code below, because RosterSizeRule no longer exists (now implemented via PlayerCountRule) and
        // we cannot use DEFINE_RULE_DEF_H directly here
        friend struct RosterSizeRuleDefinitionHelper; 
        static EA_THREAD_LOCAL RuleDefinitionId mRuleDefinitionId; /* Not valid with Multi-type rules */ 
        static EA_THREAD_LOCAL const char* mRuleDefConfigKey; 
        static EA_THREAD_LOCAL RuleDefinitionType mRuleDefType; 
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_ROSTER_SIZE_RULE_DEFINITION
