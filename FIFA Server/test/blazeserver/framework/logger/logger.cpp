/*************************************************************************************************/
/*!
    \file logger.cpp

    \attention (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Logger

    This is the main logger class for the blaze server.  Output of log data is handled by a
    separate thread to dissociate file operations from the main blaze server threads.  This is
    to prevent issues writing to disk (eg. disk full) from impacting the server's operation.
    All log events are queued to this thread for output.  A fixed number of log entries are
    preallocated for this process.  If the preallocated blocks are exhausted (typically because
    there are problems outputting data), then new log entries will be dropped and a notice will
    be added to the log output indicating the number of log messages lost.  While it is not ideal
    that log entries be lost, this is a protectionist approach to ensuring that a) the server
    doesn't stall when outputting logs is backed up and b) prevent unbounded memory growth during
    such an outage which could eventually exhaust server memory.

    There are two pools of log entries: regular and large.  The majority of log output will fit
    into the regular sized blocks.  If formatting fails on a regular sized entry, a large block
    will be grabbed instead.

    In order to limit contention between threads for the log entries and output queue, two stages
    of locking are done.  A spin lock is used to quickly grab an available log entry.  This has
    very low overhead and a very short duration of holding the log.  The entry is then formatted
    with the input data.  The queue lock is then taken and the entry is added to the queue.
    The output thread does a timed wait on a condition variable to detect new log entries being
    queued.  To prevent unnecessary mutex lock/unlock calls as the output thread is draining the
    queue, it takes the lock, swaps out the queue for a new one and unlocks.  It is then free to
    output the data without blocking other threads from queueing data.  Queueing threads will
    only notify the condition variable if the input queue is past a certain threshold.  This
    helps reduce the lock/unlock calls on the mutex.  Since the output thread is doing a timed
    wait on the condition, it will already wake up regularly to pull entries from the queue.
    This allows the entries to be batched.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/logger/logfileinfo.h"
#include "framework/logger/logoutputter.h"
//#include "framework/logger/logreader.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbscheduler.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/util/shared/rawbufferistream.h"
#include "EATDF/time.h"

#include "framework/system/allocator/callstack.h"
#include "framework/system/blazethread.h"
#include "framework/system/threadlocal.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/session.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/metrics/metrics.h"
#include "framework/component/inboundrpccomponentglue.h"

#include "EASTL/intrusive_list.h"
#include "EAIO/EAFileBase.h"
#include "EAIO/EAFileUtil.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_rwmutex.h"
#include "eathread/eathread_condition.h"
#include <sys/stat.h>
#include <stdio.h>

#include <grpc/grpc.h>
#include <google/protobuf/stubs/logging.h>

namespace Blaze
{

#ifdef __cplusplus
extern "C" {
#endif
void grpcCoreLogger(gpr_log_func_args *args)
{
    // grpc lib can log from both a thread created by Blaze and an internal thread. In order to log them in the correct order, we logs them all via
    // our external thread logger system. 
    Blaze::Logging::Level level = Logging::NONE;

    gpr_log_severity severity = args->severity;

    if (Logger::isRemapLogLevels())
    {
        severity = GPR_LOG_SEVERITY_DEBUG;
    }

    switch (severity)
    {
    case GPR_LOG_SEVERITY_DEBUG:
        level = Logging::TRACE;
        break;

    case GPR_LOG_SEVERITY_INFO:
        level = Logging::INFO;
        break;

    case GPR_LOG_SEVERITY_ERROR:
        level = Logging::ERR;
        break;

    default:
        BLAZE_ASSERT_COND_LOG(Blaze::Log::GRPC_CORE, false, "Add support for unhandled Grpc core log category");
        break;
    }

    if (Blaze::Logger::isEnabledWithoutContext(Blaze::Log::GRPC_CORE, level))
    {
        // Skip pointless spam when shutting down the server (every pending operation generates this 'ERR' message)
        if (strstr(args->message, "Server Shutdown") == nullptr)
            Blaze::Logger::logExternalThreadLogEntry(ExternalThreadLogEntry(args->message, Blaze::Log::GRPC_CORE, level));
    }

}

void protobufLogger(google::protobuf::LogLevel lvl, const char* filename, int line, const std::string& message)
{
    Blaze::Logging::Level level = Logging::NONE;
    
    switch (lvl)
    {
    case google::protobuf::LOGLEVEL_INFO:
        level = Logging::INFO;
        break;

    case google::protobuf::LOGLEVEL_WARNING:
        level = Logging::WARN;
        break;

    case google::protobuf::LOGLEVEL_ERROR:
        level = Logging::ERR;
        break;

    case google::protobuf::LOGLEVEL_FATAL:
        level = Logging::FATAL;
        break;

    default:
        BLAZE_ASSERT_COND_LOG(Blaze::Log::PROTOBUF, false, "Add support for unhandled Protobuf log category");
        break;
    }

    if (Blaze::Logger::isEnabledWithoutContext(Blaze::Log::PROTOBUF, level))
    {
        Blaze::Logger::logExternalThreadLogEntry(
            ExternalThreadLogEntry(eastl::string(eastl::string::CtorSprintf(), "Message:%s File:%s Line:%d", message.c_str(), filename, line).c_str(),
                Blaze::Log::PROTOBUF, level)
        );
    }
}

bool Logger::isRemapLogLevels()
{
    return mRemapLogLevels;
}
#ifdef __cplusplus
}
#endif


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#ifdef EA_PLATFORM_WINDOWS
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif


#define LOGGING_CONFIG_MAP_NAME "logging"

#define TRACE_NONE_STR "none"
#define TRACE_ALL_STR "all"
#define TRACE_CATEGORY_STR "category"


Logger::TraceInfo Logger::sTraceInfo[TRACE_IDX_MAX] =
{
    { "db", Logging::T_DB },
    { "rpc", Logging::T_RPC },
    { "http", Logging::T_HTTP },
    { "replication", Logging::T_REPL }
};

bool Logger::mIsInitialized = false;
const char8_t* Logger::BLAZE_LOG_FILE_EXTENSION = ".log";
const char8_t* Logger::BLAZE_LOG_BINARY_EVENTS_DIR = "binary_events";
const char8_t* Logger::BLAZE_LOG_PIN_EVENTS_DIR = "pin_events";
Logging::Level Logger::mLevel = Logging::INFO;
volatile bool Logger::mRemapLogLevels = false;
Logger::TraceSetting Logger::mTrace[TRACE_IDX_MAX];
bool Logger::mIncludeLocation = false;
bool Logger::mEncodeLocation = false;
bool Logger::mCoreOnAssertLog = false;
bool Logger::mSuppressAuditLogging = false;
bool Logger::mSuppressDebugConsoleLogging = false;
Logger::CategoryInfo Logger::mCategories[Log::CATEGORY_MAX] = { };
int32_t Logger::mMaxCategoryLength = 16;
LogOutputter* Logger::mOutputter = nullptr;
ExternalThreadLogOutputter* Logger::mExternalThreadLogOutputter = nullptr;
char8_t Logger::mLogDir[256] = { '\0' };
char8_t Logger::mName[256] = { '\0' };
bool Logger::mMainLogToStdout = true;
bool Logger::mMainLogToFile = false;
bool Logger::mOtherLogsToFile = false;
uint32_t Logger::mRotationFileSize = 10000000;
uint32_t Logger::mLargeEntrySize = 1024*1024;
uint32_t Logger::mLargeEntryCount = 64;
LoggingConfigPtr Logger::mConfig = nullptr;
EA::Thread::Mutex Logger::mConfigLock;
FiberLocalManagedPtr<StringBuilder> Logger::mCurrentLoggingMessage;
FiberLocalManagedPtr<StringBuilder> Logger::mThisFunctionLoggingMessage;
DbIdByPlatformTypeMap Logger::mDbIdByPlatformTypeMap;
static EA::Thread::RWMutex mUpdateAuditEntriesLock;
static AuditLogEntryList mAuditLogEntries(*::Blaze::Allocator::getAllocator(Blaze::MEM_GROUP_FRAMEWORK_DEFAULT), "Logger::mAuditLogEntries");
// secondary indexes for the log entry data
static eastl::hash_map<AuditId, AuditLogEntryPtr> mAuditLogEntryByAuditId(BlazeStlAllocator("Logger::mAuditLogEntryByAuditId", Blaze::MEM_GROUP_FRAMEWORK_DEFAULT));
static EA::Assert::FailureCallback sOldAssertFailureCallback = EA::Assert::GetFailureCallback();
FiberLocalManagedPtr<LogContextWrapper> gLogContext;



LogContextWrapper::LogContextWrapper(const LogContextWrapper& rhs)
{
    *this = rhs;
}

LogContextWrapper& LogContextWrapper::operator=(const LogContextWrapper& rhs)
{
    if (this != &rhs)
    {
        if (rhs.isSlaveSessionIdSet())
            setSlaveSessionId(rhs.getSlaveSessionId());
        if (rhs.isUserSessionIdSet())
            setUserSessionId(rhs.getUserSessionId());
        if (rhs.isBlazeIdSet())
            setBlazeId(rhs.getBlazeId());
        if (rhs.getClientIp().isIpSet() != 0)
            getClientIp().setIp(rhs.getClientIp().getIp());
        if (rhs.isDeviceIdSet())
            setDeviceId(rhs.getDeviceId());
        if (rhs.isPersonaNameSet())
            setPersonaName(rhs.getPersonaName());
        if (rhs.isNucleusAccountIdSet())
            setNucleusAccountId(rhs.getNucleusAccountId());
        if (rhs.isPlatformSet())
            setPlatform(rhs.getPlatform());

        mAuditIds = rhs.mAuditIds;
    }

    return *this;
}

void LogContextWrapper::set(SlaveSessionId slaveSessionId, BlazeId blazeId)
{
    clear();
    setSlaveSessionId(slaveSessionId);
    setBlazeId(blazeId);
    mAuditIds.clear();
    if (!Logger::isAuditLoggingSuppressed())
    {
        Logger::getAuditIds(blazeId, "", "", "", INVALID_ACCOUNT_ID, INVALID, mAuditIds);
    }
}

void LogContextWrapper::set(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
    const char8_t* personaName, uint32_t clientAddr, AccountId nucleusAccountId, ClientPlatformType platform)
{
    setSlaveSessionId(slaveSessionId);
    setUserSessionId(userSessionId);
    setBlazeId(blazeId);
    setDeviceId(deviceId);
    setPersonaName(personaName);
    getClientIp().setIp(clientAddr);
    setNucleusAccountId(nucleusAccountId);
    setPlatform(platform);
    mAuditIds.clear();
    if (!Logger::isAuditLoggingSuppressed())
    {
        Logger::getAuditIds(blazeId, deviceId, clientAddr, personaName, nucleusAccountId, platform, mAuditIds);
    }
}

void LogContextWrapper::clear()
{
    setSlaveSessionId(SlaveSession::INVALID_SESSION_ID);
    setUserSessionId(INVALID_USER_SESSION_ID);
    setBlazeId(INVALID_BLAZE_ID);
    setDeviceId("");
    setPersonaName("");
    getClientIp().setIp(0);
    setNucleusAccountId(INVALID_ACCOUNT_ID);
    setPlatform(INVALID);
    clearIsSetRecursive();
    mAuditIds.clear();
}

LogContextOverride::LogContextOverride(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
    const char8_t* personaName, const InetAddress& clientAddr, AccountId nucleusAccountId, ClientPlatformType platform)
{
    init(slaveSessionId, userSessionId, blazeId, deviceId, personaName, clientAddr.getIp(InetAddress::HOST), nucleusAccountId, platform);
}

LogContextOverride::LogContextOverride(BlazeId blazeId, const char8_t* deviceId, const char8_t* personaName, const InetAddress& clientAddr, AccountId nucleusAccountId, ClientPlatformType platform)
{
    SlaveSessionId slaveSessionId = (gRpcContext != nullptr ? gRpcContext->mMessage.getSlaveSessionId() : SlaveSession::INVALID_SESSION_ID);
    UserSessionId userSessionId = (gRpcContext != nullptr ? gRpcContext->mMessage.getUserSessionId() : SlaveSession::INVALID_SESSION_ID);

    init(slaveSessionId, userSessionId, blazeId, deviceId, personaName, clientAddr.getIp(InetAddress::HOST), nucleusAccountId, platform);
}

LogContextOverride::LogContextOverride(UserSessionId userSessionId)
{
    SlaveSessionId slaveSessionId = (gRpcContext != nullptr ? gRpcContext->mMessage.getSlaveSessionId() : SlaveSession::INVALID_SESSION_ID);
    UserSessionId sessionId = gLogContext->getUserSessionId();
    BlazeId blazeId = gLogContext->getBlazeId();
    const char8_t* deviceId = gLogContext->getDeviceId();
    const char8_t* personaName = gLogContext->getPersonaName();
    uint32_t clientIp = gLogContext->getClientIp().getIp();
    AccountId nucleusAccountId = gLogContext->getNucleusAccountId();
    ClientPlatformType platform = gLogContext->getPlatform();

    if ((userSessionId != INVALID_USER_SESSION_ID) && gUserSessionManager->getSessionExists(userSessionId))
    {
        sessionId = userSessionId;
        blazeId = gUserSessionManager->getBlazeId(userSessionId);
        deviceId = gUserSessionManager->getUniqueDeviceId(userSessionId);
        personaName = gUserSessionManager->getPersonaName(userSessionId);
        const PlatformInfo& platformInfo = gUserSessionManager->getPlatformInfo(userSessionId);
        nucleusAccountId = platformInfo.getEaIds().getNucleusAccountId();
        platform = platformInfo.getClientPlatform();
        clientIp = gUserSessionManager->getClientAddress(userSessionId);
    }

    init(slaveSessionId, sessionId, blazeId, deviceId, personaName, clientIp, nucleusAccountId, platform);
}

LogContextOverride::LogContextOverride(const LogContextWrapper& newLogContext)
{
    SlaveSessionId slaveSessionId = newLogContext.getSlaveSessionId();
    UserSessionId sessionId = newLogContext.getUserSessionId();
    BlazeId blazeId = newLogContext.getBlazeId();
    const char8_t* deviceId = newLogContext.getDeviceId();
    const char8_t* personaName = newLogContext.getPersonaName();
    uint32_t clientIp = newLogContext.getClientIp().getIp();
    AccountId nucleusAccountId = newLogContext.getNucleusAccountId();
    ClientPlatformType platform = newLogContext.getPlatform();

    init(slaveSessionId, sessionId, blazeId, deviceId, personaName, clientIp, nucleusAccountId, platform);
}

LogContextOverride::~LogContextOverride()
{
    if (gLogContext != nullptr)
        *gLogContext = mPreviousLogContext;
}

void LogContextOverride::init(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
    const char8_t* personaName, uint32_t clientAddr, AccountId nucleusAccountId, ClientPlatformType platform)
{
    if (gLogContext != nullptr)
        mPreviousLogContext = *gLogContext;

    gLogContext->set(slaveSessionId, userSessionId, blazeId, deviceId, personaName, clientAddr, nucleusAccountId, platform);
}


Logger::ThisFunction::ThisFunction(const char8_t *functionName, const void *thisPtr)
{
    if (functionName == nullptr)
    {
        return;
    }

    // Function parsing logic:
    // Need to handle basic cases like "int class::func(int&) const",  (Output "class::func")
    // and stupid complex cases like "typename T_CommandStub::Errors Blaze::CareerMode::WorkerRequestResponseCommand<T_CommandStub, T_Request, T_Response, checkAuth, loadInstance>::execute() [with T_CommandStub = Blaze::CareerMode::DepthChartFlow_RefreshFormCommandStub; T_Request = Blaze::FranTkMode::RefreshFormRequest; T_Response = Blaze::FranTkMode::RefreshFormResponse; bool checkAuth = true; bool loadInstance = true; typename T_CommandStub::Errors = Blaze::CareerMode::DepthChartFlow_RefreshFormError::Error]"
    //   (Output "WorkerRequestResponseCommand::execute")
    // Basic logic is as follows:
    // Start from the end of the functionName, 
    // Move back until you hit '(', indicating the function signature's end
    // Move back until you hit ':', indicating the function signature's start (and either a template or class start)
    // Move back, until you hit ' ' or ':', indicating the class name's start (if '>' is found, skip everything until '<' is found)

    int32_t funcNameStart = 0;
    int32_t funcNameEnd = 0;
    int32_t classNameStart = 0;
    int32_t classNameEnd = 0;

    bool copyingClassName = false;
    bool copyingFuncName = false;
    int templateDepth = 0;
    int32_t functionNameLen = (int32_t)strlen(functionName);
    for (int32_t riter = functionNameLen - 1; riter >= 0; --riter)
    {
        switch (functionName[riter])
        {
        case '(':
            if (funcNameEnd == 0)
            {
                copyingFuncName = true;
                funcNameEnd = riter;
            }
            break;

        case ' ':
        case ':':
            if (copyingFuncName)
            {
                copyingFuncName = false;
                copyingClassName = true;
                funcNameStart = riter + 1;  // Start after ':'
                if (functionName[riter] == ':')
                    --riter; // skip the next  ':'
                classNameEnd = riter;   // Assumes '::' or ' '
            }
            else if (copyingClassName && templateDepth == 0)
            {
                copyingClassName = false;
                classNameStart = riter + 1; // Start after ':' or ' '
            }
            break;
        case '>':
            ++templateDepth;
            break;
        case '<':
            --templateDepth;
            if (copyingClassName && templateDepth == 0)
            {
                // If templates were involved, skip 'em:
                classNameEnd = riter;   // Assumes '<'
            }
            break;
        }

        if (funcNameStart && classNameStart)
            break;
    }

    StringBuilder& nameBuf = ::Blaze::Logger::getThisFunctionLoggingMessage();
    nameBuf.reset();
    nameBuf.append("[");
    if (classNameEnd - classNameStart > 0)
        nameBuf.appendN(functionName + classNameStart, (classNameEnd - classNameStart));
    nameBuf.append(":%p].", thisPtr);
    if (funcNameEnd - funcNameStart > 0)
        nameBuf.appendN(functionName + funcNameStart, (funcNameEnd - funcNameStart));
    nameBuf.append(": ");
}


bool Logger::onAssertFailure(const char* expr, const char* filename, int line, const char* function, const char* msg, va_list args)
{
    CallStack cs;
    cs.getStack(CallStack::EA_ASSERT_FRAMES_TO_IGNORE); // Ignore stack frames belonging to the EA::Assert code

    eastl::string backtrace;
    backtrace.reserve(2048);
    cs.getSymbols(backtrace);

    char8_t buf[4096];
    blaze_vsnzprintf(buf, sizeof(buf), msg, args);
    BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].onAssertFailure: ASSERTION FAILED! message(" << buf << "), expression(" << expr << "), file ("
        << filename << ":" << line << "), function (" << function << ") ->\n" << backtrace.c_str());

    return gProcessController != nullptr ? gProcessController->getBootConfigTdf().getBreakOnAssert() : true;
}

uint32_t Logger::getDbId()
{
    DbIdByPlatformTypeMap::const_iterator itr = mDbIdByPlatformTypeMap.find(gController->getDefaultClientPlatform());
    if (itr == mDbIdByPlatformTypeMap.end())
        return DbScheduler::INVALID_DB_ID;
    
    return itr->second;
}

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
\brief Logger constructor

Initializes the logger level to DEBUG and appender to console.

\param[in]  name - <Description of variable>

\return - <Description of return value - Delete this line if returning void>

\notes
<Any additional detail including references.  Delete this section if
there are no notes.>
*/
/*************************************************************************************************/

BlazeRpcError Logger::initialize(Logging::Level level, const char8_t* logDir, const char8_t* name, LoggingConfig::Output logOutput)
{
    if (!Fiber::getFiberStorageInitialized())
    {
        // Logger makes use of per/thread fiber specific variables, we need 
        // to initialize them here in order for assertion logging to work
        EA_FAIL_MSG("Logger requires fiber storage.");
        return ERR_SYSTEM;
    }

    if (logDir == nullptr)
        mLogDir[0] = '\0';
    else
    {
        blaze_strnzcpy(mLogDir, logDir, sizeof(mLogDir));
        EA::IO::Directory::EnsureExists(eastl::string(eastl::string::CtorSprintf(), "%s" EA_FILE_PATH_SEPARATOR_STRING_8, mLogDir).c_str());
    }

    // Set the prefix for all log files.  If this is empty, then logging to file will be disabled.
    blaze_strnzcpy(mName, (name ? name : ""), sizeof(mName));

    setLogOutputMode(logOutput);

    mOutputter = BLAZE_NEW_LOG LogOutputter;

    bool categoriesRegistered = true;
    categoriesRegistered = categoriesRegistered && registerCategory(Log::CONFIG, "config");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::CONNECTION, "connection");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::DB, "database");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::FIRE, "fire");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::HTTPXML, "httpxml");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::HTTP, "http");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::REPLICATION, "replication");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::SYSTEM, "system");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::USER, "user");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::CONTROLLER, "controller");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::UTIL, "fw_util");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::PROTOCOL, "protocol");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::METRICS, "metrics");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::REST, "rest");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::REDIS, "redis");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::VAULT, "vault");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::GRPC, "grpc");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::GRPC_CORE, "grpc_core");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::PROTOBUF, "protobuf");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::PINEVENT, "pinevent", BLAZE_LOG_PIN_EVENTS_DIR);
    categoriesRegistered = categoriesRegistered && registerCategory(Log::DYNAMIC_INET, "dynamic_inet");
    categoriesRegistered = categoriesRegistered && registerCategory(Log::OAUTH, "fw_oauth");

    // Setup log defaults
    mLevel = static_cast<Logging::Level>(level | (level - 1));
    mIncludeLocation = false;
    mEncodeLocation = false;
    mMaxCategoryLength = 16;
    mCoreOnAssertLog = false;
    for (int32_t idx = 0; idx < TRACE_IDX_MAX; ++idx)
        mTrace[idx] = TRACE_NONE;

    resetLogLevels();

    initializeCategoryDefaults();

    EA::Thread::ThreadId threadId = mOutputter->start();
    if (threadId == EA::Thread::kThreadIdInvalid)
    {
        delete mOutputter;
        mOutputter = nullptr;
        return ERR_SYSTEM;
    }
    else
    {
        // Register Logger's assertion callback (enables printing backtrace to log)
        EA::Assert::SetFailureCallback(&Logger::onAssertFailure);

        if (!categoriesRegistered)
            BLAZE_FAIL_LOG(Log::SYSTEM, "[Logger::registerCategory]: Failure to register all logging categories.");
    }

    mExternalThreadLogOutputter = BLAZE_NEW_LOG ExternalThreadLogOutputter;
    EA::Thread::ThreadId exLoggerThreadId = mExternalThreadLogOutputter->start();
    if (exLoggerThreadId == EA::Thread::kThreadIdInvalid)
    {
        delete mExternalThreadLogOutputter;
        mExternalThreadLogOutputter = nullptr;

        delete mOutputter;
        mOutputter = nullptr;

        return ERR_SYSTEM;
    }

    mIsInitialized = true;
    return ERR_OK;
}

