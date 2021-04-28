/*************************************************************************************************/
/*!
    \file   luastats.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luastats.h#2 $
    $Change: 44766 $
    $DateTime: 2009/06/04 15:07:27 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    luastats.h

    Reporting and error functions for the generated luafunctions.cpp

*/
/*************************************************************************************************/

#ifndef _LUASTATS_H
#define _LUASTATS_H

#include <eathread/eathread_futex.h>

namespace Blaze
{
namespace Stress
{
namespace Lua
{

// 
/*************************************************************************************************/
/*!
    \class LuaStats

     LuaStats is a singleton with the purpose of logging timing and error information
     for all running lua fibers.  LuaStats is designed to be thread safe and collects
     errors and times for all lua RPC calls regardless of the OS thread (typically 4?)
     that they may be running on.

     Report formats:

     LuaStats automatically generates reports every n rpcs called (combined over all threads/fibers)
     where n is set by SetWriteStatsFilesFrequency(..).  The default is currently 500.

     The statistics file comes out in the following .csv format -

     TEST FILE, TEST START TIME, RPC NAME, AVERAGE TIME/CALL (ms), MAXIMUM TIME/CALL (ms), MINIMUM TIME/CALL (ms), TOTAL TIME (ms), CALLS, ERRORS
     get-stats.lua, 2009/06/03 16:13:40, GetAthleteStatsByGame, 894.50, 906.26, 890.63, 3578, 4, 0
     get-stats.lua, 2009/06/03 16:13:40, GetDisplayRoster, 991.00, 1031.26, 984.38, 6937, 7, 0
     get-stats.lua, 2009/06/03 16:13:40, GetDisplayRosterByAthlete, 1171.67, 1203.13, 1156.26, 3515, 3, 0
     ..

     And the error file is -

     TEST FILE, TEST START TIME, RPC NAME, ERROR NAME, COUNT, TOTAL CALLS, PERCENT
     get-stats.lua, 2009/06/03 16:13:40, GetAthleteStatsByGame, FRANCHISE_ERR_DATABASE_ERROR (1640449), 4, 4, 100.00
     get-stats.lua, 2009/06/03 16:13:40, GetTeamStatsByGame, FRANCHISE_ERR_DATABASE_ERROR (1640449), 5, 5, 100.00
     ..

*/
/*************************************************************************************************/
class LuaStats
{
public:
    
    //! SetLuaFile(), SetStartTime(), SetStatsFilename() and SetErrorsFilename()
    /// should be called at initialization time to set the script filename, the start time,
    /// the report stats and the errors stats filenames for the generated reports
    void             SetLuaFile(const char8_t *file);
    const char      *GetLuaFile() const;
    void             SetStartTime(const TimeValue &time);
    const TimeValue &GetStartTime() const;
    const TimeValue &GetLatestRpcTime(const char8_t *rpc) const;
    void             SetStatsFilename(const char8_t *file);
    void             SetErrorsFilename(const char8_t *file);
    
    //! SetYieldAll() can be set in lua (with SetYieldAll(..)) and instructs the lua/cpp bindings
    /// to yield instead of returning a value at the end of execution.  Normally this will
    /// be set to true.
    void             SetYieldAll(bool yieldAll);
    bool             GetYieldAll() const;
    
    BlazeRpcError    GetLastRpcError() const;

    //! ReportError(), ReportTime() are called from within the lua/cpp binding
    /// to submit the rpc error and rpc execution time.  Not all bindings will
    /// submit errors, but they should all submit times.  Look at luafunctions.cpp
    /// for examples.  Note that ReportTime() will periodically
    /// (set by SetWriteStatsFilesFrequency() - see below) write out full stats and
    /// error reports.
    BlazeRpcError    ReportError(const eastl::string &rpc, BlazeRpcError error);
    void             ReportTime (const eastl::string &rpc, const TimeValue &start);

    //! SetWriteStatsFilesFrequency() is called to set how frequently reports are
    /// generated.  Currently, this defaults to every 500 rpcs (on all threads/fibers combined)
    /// but can be set to an arbitrary number of RPCs here.
    void             SetWriteStatsFilesFrequency(int32_t frequency);
    
    static LuaStats *GetInstance();
   
private:

    struct Error
    {
        eastl::string  rpc;
        BlazeRpcError  id;
        int32_t        count;
    
        inline Error() : id(ERR_OK), count(0)  {}
    };
    
    struct Stats
    {
        TimeValue total,
                  maximum,
                  minimum,
                  latestRpcTime;
        int32_t   invocations;
        int32_t   errors;
        
        inline Stats() : total(0), maximum(0), minimum(0x7fffffff), invocations(0), errors(0) {}
        
        void AddTime(const TimeValue &time);
    };
        
    bool WriteStatsToFile(const char8_t *filename);
    bool WriteErrorsToFile(const char8_t *filename);    

    static void CreateTimeStampString(const TimeValue &time, char8_t *buffer, int32_t size);

    bool                              mYieldAll;
    BlazeRpcError                     mLastRpcError;
    int32_t                           mErrors;    
    int32_t                           mCalls;
    int32_t                           mStatsFilesFrequency;
    eastl::string                     mLuaFile,
                                      mStatsFilename,
                                      mErrorsFilename;
    TimeValue                         mRunStartTime;
    eastl::map<eastl::string, Stats>  mRpcStats;
    eastl::map<eastl::string, Error>  mRpcErrors;
    EA::Thread::Futex                 mLock;
        
    static LuaStats                   mInstance;

    // these are private because LuaStats is a singleton
    LuaStats();

    // these are not implemented
    LuaStats(const LuaStats &);
    LuaStats &operator=(const LuaStats &);
};

} // Lua
} // Stress
} // Blaze

#endif // _LUASTATS_H
