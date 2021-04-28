/*************************************************************************************************/
/*!
    \file logger.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef _LOG_LOGGER_H
#define _LOG_LOGGER_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"
#include "framework/util/shared/stringbuilder.h"
#include "framework/util/shared/blazestring.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "eathread/eathread_storage.h"
#include "eathread/eathread_mutex.h"

#include "framework/system/fiber.h"
#include "framework/system/fiberlocal.h"
#include "EATDF/tdfobjectid.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef uint64_t SlaveSessionId;
typedef uint64_t UserSessionId;
typedef uint32_t ConnectionId;
typedef uint64_t AuditId;
typedef eastl::hash_set<AuditId> AuditIdSet;

class LogOutputter;
class ExternalThreadLogOutputter;
struct ExternalThreadLogEntry;
class LoggingConfig;
class LoggerMetrics;

class DbResultBase;
typedef FrameworkResourcePtr<DbResultBase> DbResultPtr;

// This class wraps the LogContext TDF
// For performance reasons, we define a copy ctor and assignment operator
// rather than using the TDF copyInfo method.
// This wrapper also acts as an adapter for the old LogContext interface
class LogContextWrapper : public Logging::LogContext
{
public:
    LogContextWrapper() {}
    LogContextWrapper(const LogContextWrapper& rhs);
    LogContextWrapper& operator=(const LogContextWrapper& rhs);

    void set(SlaveSessionId slaveSessionId, BlazeId blazeId);
    void set(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
        const char8_t* personaName, uint32_t clientAddr, AccountId nucleusAccountId, ClientPlatformType platform);
    void clear();

    bool isAudited() const { return !mAuditIds.empty();  }
    const AuditIdSet& getAuditIds() const { return mAuditIds; }

private:
    AuditIdSet mAuditIds;
};

extern FiberLocalManagedPtr<LogContextWrapper> gLogContext;

struct LogContextOverride
{
public:
    explicit LogContextOverride(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
        const char8_t* personaName, const InetAddress& clientAddr, AccountId nucleusAccountId, ClientPlatformType platform);
    explicit LogContextOverride(BlazeId blazeId, const char8_t* deviceId, const char8_t* personaName, const InetAddress& clientAddr, AccountId nucleusAccountId, ClientPlatformType platform);
    explicit LogContextOverride(UserSessionId userSessionId);
    explicit LogContextOverride(const LogContextWrapper& newLogContext);
    ~LogContextOverride();

private:
    void init(SlaveSessionId slaveSessionId, UserSessionId userSessionId, BlazeId blazeId, const char8_t* deviceId,
        const char8_t* personaName, uint32_t clientAddr, AccountId nucleusAccountId, ClientPlatformType platform);

    LogContextWrapper mPreviousLogContext;
};

class Log
{
public:
    typedef int32_t Category;

    // Component category range
    static const Category COMPONENT_START = 0;
    static const Category COMPONENT_END = 100;

    // Framework categories
    static const Category CONFIG      = (COMPONENT_END + 0);
    static const Category CONNECTION  = (COMPONENT_END + 1);
    static const Category DB          = (COMPONENT_END + 2);    
    static const Category FIRE        = (COMPONENT_END + 4);
    static const Category HTTPXML     = (COMPONENT_END + 5);
    static const Category HTTP        = (COMPONENT_END + 6);
    static const Category REPLICATION = (COMPONENT_END + 7);
    static const Category SYSTEM      = (COMPONENT_END + 8);
    static const Category USER        = (COMPONENT_END + 9);
    static const Category CONTROLLER  = (COMPONENT_END + 11);
    static const Category UTIL        = (COMPONENT_END + 12);
    static const Category PROTOCOL    = (COMPONENT_END + 14);
    static const Category METRICS     = (COMPONENT_END + 15);
    static const Category REST        = (COMPONENT_END + 17);
    static const Category REDIS       = (COMPONENT_END + 18);
    static const Category VAULT       = (COMPONENT_END + 19);
    static const Category GRPC        = (COMPONENT_END + 20);
    static const Category GRPC_CORE   = (COMPONENT_END + 21);
    static const Category PROTOBUF    = (COMPONENT_END + 22);
    static const Category DYNAMIC_INET= (COMPONENT_END + 23);
    static const Category OAUTH       = (COMPONENT_END + 24);
    static const Category PINEVENT    = (COMPONENT_END + 25);

    //NOTE: Increase this if we ever have change framework log categories above
    static const Category FRAMEWORK_END = (COMPONENT_END + 26);

    // NOTE: When adding new framework log categories remember to
    // call registerCategory() from within ::Blaze::Logger::initialize()!
    
    // Maximum number of logger categories
    static const Category CATEGORY_MAX = FRAMEWORK_END;
};


class Logger
{
    NON_COPYABLE(Logger);

public:

    static const char8_t* BLAZE_LOG_FILE_EXTENSION;
    static const char8_t* BLAZE_LOG_BINARY_EVENTS_DIR;
    static const char8_t* BLAZE_LOG_PIN_EVENTS_DIR;

    class ThisFunction 
    { 
        public: 
            ThisFunction(const char8_t *functionName, const void *thisPtr);
            const char8_t *get() { return ::Blaze::Logger::getThisFunctionLoggingMessage().get(); } 
    }; 

    static BlazeRpcError initialize(::Blaze::Logging::Level level = ::Blaze::Logging::INFO, const char8_t* logDir = nullptr, const char8_t* name = nullptr, LoggingConfig::Output logOutput = LoggingConfig::FILE);
    static void destroy();
    static void initializeThreadLocal();
    static void destroyThreadLocal();
    static bool configure(const LoggingConfig& config, bool reconfigure);
    static void setLevel(::Blaze::Logging::Level level) { mLevel = static_cast<::Blaze::Logging::Level>(level | (level - 1)); }
    static void getAuditLogFilename(AuditId auditId, eastl::string& outString);
    static void setLogOutputMode(LoggingConfig::Output logOutputMode);

    static uint16_t getDbSchemaVersion() { return 4; }
    static const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() { return &mConfig->getDbNamesByPlatform(); }
    static DbIdByPlatformTypeMap& getDbIdByPlatformTypeMap() { return mDbIdByPlatformTypeMap; }
    static uint32_t getDbId();

    static BlazeRpcError addAudit(const UpdateAuditLoggingRequest& request, bool unrestricted);
    static BlazeRpcError removeAudit(const UpdateAuditLoggingRequest& request);
    static BlazeRpcError updateAudits();
    static BlazeRpcError expireAudits(const EA::TDF::TimeValue& expiryTime);

    static bool isAuditLoggingSuppressed();
    static uint32_t getNumAuditEntries();
    static void getAudits(AuditLogEntryList& list);
    static bool shouldAudit(BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform);
    static bool shouldAudit(BlazeId id, const char8_t* deviceId, uint32_t addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform);
    static void getAuditIds(BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform, AuditIdSet& auditIds);
    static void getAuditIds(BlazeId id, const char8_t* deviceId, uint32_t addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform, AuditIdSet& auditIds);
    /**
    This is the most generic logging method.

    @param level The level of the logging request.
    @param message The message of the logging request.
    @param file The source file of the logging request, may be null.
    @param line The number line of the logging request.
    @param pattern format for the following variable arguments.
    */
    static void log(int32_t category, ::Blaze::Logging::Level level, const char* file, int line,
            const char* message, ...) ATTRIBUTE_PRINTF_CHECK(5,6);
    static void log(int32_t category, ::Blaze::Logging::Level level, const char* file, int line,
            StringBuilder &message);
    static void _log(int32_t category, ::Blaze::Logging::Level level, const char* file, int32_t line,
            const char* message, va_list args);

    static void logEvent(int32_t category, const char8_t* event, size_t size);
    static void logTdf(int32_t category, const EA::TDF::Tdf& tdf);
    static void logExternalThreadLogEntry(const ExternalThreadLogEntry& entry);
    static bool isLogEventEnabled(int32_t category);
    static bool isLogBinaryEventEnabled(int32_t category);

    //Gets the per fiber logging message
    static StringBuilder& getCurrentLoggingMessage() { return *mCurrentLoggingMessage;}
    static StringBuilder& getThisFunctionLoggingMessage() { return *mThisFunctionLoggingMessage;}

    /**
     * Is debug enabled
     *  @return bool -  true if logging is enabled for this category and level; false otherwise
     */
    static bool isEnabled(int32_t category, int32_t level, const char8_t* file, int32_t line);
    static bool isEnabledWithoutContext(int32_t category, int32_t level);

    static bool coreOnAssertLog() { return mCoreOnAssertLog; }

    static bool registerCategory(Log::Category category, const char8_t* logFileSubName, const char8_t* logFileSubFolderName = "");
    static bool initializeCategoryLevel(const char8_t* name, ::Blaze::Logging::Level level);
    static void initializeCategoryDefaults();
    static const char8_t* getCategoryName(Log::Category category) { return mCategories[category].mName; }

    typedef eastl::vector<const char8_t*> CategoryNameList;
    static CategoryNameList* getRegisteredCategoryNames();

    static const char8_t* getLogDir() { return mLogDir; }

    // Remap the level output to a new value.
    static bool isRemapLogLevels();
    static void remapLogLevels();
    static void resetLogLevels();
    static ::Blaze::Logging::Level getRemappedLevel(::Blaze::Logging::Level);

    static void getStatus(LoggerMetrics& metrics);