void Logger::setLogOutputMode(LoggingConfig::Output logOutputMode)
{
    // Set the initial default state by turning things ON by default.
    mMainLogToFile = mOtherLogsToFile = (mName[0] != '\0');
    mMainLogToStdout = true;

    // Then, selectively turn things OFF based on the chosen mode.
    switch (logOutputMode)
    {
    case LoggingConfig::BOTH:
        // Accepts the defaults that were set above. Note that file logging will only happen if a log name was provided.
        break;
    case LoggingConfig::FILE:
        // This really just means don't write the main log to stdout. Writing to file will be enabled if a log name was provided.
        mMainLogToStdout = false;
        break;
    case LoggingConfig::STDOUT:
        // This really just means don't write the main log to file, even if a log name was provided.
        mMainLogToFile = false;
        break;
    }

    if (!mMainLogToFile && (logOutputMode == LoggingConfig::FILE))
    {
        eastl::string errorLog;
        errorLog.sprintf("Warning: Main server logging is disabled.  The --logname cmd-line arg was not specified, but the log output mode is set to FILE. "
            "See ./etc/logging.cfg for details on the output mode setting (--logdir=%s, --logname=%s, --logoutput=%s). Debuggers will always receive main server log output.\n",
            mLogDir, mName, LoggingConfig::OutputToString(logOutputMode));
        EA::StdC::Fprintf(stderr, errorLog.c_str());
    }
}

