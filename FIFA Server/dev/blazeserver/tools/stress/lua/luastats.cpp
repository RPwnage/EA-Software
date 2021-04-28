/*************************************************************************************************/
/*!
    \file   luastats.cpp

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastats.cpp#2 $
    $Change: 44766 $
    $DateTime: 2009/06/04 15:07:27 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    luastats.cpp

    Reporting and error functions for the generated luafunctions.cpp

*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#include "luathread.h"
#include "blazerpcerrors.h"
#include "luastats.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


namespace Blaze
{
namespace Stress
{
namespace Lua
{

LuaStats LuaStats::mInstance;

LuaStats::LuaStats() : mYieldAll(true), mLastRpcError(ERR_OK), mErrors(0), mCalls(0), mStatsFilesFrequency(500)
{
}
    
void LuaStats::SetLuaFile(const char8_t *file)
{
    EA::Thread::AutoFutex lock(mLock);
    mLuaFile = file;
}

void LuaStats::SetStatsFilename(const char8_t *file)
{
    EA::Thread::AutoFutex lock(mLock);    
    mStatsFilename = file;
}

void LuaStats::SetErrorsFilename(const char8_t *file)
{
    EA::Thread::AutoFutex lock(mLock);    
    mErrorsFilename = file;
}

const char8_t *LuaStats::GetLuaFile() const
{
    return mLuaFile.c_str();
}

void LuaStats::SetWriteStatsFilesFrequency(int32_t frequency)
{
    EA::Thread::AutoFutex lock(mLock);
    mStatsFilesFrequency = frequency;
}
    
void LuaStats::SetStartTime(const TimeValue &time)
{
    EA::Thread::AutoFutex lock(mLock);
    mRunStartTime = time;
}

const TimeValue &LuaStats::GetStartTime() const
{
    return mRunStartTime;
}
    
const TimeValue &LuaStats::GetLatestRpcTime(const char8_t *rpc) const
{
    eastl::map<eastl::string, Stats>::const_iterator i = mRpcStats.find(rpc);
    const Stats &stats = (*i).second;
    return stats.latestRpcTime;
}

void LuaStats::SetYieldAll(bool yieldAll)
{
    mYieldAll = yieldAll;
}

bool LuaStats::GetYieldAll() const
{
    return mYieldAll;
}
    
BlazeRpcError LuaStats::GetLastRpcError() const
{
    return mLastRpcError;
}
    
LuaStats *LuaStats::GetInstance()
{
    return &mInstance;
}

//! Set the lastest executions time per RPC in the Stats structure
void LuaStats::Stats::AddTime(const TimeValue &time)
{
    invocations++;
    total += time;
    if (time > maximum)
    {
        maximum = time;
    }
    if (time < minimum)
    {
        minimum = time;
    }

    latestRpcTime = time;
}

//! ReportTime() is called from within each and every lua/cpp rpc binding
/// to submit the rpc execution time.  See luafunctions.cpp
/// for examples.  Note that ReportTime() will periodically
/// (set by SetWriteStatsFilesFrequency() - see below) write out full stats and
/// error reports.
void LuaStats::ReportTime(const eastl::string &rpc, const TimeValue &start)
{
    EA::Thread::AutoFutex lock(mLock);
    Stats &stats = mRpcStats[rpc];
    stats.AddTime(TimeValue::getTimeOfDay() - start);
    mCalls++;
    // do reporting
    if (mStatsFilesFrequency && (mCalls % mStatsFilesFrequency == 0))
    {
        if (!mStatsFilename.empty())
        {
            WriteStatsToFile(mStatsFilename.c_str());
        }
        if (!mErrorsFilename.empty())
        {
            WriteErrorsToFile(mErrorsFilename.c_str());
        }
    }
}

//! ReportError() is called from within the lua/cpp binding
/// to submit the rpc error.  Not all bindings will submit errors.
/// See luafunctions.cpp for examples.  
BlazeRpcError LuaStats::ReportError(const eastl::string &rpc, BlazeRpcError error)
{
    enum { MAX_ID_STR_SIZE = 128 };
    
    if (error != ERR_OK && !rpc.empty())
    {
        EA::Thread::AutoFutex lock(mLock);
        mLastRpcError = error;
        mErrors++;
        char8_t id[MAX_ID_STR_SIZE];
        blaze_snzprintf(id, sizeof(id), "%s#%d", rpc.c_str(), error);
        Error &err = mRpcErrors[id];
        err.count++;
        err.id  = error;
        err.rpc = rpc;
        Stats &stats = mRpcStats[rpc];
        stats.errors++;
    }
    return error;
}

//! CreateTimeStampString() is static and creates a string with a timestamp from a blaze TimeValue.
void LuaStats::CreateTimeStampString(const TimeValue &time, char8_t *buffer, int32_t size)
{
    uint32_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millis = 0;
    TimeValue::getLocalTimeComponents(time, &year, &month, &day, &hour, &minute, &second, &millis);
    blaze_snzprintf(buffer, size, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

//! WriteStatsToFile() writes the stats report out to a .csv file in the following format:
///
/// TEST FILE, TEST START TIME, RPC NAME, AVERAGE TIME/CALL (ms), MAXIMUM TIME/CALL (ms), MINIMUM TIME/CALL (ms), TOTAL TIME (ms), CALLS, ERRORS
/// get-stats.lua, 2009/06/03 16:13:40, GetAthleteStatsByGame, 894.50, 906.26, 890.63, 3578, 4, 0
/// get-stats.lua, 2009/06/03 16:13:40, GetDisplayRoster, 991.00, 1031.26, 984.38, 6937, 7, 0
/// get-stats.lua, 2009/06/03 16:13:40, GetDisplayRosterByAthlete, 1171.67, 1203.13, 1156.26, 3515, 3, 0
/// ..
bool LuaStats::WriteStatsToFile(const char8_t *filename)
{
    EA::Thread::AutoFutex lock(mLock);
    FILE *file = fopen(filename, "w");
    if (file == nullptr)
    {
        printf("Unable to open file '%s' for writing!\n", filename);
        return false;
    }
    fprintf(file, "TEST FILE, TEST START TIME, RPC NAME, AVERAGE TIME/CALL (ms), MAXIMUM TIME/CALL (ms), MINIMUM TIME/CALL (ms), TOTAL TIME (ms), CALLS, ERRORS\n");
    const float ONE_THOUSANDTH = 1.f / 1000.f;
    enum { TIME_STR_SIZE = 128 };
    char8_t timeStr[TIME_STR_SIZE];
    CreateTimeStampString(mRunStartTime, timeStr, sizeof(timeStr));
    for (eastl::map<eastl::string, Stats>::const_iterator i = mRpcStats.begin(), e = mRpcStats.end(); i != e; ++i)
    {
        const Stats &stats = (*i).second;
        if (stats.invocations > 0)
        {
            fprintf(file, "%s, %s, %s, %.2f, %.2f, %.2f, %u, %u, %u\n",
                          mLuaFile.c_str(),
                          timeStr,
                          (*i).first.c_str(),
                          static_cast<float>(stats.total.getMillis()) / static_cast<float>(stats.invocations),
                          stats.maximum.getMicroSeconds() * ONE_THOUSANDTH, stats.minimum.getMicroSeconds()* ONE_THOUSANDTH,
                          static_cast<int32_t>(stats.total.getMillis()), stats.invocations, stats.errors);
        }
    }
    fclose(file);
    return true;
}

//! WriteErrorsToFile() writes the error report out to a .csv file in the following format:
/// 
/// TEST FILE, TEST START TIME, RPC NAME, ERROR NAME, COUNT, TOTAL CALLS, PERCENT
/// get-stats.lua, 2009/06/03 16:13:40, GetAthleteStatsByGame, FRANCHISE_ERR_DATABASE_ERROR (1640449), 4, 4, 100.00
/// get-stats.lua, 2009/06/03 16:13:40, GetTeamStatsByGame, FRANCHISE_ERR_DATABASE_ERROR (1640449), 5, 5, 100.00
/// ..
bool LuaStats::WriteErrorsToFile(const char8_t *filename)
{
    EA::Thread::AutoFutex lock(mLock);    
    FILE *file = fopen(filename, "w");
    if (file == nullptr)
    {
        printf("Unable to open file '%s' for writing!\n", filename);
        return false;
    }
    fprintf(file, "TEST FILE, TEST START TIME, RPC NAME, ERROR NAME, COUNT, TOTAL CALLS, PERCENT\n");
    enum { TIME_STR_SIZE = 128 };
    char8_t timeStr[TIME_STR_SIZE];
    CreateTimeStampString(mRunStartTime, timeStr, sizeof(timeStr));
    for (eastl::map<eastl::string, Error>::const_iterator i = mRpcErrors.begin(), e = mRpcErrors.end(); i != e; ++i)
    {
        const Error &error = (*i).second;
        if (error.count > 0)
        {
            int32_t invocations = mRpcStats[error.rpc].invocations;
            if (!invocations)    // something's messed up here, 
            {
                invocations = 1; // but let's avoid division by zero anyway
            }
            fprintf(file, "%s, %s, %s, %s (%d), %u, %u, %.2f\n",
                          mLuaFile.c_str(),
                          timeStr,
                          error.rpc.c_str(),
                          ErrorHelp::getErrorName(error.id),
                          error.id,
                          error.count,
                          invocations,
                          error.count / (float)invocations * 100.f);
        }
    }
    fclose(file);
    return true;
}

} // Lua
} // Stress
} // Blaze
