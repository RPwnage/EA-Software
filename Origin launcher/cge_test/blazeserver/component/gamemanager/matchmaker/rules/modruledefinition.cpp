#include "framework/blaze.h"
#include "modrule.h"
#include "modruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticsgamecountutils.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    // The Mod rule does not load any configuration values 
    DEFINE_RULE_DEF_CPP(ModRuleDefinition, "modRule", RULE_DEFINITION_TYPE_PRIORITY);

    ModRuleDefinition::ModRuleDefinition() 
    {
    }

    ModRuleDefinition::~ModRuleDefinition() 
    {
    }

    bool ModRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // rule does not have a configuration.
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);

        for (size_t i = 0; i < sizeof(GameModRegister) * 8; i++)
        {
            mWMEAttributeNameForBit[i].append_sprintf("ATTR_BIT_%d", i + 1);
        }

        return true;
    }

    void ModRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

    void ModRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const Rete::WMEAttribute WME_ATTR_BOOLEAN_FALSE = getWMEBooleanAttributeValue(false);
        const Rete::WMEAttribute WME_ATTR_BOOLEAN_TRUE = getWMEBooleanAttributeValue(true);

        GameModRegister serverMods = gameSessionSlave.getGameModRegister();

        for (size_t bit = 0; bit < sizeof(GameModRegister) * 8; bit++)
        {
            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
                 mWMEAttributeNameForBit[bit].c_str(), ((serverMods >> bit) & 0x1) ? WME_ATTR_BOOLEAN_TRUE : WME_ATTR_BOOLEAN_FALSE, *this);
        }
    }

    void ModRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const Rete::WMEAttribute WME_ATTR_BOOLEAN_FALSE = getWMEBooleanAttributeValue(false);
        const Rete::WMEAttribute WME_ATTR_BOOLEAN_TRUE = getWMEBooleanAttributeValue(true);

        GameModRegister serverMods = gameSessionSlave.getGameModRegister();

        for (size_t bit = 0; bit < sizeof(GameModRegister) * 8; bit++)
        {
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
                 mWMEAttributeNameForBit[bit].c_str(), ((serverMods >> bit) & 0x1) ? WME_ATTR_BOOLEAN_TRUE : WME_ATTR_BOOLEAN_FALSE, *this);
        }
    }

    const eastl::string& ModRuleDefinition::getWMEAttributeNameForBit(int bit) const
    {
        EA_ASSERT((size_t)bit < sizeof(mWMEAttributeNameForBit)/sizeof(*mWMEAttributeNameForBit));

        return mWMEAttributeNameForBit[bit];
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void ModRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        cache.updateModsGameCounts(gameSessionSlave, increment, *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