void Logger::getAuditLogFilename(AuditId auditId, eastl::string& outString)
{
    EA::Thread::AutoRWMutex readLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeRead);
    auto logEntryItr = mAuditLogEntryByAuditId.find(auditId);
    if (logEntryItr != mAuditLogEntryByAuditId.end())
    {
        outString.assign(logEntryItr->second->getFilename());
    }
}


void Logger::initializeCategoryDefaults()
{
    for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
    {
        mCategories[idx].mLevel = mLevel;
        for (int32_t traceIdx = 0; traceIdx < TRACE_IDX_MAX; ++traceIdx)
        {
            if (mTrace[traceIdx] == TRACE_ALL)
            {
                mCategories[idx].mLevel = static_cast<Logging::Level>(
                        mCategories[idx].mLevel | sTraceInfo[traceIdx].level);
            }
        }
    }
}

void Logger::destroy()
{
    mIsInitialized = false;

    if (mExternalThreadLogOutputter != nullptr)
    {
        mExternalThreadLogOutputter->stop();
        delete mExternalThreadLogOutputter;
        mExternalThreadLogOutputter = nullptr;
    }

    if (mOutputter != nullptr)
    {
        // Revert back to old callback since Logger assert handler is no longer useful
        EA::Assert::SetFailureCallback(sOldAssertFailureCallback);
        mOutputter->stop();
        delete mOutputter;
        mOutputter = nullptr;
    }
    mConfig = nullptr;
}

