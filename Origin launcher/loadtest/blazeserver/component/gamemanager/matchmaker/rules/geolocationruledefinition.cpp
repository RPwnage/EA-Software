/*! ************************************************************************************************/
/*!
\file   geolocationruledefinition.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rules/geolocationrule.h"
#include "gamemanager/matchmaker/rules/geolocationruledefinition.h"




namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    const float GeoLocationCoordinate::INVALID_LAT_LON = 360.0f;
    const float GeoLocationRange::INVALID_LAT_LON = 360.0f;
    

    GeoLocationRuleDefinition::GeoLocationRuleDefinition() 
        : mFitFormula(nullptr)
    {
    }

    GeoLocationRuleDefinition::~GeoLocationRuleDefinition() 
    {
        delete mFitFormula;
    }

    DEFINE_RULE_DEF_CPP(GeoLocationRuleDefinition, "geoLocationRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* GeoLocationRuleDefinition::GEO_LOCATION_RULE_EXACT_DISTANCE_CONFIG_KEY = "exactDistanceMatch";
    const char8_t* GeoLocationRuleDefinition::GEO_LOCATION_RULE_TOO_FAR_DISTANCE_CONFIG_KEY = "tooFarDistanceMatch";
    const char8_t* GeoLocationRuleDefinition::GEOLOCATION_RULE_LATITUDE_RETE_KEY = "latitude";
    const char8_t* GeoLocationRuleDefinition::GEOLOCATION_RULE_LONGITUDE_RETE_KEY = "longitude";

    const float GeoLocationRuleDefinition::EARTHS_RADIUS = 3963.1f;

    /*! ************************************************************************************************/
    /*! \brief Initializes the rule definition given the configuration map provided.  The map
        will be pointing to the config section that matches the rule name.

        \param[in] configMap - The section of the entire configuration map that pertains to our rule
        \return success/failure
    *************************************************************************************************/
    bool GeoLocationRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const GeoLocationRuleConfig& geoConfig = matchmakingServerConfig.getRules().getGeoLocationRule();

        if(!geoConfig.isSet())
        {
            WARN_LOG("[GeoLocationRuleDefinition] " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        geoConfig.copyInto(mGeoLocationRuleConfigTdf);

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, mGeoLocationRuleConfigTdf.getWeight());

        // Some config checking.
        if (getMinRange() > getMaxRange())
        {
            ERR_LOG("[GeoLocationRuleDefinition].initalize cannot have a configured minimum range > the maximum range.");
            return false;
        }

        if (mGeoLocationRuleConfigTdf.getAngularBucketSizeInDegrees() == 0 || mGeoLocationRuleConfigTdf.getAngularBucketSizeInDegrees() > 90)
        {
            ERR_LOG("[GeoLocationRuleDefinition].initalize cannot have a configured angular bucket size of 0 or greater than 90 degrees.");
            return false;
        }

        initDistanceHelperTables();

        FitFormulaConfig fitFormulaConfig;
        fitFormulaConfig.setName(FIT_FORMULA_LINEAR);

        mFitFormula = FitFormula::createFitFormula(fitFormulaConfig.getName());
        // we use the range configured in the range parameter of the GeoLocation rule because we calculate the fit score based on the distance from the user's location up until maxRange
        // any game or matchmaking session farther than that will have a fit score of 0.
        ((LinearFitFormula*)mFitFormula)->setDefaultMaxVal(getMaxRange());
        ((LinearFitFormula*)mFitFormula)->setDefaultMinVal(getMinRange());

        if(!mFitFormula->initialize(fitFormulaConfig, nullptr))
        {
            ERR_LOG("[GeoLocationRuleDefinition].initalize GeoLocationRule mFitFormula failed.");
            return false;
        }

        cacheWMEAttributeName(GEOLOCATION_RULE_LATITUDE_RETE_KEY);
        cacheWMEAttributeName(GEOLOCATION_RULE_LONGITUDE_RETE_KEY);

        return mRangeListContainer.initialize(getName(), mGeoLocationRuleConfigTdf.getRangeOffsetLists());
    }

    /*! ************************************************************************************************/
    /*! \brief Hook into toString, allows derived classes to write their custom rule defn info into
        an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.
        Helper subroutine to be provided by the specific implementation.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
        Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void GeoLocationRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        char8_t buf[1024];
        mGeoLocationRuleConfigTdf.print(buf, sizeof(buf));
        // actually log 'em
        dest.append_sprintf("%s %s", prefix, buf);
    }

    /*! ************************************************************************************************/
    /*! \brief Calculate a fit score based off two points of latitude and longitude.  The fit score
        will be the percentage the distance is to our allowed "too far" distance.  Closer the too far
        distance, the lower the fit score will be.  Exceeding the too far distance will return NO_MATCH

        \param[in] distance - the distance apart to get the fit percent for
        \return the floating point fit score or NO_MATCH if we've exceeded the too far distance.
    ************************************************************************************************/
    float GeoLocationRuleDefinition::calculateFitPercent(uint32_t distance) const
    {
        // Calculate the fit score as a percentage between our config values.
        if (distance <= getMinRange())
        {
            return 1.0f;
        }
        else if (distance <= getMaxRange() && mFitFormula != nullptr)
        {
            // We want a percentage between our two points of min & max
            return mFitFormula->getFitPercent(0, distance);
        }
        else
        {
            return FIT_PERCENT_NO_MATCH;
        }
    }

    uint32_t GeoLocationRuleDefinition::calculateDistance(float latA, float longA, float latB, float longB) const
    {
        // Convert back to floats (Stored as int32's in usermanager due to replication)
        int32_t alat = (int32_t)(latA);
        int32_t along = (int32_t)(longA);
        int32_t blat = (int32_t)(latB);
        int32_t blong = (int32_t)(longB);

        int32_t latDiff = abs(alat - blat);
        int32_t longDiff = abs(along - blong);

        uint32_t distance = getAccurateDistance(alat, blat, latDiff, longDiff);
        
        return distance;
    }
    
    void GeoLocationRuleDefinition::initDistanceHelperTables()
    {
        // Here's the real calculation: 
        for (uint32_t i = 0; i < DEGREE_TABLE_SIZE; ++i)
        {
            float value = (float)sin((((float)i / 180.0f) * M_PI) /2.0f);
            mSinHalfSquareTable[i] = value*value;
        }

        // Value at [0] == -180 degrees, [180] = 0, etc. 
        for (int32_t i = 0; i < (int32_t)DEGREE_TABLE_SIZE; ++i)
        {
            mCosTable[i] = (float)cos(((float)(i-(DEGREE_TABLE_SIZE/2)) / 180.0f) * M_PI);
        }

        // And finally the answer:
        for (uint32_t i = 0; i < RESULT_TABLE_SIZE; ++i)
        {
            float a = float(i)/float(RESULT_TABLE_SIZE);
            // We use a squared distribution here, because using an even distribution will put too much detail in the middle.
            a *= a;
            mResultTable[i] = uint32_t(2.f*atan2(sqrt(a), sqrt(1-a)) * EARTHS_RADIUS);
        }
    }

    void clamp(int32_t& val, int32_t min, int32_t max)
    {
        if (val > max) val = max;
        if (val < min) val = min;
    }

    uint32_t GeoLocationRuleDefinition::getAccurateDistance(int32_t lat1, int32_t lat2, int32_t latDiff, int32_t longDiff) const
    {
        lat1 += DEGREE_TABLE_SIZE/2;
        lat2 += DEGREE_TABLE_SIZE/2;

        clamp(lat1, 0, DEGREE_TABLE_SIZE-1);
        clamp(lat2, 0, DEGREE_TABLE_SIZE-1);
        clamp(latDiff, 0, DEGREE_TABLE_SIZE-1);
        clamp(longDiff, 0, DEGREE_TABLE_SIZE-1);
        float a = mSinHalfSquareTable[latDiff] + mCosTable[lat1] * mCosTable[lat2] * mSinHalfSquareTable[longDiff]; 
        
        // We do a sqrt here to better preserve small distances (mResultTable uses squared values). 
        int32_t indexValue = int32_t(sqrt(a) * RESULT_TABLE_SIZE);
        
        clamp(indexValue, 0, RESULT_TABLE_SIZE-1);
        return mResultTable[indexValue];
    }

    void GeoLocationRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isGeoLocationDirty())
        {
            matchmakingCache.cacheTopologyHostGeoLocation(gameSession);
        }
    }

        /*! ************************************************************************************************/
    /*! \brief Calculates the maximum and minimum acceptable latitude and longitude for a potential match
        given a maximum distance and the current location.
    *************************************************************************************************/
    void GeoLocationRuleDefinition::getMaxLatAndLong( const GeoLocationCoordinate &location, uint32_t maxDistance, GeoLocationRange &matchRange ) const
    {
        
        float distance;
        if (maxDistance < getMinRange())
        {
            distance = (float)getMinRange();
        }
        else
        {
            distance = (float)maxDistance;
        }

        float radialDistance = distance/EARTHS_RADIUS;

        // limit the distance to one full earth circumference:
        if (radialDistance > 2.0f*(float)M_PI)
            radialDistance = 2.0f*(float)M_PI;

        // Convert back to floats (Stored as int32's in usermanager due to replication)
        float alat = location.latitude;
        float along = location.longitude;

        // convert to radians
        alat =  alat * ((float)M_PI / 180.0f);
        along = along * ((float)M_PI / 180.0f);

        float latMax = alat + radialDistance;
        float latMin = alat - radialDistance;
        float lonMax = along + radialDistance;
        float lonMin = along - radialDistance;

        // trimming latitude only, longitude is not trimmed to enable wrapping around 180 degrees
        if (latMax > M_PI/2.0f)
            latMax = (float)M_PI/2.0f;

        if (latMin < -M_PI/2.0f)
            latMin = -(float)M_PI/2.0f;

        // convert latitudes & longitudes back to degrees
        latMax = latMax * (180.0f /(float)M_PI);
        latMin = latMin * (180.0f /(float)M_PI);
        lonMax = lonMax * (180.0f /(float)M_PI);
        lonMin = lonMin * (180.0f /(float)M_PI);

        matchRange.northMax = (float)ceil(latMax);
        matchRange.southMax = (float)floor(latMin);
        matchRange.westMax = (float)ceil(lonMax);
        matchRange.eastMax = (float)floor(lonMin);
        matchRange.maxDistance = (float)maxDistance;
    }

    int32_t GeoLocationRuleDefinition::calculateBucketedCoordinate(float coordinate) const
    {
        // round to bucket by integer division and multiplication
        int32_t result = (int32_t)(coordinate / mGeoLocationRuleConfigTdf.getAngularBucketSizeInDegrees());
        return result * (int32_t)mGeoLocationRuleConfigTdf.getAngularBucketSizeInDegrees();
    }

    void GeoLocationRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GeoLocationCoordinate &gameLocation = gameSessionSlave.getMatchmakingGameInfoCache()->getTopologyHostLocation();
        float gameLatitude = gameLocation.latitude; // we don't round, we just put a game in the bucket it fits in
        float gameLongitude = gameLocation.longitude; // we don't round, we just put a game in the bucket it fits in

        // Note inserting as an int instead of forcing to uint.  Conditon::test forces back to int.
        // We should consider providing native support for ints.
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), GEOLOCATION_RULE_LATITUDE_RETE_KEY, 
            calculateBucketedCoordinate(gameLatitude), *this);

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
            calculateBucketedCoordinate(gameLongitude), *this);
    }

    void GeoLocationRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GeoLocationCoordinate &gameLocation = gameSessionSlave.getMatchmakingGameInfoCache()->getTopologyHostLocation();
        float gameLatitude = gameLocation.latitude; // we don't round, we just put a game in the bucket it fits in
        float gameLongitude = gameLocation.longitude; // we don't round, we just put a game in the bucket it fits in

        // Note inserting as an int instead of forcing to uint.  Conditon::test forces back to int.
        // We should consider providing native support for ints.
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), GEOLOCATION_RULE_LATITUDE_RETE_KEY, 
            calculateBucketedCoordinate(gameLatitude), *this);

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), GEOLOCATION_RULE_LONGITUDE_RETE_KEY, 
            calculateBucketedCoordinate(gameLongitude), *this);
    }

    const RangeList* GeoLocationRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
