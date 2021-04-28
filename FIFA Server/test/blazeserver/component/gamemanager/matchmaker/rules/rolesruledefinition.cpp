/*! ************************************************************************************************/
/*!
\file   rolesruledefinition.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/rolesrule.h"
#include "gamemanager/matchmaker/rules/rolesruledefinition.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticsgamecountutils.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(RolesRuleDefinition, "Predefined_RolesRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* RolesRuleDefinition::ROLES_RULE_ROLE_ATTRIBUTE_NAME = "RoleSize";

    RolesRuleDefinition::RolesRuleDefinition() 
        : mGlobalMaxTotalPlayerSlotsInGame(0)
    { 
    }

    RolesRuleDefinition::~RolesRuleDefinition() { }

    void RolesRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        buildAndInsertWorkingMemoryElements(gameSessionSlave, wmeManager);

    }
    
    void RolesRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
       cleanUpRemovedRoleWMEs(wmeManager, gameSessionSlave);
       buildAndInsertWorkingMemoryElements(gameSessionSlave, wmeManager);
    }

    bool RolesRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // rule does not have a configuration.
        uint32_t weight = 0;

        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();

        RuleDefinition::initialize(name, salience, weight);
        return true;
    }

    void RolesRuleDefinition::cleanUpRemovedRoleWMEs(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        RoleSizeMap& cachedAllowedRolesWMENameMap = const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getRoleRuleAllowedRolesWMENameMap();
        RoleSizeMap::const_iterator allowedIter = cachedAllowedRolesWMENameMap.begin();
        while (allowedIter != cachedAllowedRolesWMENameMap.end())
        {
            const RoleName& roleName = allowedIter->first;
            RoleCriteriaMap::const_iterator roleIter = gameSessionSlave.getRoleInformation().getRoleCriteriaMap().find(roleName);
            if (roleIter == gameSessionSlave.getRoleInformation().getRoleCriteriaMap().end())
            {
                char8_t roleWmeNameBuf[64];
                blaze_snzprintf(roleWmeNameBuf, sizeof(roleWmeNameBuf), "%s_%s", ROLES_RULE_ROLE_ATTRIBUTE_NAME, allowedIter->first.c_str());

                wmeManager.removeWorkingMemoryElement(gameSessionSlave.getGameId(), roleWmeNameBuf, allowedIter->second, *this);
                allowedIter = cachedAllowedRolesWMENameMap.erase(allowedIter);
            }
            else
            {
                ++allowedIter;
            }
        }
    }

    void RolesRuleDefinition::buildAndInsertWorkingMemoryElements( const Search::GameSessionSearchSlave &gameSessionSlave, GameManager::Rete::WMEManager &wmeManager ) const
    {
        RoleSizeMap& cachedAllowedRolesNameMap = const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getRoleRuleAllowedRolesWMENameMap();
        RoleCriteriaMap::const_iterator roleIter = gameSessionSlave.getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator roleEnd = gameSessionSlave.getRoleInformation().getRoleCriteriaMap().end();
        for (; roleIter != roleEnd; ++roleIter)
        {
            if (roleIter->first.empty())
            {
                continue;
            }
            // first create a WME name that contains the name of the role
            char8_t roleWmeNameBuf[64];
            blaze_snzprintf(roleWmeNameBuf, sizeof(roleWmeNameBuf), "%s_%s", ROLES_RULE_ROLE_ATTRIBUTE_NAME, roleIter->first.c_str());

            // Get team with most space for role
            uint16_t teamCount = gameSessionSlave.getTeamCount();
            uint16_t mostSpace = 0;

            for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                uint16_t roleSpace = gameSessionSlave.getRoleCapacity(roleIter->first) - gameSessionSlave.getPlayerRoster()->getRoleSize(teamIndex, roleIter->first);

                if (mostSpace < roleSpace)
                {
                    mostSpace = roleSpace;
                }
            }

            // cache off the WME name for later cleanup, in case it gets removed
            cachedAllowedRolesNameMap[roleIter->first] = mostSpace;

            // Range WMEm game adds most space for the role, and searchers look for space from group size to teamCapacity
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), roleWmeNameBuf, mostSpace, *this);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void RolesRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        cache.updateRolesGameCounts(gameSessionSlave, increment, *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
