/*************************************************************************************************/
/*!
    \file   osdkseasonalplaytimeutils.h

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKSeasonalPlayTimeUtils

    Central implementation of Time Utilities for OSDK Seasonal Play.

    \note

*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKSEASONALPLAY_TIMEUTILS_H
#define BLAZE_OSDKSEASONALPLAY_TIMEUTILS_H

/*** Include Files *******************************************************************************/
// main include
#include "stats/tdf/stats.h"
#include "stats/tdf/stats_server.h"


namespace Blaze
{
    namespace OSDKSeasonalPlay
    {
        class OSDKSeasonalPlayTimeUtils
        {
        public:
            OSDKSeasonalPlayTimeUtils();
            virtual ~OSDKSeasonalPlayTimeUtils();

            // get current time
            int64_t getCurrentTime();

            // get the roll over time for the stat period type
            int64_t getRollOverTime(Blaze::Stats::StatPeriodType periodType);

            // get the previous roll over time for the stat period type
            int64_t getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType);

            // converts the specified days, hours and minutes into seconds (static)
            static int64_t getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes);

            // converts seconds to a struct tm (static)
            static void gmTime(struct tm *tM, int64_t t);

            // converts a struct tm to seconds (static)
            static int64_t mkgmTime(struct tm *tM);

            // helper to log a time (secs) in a displayable form (uses DEBUG3)
            static void logTime(const char8_t *strDescription, int64_t t);

            // helper to convert a duration (secs) into a displayable string. Buffer must be min 16 characters
            static void getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen);
            

        private:

            // helper to get the stats component period id's
            bool getStatsPeriodIds(Stats::PeriodIds& ids);

            // the rollover time (in seconds) 
            int64_t getDailyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);
            int64_t getWeeklyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);
            int64_t getMonthlyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);

            // get the previous rollover time (in seconds)
            int64_t getPreviousDailyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);
            int64_t getPreviousWeeklyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);
            int64_t getPreviousMonthlyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs);
        };

    } // OSDKSeasonalPlay
} // Blaze

#endif // BLAZE_OSDKSEASONALPLAY_TIMEUTILS_H
