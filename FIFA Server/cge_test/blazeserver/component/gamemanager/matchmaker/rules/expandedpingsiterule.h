/*! ************************************************************************************************/
/*!
    \file   expandedpingsiterule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_EXPANDED_PING_SITE_RULE
#define BLAZE_MATCHMAKING_EXPANDED_PING_SITE_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"

#include "gamemanager/matchmaker/rules/expandedpingsiteruledefinition.h"

namespace Blaze
{
namespace GameManager
{

namespace Matchmaker
{
    struct MatchmakingSupplementalData;
    class RangeList;

    typedef eastl::map<PingSiteAlias, eastl::vector<uint32_t> > LatenciesByAcceptedPingSiteIntersection;
    typedef eastl::pair<uint32_t, PingSiteAlias> PingSiteOrderPair;
    typedef eastl::vector<PingSiteOrderPair> OrderedPreferredPingSiteList;

    /*! ************************************************************************************************/
    /*!
        \brief This rule groups players has same nearest qos ping site (shortest latency) together.
        The goal is to match players share same data center 

        ExpandedPingSiteRules must be initialized before use(looking up their definition & minFitThreshold from the matchmaker).
        After initialization, update a rule's cachedInfo, then evaluate the rule against a game of matchmaking session.
    *************************************************************************************************/
    class ExpandedPingSiteRule : public SimpleRangeRule
    {
    public:

        ExpandedPingSiteRule(const ExpandedPingSiteRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ExpandedPingSiteRule(const ExpandedPingSiteRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~ExpandedPingSiteRule() override;

        /*! ************************************************************************************************/
        /*! \brief initialize the rule given the matchmaking criteria data sent from the client.

            Uses the specific matchmaking criteria data for this rule to customize how this rule evaluates
            the other matchmaking sessions and game sessions.  The MatchmakingCriteriaData object should not
            be stored off, but any of the data needed can be copied to member data on the rule.  Also, any
            caching of data that needs to be calculated should be done here.  Caching off values here can
            help a matchmaking rule run more efficiently.

            NOTE: this function is called by matchmaker and gamebrowser.  To distinguish between the two
            during a call from gamebrowser the matchmakingsession pointer will be null.  Anything requiring
            the matchmakingsession should not be needed for a gamebrowser evaluation.
            
            \param[in] criteriaData The MatchmakingCriteriaData passed up by the client.
            \param[out] err An error message to be passed out in the event of initialization failure.
            \return false if there was an error initializing this rule.
        ****************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        /*! We need to make sure that evaluation happens if one session has the rule enabled. */
        bool isEvaluateDisabled() const override { return true; }

        // Disable the ping site rule if the person searching doesn't have a best ping site set: 
        bool isDisabled() const override { return ((mRangeOffsetList == nullptr) || mBestPingSiteAlias.empty()); }

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        bool isDedicatedServerEnabled() const override { return true; }

        /*! ************************************************************************************************/
        /*! \brief Clone the current rule copying the rule criteria using the copy ctor.  Used for subsessions.
        ****************************************************************************************************/
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! ************************************************************************************************/
        /*! \brief Calculate the fitpercent and exact match for the Game Session.  This method is called
            for  evaluateForMMFindGame to determine the fitpercent for a game.

            This method is expected to be implemented by deriving classes for Game Browser and MM Find Game.

            \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
            \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
            \param[out] isExactMatch If the match to this Game Session is an exact match.
        ****************************************************************************************************/
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        /*! ************************************************************************************************/
        /*! \brief Calculate the fitpercent and exact match for the Game Session.  This method is called
            for evaluateForMMFindGame to determine the fitpercent for a game.

            This method is expected to be implemented by deriving classes for Game Browser and MM Find Game.

            \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
            \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
            \param[out] isExactMatch If the match to this Game Session is an exact match.
            \param[in/out] debugRuleConditions The debug rule condition information from the evaluation
        ****************************************************************************************************/
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        const ExpandedPingSiteRuleDefinition& getDefinition() const { return static_cast<const ExpandedPingSiteRuleDefinition&>(mRuleDefinition); }

        /*! ************************************************************************************************/
        /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
        ***************************************************************************************************/
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        /*! ************************************************************************************************/
        /*! \brief Framework hook which allows this rule to update the MatchmakingAsyncStatus before it gets
            sent to the client.  Modifies this rules portion of mMatchmakingAsyncStatus.
        ****************************************************************************************************/
        void updateAsyncStatus() override;

        void updateAcceptableRange() override;

        typedef eastl::vector_map<PingSiteAlias, uint32_t> LatencyByAcceptedPingSiteMap;
        const LatencyByAcceptedPingSiteMap& getLatencyByAcceptedPingSiteMap() const { return mLatencyByAcceptedPingSiteMap; }

        /*! ************************************************************************************************/
        /*! \brief Builds a sorted list of preferred ping sites based on the rule's criteria
        ****************************************************************************************************/
        void buildAcceptedPingSiteList(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites) const;

        /************* PingSiteOrderCalculators *************/
        typedef void(*PingSiteOrderCalculator)(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites);
        static PingSiteOrderCalculator getPingSiteOrderCalculator(PingSiteSelectionMethod method);

        static void calcPingSiteOrderStandardDeviation(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites);
        static void calcPingSiteOrderBest(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites);
        static void calcPingSiteOrderWorst(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites);
        static void calcPingSiteOrderAverage(const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection, OrderedPreferredPingSiteList& orderedPreferredPingSites);

        const PingSiteLatencyByAliasMap& getComputedLatencyByPingSiteMap() const { return mComputedLatencyByPingSite; }
        const char8_t* getBestPingSiteAlias() const { return mBestPingSiteAlias.c_str(); }
    private:
        /************* GroupLatencyCalcultors *************/
        typedef void (*GroupLatencyCalculator)(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData);
        static GroupLatencyCalculator getGroupLatencyCalculator(LatencyCalcMethod method);

        static void calcLatencyForGroupBestLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData);
        static void calcLatencyForGroupWorstLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData);
        static void calcLatencyForGroupAverageLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData);
        static void calcLatencyForGroupMedianLatency(ExpandedPingSiteRule& owner, const MatchmakingSupplementalData& matchmakingSupplementalData);

        /************* SessionMatchCalculators *************/
        typedef void(*SessionMatchFitPercentCalculator)(ExpandedPingSiteRule& owner, const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds);
        static SessionMatchFitPercentCalculator getSessionMatchFitPercentCalculator(SessionMatchCalcMethod method);

        static void calcSessionMatchFitPercentMyBest(ExpandedPingSiteRule& owner, const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds);
        static void calcSessionMatchFitPercentMutualBest(ExpandedPingSiteRule& owner, const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds);


    private:
        /*! ************************************************************************************************/
        /*!
            \brief evaluate your preferred ping site vs. another sessions preferred ping site. The evaluation will 
                help keep people playing in the same regions together.  The threshold will then decay to match 
                any other player, giving higher scores to players who have the "nearest" ping site to this
                matchmaking session based upon the fit table in the config file.
        *************************************************************************************************/
        void calcFitPercentInternal(const PingSiteAlias& pingSite, float& fitPercent, bool& isExactMatch) const;
        void calcFitPercentInternal(const PingSiteAlias& pingSite, const RangeList::Range& acceptableRange, float& fitPercent, bool& isExactMatch) const;

        /*! ************************************************************************************************/
        /*!
            \brief returns the latency for the specified ping site. This is the max latency of the MM session
                (either single user or group) for the ping site.
            \return The ping site's latency
        *************************************************************************************************/
        bool getLatencyForPingSite(const PingSiteAlias& pingSite, uint32_t& latency) const;

        /*! ************************************************************************************************/
        /*!
            \brief returns the best ping site alias of the session owner when the rule was initialized.
            \return the ping site alias
        *************************************************************************************************/
      
        bool isBestPingSiteAlias(const PingSiteAlias& pingSite) const { return pingSite == mBestPingSiteAlias; }

        bool isPingSiteAcceptable(const PingSiteAlias& pingSite, const RangeList::Range& range, const ExpandedPingSiteRule& otherRule, uint32_t myLatency) const;
        bool isInRange(const RangeList::Range& range, uint32_t latency) const;

        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

        void calcSessionMatchFitPercent(const ExpandedPingSiteRule& otherExpandedPingSiteRule, float &bestFitPercent, uint32_t &matchSeconds) const;

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

    private:
        PingSiteAlias mBestPingSiteAlias; // "best" or "nearest" qos ping site for the owner MM session;
        PingSiteAliasList mPingSiteWhitelist;  // List of ping sites we can consider (if empty, uses full qos list)
        PingSiteLatencyByAliasMap mComputedLatencyByPingSite; // The computed qos ping site latencies by ping site for the MM session
        LatencyByAcceptedPingSiteMap mLatencyByAcceptedPingSiteMap; // The latencies for our currently accepted ping sites

        SessionMatchCalcMethod mSessionMatchCalcMethod;
        PingSiteSelectionMethod mPingSiteSelectionMethod;

        bool mIsGroupExactMatch;
        bool mAcceptUnknownPingSite;

        static const int64_t RANGE_OFFSET_CENTER_VALUE = 0;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_EXPANDED_PING_SITE_RULE