void Logger::log(int32_t category, Logging::Level level, const char * file, int line,
        const char* message, ...)
{
    va_list args;
    va_start(args, message);
    mOutputter->log_va_list(category, level, file, line, true, message, args);
    va_end(args);
}

void Logger::log(int32_t category, Logging::Level level, const char * file, int line,
        StringBuilder &message)
{
    mOutputter->logf(category, level, file, line, false, message.get());
}

void Logger::_log(int32_t category, Logging::Level level, const char* file, int32_t line,
        const char* message, va_list args)
{
    mOutputter->log_va_list(category, level, file, line, true, message, args);
}

void Logger::logEvent(int32_t category, const char8_t* event, size_t size)
{
    mOutputter->logEvent(category, event, size);
}

void Logger::logTdf(int32_t category, const EA::TDF::Tdf& tdf)
{
    mOutputter->logTdf(category, tdf);
}

void Logger::logExternalThreadLogEntry(const ExternalThreadLogEntry& entry)
{
    if (mExternalThreadLogOutputter != nullptr)
        mExternalThreadLogOutputter->pushLogEntry(entry);
}

bool Logger::isLogEventEnabled(int32_t category)
{
    return mOutputter->isEventEnabled(category);
}

bool Logger::isLogBinaryEventEnabled(int32_t category)
{
    return mOutputter->isBinaryEventEnabled(category);
}

bool Logger::configure(const LoggingConfig &config, bool reconfigure)
{
    mConfigLock.Lock();
    if (!mConfig || !mConfig->equalsValue(config))
    {
        mConfig = config.clone();

        setLogOutputMode(config.getOutput());

        mRotationFileSize = config.getRotationFileSize();
        mLargeEntrySize = config.getLargeEntrySize();
        mLargeEntryCount = config.getLargeEntryCount();

        const Logging::Level level = config.getLevel();
        mLevel = static_cast<Logging::Level>(level | (level - 1));

        const TraceMap &trace = config.getTrace();
        TraceMap::const_iterator traceIt = trace.begin();
        for (; traceIt != trace.end(); ++traceIt)
        {
            for (int32_t idx = 0; idx < TRACE_IDX_MAX; ++idx)
            {
                if (blaze_stricmp(sTraceInfo[idx].name, traceIt->first) == 0)
                {
                    if (blaze_stricmp(traceIt->second, TRACE_ALL_STR) == 0)
                    {
                        mTrace[idx] = TRACE_ALL;
                        mLevel = static_cast<Logging::Level>(mLevel | sTraceInfo[idx].level);
                    }
                    else if (blaze_stricmp(traceIt->second, TRACE_CATEGORY_STR) == 0)
                    {
                        mTrace[idx] = TRACE_CATEGORY;
                    }
                    else
                    {
                        mTrace[idx] = TRACE_NONE;
                    }
                    break;
                }
            }
        }

        initializeCategoryDefaults();

        mIncludeLocation = config.getIncludeLocation();
        mEncodeLocation = config.getEncodeLocation();
        mCoreOnAssertLog = config.getCoreOnAssertLog();
        mSuppressAuditLogging = config.getSuppressAuditLogging();
        mSuppressDebugConsoleLogging = config.getSuppressDebugConsoleLogging();

        if (config.getMaxCategoryLength() > 0)
            mMaxCategoryLength = config.getMaxCategoryLength();

        mOutputter->setPrefixMultilineEnabled(config.getPrefixMultilineEnabled());
        mOutputter->setAuditLogRefreshInterval(config.getAuditLogRefreshInterval());

        Logging::Level grpcCoreLoggingLevel = level; // Do not use mLevel as that is ORed with lower levels.
        
        // Load per-category log levels
        for(auto& entry : config.getCategories())
        {
            if (!initializeCategoryLevel(entry.first, entry.second))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].initialize: Ignoring unrecognized logging category specified: '" << entry.first.c_str() << "'");
            }

            if (blaze_stricmp(entry.first, "grpc_core") == 0)
                grpcCoreLoggingLevel = entry.second;
        }

        configureGrpcCoreLogging(grpcCoreLoggingLevel, config.getGrpcCoreTracers());
        google::protobuf::SetLogHandler(Blaze::protobufLogger);

        // Determine which components events need to be logged for
        mOutputter->configureEventLogging(LogOutputter::EVENT_LOG_TYPE_TEXT, config.getEvents());
        mOutputter->configureEventLogging(LogOutputter::EVENT_LOG_TYPE_BINARY, config.getBinaryEvents());
        mOutputter->configureBehaviours(config);

        if (reconfigure)
            mOutputter->triggerReconfigure();
    }
    else
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[Logger].config: Logging configuration has not changed.");
    }

    mConfigLock.Unlock();
    return true;
}

