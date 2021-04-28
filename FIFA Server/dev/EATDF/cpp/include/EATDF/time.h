/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TIME_H
#define EA_TDF_TIME_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <EATDF/internal/config.h>

/*** Include files *******************************************************************************/
#include <time.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

struct timeval;
struct _FILETIME;

namespace EA
{
namespace TDF
{
 

// Wrapper class over top of struct timeval
class EATDF_API TimeValue
{
public:

    enum TimeFormat
    {
        TIME_FORMAT_INTERVAL,
        TIME_FORMAT_GMDATE
    };

public:
    TimeValue() : mTime(0) { }
    TimeValue(int64_t microseconds) : mTime(microseconds) { }
    TimeValue(uint32_t year, uint32_t month, uint32_t day,
              uint32_t hour, uint32_t minute, uint32_t second);
    TimeValue(const char8_t *timeString, TimeFormat format);
    TimeValue(const timeval& tv);
#if defined(EA_PLATFORM_WINDOWS)
    TimeValue(const _FILETIME& tv);
#endif

    // Copy constructor
    TimeValue(const TimeValue& rhs) : mTime(rhs.mTime) { }
 
    int64_t getMicroSeconds() const { return mTime; }
    int64_t getMillis() const { return mTime / 1000; }
    int64_t getSec() const { return mTime / 1000000; }

    void setMicroSeconds(int64_t microseconds) { mTime = microseconds; }
    void setMillis(int64_t millis)             { mTime = millis * 1000; }
    void setSeconds(int64_t seconds)           { mTime = seconds * 1000000; }

    // Assignment operator
    TimeValue& operator=(const TimeValue& rhs)
    {
        mTime = rhs.mTime;
        return *this;
    }

    // Math and comparison operators
    TimeValue& operator+=(const TimeValue& rhs)
    {
        mTime += rhs.mTime;
        return *this;
    }

    TimeValue& operator-=(const TimeValue& rhs)
    {
        mTime -= rhs.mTime;
        return *this;
    }

    friend TimeValue operator+(const TimeValue& lhs, const TimeValue& rhs)
    {
        TimeValue temp(lhs);
        temp += rhs;
        return temp;
    }

    friend TimeValue operator-(const TimeValue& lhs, const TimeValue& rhs)
    {
        TimeValue temp(lhs);
        temp -= rhs;
        return temp;
    }

    friend int64_t operator/(const TimeValue& lhs, const TimeValue& rhs)
    {
        return lhs.mTime / rhs.mTime;
    }

    friend TimeValue operator*(const TimeValue& lhs, int64_t rhs)
    {
        return TimeValue(lhs.mTime * rhs);
    }

    friend TimeValue operator*(int64_t lhs, const TimeValue& rhs)
    {
        return TimeValue(rhs.mTime * lhs);
    }

    bool operator<(const TimeValue& rhs) const
    {
        return (mTime < rhs.mTime);
    }

    bool operator==(const TimeValue& rhs) const
    {
        return (mTime == rhs.mTime);
    }

    friend bool operator>(const TimeValue& lhs, const TimeValue& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator!=(const TimeValue& lhs, const TimeValue& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<=(const TimeValue& lhs, const TimeValue& rhs)
    {
        return !(rhs < lhs);
    }

    friend bool operator>=(const TimeValue& lhs, const TimeValue& rhs)
    {
        return !(lhs < rhs);
    }

    // Uses either toIntervalString or toString depending on how big the value is.  (<946684800000000 (New Years 2000) (30 years) uses interval format)
    const char8_t* toStringBasedOnValue(char8_t* buffer, size_t len) const;

    const char8_t* toString(char8_t* buffer, size_t len) const;

    const char8_t* toDateFormat(char8_t* buffer, size_t len) const;

    const char8_t* toAccountString(char8_t* buffer, size_t len, bool includeSeconds = false) const;

    const char8_t* toIntervalString(char8_t* buffer, size_t len) const;

    // Supports formats of: parseGmTime, parseLocalTime, parseTimeInterval and parseAccountTime.
    bool parseTimeAllFormats(const char8_t* str);

    // Expected format: 16:56:08, prepends current date, returns true if success
    bool parseGmTime(const char8_t* str);
    bool parseLocalTime(const char8_t* str);

    // Expected format: 2008/09/28-16:56:08, returns true if success
    bool parseGmDateTime(const char8_t* str);
    bool parseLocalDateTime(const char8_t* str);
    
    // Expected format: 300d:17h:33m:59s:200ms, returns true if success
    bool parseTimeInterval(const char8_t* str);

    // Expected format: 2008-09-28T16:56Z, or, 2008-09-28T16:56:01Z. returns 0 if fail
    static TimeValue parseAccountTime(const char8_t* str);
    static int32_t parseAccountTime(const char8_t* str, uint32_t &year, uint32_t &month, uint32_t &day, uint32_t &hour, uint32_t &minute, uint32_t &second);
    
    // Get current time
    static TimeValue getTimeOfDay();
    
    // Get current thread CPU time
#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_LINUX)
    static TimeValue getThreadCpuTime();
#endif
    // Break a TimeValue into date/time components
    // The time components contain the values appropriate for display in the Local time
    static void getLocalTimeComponents(const TimeValue& tv,
            uint32_t* year=0, uint32_t* month=0, uint32_t* day=0, uint32_t* hour=0, uint32_t* minute=0,
            uint32_t* second=0, uint32_t* millis=0);

    // Break a TimeValue into date/time components
    // The time components contain the values appropriate for display as GMT time
    static void getGmTimeComponents(const TimeValue& tv,
            uint32_t* year=0, uint32_t* month=0, uint32_t* day=0, uint32_t* hour=0, uint32_t* minute=0,
            uint32_t* second=0, uint32_t* millis=0);
    
    // Converts epoch time into local time components
    static void localTime(struct tm *tM, time_t t);

    // Converts epoch time into GMT components
    static void gmTime(struct tm *tM, time_t t);

    // Converts GMT components into epoch time
    static int64_t mkgmTime(struct tm *tM);

    // Get the current system time as a string
    static int getLocalTimeString(char8_t* buffer, size_t len);


private:
    // Get epoch time given date/time components
    static int64_t getEpochFromGmTimeComponents(uint32_t year, uint32_t month, uint32_t day,
        uint32_t hour, uint32_t minute, uint32_t second);
    static int64_t getEpochFromLocalTimeComponents(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, 
        uint32_t minute, uint32_t second);

    bool parseDateTime(const char8_t* str, const char8_t* formats[], size_t numFormats, bool inGmt, bool prependsCurrentDateTime);

    int64_t mTime; // Time in microseconds
    
#ifdef EA_PLATFORM_WINDOWS
    static int64_t getWindowsHighFrequencyCounter();
    static double msFreqsPerMicroSecond;
#endif
    static const char8_t* sDateTimeFormats[];
    static const char8_t* sTimeFormats[];
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_TIME_H


