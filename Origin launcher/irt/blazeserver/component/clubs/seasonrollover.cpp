/*************************************************************************************************/
/*!
    \file   seasonrollover.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*************************************************************************************************/
/*!
    \class SeasonRollover
    Controls clubs season rollover. 
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

#include "time.h"
#include "seasonrollover.h"
#include "clubsdb.h"
#include "clubsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

SeasonRollover::~SeasonRollover()
{
    if (mRolloverTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mRolloverTimerId);
}

void SeasonRollover::clear()
{
    if (mRolloverTimerId != INVALID_TIMER_ID) 
        gSelector->cancelTimer(mRolloverTimerId);

    mRollover = INVALID_TIMER_ID;
    mStart = 0;
    mNumRetry = 0;
}
/*************************************************************************************************/
/*!
    \brief parseRolloverData
    Parses config file data to define rollover parameters.
    \param[in] - ConfigMap map containing data
    \return - true on success
*/
/*************************************************************************************************/
void SeasonRollover::parseRolloverData(const SeasonSettingItem &seasonSettings)
{
    TRACE_LOG("[SeasonRollover].parseRolloverData(): parsing rollover data");

    mRolloverItem = &(seasonSettings.getRollover());
}

/*************************************************************************************************/
/*!
    \brief getRolloverTimer
    Returns number of microseconds to the next rollover. 
    \param[in] - t - current epoch time in seconds. 
    \return - microseconds to the rollover
*/
/*************************************************************************************************/
int64_t SeasonRollover::getRolloverTimer(int64_t t)
{
    struct tm tM;

    gmTime(&tM, t);
    mRollover = mkgmTime(&tM);
    mStart = t;
    switch (mRolloverItem->getPeriod())
    {
    case CLUBS_PERIOD_DAILY:
        {
            tM.tm_hour = mRolloverItem->getHour();
            tM.tm_min = mRolloverItem->getMinute();
            tM.tm_sec = 0;
            mRollover = mkgmTime(&tM);

            if (mRollover <= t) 
            {
                mRollover += 24*3600; // today's rollover has passed
            }
            mStart = mRollover - 24 * 3600;
            TRACE_LOG("[SeasonRollover].getRolloverTimer: Setting mStart to (" << mStart << "), mRollover to (" << mRollover << ").");
            break;
        }
    case CLUBS_PERIOD_WEEKLY:
        {
            tM.tm_mday += mRolloverItem->getDay() - tM.tm_wday ;
            tM.tm_hour = mRolloverItem->getHour();
            tM.tm_min = mRolloverItem->getMinute();
            tM.tm_sec = 0;

            mRollover = mkgmTime(&tM);
            if (mRollover <= t) 
            {
                mRollover += 24 * 3600 * 7; // this week's rollover has passed
            }
            mStart = mRollover - 24 * 3600 * 7;
            break;
        }
    case CLUBS_PERIOD_MONTHLY:
        {
            tM.tm_hour = mRolloverItem->getHour();
            tM.tm_mday = mRolloverItem->getDay();
            tM.tm_min = mRolloverItem->getMinute();
            tM.tm_sec = 0;

            mRollover = mkgmTime(&tM);
            if (mRollover <= t) 
            {
                ++tM.tm_mon; // this month's rollover has passed
                mRollover = mkgmTime(&tM); 
            }

            if (tM.tm_mon == 0)
            {
                tM.tm_mon = 11;
                --tM.tm_year;
            }
            else
                --tM.tm_mon;
                
            mStart = mkgmTime(&tM); 
            
            break;
        }
    default:
        {
            ERR_LOG("[SeasonRollover].getRolloverTimer: invalid mPeriod(" << mRolloverItem->getPeriod() << ")");
            break;
        }
    }

    gmTime(&tM, mRollover);
    INFO_LOG("[SeasonRollover].getRolloverTimer: next rollover: " << (tM.tm_year + 1900) << "/" << (tM.tm_mon + 1) 
             << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << tM.tm_hour << "h:" << tM.tm_min << "m:" << tM.tm_sec << "s GMT");

    ClubsDatabase clubsDb;
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    t = TimeValue::getTimeOfDay().getSec();

    if (dbConn == nullptr)
    {
        WARN_LOG("[SeasonRollover].getRolloverTimer: Unable to get database connection.");
        return (mRollover - t) * 1000000;
    }

    clubsDb.setDbConn(dbConn);
    SeasonState currentState;
    int64_t seasonStartTime = 0;

    BlazeRpcError error = clubsDb.getSeasonState(currentState, seasonStartTime);
    t = TimeValue::getTimeOfDay().getSec();
    if (error != Blaze::ERR_OK)
    {
        WARN_LOG("[SeasonRollover].getRolloverTimer: Error fetching season state.");
        return (mRollover - t) * 1000000;
    }

    if ((currentState == CLUBS_IN_SEASON) &&
        (seasonStartTime != 0) &&
        (seasonStartTime < static_cast<int32_t>(mStart)))
    {
        // we missed a rollover while we were down
        // so kick one off in the next 15 secs
        TRACE_LOG("[SeasonRollover].getRolloverTimer: Rollover missed. Scheduling next rollover in 15s.");
        return 15 * 1000000;
    }

    return (mRollover - t) * 1000000;
}

/*************************************************************************************************/
/*!
    \brief setRolloverTimer
    Initializes timer at server start. Sets current season ID and state.
    \param[in] - none 
    \return - none
*/
/*************************************************************************************************/
void SeasonRollover::setRolloverTimer()
{
    int64_t timer = getRolloverTimer(TimeValue::getTimeOfDay().getSec());
    mRolloverTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay() + timer, this, &SeasonRollover::timerExpired,
            "SeasonRollover::timerExpired");
}

/*************************************************************************************************/
/*!
    \brief timerExpired
    season rollover timer callback. 
    \param[in]  - TimerId
    \return - none
*/
/*************************************************************************************************/
void SeasonRollover::timerExpired()
{
    setRolloverTimer(); // Schedule the next rollover

    mComponent->startEndOfSeason();
}

/*************************************************************************************************/
/*!
    \brief gmTime  static 
    Converts epoch time into GMT components
    \param[in]  - t - 64 bit epoch time
    \param[out]  - tM - pointer to tm structure
    \return - none
*/
/*************************************************************************************************/
void SeasonRollover::gmTime(struct tm *tM, int64_t t)
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
    \return - 64 bit epoch time
*/
/*************************************************************************************************/
int64_t SeasonRollover::mkgmTime(struct tm *tM)
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

} // namespace Clubs
} // namespace Blaze
