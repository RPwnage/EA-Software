/*! ************************************************************************************************/
/*!
\file   geolocationrule.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h" // for Geo IP default values
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/geolocationrule.h"
#include "gamemanager/gamebrowser/gamebrowserlist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief Rule constructor.

    \param[in] ruleDefinition - the definition object associated with this particular rule
    \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
    *************************************************************************************************/
    GeoLocationRule::GeoLocationRule(const GeoLocationRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(ruleDefinition, matchmakingAsyncStatus),
        mLocation()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Rule constructor.

        \param[in] ruleDefinition - the definition object associated with this particular rule
        \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
    *************************************************************************************************/
    GeoLocationRule::GeoLocationRule(const GeoLocationRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(otherRule, matchmakingAsyncStatus),
        mLocation(otherRule.mLocation.latitude, otherRule.mLocation.longitude)
        
    {
        mAcceptableGeoLocationRange = otherRule.mAcceptableGeoLocationRange;
    }

    /*! ************************************************************************************************/
    /*! \brief Rule destructor.  Cleanup any allocated memory
    *************************************************************************************************/
    GeoLocationRule::~GeoLocationRule() {}

    /*! ************************************************************************************************/
    /*! \brief Initialization of the rule based on the desired options of the session.

        \param[in] criteriaData - the options set in the session for the rules configuration.
        \param[in/out] err - Error object to report any problems initializing the rule
    *************************************************************************************************/
    bool GeoLocationRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        mLocation.latitude = matchmakingSupplementalData.mPrimaryUserInfo->getLatitude();
        mLocation.longitude = matchmakingSupplementalData.mPrimaryUserInfo->getLongitude();

        if (!mLocation.isValid())
        {
            TRACE_LOG("[GeoLocationRule] User is attempting to use an invalid location (" << mLocation.latitude << ", " << mLocation.longitude <<
                ") for latitude/longitude. Defaulting to (" << GEO_IP_DEFAULT_LATITUDE << ", " << GEO_IP_DEFAULT_LONGITUDE << ").");
            mLocation.latitude = GEO_IP_DEFAULT_LATITUDE;
            mLocation.longitude = GEO_IP_DEFAULT_LONGITUDE;
        }

        const GeoLocationRuleCriteria& geoCriteria = criteriaData.getGeoLocationRuleCriteria();

        const char8_t* rangeOffsetListName = geoCriteria.getMinFitThresholdName();
        if(rangeOffsetListName[0] == '\0')
        {
            // NOTE: nullptr is expected when the client disables the rule
            mRangeOffsetList = nullptr;
            return true;
        }
        else
        {
            // validate the listName for the rangeOffsetList
            mRangeOffsetList = getDefinition().getRangeOffsetList(rangeOffsetListName);
            if (mRangeOffsetList == nullptr)
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Could not find a range offset list named '%s' in rule definition '%s'", rangeOffsetListName, getDefinition().getName());
                err.setErrMessage(buf);
                return false;
            }
        }
        

        // only continue with initialization of the rule if we're not disabled.
        if (!isDisabled())
        {
            // Early out on geolocation util not being enabled
            if (!gUserSessionManager->isGeoLocationEnabled())
            {
                TRACE_LOG("[GeoLocationRule] Warning - attempt to use GeoLocation rule when GeoIp is not enabled in UserManager, all matches will fail.");
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Unable to initialize GeoLocationRule when Geo IP is not enabled on the server in UserManager. See framework.cfg.");
                err.setErrMessage(msg);
                return false;
            }

            updateCachedThresholds(0, true);
            updateAsyncStatus();
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Calculate the fit percent match of this rule against a game session. Used for calculating
        a match during a matchmaking find game, or a gamebrowser list search.

        \param[in] gameSession - the game you are evaluating against
        \param[in/out] fitPercent - The fit percent match your calculation yields.
        \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
        \param[in/out] debugRuleConditions - the debug condition information from the evaluation.
    *************************************************************************************************/
    void GeoLocationRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        GeoLocationCoordinate pt;
        pt.latitude = gameSession.getMatchmakingGameInfoCache()->getTopologyHostLocation().latitude;
        pt.longitude = gameSession.getMatchmakingGameInfoCache()->getTopologyHostLocation().longitude;
        
        if (pt.isValid())
        {
            const GeoLocationRuleDefinition &myDefn = getDefinition();
            uint32_t distance = myDefn.calculateDistance(mLocation.latitude, mLocation.longitude,
                pt.latitude, pt.longitude);
            fitPercent = myDefn.calculateFitPercent(distance);

            if (fitPercent == 1.0f)
            {
                isExactMatch = true;
                debugRuleConditions.writeRuleCondition("Host geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], closer than %" PRId64 " min range", pt.latitude, pt.longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMinRange());
            }
            else if (fitPercent == FIT_PERCENT_NO_MATCH)
            {
                isExactMatch = false;
                debugRuleConditions.writeRuleCondition("Host geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], further than %" PRId64 " max range", pt.latitude, pt.longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMaxRange());
            }
            else
            {
                isExactMatch = false;
                debugRuleConditions.writeRuleCondition("Host geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], between %" PRId64 " - %" PRId64 " range (%f)", pt.latitude, pt.longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMinRange(), getDefinition().getMaxRange(), fitPercent);
            }

        }
        else
        {
            debugRuleConditions.writeRuleCondition("Host %" PRId64 " does not have a geoIp.", gameSession.getTopologyHostInfo().getPlayerId());
            TRACE_LOG("[GeoLocationRule] Unable to find topology user session for game.");
            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Calculate the fit percent match of this rule against another rule.  Used for calculating
        a match during a matchmaking create game.

        \param[in] otherRule - The other sessions rule to calculate against
        \param[in/out] fitPercent - The fit percent match your calculation yields.
        \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
    *************************************************************************************************/
    void GeoLocationRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const GeoLocationRule& otherGeoLocationRule = static_cast<const GeoLocationRule&>(otherRule);

        const GeoLocationRuleDefinition &myDefn = static_cast<const GeoLocationRuleDefinition &>(mRuleDefinition);
        uint32_t distance = myDefn.calculateDistance(mLocation.latitude, mLocation.longitude,
            otherGeoLocationRule.getLocation().latitude, otherGeoLocationRule.getLocation().longitude);
        fitPercent = myDefn.calculateFitPercent(distance);
        
        if (fitPercent == 1.0f)
        {
            isExactMatch = true;
            debugRuleConditions.writeRuleCondition("Other geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], closer than %" PRId64 " min range", otherGeoLocationRule.getLocation().latitude, otherGeoLocationRule.getLocation().longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMinRange());
        }
        else if (fitPercent == FIT_PERCENT_NO_MATCH)
        {
            isExactMatch = false;
            debugRuleConditions.writeRuleCondition("Other geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], further than %" PRId64 " max range", otherGeoLocationRule.getLocation().latitude, otherGeoLocationRule.getLocation().longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMaxRange());
        }
        else
        {
            isExactMatch = false;
            debugRuleConditions.writeRuleCondition("Other geoIp [%f,%f] is %" PRIu32 " from my geoIp [%f,%f], between %" PRId64 " - %" PRId64 " range (%f)", otherGeoLocationRule.getLocation().latitude, otherGeoLocationRule.getLocation().longitude, distance, mLocation.latitude, mLocation.longitude, getDefinition().getMinRange(), getDefinition().getMaxRange(), fitPercent);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Clones the rule.  This is used to create the rule to match against another rule.

        \param[in] matchmakingAsyncStatus - the async status object to send notifications
    *************************************************************************************************/
    Rule* GeoLocationRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GeoLocationRule(*this, matchmakingAsyncStatus);
    }

    /*! ************************************************************************************************/
    /*! \brief Asynchronous status update of the match for the rule. GeoLocationRule updates the maximum
        distance currently being allowed for a match.
    *************************************************************************************************/
    void GeoLocationRule::updateAsyncStatus()
    {
        GeoLocationRuleStatus &myStatus = mMatchmakingAsyncStatus->getGeoLocationRuleStatus();

        if (mCurrentRangeOffset != nullptr)
            myStatus.setMaxDistance((uint32_t)mCurrentRangeOffset->mMaxValue);
    }

    /*! ************************************************************************************************/
    /*! \brief update the current threshold based upon the current time (only updates if the next threshold is reached).
    ****************************************************************************************************/
    void GeoLocationRule::updateAcceptableRange() 
    {
        getDefinition().getMaxLatAndLong(mLocation, (uint32_t)mCurrentRangeOffset->mMaxValue, mAcceptableGeoLocationRange);
    }

    bool GeoLocationRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if(!isDisabled())
        {

            if(!mAcceptableGeoLocationRange.isValid())
            {
                // log error for invalid range score list
                return false;
            }

            Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
            Rete::OrConditionList& geoLocationLatitudeOrConds = conditions.push_back();

            geoLocationLatitudeOrConds.push_back(Rete::ConditionInfo(
                Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LATITUDE_RETE_KEY, 
                (int64_t)mAcceptableGeoLocationRange.southMax, (int64_t)mAcceptableGeoLocationRange.northMax, 
                mRuleDefinition),
                0, this
                ));

            Rete::OrConditionList& geoLocationLongitudeOrConds = conditions.push_back();

            // Range searches need to constrain by the input values which are between +-180
            // but can go around the world.  Eg 180+40 = 220 = -140
            float trimmedWest = mAcceptableGeoLocationRange.westMax;
            float trimmedEast = mAcceptableGeoLocationRange.eastMax;

            // East is always less than our actual point.
            if (trimmedEast <= -180)
            {
                trimmedEast = 360 + trimmedEast;
                if (trimmedEast < -180)
                {
                    // hmm, one loop around the world wasn't enough
                    return true;
                }

                geoLocationLongitudeOrConds.push_back(Rete::ConditionInfo(
                    Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
                    (int64_t)trimmedEast, (int64_t)180, 
                    mRuleDefinition),
                    0, this
                    ));


                geoLocationLongitudeOrConds.push_back(Rete::ConditionInfo(
                    Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
                    (int64_t)-180, (int64_t)mAcceptableGeoLocationRange.westMax, 
                    mRuleDefinition),
                    0, this
                    ));

            }
            else if (trimmedWest > 180)
            {
                trimmedWest = -360 + trimmedWest;
                if (trimmedWest > 180)
                {
                    // hmmm, one loop around the world wasn't enough.
                    return true;
                }

                geoLocationLongitudeOrConds.push_back(Rete::ConditionInfo(
                    Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
                    (int64_t)mAcceptableGeoLocationRange.eastMax, (int64_t)180, 
                    mRuleDefinition),
                    0, this
                    ));

                geoLocationLongitudeOrConds.push_back(Rete::ConditionInfo(
                    Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
                    (int64_t)-180, (int64_t)trimmedWest, 
                    mRuleDefinition),
                    0, this
                    ));

            }
            else
            {
                // We didn't crack the +/- 180 barrier, just use a single range.
                geoLocationLongitudeOrConds.push_back(Rete::ConditionInfo(
                    Rete::Condition(GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
                    (int64_t)mAcceptableGeoLocationRange.eastMax, (int64_t)mAcceptableGeoLocationRange.westMax, 
                    mRuleDefinition),
                    0, this
                    ));
            }

        }

        return true;
    }

    // Override this from Rule since we use range thresholds instead of fit thresholds
    void GeoLocationRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[GeoLocationRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
            EA_ASSERT(mRangeOffsetList != nullptr);
            return;
        }

        // Check exact match (this indicates that the distance was < than the minimum)
        if (evalInfo.mIsExactMatch && 
            mCurrentRangeOffset != nullptr)
        {
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight()), 0);
            return;
        }

        // We are doing the distance calculation twice, because SimpleRule calls calcFitPercent.
        const GeoLocationRule& otherGeoLocationRule = static_cast<const GeoLocationRule&>(otherRule);
        float distance = (float)getDefinition().calculateDistance(mLocation.latitude, mLocation.longitude,
            otherGeoLocationRule.getLocation().latitude, otherGeoLocationRule.getLocation().longitude);

        // Indicates an exact match: (should already handled above, this is just a sanity check)
        if ((int32_t)distance <= getDefinition().getMinRange())
        {
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight()), 0);
            return;
        }

        RangeList::Range acceptableRange;
        const RangeList::RangePairContainer &rangePairContainer = mRangeOffsetList->getContainer();
        RangeList::RangePairContainer::const_iterator iter = rangePairContainer.begin();
        RangeList::RangePairContainer::const_iterator end = rangePairContainer.end();
        for ( ; iter != end; ++iter )
        {
            uint32_t matchSeconds = iter->first;
            const RangeList::Range &rangeOffset = iter->second;
            acceptableRange.setRange(0, rangeOffset, getDefinition().getMinRange(), getDefinition().getMaxRange());
            if (acceptableRange.isInRange((int32_t)distance))
            {
                FitScore fitScore = static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight());
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

