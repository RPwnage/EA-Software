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

#include <string.h>
#include <stdint.h>

#include "platform.h"

#if (DIRTYCODE_PLATFORM == DIRTYCODE_PS2) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS2EE)
#include <eekernel.h>
#include <libcdvd.h>
#include <libscf.h>
#endif

#if (DIRTYCODE_PLATFORM == DIRTYCODE_PSP)
#include <rtcsvc.h>
#endif

#if (DIRTYCODE_PLATFORM == DIRTYCODE_PS3)
#include <cell/rtc.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ds_timeinsecs (timeinsecs)

    \Description
        This function replaces the standard library time() function. Main differences
        are the missing pointer parameter (not needed) and the uint32_t return
        value. The function returns 0 on unsupported platforms vs time which returns
        -1.

    \Input None
    
    \Output uint32_t - number of elapsed seconds since Jan 1, 1970.

    \Version 01/12/2005 (gschaefer)
*/
/********************************************************************************F*/
uint32_t ds_timeinsecs(void)
{
    #if (DIRTYCODE_PLATFORM == DIRTYCODE_PS2) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS2EE) || \
        (DIRTYCODE_PLATFORM == DIRTYCODE_PSP) || (DIRTYCODE_PLATFORM == DIRTYCODE_IOP) || \
        (DIRTYCODE_PLATFORM == DIRTYCODE_DS)  || (DIRTYCODE_PLATFORM == DIRTYCODE_REVOLUTION)
    {
        return(0);
    }
    #else
    {
        return((uint32_t)time(NULL));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function ds_timezone (timezone)

    \Description
        This function returns the current system timezone as its offset from GMT in
        seconds. There is no direct equivilent function in the standard C libraries.

    \Input None
    
    \Output int32_t - local timezone offset from GMT in seconds

    \Version 11/06/2002 (jbrookes)
*/
/********************************************************************************F*/
uint32_t ds_timezone(void)
{
    time_t iGmt, iLoc;
    static int32_t iZone = -1;

    // just calc the timezone one time
    if (iZone == -1)
    {
        // ps2 specific
        #if (DIRTYCODE_PLATFORM == DIRTYCODE_PS2) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS2EE)
        {
            #if 1
            sceCdCLOCK Gmt, Loc;

            // get current time (native format is JST) and make a copy
            memset(&Gmt, 0, sizeof(Gmt));
            sceCdReadClock(&Gmt);
            memcpy(&Loc, &Gmt, sizeof(Loc));

            // get gmt time
            sceScfGetGMTfromRTC(&Gmt);
            iGmt = ((Gmt.day>>4)*10+(Gmt.day&15))*86400 + ((Gmt.hour>>4)*10+(Gmt.hour&15))*3600 +
                ((Gmt.minute>>4)*10+(Gmt.minute&15))*60 + ((Gmt.second>>4)*10+(Gmt.second&15))*1;

            // get local time
            sceScfGetLocalTimefromRTC(&Loc);
            iLoc = ((Loc.day>>4)*10+(Loc.day&15))*86400 + ((Loc.hour>>4)*10+(Loc.hour&15))*3600 +
                ((Loc.minute>>4)*10+(Loc.minute&15))*60 + ((Loc.second>>4)*10+(Loc.second&15))*1;
            #else
            //$$ jbrookes - this needs to be tested to see if it works correctly
            iGmt = 0;
            iLoc = sceScfGetTimeZone() * 60;
            #endif
        }
        #elif DIRTYCODE_PLATFORM == DIRTYCODE_PSP
        {
            ScePspDateTime Gmt, Loc;

            // get gmt time
            sceRtcGetCurrentClock(&Gmt, 0);
            sceRtcGetTime_t(&Gmt, &iGmt);
            
            // get local time
            sceRtcGetCurrentClockLocalTime(&Loc);
            sceRtcGetTime_t(&Loc, &iLoc);
        }
        #elif DIRTYCODE_PLATFORM == DIRTYCODE_PS3
        {
            CellRtcDateTime Gmt, Loc;

            // get gmt time
            cellRtcGetCurrentClock(&Gmt, 0);
            cellRtcGetTime_t(&Gmt, &iGmt);
            
            // get local time
            cellRtcGetCurrentClockLocalTime(&Loc);
            cellRtcGetTime_t(&Loc, &iLoc);
        }
        #elif (DIRTYCODE_PLATFORM == DIRTYCODE_IOP) || (DIRTYCODE_PLATFORM == DIRTYCODE_DS) || (DIRTYCODE_PLATFORM == DIRTYCODE_REVOLUTION)
        iLoc = iGmt = 0;
        #else // standard c library version
        {
            time_t uTime = time(0);
            struct tm *pTm;
        
            // convert to gmt time
            pTm = gmtime(&uTime);
            iGmt = (uint32_t)mktime(pTm);

            // convert to local time            
            pTm = localtime(&uTime);
            iLoc = (uint32_t)mktime(pTm);
        }
        #endif

        // calculate timezone difference
        iZone = (int32_t)(iLoc - iGmt);
    }
    
    // return the timezone offset in seconds
    return(iZone);
}

/*F********************************************************************************/
/*!
    \Function ds_secstotime (secstotime)

    \Description
        Convert elapsed seconds to discrete time components. This is essentially a
        localtime() replacement with better syntax that is available on all platforms.

    \Input *tm - target component record
    \Input et  - epoch time input

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

    // return pointer to argument to make it closer to localtime()
    return(tm);
}

/*F********************************************************************************/
/*!
    \Function ds_timetosecs (timetosecs)

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
    max = UINT32_MAX;

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
    \Function ds_strtotime (strtotime)

    \Description
        Convert a date formatted in a number of common Unix and Internet formats
        and convert to a struct tm.

    \Input *pStr - textual date string

    \Output uint32_t - zero=failure, else epoch time

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
        tm.tm_mon = i;
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
        while ((*pStr != 0) && ((*pStr < '0') && (*pStr > '9')))
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
