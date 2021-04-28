/*! ************************************************************************************************/
/*!
    \file timetomatchestimator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmakercomponent/timetomatchestimator.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace Matchmaker
{
    EA::TDF::TimeValue getScenarioTotalDuration(GameManager::ScenarioConfig& scenarioConfig)
    {
        // Iterate over all subsessions, use the greatest session end time, or total duration:
        EA::TDF::TimeValue endTime = scenarioConfig.getTotalDuration();

        for (auto& curSession : scenarioConfig.getSubSessions())
        {
            EA::TDF::TimeValue subsessionEndTime = curSession.second->getSessionEndTime();
            if (subsessionEndTime > endTime)
                endTime = subsessionEndTime;
        }

        return endTime;
    }

    // The census estimate time goes from the MM collated average to the global/scenario time, over the course of the scenario's falloff window.
    // (Ex. 15s scenario with 30 min cutoff.  Last Estimate at curTime = Use Last Estimate, Last Estimate is 30 min old = Use 15s, Last Estimate is 15m old = Avg(Estimate, 15s))
    TimeValue blendTimeValues(const TimeToMatchEstimator::TimeToMatchEstimate& ttmEstimate, TimeValue curTimeOfDay, TimeValue cutoffTime, TimeValue defaultEstimate)
    {
        float oldFactor = (float)(curTimeOfDay - ttmEstimate.mLastDatapointTimestamp).getMillis() / (float)cutoffTime.getMillis();
        oldFactor = (oldFactor > 1.0f) ? 1.0f : oldFactor;
        return TimeValue((int64_t)(ttmEstimate.mEstimatedTimeToMatch.getMicroSeconds() * (1.0f - oldFactor) + defaultEstimate.getMicroSeconds() * oldFactor));
    }

    TimeValue TimeToMatchEstimator::getTimeToMatchEstimate(const GameManager::ScenarioName& scenarioName, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup,
        const GameManager::ScenariosConfig &scenariosConfig) const
    {
        // find the scenario
        TimeToMatchEstimatesByScenario::const_iterator scenarioIter = mScenarioTTMEstimates.find(scenarioName);
        if (scenarioIter != mScenarioTTMEstimates.end())
        {
            const ScenarioTimeToMatchEstimate& ttmEstimate = scenarioIter->second;
            bool found = false;
            TimeToMatchEstimate ttmEstimateCopy;
            const TimeToMatchEstimate* scenarioTtmEstimate = getScenarioTtmEstimate(ttmEstimate, pingSite, delineationGroup);
            if (scenarioTtmEstimate != nullptr)
            {
                ttmEstimateCopy = *scenarioTtmEstimate;
                found = true;
            }
            
            if (!found)
            {
                TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimate: attempt to get estimate for scenario '" << scenarioName
                    << "' in ping site '" << pingSite << "', delineation group '"<< delineationGroup <<"' failed, using global scenario estimate.");
                ttmEstimateCopy = ttmEstimate.mGlobalTimeToMatchEstimate;
            }

            TimeValue matchEstimateFalloffWindow;
            TimeValue scenarioTimeoutDuration;
            GameManager::ScenarioMap::const_iterator scenarioConfigIter = scenariosConfig.getScenarios().find(scenarioName);
            if (scenarioConfigIter != scenariosConfig.getScenarios().end())
            {
                matchEstimateFalloffWindow = scenarioConfigIter->second->getMatchEstimateFalloffWindow();
                scenarioTimeoutDuration = getScenarioTotalDuration(*scenarioConfigIter->second);
            }
            else
            {
                matchEstimateFalloffWindow = scenariosConfig.getGlobalMatchEstimateFalloffWindow();
                scenarioTimeoutDuration = ttmEstimateCopy.mEstimatedTimeToMatch;  // If the scenario no longer exists, just keep the old estimate the same.
            }

            if (ttmEstimateCopy.mEstimatedTimeToMatch != scenarioTimeoutDuration)
            {
                // now adjust for the age of the estimate, if we know the total duration, otherwise we'll use the stored value
                ttmEstimateCopy.mEstimatedTimeToMatch = blendTimeValues(ttmEstimateCopy, TimeValue::getTimeOfDay(), matchEstimateFalloffWindow, scenarioTimeoutDuration);
            }
           
            return ttmEstimateCopy.mEstimatedTimeToMatch;
        }
        
        // if we don't have a scenario ttm to use, just return the raw estimate without an age correction
        TimeToMatchEstimatesByPingsiteGroup::const_iterator globalTtmByPingSiteIter = mGlobalTTMEstimates.find(pingSite);
        if (globalTtmByPingSiteIter != mGlobalTTMEstimates.end() && globalTtmByPingSiteIter->second.find("") != globalTtmByPingSiteIter->second.end())
        {
            TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimate: attempt to get estimate for scenario '" << scenarioName 
                << "' in ping site '" << pingSite << "' failed, using global ping site estimate.");
            return globalTtmByPingSiteIter->second.find("")->second.mEstimatedTimeToMatch;
        }

        // use global estimate for unknown scenario & ping site.
        TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimate: attempt to get estimate for scenario '" << scenarioName 
            << "' in ping site '" << pingSite << "' failed, using global overall estimate.");
        return mGlobalTtmEstimate.mEstimatedTimeToMatch;
    }

    TimeValue TimeToMatchEstimator::getTimeToMatchEstimateForPacker(const GameManager::ScenarioName& scenarioName, const TimeValue& scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup,
        const TimeValue& scenarioMatchEstimateFalloffWindow, const TimeValue& globalMatchEstimateFalloffWindow) const
    {
        // find the scenario
        TimeToMatchEstimatesByScenario::const_iterator scenarioIter = mScenarioTTMEstimates.find(scenarioName);
        if (scenarioIter != mScenarioTTMEstimates.end())
        {
            const ScenarioTimeToMatchEstimate& ttmEstimate = scenarioIter->second;
            bool found = false;
            TimeToMatchEstimate ttmEstimateCopy;
            const TimeToMatchEstimate* scenarioTtmEstimate = getScenarioTtmEstimate(ttmEstimate, pingSite, delineationGroup);
            if (scenarioTtmEstimate != nullptr)
            {
                ttmEstimateCopy = *scenarioTtmEstimate;
                found = true;
            }

            if (!found)
            {
                TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimateForPacker: attempt to get estimate for scenario '" << scenarioName
                    << "' in ping site '" << pingSite << "', delineation group '" << delineationGroup << "' failed, using global scenario estimate.");
                ttmEstimateCopy = ttmEstimate.mGlobalTimeToMatchEstimate;
            }

            TimeValue matchEstimateFalloffWindow;
            TimeValue scenarioTimeoutDuration;
            if (scenarioName.empty())
            { 
                matchEstimateFalloffWindow = globalMatchEstimateFalloffWindow;
                scenarioTimeoutDuration = ttmEstimateCopy.mEstimatedTimeToMatch; 
            }
            else
            {
                matchEstimateFalloffWindow = scenarioMatchEstimateFalloffWindow;
                scenarioTimeoutDuration = scenarioDuration;
            }

            if (ttmEstimateCopy.mEstimatedTimeToMatch != scenarioTimeoutDuration)
            {
                // now adjust for the age of the estimate, if we know the total duration, otherwise we'll use the stored value
                ttmEstimateCopy.mEstimatedTimeToMatch = blendTimeValues(ttmEstimateCopy, TimeValue::getTimeOfDay(), matchEstimateFalloffWindow, scenarioTimeoutDuration);
            }

            return ttmEstimateCopy.mEstimatedTimeToMatch;
        }

        // if we don't have a scenario ttm to use, try returning global ping site estimate
        TimeToMatchEstimatesByPingsiteGroup::const_iterator globalTtmByPingSiteIter = mGlobalTTMEstimates.find(pingSite);
        if (globalTtmByPingSiteIter != mGlobalTTMEstimates.end() && globalTtmByPingSiteIter->second.find(delineationGroup) != globalTtmByPingSiteIter->second.end())
        {
            TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimate: attempt to get estimate for scenario '" << scenarioName
                << "' in ping site '" << pingSite << "' failed, using global ping site estimate.");
            return globalTtmByPingSiteIter->second.find(delineationGroup)->second.mEstimatedTimeToMatch;
        }

        // otherwise, just return the total scenario duration
        TRACE_LOG("[TimeToMatchEstimator].getTimeToMatchEstimateForPacker: attempt to get estimate for scenario '" << scenarioName
            << "' in ping site '" << pingSite << "' failed, using total scenario duration instead.");
        return scenarioDuration;
    }

    const TimeToMatchEstimator::TimeToMatchEstimate* TimeToMatchEstimator::getScenarioTtmEstimate(const ScenarioTimeToMatchEstimate& scenarioTtmEstimate, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup) const
    {
        const TimeToMatchEstimate* ttmEstimate = nullptr;
        const auto ttmByPingSiteIter = scenarioTtmEstimate.mTimesToMatchByPingsiteGroup.find(pingSite);
        if (ttmByPingSiteIter != scenarioTtmEstimate.mTimesToMatchByPingsiteGroup.end())
        {
            const auto ttmByDGIter = ttmByPingSiteIter->second.find(delineationGroup);
            if (ttmByDGIter != ttmByPingSiteIter->second.end())
            {
                ttmEstimate = &(ttmByDGIter->second);
            }
        }
        return ttmEstimate;
    }

    void TimeToMatchEstimator::updateTimeToMatchEstimate(TimeToMatchEstimate &ttmEstimate, const TimeValue &scenarioDuration, const TimeValue &weightFactor, 
        const TimeValue &referenceTime /*= TimeValue::getTimeOfDay()*/, GameManager::MatchmakingResult result) const
    {
        // Special cases for a various MM results: 
        switch (result)
        {
        case GameManager::SESSION_TERMINATED:                // Terminated by Blaze reconfiguration - no ttm update 
        case GameManager::SUCCESS_PSEUDO_CREATE_GAME:        // Debug command - ignore results. 
        case GameManager::SUCCESS_PSEUDO_FIND_GAME:          // Debug command - ignore results. 
            return;
        case GameManager::SESSION_CANCELED:
        {
            // We only update the time to increase it, otherwise we maintain it:
            if (scenarioDuration < ttmEstimate.mEstimatedTimeToMatch)
                return;
            break;
        }
        default:            // Includes SUCCESS_*_GAME, SESSION_TIMED_OUT, and SESSION_*_FAILED  (Failure cases still found a game, so they have a valid TTM)
            break;
        }
        TimeValue timeSinceLastSample;
        double ttmEma = 0;

        if (referenceTime >= ttmEstimate.mLastDatapointTimestamp)
        {
            timeSinceLastSample = referenceTime - ttmEstimate.mLastDatapointTimestamp;
            // exponential moving average for irregular time series
            ttmEma = calculateTtmEstimateMillis(scenarioDuration, ttmEstimate.mLastTimeToMatch, ttmEstimate.mEstimatedTimeToMatch, timeSinceLastSample, weightFactor);
            // only update timestamp and last ttm if the datapoint is newer
            ttmEstimate.mLastDatapointTimestamp = referenceTime;
            ttmEstimate.mLastTimeToMatch = scenarioDuration;
        }
        else
        {
            // we may have to calculate using samples that are out-of-order when building the census data
            timeSinceLastSample = ttmEstimate.mLastDatapointTimestamp - referenceTime;
            // exponential moving average for irregular time series
            // flipping sLast and current duration because sLast is newer.
            ttmEma = calculateTtmEstimateMillis(ttmEstimate.mLastTimeToMatch, scenarioDuration, ttmEstimate.mEstimatedTimeToMatch, timeSinceLastSample, weightFactor);
        }

        
        int64_t ttmMilliseconds = (int64_t)ceil(ttmEma);
        // update the struct
        ttmEstimate.mEstimatedTimeToMatch.setMillis(ttmMilliseconds);
    }


    /*! ************************************************************************************************/
    /*! \brief Calculate a new time to match estimate
    - Using an exponential moving average for an irregular time series to weight a new datapoint with the current average, 
        and the last recorded datapoint (based on their age.)

    \param[in] latestTtm - the newest datapoint
    \param[in] lastTtm - the previous datapoint
    \param[in] currentTtmEstimate - last calculated ttm estimate
    \param[in] timeSinceLastSample - age of the last datapoint
    \param[in] weightFactor - the age at which the newest datapoint will count for approximately 2/3 of the weight in the new calculation
    \return the newly calculated estimate in milliseconds
    ***************************************************************************************************/
    double TimeToMatchEstimator::calculateTtmEstimateMillis(const TimeValue &latestTtm, const TimeValue& lastTtm, const TimeValue& currentTtmEstimate, 
        const TimeValue &timeSinceLastSample, const TimeValue &weightFactor) const
    {
        if (weightFactor.getMillis() == 0)
        {

            // protect against div/0
            return (double)currentTtmEstimate.getMillis();
        }

        double a;
        // if time difference is 0, set to 1ms.
        if (timeSinceLastSample.getMillis() != 0)
        {
            a = ((double)timeSinceLastSample.getMillis() / weightFactor.getMillis());
        }
        else
        {
            a = 1.0 / weightFactor.getMillis();
        }

        double u = exp(a * -1);
        double v = (1 - u) / a;
        double result = 0;
        result = (u * currentTtmEstimate.getMillis()) + ((v - u) * lastTtm.getMillis()) + ((1.0 - v) * latestTtm.getMillis());
        
        return result;
    }

    void TimeToMatchEstimator::updateScenarioTimeToMatch(const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup, const GameManager::ScenarioName &scenarioName,
        const GameManager::ScenariosConfig &scenariosConfig, GameManager::MatchmakingResult result)
    {
        // find the scenario
        TimeValue scenarioMatchEstimateFalloffWindow;
        GameManager::ScenarioMap::const_iterator scenarioConfigIter = scenariosConfig.getScenarios().find(scenarioName);
        if (scenarioConfigIter != scenariosConfig.getScenarios().end())
        {
            scenarioMatchEstimateFalloffWindow = scenarioConfigIter->second->getMatchEstimateFalloffWindow();
        }
        else
        {
            scenarioMatchEstimateFalloffWindow = scenariosConfig.getGlobalMatchEstimateFalloffWindow();
        }

        updateScenarioTimeToMatch(scenarioName, scenarioDuration, pingSite, delineationGroup, scenarioMatchEstimateFalloffWindow, scenariosConfig.getGlobalMatchEstimateFalloffWindow(), result);
    }

    void TimeToMatchEstimator::updateScenarioTimeToMatch(const GameManager::ScenarioName &scenarioName, const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup,
        const TimeValue& scenarioMatchEstimateFalloffWindow, const TimeValue& globalMatchEstimateFalloffWindow, GameManager::MatchmakingResult result)
    {
        TimeToMatchEstimatesByScenario::iterator scenarioIter = mScenarioTTMEstimates.find(scenarioName);
        if (scenarioIter != mScenarioTTMEstimates.end())
        {
            // global update for this value
            ScenarioTimeToMatchEstimate &scenarioTtmEstimate = scenarioIter->second;
            updateTimeToMatchEstimate(scenarioTtmEstimate.mGlobalTimeToMatchEstimate, scenarioDuration, scenarioMatchEstimateFalloffWindow, TimeValue::getTimeOfDay(), result);

            // Set the Base values if this is a new estimate:
            auto& ttmByPingsite = scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[pingSite];
            if (ttmByPingsite.find(delineationGroup) == ttmByPingsite.end())
                scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[pingSite][delineationGroup] = scenarioTtmEstimate.mBaseTimeToMatchEstimate;

            // ping-site update
            updateTimeToMatchEstimate(scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[pingSite][delineationGroup], scenarioDuration, scenarioMatchEstimateFalloffWindow, TimeValue::getTimeOfDay(), result);
        }
        else
        {
            // add the scenario to the map, initializing with the current scenario's TTM across the board.
            intializeScenarioTtm(scenarioName, scenarioDuration, 0);
        }

        // update it's global TTM
        updateNonScenarioTimeToMatch(scenarioDuration, pingSite, delineationGroup, globalMatchEstimateFalloffWindow, result);
    }

    void TimeToMatchEstimator::updateNonScenarioTimeToMatch(const TimeValue &matchmakingDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup,
        const TimeValue& globalMatchEstimateFalloffWindow, GameManager::MatchmakingResult result)
    {
        updateTimeToMatchEstimate(mGlobalTtmEstimate, matchmakingDuration, globalMatchEstimateFalloffWindow, TimeValue::getTimeOfDay(), result);
        updateTimeToMatchEstimate(mGlobalTTMEstimates[pingSite][""], matchmakingDuration, globalMatchEstimateFalloffWindow, TimeValue::getTimeOfDay(), result);
    }

    /*! ************************************************************************************************/
    /*! \brief clears existing TTM estimates, and rebuilds map based on server config
    *************************************************************************************************/
    void TimeToMatchEstimator::initializeTtmEstimates(const GameManager::ScenariosConfig &scenariosConfig)
    {
        // clear any existing estimates
        mGlobalTTMEstimates.clear();
        mScenarioTTMEstimates.clear();
        // if there are no scenarios, we start the TTM estimate at 0.
        TimeValue globalTtmEstimate = 0;
        // add TTM estimate for each ping site
        if (!scenariosConfig.getScenarios().empty())
        {
            for(auto& scenarioIter : scenariosConfig.getScenarios())
            {
                TimeValue scenarioTotalDuration = getScenarioTotalDuration(*scenarioIter.second);
                intializeScenarioTtm(scenarioIter.first, scenarioTotalDuration, 0);
                if (scenarioTotalDuration > globalTtmEstimate)
                    globalTtmEstimate = scenarioTotalDuration;
            }
        }

        mGlobalTtmEstimate.mEstimatedTimeToMatch = globalTtmEstimate;
        mGlobalTtmEstimate.mLastTimeToMatch = globalTtmEstimate;
        mGlobalTtmEstimate.mLastDatapointTimestamp = 0;

        // now fill out the ping site estimates 
        Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasIter = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().begin();
        Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasEnd = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().end();
        for(; pingSiteAliasIter != pingSiteAliasEnd; ++pingSiteAliasIter)
        {
            mGlobalTTMEstimates[pingSiteAliasIter->first][""] = mGlobalTtmEstimate;
        }
    }

    void TimeToMatchEstimator::initializeTtmEstimatesForPackerScenario(const Blaze::GameManager::ScenarioName& scenarioName, const TimeValue& scenarioTotalDuration)
    {
        // If it's the first scenario init
        if (mGlobalTtmEstimate.mEstimatedTimeToMatch == 0)
        {
            mGlobalTtmEstimate.mEstimatedTimeToMatch = scenarioTotalDuration;
            mGlobalTtmEstimate.mLastTimeToMatch = scenarioTotalDuration;
            mGlobalTtmEstimate.mLastDatapointTimestamp = 0;

            Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasIter = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().begin();
            Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasEnd = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().end();
            for (; pingSiteAliasIter != pingSiteAliasEnd; ++pingSiteAliasIter)
            {
                mGlobalTTMEstimates[pingSiteAliasIter->first][""] = mGlobalTtmEstimate;
            }
        }
        
        if (mScenarioTTMEstimates.find(scenarioName) == mScenarioTTMEstimates.end())
        {
            intializeScenarioTtm(scenarioName, scenarioTotalDuration, 0);
        }
    }

    void TimeToMatchEstimator::initializeTtmEstimates(const GameManager::MatchmakingInstanceStatusResponse& matchmakingInstanceStatusResponse)
    {
        initializeTtmEstimates(matchmakingInstanceStatusResponse.getGlobalTimeToMatchEstimateData(), matchmakingInstanceStatusResponse.getScenarioTimeToMatchEstimateData());
    }

    void TimeToMatchEstimator::initializeTtmEstimates(const GameManager::PackerInstanceStatusResponse& packerInstanceStatusResponse)
    {
        initializeTtmEstimates(packerInstanceStatusResponse.getGlobalTimeToMatchEstimateData(), packerInstanceStatusResponse.getScenarioTimeToMatchEstimateData());
    }

    void TimeToMatchEstimator::initializeTtmEstimates(const GameManager::InstanceTimeToMatchEstimateData& globalTimeToMatchEstimateData, const GameManager::TimeToMatchEstimateDataPerScenario& scenarioTimeToMatchEstimateData)
    {
        // clear any existing estimates
        mGlobalTTMEstimates.clear();
        mScenarioTTMEstimates.clear();

        mGlobalTtmEstimate.mLastTimeToMatch = globalTimeToMatchEstimateData.getGlobalEstimatedTimeToMatch();
        mGlobalTtmEstimate.mEstimatedTimeToMatch = globalTimeToMatchEstimateData.getGlobalEstimatedTimeToMatch();
        mGlobalTtmEstimate.mLastDatapointTimestamp = globalTimeToMatchEstimateData.getLastMatchTimestamp();

        // No good way to combine estimates when multiple groups are used:
        for (const auto& curPingSiteEstimate : globalTimeToMatchEstimateData.getEstimatedTimeToMatchPerPingsiteGroup())
        {
            for (const auto& curGroupEstimate : *curPingSiteEstimate.second)
            {
                TimeToMatchEstimate& localTTMEstimate = mGlobalTTMEstimates[curPingSiteEstimate.first][curGroupEstimate.first];
                localTTMEstimate.mLastTimeToMatch = curGroupEstimate.second->getEstimatedTimeToMatch();
                localTTMEstimate.mEstimatedTimeToMatch = curGroupEstimate.second->getEstimatedTimeToMatch();
                localTTMEstimate.mLastDatapointTimestamp = curGroupEstimate.second->getEstimateTimeStamp();
            }
        }

        for (const auto& curScenario : scenarioTimeToMatchEstimateData)
        {
            const GameManager::ScenarioName& scenarioName = curScenario.first;

            ScenarioTimeToMatchEstimate &scenarioTtmEstimate = mScenarioTTMEstimates[scenarioName];
            scenarioTtmEstimate.mScenarioName = scenarioName;
            scenarioTtmEstimate.mGlobalTimeToMatchEstimate.mLastTimeToMatch = curScenario.second->getGlobalEstimatedTimeToMatch();
            scenarioTtmEstimate.mGlobalTimeToMatchEstimate.mEstimatedTimeToMatch = curScenario.second->getGlobalEstimatedTimeToMatch();
            scenarioTtmEstimate.mGlobalTimeToMatchEstimate.mLastDatapointTimestamp = curScenario.second->getLastMatchTimestamp();

            /*! ***************************************************************/
            for (const auto& curPingSiteEstimate : curScenario.second->getEstimatedTimeToMatchPerPingsiteGroup())
            {
                for (const auto& curGroupEstimate : *curPingSiteEstimate.second)
                {
                    TimeToMatchEstimate& localTTMEstimate = scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[curPingSiteEstimate.first][curGroupEstimate.first];
                    localTTMEstimate.mLastTimeToMatch = curGroupEstimate.second->getEstimatedTimeToMatch();
                    localTTMEstimate.mEstimatedTimeToMatch = curGroupEstimate.second->getEstimatedTimeToMatch();
                    localTTMEstimate.mLastDatapointTimestamp = curGroupEstimate.second->getEstimateTimeStamp();
                }
            }
        }
    }

    void TimeToMatchEstimator::intializeScenarioTtm(const Blaze::GameManager::ScenarioName& scenarioName, const TimeValue& scenarioTimeoutDuration, const TimeValue& initialDatapointTimestamp)
    {
        ScenarioTimeToMatchEstimate &scenarioTtmEstimate = mScenarioTTMEstimates[scenarioName];
        scenarioTtmEstimate.mScenarioName = scenarioName;
        
        // Set the Base data, used when new entries are added:
        scenarioTtmEstimate.mBaseTimeToMatchEstimate.mEstimatedTimeToMatch = scenarioTimeoutDuration;
        scenarioTtmEstimate.mBaseTimeToMatchEstimate.mLastTimeToMatch = scenarioTimeoutDuration;
        scenarioTtmEstimate.mBaseTimeToMatchEstimate.mLastDatapointTimestamp = initialDatapointTimestamp;

        // Set the global:
        scenarioTtmEstimate.mGlobalTimeToMatchEstimate = scenarioTtmEstimate.mBaseTimeToMatchEstimate;

        // Clear any existing TTMs:
        scenarioTtmEstimate.mTimesToMatchByPingsiteGroup.clear();

        // now fill out the ping site estimates 
        Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasIter = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().begin();
        Blaze::PingSiteInfoByAliasMap::const_iterator pingSiteAliasEnd = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap().end();
        for (; pingSiteAliasIter != pingSiteAliasEnd; ++pingSiteAliasIter)
        {
            scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[pingSiteAliasIter->first][""] = scenarioTtmEstimate.mBaseTimeToMatchEstimate;
        }
    }

    void TimeToMatchEstimator::clearTtmEstimates()
    {
        mGlobalTTMEstimates.clear();
        mScenarioTTMEstimates.clear();
        mGlobalTtmEstimate.mEstimatedTimeToMatch = 0;
        mGlobalTtmEstimate.mLastTimeToMatch = 0;
        mGlobalTtmEstimate.mLastDatapointTimestamp = 0;
    }

    void TimeToMatchEstimator::populateMatchmakingCensusData(GameManager::InstanceTimeToMatchEstimateData &globalTtmData, 
        GameManager::TimeToMatchEstimateDataPerScenario &scenarioTtmData) const
    {
        globalTtmData.setGlobalEstimatedTimeToMatch(mGlobalTtmEstimate.mEstimatedTimeToMatch);
        globalTtmData.setLastMatchTimestamp(mGlobalTtmEstimate.mLastDatapointTimestamp);

        // insert overall ping site data
        TimeToMatchEstimatesByPingsiteGroup::const_iterator globalTtmByPingsiteGroupIter = mGlobalTTMEstimates.begin();
        for(; globalTtmByPingsiteGroupIter != mGlobalTTMEstimates.end(); ++globalTtmByPingsiteGroupIter)
        {
            auto ttmEstimateData = globalTtmData.getEstimatedTimeToMatchPerPingsiteGroup().allocate_element();
            globalTtmData.getEstimatedTimeToMatchPerPingsiteGroup()[globalTtmByPingsiteGroupIter->first] = ttmEstimateData;

            for (auto& groupIter : globalTtmByPingsiteGroupIter->second)
            {
                auto ttmGroupEstimateData = ttmEstimateData->allocate_element();
                ttmGroupEstimateData->setEstimatedTimeToMatch(groupIter.second.mEstimatedTimeToMatch);
                ttmGroupEstimateData->setEstimateTimeStamp(groupIter.second.mLastDatapointTimestamp);

                (*ttmEstimateData)[groupIter.first] = ttmGroupEstimateData;
            }
        }

        TimeToMatchEstimatesByScenario::const_iterator scenarioTtmIterator = mScenarioTTMEstimates.begin();
        for(; scenarioTtmIterator != mScenarioTTMEstimates.end(); ++scenarioTtmIterator)
        {
            GameManager::InstanceTimeToMatchEstimateDataPtr scenarioTtmEstimateData = scenarioTtmData.allocate_element();
            scenarioTtmEstimateData->setGlobalEstimatedTimeToMatch(scenarioTtmIterator->second.mGlobalTimeToMatchEstimate.mEstimatedTimeToMatch);
            scenarioTtmEstimateData->setLastMatchTimestamp(scenarioTtmIterator->second.mGlobalTimeToMatchEstimate.mLastDatapointTimestamp);
            scenarioTtmData[scenarioTtmIterator->first] = scenarioTtmEstimateData;

            for (const auto& scenarioTtmByPingSiteIter : scenarioTtmIterator->second.mTimesToMatchByPingsiteGroup)
            {
                auto scenarioPingSiteTtmEstimateData = scenarioTtmEstimateData->getEstimatedTimeToMatchPerPingsiteGroup().allocate_element();
                scenarioTtmEstimateData->getEstimatedTimeToMatchPerPingsiteGroup()[scenarioTtmByPingSiteIter.first] = scenarioPingSiteTtmEstimateData;

                // ping site for this scenario
                for (const auto& groupData : scenarioTtmByPingSiteIter.second)
                {
                    auto groupTtmEstimate = scenarioPingSiteTtmEstimateData->allocate_element();

                    groupTtmEstimate->setEstimatedTimeToMatch(groupData.second.mEstimatedTimeToMatch);
                    groupTtmEstimate->setEstimateTimeStamp(groupData.second.mLastDatapointTimestamp);

                    (*scenarioPingSiteTtmEstimateData)[groupData.first] = groupTtmEstimate;
                }
            }
        }

    }

    // This function combines the existing TTM data with the incoming data from another MM slave: 
    void TimeToMatchEstimator::collateMatchmakingCensusData(const GameManager::InstanceTimeToMatchEstimateData &globalTtmData, 
        const GameManager::TimeToMatchEstimateDataPerScenario &scenarioTtmData, 
        const GameManager::ScenariosConfig &scenariosConfig)
    {
        // calculate global data
        updateTimeToMatchEstimate(mGlobalTtmEstimate, globalTtmData.getGlobalEstimatedTimeToMatch(), scenariosConfig.getGlobalMatchEstimateFalloffWindow(), globalTtmData.getLastMatchTimestamp());
        
        // calculate overall ping site data
        auto globalTtmByPingSiteIter = globalTtmData.getEstimatedTimeToMatchPerPingsiteGroup().begin();
        for (; globalTtmByPingSiteIter != globalTtmData.getEstimatedTimeToMatchPerPingsiteGroup().end(); ++globalTtmByPingSiteIter)
        {
            GameManager::TimeToMatchEstimateDataPerGroup::const_iterator globalTtmByGroupIter = globalTtmByPingSiteIter->second->begin();
            for (; globalTtmByGroupIter != globalTtmByPingSiteIter->second->end(); ++globalTtmByGroupIter)
            {
                TimeToMatchEstimate& ttmEstimate = mGlobalTTMEstimates[globalTtmByPingSiteIter->first][""];
                updateTimeToMatchEstimate(ttmEstimate, globalTtmByGroupIter->second->getEstimatedTimeToMatch(), scenariosConfig.getGlobalMatchEstimateFalloffWindow(), globalTtmByGroupIter->second->getEstimateTimeStamp());
            }
        }

        // do per-scenario data
        GameManager::TimeToMatchEstimateDataPerScenario::const_iterator scenarioTtmIterator = scenarioTtmData.begin();
        for(; scenarioTtmIterator != scenarioTtmData.end(); ++scenarioTtmIterator)
        {
            TimeValue matchEstimateFalloffWindow;
            GameManager::ScenarioMap::const_iterator scenarioConfigIter = scenariosConfig.getScenarios().find(scenarioTtmIterator->first);
            if (scenarioConfigIter != scenariosConfig.getScenarios().end())
            {
                matchEstimateFalloffWindow = scenarioConfigIter->second->getMatchEstimateFalloffWindow();
            }
            else
            {
                matchEstimateFalloffWindow = scenariosConfig.getGlobalMatchEstimateFalloffWindow();
            }

            ScenarioTimeToMatchEstimate& scenarioTtmEstimate = mScenarioTTMEstimates[scenarioTtmIterator->first];
            updateTimeToMatchEstimate(scenarioTtmEstimate.mGlobalTimeToMatchEstimate, scenarioTtmIterator->second->getGlobalEstimatedTimeToMatch(), matchEstimateFalloffWindow, scenarioTtmIterator->second->getLastMatchTimestamp());


            for (auto& scenarioTtmByPingSiteIter : scenarioTtmIterator->second->getEstimatedTimeToMatchPerPingsiteGroup())
            {
                auto& scenarioPsTtmEstimate = scenarioTtmEstimate.mTimesToMatchByPingsiteGroup[scenarioTtmByPingSiteIter.first];

                // ping site for this scenario
                for (auto& scenarioTtmByGroupIter : *scenarioTtmByPingSiteIter.second)
                {
                    TimeToMatchEstimate& scenarioPingSiteTtmEstimate = scenarioPsTtmEstimate[scenarioTtmByGroupIter.first];
                    updateTimeToMatchEstimate(scenarioPingSiteTtmEstimate, scenarioTtmByGroupIter.second->getEstimatedTimeToMatch(), matchEstimateFalloffWindow, scenarioTtmByGroupIter.second->getEstimateTimeStamp());
                }
            }
        }
    }

    void TimeToMatchEstimator::getMatchmakingCensusData(GameManager::MatchmakingCensusData &mmCensusData, const GameManager::ScenariosConfig &scenariosConfig) const
    {
        // if there are no scenarios, we start the TTM estimate at 0.
        TimeValue defaultTtmEstimate = 0;
        TimeValue scenarioCutoffTime = 0;
        TimeValue curTimeOfDay = TimeValue::getTimeOfDay();
        // add TTM estimate for each ping site
        for (auto& scenarioIter : scenariosConfig.getScenarios())
        {
            TimeValue scenarioTotalDuration = getScenarioTotalDuration(*scenarioIter.second);
            if (scenarioTotalDuration > defaultTtmEstimate)
                defaultTtmEstimate = scenarioTotalDuration;

            if (scenarioIter.second->getMatchEstimateFalloffWindow() > scenarioCutoffTime)
                scenarioCutoffTime = scenarioIter.second->getMatchEstimateFalloffWindow();
        }

        // calculate overall ping site data
        for (auto& globalTtmByPingSiteIter : mGlobalTTMEstimates)
        {
            for (auto& globalTtmByGroupIter : globalTtmByPingSiteIter.second)
            {
                // The insert here is fine because we're the only thing touching the EstimatedTimeToMatchPerPingSite (otherwise we'd need to use find() first)
                TimeValue estTime = blendTimeValues(globalTtmByGroupIter.second, curTimeOfDay, scenarioCutoffTime, defaultTtmEstimate);
                
                auto pingsiteGroupElem = mmCensusData.getEstimatedTimeToMatchPerPingsiteGroup().allocate_element();
                (*pingsiteGroupElem)[""] = estTime;
                mmCensusData.getEstimatedTimeToMatchPerPingsiteGroup()[globalTtmByPingSiteIter.first] = pingsiteGroupElem;
            }
        }

        // do per-scenario data
        for (auto& scenarioTtmIterator : mScenarioTTMEstimates)
        {
            const GameManager::ScenarioName& scenarioName = scenarioTtmIterator.first;
            auto scenarioConfig = scenariosConfig.getScenarios().find(scenarioName);
            if (scenarioConfig == scenariosConfig.getScenarios().end())
            {
                // Skip any Scenario time estimates from Scenarios that are no longer part of the Config:
                continue;
            }
            defaultTtmEstimate = getScenarioTotalDuration(*scenarioConfig->second);
            scenarioCutoffTime = scenarioConfig->second->getMatchEstimateFalloffWindow();

            auto outScenarioIter = mmCensusData.getScenarioMatchmakingData().find(scenarioName);
            if (outScenarioIter == mmCensusData.getScenarioMatchmakingData().end())
            {
                auto outScenarioIterElem = mmCensusData.getScenarioMatchmakingData().allocate_element();
                outScenarioIterElem->setScenarioName(scenarioName);  // Not sure why this is a member since the map already tracks it, but w/e. 
                mmCensusData.getScenarioMatchmakingData()[scenarioName] = outScenarioIterElem;
                outScenarioIter = mmCensusData.getScenarioMatchmakingData().find(scenarioName);
            }
            GameManager::ScenarioMatchmakingCensusDataPtr scenarioMMCensusData = outScenarioIter->second;

            TimeValue estScenarioTime = blendTimeValues(scenarioTtmIterator.second.mGlobalTimeToMatchEstimate, curTimeOfDay, scenarioCutoffTime, defaultTtmEstimate);
            scenarioMMCensusData->setGlobalEstimatedTimeToMatch(estScenarioTime);

            // ping site for this scenario
            for (auto& scenarioTtmByPingSiteIter : scenarioTtmIterator.second.mTimesToMatchByPingsiteGroup)
            {
                for (auto& scenarioTtmByGroupIter : scenarioTtmByPingSiteIter.second)
                {
                    // The insert here is fine because we're the only thing touching the EstimatedTimeToMatchPerPingSite (otherwise we'd need to use find() first)
                    TimeValue estPingSiteTime = blendTimeValues(scenarioTtmByGroupIter.second, curTimeOfDay, scenarioCutoffTime, defaultTtmEstimate);
                    auto pingsiteGroupElem = scenarioMMCensusData->getEstimatedTimeToMatchPerPingsiteGroup().allocate_element();
                    (*pingsiteGroupElem)[scenarioTtmByGroupIter.first] = estPingSiteTime;
                    scenarioMMCensusData->getEstimatedTimeToMatchPerPingsiteGroup()[scenarioTtmByPingSiteIter.first] = pingsiteGroupElem;
                }
            }
        }
    }

} // namespace Matchmaker
} // namespace Blaze
