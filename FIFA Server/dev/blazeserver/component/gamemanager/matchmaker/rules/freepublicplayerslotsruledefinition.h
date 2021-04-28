/*! ************************************************************************************************/
/*!
    \file   freepublicplayerslotsruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE_DEFINITION
#define BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for a game's number of open public player slots will always fit the
        session's group size.  When doing matchmaking searches, this rule is internally always enabled to 
        ensure players can get into the games they are matchmaking. This rule is not exposed to clients.

    ***************************************************************************************************/
    class FreePublicPlayerSlotsRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(FreePublicPlayerSlotsRuleDefinition);
        DEFINE_RULE_DEF_H(FreePublicPlayerSlotsRuleDefinition, FreePublicPlayerSlotsRule);

    public:
        static const char8_t* FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY;

        FreePublicPlayerSlotsRuleDefinition();
        ~FreePublicPlayerSlotsRuleDefinition() override;

        bool isDisabled() const override { return false; }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }

        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override {return true;}

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

    private:
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE_DEFINITION