private:
    friend class LogOutputter;
    friend class LogFileInfo;

    static void configureGrpcCoreLogging(Logging::Level level, const ::Blaze::LoggingConfig::StringGrpcCoreTracersList& grpcCoreTracers);

    static const int32_t NAME_LEN_MAX = 64;

    enum
    {
        TRACE_IDX_DB = 0, TRACE_IDX_RPC, TRACE_IDX_HTTP, TRACE_IDX_REPLICATION, TRACE_IDX_MAX
    };

    struct CategoryInfo
    {
        char8_t mName[NAME_LEN_MAX];
        ::Blaze::Logging::Level mLevel;
    };

    struct TraceInfo
    {
        const char8_t* name;
        ::Blaze::Logging::Level level;
    };

    enum TraceSetting
    {
        TRACE_NONE, TRACE_ALL, TRACE_CATEGORY
    };

    static bool onAssertFailure(const char* expr, const char* filename, int line, const char* function, const char* msg, va_list args);

    static const uint32_t MIN_SUBNET_MASK = 0xfffff800;
    static bool auditLogEntryIsMatch(const AuditLogEntry& entry, BlazeId id, const char8_t* deviceId, const InetAddress& addr, const char8_t* persona, AccountId nucleusAccountId, ClientPlatformType platform);
    static BlazeRpcError validateAuditLogEntry(const AuditLogEntry& entry, bool unrestricted);

