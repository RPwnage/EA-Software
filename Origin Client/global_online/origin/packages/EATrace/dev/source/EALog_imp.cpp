/////////////////////////////////////////////////////////////////////////////
// EALog_imp.cpp
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// Defines an implementation of the Log interface.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// To consider: Implement a scriptable uber-filter.
//              Add support for wildcard filters on groups, filename, and function (e.g. namespace), 
//              just look at incoming string format to determine which wildcard type it is
//              those filtering results should be &&'ed by the uber filter which handles the configuration.
//
// To consider: Add a mutex protected buffer reporter that accumulates messages. 
//              Use that with UI which can only display in the message pump thread.
//
// To consider: Implement a scriptable formatter (see GZLogService.h), has per level 
//              configuration of printf like tokens.
//
// To consider: Have reporter to buffer data (into 32K). Accumulates its filtered strings, 
//              and then UI can grab that buffer.  Have the buffer specify how many records 
//              it accums (title bar only wants one record), vs. a scrollpane console which 
//              can display more info.
/////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#include <EATrace/internal/CoreAllocatorNew.h>
#include <EATrace/Allocator.h>
#include <EATrace/EALog_imp.h>
#include <EATrace/EATrace_imp.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAStdC/EASprintf.h>
#include <stdarg.h> // for varargs

#if defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_OSX)   // other platforms not tested.
    #include <time.h>   // ANSI C time functions for LogFormatterFancy
    #define LOG_FORMATTER_FANCY_TIME
#endif

#if defined(EA_PLATFORM_PLAYSTATION2)
    #include <eekernel.h>
#elif defined(EA_PLATFORM_PS3)
    #include <sys/time_util.h>
    #include <sys/sys_time.h>
    #ifndef SYS_TIMEBASE_GET // If using older Sony SDK...
        #define SYS_TIMEBASE_GET TIMEBASE_GET
    #endif
#elif defined(EA_PLATFORM_PS3_SPU)
    #include <time.h>
#elif defined(EA_PLATFORM_REVOLUTION)
    #include <revolution/os.h>
    #include <stdarg.h>
#elif defined(EA_PLATFORM_GAMECUBE)
    #include <dolphin/os.h>
    #include <stdarg.h>
#elif defined(EA_PLATFORM_XENON)
    #pragma warning(push, 1)
    #pragma warning(disable: 4311 4312)
    #include <XTL.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_DS)
	#include <nitro/os.h>
#elif defined(EA_PLATFORM_PSP2)
    #include <rtc.h>
#else
    #include <time.h>
#endif


#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 6011) // Dereferencing NULL pointer.  (VC++ /analysis is buggy)
#endif


namespace EA
{
namespace Trace
{


static inline uint64_t GetCurrentTimeMs()
{
    #if defined(EA_PLATFORM_MICROSOFT)
        // GetTickCount() about 23 days worth of milliseconds. However, if the computer
        // has been running for 22 days, then the return value will turn over within 
        // only 24 hours. This may be a small problem for our uses, but not deadly, 
        // as this is debug code only.
        // return (unsigned)(GetTickCount() / 1000); 

        uint64_t nTime;
        GetSystemTimeAsFileTime((FILETIME*)&nTime);
        nTime /= 10000; // Convert from 100ns to 1 ms.
        return nTime;

    #elif defined(EA_PLATFORM_PLAYSTATION2)
        const uint64_t nBusClock64 = (uint64_t)GetTimerSystemTime();
        return (uint64_t)(nBusClock64 * 1000 / 147456000);

    #elif defined(EA_PLATFORM_PS3)
        uint64_t nTimeBase;
        SYS_TIMEBASE_GET(nTimeBase);
        return (uint64_t)(nTimeBase / (sys_time_get_timebase_frequency() / 1000));

    #elif defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_REVOLUTION)
        return (uint64_t)((uint64_t)OSGetTime() * 1000 / OS_TIMER_CLOCK);

    #elif defined(EA_PLATFORM_PSP2)
        SceRtcTick ticks;
        sceRtcGetCurrentTick(&ticks); //returns ticks with microsecond precision
        return (ticks.tick / 1000);

    #else
        return (uint64_t)(clock() * 1000 / CLOCKS_PER_SEC);
    #endif
}



LogFormatterSimple::LogFormatterSimple(Allocator::ICoreAllocator* pAllocator)
  : Trace::ZoneObject()
  , mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator())
  , mnRefCount(0)
  , mName()
  , mLine()
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterSimple");
        mLine.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterSimple");
    #endif
}


LogFormatterSimple::LogFormatterSimple(const char* name, Allocator::ICoreAllocator* pAllocator)
  : mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator())
  , mnRefCount(0) 
  , mName(name)
  , mLine()
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterSimple");
        mLine.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterSimple");
    #endif
}


LogFormatterSimple::~LogFormatterSimple()
{
    // Empty
}


const char* LogFormatterSimple::FormatRecord(const LogRecord& inRecord)
{
    // A super simple implementation:
    // return inRecord.GetMessageText();

    // Example of a slightly better formatter, still hardcoded instead of scriptable
    mLine.assign(inRecord.GetMessageText());
    const eastl_size_t lineLength = mLine.length();

    if(!lineLength || (mLine[lineLength - 1] != '\n'))
        mLine += '\n'; 
    
    // Default formatter outputs the file/line number if level warrants it
    // this must be first character on line to support hypertext VC++ feature.
    const TraceHelper* const pTraceHelper = inRecord.GetTraceHelper();

    if(pTraceHelper->GetLevel() >= kLevelWarn)
    {
        const ReportingLocation* const pLoc = pTraceHelper->GetReportingLocation();

        mLine.append_sprintf("%s(%d): %s\n", pLoc->GetFilename(), pLoc->GetLine(), pLoc->GetFunction());
    }

    return mLine.c_str();
}


void LogFormatterSimple::SetName(const char* name)
{
    mName = name;
}


const char* LogFormatterSimple::GetName() const
{
    return mName.c_str();
}


// After calling Clone, the caller must call SetName() with a unique name.
ILogFormatter* LogFormatterSimple::Clone()
{
    LogFormatterSimple* const pFormatter = new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "LogFormatterSimple")
        LogFormatterSimple(mName.c_str(), mpCoreAllocator);
    return pFormatter;
}


void* LogFormatterSimple::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID:
            return static_cast<IUnknown32*>(this);

        case ILogFormatter::kIID:
            return static_cast<ILogFormatter*>(this);

        case LogFormatterSimple::kIID:
            return static_cast<LogFormatterSimple*>(this);
    }

    return NULL; 
}


int LogFormatterSimple::AddRef()
{
    return ++mnRefCount;
}


int LogFormatterSimple::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    mnRefCount = 0; // This is useful for users with debugger, but has no benefit to the running code itself.
    delete this;    // LogFormatterSimple derives from ZoneObject and has overloaded 'operator delete'
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// LogFormatterPrefixed
///////////////////////////////////////////////////////////////////////////////

LogFormatterPrefixed::LogFormatterPrefixed(const char* pName, const char8_t* pLineEnd)
  : LogFormatterSimple(pName),
    mLine(),
    mpLineEnd(pLineEnd)
{
}

/// LogFormatterPrefixed
const char* LogFormatterPrefixed::FormatRecord(const EA::Trace::LogRecord &inRecord)
{
    // Build the formatted string representing this log record.
    const EA::Trace::TraceHelper * const pTraceHelper = inRecord.GetTraceHelper();
    mLine.sprintf("[%s] %s", pTraceHelper->GetGroupName(), inRecord.GetMessageText());

    // Strip off any trailing newlines the caller may have appended.
    if (mLine.back() == '\n')
    {
        mLine.pop_back();
        #if defined(EA_PLATFORM_WINDOWS)    // on Windows it might be necessary to strip-off CR
            if (mLine.back() == '\r')
                mLine.pop_back();
        #endif
    }

    // Now optionally append our configured line ending sequence.
    if (mpLineEnd)
        mLine += mpLineEnd;

    return mLine.c_str();
}


///////////////////////////////////////////////////////////////////////////////
// LogFormatterFancy
///////////////////////////////////////////////////////////////////////////////

LogFormatterFancy::LogFormatterFancy(Allocator::ICoreAllocator* pAllocator)
  : Trace::ZoneObject(),
    mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator()),
    mnRefCount(0),
    mName(),
    mnFlags(0),
    mnFileInfoLevel(kLevelWarn),
    mInitialTime(GetCurrentTimeMs()),
    mLine()
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterFancy");
        mLine.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterFancy");
    #endif
}


LogFormatterFancy::LogFormatterFancy(const char* name, Allocator::ICoreAllocator* pAllocator)
  : mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator()),
    mnRefCount(0),
    mName(name),
    mnFlags(0),
    mnFileInfoLevel(kLevelWarn),
    mInitialTime(GetCurrentTimeMs()),
    mLine()
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterFancy");
        mLine.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFormatterFancy");
    #endif
}


LogFormatterFancy::~LogFormatterFancy()
{
    // Empty
}


const char* LogFormatterFancy::FormatRecord(const LogRecord& inRecord)
{
    const TraceHelper* const helper = inRecord.GetTraceHelper();
    const char8_t*     const text   = inRecord.GetMessageText();

    if(!mnFlags && (helper->GetLevel() < mnFileInfoLevel))
        return text;

    mLine.clear();

    #ifdef LOG_FORMATTER_FANCY_TIME
        if(mnFlags & (kFlagTimeStamp | kFlagDeltaTimeStamp))
        {
            uint64_t  elapsedMs      = GetCurrentTimeMs() - mInitialTime;
            time_t    timeVal        = time_t((time_t)(elapsedMs / 1000));
            tm* const timeComponents = gmtime(&timeVal); 
            char      buffer[32];

            if(mnFlags & kFlagIncludeMs)
            {
                char* const pEnd = buffer + strftime(buffer, 32, "%H:%M:%S", timeComponents);
                sprintf(pEnd, ":%03llu ", elapsedMs % 1000);
            }
            else
                strftime(buffer, 32, "%H:%M:%S ", timeComponents);

            mLine.append(buffer);

            if(mnFlags & kFlagDeltaTimeStamp)
                mInitialTime = GetCurrentTimeMs();
        }

        if(mnFlags & kFlagDate)
        {
            const time_t timeVal        = time(0);
            tm* const    timeComponents = localtime(&timeVal); 
            char         buffer[32];

            strftime(buffer, 32, "%b %d %H:%M:%S ", timeComponents);  // syslog style

            mLine.append(buffer);
        }
    #endif

    if(mnFlags & kFlagGroup)
    {
        mLine.append(helper->GetGroupName());
        mLine += ' ';
    }

    if(mnFlags & kFlagLevel)
    {
        mLine.append(inRecord.GetLevelName());
        mLine += ' ';
    }

    mLine.append(text);

    const eastl_size_t lineLength = mLine.length();

    if(!lineLength || (mLine[lineLength - 1] != '\n'))
        mLine += '\n'; 
    
    // Default formatter outputs the file/line number if level warrants it
    // this must be first character on line to support hypertext VC++ feature.
    if(helper->GetLevel() >= mnFileInfoLevel)
    {
        const ReportingLocation* const loc = helper->GetReportingLocation();

        mLine.append_sprintf("%s(%d): %s\n", loc->GetFilename(), loc->GetLine(), loc->GetFunction());
    }

    return mLine.c_str();
}


void LogFormatterFancy::SetName(const char* name)
{
    mName = name;
}


const char* LogFormatterFancy::GetName() const
{
    return mName.c_str();
}


/// caller must call SetName() after this with a unique name
ILogFormatter* LogFormatterFancy::Clone()
{
    LogFormatterFancy* const pFormatter = new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "LogFormatterFancy")
        LogFormatterFancy(mName.c_str(), mpCoreAllocator);

    pFormatter->mnFlags         = mnFlags;
    pFormatter->mnFileInfoLevel = mnFileInfoLevel;

    return pFormatter;
}


void* LogFormatterFancy::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID:
            return static_cast<IUnknown32*>(this);

        case ILogFormatter::kIID:
            return static_cast<ILogFormatter*>(this);

        case LogFormatterFancy::kIID:
            return static_cast<LogFormatterFancy*>(this);
    }

    return NULL; 
}


int LogFormatterFancy::AddRef()
{
    return ++mnRefCount;
}


int LogFormatterFancy::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    mnRefCount = 0; // This is useful for users with debugger, but has no benefit to the running code itself.
    delete this;    // LogFormatterFancy derives from ZoneObject and has overloaded 'operator delete'
    return 0;
}


void LogFormatterFancy::SetFlags(uint16_t flags)
{
    mnFlags = flags;
}


uint16_t LogFormatterFancy::GetFlags() const
{
    return mnFlags;
}


void LogFormatterFancy::SetFileInfoLevel(tLevel level)
{
    mnFileInfoLevel = level;
}


tLevel LogFormatterFancy::GetFileInfoLevel() const
{
    return mnFileInfoLevel;
}


/////////////////////////////////////////////////////////////////////////////
// LogFilterGroupLevels
/////////////////////////////////////////////////////////////////////////////

LogFilterGroupLevels::LogFilterGroupLevels(Allocator::ICoreAllocator* pAllocator)
  : Trace::ZoneObject(),
    mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator()),
    mnRefCount(0),
    mName(),
    mGlobalLevel(kLevelAll),
    mGroupLevelMap(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "LogFilterGroupLevels/GroupLevelMap", 
        pAllocator ? pAllocator : EA::Trace::GetAllocator()))
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFilterGroupLevels");
    #endif
}


LogFilterGroupLevels::LogFilterGroupLevels(const char* name, Allocator::ICoreAllocator* pAllocator)
  : mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator()),
    mnRefCount(0),
    mName(name),
    mGlobalLevel(kLevelAll),
    mGroupLevelMap(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "LogFilterGroupLevels/GroupLevelMap", 
        pAllocator ? pAllocator : EA::Trace::GetAllocator()))
{
    #if EASTL_NAME_ENABLED
        mName.get_allocator().set_name(EATRACE_ALLOC_PREFIX "LogFilterGroupLevels");
    #endif
}


LogFilterGroupLevels::~LogFilterGroupLevels()
{
    Reset();
}

void LogFilterGroupLevels::SetAllocator(Allocator::ICoreAllocator* pAllocator)
{
    mpCoreAllocator = pAllocator;
    mGroupLevelMap.get_allocator().set_allocator(pAllocator);
}

void LogFilterGroupLevels::AddGroupLevel(const char* pGroup, tLevel nLevel)
{
    if(pGroup && pGroup[0])
    {
        GroupLevelMap::iterator it = mGroupLevelMap.find(pGroup);

        if(it == mGroupLevelMap.end())
        {
            // In this case we create a new group/nLevel association.
            char* const pGroupContainer = EA_TRACE_CA_NEW_ARRAY(char, mpCoreAllocator, EATRACE_ALLOC_PREFIX "LogFilterGroupLevels/GroupContainer/char[]", strlen(pGroup) + 1);
            strcpy(pGroupContainer, pGroup);

            const GroupLevelMap::value_type value(pGroupContainer, nLevel);
            mGroupLevelMap.insert(value);
        }
        else
        {
            // In this case we change an existing new group/nLevel association.
            GroupLevelMap::value_type& value = *it;
            value.second = nLevel;
        }
    }
    else
    {
        mGlobalLevel = nLevel;
    }
}


bool LogFilterGroupLevels::RemoveGroupLevel(const char* pGroup)
{
    if (pGroup && pGroup[0])
    {
        GroupLevelMap::iterator it = mGroupLevelMap.find(pGroup);

        if(it != mGroupLevelMap.end())
        {
            EA_TRACE_CA_DELETE_ARRAY((char*)(it->first), mpCoreAllocator);
            mGroupLevelMap.erase(it); 

            return true;
        }
    }
    else
    {
        // erase all groups-levels
        for(GroupLevelMap::iterator it(mGroupLevelMap.begin()), itEnd(mGroupLevelMap.end()); it != itEnd; ++it)
            EA_TRACE_CA_DELETE_ARRAY((char*)(it->first), mpCoreAllocator);

        mGroupLevelMap.clear();
    }

    return false;
}

tLevel LogFilterGroupLevels::GetGroupLevel(const char* pGroup) const
{
    if(pGroup && pGroup[0])
    {
        const GroupLevelMap::const_iterator it = mGroupLevelMap.find(pGroup);

        if(it != mGroupLevelMap.end())
        {
            const GroupLevelMap::value_type& value = *it;
            return value.second;
        }
    }

    // Else the above didn't find a match so we fall through to the global nLevel.
    return mGlobalLevel;
}

void LogFilterGroupLevels::GetGroupList(GroupList* pGroupList) const
{
    pGroupList->clear();
    for (GroupLevelMap::const_iterator it = mGroupLevelMap.begin(), itEnd = mGroupLevelMap.end(); it != itEnd; ++it)
        pGroupList->push_back((*it).first);
}

