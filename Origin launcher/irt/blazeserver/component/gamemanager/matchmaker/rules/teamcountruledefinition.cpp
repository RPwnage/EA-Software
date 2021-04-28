/*! ************************************************************************************************/
/*!
    \file teamCOUNTruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/teamcountruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rules/teamcountrule.h"

#include "gamemanager/tdf/matchmaker_config_server.h"

#include <math.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamCountRuleDefinition, "Predefined_TeamCountRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* TeamCountRuleDefinition::MAX_NUM_TEAMS_IN_GAME_KEY = "maxNumTeamsInGame";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamCountRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamCountRuleDefinition::TeamCountRuleDefinition()
        : RuleDefinition()
    {
    }

    TeamCountRuleDefinition::~TeamCountRuleDefinition()
    {}

    bool TeamCountRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 0);
        return true;
    }

    void TeamCountRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void TeamCountRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty())
        {
            matchmakingCache.updateCachedTeamIds(gameSession.getTeamIds());
        }
    }

    void TeamCountRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // no rule-specific config values for team COUNT rule
    }

    void TeamCountRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // The sorted teams vector on the game is only on the cache.
        const MatchmakingGameInfoCache::TeamIdInfoVector &gameTeamIdInfoVector = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedTeamIdInfoVector();
        if (gameTeamIdInfoVector.size() == 0)
        {
            TRACE_LOG("[TeamCountRuleDefinition] Not adding WME's for game(" << gameSessionSlave.getGameId() << "), does not contain teams.");
            return;
        }

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            MAX_NUM_TEAMS_IN_GAME_KEY, getWMEUInt16AttributeValue(gameSessionSlave.getTeamCount()), *this);
    }
    
    void TeamCountRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            MAX_NUM_TEAMS_IN_GAME_KEY, getWMEUInt16AttributeValue(gameSessionSlave.getTeamCount()), *this);
    }

    /*! ************************************************************************************************/
    /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
                            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void TeamCountRuleDefinition::toString(eastl::string &dest, const char8_t* prefix /*=""*/) const
    {
        RuleDefinition::toString(dest, prefix);
    }



} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
