/*************************************************************************************************/
/*!
    \file   timeutils.cpp

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

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "timeutils.h"
#include "EATDF/time.h"

// global includes
#include "time.h"

#include "stats/rpc/statsslave.h"

namespace Blaze
{
    namespace UserActivityTracker
    {
        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

        */
        /*************************************************************************************************/
        TimeUtils::TimeUtils()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

        */
        /*************************************************************************************************/
        TimeUtils::~TimeUtils()
        {
        }

        int64_t TimeUtils::getCurrentTime()
        {
            return TimeValue::getTimeOfDay().getSec();
        }

        int64_t TimeUtils::getRollOverTime(Blaze::Stats::StatPeriodType periodType)
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
                    iRollOverTime = getDailyRollOverTime(pPeriodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getWeeklyRollOverTime(pPeriodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getMonthlyRollOverTime(pPeriodIds, iSecsSinceEpoch);
                }
                break;
            default:
                {
                    WARN_LOG("[TimeUtils].getRolloverTime. invalid period type "<<periodType<<" ");
                }
                
                break;
            }

            return iRollOverTime;
        }

        int64_t TimeUtils::getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType, int32_t multiple)
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
                    iRollOverTime = getPreviousDailyRollOverTime(pPeriodIds, iSecsSinceEpoch, multiple);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getPreviousWeeklyRollOverTime(pPeriodIds, iSecsSinceEpoch, multiple);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getPreviousMonthlyRollOverTime(pPeriodIds, iSecsSinceEpoch, multiple);
                }
                break;
            default:
                {
                    WARN_LOG("[TimeUtils].getPreviousRolloverTime. invalid period type "<<periodType<<" ");
                }

                break;
            }

            return iRollOverTime;

        }


        int64_t TimeUtils::getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes)
        {
            int64_t iSeconds = (iDays * SECS_PER_DAY) + (iHours * SECS_PER_HOUR) + (iMinutes * SECS_PER_MINUTE);
            return iSeconds;
        }
         
        void TimeUtils::logTime(const char8_t *strDescription, int64_t t)
        {
            struct tm tM;
			TimeValue::gmTime(&tM, t); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].logTime(): "<< strDescription <<": "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");
        }

        void TimeUtils::getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen)
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


        bool TimeUtils::getStatsPeriodIds(Stats::PeriodIds& ids)
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
					ERR_LOG("[TimeUtils:" << this << "].getStatsPeriodIds. Unable to retrieve the stats period ids");
				}
				else
				{
					bSuccess = true;
				}
			}
			else
			{
				ERR_LOG("[TimeUtils:" << this << "].getStatsPeriodIds. Unable to retrieve the stats slave component");

			}

			return bSuccess;
        }


        int64_t TimeUtils::getDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

			TimeValue::gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate today's rollover time
            tM.tm_hour = periodIds.getDailyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = TimeValue::mkgmTime(&tM);

            if (iRollOverTime <= iEpochTimeSecs)
            {
                // today's rollover has passed. add a day's worth of time
                iRollOverTime += SECS_PER_DAY;
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].getDailyRollOverTime(): next daily rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t TimeUtils::getWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs)
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

//			int weekNumber = tM.tm_yday/7;

			if (iRollOverTime <= iEpochTimeSecs)
            {
                // this week's rollover has passed. add a week's worth of time
                iRollOverTime += 7 * SECS_PER_DAY;
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].getWeeklyRollOverTime(): next weekly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t TimeUtils::getMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs)
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

			if (iRollOverTime <= iEpochTimeSecs)
            {
                // this month's rollover has passed. add a month's worth of time
                ++tM.tm_mon;
                iRollOverTime = TimeValue::mkgmTime(&tM);
            }

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].getMonthlyRollOverTime(): next monthly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t TimeUtils::getPreviousDailyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple)
        {
            struct tm tM;   
            int64_t iRollOverTime = getDailyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous daily rollover is simply the next rollover - 1 day 
            iRollOverTime -= (SECS_PER_DAY * multiple);

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].getPreviousDailyRollOverTime(): previous daily rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t TimeUtils::getPreviousWeeklyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple)
        {
            struct tm tM;   
            int64_t iRollOverTime = getWeeklyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous weekly rollover is simply the next rollover - 7 days
            iRollOverTime -= (SECS_PER_WEEK * multiple);

            TimeValue::gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[TimeUtils].getPreviousWeeklyRollOverTime(): previous weekly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

        int64_t TimeUtils::getPreviousMonthlyRollOverTime(Stats::PeriodIds& periodIds, int64_t iEpochTimeSecs, int32_t multiple)
        {
            struct tm tM;   
            int64_t iRollOverTime = getMonthlyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous monthly rollover time is the next rollover - 1 month. Need to us the tm struct to calculate this
            TimeValue::gmTime(&tM, iRollOverTime); // convert iRollover to a tm struct
			for (int i = 0; i < multiple; i++)
			{
				--tM.tm_mon;
			}

			iRollOverTime = TimeValue::mkgmTime(&tM);

            TRACE_LOG("[TimeUtils].getPreviousMonthlyRollOverTime(): previous monthly rollover: "<<tM.tm_year + 1900 <<"/"<<tM.tm_mon + 1 <<"/"<<tM.tm_mday <<" ("<<tM.tm_wday <<") "<<tM.tm_hour <<":"<<tM.tm_min <<":"<<tM.tm_sec <<" GMT");

            return iRollOverTime;
        }

    } // UserActivityTracker
} // Blaze
