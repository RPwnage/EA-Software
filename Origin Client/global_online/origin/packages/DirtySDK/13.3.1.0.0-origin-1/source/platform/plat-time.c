/*H********************************************************************************/
/*!
    \File plat-time.c

    \Description
        This module implements variations on the standard library time functions.
        The originals had a number of problems with internal static storage,
        bizarre parameter passing (pointers to input values instead of the
        actual value) and time_t which is different on different platforms.

        All these functions work with uint32_t values which provide a time
        range of 1970-2107.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 01/11/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"

#if (defined(DIRTYCODE_PS3))
#include <cell/rtc.h>
#elif (defined(DIRTYCODE_PSP2))
#ifndef SCE_RTC_USE_LIBC_TIME_H
#define SCE_RTC_USE_LIBC_TIME_H 1
#endif
#include <rtc.h>
#elif (defined(DIRTYCODE_XENON))
#include <xtl.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ds_strtoint

    \Description
        Converts a string number to an integer.  Does not support sign.

    \Input *pStr        - pointer to int to read
    \Input *pValue      - [out] storage for result

    \Output
        const char *    - pointer past end of number

    \Version 12/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ds_strtoint(const char *pStr, int32_t *pValue)
{
    int32_t iNum;
    for (iNum = 0; ((*pStr >= '0') && (*pStr <= '9')); pStr += 1)
    {
        iNum = (iNum * 10) + (*pStr & 15);
    }
    *pValue = iNum;
    return(pStr);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ds_timeinsecs

    \Description
        This function replaces the standard library time() function. Main differences
        are the missing pointer parameter (not needed) and the uint32_t return
        value. The function returns 0 on unsupported platforms vs time which returns
        -1.

    \Output uint32_t - number of elapsed seconds since Jan 1, 1970.

    \Version 01/12/2005 (gschaefer)
*/
/********************************************************************************F*/
uint32_t ds_timeinsecs(void)
{
    return((uint32_t)time(NULL));
}

/*F********************************************************************************/
/*!
    \Function ds_timezone

    \Description
        This function returns the current system timezone as its offset from GMT in
        seconds. There is no direct equivilent function in the standard C libraries.

    \Output int32_t - local timezone offset from GMT in seconds

    \Version 11/06/2002 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_timezone(void)
{
    time_t iGmt, iLoc;
    static int32_t iZone = -1;

    // just calc the timezone one time
    if (iZone == -1)
    {
        #if defined(DIRTYCODE_PS3)
        {
            CellRtcDateTime Gmt, Loc;

            // get gmt time
            cellRtcGetCurrentClock(&Gmt, 0);
            cellRtcGetTime_t(&Gmt, &iGmt);

            // get local time
            cellRtcGetCurrentClockLocalTime(&Loc);
            cellRtcGetTime_t(&Loc, &iLoc);
        }
        #elif defined(DIRTYCODE_PSP2)
        {
            SceDateTime Gmt, Loc;

            // get gmt time
            sceRtcGetCurrentClock(&Gmt, 0);
            sceRtcGetTime_t(&Gmt, &iGmt);

            // get local time
            sceRtcGetCurrentClockLocalTime(&Loc);
            sceRtcGetTime_t(&Loc, &iLoc);
        }
        #elif defined(DIRTYCODE_XENON)
        {
            TIME_ZONE_INFORMATION ZoneInfo;
            DWORD dwResult = GetTimeZoneInformation(&ZoneInfo);
            switch(dwResult)
            {
                case TIME_ZONE_ID_STANDARD:
                    iLoc = -((ZoneInfo.Bias + ZoneInfo.StandardBias) * 60);
                    break;
                case TIME_ZONE_ID_DAYLIGHT:
                    iLoc = -((ZoneInfo.Bias + ZoneInfo.DaylightBias)) * 60;
                    break;
                case TIME_ZONE_ID_UNKNOWN:
                default:
                    iLoc = 0;
                    break;
            }
            iGmt = 0;
        }
        #else // standard c library version
        {
            time_t uTime = time(0);
            struct tm *pTm, TmTime;

            // convert to gmt time
            pTm = gmtime(&uTime);
            iGmt = (uint32_t)mktime(pTm);

            // convert to local time
            pTm = ds_localtime(&TmTime, uTime);
            iLoc = (uint32_t)mktime(pTm);
        }
        #endif

        // calculate timezone difference
        iZone = iLoc - iGmt;
    }

    // return the timezone offset in seconds
    return(iZone);
}

/*F********************************************************************************/
/*!
    \Function ds_localtime

    \Description
        This converts the input GMT time to the local time as specified by the
        system clock.  This function follows the re-entrant localtime_r function
        signature.

    \Input *tm          - storage for localtime output
    \Input elap         - GMT time

    \Output
        struct tm *     - pointer to localtime result

    \Version 04/23/2008 (jbrookes)
*/
/********************************************************************************F*/
struct tm *ds_localtime(struct tm *tm, uint32_t elap)
{
    time_t uTimeT = (time_t)elap;

    #if defined(DIRTYCODE_PS3)
    {
        CellRtcDateTime GmtTime, LocTime;
        CellRtcTick GmtTick, LocTick;

        // convert GMT to DateTime
        cellRtcSetTime_t(&GmtTime, uTimeT);
        // convert DateTime to Tick
        cellRtcGetTick(&GmtTime, &GmtTick);
        // convert UTC tick to Local tick
        cellRtcConvertUtcToLocalTime(&GmtTick, &LocTick);
        // convert Tick to DateTime
        cellRtcSetTick(&LocTime, &LocTick);
        // convert DateTime to posix time_t
        cellRtcGetTime_t(&LocTime, &uTimeT);
        // convert time_t to struct tm
        ds_secstotime(tm, (uint32_t)uTimeT);
        return(tm);
    }
    #elif defined(DIRTYCODE_PSP2)
    struct tm *tm2 = localtime(&uTimeT);
    memcpy(tm, tm2, sizeof(*tm));
    return(tm);
    #elif defined(DIRTYCODE_PC) || defined(DIRTYCODE_XENON)
    localtime_s(tm, &uTimeT);
    return(tm);
    #elif defined(DIRTYCODE_WINRT) || defined(DIRTYCODE_WINPRT)
    uTimeT = (time_t)0;
    return(NULL);
    #else
    return(localtime_r(&uTimeT, tm));
    #endif
}

/*F********************************************************************************/
/*!
    \Function ds_secstotime

    \Description
        Convert elapsed seconds to discrete time components. This is essentially a
        ds_localtime() replacement with better syntax that is available on all platforms.

    \Input *tm  - target component record
    \Input elap - epoch time input

    \Output struct tm * - returns tm if successful, NULL if failed

    \Version 01/23/2000 (gschaefer)
*/
/********************************************************************************F*/
struct tm *ds_secstotime(struct tm *tm, uint32_t elap)
{
    int32_t year;
    int32_t leap;
    int32_t next;
    int32_t days;
    int32_t secs;
    const int32_t *mon;

    // table to find days per month
    static const int32_t dayspermonth[24] = {
        31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, // leap
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  // norm
    };

    // divide out secs within day and days
    // (we ignore leap-seconds cause it requires tables and nasty stuff)
    days = elap / (24*60*60);
    secs = elap % (24*60*60);

    // store the time info
    tm->tm_sec = secs % 60;
    secs /= 60;
    tm->tm_min = secs % 60;
    secs /= 60;
    tm->tm_hour = secs;

    // determine what year we are in
    for (year = 1970;; year = next) {
        // calc the length of the year in days
        leap = (((year & 3) == 0) && (((year % 100 != 0) || ((year % 400) == 0))) ? 366 : 365);
        // see if date is within this year
        if (days < leap)
            break;

        // estimate target year assuming every year is a leap year
        // (this may underestimate the year, but will never overestimate)
        next = year + (days / 366);

        // make sure year got changed and force if it did not
        /// (can happen on dec 31 of non-leap year)
        if (next == year)
            ++next;

        // subtract out normal days/year
        days -= (next - year) * 365;
        // add in leap years from previous guess
        days += ((year-1)/4 - (year-1)/100 + (year-1)/400);
        // subtract out leap years since new guess
        days -= ((next-1)/4 - (next-1)/100 + (next-1)/400);
    }

    // save the year and day within year
    tm->tm_year = year - 1900;
    tm->tm_yday = days;
    // calc the month
    mon = dayspermonth + 12*(leap&1);
    for (tm->tm_mon = 0; days >= *mon; tm->tm_mon += 1)
        days -= *mon++;
    // save the days
    tm->tm_mday = days + 1;
    tm->tm_isdst = 0;

    // return pointer to argument to make it closer to ds_localtime()
    return(tm);
}

/*F********************************************************************************/
/*!
    \Function ds_timetosecs

    \Description
        Convert discrete components to elapsed time.

    \Input *tm  - source component record

    \Output uint32_t - zero=failure, else epoch time

    \Version 01/23/2000 (gschaefer)
*/
/********************************************************************************F*/
uint32_t ds_timetosecs(const struct tm *tm)
{
    int32_t res;
    struct tm cmp;
    uint32_t min;
    uint32_t max;
    uint32_t mid;

    // do a binary search using _SecsToTime to encode prospective time_t values
    // though iterative, it requires max of 32 cycles which is actually pretty
    // good considering the complexity of the calculation (this allows all the
    // time nastiness to remain in a single function).

    // do binary search
    min = 0;
    max = (uint32_t)-1;

    // silence compiler warning
    res = 0;
    mid = 0;

    // perform binary search
    while (min <= max) {
        // test at midpoint -- since these are large unsigned values, they can
        // overflow in the (min+max)/2 case hense the individual divide and
        // lost bit recovery
        mid = (min/2)+(max/2)+(min&max&1);
        // do the time conversion
        ds_secstotime(&cmp, mid);
        // do the compare
        if ((res = (cmp.tm_year - tm->tm_year)) == 0) {
            if ((res = (cmp.tm_mon - tm->tm_mon)) == 0) {
                if ((res = (cmp.tm_mday - tm->tm_mday)) == 0) {
                    if ((res = (cmp.tm_hour - tm->tm_hour)) == 0) {
                        if ((res = (cmp.tm_min - tm->tm_min)) == 0) {
                            if ((res = cmp.tm_sec - tm->tm_sec) == 0) {
                                // got an exact match!
                                break;
                            }
                        }
                    }
                }
            }
        }

        // force break once min/max converge
        // (cannot do this within while condition as res will not be setup correctly)
        if (min == max)
            break;

        // narrow the search range
        if (res > 0)
            max = mid-1;
        else
            min = mid+1;
    }

    // return converted time or zero if failed
    return(res == 0 ? mid : 0);
}

/*F********************************************************************************/
/*!
    \Function ds_plattimetotime (plattimetotime)

    \Description
        This converts the input platform-specific time data structure to the
        generic time data structure.

    \Input *tm          - generic time data structure to be filled by the function
    \Input *pPlatTime   - pointer to the platform-specific data structure

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Notes
        \verbatim
        Data type to be used with plattime:
            On PS3:     CellRtcTick
            On Xenon:   FILETIME
        \endverbatim

    \Version 05/08/2010 (mclouatre)
*/
/********************************************************************************F*/
struct tm *ds_plattimetotime(struct tm *tm, void *pPlatTime)
{
    #if defined(DIRTYCODE_PS3)
    CellRtcTick *pPs3Tick = (CellRtcTick *)pPlatTime;
    CellRtcDateTime ps3tm;

    // convert ps3 tick to ps3 datetime
    cellRtcConvertTickToDateTime(pPs3Tick, &ps3tm);

    // convert ps3 datetime to generic datetime
    // (ignore day of week (tm_wday), day of year (tm_yday) and day saving time flag (tm_isdst))
    tm->tm_sec = ps3tm.second;
    tm->tm_min = ps3tm.minute;
    tm->tm_hour = ps3tm.hour;
    tm->tm_mday = ps3tm.day;
    tm->tm_mon = ps3tm.month - 1;
    tm->tm_year = ps3tm.year - 1900;
    tm->tm_wday = 0;
    tm->tm_yday = 0;
    tm->tm_isdst = 0;
    #elif defined(DIRTYCODE_XENON)
    PFILETIME pXenonTick = (FILETIME *)pPlatTime;
    SYSTEMTIME xenontm;

