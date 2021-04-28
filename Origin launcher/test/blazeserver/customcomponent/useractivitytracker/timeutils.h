/*************************************************************************************************/
/*!
    \file  timeutils.h

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2020
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class TimeUtils

    \note

*/
/*************************************************************************************************/

#ifndef BLAZE_USERACTIVITYTRACKER_TIMEUTILS_H
#define BLAZE_USERACTIVITYTRACKER_TIMEUTILS_H

/*** Include Files *******************************************************************************/
// main include
#include "stats/tdf/stats.h"
#include "stats/tdf/stats_server.h"


namespace Blaze
{
    namespace UserActivityTracker
    {
#define SECS_PER_WEEK   604800
#define SECS_PER_DAY    86400
#define SECS_PER_HOUR   3600
#define SECS_PER_MINUTE 60


        class TimeUtils
        {
        public:
            TimeUtils();
            virtual ~TimeUtils();

            // get current time
            int64_t getCurrentTime();

            // get the roll over time for the stat period type
            int64_t getRollOverTime(Blaze::Stats::StatPeriodType periodType);

            // get the previous roll over time for the stat period type
            int64_t getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType, int32_t multiple = 1);

            // converts the specified days, hours and minutes into seconds (static)
            static int64_t getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes);

            // helper to log a time (secs) in a displayable form (uses DEBUG3)
            static void logTime(const char8_t *strDescription, int64_t t);

            // helper to convert a duration (secs) into a displayable string. Buffer must be min 16 characters
            static void getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen);
            
			// helper to get the stats component period id's
			bool getStatsPeriodIds(Stats::PeriodIds& ids);

        private:


            // the rollover time (in seconds) 
            int64_t getDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs);
            int64_t getWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs);
            int64_t getMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs);

            // get the previous rollover time (in seconds)
            int64_t getPreviousDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple);
            int64_t getPreviousWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple);
            int64_t getPreviousMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple);
        };

    } // UserActivtiyTracker
} // Blaze

#endif // BLAZE_USERACTIVITYTRACKER_TIMEUTILS_H
