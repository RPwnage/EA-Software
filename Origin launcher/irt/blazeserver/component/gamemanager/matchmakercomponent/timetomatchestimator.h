/*! ************************************************************************************************/
/*!
    \file timetomatchestimator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_MATCHMAKER_TIME_TO_MATCH_ESTIMATOR_H
#define BLAZE_GAMEMANAGER_MATCHMAKER_TIME_TO_MATCH_ESTIMATOR_H


#include "EASTL/map.h"
#include "framework/tdf/qosdatatypes.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/tdf/scenarios_config_server.h"

namespace Blaze
{

namespace Matchmaker
{
    class TimeToMatchEstimator
    {
    public:
        struct TimeToMatchEstimate
        {
            
            TimeToMatchEstimate()
                : mLastDatapointTimestamp(0),
                mLastTimeToMatch(0),
                mEstimatedTimeToMatch(0)
            {}

            TimeToMatchEstimate& operator=(const TimeToMatchEstimate& ttmEstimate)
            {
                if (this != &ttmEstimate)
                {
                    mLastDatapointTimestamp = ttmEstimate.mLastDatapointTimestamp;
                    mLastTimeToMatch = ttmEstimate.mLastTimeToMatch;
                    mEstimatedTimeToMatch = ttmEstimate.mEstimatedTimeToMatch;
                }

                return *this;
            }

            TimeValue mLastDatapointTimestamp;
            TimeValue mLastTimeToMatch;
            TimeValue mEstimatedTimeToMatch;
        };

        typedef eastl::map<GameManager::DelineationGroupType, TimeToMatchEstimate> TimeToMatchEstimatesByGroup;
        typedef eastl::map<PingSiteAlias, TimeToMatchEstimatesByGroup> TimeToMatchEstimatesByPingsiteGroup;

        struct ScenarioTimeToMatchEstimate
        {
            ScenarioTimeToMatchEstimate()
                : mScenarioName("")
            {}

            GameManager::ScenarioName mScenarioName;
            TimeToMatchEstimate mBaseTimeToMatchEstimate;
            TimeToMatchEstimate mGlobalTimeToMatchEstimate;
            TimeToMatchEstimatesByPingsiteGroup mTimesToMatchByPingsiteGroup;
        };

        typedef eastl::map<GameManager::ScenarioName,ScenarioTimeToMatchEstimate> TimeToMatchEstimatesByScenario;
        
        
        void initializeTtmEstimates(const GameManager::ScenariosConfig &scenariosConfig);
        void initializeTtmEstimates(const GameManager::MatchmakingInstanceStatusResponse& matchmakingInstanceStatusResponse);
        
        void updateScenarioTimeToMatch(const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup, const GameManager::ScenarioName &scenarioName,
            const GameManager::ScenariosConfig &scenariosConfig, GameManager::MatchmakingResult result);
        void updateNonScenarioTimeToMatch(const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup, const GameManager::ScenariosConfig &scenariosConfig, GameManager::MatchmakingResult result);

        TimeValue getTimeToMatchEstimate(const GameManager::ScenarioName& scenarioName, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType& delineationGroup, const GameManager::ScenariosConfig &scenariosConfig) const;

        void populateMatchmakingCensusData(GameManager::InstanceTimeToMatchEstimateData &globalTtmData, GameManager::TimeToMatchEstimateDataPerScenario &scenarioTtmData) const;
        void collateMatchmakingCensusData(const GameManager::InstanceTimeToMatchEstimateData &globalTtmData, 
            const GameManager::TimeToMatchEstimateDataPerScenario &scenarioTtmData,
            const GameManager::ScenariosConfig &scenariosConfig);
        void getMatchmakingCensusData(GameManager::MatchmakingCensusData &mmCensusData, const GameManager::ScenariosConfig &scenariosConfig) const;
    
    private:
        void intializeScenarioTtm(const GameManager::ScenarioName& scenarioName, const TimeValue& scenarioTimeoutDuration, const TimeValue& initialDatapointTimestamp);
        void updateTimeToMatchEstimate(TimeToMatchEstimate &ttmEstimate, const TimeValue &scenarioDuration, const TimeValue &weightFactor,
            const TimeValue &referenceTime = TimeValue::getTimeOfDay(), GameManager::MatchmakingResult result = GameManager::SESSION_TIMED_OUT) const;
        double calculateTtmEstimateMillis(const TimeValue &latestTtm, const TimeValue& lastTtm, const TimeValue& currentTtmEstimate, 
            const TimeValue &timeSinceLastSample, const TimeValue &weightFactor) const;

        


        TimeToMatchEstimatesByScenario mScenarioTTMEstimates;
        TimeToMatchEstimatesByPingsiteGroup mGlobalTTMEstimates;
        TimeToMatchEstimate mGlobalTtmEstimate;
    };
    

} // namespace Matchmaker
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_MATCHMAKER_TIME_TO_MATCH_ESTIMATOR_H