    // convert xenon tick to xenon datetime
    FileTimeToSystemTime(pXenonTick, &xenontm);

    // convert ps3 datetime to generic datetime
    // (ignore day of year (tm_yday) and day saving time flag (tm_isdst))
    tm->tm_sec = xenontm.wSecond;
    tm->tm_min = xenontm.wMinute;
    tm->tm_hour = xenontm.wHour;
    tm->tm_mday = xenontm.wDay;
    tm->tm_mon = xenontm.wMonth - 1;
    tm->tm_year = xenontm.wYear - 1900;
    tm->tm_wday = xenontm.wDayOfWeek;
    tm->tm_yday = 0;
    tm->tm_isdst = 0;
    #else
    tm = NULL;
    #endif

    return(tm);
}

/*F********************************************************************************/
/*!
    \Function ds_timetostr

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pTm       - input time stucture to be converted to string
    \Input eConvType  - user-selected conversion type
    \Input bLocalTime - whether input time is local time or UTC 0 offset time.
    \Input *pStrBuf   - user-provided buffer to be filled with datetime string
    \Input iBufSize   - size of output buffer (must be at least 18 bytes to receive null-terminated yyyy-MM-ddTHH:mm:ssZ

    \Output
        char *        - zero=failure, else epoch time

    \Version 07/12/2012 (jbrookes)
*/
/********************************************************************************F*/
char *ds_timetostr(const struct tm *pTm, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStrBuf, int32_t iBufSize)
{
    switch(eConvType)
    {
        case TIMETOSTRING_CONVERSION_ISO_8601:
            ds_snzprintf(pStrBuf, iBufSize, "%04d-%02d-%02dT%02d:%02d:%02d%s",
                pTm->tm_year+1900, pTm->tm_mon+1, pTm->tm_mday,
                pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
                !bLocalTime ? "Z" : "");
            break;
        default:
            // unsupported conversion type
            pStrBuf = NULL;
            break;
    }

    return(pStrBuf);
}

