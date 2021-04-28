/*************************************************************************************************/
/*!
    \file   fifacupstimeutils.cpp

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
        Intended to only be used on the slaves

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "fifacups/fifacupstimeutils.h"
#include "EATDF/time.h"

// global includes
#include "time.h"

#include "stats/rpc/statsslave.h"

namespace Blaze
{
    namespace FifaCups
    {
#define SECS_PER_WEEK   604800
#define SECS_PER_DAY    86400
#define SECS_PER_HOUR   3600
#define SECS_PER_MINUTE 60

        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

        */
        /*************************************************************************************************/
        FifaCupsTimeUtils::FifaCupsTimeUtils()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

        */
        /*************************************************************************************************/
        FifaCupsTimeUtils::~FifaCupsTimeUtils()
        {
        }

        int64_t FifaCupsTimeUtils::getCurrentTime()
        {
            return TimeValue::getTimeOfDay().getSec();
        }

        int64_t FifaCupsTimeUtils::getRollOverTime(Blaze::Stats::StatPeriodType periodType, StatMode statMode)
        {
            int64_t iRollOverTime = 0;
            int64_t iSecsSinceEpoch = getCurrentTime();

            Stats::PeriodIds pPeriodIds;
            if (!getStatsPeriodIds(pPeriodIds))
            {
                // message already logged by getStatsPeriodIds()
                return iRollOverTime;
            }

            switch(periodType)
            {
            case Stats::STAT_PERIOD_DAILY:
                {
                    iRollOverTime = getDailyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getWeeklyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getMonthlyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            default:
                {
                    WARN_LOG("[FifaCupsTimeUtils].getRolloverTime. invalid period type "<<periodType<<" ");
                }
                
                break;
            }

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType, StatMode statMode)
        {
            int64_t iRollOverTime = 0;
            int64_t iSecsSinceEpoch = getCurrentTime();

            Stats::PeriodIds pPeriodIds;
            if (!getStatsPeriodIds(pPeriodIds))
            {
                // message already logged by getStatsPeriodIds()
                return iRollOverTime;
            }

            switch(periodType)
            {
            case Stats::STAT_PERIOD_DAILY:
                {
                    iRollOverTime = getPreviousDailyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getPreviousWeeklyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getPreviousMonthlyRollOverTime(pPeriodIds, iSecsSinceEpoch, statMode);
                }
                break;
            default:
                {
                    WARN_LOG("[FifaCupsTimeUtils].getPreviousRolloverTime. invalid period type "<<periodType<<" ");
                }

                break;
            }

            return iRollOverTime;

        }


        int64_t FifaCupsTimeUtils::getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes)
        {
            int64_t iSeconds = (iDays * 24 * 3600) + (iHours * 3600) + (iMinutes * 60);
            return iSeconds;
        }
          
        void FifaCupsTimeUtils::logTime(const char8_t *strDescription, int64_t t)
        {
            struct tm tM;
			TimeValue::gmTime(&tM, t); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].logTime(): "<< strDescription <<": "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");
        }

        void FifaCupsTimeUtils::getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen)
        {
            int64_t iTimeLeft = t;

            int32_t iDays = static_cast<int32_t>(iTimeLeft / SECS_PER_DAY);
            iTimeLeft -= (iDays * SECS_PER_DAY);

            int32_t iHours = static_cast<int32_t>(iTimeLeft / SECS_PER_HOUR);
            iTimeLeft -= (iHours * SECS_PER_HOUR);

            int32_t iMinutes = static_cast<int32_t>(iTimeLeft / SECS_PER_MINUTE);
            iTimeLeft -= (iMinutes * SECS_PER_MINUTE);

            blaze_snzprintf(strBuffer, bufferLen, "%02dd %02dh %02dm %02ds", iDays, iHours, iMinutes, static_cast<int32_t>(iTimeLeft));
        }


        bool FifaCupsTimeUtils::getStatsPeriodIds(Stats::PeriodIds& ids)
        {
			bool bSuccess = false;

			// get the stats slave
			Stats::StatsSlave* statsComponent = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID,false));
			if (NULL != statsComponent)
			{
				// get the stats period ids which has the rollover days/hours
				UserSession::pushSuperUserPrivilege();
				BlazeRpcError error = statsComponent->getPeriodIds(ids);
				UserSession::popSuperUserPrivilege();

				if (Blaze::ERR_OK != error)
				{
					ERR_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getStatsPeriodIds. Unable to retrieve the stats period ids");
				}
				else
				{
					bSuccess = true;
				}
			}
			else
			{
				ERR_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getStatsPeriodIds. Unable to retrieve the stats slave component");

			}

			return bSuccess;
        }


        int64_t FifaCupsTimeUtils::getDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

			TimeValue::gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate today's rollover time
            tM.tm_hour = periodIds.getDailyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = TimeValue::mkgmTime(&tM);

			//figure out the day number since epoch.
			int64_t dayNumSinceEpoch = iRollOverTime/SECS_PER_DAY;

			switch(statMode)
			{
			case EVERY_OTHER_ODD:
				if (dayNumSinceEpoch % 2 == 1)
				{
					iRollOverTime += SECS_PER_DAY;
				}
				break;
			case EVERY_OTHER_EVEN:
				if (dayNumSinceEpoch % 2 == 0)
				{
					iRollOverTime += SECS_PER_DAY;
				}
				break;
			case EVERY_OTHER_NONE:
			default:
				break;
			}

            if (iRollOverTime <= iEpochTimeSecs)
            {
                // today's rollover has passed. add a day's worth of time
                iRollOverTime += 24 * 3600;
			
				if (statMode == EVERY_OTHER_ODD || statMode == EVERY_OTHER_EVEN)
				{
					iRollOverTime += SECS_PER_DAY;
				}
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].getDailyRollOverTime(): next daily rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

            TimeValue::gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate this week's rollover time
            tM.tm_mday += periodIds.getWeeklyDay() - tM.tm_wday;
            tM.tm_hour = periodIds.getWeeklyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = TimeValue::mkgmTime(&tM);

			int weekNumber = tM.tm_yday/7;
			switch(statMode)
			{
			case EVERY_OTHER_ODD:
				if (weekNumber % 2 == 1)
				{
					iRollOverTime += SECS_PER_WEEK;
				}
				break;
			case EVERY_OTHER_EVEN:
				if (weekNumber % 2 == 0)
				{
					iRollOverTime += SECS_PER_WEEK;
				}
				break;
			case EVERY_OTHER_NONE:
			default:
				break;
			}

			if (iRollOverTime <= iEpochTimeSecs)
            {
                // this week's rollover has passed. add a week's worth of time
                iRollOverTime += 7 * 24 * 3600;

				if (statMode == EVERY_OTHER_ODD || statMode == EVERY_OTHER_EVEN)
				{
					iRollOverTime += SECS_PER_WEEK;
				}
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].getWeeklyRollOverTime(): next weekly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

            TimeValue::gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate this week's rollover time
            tM.tm_mday = periodIds.getMonthlyDay();
            tM.tm_hour = periodIds.getMonthlyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = TimeValue::mkgmTime(&tM);

			switch(statMode)
			{
			case EVERY_OTHER_ODD:
				if (tM.tm_mon % 2 == 1)
				{
					++tM.tm_mon;
					iRollOverTime = TimeValue::mkgmTime(&tM);
				}
				break;
			case EVERY_OTHER_EVEN:
				if (tM.tm_mon % 2 == 0)
				{
					++tM.tm_mon;
					iRollOverTime = TimeValue::mkgmTime(&tM);
				}
				break;
			case EVERY_OTHER_NONE:
			default:
				break;
			}

			if (iRollOverTime <= iEpochTimeSecs)
            {
                // this month's rollover has passed. add a month's worth of time
                ++tM.tm_mon;
                iRollOverTime = TimeValue::mkgmTime(&tM);

				if (statMode == EVERY_OTHER_ODD || statMode == EVERY_OTHER_EVEN)
				{
					++tM.tm_mon;
					iRollOverTime = TimeValue::mkgmTime(&tM);
				}
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].getMonthlyRollOverTime(): next monthly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getPreviousDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = getDailyRollOverTime(periodIds, iEpochTimeSecs, statMode);

            // the previous daily rollover is simply the next rollover - 1 day 
            iRollOverTime -= SECS_PER_DAY;
			if (statMode == EVERY_OTHER_EVEN || statMode == EVERY_OTHER_ODD)
			{
				iRollOverTime -= SECS_PER_DAY;
			}

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].getPreviousDailyRollOverTime(): previous daily rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getPreviousWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = getWeeklyRollOverTime(periodIds, iEpochTimeSecs, statMode);

            // the previous weekly rollover is simply the next rollover - 7 days
            iRollOverTime -= SECS_PER_WEEK;
			if (statMode == EVERY_OTHER_EVEN || statMode == EVERY_OTHER_ODD)
			{
				iRollOverTime -= SECS_PER_WEEK;
			}

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[FifaCupsTimeUtils].getPreviousWeeklyRollOverTime(): previous weekly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t FifaCupsTimeUtils::getPreviousMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, StatMode statMode)
        {
            struct tm tM;   
            int64_t iRollOverTime = getMonthlyRollOverTime(periodIds, iEpochTimeSecs, statMode);

            // the previous monthly rollover time is the next rollover - 1 month. Need to us the tm struct to calculate this
            TimeValue::gmTime(&tM, iRollOverTime); // convert iRollover to a tm struct
            --tM.tm_mon;
			if (statMode == EVERY_OTHER_EVEN || statMode == EVERY_OTHER_ODD)
			{
				--tM.tm_mon;
			}
            iRollOverTime = TimeValue::mkgmTime(&tM);

            TRACE_LOG("[FifaCupsTimeUtils].getPreviousMonthlyRollOverTime(): previous monthly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

    } // FifaCups
} // Blaze