void LogFilterGroupLevels::Reset()
{
    mGlobalLevel = kLevelAll;

    // remove all of the group-level settings
    const char emptyString = 0; // It is more cache-friendly to use this than "".
    RemoveGroupLevel(&emptyString);
}

bool LogFilterGroupLevels::IsFiltered(const TraceHelper& helper) const
{
    const tLevel nOutputLevel = GetGroupLevel(helper.GetGroupName());

    // any level less than output level is filtered out
    return (helper.GetLevel() < nOutputLevel);
}

bool LogFilterGroupLevels::IsFiltered(const LogRecord& record) const
{
    return IsFiltered(*record.GetTraceHelper());
}

const char* LogFilterGroupLevels::GetName() const
{
    return mName.c_str();
}

void LogFilterGroupLevels::SetName(const char* pName)
{
    mName = pName;
}

ILogFilter* LogFilterGroupLevels::Clone()
{
    LogFilterGroupLevels* filter = new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "LogFilterGroupLevels/LogFilterGroupLevels")
        LogFilterGroupLevels;

    filter->SetName(GetName()); // caller should callSetName() to ensure unique name after clone
    
    // set the global level
    const char emptyString = 0;
    filter->AddGroupLevel(&emptyString, mGlobalLevel);

    // cannot copy the map, cloned map must allocate its own strings
    for (GroupLevelMap::iterator it(mGroupLevelMap.begin()), itEnd(mGroupLevelMap.end()); it != itEnd; ++it)
    {
        filter->AddGroupLevel((*it).first, (*it).second);
    }

    return filter;
}

void* LogFilterGroupLevels::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID:
            return static_cast<IUnknown32*>(this);

        case ILogFilter::kIID:
            return static_cast<ILogFilter*>(this);

        case LogFilterGroupLevels::kIID:
            return static_cast<LogFilterGroupLevels*>(this);
    }

    return NULL; 
}


int LogFilterGroupLevels::AddRef()
{
    return ++mnRefCount;
}


int LogFilterGroupLevels::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    mnRefCount = 0; // This is useful for users with debugger, but has no benefit to the running code itself.
    delete this;    // LogFilterGroupLevels derives from ZoneObject and has overloaded 'operator delete'
    return 0;
}



/////////////////////////////////////////////////////////////////////////////
// LogReporter
/////////////////////////////////////////////////////////////////////////////

LogReporter::LogReporter()
  : Trace::ZoneObject(),
    mIsEnabled(true),
   #if EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
    mbFlushOnWrite(true),
   #else
    mbFlushOnWrite(false),
   #endif
    mnRefCount(0),
    mName()
{
    // caller must supply name
    // default filter/formatter is supplied by the service at registration if one is not already present
}

LogReporter::LogReporter(const char* pName)
  : Trace::ZoneObject(),
    mIsEnabled(true),
   #if EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
    mbFlushOnWrite(true),
   #else
    mbFlushOnWrite(false),
   #endif
    mnRefCount(0),
    mName()
{
    SetName(pName);
    // default filter/formatter is supplied by the service at registration if one is not already present
}

LogReporter::~LogReporter()
{
    // Empty
}

const char* LogReporter::GetName() const
{
    return mName.c_str();
}

void LogReporter::SetName(const char* pName)
{
    mName = pName;

    // unless we support filter aliasing, make sure the names match on the filter/formatter
    if (mFilter)
        mFilter->SetName(pName); 
    if (mFormatter)
        mFormatter->SetName(pName);
}

void LogReporter::SetEnabled(bool isEnabled)
{
    // config code should make sure that it should re-eval filtering
    //  on the helper when this changes state
    mIsEnabled = isEnabled;
}

bool LogReporter::IsEnabled() const
{
    return mIsEnabled;
}

void LogReporter::SetFlushOnWrite(bool flushOnWrite)
{
    mbFlushOnWrite = flushOnWrite;
}

bool LogReporter::GetFlushOnWrite() const
{
    return mbFlushOnWrite;
}

bool LogReporter::IsFiltered(const TraceHelper& helper)
{
    return (!(mIsEnabled && mFilter && mFormatter)) || mFilter->IsFiltered(helper);
}

bool LogReporter::IsFiltered(const LogRecord& record)
{
    return (!(mIsEnabled && mFilter && mFormatter)) || mFilter->IsFiltered(record);
}

void LogReporter::SetFilter(ILogFilter* filter)
{
    mFilter = filter;
}

ILogFilter* LogReporter::GetFilter()
{
    return mFilter;
}

void LogReporter::SetFormatter(ILogFormatter* formatter)
{
    mFormatter = formatter;
}

ILogFormatter*  LogReporter::GetFormatter()
{
    return mFormatter;
}

void* LogReporter::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID:
            return static_cast<IUnknown32*>(this);

        case ILogReporter::kIID:
            return static_cast<ILogReporter*>(this);
    }

    return NULL; 
}


int LogReporter::AddRef()
{
    return ++mnRefCount;
}


int LogReporter::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    mnRefCount = 0; // This is useful for users with debugger, but has no benefit to the running code itself.
    delete this;    // LogReporter derives from ZoneObject and has overloaded 'operator delete'
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Server
/////////////////////////////////////////////////////////////////////////////

// If EA_CUSTOM_TRACER_ENABLED is defined to 1, then we expect that 
// the user will implement their own version of CreateDefaultTracer.
// Also, there is no use creating a log server if logging is disabled.
#if !EA_CUSTOM_TRACER_ENABLED && EA_LOG_ENABLED
    ITracer* CreateDefaultTracer(Allocator::ICoreAllocator* pAllocator)
    { 
        pAllocator = pAllocator ? pAllocator : EA::Trace::GetAllocator();
        Server* const pServer = new(pAllocator, EATRACE_ALLOC_PREFIX "Server") Server(pAllocator);
        pServer->Init();
        return pServer->AsTracer();
    }
