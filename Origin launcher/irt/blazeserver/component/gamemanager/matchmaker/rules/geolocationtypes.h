/*! ************************************************************************************************/
/*!
\file   geolocationtypes.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GEO_LOCATION_TYPES_H
#define BLAZE_MATCHMAKING_GEO_LOCATION_TYPES_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    struct GeoLocationCoordinate
    {
        GeoLocationCoordinate()
            : latitude(INVALID_LAT_LON), longitude(INVALID_LAT_LON)
        {}
        GeoLocationCoordinate(float lat, float lon)
            : latitude(lat), longitude(lon)
        {}

        bool isValid() const { return ((fabs(latitude) < INVALID_LAT_LON) && (fabs(longitude) < INVALID_LAT_LON)); }

        static const float INVALID_LAT_LON;

        float latitude;
        float longitude;
    };

    struct GeoLocationRange
    {
        GeoLocationRange()
            : northMax(INVALID_LAT_LON), 
            southMax(INVALID_LAT_LON),
            westMax(INVALID_LAT_LON), 
            eastMax(INVALID_LAT_LON),
            maxDistance(FIT_PERCENT_NO_MATCH)
        {}

        GeoLocationRange(float latMax, float latMin, float longMax, float longMin, float distance)
            : northMax(latMax), 
            southMax(latMin),
            westMax(longMax), 
            eastMax(longMin),
            maxDistance(distance)
        {}

        bool isValid() const 
        { 
            return ((fabs(northMax) < INVALID_LAT_LON) 
                    && (fabs(southMax) < INVALID_LAT_LON) 
                    && (fabs(westMax) < INVALID_LAT_LON)
                    && (fabs(eastMax) < INVALID_LAT_LON)); 
        }

        static const float INVALID_LAT_LON;

        float northMax;
        float southMax;
        float westMax;
        float eastMax;
        float maxDistance;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif
