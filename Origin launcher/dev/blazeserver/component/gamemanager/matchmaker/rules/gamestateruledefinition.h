/*! ************************************************************************************************/
/*!
\file gamestateruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMESTATERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_GAMESTATERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
namespace GameManager
{

namespace Matchmaker
{
    class GameStateRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameStateRuleDefinition);
        DEFINE_RULE_DEF_H(GameStateRuleDefinition, GameStateRule);
    public:
        static const char8_t* GAMESTATERULE_RETE_KEY;

        GameStateRuleDefinition();
        ~GameStateRuleDefinition() override;

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        //////////////////////////////////////////////////////////////////////////
        // from RuleDefinition
        //////////////////////////////////////////////////////////////////////////
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        bool isReteCapable() const override { return true; }

        bool isDisabled() const override { return false; }

        const char8_t* getGameStateAttributeNameStr() const { return GAMESTATERULE_RETE_KEY; }
        Rete::WMEAttribute getGameStateAttributeValue(const GameManager::GameState& gameState) const;
        Rete::WMEAttribute getNoMatchAttributeValue() const;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMESTATERULEDEFINITION_H
