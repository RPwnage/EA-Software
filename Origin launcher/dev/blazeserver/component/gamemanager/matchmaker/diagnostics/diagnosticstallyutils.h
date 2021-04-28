/*! ************************************************************************************************/
/*!
    \file diagnosticstallyutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKER_DIAGNOSTICS_TALLY_UTIL_H
#define BLAZE_MATCHMAKER_DIAGNOSTICS_TALLY_UTIL_H

#include "gamemanager/tdf/matchmaker_types.h"
#include "gamemanager/tdf/matchmakingmetrics_server.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticsgamecountutils.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class Rule;
    class MatchmakingSessionSlave;
    

    /*! ************************************************************************************************/
    /*! \brief info for building a diagnostic counts entry, for a MM session's rule/criteria
    ***************************************************************************************************/
    struct RuleDiagnosticSetupInfo
    {
        static const char8_t *CATEGORY_RULE_SUMMARY;

        RuleDiagnosticSetupInfo(const char8_t* categoryTag = CATEGORY_RULE_SUMMARY, const char8_t* valueTag = "")
            :
            mIsRuleTotal(categoryTag == CATEGORY_RULE_SUMMARY),
            mCategoryTag(categoryTag),
            mValueTag(valueTag),
            mCachedSubDiagnosticPtr(nullptr)
        {
        }

        // whether for the rule summary/total
        bool mIsRuleTotal;

        // category name
        eastl::string mCategoryTag;
        
        // counted value's display string. e.g. multiple desired values can be comma delimited
        eastl::string mValueTag;

        // cached pointer to the MM session's final diagnostic counts
        mutable MatchmakingRuleDiagnosticCountsPtr mCachedSubDiagnosticPtr;
    };

    typedef eastl::list<RuleDiagnosticSetupInfo> RuleDiagnosticSetupInfoList;
    typedef EA::TDF::tdf_ptr<DiagnosticsByRuleCategoryMap> DiagnosticsByRuleCategoryMapPtr;

    /*! ************************************************************************************************/
    /*! \brief misc helpers to tally diagnostics for a MM session
    ***************************************************************************************************/
    class DiagnosticsTallyUtils
    {
    public:
        static DiagnosticsByRuleCategoryMapPtr getOrAddRuleDiagnostic(DiagnosticsByRuleMap& diagnosticsMap, const char8_t* ruleName);
        static MatchmakingRuleDiagnosticCountsPtr getOrAddRuleSubDiagnostic(DiagnosticsByRuleCategoryMap& ruleSubDiagnostics, const char8_t* categoryStr, const char8_t* valueStr);

        static void addDiagnosticsCountsToOther(MatchmakingSubsessionDiagnostics& other, const MatchmakingSubsessionDiagnostics& countsToAdd, bool addCreateMode, bool addFindMode, bool mergeSessionCount);

    private:
        static void addRuleDiagnosticsCountsToOther(DiagnosticsByRuleMap& other, const DiagnosticsByRuleMap& countsToAdd, bool addCreateMode, bool addFindMode, bool mergeSessionCount);
        static void addRuleValueCountToOther(MatchmakingRuleDiagnosticCounts& other, const MatchmakingRuleDiagnosticCounts& countsToAdd, bool addCreateMode, bool addFindMode, bool mergeSessionCount);
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKER_DIAGNOSTICS_TALLY_UTIL_H
