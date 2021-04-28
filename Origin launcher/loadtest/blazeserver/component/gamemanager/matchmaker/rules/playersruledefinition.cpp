/*! ************************************************************************************************/
/*!
    \file   playersruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/playersruledefinition.h"
#include "gamemanager/matchmaker/rules/playersrule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    PlayersRuleDefinition::PlayersRuleDefinition() 
        : mMaxPlayersUsed(0),
          mPlayersRulesUseRete(true)
    {
    }

    PlayersRuleDefinition::~PlayersRuleDefinition()
    {
    }

    bool PlayersRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        mPlayersRulesUseRete = matchmakingServerConfig.getRunPlayerRulesInRete();

        return true;
    }

    void PlayersRuleDefinition::generateRuleInstanceWmeName(eastl::string& wmeName, UserSessionId ownerSessionId) const
    {
        if (wmeName.empty())
        {
            // don't have the id within the rule (we may be in the browser!), using timestamp as temporary hack
            uint64_t timestamp = TimeValue::getTimeOfDay().getMicroSeconds();
            wmeName.clear();
            wmeName.append_sprintf("%" PRIu64 "_%" PRIu64, ownerSessionId, timestamp);
        }
        else
        {
            ERR_LOG("[PlayersRule].setWmeName was called multiple times for the same instance of the " << getName() << " rule.");
        }
    }

    void PlayersRuleDefinition::updateWMEOnPlayerAction(GameManager::Rete::WMEManager& wmeManager, const char8_t *wmeName,
        GameManager::GameId gameId, bool playerAdded) const
    {
        if ((wmeName == nullptr) || (wmeName[0] == '\0'))
        {
            ERR_LOG("[PlayersRuleDefinition].updateWMEOnPlayerAction: called without wmeName for rule '" << getName() << "'.");
            return;
        }

        if (EA_LIKELY(mPlayersRulesUseRete))
        {
            if (playerAdded)
                wmeManager.upsertWorkingMemoryElement(gameId, wmeName, getWMEBooleanAttributeValue(true), *this);
            else
                wmeManager.removeWorkingMemoryElement(gameId, wmeName, getWMEBooleanAttributeValue(true), *this);
        }
    }

    void PlayersRuleDefinition::removeAllWMEsForRuleInstance(GameManager::Rete::ReteNetwork& reteNetwork, const char8_t *wmeName) const
    {
        if ((wmeName == nullptr) || (wmeName[0] == '\0'))
        {
            ERR_LOG("[PlayersRuleDefinition].removeAllWMEsForRuleInstance: called without wmeName for rule '" << getName() << "'.");
            return;
        }

        if (mPlayersRulesUseRete)
        {
            reteNetwork.getAlphaNetwork().removeAlphaMemoryProvider(GameManager::Rete::Condition::getWmeName(wmeName, *this), getWMEBooleanAttributeValue(true));
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

