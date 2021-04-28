/*! ************************************************************************************************/
/*!
    \file   freeplayerslotsruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/matchmaker/rules/freeplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/freeplayerslotsrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(FreePlayerSlotsRuleDefinition, "Predefined_FreePlayerSlotsRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* FreePlayerSlotsRuleDefinition::FREESLOTSRULE_SIZE_RETE_KEY = "freeParticipantSlotsSize";

    FreePlayerSlotsRuleDefinition::FreePlayerSlotsRuleDefinition()
        : mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    bool FreePlayerSlotsRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();
        if (mGlobalMaxTotalPlayerSlotsInGame == 0)
        {
            ERR_LOG("[FreePlayerSlotsRuleDefinition] configured MaxTotalPlayerSlotsInGame cannot be 0.");
            return false;
        }

        // rule does not have configuration
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);
        cacheWMEAttributeName(FREESLOTSRULE_SIZE_RETE_KEY);
        return true;
    }

    void FreePlayerSlotsRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void FreePlayerSlotsRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());

        uint16_t totalOpenParticipantSlots = (gameSessionSlave.getTotalParticipantCapacity() - gameRoster.getParticipantCount());

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            FREESLOTSRULE_SIZE_RETE_KEY, totalOpenParticipantSlots, *this);
    }

    void FreePlayerSlotsRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());
        uint16_t totalOpenParticipantSlots = (gameSessionSlave.getTotalParticipantCapacity() - gameRoster.getParticipantCount());

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            FREESLOTSRULE_SIZE_RETE_KEY, totalOpenParticipantSlots, *this);
        
    }

    void FreePlayerSlotsRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // rule only has configurable salience
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
