/*! ************************************************************************************************/
/*!
    \file teamminsizeruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/teamminsizeruledefinition.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rules/teamminsizerule.h"

#include "gamemanager/tdf/matchmaker_config_server.h"

#include <math.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamMinSizeRuleDefinition, "Predefined_TeamMinSizeRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* TeamMinSizeRuleDefinition::MIN_TEAM_SIZE_IN_GAME_KEY = "minTeamSizeInGame";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamMinSizeRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamMinSizeRuleDefinition::TeamMinSizeRuleDefinition()
        : RuleDefinition(),
        mFitFormula(nullptr),
        mGlobalMaxTotalPlayerSlotsInGame(0),
        mGlobalMinTotalPlayerSlotsInGame(0)
    {
    }

    TeamMinSizeRuleDefinition::~TeamMinSizeRuleDefinition()
    {
        delete mFitFormula;
    }

    bool TeamMinSizeRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        mGlobalMinTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMin();
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();
        if (mGlobalMaxTotalPlayerSlotsInGame == 0)
        {
            ERR_LOG("[TeamMinSizeRuleDefinition] configured MaxTotalPlayerSlotsInGame cannot be 0.");
            return false;
        }

        const TeamMinSizeRuleConfig& teamMinSizeRuleConfig = matchmakingServerConfig.getRules().getPredefined_TeamMinSizeRule();
        if (!teamMinSizeRuleConfig.isSet())
        {
            WARN_LOG("[TeamMinSizeRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }
        else
        {
            mFitFormula = FitFormula::createFitFormula(teamMinSizeRuleConfig.getFitFormula().getName());
            if (mFitFormula == nullptr)
            {
                ERR_LOG("[TeamMinSizeRuleDefinition].create TeamMinSizeRuleDefinition mFitFormula failed. The fit formula name is " << teamMinSizeRuleConfig.getFitFormula().getName());
                return false;
            }

            if (!mFitFormula->initialize(teamMinSizeRuleConfig.getFitFormula(), &teamMinSizeRuleConfig.getRangeOffsetLists()))
            {
                ERR_LOG("[TeamMinSizeRuleDefinition].initalize TeamMinSizeRuleDefinition mFitFormula failed.");
                return false;
            }

            RuleDefinition::initialize(name, salience, teamMinSizeRuleConfig.getWeight());
            return mRangeListContainer.initialize(getName(), teamMinSizeRuleConfig.getRangeOffsetLists());
        }
    }

    void TeamMinSizeRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
        mRangeListContainer.initRangeListInfo(predefinedRuleConfig.getThresholdNames());
    }

    void TeamMinSizeRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty())
        {
            matchmakingCache.updateCachedTeamIds(gameSession.getTeamIds());
        }
    }

    const RangeList* TeamMinSizeRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    void TeamMinSizeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // no rule-specific config values for team MINSIZE rule
    }

    void TeamMinSizeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // The sorted teams vector on the game is only on the cache.
        const MatchmakingGameInfoCache::TeamIdInfoVector &gameTeamIdInfoVector = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedTeamIdInfoVector();
        if (gameTeamIdInfoVector.size() == 0)
        {
            TRACE_LOG("[TeamCountRuleDefinition] Not adding WME for game(" << gameSessionSlave.getGameId() << "), does not contain teams.");
            return;
        }

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            MIN_TEAM_SIZE_IN_GAME_KEY, getWMEUInt16AttributeValue(calcMinTeamSize(gameSessionSlave)), *this);
    }
    
    uint16_t TeamMinSizeRuleDefinition::calcMinTeamSize(const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());

        // WE find the two teams with the smallest sizes:
        // Then we will check for smallest < minsize.
        uint16_t minTeamSize = mGlobalMaxTotalPlayerSlotsInGame;
        uint16_t teamCount = gameSessionSlave.getTeamCount();
        for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t curTeamSize = gameRoster.getTeamSize(teamIndex);
            if (curTeamSize < minTeamSize)
                minTeamSize = curTeamSize;
        }
        return minTeamSize;
    }

    void TeamMinSizeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            MIN_TEAM_SIZE_IN_GAME_KEY, getWMEUInt16AttributeValue(calcMinTeamSize(gameSessionSlave)), *this);
    }

    /*! ************************************************************************************************/
    /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
                            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void TeamMinSizeRuleDefinition::toString(eastl::string &dest, const char8_t* prefix /*=""*/) const
    {
        RuleDefinition::toString(dest, prefix);
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }


    float TeamMinSizeRuleDefinition::getFitPercent(uint16_t desiredValue, uint16_t actualValue) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(desiredValue, actualValue);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
