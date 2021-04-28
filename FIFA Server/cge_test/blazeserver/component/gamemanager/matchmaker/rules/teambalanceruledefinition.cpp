/*! ************************************************************************************************/
/*!
    \file teambalanceruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/teambalanceruledefinition.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rules/teambalancerule.h"

#include "gamemanager/tdf/matchmaker_config_server.h"

#include <math.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamBalanceRuleDefinition, "Predefined_TeamBalanceRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* TeamBalanceRuleDefinition::TEAM_IMBALANCE_GAME_KEY = "teamImbalance";
    const char8_t* TeamBalanceRuleDefinition::TEAM_IMBALANCE_DUMMY_RANGE_OFFSET_NAME = "DUMMY_RANGE_OFFSET";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamBalanceRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamBalanceRuleDefinition::TeamBalanceRuleDefinition()
        : RuleDefinition(),
        mFitFormula(nullptr),
        mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    TeamBalanceRuleDefinition::~TeamBalanceRuleDefinition()
    {
        delete mFitFormula;
    }

    bool TeamBalanceRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();
        if (mGlobalMaxTotalPlayerSlotsInGame == 0)
        {
            ERR_LOG("[TeamBalanceRuleDefinition] configured MaxTotalPlayerSlotsInGame cannot be 0.");
            return false;
        }

        GameManager::RangeOffsetLists dummyLists;
        GameManager::RangeOffsetList* dummyList = dummyLists.pull_back();
        dummyList->setName(TEAM_IMBALANCE_DUMMY_RANGE_OFFSET_NAME);
        RangeOffset* offset = dummyList->getRangeOffsets().pull_back();
        offset->setT(0);
        offset->getOffset().push_back(RangeListContainer::EXACT_MATCH_REQUIRED_TOKEN);

        const TeamBalanceRuleConfig& teamBalanceRuleConfig = matchmakingServerConfig.getRules().getPredefined_TeamBalanceRule();
        if (!teamBalanceRuleConfig.isSet())
        {
            WARN_LOG("[TeamBalanceRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }
        else
        {
            mFitFormula = FitFormula::createFitFormula(teamBalanceRuleConfig.getFitFormula().getName());
            if (mFitFormula == nullptr)
            {
                ERR_LOG("[TeamBalanceRuleDefinition].create TeamBalanceRuleDefinition mFitFormula failed. The fit formula name is " << teamBalanceRuleConfig.getFitFormula().getName());
                return false;
            }
            
            if (!mFitFormula->initialize(teamBalanceRuleConfig.getFitFormula(), &teamBalanceRuleConfig.getRangeOffsetLists()))
            {
                ERR_LOG("[TeamBalanceRuleDefinition].initalize TeamBalanceRuleDefinition mFitFormula failed.");
                return false;
            }

            RuleDefinition::initialize(name, salience, teamBalanceRuleConfig.getWeight());
        
            // Call helper to parse the min fit threshold list for us
            bool ret = mRangeListContainer.initialize(getName(), teamBalanceRuleConfig.getRangeOffsetLists(), false);
            ret = ret && mRangeListContainer.initialize(getName(), dummyLists);
            return ret;
        }
    }

    void TeamBalanceRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
        mRangeListContainer.initRangeListInfo(predefinedRuleConfig.getThresholdNames());
    }

    void TeamBalanceRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty())
        {
            matchmakingCache.updateCachedTeamIds(gameSession.getTeamIds());
        }
    }

    const RangeList* TeamBalanceRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    void TeamBalanceRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // no rule-specific config values for team BALANCE rule
    }

    void TeamBalanceRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // The sorted teams vector on the game is only on the cache.
        const MatchmakingGameInfoCache::TeamIdInfoVector &gameTeamIdInfoVector = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedTeamIdInfoVector();
        if (gameTeamIdInfoVector.size() == 0)
        {
            TRACE_LOG("[TeamCountRuleDefinition] Not adding WME's for game(" << gameSessionSlave.getGameId() << "), does not contain teams.");
            return;
        }

        // And that's the balance (only useful for the game browser, since real find game would need to do additional logic with # of players)
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            TEAM_IMBALANCE_GAME_KEY, calcGameBalance(gameSessionSlave), *this);
    }
    
    uint16_t TeamBalanceRuleDefinition::calcGameBalance(const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());

        // We find the two teams with the largest and smallest sizes:
        uint16_t minTeamSize = mGlobalMaxTotalPlayerSlotsInGame;
        uint16_t maxTeamSize = 0;
        uint16_t teamCount = gameSessionSlave.getTeamCount();
        for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t curTeamSize = gameRoster.getTeamSize(teamIndex);
            if (curTeamSize < minTeamSize)
                minTeamSize = curTeamSize;
            if (curTeamSize > maxTeamSize)
                maxTeamSize = curTeamSize;
        }

        uint16_t gameBalance = maxTeamSize - minTeamSize;
        return gameBalance;
    }

    void TeamBalanceRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // The sorted teams vector on the game is only on the cache.
        const MatchmakingGameInfoCache::TeamIdInfoVector &gameTeamIdInfoVector = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedTeamIdInfoVector();
        if (gameTeamIdInfoVector.size() == 0)
        {
            TRACE_LOG("[TeamCountRuleDefinition] Not adding WME for game(" << gameSessionSlave.getGameId() << "), does not contain teams.");
            return;
        }

        // And that's the balance (only useful for the game browser, since real find game would need to do additional logic with # of players)
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            TEAM_IMBALANCE_GAME_KEY, calcGameBalance(gameSessionSlave), *this);
    }

    /*! ************************************************************************************************/
    /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
                            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void TeamBalanceRuleDefinition::toString(eastl::string &dest, const char8_t* prefix /*=""*/) const
    {
        RuleDefinition::toString(dest, prefix);
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }

    float TeamBalanceRuleDefinition::getFitPercent(uint16_t actualValue, uint16_t desiredValue) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(actualValue, desiredValue);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
