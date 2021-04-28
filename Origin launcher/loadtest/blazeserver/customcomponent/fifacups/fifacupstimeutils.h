/*************************************************************************************************/
/*!
    \file   fifacupstimeutils.h

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaCupsTimeUtils

    Central implementation of Time Utilities for OSDK Seasonal Play.

    \note

*/
/*************************************************************************************************/

#ifndef BLAZE_FIFACUPS_TIMEUTILS_H
#define BLAZE_FIFACUPS_TIMEUTILS_H

/*** Include Files *******************************************************************************/
// main include
#include "stats/tdf/stats.h"
#include "stats/tdf/stats_server.h"

#include "fifacups/tdf/fifacupstypes.h"

namespace Blaze
{
    namespace FifaCups
    {
        class FifaCupsTimeUtils
        {
        public:
            FifaCupsTimeUtils();
            virtual ~FifaCupsTimeUtils();

            // get current time
            int64_t getCurrentTime();

            // get the roll over time for the stat period type
            int64_t getRollOverTime(Blaze::Stats::StatPeriodType periodType, StatMode statMode = EVERY_OTHER_NONE);

            // get the previous roll over time for the stat period type
            int64_t getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType, StatMode statMode = EVERY_OTHER_NONE);

            // converts the specified days, hours and minutes into seconds (static)
            static int64_t getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes);

            // helper to log a time (secs) in a displayable form (uses DEBUG3)
            static void logTime(const char8_t *strDescription, int64_t t);

            // helper to convert a duration (secs) into a displayable string. Buffer must be min 16 characters
            static void getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen);
            

        private:

            // helper to get the stats component period id's
            bool getStatsPeriodIds(Stats::PeriodIds& ids);

            // the rollover time (in seconds) 
            int64_t getDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);
            int64_t getWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);
            int64_t getMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);

            // get the previous rollover time (in seconds)
            int64_t getPreviousDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);
            int64_t getPreviousWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);
            int64_t getPreviousMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode);
        };

    } // FifaCups
} // Blaze

#endif // BLAZE_FIFACUPS_TIMEUTILS_H