/*F********************************************************************************/
/*!
    \Function ds_strtotime

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pStr - textual date string

    \Output uint32_t - zero=failure, else epoch time

    \Notes
        ($todo) document supported input string formats here

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
uint32_t ds_strtotime(const char *pStr)
{
    int32_t i;
    const char *s;
    struct tm tm;
    static const char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0 };
    static const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0 };

    // reset the fields
    memset(&tm, -1, sizeof(tm));

    // skip past any white space
    while ((*pStr != 0) && (*pStr <= ' '))
    {
        pStr++;
    }

    // see if starts with day of week
    for (i = 0; (s=wday[i]) != 0; ++i)
    {
        if ((pStr[0] == s[0]) && (pStr[1] == s[1]) && (pStr[2] == s[2]))
        {
            tm.tm_wday = i;
            // skip name of weekday
            while ((*pStr != ',') && (*pStr != ' ') && (*pStr != 0))
                ++pStr;
            // skip the divider
            while ((*pStr == ',') || (*pStr == ' '))
                ++pStr;
            break;
        }
    }

    // check for mmm dd
    if ((*pStr < '0') || (*pStr > '9'))
    {
        for (i = 0; (s=month[i]) != 0; ++i)
        {
            // look for a match
            if ((pStr[0] != s[0]) || (pStr[1] != s[1]) || (pStr[2] != s[2]))
                continue;
            // found the month
            tm.tm_mon = i;
            // skip to the digits
            while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
                ++pStr;
            // get the day of month
            for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
                i = (i * 10) + (*pStr & 15);
            if (i > 0)
                tm.tm_mday = i;
            break;
        }
    }

    // check for dd mmm
    if ((tm.tm_mon < 0) && (pStr[0] >= '0') && (pStr[0] <= '9') &&
        ((pStr[1] > '@') || (pStr[2] > '@') || (pStr[3] > '@')))
    {
        // get the day
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mday = i;
        while (*pStr < '@')
            ++pStr;
        // get the month
        for (i = 0; (s=month[i]) != 0; ++i)
        {
            // look for a match
            if ((pStr[0] != s[0]) || (pStr[1] != s[1]) || (pStr[2] != s[2]))
                continue;
            tm.tm_mon = i;
            while ((*pStr != 0) && (*pStr != ' '))
                ++pStr;
            break;
        }
    }

    // check for xx/xx or xx/xx/xx pStr
    if ((*pStr >= '0') && (*pStr <= '9') && (tm.tm_mon < 0))
    {
        // get the month
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mon = i - 1;
        if (*pStr != 0)
            ++pStr;
        // get the day
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        tm.tm_mday = i;
        if (*pStr != 0)
            ++pStr;
    }

    // check for year
    while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
        ++pStr;
    // see if the year is here
    if ((pStr[0] >= '0') && (pStr[0] <= '9') && (pStr[1] != ':') && (pStr[2] != ':'))
    {
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        if (i > 999)
            tm.tm_year = i;
        else if (i >= 50)
            tm.tm_year = 1900+i;
        else
            tm.tm_year = 2000+i;
        // find next digit sequence
        while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
            ++pStr;
    }

    // save the hour
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_hour = i;
        if (*pStr == ':')
            ++pStr;
    }

    // save the minute
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_min = i;
        if (*pStr == ':')
            ++pStr;
    }

    // save the second (if present)
    if ((*pStr >= '0') && (*pStr <= '9'))
    {
        i = (*pStr++ & 15);
        if ((*pStr >= '0') && (*pStr <= '9'))
            i = (i * 10) + (*pStr++ & 15);
        tm.tm_sec = i;
    }

    // see if year is still remaining
    if (tm.tm_year < 0)
    {
        // see if any digits left
        while ((*pStr != 0) && ((*pStr < '0') || (*pStr > '9')))
            ++pStr;
        for (i = 0; ((*pStr >= '0') && (*pStr <= '9')); ++pStr)
            i = (i * 10) + (*pStr & 15);
        if (i > 999)
            tm.tm_year = i;
    }

    // make year relative to 1900 (really dumb)
    if (tm.tm_year > 1900)
        tm.tm_year -= 1900;

    // convert from struct tm to uint32_t and return to caller
    return(ds_timetosecs(&tm));
}

/*F********************************************************************************/
/*!
    \Function ds_strtotime2

    \Description
        Converts a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pStr        - textual date string
    \Input eConvType    - time format to convert from

    \Output
        uint32_t        - zero=failure, else epoch time

    \Notes
        ($todo) document supported input string formats here

    \Version 12/13/2012 (jbrookes)
*/
/********************************************************************************F*/
uint32_t ds_strtotime2(const char *pStr, TimeToStringConversionTypeE eConvType)
{
    uint32_t uTime = 0;

    if (eConvType == TIMETOSTRING_CONVERSION_ISO_8601)
    {
        // format: YYYY-MM-DDTHH:MM:SSZ
        if (strlen(pStr) >= 19) // 'Z' is optional
        {
            struct tm tm;
            // read the date/time
            pStr = _ds_strtoint(pStr, &tm.tm_year) + 1;     // get the year, skip hypen
            pStr = _ds_strtoint(pStr, &tm.tm_mon) + 1;      // get the month, skip hypen
            pStr = _ds_strtoint(pStr, &tm.tm_mday) + 1;     // get the day, skip 'T'
            pStr = _ds_strtoint(pStr, &tm.tm_hour) + 1;     // get the hour, skip ':'
            pStr = _ds_strtoint(pStr, &tm.tm_min) + 1;      // get the minute, skip ':'
            pStr = _ds_strtoint(pStr, &tm.tm_sec) + 1;      // get the second, skip ':'
            //$$TODO - do something with 'Z'?
            // adjust year and month
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            // convert from struct tm to uint32_t and return to caller
            uTime = ds_timetosecs(&tm);
        }
    }
    else
    {
        uTime = ds_strtotime(pStr);
    }
    return(uTime);
}
