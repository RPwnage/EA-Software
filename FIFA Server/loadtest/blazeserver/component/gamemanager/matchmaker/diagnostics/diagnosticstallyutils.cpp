/*! ************************************************************************************************/
/*!
    \file diagnosticstallyutils.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticstallyutils.h"
#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/matchmakingsessionslave.h" //for MatchmakingSessionSlave in tallyDiagnosticsForFgSession
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    const char8_t * RuleDiagnosticSetupInfo::CATEGORY_RULE_SUMMARY = "RULE_SUMMARY";

    /*! ************************************************************************************************/
    /*! \brief helper to add rule's entry to diagnostic map
    ***************************************************************************************************/
    DiagnosticsByRuleCategoryMapPtr DiagnosticsTallyUtils::getOrAddRuleDiagnostic(
        DiagnosticsByRuleMap& diagnosticsMap, const char8_t* ruleName)
    {
        if (ruleName == nullptr)
        {
            ASSERT_LOG("getOrAddRuleDiagnostic: internal error: nullptr rule name. Diagnostics may not be updated correctly.");
            return nullptr;
        }
        DiagnosticsByRuleMap::iterator foundDItr = diagnosticsMap.find(ruleName);
        if (foundDItr == diagnosticsMap.end())
        {
            diagnosticsMap[ruleName] = diagnosticsMap.allocate_element();
            return diagnosticsMap[ruleName];
        }
        return foundDItr->second;
    }

    /*! ************************************************************************************************/
    /*! \brief helper to add the rule search value's entry to the rule's sub diagnostics map
    ***************************************************************************************************/
    MatchmakingRuleDiagnosticCountsPtr DiagnosticsTallyUtils::getOrAddRuleSubDiagnostic(
        DiagnosticsByRuleCategoryMap& ruleSubDiagnostics, const char8_t* category, const char8_t* valueStr)
    {
        if ((category == nullptr) || (valueStr == nullptr))
        {
            ASSERT_LOG("getOrAddRuleSubDiagnostic: internal error: nullptr categoryName(" << (category != nullptr ? category : "<nullptr>")
                << "), valuestr(" << (valueStr != nullptr ? valueStr : "<nullptr>") << "). Diagnostics may not be updated correctly.");
            return nullptr;
        }

        // get or add the category's entry, and get its rule value map
        DiagnosticsByRuleCriteriaValueMap* ruleValueMap = nullptr;
        for (auto& itr : ruleSubDiagnostics)
        {
            if (blaze_stricmp(itr.first, category) == 0)
            {
                ruleValueMap = itr.second;
                break;
            }
        }
        if (ruleValueMap == nullptr)
        {
            ruleValueMap = ruleSubDiagnostics.allocate_element();
            ruleSubDiagnostics[category] = ruleValueMap;
        }

        // get or add the end rule value entry
        DiagnosticsByRuleCriteriaValueMap::iterator ruleValueEntryItr = ruleValueMap->find(valueStr);
        if (ruleValueEntryItr == ruleValueMap->end())
        {
            (*ruleValueMap)[valueStr] = ruleValueMap->allocate_element();
            return (*ruleValueMap)[valueStr];
        }
        return ruleValueEntryItr->second;
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to add one diagnostic object's counts to the other object, i.e. the sub session's appropriate:
            sessions
            createEvaluations
            createEvaluationsMatched
            findRequestsGamesAvailable
            ruleDiagnostics map

        \param[in] mergeSessionCount If true, rather than adding session count, use max of the 2. To avoid
            double counting for same MM session.
    ***************************************************************************************************/
    void DiagnosticsTallyUtils::addDiagnosticsCountsToOther(MatchmakingSubsessionDiagnostics& other,
        const MatchmakingSubsessionDiagnostics& countsToAdd,
        bool addCreateMode, bool addFindMode, bool mergeSessionCount)
    {
        other.setSessions(mergeSessionCount ? eastl::max(other.getSessions(), countsToAdd.getSessions())
            : (other.getSessions() + countsToAdd.getSessions()));

        if (addCreateMode)
        {
            other.setCreateEvaluations(other.getCreateEvaluations() + countsToAdd.getCreateEvaluations());
            other.setCreateEvaluationsMatched(other.getCreateEvaluationsMatched() + countsToAdd.getCreateEvaluationsMatched());
        }

        if (addFindMode)
        {
            other.setFindRequestsGamesAvailable(other.getFindRequestsGamesAvailable() +
                countsToAdd.getFindRequestsGamesAvailable());
        }

        // add rule breakdown diagnostic entries
        addRuleDiagnosticsCountsToOther(other.getRuleDiagnostics(), countsToAdd.getRuleDiagnostics(), addCreateMode,
            addFindMode, mergeSessionCount);
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to add one diagnostics rule map's counts to the other map.

        \param[in] mergeSessionCount If true, rather than adding session count, use max of the 2. To avoid 
            double counting for same MM session.
    ***************************************************************************************************/
    void DiagnosticsTallyUtils::addRuleDiagnosticsCountsToOther(DiagnosticsByRuleMap& other,
        const DiagnosticsByRuleMap& countsToAdd,
        bool addCreateMode, bool addFindMode, bool mergeSessionCount)
    {
        for (auto& ruleDiagosticsToAdd : countsToAdd)
        {
            const auto* ruleName = ruleDiagosticsToAdd.first.c_str();
            const auto& ruleCategoriesToAdd = ruleDiagosticsToAdd.second;

            DiagnosticsByRuleCategoryMapPtr ruleDiagnosticOther = getOrAddRuleDiagnostic(other, ruleName);
            if ((ruleDiagnosticOther == nullptr) || (ruleCategoriesToAdd == nullptr))
            {
                ASSERT_LOG("addRuleDiagnosticsCountsToOther: couldn't get or add rule(" << ruleName << ") diagnostic in other map. No op.");
                continue;
            }

            // scan the source categories and add each category to other
            for (auto& ruleCategoryToAdd : *ruleCategoriesToAdd)
            {
                // scan the source category's values map and add each map entry to other
                const auto* categoryTag = ruleCategoryToAdd.first.c_str();
                const auto& ruleValuesToAdd = ruleCategoryToAdd.second;

                for (auto& ruleValueToAdd : *ruleValuesToAdd)
                {
                    const auto* valueTag = ruleValueToAdd.first.c_str();
                    const auto& ruleValueCountToAdd = ruleValueToAdd.second;

                    MatchmakingRuleDiagnosticCountsPtr ruleValueCountsOther = getOrAddRuleSubDiagnostic(*ruleDiagnosticOther, categoryTag, valueTag);
                    if ((ruleValueCountsOther == nullptr) || (ruleValueCountToAdd == nullptr))
                    {
                        continue;//logged
                    }

                    addRuleValueCountToOther(*ruleValueCountsOther, *ruleValueCountToAdd, addCreateMode, addFindMode,
                        mergeSessionCount);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to add one diagnostic object's counts to the other object, i.e. the rule's appropriate:
            sessions
            createEvaluations
            createEvaluationsMatched
            findRequests
            findRequestsHadGames
            findRequestsGamesVisible
    ***************************************************************************************************/
    void DiagnosticsTallyUtils::addRuleValueCountToOther(MatchmakingRuleDiagnosticCounts& other,
        const MatchmakingRuleDiagnosticCounts& countsToAdd,
        bool addCreateMode, bool addFindMode, bool mergeSessionCount)
    {
        other.setSessions(mergeSessionCount ? eastl::max(other.getSessions(), countsToAdd.getSessions())
            : (other.getSessions() + countsToAdd.getSessions()));

        if (addCreateMode)
        {
            other.setCreateEvaluations(other.getCreateEvaluations() + countsToAdd.getCreateEvaluations());
            other.setCreateEvaluationsMatched(other.getCreateEvaluationsMatched() + countsToAdd.getCreateEvaluationsMatched());
        }

        if (addFindMode)
        {
            other.setFindRequests(mergeSessionCount ?
                eastl::max(other.getFindRequests(), countsToAdd.getFindRequests())
                : (other.getFindRequests() + countsToAdd.getFindRequests()));

            other.setFindRequestsHadGames(mergeSessionCount ?
                eastl::max(other.getFindRequestsHadGames(), countsToAdd.getFindRequestsHadGames())
                : (other.getFindRequestsHadGames() + countsToAdd.getFindRequestsHadGames()));

            other.setFindRequestsGamesVisible(other.getFindRequestsGamesVisible() + countsToAdd.getFindRequestsGamesVisible());
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
