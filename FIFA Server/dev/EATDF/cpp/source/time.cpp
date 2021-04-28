/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <EATDF/time.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EADateTime.h>
#include <string.h>     // memset

#if defined(EA_PLATFORM_WINDOWS)
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
// if <winsock.h> was included, do not include <winsock2.h> as there are conflicting definitions; 
// we skip this if using a special directive, because some codebases (Frostbite) define _WINSOCKAPI_
// globally to avoid winsock.h entirely.
#if !defined(_WINSOCKAPI_) || defined(EA_FORCE_WINSOCK2_INCLUDE)
#pragma warning(push,0)
  #if defined(_WINSOCKAPI_)
  #pragma warning(disable : 4005)   // Disable the 4005 error (_WINSOCKAPI_ defined 2x) if we're in this block because of EA_FORCE_WINSOCK2_INCLUDE
  #endif

  #include <winsock2.h>
  #include <windows.h>
#pragma warning(pop)
#endif
#elif defined(EA_PLATFORM_LINUX)
  #include <sys/time.h>
  #include <time.h>
#elif defined(EA_PLATFORM_KETTLE)
  #include <kernel.h>
#elif defined(EA_PLATFORM_CAPILANO) || defined(EA_PLATFORM_XBSX) || defined(_GAMING_XBOX_SCARLETT)
  #pragma warning(push,0)
  #include <winsock2.h>
  #include <windows.h>
  #pragma warning(pop)
#endif

