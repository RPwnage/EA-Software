/*! ************************************************************************************************/
/*!
\file expandedpingsiteruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_EXPANDEDPINGSITERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_EXPANDEDPINGSITERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RangeList;

    class ExpandedPingSiteRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(ExpandedPingSiteRuleDefinition);
        DEFINE_RULE_DEF_H(ExpandedPingSiteRuleDefinition, ExpandedPingSiteRule);

    public:
        static const char8_t* EXPANDED_PING_SITE_RULE_DEFINITION_RETE_NAME;
        static const char8_t* UNKNOWN_PING_SITE_RULE_DEFINITION_RETE_NAME;
        static const char8_t* DEDICATED_SERVER_WHITELIST;
        static const char8_t* DEDICATED_SERVER_MATCH_ANY_THRESHOLD;

    public:
        ExpandedPingSiteRuleDefinition();
        ~ExpandedPingSiteRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override { return true; }

        const RangeList* getRangeOffsetList(const char8_t* thresholdName) const;
        uint32_t getMinLatencyOverride() const { return mMinLatencyOverride; }

        float getFitPercent(uint32_t myValue) const;

        bool isDisabled() const override { return mRangeListContainer.empty(); }

        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

    private:
        ExpandedPingSiteRuleConfig mRuleConfig;

        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;
        uint32_t mMinLatencyOverride;
    };
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_EXPANDEDPINGSITERULEDEFINITION_H
