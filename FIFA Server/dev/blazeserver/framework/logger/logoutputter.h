/*************************************************************************************************/
/*!
\file logoutputter.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_LOGOUTPUTTER_H
#define BLAZE_LOGOUTPUTTER_H

#include "framework/blazedefines.h"

#include "framework/logger/logfileinfo.h"
#include "EASTL/intrusive_list.h"

namespace Blaze
{

struct LogEntry : public eastl::intrusive_list_node
{
    typedef enum { UNSET, DROPPED, NORMAL, EVENT, TDF } Type;

    LogEntry(Type type, uint32_t size, bool large = false)
        : mType(type),
        mLarge(large),
        mData(nullptr),
        mCount(0),
        mPrefixLength(0),
        mSize(size),
        mEventCategory(0)
    {
        if (size > 0)
            mData = (char8_t*)BLAZE_ALLOC_MGID(size, Blaze::MEM_GROUP_FRAMEWORK_LOG, "LogEntry");
    }

    virtual ~LogEntry() { BLAZE_FREE(mData); }

    Type mType;
    bool mLarge;

    char8_t* mData;
    uint32_t mCount;
    uint32_t mPrefixLength;
    uint32_t mSize;

    int32_t mEventCategory;
    Logging::Level mLevel;
    AuditIdSet mAuditIds;
};

class LogOutputter : public BlazeThread
{

private:


    class LogEntryBehavioursData
    {
    public:
        LogEntryBehavioursData();

        void initialize(const LogEntryBehaviours& behaviours);

        bool isMatch(const char8_t* file, int32_t line) const;

        const char8_t* getFilename() const { return mFilename; }

        int32_t getLineNum() const { return mLineNum; }

        bool checkAndTickSuppression(bool& logSuppression);

        uint32_t getSuppressionThreshold() const { return mSuppressionThreshold; }

        bool shouldGenerateCore() const;

        bool isPinned() const { return mShouldPin; }

        void tickCores();
        void tickPinned();

        void getStatus(LoggerMetrics::BehaviourMetrics& metrics);

    private:
        bool mShouldSuppress;
        EA::Thread::AtomicUint64 mTotalSuppressions;
        uint32_t mSuppressionThreshold;

        bool mShouldGenerateCore;
        EA::Thread::AtomicUint64 mTotalCores;
        uint64_t mMaxCores;

        bool mShouldPin;
        EA::Thread::AtomicUint64 mTotalPinned;

        char8_t mFilename[1024];
        int32_t mLineNum;
        eastl::string mLocation;
    };

public:
    typedef enum { EVENT_LOG_TYPE_TEXT, EVENT_LOG_TYPE_BINARY } EventLoggingType;

    LogOutputter();

    // Initialize thread-local metrics
    void initializeThreadLocal();

    // Destroy thread-local metrics
    void destroyThreadLocal();

    ~LogOutputter() override;

    void registerCategory(Log::Category category, const char8_t* logFileSubName, const char8_t* logFileSubFolderName = "");

    LogEntry* getEntry(bool large);

    void releaseEntry(LogEntry& entry);

    void trimEntry(LogEntry& entry);

    void stop() override;

    void logf(int32_t category, Logging::Level level, const char* file,
        int32_t line, bool hasVaList, const char* message, ...);

    bool shouldLog(const char8_t* file, int32_t line);

    void log_va_list(int32_t category, Logging::Level level, const char* file,
        int32_t line, bool hasVaList, const char* message, va_list args);

    bool isEventEnabled(int32_t category) const { return mEventLogs[category].isEnabled(); }

    bool isBinaryEventEnabled(int32_t category) const { return mHeat2Logs[category].isEnabled(); }

    void logEvent(int32_t category, const char8_t* event, size_t size);

    void logTdf(int32_t category, const EA::TDF::Tdf& tdf);

    bool getPrefixMultilineEnabled() const { return mPrefixMultilineEnabled; }
    void setPrefixMultilineEnabled(bool enabled) {  mPrefixMultilineEnabled = enabled; }

    void setAuditLogRefreshInterval(TimeValue interval) { mAuditLogRefreshInterval = interval; }

    void triggerReconfigure() { mReconfigure = true; }

    void configureBehaviours(const LoggingConfig& config);

    void setCategory(EventLoggingType type, bool state, TimeValue rotationPeriod, int32_t index);

    void reconfigure();

    void rotateLogs(bool forced, TimeValue now = TimeValue::getTimeOfDay());

    void rotateMainLog(bool forced, TimeValue now);

    void rotateEvent(bool forced, LogFileInfo* event, TimeValue now);

    void configureEventLogging(EventLoggingType type, const EventsList& events);

    void getStatus(LoggerMetrics& metrics);

    int getMainLogFd() { return mMainLog.getFd(); }

private:
    static const size_t REGULAR_ENTRY_SIZE = 1024 * 8;
    static const size_t REGULAR_ENTRY_COUNT = 1024 * 8;
    static const size_t ENTRY_DRAIN_BATCH_SIZE = REGULAR_ENTRY_COUNT / 4;
    static const size_t ENTRY_RELEASE_BATCH_SIZE = 100;
    static const size_t AUDIT_LOG_REFRESH_INTERVAL = 60 * 1000 * 1000; // default until overridden from Logger config

                                                                        // Maximum number of log entry behaviours that can be configured.  This number should remain
                                                                        // reasonably small as it has performance implications due to the frequency of calls to
                                                                        // the logger.
    static const size_t MAX_BEHAVIOURS = 16;

    volatile bool mShutdown;

    typedef eastl::intrusive_list<LogEntry> EntryList;
    LogEntry mDropped;
    EntryList mFreeListRegular;
    EntryList mActive[2];
    size_t mActiveSizes[2];
    EA::Thread::AtomicInt32 mLargeEntryBytes;
    int32_t mQueueList;
    int32_t mDrainList;
    EA::Thread::Mutex mFreeLock;
    EA::Thread::Mutex mActiveLock;
    EA::Thread::Condition mActiveCond;

    LogFileInfo mMainLog;
    LogFileInfo mEventLogs[Log::CATEGORY_MAX];
    LogFileInfo mHeat2Logs[Log::CATEGORY_MAX];
    eastl::hash_map<AuditId, LogFileInfo> mAuditLogs;
    bool mReconfigure;
    bool mPrefixMultilineEnabled;
    TimeValue mNextDailyRotation;
    TimeValue mNextAuditLogRefresh;
    TimeValue mAuditLogRefreshInterval;
    LogEntryBehavioursData mBehaviours[MAX_BEHAVIOURS];
    size_t mNumBehaviours;
    LogEntryBehavioursData mDefaultBehaviours;

    void run() override;

    void refreshAuditLogs(bool force, TimeValue now);

    TimeValue getNextDailyRotationTime() const;

    void drainEntries();

    int32_t log_snzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, ...) const;

    int32_t log_vsnzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, va_list Args) const;

    bool logToEntry(LogEntry& entry, const char8_t* timestamp, int32_t category,
        Logging::Level level, const char* file, int32_t line, bool hasVaList,
        const char* message, va_list args) const;

    void logDrop();

    LogEntryBehavioursData& getBehaviours(const char8_t* file, int32_t line);

    LogFileInfo* getOrCreateAuditLogFileInfo(AuditId auditId);
};

// The logs created by an external thread in blaze server has to go via an intermediate shim thread. Direct usage of our logging system is not possible as it
// relies on the OS thread to be capable of fiber support.

struct ExternalThreadLogEntry
{
public:
    eastl::string mMessage;
    
    Blaze::Log::Category mCategory;
    Blaze::Logging::Level mLevel;

    ExternalThreadLogEntry(const char8_t* msg, Blaze::Log::Category category, Blaze::Logging::Level level)
        : mMessage(msg)
        , mCategory(category)
        , mLevel(level)
    {

    }

    ExternalThreadLogEntry(const ExternalThreadLogEntry& entry)
        : mMessage(entry.mMessage)
        , mCategory(entry.mCategory)
        , mLevel(entry.mLevel)
    {

    }
};

class ExternalThreadLogEntryJob : public Job
{
public:
    ExternalThreadLogEntryJob(const ExternalThreadLogEntry& logEntry)
        : mLogEntry(logEntry)
    {
    }

    void execute() override;

private:
    ExternalThreadLogEntry mLogEntry;
};

class ExternalThreadLogOutputter : public BlazeThread
{

public:
    ExternalThreadLogOutputter()
        : BlazeThread("ExLogger", BlazeThread::EXLOGGER)
        , mJobQueue("ExternalThreadLogs")
        , mShutdown(false)
    {

    }

    ~ExternalThreadLogOutputter() override
    {

    }

    void stop() override;
    void run() override;

    void pushLogEntry(const ExternalThreadLogEntry& logEntry);

private:
    JobQueue mJobQueue;
    volatile bool mShutdown;

};
}

#endif