#endif // Else the user is expected to provide this function.


IServer* GetServer()
{
    ITracer* const tracer = EA::Trace::GetTracer();
    return tracer ? (IServer*)tracer->AsInterface(IServer::kIID) : NULL;
}


void SetServer(IServer* pServer)
{
    EA::Trace::SetTracer(pServer ? pServer->AsTracer() : NULL);
}



Server::Server(Allocator::ICoreAllocator* pAllocator)
  : Trace::ZoneObject()
  , mpCoreAllocator(pAllocator ? pAllocator : EA::Trace::GetAllocator())
  , mbIsReporting(false)
  , mpHeapBuffer(EA_TRACE_CA_NEW_ARRAY(char, mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/HeapBuffer/char[]", 4096))
  , mnHeapBufferSize(4096)
  , mLogRecordCounter(0)
  , mLogReporters(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "Server/LogReporters", mpCoreAllocator))
  , mDefaultFilter()
  , mDefaultFormatter()
  , mnRefCount(0)
  , mMutex()
{
    mLogReporters.reserve(8);

    #ifdef EA_DEBUG
        memset(mpHeapBuffer, 0, mnHeapBufferSize * sizeof(char));
    #endif
}


Server::~Server()
{
    EA_TRACE_CA_DELETE_ARRAY(mpHeapBuffer, mpCoreAllocator);
    RemoveAllLogReporters();
    mnRefCount = 0; // This is useful for users with debugger, but has no benefit to the running code itself.
}


void Server::Init()
{
    Thread::AutoMutex autoMutex(mMutex);

    // add some default filters, formatters, and reporters
    if(!mDefaultFilter)
    {
        mDefaultFilter = new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/DefaultFilter/LogFilterGroupLevels") 
            LogFilterGroupLevels("DefaultFilter");
    }
    if(!mDefaultFormatter)
    {
        // LogFormatterSimple is a ref-counting, ZoneObject-deriving class
        mDefaultFormatter = new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/DefaultFormatter") LogFormatterSimple("DefaultFormatter");
    }

    if(mLogReporters.empty())
    {
        // On windows console and debugger both may be present, but stdout is more standard across platforms.
        // TODO: should default logger be debugger or console ? 
        // add in the default reporters, first to debugger, then to a dialog
        AddLogReporter(new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/LogReporterDebugger")LogReporterDebugger("AppDebugger"));

        // only have dialog support on windows currently
        //  should this be enabled/disabled by init/shutdown ?
        // automation needs the dialogs off to run the app
        AddLogReporter(new(mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/LogReporterDialog")LogReporterDialog("AppAlertDialog"));
    }
    
    // make sure to update the filtering at the end
    UpdateLogFilters();
}

ITracer* Server::AsTracer()
{
    return static_cast<ITracer*>(this);
}


bool Server::AddLogReporter(ILogReporter* pLogReporter, bool bAllowDuplicateName)
{
    Thread::AutoMutex autoMutex(mMutex);

    // handle duplicates
    EA::Trace::AutoRefCount<ILogReporter> existingReporter;
    const char8_t* const pName = pLogReporter->GetName();

    if(GetLogReporter(pName, existingReporter.AsPPTypeParam(), 0))
    {
        if (existingReporter == pLogReporter) // if pLogReported has already been added...
            return true;

        if(!bAllowDuplicateName)
        {
            RemoveLogReporter(existingReporter); 
            existingReporter = NULL; // destroy existing reporter
        }
    }

    // This impl clones the formatter and filter, other impls might alias/share them
    // if aliasing occurs, it is suggested that the server keep an array of named 
    // filters and formatters.  This allows a filter to be defined before the reporter
    // exists which can be quite useful to define UI reporter filters since the actual
    // reporter does not exist until after UI startup.
    
    // clone the default formatter if one is not already present
    if (!pLogReporter->GetFormatter() && mDefaultFormatter)
    {
        ILogFormatter* const formatter = mDefaultFormatter->Clone(); 

        formatter->SetName(pName); // make the name match our own
        pLogReporter->SetFormatter(formatter);
    }
    
    // clone the default filter if one is not already present
    if (!pLogReporter->GetFilter() && mDefaultFilter)
    {
        ILogFilter* const filter = mDefaultFilter->Clone(); 

        filter->SetName(pName); // make the name match our own
        pLogReporter->SetFilter(filter);
    }

    mLogReporters.push_back(pLogReporter); // This will AddRef the back-end

    return true;
}


bool Server::RemoveLogReporter(ILogReporter* pLogReporter)
{
    Thread::AutoMutex autoMutex(mMutex);

    for(LogReporters::iterator it(mLogReporters.begin()), itEnd(mLogReporters.end()); it != itEnd; ++it)
    {
        const ILogReporter* const pLogReporterTemp = *it;

        if(pLogReporterTemp == pLogReporter)
        {
            mLogReporters.erase(it); // This will auto-release the pointer.
            return true;
        }
    }

    return false;
}


void Server::RemoveAllLogReporters()
{
    Thread::AutoMutex autoMutex(mMutex);

    mLogReporters.clear(); // This will auto-release the pointers.
}


bool Server::GetLogReporter(const char* pOuputBackEndName, ILogReporter** ppLogReporter, int reporterIndex) const
{
    Thread::AutoMutex autoMutex(mMutex);
    int i = 0;

    *ppLogReporter = NULL;

    for(LogReporters::const_iterator it(mLogReporters.begin()), itEnd(mLogReporters.end()); it != itEnd; ++it)
    {
        ILogReporter*  const pLogReporter = *it;
        const char8_t* const pName = pLogReporter->GetName();

        if(EA::StdC::Stricmp(pName, pOuputBackEndName) == 0)
        {
            if(i++ == reporterIndex)
            {
                pLogReporter->AddRef(); // AddRef for the caller.
                *ppLogReporter = pLogReporter;
                break;
            }
        }
    }

    return (*ppLogReporter != NULL);
}


size_t Server::EnumerateLogReporters(ILogReporter* pLogReporterArray[], size_t nLogReporterArrayCount)
{
    Thread::AutoMutex autoMutex(mMutex);

    size_t i = 0;
    
    if(pLogReporterArray)
    {
        for(LogReporters::iterator it(mLogReporters.begin()), itEnd(mLogReporters.end()); it != itEnd; ++it)
        {
            if (i >= nLogReporterArrayCount)
                break;

            ILogReporter* const pLogReporter = *it;
            pLogReporter->AddRef();
            pLogReporterArray[i++] = pLogReporter;
        }
    }

    return i;
}


void Server::SetDefaultFilter(ILogFilter* filter)
{
    mDefaultFilter = filter;
}


void Server::SetDefaultFormatter(ILogFormatter* formatter)
{
    mDefaultFormatter = formatter;
}


bool Server::SetOutputLevel(const char* pLogReporterName, const char* pGroup, tLevel nLevel, int reporterIndex)
{
    Thread::AutoMutex autoMutex(mMutex);

    bool success = true;

    if(pLogReporterName)
    {
        EA::Trace::AutoRefCount<ILogReporter> pLogReporter;

        if(GetLogReporter(pLogReporterName, pLogReporter.AsPPTypeParam(), reporterIndex))
        {
            ILogFilter*           const pLogFilter = pLogReporter->GetFilter();
            LogFilterGroupLevels* const pLogFilterGroupLevels = pLogFilter ? (LogFilterGroupLevels*)pLogFilter->AsInterface(LogFilterGroupLevels::kIID) : NULL;

            if (pLogFilterGroupLevels)
            {
                if (nLevel == kLevelUndefined)
                    pLogFilterGroupLevels->RemoveGroupLevel(pGroup);
                else
                    pLogFilterGroupLevels->AddGroupLevel(pGroup, nLevel);  
            }
        }
        else
        {
            success = false;
        }
    }
    else
    {
        // is this particularly useful to be able to set a level across all groups ?
        //    typically each filter has been specifically configured to specific setting and this corrups that filter definition
         //  Sims2 pulled this functionality for that reason. 

        LogFilterGroupLevels* const pDefaultLogFilterGroupLevels = mDefaultFilter ? (LogFilterGroupLevels*)mDefaultFilter->AsInterface(LogFilterGroupLevels::kIID) : NULL;
        if (pDefaultLogFilterGroupLevels)
        {
            if (nLevel == kLevelUndefined)
                pDefaultLogFilterGroupLevels->RemoveGroupLevel(pGroup);
            else
                pDefaultLogFilterGroupLevels->AddGroupLevel(pGroup, nLevel);
        }

        // add group/level to all of the reporters
        for(LogReporters::iterator it(mLogReporters.begin()), itEnd(mLogReporters.end()); it != itEnd; ++it)
        {
            ILogReporter*         const pLogReporter = *it;
            ILogFilter*           const pLogFilter = pLogReporter->GetFilter();
            LogFilterGroupLevels* const pLogFilterGroupLevels = pLogFilter ? (LogFilterGroupLevels*)pLogFilter->AsInterface(LogFilterGroupLevels::kIID) : NULL;

            if (pLogFilterGroupLevels)
            {
                if (nLevel == kLevelUndefined)
                    pLogFilterGroupLevels->RemoveGroupLevel(pGroup);
                else
                    pLogFilterGroupLevels->AddGroupLevel(pGroup, nLevel);
            }
        }
    }

    return success;
}


void Server::SetAllEnabled(bool enabled)
{
    // this invalidates all of the filtering states on the helpers
    GetTraceHelperTable()->SetAllEnabled(enabled);
}

void Server::UpdateLogFilters()
{
    // this invalidates all of the filtering states on the helpers
    GetTraceHelperTable()->UpdateTracer();
}

bool Server::IsFiltered(const TraceHelper& helper) const
{
    Thread::AutoMutex autoMutex(mMutex);

    // Here we do an initial test to see if we should go through the trouble to do the 
    // string printf and output processing. This is a quick test that obviates memory 
    // allocations, string processing, and thread and reentrancy protection efforts.
    for(LogReporters::const_iterator it(mLogReporters.begin()), itEnd(mLogReporters.end()); it != itEnd; ++it)
    {
        // have to do this to call non-const AsTracer (no real const policy for AsXXX calls yet)
        ILogReporter* pLogReporter = *it; 

        if(pLogReporter && !pLogReporter->IsFiltered(helper))
            return false;
    }

    return true;
}


tAlertResult Server::TraceV(const TraceHelper& helper, const char* pFormat, va_list argList)
{
    int  alertResult = kAlertResultNone;
    if (!pFormat)
        return alertResult;

    // This is to protect the shared heap buffer from changing
    Thread::AutoMutex autoMutex(mMutex);

    // Re-entrancy protection, only one heap buffer
    if (mbIsReporting)
        return alertResult;

    const int kStackBufferSize = 256;
    char      pStackBuffer[kStackBufferSize];

    #if EATRACE_VA_COPY_ENABLED
        va_list argListSaved;
        va_copy(argListSaved, argList);
    #endif

    int nCharsThatShouldBeWritten = EA::StdC::Vsnprintf(pStackBuffer, kStackBufferSize, pFormat, argList);
    pStackBuffer[kStackBufferSize-1] = '\0'; // TODO: make vsnprintf do this, should not have to fix up afterwards ?

    if (nCharsThatShouldBeWritten >= -1)  // DONE: specifically -1 means that more chars than buffer size, other negatives imply an error with params
    {
        if((nCharsThatShouldBeWritten >= 0) && // If the input buffer to vsnprintf above was sufficiently large.
            (nCharsThatShouldBeWritten < kStackBufferSize))
        {
            alertResult = Trace(helper, pStackBuffer);
        }
        else if(nCharsThatShouldBeWritten >= 0) // If we are seeing C99 behaviour whereby the return value is larger than the input buffer size...
        {
            if (nCharsThatShouldBeWritten >= mnHeapBufferSize)
            {
                while (mnHeapBufferSize < nCharsThatShouldBeWritten)
                    mnHeapBufferSize *= 2;
                EA_TRACE_CA_DELETE_ARRAY(mpHeapBuffer, mpCoreAllocator);
                mpHeapBuffer = EA_TRACE_CA_NEW_ARRAY(char, mpCoreAllocator, EATRACE_ALLOC_PREFIX "Server/HeapBuffer/char[]", (size_t)(unsigned)mnHeapBufferSize);
            }

            #if EATRACE_VA_COPY_ENABLED
                va_copy(argList, argListSaved);
            #endif

            int value = EA::StdC::Vsnprintf(mpHeapBuffer, (size_t)mnHeapBufferSize, pFormat, argList);
            mpHeapBuffer[mnHeapBufferSize-1] = '\0'; // TODO: make vsnprintf do this, should not have to fix up afterwards ?
            if (value >= -1)
                alertResult = Trace(helper, mpHeapBuffer);

            EA_ASSERT_MESSAGE(value >= 0 && value < mnHeapBufferSize, ("C99 vsnprintf inconsistent return value\n"));
        }  
        else
        { 
            #if EATRACE_VA_COPY_ENABLED
                va_copy(argList, argListSaved);
            #endif

            int value = EA::StdC::Vsnprintf(mpHeapBuffer, (size_t)mnHeapBufferSize, pFormat, argList);
            mpHeapBuffer[mnHeapBufferSize-1] = '\0';
            if(value >= -1)
                alertResult = Trace(helper, mpHeapBuffer);
        }
    }
        
    return alertResult;
}



tAlertResult Server::Trace(const TraceHelper& helper, const char* pText)
{
    int alertResult = kAlertResultNone;

    if(pText)
    {
        // This code blocks multiple incoming threads, but means that we do not have to copy the aliased strings and can also
        //  re-use the helper class.
        // This also simplifies the reporters, filters, and formatters which only need to be thread safe if they have external access.
        //  An example of this is a title bar reporter, whose Report() would lock then set text, then main thread would display using locked GetText() call.
        // It also means that the alert result is returned correctly.  Thread safety is ensured because the incoming threads are
        //  blocked (note that pre-filtering passed, so some reporter is likely going to log the record.  
        // Re-entrancy is blocked via the mbIsReporting switch, and should not be used with the implementation.  
        //  This just means that reporters should not log/assert.

        Thread::AutoMutex autoMutex(mMutex);

        if(!mbIsReporting)
        {
            // Lock out this same thread from re-entering the reporting loop.
            mbIsReporting = true;

            // log record timing should be recorded and pass to this routine before the lock
            LogRecord logRecord;
            logRecord.SetTraceHelper(helper);
            logRecord.SetRecordInfo(mLogRecordCounter++, pText, GetLevelName(helper.GetLevel())); 

            // We make a copy of the LogReporter list. The reason we do this is that we will be 
            // calling external code, which may in turn come back and modify us and invalidate mLogReporters.
            LogReporters logReportersCopy(mLogReporters); // logReportersCopy has auto-refcounted members.

            mMutex.Unlock(); // Our mutex must be unlocked while calling external code, or else we invite opportunities for deadlocks.

            for(LogReporters::iterator it(logReportersCopy.begin()), itEnd(logReportersCopy.end()); it != itEnd; ++it)
            {
                ILogReporter* const pLogReporter = *it;

                // The reporters get one last chance to filter the record with it's additional info. Then they report.
                if(pLogReporter && !pLogReporter->IsFiltered(logRecord)) 
                    alertResult |= pLogReporter->Report(logRecord);
            }

            mMutex.Lock(); // Match the explicit lock above.

            mbIsReporting = false;
        }
    }

    return alertResult;
}


const char* Server::GetLevelName(tLevel level) const
{
    // ToDo: move this to a level class, so that a server is not necesary for translation
    // Counter suggestion by Paul P: Change the names so that merely numbers are used internally.

    // In this implementation we recognize only a few specific levels, but we allow the 
    // user to safely specify any level by simply promoting user-specified levels to our
    // known levels.
    if(level <= kLevelDebug)
        return "Debug";
    else if(level <= kLevelInfo)
        return "Info";
    else if(level <= kLevelWarn)
        return "Warn";
    else if(level <= kLevelError)
        return "Error";
    return "Fatal";
}


void* Server::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID:
            return static_cast<IUnknown32*>(static_cast<IServer*>(this));

        case IServer::kIID:
            return static_cast<IServer*>(this);

        case Server::kIID:
            return static_cast<Server*>(this);

        case ITracer::kIID:
            return static_cast<ITracer*>(this);
    }

    return NULL;
}


int Server::AddRef()
{
    return mnRefCount.Increment();     // AtomicInteger thread-safe operation.
}


int Server::Release()
{
    int32_t rc = mnRefCount.Decrement();
    if(rc)
        return rc;

    // Set ref count to 1 to prevent double destroy if AddRef/Release is called while in destructor. 
    // This can happen if the destructor does things like debug traces. We set the ref count to 0
    // in the destructor itself to help the programmer see that the object is deleted.
    mnRefCount.SetValue(1); 

    delete this; // Server derives from ZoneObject and has overloaded 'operator delete'
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// LogReporterDialog
///////////////////////////////////////////////////////////////////////////////

tAlertResult LogReporterDialog::Report(const LogRecord& record)
{
    tAlertResult result = kAlertResultNone;
    
    const char8_t* const text        = mFormatter->FormatRecord(record); 
    const char8_t* const levelName   = record.GetLevelName();
    const char*    const dialogTitle = levelName ? levelName : "Alert"; 
    
    // On the Windows platform, this launches a thread with the dialog box, 
    // avoids paint and windows messages hitting the main thread and causing 
    // further errors there.
    void* pContext;
    DisplayTraceDialogFunc pFunc = GetDisplayTraceDialogFunc(pContext);

    result = pFunc(dialogTitle, text, pContext);
    
    return result;
}



///////////////////////////////////////////////////////////////////////////////
// LogReporterDebugger
///////////////////////////////////////////////////////////////////////////////

tAlertResult LogReporterDebugger::Report(const LogRecord& record)
{
    const char* const pOutput = mFormatter->FormatRecord(record);

    #if defined(EA_PLATFORM_MICROSOFT)
        // Note: there is an undocumented 32K limit to the number of chars that can be output via OutputDebugString.
        OutputDebugStringA(pOutput);
        return kAlertResultNone;
    #else
        // We use Printf("%", pOutput) instead of Printf(pOutput) because pOutput might have "%" chars in it.
        EA::StdC::Fprintf(stdout, "%s", pOutput);

        if(mbFlushOnWrite)
            fflush(stdout);

        return kAlertResultNone;
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// LogReporterStdio
///////////////////////////////////////////////////////////////////////////////

tAlertResult LogReporterStdio::Report(const EA::Trace::LogRecord& record)
{
    EA::StdC::Fprintf(mpFILE, "%s", mFormatter->FormatRecord(record));

    if(mbFlushOnWrite)
        fflush(stdout);

    return kAlertResultNone;
}



///////////////////////////////////////////////////////////////////////////////
// LogReporterStream
///////////////////////////////////////////////////////////////////////////////

LogReporterStream::LogReporterStream(const char8_t* name, IO::IStream* pStream)
  : LogReporter(name),
    mpStream(pStream)
{
    if(mpStream)
        mpStream->AddRef();
}


LogReporterStream::~LogReporterStream()
{
    if(mpStream)
        mpStream->Release();
}


tAlertResult LogReporterStream::Report(const LogRecord& record)
{
    const char* const pOutput = mFormatter->FormatRecord(record);

    mpStream->Write(pOutput, strlen(pOutput));

    if(mbFlushOnWrite)
        mpStream->Flush();

    return kAlertResultNone;
}



///////////////////////////////////////////////////////////////////////////////
// LogReporterFile
///////////////////////////////////////////////////////////////////////////////

LogReporterFile::LogReporterFile(const char8_t* name, const char8_t* path, 
                                    Allocator::ICoreAllocator* /*pAllocator*/, bool openImmediately)
  : LogReporter(name),
    mFileStream(path),
    mAttemptedOpen(false)
{
    if(openImmediately)
        Open();
}

LogReporterFile::LogReporterFile(const char8_t* name, const char16_t* path, 
                                    Allocator::ICoreAllocator* /*pAllocator*/, bool openImmediately)
  : LogReporter(name),
    mFileStream(path),
    mAttemptedOpen(false)
{
    if(openImmediately)
        Open();
}

LogReporterFile::LogReporterFile(const char8_t* name, const char32_t* path, 
                                    Allocator::ICoreAllocator* /*pAllocator*/, bool openImmediately)
  : LogReporter(name),
    mFileStream(),          // Some FileStream implementations don't support char32_t ctor, as of this writing. So we Strlcpy below.
    mAttemptedOpen(false)
{
    char8_t path8[EA::IO::kMaxPathLength];
    EA::StdC::Strlcpy(path8, path, EAArrayCount(path8));
    mFileStream.SetPath(path8);

    if(openImmediately)
        Open();
}


#if EA_WCHAR_UNIQUE
    LogReporterFile::LogReporterFile(const char8_t* name, const wchar_t* path, 
                                        Allocator::ICoreAllocator* /*pAllocator*/, bool openImmediately)
      : LogReporter(name),
        mFileStream(path),
        mAttemptedOpen(false)
    {
        if(openImmediately)
            Open();
    }
#endif


LogReporterFile::~LogReporterFile()
{
    // File will auto-close if open.
}


bool LogReporterFile::Open(int nAccessFlags, int nCreationDisposition, int nSharing)
{
    bool result;

    mAttemptedOpen = true;

    if(mFileStream.GetAccessFlags()) // If open already...
        result = true;
    else
    {
        result = mFileStream.Open(nAccessFlags, nCreationDisposition, nSharing);
        EA_ASSERT_FORMATTED(result, ("LogReporterFile::Open() -- failed with error: %d (see IOResultCode)\n", mFileStream.GetState() - EA::IO::kFSErrorBase));
    }

    return result;
}


tAlertResult LogReporterFile::Report(const LogRecord& record)
{
    const char* const pOutput = mFormatter->FormatRecord(record);

    if(!mAttemptedOpen)
        Open();

    // This following is disabled because WriteLine unilaterally appends 
    // a \n to every output, and the user doesn't necessarily want that.
    // EA::IO::WriteLine(mpFileStream, pOutput, strlen(pOutput));

    mFileStream.Write(pOutput, strlen(pOutput));

    if(mbFlushOnWrite)
        mFileStream.Flush();

    return kAlertResultNone;
}



} // namespace Trace

} // namespace EA


// For unity build friendliness, undef all local #defines.
#undef LOG_FORMATTER_FANCY_TIME


#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

