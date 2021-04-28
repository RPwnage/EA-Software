/*! ************************************************************************************************/
/*!
    \file   preferredplayersruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/playersruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class PreferredPlayersRuleDefinition : public GameManager::Matchmaker::PlayersRuleDefinition
    {
        NON_COPYABLE(PreferredPlayersRuleDefinition);
        DEFINE_RULE_DEF_H(PreferredPlayersRuleDefinition, PreferredPlayersRule);
    public:
        PreferredPlayersRuleDefinition();
        ~PreferredPlayersRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

    private:
        
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE_DEFINITION
#endif