BlazeRpcError Logger::validateAuditLogEntry(const AuditLogEntry& entry, bool unrestricted)
{
    if (!entry.getIpAddressAsTdfString().empty())
    {
        InetFilter filter;
        NetworkFilterConfig c;
        c.push_back(entry.getIpAddress());
        if (!filter.initialize(false, c))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].validateAuditLogEntry: Unable to parse ip address: " << entry.getIpAddress());
            return ERR_SYSTEM;
        }
        if (unrestricted)
            return ERR_OK;

        // If the caller doesn't have unrestricted permission to add audit log entries, then a maximum ip
        // range of 4096 addresses is allowed when ip and/or platform are the only fields specified.
        //
        // Note that it's possible for callers without the unrestricted permission to circumvent this check
        // by splitting up a large ip range across multiple AuditLogEntrys, or by specifying the ip range using
        // a DynamicInetFilter group instead of an ip filter. The check is intended to help catch common user
        // errors (e.g. WWCE user forgetting to fill in a field in an audit logging form); it is not a comprehensive
        // defense against audit logging abuse.

        if (filter.getFilters().empty()) // In the (very unlikely) case that ip range was specified by DynamicInetFilter group,
            return ERR_OK;               // we trust that it's narrow enough

        const InetFilter::Filter& curFilter = filter.getFilters().back();
        if (curFilter.mask >= MIN_SUBNET_MASK)
            return ERR_OK;
    }

    if (unrestricted)
        return ERR_OK;

    if (entry.getBlazeId() != INVALID_BLAZE_ID)
        return ERR_OK;
    if (!entry.getDeviceIdAsTdfString().empty())
        return ERR_OK;
    if (!entry.getPersonaAsTdfString().empty())
        return ERR_OK;
    if (entry.getNucleusAccountId() != INVALID_ACCOUNT_ID)
        return ERR_OK;

    eastl::string auditLogStr;
    if (!entry.getIpAddressAsTdfString().empty())
        auditLogStr.append_sprintf("in ip range %s ", entry.getIpAddress());
    if (entry.getPlatform() == INVALID)
        auditLogStr.append("across all platforms");
    else
        auditLogStr.append_sprintf("on platform '%s'", ClientPlatformTypeToString(entry.getPlatform()));
    BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].validateAuditLogEntry: User does not have permission to enable audit logging for all users " << auditLogStr.c_str() << " (request is too broad).");

    return ERR_AUTHORIZATION_REQUIRED;
}

BlazeRpcError Logger::addAudit(const UpdateAuditLoggingRequest& request, bool unrestricted)
{
    BlazeRpcError rc = ERR_OK;
    if (request.getAuditLogEntries().empty())
        return ERR_OK;

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId(), Log::SYSTEM);
    if (dbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        DbResultPtr res;
        query->append("INSERT INTO `log_audits` (`blazeid`, `deviceid`, `ip`, `persona`, `accountid`, `platform`, `filename`) VALUES ");
        for (auto& entry : request.getAuditLogEntries())
        {
            rc = validateAuditLogEntry(*entry, unrestricted);
            if (rc == ERR_AUTHORIZATION_REQUIRED)
                return rc;
            else if (rc != ERR_OK)
                continue;

            query->append("($D, ", entry->getBlazeId());
            query->append("'$s', ", entry->getDeviceId());
            query->append("'$s', ", entry->getIpAddress());
            query->append("'$s', ", entry->getPersona());
            query->append("$D, ", entry->getNucleusAccountId());
            query->append("'$s', ", (entry->getPlatform() == INVALID ? "" : ClientPlatformTypeToString(entry->getPlatform())));

            if (entry->getFilename()[0] != '\0')
                query->append("'$s')", entry->getFilename());
            else
            {
                StringBuilder sb;
                sb.append("audit");
                if (entry->getBlazeId() != INVALID_BLAZE_ID)
                    sb.append("_%" PRId64, entry->getBlazeId());
                if (entry->getPersona()[0] != '\0')
                    sb.append("_%s", entry->getPersona());
                if (entry->getDeviceId()[0] != '\0')
                    sb.append("_%s", entry->getDeviceId());
                if (entry->getIpAddress()[0] != '\0')
                    sb.append("_%s", entry->getIpAddress());
                if (entry->getNucleusAccountId() != INVALID_ACCOUNT_ID)
                    sb.append("_%" PRId64, entry->getNucleusAccountId());
                if (entry->getPlatform() != INVALID)
                    sb.append("_%s", ClientPlatformTypeToString(entry->getPlatform()));

                // Need to sanitize. IP address could be a subnet mask (e.g., 10.10.0.0/16), Persona name may contain illegal characters too with the addition of unicode 4byte char support.
                eastl::string queryString(sb.get());
                eastl::string illegalChars("\\/:?\"<>|*");
                for (auto& c : queryString) {
                    bool found = illegalChars.find(c) != eastl::string::npos;
                    if (found) {
                        c = '_';
                    }
                }

                query->append("'$s')", queryString.c_str());
            }
            query->append(",");
        }
        query->trim(1);
        query->append(" ON DUPLICATE KEY UPDATE `filename`=VALUES(`filename`)");
        rc = dbConn->executeQuery(query, res);
        if (rc == ERR_OK)
        {
            if (res->getAffectedRowCount() > 0)
                gController->sendNotifyUpdateAuditStateToAllSlaves();
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].addAudit: Failed to execute query with error " << ErrorHelp::getErrorName(rc));
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].addAudit: Failed to obtain DB connection.");
        rc = DB_ERR_SYSTEM;
    }

    return rc;
}

