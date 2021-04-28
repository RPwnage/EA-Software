/*! ************************************************************************************************/
/*!
\file   rolesruledefinition.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_ROLES_RULE_DEFINITION_H
#define BLAZE_MATCHMAKING_ROLES_RULE_DEFINITION_H

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
       \brief This rule ensures that only games meeting that allow a GB/MM session's role requirements are considered a match. 
       
       This rule is automatically included in every MM/Gamebrowsing search.
    *************************************************************************************************/
    class RolesRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(RolesRuleDefinition);
        DEFINE_RULE_DEF_H(RolesRuleDefinition, RolesRule);
    public:
        RolesRuleDefinition();
        ~RolesRuleDefinition() override;

        static const char8_t* ROLES_RULE_ROLE_ATTRIBUTE_NAME;

        //These functions have no 
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override {}

        //can't turn this one off
        bool isDisabled() const override { return false; }

        bool isReteCapable() const override { return true;}
        bool callEvaluateForMMFindGameAfterRete() const override {return true;}
        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        void buildAndInsertWorkingMemoryElements( const Search::GameSessionSearchSlave &gameSessionSlave, GameManager::Rete::WMEManager &wmeManager ) const;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        uint16_t getGlobalMaxSlots() const { return mGlobalMaxTotalPlayerSlotsInGame; }

        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;

    private:

        void cleanUpRemovedRoleWMEs(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const;
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_ROLES_RULE_DEFINITION_H
