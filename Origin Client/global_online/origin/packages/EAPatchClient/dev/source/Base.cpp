///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Base.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EADateTime.h>
#include <EAStdC/EAScanf.h>
#include <errno.h>


namespace EA
{
namespace Patch
{

///////////////////////////////////////////////////////////////////////////////
// AsyncStatus
///////////////////////////////////////////////////////////////////////////////

const char8_t* kAsyncStatusStateTable[kNumAsyncStatus] = 
{
    "none",
    "started",
    "cancelling",
    "completed",
    "deferred",
    "cancelled",
    "aborted",
    "crashed"
};

const char8_t* GetAsyncStatusString(AsyncStatus status)
{
    EA_COMPILETIME_ASSERT(kAsyncStatusNone == 0);

    if((status < kAsyncStatusNone) || (status >= kNumAsyncStatus))
        status = kAsyncStatusNone;

    return kAsyncStatusStateTable[status];
}

///////////////////////////////////////////////////////////////////////////////
// Time functions
///////////////////////////////////////////////////////////////////////////////

EAPATCHCLIENT_API uint64_t GetTimeSeconds()
{
    return EA::StdC::GetTime() / UINT64_C(1000000000);
}

EAPATCHCLIENT_API uint64_t GetTimeMilliseconds()
{
    return EA::StdC::GetTime() / UINT64_C(1000000);
}

EAPATCHCLIENT_API uint64_t GetTimeMicroseconds()
{
    return EA::StdC::GetTime() / UINT64_C(1000);
}

EAPATCHCLIENT_API uint64_t GetTimeNanoseconds()
{
    return EA::StdC::GetTime();
}


/////////////////////////////////////////////////////////////////////////////
// Utility functions
/////////////////////////////////////////////////////////////////////////////

EAPATCHCLIENT_API bool StringToInt64(const char8_t* p, int64_t& i64, int base)
{
    i64 = EA::StdC::StrtoI64(p, NULL, base);

    if(((i64 == INT64_MIN) || (i64 == INT64_MAX)) && (errno == ERANGE))
    {
        i64 = 0;
        return false;
    }

    return true;
}

EAPATCHCLIENT_API int64_t StringToInt64(const char8_t* p, int base)
{
    int64_t i64;
    StringToInt64(p, i64, base);
    return i64;
}


EAPATCHCLIENT_API bool StringToUint64(const char8_t* p, uint64_t& u64, int base)
{
    u64 = EA::StdC::StrtoU64(p, NULL, base);

    if((u64 == UINT64_MAX) && (errno == ERANGE))
    {
        u64 = 0;
        return false;
    }

    return true;
}

EAPATCHCLIENT_API uint64_t StringToUint64(const char8_t* p, int base)
{
    uint64_t u64;
    StringToUint64(p, u64, base);
    return u64;
}




EAPATCHCLIENT_API bool StringToDouble(const char8_t* p, double& d)
{
    d = EA::StdC::Strtod(p, NULL);

    if((fabs(d) == HUGE_VAL) && (errno == ERANGE))
    {
        d = 0;
        return false;
    }

    return true;
}

EAPATCHCLIENT_API double StringToDouble(const char8_t* p)
{
    double d;
    StringToDouble(p, d);
    return d;
}



EAPATCHCLIENT_API bool DateStringToTimeT(const char8_t* pDateString, time_t& t)
{
    uint32_t Y, M, D, h, m, s;
    int     fieldCount = EA::StdC::Sscanf(pDateString, "%04u-%02u-%02u %02u:%02u:%02u", &Y, &M, &D, &h, &m, &s);

    if(fieldCount == 6) // If all were read...
    {
        EA::StdC::DateTime d(Y, M, D, h, m, s);
        t = EA::StdC::DateTimeSecondsToTimeTSeconds(d.GetSeconds());
        return true;
    }

    t = 0;
    return false;
}


EAPATCHCLIENT_API bool TimeTToDateString(const time_t& t, char8_t* pDateString)
{
    EA::StdC::DateTime d(EA::StdC::TimeTSecondsSecondsToDateTime(t));
    String sTemp;

    // "YYYY-MM-DD hh:mm:ss GMT"
    sTemp.sprintf("%04d-%02d-%02d %02d:%02d:%02d GMT",
                   d.GetParameter(EA::StdC::kParameterYear),
                   d.GetParameter(EA::StdC::kParameterMonth),
                   d.GetParameter(EA::StdC::kParameterDayOfMonth),
                   d.GetParameter(EA::StdC::kParameterHour),
                   d.GetParameter(EA::StdC::kParameterMinute),
                   d.GetParameter(EA::StdC::kParameterSecond));

    EA::StdC::Strlcpy(pDateString, sTemp.c_str(), kMinDateStringCapacity);
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// ClientLimits
///////////////////////////////////////////////////////////////////////////////

ClientLimits::ClientLimits()
{
    mDiskUsageLimit        = UINT64_MAX; 
    mMemoryUsageLimit      = 1000000; 
    mFileLimit             = 8; 
    mThreadCountLimit      = 2; 
    mSocketCountLimit      = 8; 
    mCPUCountLimit         = 1; 
    mCPUUsageLimit         = 0; 
    mNetworkReadLimit      = 0; 
    mNetworkRetrySeconds   = 60;
    mHTTPMemorySize        = 1024 * 1024;
    #if defined(EA_PLATFORM_DESKTOP)
    mHTTPConcurrentLimit   = 8;
    #else
    mHTTPConcurrentLimit   = 1;
    #endif
    mHTTPTimeoutSeconds    = 30; 
    mHTTPRetryCount        = 256;           // To do: Decide the best value for this, which depends on how we end up using it in practice.
    mHTTPPipeliningEnabled = false;

    #if (EAPATCH_DEBUG >= 2)
        mHTTPTimeoutSeconds = 7200;
    #endif
}


ClientLimits gClientLimits;

EAPATCHCLIENT_API ClientLimits& GetDefaultClientLimits()
{
    return gClientLimits;
}


EAPATCHCLIENT_API void SetDefaultClientLimits(const ClientLimits& clientLimits)
{
    gClientLimits = clientLimits;
}



///////////////////////////////////////////////////////////////////////////////
// DataRateMeasure
///////////////////////////////////////////////////////////////////////////////

DataRateMeasure::DataRateMeasure()
  : mStopwatch(EA::StdC::Stopwatch::kUnitsNanoseconds), mDataSizeSum(0)
{
}

void DataRateMeasure::Start()
{
    // There's no harm if the stopwatch is already Started.
    mStopwatch.Start();
}


bool DataRateMeasure::IsStarted() const
{
    return mStopwatch.IsRunning();
}

void DataRateMeasure::Stop()
{
    // There's no harm if the stopwatch is already Stopped.
    mStopwatch.Stop();
}

void DataRateMeasure::Reset()
{
    mStopwatch.Reset();
    mDataSizeSum = 0;
}

void DataRateMeasure::ProcessData(uint64_t dataSize)
{
    EAPATCH_ASSERT(mStopwatch.IsRunning());
    mDataSizeSum += dataSize;
}

uint64_t DataRateMeasure::GetDataSize() const
{
    return mDataSizeSum;
}

double DataRateMeasure::GetDataRate() const
{
    // Our stopwatch measures in nanoseconds, but we report in milliseconds.
    const float    fElapsedTimeNs = mStopwatch.GetElapsedTimeFloat();
    const uint64_t kNsToMs        = 1000000;

    if(fElapsedTimeNs == 0)
        return 0.0;

    return (double)(mDataSizeSum * kNsToMs) / fElapsedTimeNs;
}


} // namespace Patch
} // namespace EA








