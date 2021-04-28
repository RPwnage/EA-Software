/*! ************************************************************************************************/   
/*!
    \file   DedicatedServerPlayerCapacityRuleDefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_DEDICATED_SERVER_PLAYER_CAPACITY_RULE_DEFINITION
#define BLAZE_MATCHMAKING_DEDICATED_SERVER_PLAYER_CAPACITY_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for finding a dedicated server based on the needed capacity.
        
        The rule uses a linear fit threshold, such that searches will prefer finding games that are near the max capacity.
    ***************************************************************************************************/
    class DedicatedServerPlayerCapacityRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(DedicatedServerPlayerCapacityRuleDefinition);
        DEFINE_RULE_DEF_H(DedicatedServerPlayerCapacityRuleDefinition, DedicatedServerPlayerCapacityRule);
    public:
        static const char8_t* MIN_CAPACITY_RETE_KEY;
        static const char8_t* MAX_CAPACITY_RETE_KEY;
        static const char8_t* GAME_NETWORK_TOPOLOGY_RETE_KEY;

        DedicatedServerPlayerCapacityRuleDefinition();

        //! \brief destructor
        ~DedicatedServerPlayerCapacityRuleDefinition() override;

        bool isDisabled() const override { return false; }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        //
        // From GameReteRuleDefinition
        //

        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override { return true; }
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        // End from GameReteRuleDefintion
        uint16_t getMaxTotalPlayerSlots() const { return mGlobalMaxTotalPlayerSlotsInGame; }

    private:
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_DEDICATED_SERVER_PLAYER_CAPACITY_RULE_DEFINITION
