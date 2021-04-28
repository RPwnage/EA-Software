/*! ************************************************************************************************/
/*!
\file gametyperuledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMETYPERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_GAMETYPERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
namespace GameManager
{

namespace Matchmaker
{
    class GameTypeRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameTypeRuleDefinition);
        DEFINE_RULE_DEF_H(GameTypeRuleDefinition, GameTypeRule);
   public:
        static const char8_t* GAMETYPERULE_RETE_KEY;

        GameTypeRuleDefinition();
        ~GameTypeRuleDefinition() override;

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

        const char8_t* getGameReportNameStr() const { return GAMETYPERULE_RETE_KEY; }
        Rete::WMEAttribute getGameTypeValue(const GameManager::GameType& gameType) const;
        Rete::WMEAttribute getNoMatchValue() const;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMETYPERULEDEFINITION_H
