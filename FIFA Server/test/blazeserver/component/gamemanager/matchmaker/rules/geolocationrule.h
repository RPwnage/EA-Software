/*! ************************************************************************************************/
/*!
\file   geolocationrule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GEO_LOCATION_RULE_H
#define BLAZE_MATCHMAKING_GEO_LOCATION_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/geolocationruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief A Matchmaking rule to match based of Latitude and Longitude location information.
        This rule depends on the GeoLocation information that is populated by UserManager.  To enable
        GeoLocation information in UserManager, see the framwork.cfg
    *************************************************************************************************/
    class GeoLocationRule : public SimpleRangeRule
    {
        NON_COPYABLE(GeoLocationRule);

    public:
        GeoLocationRule(const GeoLocationRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        GeoLocationRule(const GeoLocationRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~GeoLocationRule() override;

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        /*! We need to make sure that evaluation happens if one session has the rule enabled. */
        bool isEvaluateDisabled() const override { return true; }


        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;


        const GeoLocationRuleDefinition& getDefinition() const { return static_cast<const GeoLocationRuleDefinition&>(mRuleDefinition); }


        // RETE implementations
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        bool isDedicatedServerEnabled() const override { return true; }
        // end RETE

    protected:

        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        const GeoLocationCoordinate& getLocation() const { return mLocation; }
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

    private:
        GeoLocationCoordinate mLocation;
        GeoLocationRange mAcceptableGeoLocationRange;
        
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif
