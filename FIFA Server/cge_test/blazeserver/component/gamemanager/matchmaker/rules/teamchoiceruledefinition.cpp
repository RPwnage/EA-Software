/*! ************************************************************************************************/
/*!
    \file teamchoiceruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/teamchoiceruledefinition.h"
#include "gamemanager/matchmaker/rules/teaminfohelper.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rules/teamchoicerule.h"

#include "gamemanager/tdf/matchmaker_config_server.h"

#include <math.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamChoiceRuleDefinition, "Predefined_TeamChoiceRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* TeamChoiceRuleDefinition::TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY = "Team[%u]";
    const char8_t* TeamChoiceRuleDefinition::TEAMCHOICERULE_DUPLICATE_RETE_KEY = "DuplicateFactions";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamChoiceRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamChoiceRuleDefinition::TeamChoiceRuleDefinition()
        : RuleDefinition(), mMaxGameSize(0)
    {
    }

    TeamChoiceRuleDefinition::~TeamChoiceRuleDefinition()
    {}

    bool TeamChoiceRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 0);
        mMaxGameSize = matchmakingServerConfig.getGameSizeRange().getMax();
        cacheWMEAttributeName(TEAMCHOICERULE_DUPLICATE_RETE_KEY);
        return true;
    }

    void TeamChoiceRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void TeamChoiceRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty())
        {
            matchmakingCache.updateCachedTeamIds(gameSession.getTeamIds());
        }
    }

    void TeamChoiceRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // no rule-specific config values for team CHOICE rule
    }


    void TeamChoiceRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // The sorted teams vector on the game is only on the cache.
        const MatchmakingGameInfoCache::TeamIdInfoVector &gameTeamIdInfoVector = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedTeamIdInfoVector();
        if (gameTeamIdInfoVector.size() == 0)
        {
            TRACE_LOG("[TeamChoiceRuleDefinition] Not adding WME for game(" << gameSessionSlave.getGameId() << "), does not contain teams.");
            return;
        }

        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());
        TeamChoiceWMEMap& cachedNameMap = const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getCachedTeamChoiceRuleTeams();
        // The WME's to be removed at the end of all this (if they no longer exist in the game)
        TeamChoiceWMEMap removeNameMap(cachedNameMap);

        uint16_t maxUnsetTeamFreeSpace = 0;
        uint16_t maxFreeSpace = 0;
        uint16_t teamCapacity = gameSessionSlave.getTeamCapacity();
        uint16_t teamCount = gameSessionSlave.getTeamCount();
        char8_t wmeNameBuf[32];

        // add a WME for each existing team in the game.
        for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t teamSize = gameRoster.getTeamSize(teamIndex);
            uint16_t teamFreeSpace = teamCapacity - teamSize;

            // guard to prevent excess WMEs from being added to the RETE tree if teamSize is ever inadvertently greater than capacity
            if ( EA_UNLIKELY(teamCapacity < teamSize))
            {
                teamFreeSpace = 0;
            }

            uint16_t curTeamId = gameSessionSlave.getTeamIdByIndex(teamIndex);

            // Max free space if we don't care about our team id:
            maxFreeSpace = eastl::max(maxFreeSpace, teamFreeSpace);

            // Set the Team[#]Size to be all sizes at or below the current free space: (Values are [0 -> FreeSpace])
            if (curTeamId != ANY_TEAM_ID && curTeamId != UNSET_TEAM_ID)
            {
                blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, curTeamId);
                if (cachedNameMap.find(wmeNameBuf) != cachedNameMap.end())
                {
                    wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), wmeNameBuf, teamFreeSpace, *this);
                    removeNameMap.erase(wmeNameBuf);
                }
                else
                {
                    wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), wmeNameBuf, teamFreeSpace, *this);
                }
                cachedNameMap[wmeNameBuf] = teamFreeSpace;
            }
            else
            {   // Max free space if we care about our team id:
                maxUnsetTeamFreeSpace = eastl::max(maxUnsetTeamFreeSpace, teamFreeSpace);
            }
        }

        // The UNSET team id is for people WITH a TeamId, that are willing to join a team, as long as they can SET the TeamId 
        if (maxUnsetTeamFreeSpace != 0)     // We don't set [0] if no one can join the unset team
        {
            blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, UNSET_TEAM_ID);
            
            if (cachedNameMap.find(wmeNameBuf) != cachedNameMap.end())
            {
                wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), wmeNameBuf, maxUnsetTeamFreeSpace, *this);
                removeNameMap.erase(wmeNameBuf);
            }
            else
            {
               wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), wmeNameBuf, maxUnsetTeamFreeSpace, *this);
            }
            cachedNameMap[wmeNameBuf] = maxUnsetTeamFreeSpace;
        }

        // The ANY team id is for people WITHOUT a TeamId, that are willing to join ANY team
        blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, ANY_TEAM_ID);
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), wmeNameBuf, maxFreeSpace, *this);

        // Duplicate factions allowed
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            TEAMCHOICERULE_DUPLICATE_RETE_KEY, getWMEBooleanAttributeValue(gameSessionSlave.getGameSettings().getAllowSameTeamId()), *this);

        // now go through and remove an WMEs that weren't upserted from the previous attempt.
        TeamChoiceWMEMap::const_iterator iter = removeNameMap.begin();
        for (; iter != removeNameMap.end(); ++iter)
        {
            cachedNameMap.erase(iter->first);
            wmeManager.removeWorkingMemoryElement(gameSessionSlave.getGameId(), (iter->first).c_str(), iter->second, *this);
        }
    }
    
    void TeamChoiceRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        insertWorkingMemoryElements(wmeManager, gameSessionSlave);
    }

    /*! ************************************************************************************************/
    /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
                            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void TeamChoiceRuleDefinition::toString(eastl::string &dest, const char8_t* prefix /*=""*/) const
    {
        RuleDefinition::toString(dest, prefix);
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void TeamChoiceRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        cache.updateTeamIdsGameCounts(gameSessionSlave, increment, *this);
    }



} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
