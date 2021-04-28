/*! ************************************************************************************************/
/*!
    \file   avoidplayersruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE_DEFINITION
#define BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/playersruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class AvoidPlayersRuleDefinition : public GameManager::Matchmaker::PlayersRuleDefinition
    {
        NON_COPYABLE(AvoidPlayersRuleDefinition);
        DEFINE_RULE_DEF_H(AvoidPlayersRuleDefinition, AvoidPlayersRule);
    public:
        AvoidPlayersRuleDefinition();
        ~AvoidPlayersRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

    private:
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE_DEFINITION
#endif

