/*! ************************************************************************************************/
/*!
\file   exampleupstreambpsrule.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "exampleupstreambpsrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    ExampleUpstreamBPSRule::ExampleUpstreamBPSRule(const ExampleUpstreamBPSRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mDesiredUpstreamBps(0),
        mMyUpstreamBps(0)
    {
    }

    ExampleUpstreamBPSRule::ExampleUpstreamBPSRule(const ExampleUpstreamBPSRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
          mDesiredUpstreamBps(otherRule.mDesiredUpstreamBps),
          mMyUpstreamBps(otherRule.mMyUpstreamBps)
    {
    }

    ExampleUpstreamBPSRule::~ExampleUpstreamBPSRule() {}

    bool ExampleUpstreamBPSRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        // MM_CUSTOM_CODE - BEGIN exampleUpstreamBPSRule
        //  verifies the rule criteria (named ExampleUpstreamBPSRule) inside the variable custom rule map and returns the custom rule critieria tdf
        //const ExampleUpstreamBPSRuleCriteria* criteriaPtr = getCustomRulePrefFromCriteria<ExampleUpstreamBPSRuleCriteria>(criteriaData, "exampleUpstreamBPSRule");
        //if (criteriaPtr == nullptr)
        //{
        //    return false;
        //}
        //
        //const ExampleUpstreamBPSRuleCriteria& exampleCriteria = *criteriaPtr;
        //mDesiredUpstreamBps = exampleCriteria.getDesiredUpstreamBps();
        //
        //// NOTE: initMinFitThreshold should be called in the initialize method of any rule that uses threshold lists.
        //const char8_t* listName = exampleCriteria.getMinFitThresholdName();
        //if (!initMinFitThreshold(listName, mRuleDefinition, err))
        //{
        //    return false;
        //}
        //
        //// Game Browser should never call this function so check that the MM session exists.
        //if (mMatchmakingSession != nullptr)
        //{
        //    mMyUpstreamBps = determineUpstreamBps();
        //}
        // MM_CUSTOM_CODE - END exampleUpstreamBPSRule

        return true;
    }

    uint32_t ExampleUpstreamBPSRule::determineUpstreamBps() const
    {
        EA_ASSERT_MSG(getMatchmakingSession() != nullptr, "Init by Game Browser should never call this function.");

        if (getMatchmakingSession()->isSinglePlayerSession())
        {
            // For a single player mm session just use that players upstream BPS.
            const GameManager::UserSessionInfo* info = getMatchmakingSession()->getPrimaryUserSessionInfo();
            if (info != nullptr)
                return info->getQosData().getUpstreamBitsPerSecond();
            else
                return 0;
        }
        else
        {
            // For a multi player mm session take the highest upstream BPS from the group of players
            uint32_t upstreamBps = 0;

            for (MatchmakingSession::MemberInfoList::const_iterator itr = getMatchmakingSession()->getMemberInfoList().begin(), 
                 end = getMatchmakingSession()->getMemberInfoList().end(); itr != end; ++itr)
            {
                const MatchmakingSession::MemberInfo& memberInfo = static_cast<const MatchmakingSession::MemberInfo&>(*itr);
                upstreamBps = eastl::max(upstreamBps, memberInfo.mUserSessionInfo.getQosData().getUpstreamBitsPerSecond());               
            }

            return upstreamBps;
        }
    }

    void ExampleUpstreamBPSRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch) const
    {
        uint32_t gameUpstreamBps = gameSession.getNetworkQosData().getUpstreamBitsPerSecond();

        isExactMatch = (mDesiredUpstreamBps == gameUpstreamBps);

        const ExampleUpstreamBPSRuleDefinition& exampleDefn = static_cast<const ExampleUpstreamBPSRuleDefinition &>(mRuleDefinition);
        fitPercent = exampleDefn.getFitPercent(mDesiredUpstreamBps, gameUpstreamBps);

        TRACE_LOG("[ExampleUpstreamBPSRule] rule '" << getRuleName() << "' : Game (id '" << gameSession.getGameId() << "', upBPS '" 
                  << gameSession.getNetworkQosData().getUpstreamBitsPerSecond() << "'), Rule (desBPS '" << mDesiredUpstreamBps << "', threshold '" 
                  << mCurrentMinFitThreshold << "')");
    }


    void ExampleUpstreamBPSRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const
    {
        const ExampleUpstreamBPSRule& otherExampleUpstreamBPSRule = static_cast<const ExampleUpstreamBPSRule &>(otherRule);
        uint32_t otherUpstreamBps = otherExampleUpstreamBPSRule.mMyUpstreamBps;
        isExactMatch = (mDesiredUpstreamBps == otherUpstreamBps);

        const ExampleUpstreamBPSRuleDefinition& exampleDefn = static_cast<const ExampleUpstreamBPSRuleDefinition &>(mRuleDefinition);
        fitPercent = exampleDefn.getFitPercent(mDesiredUpstreamBps, otherUpstreamBps);

        TRACE_LOG("[ExampleUpstreamBPSRule] rule '" << getRuleName() << "' - This Rule (id '" << getMMSessionId() 
                    << "', desBPS '" << mDesiredUpstreamBps << "', actualBPS '" << mMyUpstreamBps << "', threshold '" << mCurrentMinFitThreshold 
                    << "'), Other Rule (id '" << otherExampleUpstreamBPSRule.getMMSessionId() << "', desBPS '" 
                    << otherExampleUpstreamBPSRule.mDesiredUpstreamBps << "', actualBPS '" << otherExampleUpstreamBPSRule.mMyUpstreamBps 
                    << "', threshold '" << otherExampleUpstreamBPSRule.mCurrentMinFitThreshold << "')");
    }


    void ExampleUpstreamBPSRule::updateAsyncStatus()
    {
        // MM_CUSTOM_CODE - BEGIN exampleUpstreamBPSRule
        //ExampleUpstreamBPSRuleStatus *myStatus = BLAZE_NEW ExampleUpstreamBPSRuleStatus();
        //const ExampleUpstreamBPSRuleDefinition& exampleDefn = static_cast<const ExampleUpstreamBPSRuleDefinition &>(mRuleDefinition);
        //
        //if (MinFitThresholdList::isThresholdExactMatchRequired(mCurrentMinFitThreshold))
        //{
        //   myStatus->setUpstreamBpsMin(mDesiredUpstreamBps);
        //    myStatus->setUpstreamBpsMax(mDesiredUpstreamBps);
        //}
        //else 
        //{
        //    uint32_t bpsDelta = exampleDefn.calcBpsDifference(mCurrentMinFitThreshold);
        //    myStatus->setUpstreamBpsMin(mDesiredUpstreamBps <= bpsDelta ? 0 : mDesiredUpstreamBps - bpsDelta); // clamp at 0
        //    myStatus->setUpstreamBpsMax(mDesiredUpstreamBps >= (UINT32_MAX - bpsDelta) ? UINT32_MAX : mDesiredUpstreamBps + bpsDelta); // clamp at UINT32_MAX
        //}
        //
        ////  pass in allocated status object - status is managed by and added to rule's async status object.
        //setCustomRuleAsyncStatus("ExampleUpstreamBPSRule", *myStatus);
        // MM_CUSTOM_CODE - END exampleUpstreamBPSRule
    }

    Rule* ExampleUpstreamBPSRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW ExampleUpstreamBPSRule(*this, matchmakingAsyncStatus);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