BlazeRpcError Logger::removeAudit(const UpdateAuditLoggingRequest& request)
{
    BlazeRpcError rc = ERR_OK;
    if (request.getAuditLogEntries().empty())
        return rc;

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId(), Log::SYSTEM);
    if (dbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        DbResultPtr res;
        query->append("DELETE FROM log_audits WHERE ");
        
        for (auto& entry : request.getAuditLogEntries())
        {
            query->append("(`blazeid` = $D AND `deviceid` = '$s' AND `ip` = '$s' AND `persona` = '$s' AND `accountid` = $D AND `platform` = '$s') OR ",
                entry->getBlazeId(), entry->getDeviceId(), entry->getIpAddress(), entry->getPersona(), entry->getNucleusAccountId(), (entry->getPlatform() == INVALID ? "" : ClientPlatformTypeToString(entry->getPlatform())));
        }

        query->trim(4);
        rc = dbConn->executeQuery(query, res);
        if (rc == ERR_OK)
        {
            if (res->getAffectedRowCount() > 0)
                gController->sendNotifyUpdateAuditStateToAllSlaves();
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].removeAudit: Failed to execute query with error " << ErrorHelp::getErrorName(rc));
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].removeAudit: Failed to obtain DB connection.");
        rc = DB_ERR_SYSTEM;
    }

    return rc;
}

BlazeRpcError Logger::updateAudits()
{
    BlazeRpcError rc = ERR_OK;
    decltype(mAuditLogEntryByAuditId) auditLogEntryByAuditId(BlazeStlAllocator("Logger::mAuditLogEntryByAuditId", Blaze::MEM_GROUP_FRAMEWORK_DEFAULT));
    decltype(mAuditLogEntries) auditLogEntries(*::Blaze::Allocator::getAllocator(Blaze::MEM_GROUP_FRAMEWORK_DEFAULT), "Logger::mAuditLogEntries");
    {
        // NOTE: This scope intentionally separates db conn lifetime from the RW mutex lock below because we don't want to hold the write lock longer than necessary. It is assumed that concurrently triggered updateAudits() operations shall not guarantee that mutations to log_audits occurring during this process are guaranteed to be applied in order.
        DbConnPtr dbConn = gDbScheduler->getLagFreeReadConnPtr(getDbId(), Log::SYSTEM);
        if (dbConn != nullptr)
        {
            DbResultPtr dbResults;
            QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
            query->append("SELECT * FROM log_audits");
            rc = dbConn->executeQuery(query, dbResults);
            if (rc == ERR_OK)
            {
                auditLogEntries.reserve(dbResults->size());
                for (auto& row : *dbResults)
                {
                    auto entry = auditLogEntries.pull_back();
                    uint32_t col = 0;
                    ClientPlatformType platform = INVALID;
                    uint64_t auditId = row->getUInt64(col++);
                    entry->setAuditId(auditId);
                    entry->setBlazeId(row->getInt64(col++));
                    entry->setDeviceId(row->getString(col++));
                    entry->setIpAddress(row->getString(col++));
                    entry->setPersona(row->getString(col++));
                    entry->setNucleusAccountId(row->getInt64(col++));
                    ParseClientPlatformType(row->getString(col++), platform);
                    entry->setPlatform(platform);
                    entry->setFilename(row->getString(col++));
                    auditLogEntryByAuditId.emplace(entry->getAuditId(), entry);
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].updateAudits: Failed to execute query with error " << ErrorHelp::getErrorDescription(rc));
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].updateAudits: Failed to obtain DB connection.");
            rc = DB_ERR_SYSTEM;
        }
    }

    {
        EA::Thread::AutoRWMutex writeLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeWrite);
        auditLogEntries.swap(mAuditLogEntries);
        auditLogEntryByAuditId.swap(mAuditLogEntryByAuditId);
    }

    return rc;
}

BlazeRpcError Logger::expireAudits(const TimeValue& expiryTime)
{
    BlazeRpcError rc = ERR_OK;
    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId(), Log::SYSTEM);
    if (dbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        DbResultPtr res;
        query->append("DELETE FROM `log_audits` WHERE `deviceid` != '' AND `timestamp` < DATE_SUB(NOW(), INTERVAL $U SECOND);", expiryTime.getSec());

        rc = dbConn->executeQuery(query, res);
        if (rc == ERR_OK)
        {
            if (res->getAffectedRowCount() > 0)
                gController->sendNotifyUpdateAuditStateToAllSlaves();
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].expireAudits: Failed to execute query with error " << ErrorHelp::getErrorName(rc));
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].expireAudits: Failed to obtain DB connection.");
        rc = ERR_SYSTEM;
    }

    return rc;
}

bool Logger::isAuditLoggingSuppressed()
{
    return mSuppressAuditLogging;
}

uint32_t Logger::getNumAuditEntries()
{
    EA::Thread::AutoRWMutex readLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeRead);
    return (uint32_t) mAuditLogEntries.size();
}

void Logger::getAudits(AuditLogEntryList& list)
{
    EA::Thread::AutoRWMutex readLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeRead);
    mAuditLogEntries.copyInto(list);
}

bool Logger::auditLogEntryIsMatch(const AuditLogEntry& entry, BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform)
{
    if (entry.getBlazeId() != INVALID_BLAZE_ID)
    {
        if (id != entry.getBlazeId())
            return false;
    }
    if (!entry.getDeviceIdAsTdfString().empty())
    {
        if (entry.getDeviceIdAsTdfString() != deviceId)
            return false;
    }
    if (!entry.getIpAddressAsTdfString().empty())
    {
        InetFilter filter;
        NetworkFilterConfig c;
        c.push_back(entry.getIpAddress());
        filter.initialize(false, c);
        if (!filter.match(addr))
            return false;
    }
    if (!entry.getPersonaAsTdfString().empty())
    {
        if (entry.getPersonaAsTdfString() != persona)
            return false;
    }
    if (entry.getNucleusAccountId() != INVALID_ACCOUNT_ID)
    {
        if (nucleusAccountId != entry.getNucleusAccountId())
            return false;
    }
    if (entry.getPlatform() != INVALID)
    {
        if (entry.getPlatform() != platform)
            return false;
    }
    return true;
}