namespace EA
{
namespace TDF
{
 

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  116444736000000000ULL
#endif

#if defined(EA_PLATFORM_WINDOWS)
TimeValue::TimeValue(const FILETIME& tv)
{
    LARGE_INTEGER li;
    li.LowPart = tv.dwLowDateTime;
    li.HighPart = (LONG)(tv.dwHighDateTime);
    mTime = ((int64_t)li.QuadPart - DELTA_EPOCH_IN_MICROSECS) / 10;
}

int64_t TimeValue::getWindowsHighFrequencyCounter()
{
    LARGE_INTEGER freq;
    ::QueryPerformanceFrequency(&freq);
    return ((int64_t) freq.QuadPart);
}

double TimeValue::msFreqsPerMicroSecond = ((double)TimeValue::getWindowsHighFrequencyCounter()) / 1000000;
#endif

const char8_t* TimeValue::sDateTimeFormats[] =
{
    "%Y/%m/%d-%H:%M:%S",
    "%Y/%m/%d-%H:%M",
    "%Y/%m/%d-%H",
    "%Y/%m/%d"
};

const char8_t* TimeValue::sTimeFormats[] =
{
    "%H:%M:%S",
    "%H:%M",
    "%H"
};

/*** Public Methods ******************************************************************************/

TimeValue::TimeValue(const struct timeval& tv)
{
    mTime = (((int64_t)tv.tv_sec) * 1000000) + tv.tv_usec;
}

TimeValue::TimeValue(
        uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second)
{
    mTime = TimeValue::getEpochFromGmTimeComponents(year, month, day, hour, minute, second);
}

TimeValue::TimeValue(const char8_t *timeString, TimeValue::TimeFormat format) : mTime(0)
{
    switch (format)
    {
        case TIME_FORMAT_INTERVAL:
            parseTimeInterval(timeString);
        break;
        case TIME_FORMAT_GMDATE:
            parseGmDateTime(timeString);
        break;
        default: //nothing 
        break;        
    };
}

const char8_t* TimeValue::toStringBasedOnValue(char8_t* buffer, size_t len) const
{
    // (<946684800000000 (New Years 2000) (30 years) uses interval format)
    if (mTime < 946684800000000)
        return toIntervalString(buffer, len);
    else
        return toString(buffer, len);
}

const char8_t* TimeValue::toString(char8_t* buffer, size_t len) const
{
    uint32_t year, month, day, hour, minute, second, millis;
    year = month = day = hour = minute = second = millis = 0;

    TimeValue::getGmTimeComponents(*this, &year, &month, &day, &hour, &minute, &second, &millis);
    EA::StdC::Snprintf(buffer, len, "%d/%02d/%02d-%02d:%02d:%02d.%03d", year, month, day,
        hour, minute, second, millis);
    return buffer;
}

const char8_t* TimeValue::toDateFormat(char8_t* buffer, size_t len) const
{
    uint32_t year, month, day, hour, minute, second, millis;
    year = month = day = hour = minute = second = millis = 0;

    TimeValue::getGmTimeComponents(*this, &year, &month, &day, &hour, &minute, &second, &millis);
    EA::StdC::Snprintf(buffer, len, "%d-%02d-%02d %02d:%02d:%02d", year, month, day,
        hour, minute, second);
    return buffer;
}

/*! ***************************************************************************/
/*! \brief Format time in format for Account calls like: "2008-09-28T16:56Z".

\param output buffer and len.
\return Formatted string.
*******************************************************************************/
const char8_t* TimeValue::toAccountString(char8_t* buffer, size_t len, bool includeSeconds/* = false*/) const
{
    uint32_t year, month, day, hour, minute, second, millis;
    year = month = day = hour = minute = second = millis = 0;

    TimeValue::getGmTimeComponents(*this, &year, &month, &day, &hour, &minute, &second, &millis);
    if (includeSeconds)
    {
        EA::StdC::Snprintf(buffer, len, "%d-%02d-%02dT%02d:%02d:%02dZ", year, month, day,
            hour, minute, second);
    }
    else
    {
        EA::StdC::Snprintf(buffer, len, "%d-%02d-%02dT%02d:%02dZ", year, month, day,
            hour, minute);
    }
    return buffer;
}

/*! ***************************************************************************/
/*! \brief Format time interval string in format like: "300d:17h:33m:59s:200ms".

\param output buffer and len.
\return Formatted string.
*******************************************************************************/
const char8_t* TimeValue::toIntervalString(char8_t* buffer, size_t len) const
{
    if (len <= 0)
        return buffer;

    uint64_t remainingTime = (uint64_t) ((mTime >= 0) ? mTime : -mTime);
    bool negative = (mTime < 0);
   
    static const char8_t* timeSuffixes[] = { "y", "d", "h", "m", "s", "ms" };
    uint64_t timeComponents[EAArrayCount(timeSuffixes)];
    memset(timeComponents, 0, sizeof(timeComponents));

    size_t index = (EAArrayCount(timeSuffixes)) - 1;
    remainingTime = (uint64_t)(remainingTime/1000LL);
    timeComponents[index--] = (uint64_t)(remainingTime%1000LL); // milliseconds
    remainingTime = (uint64_t)(remainingTime/1000LL);
    timeComponents[index--] = (uint64_t)(remainingTime%60LL); // seconds
    remainingTime = (uint64_t)(remainingTime/60LL);
    timeComponents[index--] = (uint64_t)(remainingTime%60LL); // minutes
    remainingTime = (uint64_t)(remainingTime/60LL);
    timeComponents[index--] = (uint64_t)(remainingTime%24LL); // hours
    remainingTime = (uint64_t)(remainingTime/24LL);
    timeComponents[index--] = (uint64_t)(remainingTime%365LL); // days
    timeComponents[index] = (uint64_t)(remainingTime/365LL); // years


    buffer[0] = '\0';

    char8_t* b = buffer;
    int32_t availableSpace = (int32_t)len;
    int32_t spaceNeeded;
    bool empty = true;

    if (negative)
    {
        spaceNeeded = EA::StdC::Snprintf(b, availableSpace, "-");
        if (spaceNeeded >= availableSpace)
            return buffer;
        availableSpace -= spaceNeeded;
        b += spaceNeeded;
    }

    for(size_t idx = 0; idx < EAArrayCount(timeComponents); ++idx)
    {
        if (timeComponents[idx] > 0)
        {
            spaceNeeded = EA::StdC::Snprintf(b, availableSpace, "%s%" PRIu64 "%s", empty ? "" : ":", timeComponents[idx], timeSuffixes[idx]);
            empty = false;
            if (spaceNeeded >= availableSpace)
                return buffer;
            availableSpace -= spaceNeeded;
            b += spaceNeeded;
        }
    }

    // If the value passed in is less than 1ms, then just print out the number of microseconds
    if (empty)
    {
        spaceNeeded = EA::StdC::Snprintf(b, availableSpace, "%" PRId64 "us", mTime);
        if (spaceNeeded >= availableSpace)
            return buffer;
        availableSpace -= spaceNeeded;
        b += spaceNeeded;
    }

    return buffer;
}
/*! ***************************************************************************/
/*! \brief Parses time string (as GMT) in format like: "16:56:08", and prepends CURRENT date.

    Caller is allowed to omit TRAILING components:
        16:56
        16
    But it may be preferable to make it more explicit:
        16:56:00
        16:00:00

    \param str The input string.
    \return true upon success or false upon error.
*******************************************************************************/
bool TimeValue::parseGmTime(const char8_t* str)
{
    return parseDateTime(str, sTimeFormats, EAArrayCount(sTimeFormats), true, true);
}

/*! ***************************************************************************/
/*! \brief Parses time string (as Local Time) in format like: "16:56:08", and prepends CURRENT date.

    Caller is allowed to omit TRAILING components:
        16:56
        16
    But it may be preferable to make it more explicit:
        16:56:00
        16:00:00

    \param str The input string.
    \return true upon success or false upon error.
*******************************************************************************/
bool TimeValue::parseLocalTime(const char8_t* str)
{
    return parseDateTime(str, sTimeFormats, EAArrayCount(sTimeFormats), false, true);
}

/*! ***************************************************************************/
/*! \brief Parses date/time string in format like: "2008/09/28-16:56:08".

    Caller is allowed to omit TRAILING components:
        2008/09/28-16:56
        2008/09/28
    But it may be preferable to make it more explicit:
        2008/09/28-16:56:00
        2008/09/28-00:00:00

    \param str The input string.
    \return true upon success or false upon error.
*******************************************************************************/
bool TimeValue::parseGmDateTime(const char8_t* str)
{
    return parseDateTime(str, sDateTimeFormats, EAArrayCount(sDateTimeFormats), true, false);
}

bool TimeValue::parseLocalDateTime(const char8_t* str)
{
    return parseDateTime(str, sDateTimeFormats, EAArrayCount(sDateTimeFormats), false, false);
}

bool TimeValue::parseDateTime(const char8_t* str, const char8_t* formats[], size_t numFormats, bool inGmt, bool prependsCurrentDateTime)
{
    struct tm timeInfo;
    char* s = nullptr;
    for(size_t i = 0; i < numFormats; ++i)
    {
        memset(&timeInfo, 0, sizeof(timeInfo));
        
        if (prependsCurrentDateTime)
        {
            time_t rawTime;
            time(&rawTime);

            // retrieve the current date time
            if (inGmt)
                timeInfo = *(gmtime(&rawTime));
            else
                timeInfo = *(localtime(&rawTime));
        }

        // replace with string data supplied
        s = EA::StdC::Strptime(str, formats[i], &timeInfo);
        if (s != nullptr)
        {
            if (*s != '\0')
            {
                // There are unparsed characters so reject string
                return false;
            }

            // save the date time
            if (inGmt)
                mTime = mkgmTime(&timeInfo) * 1000 * 1000; // change to microseconds
            else
                mTime = mktime(&timeInfo) * 1000 * 1000; // change to microseconds
            return true;
        }
    }
    return false;
}

/*! ***************************************************************************/
/*! \brief Parses time interval string in format like: "300d:17h:33m:59s:200ms".

    Caller can provide any combination of time components:
        300d:17h:33m
        300d:17h
        33m:59s
        300d
        17h
        33m
        59s
        200ms
    Finally the caller can 'overflow' the component's natural modulo if it is convenient:
        48h
        600m
    
    NOTE: The maximum valid number of digits in a time component is 10, i.e.: 1234567890s
    
\param str The input string.
\return true upon success or false upon error.
*******************************************************************************/
bool TimeValue::parseTimeInterval(const char8_t* str)
{
    uint32_t val, years, days, hours, minutes, seconds, milliseconds;
    const uint32_t max = UINT32_MAX / 10;
    const uint32_t rem = UINT32_MAX % 10;
    val = years = days = hours = minutes = seconds = milliseconds = 0;
    bool negative = false;
    if (*str == '-')
    {
        negative = true;
        ++str;
    }
    char8_t ch;
    for (;;)
    {
        ch = *str++;
        if ((ch >= '0') && (ch <= '9'))
        {
            ch -= '0';
            if ((val < max) || ((val == max) && ((uint32_t)ch <= rem)))
            {
                val = (val * 10) + ch;
                continue;
            }
            // overflow detected
            return false;
        }
        
        switch(ch)
        {
        case 'y':
            years = val;
            break;
        case 'd':
            days = val;
            break;
        case 'h':
            hours = val;
            break;
        case 'm':
            if(*str == 's')
            {
                ++str; // consume 's'
                milliseconds = val;
            }
            else
                minutes = val;
            break;
        case 's':
            seconds = val;
            break;
        default:
            return false; // encountered unhandled token, or early '\0'
        }
        
        if (*str == '\0')
            break;
        
        val = 0; // reset the value
        
        if(*str == ':')
            ++str; // consume ':'
    }
    
    mTime = ((((((years*365LL) + days)*24LL + hours)*60LL + minutes)*60LL + seconds)*1000LL + milliseconds)*1000LL;
    if (negative)
        mTime *= -1;
    
    return true;
}


/*! ***************************************************************************/
/*! \brief Attempts to parse date/time string using each of the supported formats.

    \param str The input string.
    \return true upon success or false upon error.
*******************************************************************************/
bool TimeValue::parseTimeAllFormats(const char8_t* str)
{
    if (str == nullptr || *str == '\0')
    {
        mTime = 0;      // Treat an empty or missing string like a clear call.
        return true;
    }

    if (parseTimeInterval(str))
        return true;
    if (parseGmDateTime(str))
        return true;
    if (parseLocalDateTime(str))
        return true;

    uint32_t year, month, day, hour, minute, second;
    if (parseAccountTime(str, year, month, day, hour, minute, second) > 2)  // "2014" would not be considered valid, but "2014-12-1" would be
    {
        mTime = TimeValue::getEpochFromGmTimeComponents(year, month, day, hour, minute, second);
        return true;
    }

    return false;
}

/*! ***************************************************************************/
/*! \brief Parses date/time string in format like: "2008-09-28T16:56Z", or "2008-09-28T16:56:01Z"
    \param str The input string.
    \return Epoch time.
*******************************************************************************/
TimeValue TimeValue::parseAccountTime(const char8_t* str)
{
    uint32_t year, month, day, hour, minute, second;
    parseAccountTime(str, year, month, day, hour, minute, second);
    return TimeValue(year, month, day, hour, minute, second);
}

int32_t TimeValue::parseAccountTime(const char8_t* str, uint32_t &year, uint32_t &month, uint32_t &day, uint32_t &hour, uint32_t &minute, uint32_t &second)
{
    year = month = day = hour = minute = second = 0;
    int32_t numUnits = sscanf(str, "%u-%02u-%02uT%u:%u:%uZ", &year, &month, &day, &hour, &minute, &second);
    if ((month <= 0) || (month > 12))
    {
        month = day = hour = minute = second = 0;
        numUnits = 1;
    }
    else if ((day <= 0) || (day > 31))
    {
        day = hour = minute = second = 0;
        numUnits = 2;
    }
    else if (hour > 23)
    {
        hour = minute = second = 0;
        numUnits = 3;
    }
    else if (minute > 59)
    {
        minute = second = 0;
        numUnits = 4;
    }
    else if (second > 60)//60 for leap seconds
    {
        second = 0;
        numUnits = 5;
    }
    return numUnits;
}


/*! ***************************************************************************/
/*! \brief Returns a TimeValue containing the time in GMT

\return TimeValue.
*******************************************************************************/
TimeValue TimeValue::getTimeOfDay() 
{
#if defined(EA_PLATFORM_WINDOWS) && (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    return TimeValue(ft);
#elif defined(EA_PLATFORM_LINUX)
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    ::gettimeofday(&tv, 0);
    return TimeValue(tv);
#else
    timeval tv;
    EA::StdC::GetTimeOfDay(&tv, NULL);
    return TimeValue(tv);
#endif
}

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_LINUX)
TimeValue TimeValue::getThreadCpuTime() 
{
#if defined(EA_PLATFORM_WINDOWS)
    //Note:  Even though this is supposed to be CPU Time, if you lookup QueryPerformanceCounter, you'll 
    //quickly realize this isn't.  The discrepency here comes from the fact that there is _no_ high
    //performance thread counter on Windows.  You can get thread timings, but those are limited to the quanta
    //of the OS timer (10-15ms), which doesn't help with microsecond scale timing.  So take these timings
    //with a very large grain of salt, as they incorporate thread sleeps, swaps, etc on a busy machine.
    LARGE_INTEGER time;
    ::QueryPerformanceCounter(&time);
    return TimeValue((int64_t)(time.QuadPart / msFreqsPerMicroSecond));
#elif defined(EA_PLATFORM_LINUX)
    timespec time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    return TimeValue((time.tv_sec * 1000000) + (time.tv_nsec / 1000));
   
#else
    return TimeValue(0);
#endif
}

#endif
// static
void TimeValue::getLocalTimeComponents(const TimeValue& tv,
        uint32_t* year, uint32_t* month, uint32_t* day, uint32_t* hour, uint32_t* minute,
        uint32_t* second, uint32_t* millis)
{
    struct timeval tval;
    tval.tv_sec = (long)(tv.mTime / 1000000);
    tval.tv_usec = (long)(tv.mTime % 1000000);
    struct tm time;
    localTime(&time, tval.tv_sec);

    if(year)
        *year = (uint32_t)(1900 + time.tm_year);
    if(month)
        *month = (uint32_t)(time.tm_mon + 1);
    if(day)
        *day = (uint32_t)(time.tm_mday);
    if(hour)
        *hour = (uint32_t)(time.tm_hour);
    if(minute)
        *minute = (uint32_t)(time.tm_min);
    if(second)
        *second = (uint32_t)(time.tm_sec);
    if(millis)
        *millis = (uint32_t)(tval.tv_usec / 1000);
}

// static
void TimeValue::getGmTimeComponents(const TimeValue& tv,
        uint32_t* year, uint32_t* month, uint32_t* day, uint32_t* hour, uint32_t* minute,
        uint32_t* second, uint32_t* millis)
{
    struct timeval tval;
    tval.tv_sec = (long)(tv.mTime / 1000000);
    tval.tv_usec = (long)(tv.mTime % 1000000);
    struct tm time;
    gmTime(&time, tval.tv_sec);

    if(year)
        *year = (uint32_t)(1900 + time.tm_year);
    if(month)
        *month = (uint32_t)(time.tm_mon + 1);
    if(day)
        *day = (uint32_t)(time.tm_mday);
    if(hour)
        *hour = (uint32_t)(time.tm_hour);
    if(minute)
        *minute = (uint32_t)(time.tm_min);
    if(second)
        *second = (uint32_t)(time.tm_sec);
    if(millis)
        *millis = (uint32_t)(tval.tv_usec / 1000);
}

//static 
int64_t TimeValue::getEpochFromGmTimeComponents(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, 
        uint32_t minute, uint32_t second)
{
    struct tm time;
    time.tm_year = (int)(year - 1900);
    time.tm_mon = (int)(month - 1);
    time.tm_mday = (int)(day);
    time.tm_hour = (int)(hour);
    time.tm_min = (int)(minute);
    time.tm_sec = (int)(second);

    // These fields are set to avoid memory checking tool warnings.  According to the manpage,
    // wday and yday have no effect in mktime and are overridden.  Setting isdst
    // to -1 means information is not available, so in theory it should not affect
    // any computations done in mktime.
    time.tm_wday = 0;
    time.tm_yday = 0;
    time.tm_isdst = -1;

    time_t result = mkgmTime(&time) * 1000 * 1000; // change to microseconds
    
    return result;
}

//static 
int64_t TimeValue::getEpochFromLocalTimeComponents(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, 
        uint32_t minute, uint32_t second)
{
    struct tm time;
    time.tm_year = (int)(year - 1900);
    time.tm_mon = (int)(month - 1);
    time.tm_mday = (int)(day);
    time.tm_hour = (int)(hour);
    time.tm_min = (int)(minute);
    time.tm_sec = (int)(second);

    // These fields are set to avoid memory checking tool warnings.  According to the manpage,
    // wday and yday have no effect in mktime and are overridden.  Setting isdst
    // to -1 means information is not available, so in theory it should not affect
    // any computations done in mktime.
    time.tm_wday = 0;
    time.tm_yday = 0;
    time.tm_isdst = -1;

    time_t result = mktime(&time) * 1000 * 1000; // change to microseconds
    
    return result;
}

void TimeValue::localTime(struct tm *tM, time_t t)
{
#ifdef EA_PLATFORM_WINDOWS
    _localtime64_s(tM, &t);
#elif EA_PLATFORM_LINUX
    localtime_r(&t, tM);
#else
    *tM = *localtime(&t);
#endif
}

/*************************************************************************************************/
/*!
    \brief gmTime 
        Converts epoch time into GMT components
    \param[in]  - t - 64 bit epoch time
    \param[out]  - tM - pointer to tm structure
    \return - none
*/
/*************************************************************************************************/
void TimeValue::gmTime(struct tm *tM, time_t t)
{
    
#ifdef EA_PLATFORM_WINDOWS
    _gmtime64_s(tM, &t);
#elif EA_PLATFORM_LINUX
    gmtime_r(&t, tM);
#else
    *tM = *gmtime(&t);
#endif
} 

/*************************************************************************************************/
/*!
    \brief mkgmTime 
        Converts GMT components into epoch time
    \param[in]  - tM - pointer to tm structure
    \return - 64 bit epoch time
*/
/*************************************************************************************************/
int64_t TimeValue::mkgmTime(struct tm *tM)
{
    tM->tm_wday = 0;
    tM->tm_yday = 0;
    tM->tm_isdst = 0;

    int64_t t = mktime(tM);
#ifdef EA_PLATFORM_WINDOWS
    t -= _timezone;
#elif EA_PLATFORM_LINUX
    t += tM->tm_gmtoff;
#else
    t += EA::StdC::GetTimeZoneBias();
#endif

    return t;
}

int TimeValue::getLocalTimeString(char8_t* buffer, size_t len)
{
    int printtimeoffset = 0;

    timeval tv;
    EA::StdC::GetTimeOfDay(&tv, NULL, true);
    char tsbuf[25];
    tm tmResult;
    localTime(&tmResult, tv.tv_sec);
    EA::StdC::Strftime(tsbuf, sizeof(tsbuf), sDateTimeFormats[0], &tmResult);
    printtimeoffset = EA::StdC::Snprintf(buffer, len, "%s.%03ld ", tsbuf, tv.tv_usec/1000);

    return printtimeoffset;
}

} //namespace TDF
} //namespace EA


