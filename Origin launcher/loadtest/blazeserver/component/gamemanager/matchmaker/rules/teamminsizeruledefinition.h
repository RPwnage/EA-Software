/*! ************************************************************************************************/
/*!
    \file   teamMINSIZEruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_DEFINITION
#define BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RangeList;
    class FitFormula;
    class RangeListContainer;
    class MatchmakingGameInfoCache;

    class TeamMinSizeRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(TeamMinSizeRuleDefinition);
        DEFINE_RULE_DEF_H(TeamMinSizeRuleDefinition, TeamMinSizeRule);
    public:
        static const char8_t* MIN_TEAM_SIZE_IN_GAME_KEY;

        TeamMinSizeRuleDefinition();
        ~TeamMinSizeRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        /*! ************************************************************************************************/
        /*! \brief write the configuration information for this rule into a MatchmakingRuleInfo TDF object
            \param[in\out] ruleConfig - rule configuration object to pack with data for this rule
        *************************************************************************************************/
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        bool isDisabled() const override { return mRangeListContainer.empty(); }
        uint32_t getWeight() const { return mWeight; }
        const RangeList* getRangeOffsetList(const char8_t *listName) const;

        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;

        /*! ************************************************************************************************/
        /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

            The ruleDefn is written into the string using the config file's syntax.

            \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
            \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
            Used to 'indent' a rule for more readable logs.
        *************************************************************************************************/
        void toString(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override {return true;}
        bool callEvaluateForMMFindGameAfterRete() const override {return true;}

        // RETE impl
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        // end RETE

        float getFitPercent(uint16_t desiredValue, uint16_t actualValue) const;
        uint16_t getGlobalMinTotalPlayerSlotsInGame() const { return mGlobalMinTotalPlayerSlotsInGame; }
        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }
    protected:
        uint16_t calcMinTeamSize(const Search::GameSessionSearchSlave& gameSessionSlave) const;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
        uint16_t mGlobalMinTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_DEFINITION