private:
    static bool mIsInitialized;
    static ::Blaze::Logging::Level mLevel;
    static volatile bool mRemapLogLevels;
    static enum TraceSetting mTrace[TRACE_IDX_MAX];
    static bool mIncludeLocation;
    static bool mEncodeLocation;
    static bool mCoreOnAssertLog;
    static bool mSuppressAuditLogging;
    static bool mSuppressDebugConsoleLogging;
    static CategoryInfo mCategories[Log::CATEGORY_MAX];
    static int32_t mMaxCategoryLength;
    static TraceInfo sTraceInfo[TRACE_IDX_MAX];
    static LogOutputter* mOutputter;
    static ExternalThreadLogOutputter* mExternalThreadLogOutputter;
    static char8_t mLogDir[256];
    static char8_t mName[256];
    static bool mMainLogToStdout;
    static bool mMainLogToFile;
    static bool mOtherLogsToFile;
    static uint32_t mRotationFileSize;
    static uint32_t mLargeEntrySize;
    static uint32_t mLargeEntryCount;
    static LoggingConfigPtr mConfig;
    static EA::Thread::Mutex mConfigLock;
    static FiberLocalManagedPtr<StringBuilder> mCurrentLoggingMessage;
    static FiberLocalManagedPtr<StringBuilder> mThisFunctionLoggingMessage;
    static DbIdByPlatformTypeMap mDbIdByPlatformTypeMap;
};

