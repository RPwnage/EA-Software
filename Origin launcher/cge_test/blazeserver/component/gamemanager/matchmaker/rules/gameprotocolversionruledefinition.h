/*! ************************************************************************************************/
/*!
\file   GameProtocolVersionRule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE_DEFINITION_H
#define BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE_DEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RuleDefinitionCollection;

    /*! ************************************************************************************************/
    /*!
       \brief This rule ensures no games with differing game protocol versions match.  
       
       This rule is automatically included in every MM/Gamebrowsing search.  It is exact match only
       and has no timing thresholds.
    *************************************************************************************************/
    class GameProtocolVersionRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameProtocolVersionRuleDefinition);
        DEFINE_RULE_DEF_H(GameProtocolVersionRuleDefinition, GameProtocolVersionRule);
    public:

        GameProtocolVersionRuleDefinition();
        ~GameProtocolVersionRuleDefinition() override;

        static const char8_t* GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME;

        //These functions have no 
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override {}

        //can't turn this one off
        bool isDisabled() const override { return false; }

        bool isReteCapable() const override { return true;}
        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        uint64_t getMatchAnyHash() const { return mMatchAnyHash; }
    private:
        uint64_t mMatchAnyHash;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAME_PROTOCOL_VERSION_RULE_DEFINITION_H
