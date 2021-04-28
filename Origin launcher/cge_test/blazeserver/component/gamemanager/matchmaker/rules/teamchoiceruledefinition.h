/*! ************************************************************************************************/
/*!
    \file   teamCHOICEruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_DEFINITION
#define BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RangeList;
    class RangeListContainer;
    class MatchmakingGameInfoCache;

    class TeamChoiceRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(TeamChoiceRuleDefinition);
        DEFINE_RULE_DEF_H(TeamChoiceRuleDefinition, TeamChoiceRule);
    public:
        static const char8_t* TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY;
        static const char8_t* TEAMCHOICERULE_DUPLICATE_RETE_KEY;

        TeamChoiceRuleDefinition();
        ~TeamChoiceRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        /*! ************************************************************************************************/
        /*! \brief write the configuration information for this rule into a MatchmakingRuleInfo TDF object
            \param[in\out] ruleConfig - rule configuration object to pack with data for this rule
        *************************************************************************************************/
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        bool isDisabled() const override { return false; }

        uint32_t getWeight() const { return mWeight; }

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


        // RETE impl
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        // end RETE

        int64_t getMaxGameSize() const { return mMaxGameSize; }

    protected:
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;

        int64_t mMaxGameSize;
    
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_DEFINITION
