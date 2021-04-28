/*! ************************************************************************************************/
/*!
\file   geolocationruledefinition.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GEO_LOCATION_RULE_DEFINITION_H
#define BLAZE_MATCHMAKING_GEO_LOCATION_RULE_DEFINITION_H

#define _USE_MATH_DEFINES // for match.h use of M_PI
#include "math.h"
#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/rules/geolocationtypes.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    class FitFormula;

    class GeoLocationRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GeoLocationRuleDefinition);
        DEFINE_RULE_DEF_H(GeoLocationRuleDefinition, GeoLocationRule);

    public:

        GeoLocationRuleDefinition();
        ~GeoLocationRuleDefinition() override;

        static const char8_t* GEO_LOCATION_RULE_EXACT_DISTANCE_CONFIG_KEY;
        static const char8_t* GEO_LOCATION_RULE_TOO_FAR_DISTANCE_CONFIG_KEY;
        static const char8_t* GEOLOCATION_RULE_LATITUDE_RETE_KEY;
        static const char8_t* GEOLOCATION_RULE_LONGITUDE_RETE_KEY;

        static const float EARTHS_RADIUS;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        const RangeList* getRangeOffsetList(const char8_t *listName) const;

        float calculateFitPercent(uint32_t distance) const;
        uint32_t calculateDistance(float latA, float longA, float latB, float longB) const;
        int64_t getMinRange() const { return mGeoLocationRuleConfigTdf.getRange().getMin(); }
        int64_t getMaxRange() const { return mGeoLocationRuleConfigTdf.getRange().getMax(); }
        int32_t calculateBucketedCoordinate(float coordinate) const;
        uint32_t getAngularBucketSize() const { return mGeoLocationRuleConfigTdf.getAngularBucketSizeInDegrees(); }


        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override { return true; }

        bool isDisabled() const override { return mRangeListContainer.empty(); }
        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;

        void getMaxLatAndLong(const GeoLocationCoordinate &location, uint32_t maxDistance, GeoLocationRange &matchRange) const;
        
    private:
        void initDistanceHelperTables();
        uint32_t getAccurateDistance(int32_t lat1, int32_t lat2, int32_t latDiff, int32_t longDiff) const;

    private:

        GeoLocationRuleConfig mGeoLocationRuleConfigTdf;
        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;

        // Helper Tables: 
        static const uint32_t DEGREE_TABLE_SIZE = 360;
        static const uint32_t RESULT_TABLE_SIZE = 1000;

        float mSinHalfSquareTable[DEGREE_TABLE_SIZE];
        float mCosTable[DEGREE_TABLE_SIZE];
        uint32_t mResultTable[RESULT_TABLE_SIZE];
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif
