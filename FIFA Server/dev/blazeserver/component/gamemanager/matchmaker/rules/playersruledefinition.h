/*! ************************************************************************************************/
/*!
    \file   playersruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_PLAYERS_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PLAYERS_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class PlayersRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(PlayersRuleDefinition);
    public:
        PlayersRuleDefinition();
        ~PlayersRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        bool isReteCapable() const override { return mPlayersRulesUseRete; }

        virtual bool isDisabled() const override { return (mMaxPlayersUsed == 0); }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            if (EA_UNLIKELY(!isReteCapable()))
            { // this is a no-op, as we do inserts and upserts differently than other rete rules, but log an error if we got here and weren't rete-enabled.
                ERR_LOG("[PlayersRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED.");
            }
        }
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            if (EA_UNLIKELY(!isReteCapable()))
            { // this is a no-op, as we do inserts and upserts differently than other rete rules, but log an error if we got here and weren't rete-enabled.
                ERR_LOG("[PlayersRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED.");
            }
        }

        uint32_t getMaxPlayersUsed() const { return mMaxPlayersUsed; }
        void setMaxPlayersUsed(uint32_t playersUsed) { mMaxPlayersUsed = playersUsed; }

        void updateWMEOnPlayerAction(GameManager::Rete::WMEManager& wmeManager, const char8_t *wmeName,
            GameManager::GameId gameId, bool playerAdded) const;

        void removeAllWMEsForRuleInstance(GameManager::Rete::ReteNetwork& reteNetwork, const char8_t *wmeName) const;

        
        void generateRuleInstanceWmeName(eastl::string& wmeName, UserSessionId ownerSessionId) const;


    private:
        uint32_t mMaxPlayersUsed;
        bool mPlayersRulesUseRete;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PLAYERS_RULE_DEFINITION
#endif

