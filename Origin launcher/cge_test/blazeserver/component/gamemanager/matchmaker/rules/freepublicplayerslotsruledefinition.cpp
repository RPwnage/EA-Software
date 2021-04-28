/*! ************************************************************************************************/
/*!
    \file   freepublicplayerslotsruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/freepublicplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/freepublicplayerslotsrule.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX

#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(FreePublicPlayerSlotsRuleDefinition, "Predefined_FreePublicPlayerSlotsRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* FreePublicPlayerSlotsRuleDefinition::FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY = "publicSlots";

    FreePublicPlayerSlotsRuleDefinition::FreePublicPlayerSlotsRuleDefinition()
        : mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    FreePublicPlayerSlotsRuleDefinition::~FreePublicPlayerSlotsRuleDefinition() 
    {
    }

    bool FreePublicPlayerSlotsRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();

        if (mGlobalMaxTotalPlayerSlotsInGame == 0)
        {
            ERR_LOG("[FreePublicPlayerSlotsRuleDefinition] configured MaxTotalPlayerSlotsInGame cannot be 0.");
            return false;
        }

        // parses common configuration.  0 weight, this rule does not effect the final result.
        RuleDefinition::initialize(name, salience, 0);

        cacheWMEAttributeName(FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY);

        return true;
    }

    void FreePublicPlayerSlotsRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // no op, this rule has no config
    }

    void FreePublicPlayerSlotsRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());
   
        //we disregard the size of the queue, as with teams and roles, it's possible that queued players are waiting for a specific slot, that's not currently open.
        //game manager will handle determining if that is the case.
        uint16_t totalOpenPublicParticipantSlots = gameSessionSlave.getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT) - gameRoster.getPlayerCount(SLOT_PUBLIC_PARTICIPANT); 

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY, totalOpenPublicParticipantSlots, *this);
    }

    void FreePublicPlayerSlotsRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());

        //we disregard the size of the queue, as with teams and roles, it's possible that queued players are waiting for a specific slot, that's not currently open.
        //game manager will handle determining if that is the case.
        uint16_t totalOpenPublicSlots = gameSessionSlave.getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT) - gameRoster.getPlayerCount(SLOT_PUBLIC_PARTICIPANT); 

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY, totalOpenPublicSlots, *this);
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
