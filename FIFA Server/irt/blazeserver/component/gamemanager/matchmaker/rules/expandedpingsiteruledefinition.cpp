/*! ************************************************************************************************/
/*!
\file expandedpingsiteruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/expandedpingsiteruledefinition.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/rules/uedrule.h"
#include "gamemanager/matchmaker/groupvalueformulas.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rangelist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(ExpandedPingSiteRuleDefinition, "Predefined_ExpandedPingSiteRule", RULE_DEFINITION_TYPE_SINGLE);

    const char8_t* ExpandedPingSiteRuleDefinition::EXPANDED_PING_SITE_RULE_DEFINITION_RETE_NAME = "expandedPingSite";
    const char8_t* ExpandedPingSiteRuleDefinition::UNKNOWN_PING_SITE_RULE_DEFINITION_RETE_NAME = "unknownPingSite";
    const char8_t* ExpandedPingSiteRuleDefinition::DEDICATED_SERVER_WHITELIST = "DEDICATED_SERVER_WHITELIST";
    const char8_t* ExpandedPingSiteRuleDefinition::DEDICATED_SERVER_MATCH_ANY_THRESHOLD = "fdsv_matchAny";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    ExpandedPingSiteRuleDefinition::ExpandedPingSiteRuleDefinition()
        : mFitFormula(nullptr),
          mMinLatencyOverride(0)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    ExpandedPingSiteRuleDefinition::~ExpandedPingSiteRuleDefinition()
    {
        delete mFitFormula;
    }

    bool ExpandedPingSiteRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        const ExpandedPingSiteRuleConfig& config = matchmakingServerConfig.getRules().getPredefined_ExpandedPingSiteRule();

        if (!config.isSet())
        {
            WARN_LOG("[ExpandedPingSiteRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        config.copyInto(mRuleConfig);
        RuleDefinition::initialize(name, salience, mRuleConfig.getWeight());

        mFitFormula = FitFormula::createFitFormula(mRuleConfig.getFitFormula().getName());
        if (!mFitFormula->initialize(mRuleConfig.getFitFormula(), &mRuleConfig.getRangeOffsetLists()))
        {
            WARN_LOG("[ExpandedPingSiteRuleDefinition] Rule " << getConfigKey() << " failed to initialize fit formula.");
            return false;
        }

        mMinLatencyOverride = config.getMinLatencyOverride();

        GameManager::RangeOffsetLists tempLists;
        mRuleConfig.getRangeOffsetLists().copyInto(tempLists);
        for (GameManager::RangeOffsetLists::const_iterator itr = tempLists.begin(), end = tempLists.end()
             ; itr != end
             ; ++itr)
        {
            const RangeOffsetList& list = **itr;
            if (blaze_stricmp(list.getName(), DEDICATED_SERVER_MATCH_ANY_THRESHOLD) == 0)
            {
                WARN_LOG("[ExpandedPingSiteRuleDefinition] Rule " << getConfigKey() << " disabled, you can't have " << list.getName() << " as a range offset.");
                return false;
            }
        }

        GameManager::RangeOffsetList *anyMatchList = tempLists.pull_back();
        anyMatchList->setName(DEDICATED_SERVER_MATCH_ANY_THRESHOLD);
        GameManager::RangeOffset *anyOffset = anyMatchList->getRangeOffsets().pull_back();
        anyOffset->setT(0);
        anyOffset->getOffset().push_back("INF");

        return mRangeListContainer.initialize(getName(), tempLists);
    }

    void ExpandedPingSiteRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = %s\n", prefix, "ruleDefinitionType", getType());
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }

    const RangeList* ExpandedPingSiteRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    float ExpandedPingSiteRuleDefinition::getFitPercent(uint32_t myValue) const
    {
        if (myValue <= mMinLatencyOverride)
        {
            TRACE_LOG("[ExpandedPingSiteRuleDefinition].getFitPercent: My latency (" << myValue << "ms) is below min latency threshold (" << mMinLatencyOverride << "ms).");
            return 1.0f;
        }

        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(mMinLatencyOverride, myValue);

        return 0.0f;
    }

    //////////////////////////////////////////////////////////////////////////
    // GameReteRuleDefinition Functions
    //////////////////////////////////////////////////////////////////////////
    void ExpandedPingSiteRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const char8_t* bestPingSite = gameSessionSlave.getBestPingSiteAlias();
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), EXPANDED_PING_SITE_RULE_DEFINITION_RETE_NAME, getWMEAttributeValue(bestPingSite), *this);

        // Special case for servers that somehow didn't set their pingsites correctly:
        bool unknownPingSite = (gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().find(bestPingSite) == gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().end());
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), UNKNOWN_PING_SITE_RULE_DEFINITION_RETE_NAME, getWMEBooleanAttributeValue(unknownPingSite), *this);
    }

    void ExpandedPingSiteRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        insertWorkingMemoryElements(wmeManager, gameSessionSlave);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze