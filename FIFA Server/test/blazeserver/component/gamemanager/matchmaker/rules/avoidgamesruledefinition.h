/*! ************************************************************************************************/
/*!
    \file   avoidgamesruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_AVOID_GAMES_RULE_DEFINITION
#define BLAZE_MATCHMAKING_AVOID_GAMES_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief The AvoidGamesRule definition
    ***************************************************************************************************/
    class AvoidGamesRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(AvoidGamesRuleDefinition);
        DEFINE_RULE_DEF_H(AvoidGamesRuleDefinition, AvoidGamesRule);
    public:
        AvoidGamesRuleDefinition();
        ~AvoidGamesRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return false; } //MM_AUDIT: consider researching a rete implementation

        bool isDisabled() const override { return (mMaxGameIdListSize == 0); }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[AvoidGamesRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED.");
        }
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[AvoidGamesRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED.");
        }

        uint32_t getMaxGameIdListSize() const { return mMaxGameIdListSize; }

    private:
        uint32_t mMaxGameIdListSize;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_AVOID_GAMES_RULE_DEFINITION
#endif