namespace BlazeRpcLog
{
    int32_t getLogCategory(EA::TDF::ComponentId componentId);
};

#define BLAZE_SPAM(category, ...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::SPAM, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::SPAM, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_TRACE(category, ...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::TRACE, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::TRACE, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_TRACE_DB(category, ...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_DB, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::T_DB, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_TRACE_RPC(category, ...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_RPC, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::T_RPC, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_TRACE_HTTP(category, ...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_HTTP, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::T_HTTP, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_TRACE_REPLICATION(category, ...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_REPL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::T_REPL, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_INFO(category, ...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::INFO, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::INFO, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_WARN(category, ...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::WARN, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::WARN, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_ERR(category, ...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::ERR, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::ERR, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_FATAL(category, ...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::FATAL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::FATAL, __FILE__, __LINE__, __VA_ARGS__); \
}

#define BLAZE_LOG(level, category, message) \
{ \
    if (::Blaze::Logger::isEnabled(category, level, __FILE__, __LINE__)) \
        ::Blaze::Logger::log(category, level, __FILE__, __LINE__, (::Blaze::Logger::getCurrentLoggingMessage().reset() << message)); \
}

#define BLAZE_SPAM_LOG(category, message) BLAZE_LOG(::Blaze::Logging::SPAM, category, message)
#define BLAZE_TRACE_LOG(category, message) BLAZE_LOG(::Blaze::Logging::TRACE, category, message)
#define BLAZE_TRACE_DB_LOG(category, message) BLAZE_LOG(::Blaze::Logging::T_DB, category, message)
#define BLAZE_TRACE_RPC_LOG(category, message) BLAZE_LOG(::Blaze::Logging::T_RPC, category, message)
#define BLAZE_TRACE_HTTP_LOG(category, message) BLAZE_LOG(::Blaze::Logging::T_HTTP, category, message)
#define BLAZE_TRACE_REPLICATION_LOG(category, message) BLAZE_LOG(::Blaze::Logging::T_REPL, category, message)
#define BLAZE_INFO_LOG(category, message) BLAZE_LOG(::Blaze::Logging::INFO, category, message)
#define BLAZE_WARN_LOG(category, message) BLAZE_LOG(::Blaze::Logging::WARN, category, message)
#define BLAZE_ERR_LOG(category, message) BLAZE_LOG(::Blaze::Logging::ERR, category, message)
#define BLAZE_FAIL_LOG(category, message) BLAZE_LOG(::Blaze::Logging::FAIL, category, message)

// TODO_MC: Change to BLAZE_FAIL()
#define BLAZE_ASSERT_LOG(category, message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::FAIL, __FILE__, __LINE__))) { \
        ::Blaze::Logger::log(category, ::Blaze::Logging::FAIL, __FILE__, __LINE__, (::Blaze::Logger::getCurrentLoggingMessage().reset() << message)); \
        if (::Blaze::Logger::coreOnAssertLog()) { \
            EA_FAIL_MSG(::Blaze::Logger::getCurrentLoggingMessage().get()); \
        } \
    } \
}
// TODO_MC: rename BLAZE_ASSERT_LOG -> BLAZE_FAIL_LOG, then rename BLAZE_ASSERT_COND_LOG -> BLAZE_ASSERT_LOG
#define BLAZE_ASSERT_COND_LOG(category, expr, message) \
{ \
    if (!(expr)) { \
        if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::FAIL, __FILE__, __LINE__))) { \
            ::Blaze::Logger::log(category, ::Blaze::Logging::FAIL, __FILE__, __LINE__, (::Blaze::Logger::getCurrentLoggingMessage().reset() << message)); \
            if (::Blaze::Logger::coreOnAssertLog()) { \
                EA_FAIL_MSG(::Blaze::Logger::getCurrentLoggingMessage().get()); \
            } \
        } \
    } \
}

#define BLAZE_FATAL_LOG(category, message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::FATAL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(category, ::Blaze::Logging::FATAL, __FILE__, __LINE__, (::Blaze::Logger::getCurrentLoggingMessage().reset() << message)); \
}

