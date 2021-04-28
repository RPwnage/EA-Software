/*! ************************************************************************************************/
/*!
    \file   reputationruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/reputationruledefinition.h"
#include "gamemanager/matchmaker/rules/reputationrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(ReputationRuleDefinition, "Predefined_ReputationRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* ReputationRuleDefinition::REPUTATIONRULE_ALLOW_ANY_REPUTATION_RETE_KEY = "allowsAnyReputation";
    const char8_t* ReputationRuleDefinition::REPUTATIONRULE_REQUIRE_GOOD_REPUTATION_RETE_KEY = "requiresGoodReputation";
    const char8_t* ReputationRuleDefinition::REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY = "Reputation";

    ReputationRuleDefinition::ReputationRuleDefinition() : mDisabled(false)
    {
    }

    ReputationRuleDefinition::~ReputationRuleDefinition()
    {}

    bool ReputationRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        mDisabled = gUserSessionManager->isReputationDisabled();

        // rule does not have a configuration.
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);
        return true;
    }

    void ReputationRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix /* = "" */) const
    {

    }

    void ReputationRuleDefinition::insertWorkingMemoryElements( GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {

        const char8_t* reputationCheckSetting = gameSessionSlave.getGameSettings().getAllowAnyReputation() ? REPUTATIONRULE_ALLOW_ANY_REPUTATION_RETE_KEY 
                                                                          : REPUTATIONRULE_REQUIRE_GOOD_REPUTATION_RETE_KEY;

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
              REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY, RuleDefinition::getWMEAttributeValue(reputationCheckSetting), *this);
    }

    void ReputationRuleDefinition::upsertWorkingMemoryElements( GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave ) const
    {

        const char8_t* reputationCheckSetting = gameSessionSlave.getGameSettings().getAllowAnyReputation() ? REPUTATIONRULE_ALLOW_ANY_REPUTATION_RETE_KEY 
            : REPUTATIONRULE_REQUIRE_GOOD_REPUTATION_RETE_KEY;

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY, RuleDefinition::getWMEAttributeValue(reputationCheckSetting), *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
