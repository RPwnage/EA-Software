#ifndef BLAZE_MATCHMAKING_MOD_RULE_DEFINITION_H
#define BLAZE_MATCHMAKING_MOD_RULE_DEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class DiagnosticsSearchSlaveHelper;

    class RuleDefinitionCollection;

    class ModRuleDefinition : public RuleDefinition
    {

        DEFINE_RULE_DEF_H(ModRuleDefinition, ModRule);
    public:

        ModRuleDefinition();

        ~ModRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override 
        { 
            return true;
        }

        //can't turn this one off
        bool isDisabled() const override { return false; }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        
        const eastl::string& getWMEAttributeNameForBit(int bit) const;

        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;

    private:
        NON_COPYABLE(ModRuleDefinition);

        eastl::string mWMEAttributeNameForBit[sizeof(GameModRegister) * 8];
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MOD_RULE_DEFINITION_H
