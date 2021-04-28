/*! ************************************************************************************************/
/*!
    \file   freeplayerslotsruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE_DEFINITION
#define BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \brief FreePlayerSlotsRule specifies the number of empty slots required for a game to match.
    ***************************************************************************************************/
    class FreePlayerSlotsRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(FreePlayerSlotsRuleDefinition);
        DEFINE_RULE_DEF_H(FreePlayerSlotsRuleDefinition, FreePlayerSlotsRule);
    public:

        static const char8_t* FREESLOTSRULE_SIZE_RETE_KEY;

        FreePlayerSlotsRuleDefinition();
        ~FreePlayerSlotsRuleDefinition() override {};

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return true; }

        // always enabled in the definition.  The rule can only be not used by setting max value to max uint
        bool isDisabled() const override { return false; }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }
    private:
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE_DEFINITION
