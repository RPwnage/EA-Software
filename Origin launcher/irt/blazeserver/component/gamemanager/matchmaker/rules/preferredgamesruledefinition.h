/*! ************************************************************************************************/
/*!
    \file   PreferredGamesRuledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_PREFERRED_GAMES_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PREFERRED_GAMES_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class PreferredGamesRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(PreferredGamesRuleDefinition);
        DEFINE_RULE_DEF_H(PreferredGamesRuleDefinition, PreferredGamesRule);
    public:
        PreferredGamesRuleDefinition();
        ~PreferredGamesRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return false; }

        bool isDisabled() const override { return (mMaxGamesUsed == 0); }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[PreferredGamesRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED.");
        }
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[PreferredGamesRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED.");
        }

        uint32_t getMaxGamesUsed() const { return mMaxGamesUsed; }

    private:
        uint32_t mMaxGamesUsed;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE_DEFINITION
#endif