bool Logger::shouldAudit(BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform)
{
    EA::Thread::AutoRWMutex readLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeRead);
    for (const auto& entry : mAuditLogEntries)
    {
        if (auditLogEntryIsMatch(*entry, id, deviceId, addr, persona, nucleusAccountId, platform))
            return true;
    }

    return false;
}

bool Logger::shouldAudit(BlazeId id, const char8_t* deviceId, uint32_t addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform)
{
    InetAddress ip(addr, 0, InetAddress::HOST);
    return shouldAudit(id, deviceId, ip, persona, nucleusAccountId, platform);
}

void Logger::getAuditIds(BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform, AuditIdSet& auditIds)
{
    EA::Thread::AutoRWMutex readLock(mUpdateAuditEntriesLock, EA::Thread::RWMutex::kLockTypeRead);
    for (const auto& entry : mAuditLogEntries)
    {
        if (auditLogEntryIsMatch(*entry, id, deviceId, addr, persona, nucleusAccountId, platform))
            auditIds.insert(entry->getAuditId());
    }
}

void Logger::getAuditIds(BlazeId id, const char8_t* deviceId, uint32_t addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform, AuditIdSet& auditIds)
{
    InetAddress ip(addr, 0, InetAddress::HOST);
    getAuditIds(id, deviceId, ip, persona, nucleusAccountId, platform, auditIds);
}

/*** Private Methods *****************************************************************************/

bool Logger::registerCategory(Log::Category category, const char8_t* logFileSubName, const char8_t* logFileSubFolderName)
{
    if ((category >= 0) && (category < Log::CATEGORY_MAX))
    {
        if (mCategories[category].mName[0] != '\0')
        {
            BLAZE_FAIL_LOG(Log::SYSTEM, "[Logger::registerCategory]: Category id " << category << " is already in use with name " << mCategories[category].mName
                << " and cannot be used to register category " << logFileSubName);
            return false;
        }

        // Check to ensure that there isn't already a category registered with this name
        for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
        {
            if (strcmp(mCategories[idx].mName, logFileSubName) == 0)
            {
                BLAZE_FAIL_LOG(Log::SYSTEM, "[Logger::registerCategory]: Category name conflict ('" << logFileSubName << "'); conflicting IDs are " << idx << " and " << category);
                return false;
            }
        }

        blaze_strnzcpy(mCategories[category].mName, logFileSubName, sizeof(mCategories[category].mName));

        if (mOutputter != nullptr)
        {
            mOutputter->registerCategory(category, logFileSubName, logFileSubFolderName);
        }
        return true;
    }
    else
    {
        BLAZE_FAIL_LOG(Log::SYSTEM, "[Logger::registerCategory]: Unknown category id - category = " << category << ", name = " << logFileSubName)
    }
    return false;
}



Logger::CategoryNameList* Logger::getRegisteredCategoryNames()
{
    CategoryNameList* names = BLAZE_NEW_LOG CategoryNameList;
    for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
    {
        if (mCategories[idx].mName[0] != '\0')
            names->push_back(mCategories[idx].mName);
    }
    return names;
}

bool Logger::initializeCategoryLevel(const char8_t* name, Logging::Level level)
{
    for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
    {
        if (blaze_stricmp(name, mCategories[idx].mName) == 0)
        {
            mCategories[idx].mLevel = static_cast<Logging::Level>(level | (level - 1));
            for (int32_t traceIdx = 0; traceIdx < TRACE_IDX_MAX; ++traceIdx)
            {
                if ((mTrace[traceIdx] == TRACE_CATEGORY) || (mTrace[traceIdx] == TRACE_ALL))
                {
                    mCategories[idx].mLevel = static_cast<Logging::Level>(
                            mCategories[idx].mLevel | sTraceInfo[traceIdx].level);
                }
            }
            
            return true;
        }
    }
    return false;
}

void Logger::configureGrpcCoreLogging(Logging::Level level, const Blaze::LoggingConfig::StringGrpcCoreTracersList& grpcCoreTracers)
{
    grpc_tracer_set_enabled("all", 0); // Disable current tracers first (in case, we are reconfiguring).
    
    if (level != Logging::Level::NONE)
    {
        gpr_set_log_function(grpcCoreLogger);
        
        for (auto& tracer : grpcCoreTracers)
            grpc_tracer_set_enabled(tracer.c_str(), 1);

        if (level >= Logging::Level::TRACE)
            gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
        else if (level >= Logging::Level::INFO)
            gpr_set_log_verbosity(GPR_LOG_SEVERITY_INFO);
        else
            gpr_set_log_verbosity(GPR_LOG_SEVERITY_ERROR);
    }
}

void Logger::remapLogLevels()
{
    mRemapLogLevels = true;    
}

void Logger::resetLogLevels()
{
    mRemapLogLevels = false;
}

//Remapping FATAL logs as WARN when server is shutting down
Logging::Level Logger::getRemappedLevel(Logging::Level level)
{
    if (level != Logging::FATAL || !isRemapLogLevels())
    {
        return level;
    }

    return Logging::WARN;
}

int32_t BlazeRpcLog::getLogCategory(ComponentId componentId)
{
    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        return info->baseInfo.index;
    }

    return Log::SYSTEM;
}

bool Logger::isEnabled(int32_t category, int32_t level, const char8_t* file, int32_t line)
{
    if (!mIsInitialized)
        return false;

    return (isEnabledWithoutContext(category, level)
        || EA_UNLIKELY(gLogContext->isAudited())
        || EA_UNLIKELY(mOutputter->shouldLog(file, line)));
}

bool Logger::isEnabledWithoutContext(int32_t category, int32_t level)
{
    if (!mIsInitialized)
        return false;

    return ((((uint32_t)level) & ((uint32_t)mCategories[category].mLevel)) != 0);
}

void Logger::getStatus(LoggerMetrics& metrics)
{
    mOutputter->getStatus(metrics);
}

void Logger::initializeThreadLocal()
{
    mOutputter->initializeThreadLocal();
}

void Logger::destroyThreadLocal()
{
    mOutputter->destroyThreadLocal();
}

} // Blaze

void BlazePrintConfigErrorLog(const char8_t* message)
{
    BLAZE_ERR_LOG(Blaze::Log::CONFIG, message);
}
