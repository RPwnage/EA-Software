/*! ************************************************************************************************/
/*!
    \file   virtualgameruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/virtualgameruledefinition.h"
#include "gamemanager/matchmaker/rules/virtualgamerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(VirtualGameRuleDefinition, "Predefined_VirtualGameRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* VirtualGameRuleDefinition::VIRTUALRULE_RETE_KEY = "virtualGame";

    VirtualGameRuleDefinition::VirtualGameRuleDefinition()
        : mMatchingFitPercent(1.0f),
          mMismatchFitPercent(0.5f)
    {}

    VirtualGameRuleDefinition::~VirtualGameRuleDefinition()
    {}

    bool VirtualGameRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const VirtualGameRuleConfig& virtualGameRuleConfig = matchmakingServerConfig.getRules().getPredefined_VirtualGameRule();

        if(!virtualGameRuleConfig.isSet())
        {
            WARN_LOG("[VirtualGameRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // parses common weight, salience, minFitThresholdList (blaze3.03)
        if (!RuleDefinition::initialize(name, salience, virtualGameRuleConfig.getWeight(), virtualGameRuleConfig.getMinFitThresholdLists()))
        {
            return false;
        }

        cacheWMEAttributeName(VIRTUALRULE_RETE_KEY);

        mMismatchFitPercent = virtualGameRuleConfig.getMismatchFitPercent();
        mMatchingFitPercent = virtualGameRuleConfig.getMatchingFitPercent();
        return true;
    }

    void VirtualGameRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix /* = "" */) const
    {

    }

    void VirtualGameRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {

        // Find the setting for this rule to insert
        const VirtualGameRulePrefs::VirtualGameDesiredValue ranked = gameSessionSlave.getGameSettings().getVirtualized()?
            VirtualGameRulePrefs::VIRTUALIZED : VirtualGameRulePrefs::STANDARD;

        // insert
        const char8_t* value = VirtualGameRulePrefs::VirtualGameDesiredValueToString(ranked);
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            VIRTUALRULE_RETE_KEY, RuleDefinition::getWMEAttributeValue(value), *this);
    }

    void VirtualGameRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Find the setting for this rule to insert
        const VirtualGameRulePrefs::VirtualGameDesiredValue ranked = gameSessionSlave.getGameSettings().getVirtualized()?
            VirtualGameRulePrefs::VIRTUALIZED : VirtualGameRulePrefs::STANDARD;

        // insert
        const char8_t* value = VirtualGameRulePrefs::VirtualGameDesiredValueToString(ranked);
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            VIRTUALRULE_RETE_KEY, RuleDefinition::getWMEAttributeValue(value), *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