#define THIS_FUNC ::Blaze::Logger::ThisFunction(EA_CURRENT_FUNCTION, this).get() 
#define STATIC_FUNC ::Blaze::Logger::ThisFunction(EA_CURRENT_FUNCTION, nullptr).get() 

#define BLAZE_IS_TRACE_DB_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_DB, __FILE__, __LINE__))
#define BLAZE_IS_TRACE_RPC_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_RPC, __FILE__, __LINE__))
#define BLAZE_IS_TRACE_HTTP_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_HTTP, __FILE__, __LINE__))
#define BLAZE_IS_TRACE_REPLICATION_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::T_REPL, __FILE__, __LINE__))
#define BLAZE_IS_INFO_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::INFO, __FILE__, __LINE__))
#define BLAZE_IS_WARN_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::WARN, __FILE__, __LINE__))
#define BLAZE_IS_ERR_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::ERR, __FILE__, __LINE__))
#define BLAZE_IS_FATAL_ENABLED(category) EA_UNLIKELY(::Blaze::Logger::isEnabled(category, ::Blaze::Logging::FATAL, __FILE__, __LINE__))
#define BLAZE_IS_LOGGING_ENABLED(category, level) ::Blaze::Logger::isEnabled(category, level, __FILE__, __LINE__)

}//Blaze

#ifdef BLAZE_COMPONENT_TYPE_INDEX_NAME

#define LOGGER_CATEGORY ::Blaze::BLAZE_COMPONENT_TYPE_INDEX_NAME

/**
Logs a message to a specified logger with the INFO level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define INFO(...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::INFO, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::INFO, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the WARN level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define WARN(...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::WARN, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::WARN, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the ERR level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define ERR(...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::ERR, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::ERR, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the FATAL level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define FATAL(...) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::FATAL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::FATAL, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the SPAM level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define SPAM(...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::SPAM, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::SPAM, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the SPAM level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define SPAM_LOG(message) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::SPAM, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::SPAM, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the TRACE level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define TRACE(...) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::TRACE, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::TRACE, __FILE__, __LINE__, __VA_ARGS__); \
}

/**
Logs a message to a specified logger with the TRACE level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define TRACE_LOG(message) \
{ \
    if (EA_UNLIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::TRACE, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::TRACE, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the INFO level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define INFO_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::INFO, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::INFO, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the WARN level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define WARN_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::WARN, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::WARN, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the ERR level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define ERR_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::ERR, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::ERR, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the FAIL level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define FAIL_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/**
Logs a message to a specified logger with the FAIL level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define ASSERT_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__))) { \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
        if (::Blaze::Logger::coreOnAssertLog()) { \
            EA_FAIL_MSG(::Blaze::Logger::getCurrentLoggingMessage().get()); \
        } \
    } \
}

// TODO_MC: rename ASSERT_LOG -> FAIL_LOG, then rename ASSERT_COND_LOG -> ASSERT_LOG
/**
Logs a message to a specified logger with the FAIL level, if the conditional expression is false.

@param expr conditional expression to check.
@param message the message string to log.
*/
#define ASSERT_COND_LOG(expr, message) \
{ \
    if (!(expr)) { \
        if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__))) { \
            ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::FAIL, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
            if (::Blaze::Logger::coreOnAssertLog()) { \
                EA_FAIL_MSG(::Blaze::Logger::getCurrentLoggingMessage().get()); \
            } \
        } \
    } \
}

/**
Logs a message to a specified logger with the FATAL level.

@param logger the logger to be used.
@param message the message string to log.
*/
#define FATAL_LOG(message) \
{ \
    if (EA_LIKELY(::Blaze::Logger::isEnabled(LOGGER_CATEGORY, ::Blaze::Logging::FATAL, __FILE__, __LINE__))) \
        ::Blaze::Logger::log(LOGGER_CATEGORY, ::Blaze::Logging::FATAL, __FILE__, __LINE__, ::Blaze::Logger::getCurrentLoggingMessage().reset() << message); \
}

/* Check if logging is enabled at the given level */
#define IS_LOGGING_ENABLED(level) ::Blaze::Logger::isEnabled(LOGGER_CATEGORY, level, __FILE__, __LINE__)
#endif

#endif

