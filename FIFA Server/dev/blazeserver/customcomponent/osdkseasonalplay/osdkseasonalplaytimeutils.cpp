/*************************************************************************************************/
/*!
    \file   osdkseasonalplaytimeutils.cpp

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
        Intended to only be used on the slaves

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "component/stats/rpc/statsslave.h"
#include "osdkseasonalplay/osdkseasonalplaytimeutils.h"

// global includes
#include "time.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
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
        OSDKSeasonalPlayTimeUtils::OSDKSeasonalPlayTimeUtils()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

        */
        /*************************************************************************************************/
        OSDKSeasonalPlayTimeUtils::~OSDKSeasonalPlayTimeUtils()
        {
        }

        int64_t OSDKSeasonalPlayTimeUtils::getCurrentTime()
        {
            return TimeValue::getTimeOfDay().getSec();
        }

        int64_t OSDKSeasonalPlayTimeUtils::getRollOverTime(Blaze::Stats::StatPeriodType periodType)
        {
            int64_t iRollOverTime = 0;
            int64_t iSecsSinceEpoch = getCurrentTime();

            Stats::PeriodIds periodIds;
            bool bSuccess = getStatsPeriodIds(periodIds);
            if (!bSuccess)
            {
                // message already logged by getStatsPeriodIds()
                return iRollOverTime;
            }

            switch(periodType)
            {
            case Stats::STAT_PERIOD_DAILY:
                {
                    iRollOverTime = getDailyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getWeeklyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getMonthlyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            default:
                {
                    WARN_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getRolloverTime. invalid period type " << periodType);
                }
                
                break;
            }

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getPreviousRollOverTime(Blaze::Stats::StatPeriodType periodType)
        {
            int64_t iRollOverTime = 0;
            int64_t iSecsSinceEpoch = getCurrentTime();

            Stats::PeriodIds periodIds;
            bool bSuccess = getStatsPeriodIds(periodIds);
            if (!bSuccess)
            {
                // message already logged by getStatsPeriodIds()
                return iRollOverTime;
            }

            switch(periodType)
            {
            case Stats::STAT_PERIOD_DAILY:
                {
                    iRollOverTime = getPreviousDailyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_WEEKLY:
                {
                    iRollOverTime = getPreviousWeeklyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            case Stats::STAT_PERIOD_MONTHLY:
                {
                    iRollOverTime = getPreviousMonthlyRollOverTime(&periodIds, iSecsSinceEpoch);
                }
                break;
            default:
                {
                    WARN_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getPreviousRolloverTime. invalid period type " << periodType);
                }

                break;
            }

            return iRollOverTime;

        }


        int64_t OSDKSeasonalPlayTimeUtils::getDurationInSeconds(int32_t iDays, int32_t iHours, int32_t iMinutes)
        {
            int64_t iSeconds = (iDays * 24 * 3600) + (iHours * 3600) + (iMinutes * 60);
            return iSeconds;
        }
    
        /*************************************************************************************************/
        /*!
            \brief gmTime  static 
                Converts epoch time into GMT components
            \param[in]  - t - 64 bit epoch time (seconds)
            \param[out] - tM - pointer to tm structure
            \return - none
        */
        /*************************************************************************************************/
        void OSDKSeasonalPlayTimeUtils::gmTime(struct tm *tM, int64_t t)
        {
            #ifdef EA_PLATFORM_WINDOWS
                _gmtime64_s(tM, &t);
            #else
                gmtime_r(&t, tM);
            #endif
        } 

        /*************************************************************************************************/
        /*!
            \brief mkgmTime  static 
                Converts GMT components into epoch time
            \param[in]  - tM - pointer to tm structure
            \return - 64 bit epoch time (seconds)
        */
        /*************************************************************************************************/
        int64_t OSDKSeasonalPlayTimeUtils::mkgmTime(struct tm *tM)
        {
            tM->tm_wday = 0;
            tM->tm_yday = 0;
            tM->tm_isdst = -1;

            int64_t t = mktime(tM);

            #ifdef EA_PLATFORM_WINDOWS
                t -= _timezone;
            #else
                t -= timezone;
            #endif

            if (tM->tm_isdst > 0) t += 3600; // daylight saving time is in effect

            return t;
        }

        
        void OSDKSeasonalPlayTimeUtils::logTime(const char8_t *strDescription, int64_t t)
        {
            struct tm tM;
            gmTime(&tM, t); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils].logTime(): " << strDescription << ": " << tM.tm_year + 1900 << "/" << tM.tm_mon + 1 << "/" << tM.tm_mday <<
                      " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");
        }

        void OSDKSeasonalPlayTimeUtils::getDurationAsString(int64_t t, char8_t *strBuffer, size_t bufferLen)
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

        bool OSDKSeasonalPlayTimeUtils::getStatsPeriodIds(Stats::PeriodIds& ids)
        {
            bool bSuccess = false;

            // get the stats slave
            Stats::StatsSlave* statsComponent = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID,false));
            if (NULL != statsComponent)
            {
                // get the stats period ids which has the rollover days/hours
                // to invoke this RPC will require authentication
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


        int64_t OSDKSeasonalPlayTimeUtils::getDailyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

            gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate today's rollover time
            tM.tm_hour = periodIds->getDailyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = mkgmTime(&tM);
            if (iRollOverTime <= iEpochTimeSecs)
            {
                // today's rollover has passed. add a day's worth of time
                iRollOverTime += SECS_PER_DAY;
            }

            gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getDailyRollOverTime(): next daily rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT"); 

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getWeeklyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

            gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate this week's rollover time
            tM.tm_mday += periodIds->getWeeklyDay() - tM.tm_wday;
            tM.tm_hour = periodIds->getWeeklyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = mkgmTime(&tM);
            if (iRollOverTime <= iEpochTimeSecs)
            {
                // this week's rollover has passed. add a week's worth of time
                iRollOverTime += SECS_PER_WEEK; 
            }

            gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getWeeklyRollOverTime(): next weekly rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" <<tM.tm_wday  << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getMonthlyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = 0;

            gmTime(&tM, iEpochTimeSecs);  // converts the iEpochTime into the tm struct

            // set the tm struct to indicate this week's rollover time
            tM.tm_mday = periodIds->getMonthlyDay();
            tM.tm_hour = periodIds->getMonthlyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;

            iRollOverTime = mkgmTime(&tM);
            if (iRollOverTime <= iEpochTimeSecs)
            {
                // this month's rollover has passed. add a month's worth of time
                ++tM.tm_mon;
                iRollOverTime = mkgmTime(&tM);
            }

            gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getMonthlyRollOverTime(): next monthly rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getPreviousDailyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = getDailyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous daily rollover is simply the next rollover - 1 day 
            iRollOverTime -= SECS_PER_DAY;

            gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getPreviousDailyRollOverTime(): previous daily rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getPreviousWeeklyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = getWeeklyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous weekly rollover is simply the next rollover - 7 days
            iRollOverTime -= SECS_PER_WEEK;

            gmTime(&tM, iRollOverTime); // convert the iRollOverTime back to the tm struct so we can display it
            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getPreviousWeeklyRollOverTime(): previous weekly rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");

            return iRollOverTime;
        }

        int64_t OSDKSeasonalPlayTimeUtils::getPreviousMonthlyRollOverTime(Stats::PeriodIds *periodIds, int64_t iEpochTimeSecs)
        {
            struct tm tM;   
            int64_t iRollOverTime = getMonthlyRollOverTime(periodIds, iEpochTimeSecs);

            // the previous monthly rollover time is the next rollover - 1 month. Need to us the tm struct to calculate this
            gmTime(&tM, iRollOverTime); // convert iRollover to a tm struct
            --tM.tm_mon;
            iRollOverTime = mkgmTime(&tM);

            TRACE_LOG("[OSDKSeasonalPlayTimeUtils:" << this << "].getPreviousMonthlyRollOverTime(): previous monthly rollover: " << tM.tm_year + 1900 <<
                      "/" << tM.tm_mon + 1 << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << ":" << tM.tm_min << ":" << tM.tm_sec << " GMT");

            return iRollOverTime;
        }

    } // OSDKSeasonalPlay
} // Blaze
