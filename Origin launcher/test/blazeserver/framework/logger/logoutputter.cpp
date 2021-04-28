/*************************************************************************************************/
/*!
\file logoutputter.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "framework/logger/logoutputter.h"
#include "framework/controller/processcontroller.h"
#include "framework/system/threadlocal.h"
#include "framework/protocol/shared/heat2encoder.h"

#include "EAIO/EAFileBase.h"
#include "EAIO/PathString.h"
#include "EAIO/EAFileUtil.h"

namespace Blaze
{

#ifdef EA_PLATFORM_WINDOWS
#define DEBUG_STRING_OUTPUT_ENABLED 1
#else
#define DEBUG_STRING_OUTPUT_ENABLED 0
#endif

/**
* Debug string output is a windows-only blaze feature that pipes log output to the windows debug handler.
* It is on default for convenience when building/debugging with Visual Studio, but should be disabled when profiling
* the performance of the logging subsystem under Visual Studio because the debug handler has very high overhead.
* \Note: OutputDebugString() only accepts 0x0 terminated strings, thus we need to terminate partial strings manually.
*/
#if DEBUG_STRING_OUTPUT_ENABLED
#define BlazeShouldLogToDebugger() (!Logger::mSuppressDebugConsoleLogging && IsDebuggerPresent())
#define BlazeOutputDebugString(text, length) if ((length) == 0) { OutputDebugString(text); } else { auto* textEnd = (text) + (length); auto tmp = *(textEnd); *(textEnd) = '\0'; OutputDebugString(text); *(textEnd) = tmp; }
#else
#define BlazeShouldLogToDebugger() false
#define BlazeOutputDebugString(text, length)
#endif


#define CREATE_TIMESTAMP(timestamp) \
    char8_t timestamp[32]; \
    uint32_t year; \
    uint32_t month; \
    uint32_t day; \
    uint32_t hour; \
    uint32_t min; \
    uint32_t sec; \
    uint32_t msec; \
    TimeValue::getGmTimeComponents( \
            TimeValue::getTimeOfDay(), &year, &month, &day, &hour, &min, &sec, &msec); \
    blaze_snzprintf(timestamp, sizeof(timestamp), "%d/%02d/%02d-%02d:%02d:%02d.%03dZ", \
            year, month, day, hour, min, sec, msec);

    
typedef eastl::intrusive_list<LogFileInfo> LogFileInfoList;

// Metrics are tracked per-instance but the Logger is global so setup a static
// thread-local to keep track of per-instance logger metrics
struct LogMetrics
{
    LogMetrics()
        : mCategoryMetrics(*Metrics::gFrameworkCollection, "logger.count", Metrics::Tag::log_category, Metrics::Tag::log_level)
        , mTotalSuppressions(*Metrics::gFrameworkCollection, "logger.suppressions", Metrics::Tag::log_location)
        , mTotalPinned(*Metrics::gFrameworkCollection, "logger.pinned", Metrics::Tag::log_location)
        , mTotalCores(*Metrics::gFrameworkCollection, "logger.cores", Metrics::Tag::log_location)
    {
    }

    Metrics::TaggedCounter<Log::Category, Logging::Level> mCategoryMetrics;

    Metrics::TaggedCounter<Metrics::LogLocation> mTotalSuppressions;
    Metrics::TaggedCounter<Metrics::LogLocation> mTotalPinned;
    Metrics::TaggedCounter<Metrics::LogLocation> mTotalCores;
};

static EA_THREAD_LOCAL LogMetrics* sLoggerMetrics = nullptr;


LogOutputter::LogEntryBehavioursData::LogEntryBehavioursData()
    : mShouldSuppress(false),
    mTotalSuppressions(0),
    mSuppressionThreshold(UINT32_MAX),
    mShouldGenerateCore(false),
    mTotalCores(0),
    mMaxCores(0),
    mShouldPin(false),
    mTotalPinned(0),
    mLineNum(0)
{
    mFilename[0] = '\0';
}

void LogOutputter::LogEntryBehavioursData::initialize(const LogEntryBehaviours& behaviours)
{
    mTotalSuppressions = 0;
    mTotalCores = 0;
    mTotalPinned = 0;

    // Zero out the mFilename member first since it is theoretically possible for there
    // to be concurrent access to it from another thread.  It's not important that the
    // value be intact for the other thread since it would just be using it to determine
    // if a log entry matched but it is important that it cannot trigger an invalid
    // memory access by walking off the end of an unterminated string.
    memset(mFilename, 0, sizeof(mFilename));

    EA::IO::Path::PathString8 filename(behaviours.getFilename());
    if (EA::IO::Path::IsRelative(filename))
    {
        const char8_t* buildLocation = gProcessController->getVersion().getBuildLocation();
        if (buildLocation != nullptr)
        {
            // Skip over the username@hostname: portion of the build location
            const char8_t* pathStart = strchr(buildLocation, ':');
            if (pathStart != nullptr)
                buildLocation = pathStart + 1;

            filename = buildLocation;
            EA::IO::Path::Append(filename, behaviours.getFilename());
        }
    }
    blaze_strnzcpy(mFilename, filename.c_str(), sizeof(mFilename));

    mLineNum = behaviours.getLineNum();
    mLocation.sprintf("%s:%d", mFilename, mLineNum);

    mShouldSuppress = behaviours.getSuppress();
    mShouldPin = behaviours.getPin();
    mShouldGenerateCore = behaviours.getGenerateCore();
    mMaxCores = behaviours.getMaxCores();
    mSuppressionThreshold = behaviours.getSuppressionThreshold();
    if (mSuppressionThreshold == 0)
        mSuppressionThreshold = UINT32_MAX;
}

bool LogOutputter::LogEntryBehavioursData::isMatch(const char8_t* file, int32_t line) const
{
    return ((mLineNum == line) && (strcmp(file, mFilename) == 0));
}


bool LogOutputter::LogEntryBehavioursData::checkAndTickSuppression(bool& logSuppression)
{
    if (!mShouldSuppress)
        return false;

    bool shouldSuppress = true;

    uint64_t total = mTotalSuppressions++;
    if ((total % mSuppressionThreshold) == 0)
    {
        logSuppression = true;
        shouldSuppress = false;
    }

    if (sLoggerMetrics != nullptr)
        sLoggerMetrics->mTotalSuppressions.increment(1, mLocation.c_str());

    return shouldSuppress;
}


bool LogOutputter::LogEntryBehavioursData::shouldGenerateCore() const
{
    if (!mShouldGenerateCore)
        return false;
    if (mTotalCores >= mMaxCores)
        return false;
    return true;
}


void LogOutputter::LogEntryBehavioursData::tickCores()
{
    mTotalCores++;
    if (sLoggerMetrics != nullptr)
        sLoggerMetrics->mTotalCores.increment(1, mLocation.c_str());
}

void LogOutputter::LogEntryBehavioursData::tickPinned()
{
    if (mShouldPin)
    {
        mTotalPinned++;
        if (sLoggerMetrics != nullptr)
            sLoggerMetrics->mTotalPinned.increment(1, mLocation.c_str());
    }
}

void LogOutputter::LogEntryBehavioursData::getStatus(LoggerMetrics::BehaviourMetrics& metrics)
{
    metrics.setFilename(mFilename);
    metrics.setLineNum(mLineNum);
    metrics.setSuppressions(sLoggerMetrics->mTotalSuppressions.getTotal({ { Metrics::Tag::log_location, mLocation.c_str() } }));
    metrics.setPinned(sLoggerMetrics->mTotalPinned.getTotal({ { Metrics::Tag::log_location, mLocation.c_str() } }));
    metrics.setCores(sLoggerMetrics->mTotalCores.getTotal({ { Metrics::Tag::log_location, mLocation.c_str() } }));
}

typedef enum { EVENT_LOG_TYPE_TEXT, EVENT_LOG_TYPE_BINARY } EventLoggingType;

LogOutputter::LogOutputter()
    : BlazeThread("logger", BlazeThread::LOGGER),
    mShutdown(false),
    mDropped(LogEntry::DROPPED, 0),
    mLargeEntryBytes(0),
    mQueueList(0),
    mDrainList(1),
    mAuditLogs(BlazeStlAllocator("LogOutputter::mAuditLogs", Blaze::MEM_GROUP_FRAMEWORK_DEFAULT)),
    mReconfigure(false),
    mPrefixMultilineEnabled(true),
    mNumBehaviours(0)
{
    for (size_t idx = 0; idx < REGULAR_ENTRY_COUNT; ++idx)
    {
        LogEntry* entry = BLAZE_NEW_LOG LogEntry(LogEntry::NORMAL, REGULAR_ENTRY_SIZE);
        mFreeListRegular.push_back(*entry);
    }

    mMainLog.initialize(nullptr);
    mMainLog.setState(true);

    for (size_t idx = 0; idx < EAArrayCount(mActiveSizes); ++idx)
        mActiveSizes[idx] = 0;

    //Register any categories made before we were initialized
    for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
    {
        if (*Logger::mCategories[idx].mName != '\0')
        {
            registerCategory(idx, Logger::mCategories[idx].mName);
        }
    }

    mAuditLogRefreshInterval = AUDIT_LOG_REFRESH_INTERVAL;
}


// Destroy thread-local metrics
void LogOutputter::destroyThreadLocal()
{
    delete sLoggerMetrics;
    sLoggerMetrics = nullptr;
}

LogOutputter::~LogOutputter()
{
    // Make sure there isn't anything left to log in case stuff was queued after the
    // logger thread was stopped
    mDrainList = 0;
    drainEntries();
    mDrainList = 1;
    drainEntries();

    // Force log rotation before shutdown
    rotateLogs(true);

    // Close all the log files
    mMainLog.close();
    for (size_t idx = 0; idx < EAArrayCount(mEventLogs); ++idx)
    {
        mEventLogs[idx].close();
        mHeat2Logs[idx].close();
    }

    // Free the log entries
    while (!mFreeListRegular.empty())
    {
        LogEntry* entry = &mFreeListRegular.front();
        mFreeListRegular.pop_front();
        delete entry;
    }
}

void LogOutputter::initializeThreadLocal() 
{ 
    sLoggerMetrics = BLAZE_NEW LogMetrics(); 
}

void LogOutputter::registerCategory(Log::Category category, const char8_t* logFileSubName, const char8_t* logFileSubFolderName)
{
    mEventLogs[category].initialize(logFileSubName, logFileSubFolderName);
    mHeat2Logs[category].initialize(logFileSubName, Logger::BLAZE_LOG_BINARY_EVENTS_DIR);
}

LogEntry* LogOutputter::getEntry(bool large)
{
    LogEntry* entry = nullptr;
    if (EA_LIKELY(!large))
    {
        if (!mFreeListRegular.empty())
        {
            entry = &mFreeListRegular.front();
            mFreeListRegular.pop_front();
        }
        else
        {
            // if out of regular entries, the logging rate is too high;
            // therefore, drop further regular entries rather than eating
            // up our precious large entries budget.
        }
    }
    else if ((mLargeEntryBytes + Logger::mLargeEntrySize) <= (Logger::mLargeEntrySize * Logger::mLargeEntryCount))
    {
        // This enforces the amount of memory we allow to be used by large log entries.
        // Instead of capping the number of entries we use the max computed memory budget.
        // This coupled with trimEntry() enables us to utilize the large entry memory much more efficiently.
        entry = BLAZE_NEW_LOG LogEntry(LogEntry::NORMAL, Logger::mLargeEntrySize, true);
        mLargeEntryBytes += entry->mSize;
    }

    return entry;
}

void LogOutputter::releaseEntry(LogEntry& entry)
{
    if (!entry.mLarge)
    {
        entry.mType = LogEntry::UNSET;
        entry.mCount = 0;
        entry.mAuditIds.clear();
        mFreeListRegular.push_back(entry);
    }
    else
    {
        // large entries are not pooled
        mLargeEntryBytes -= entry.mSize;
        delete &entry;
    }
}

void LogOutputter::trimEntry(LogEntry& entry)
{
    if (!entry.mLarge)
        return; // Don't trim small entries because they are reused in the freelist and must maintain fixed capacity

    if (entry.mCount < entry.mSize)
    {
        // Trim large entry to enable *much* better memory utilization and handle bursts better
        auto* data = (char8_t*) BLAZE_REALLOC_MGID(entry.mData, entry.mCount, Blaze::MEM_GROUP_FRAMEWORK_LOG, "LogEntry");
        if (data != nullptr)
        {
            mLargeEntryBytes -= (entry.mSize - entry.mCount); // reduce outstanding memory by the trimmed amount
            entry.mData = data;
            entry.mSize = entry.mCount;
        }
        // else
        //  if realloc returns nullptr, it leaves original memory unchanged
    }
}

void LogOutputter::stop()
{
    mShutdown = true;
    waitForEnd();
}

void LogOutputter::logf(int32_t category, Logging::Level level, const char* file,
    int32_t line, bool hasVaList, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_va_list(category, level, file, line, hasVaList, message, args);
    va_end(args);
}

bool LogOutputter::shouldLog(const char8_t* file, int32_t line)
{
    // Quick pre-check here since this function is executed with every log entry
    if (mNumBehaviours == 0)
        return false;

    LogEntryBehavioursData& behaviours = getBehaviours(file, line);
    return behaviours.shouldGenerateCore() || behaviours.isPinned();
}

void LogOutputter::log_va_list(int32_t category, Logging::Level level, const char* file,
    int32_t line, bool hasVaList, const char* message, va_list args)
{
    LogEntryBehavioursData& behaviours = getBehaviours(file, line);
    if (behaviours.shouldGenerateCore())
    {
        if (gProcessController != nullptr)
        {
            gProcessController->coreDump("logger", 0);
            behaviours.tickCores();
        }
    }

    behaviours.tickPinned();

    // Check if the log entry should be suppressed
    bool logSuppression = false;
    if (behaviours.checkAndTickSuppression(logSuppression))
        return;


    CREATE_TIMESTAMP(timestamp);

    mFreeLock.Lock();
    LogEntry* entry = getEntry(false);
    mFreeLock.Unlock();
    if (entry == nullptr)
    {
        logDrop();
        return;
    }

#ifndef EA_PLATFORM_WINDOWS
    // Since the va_list may be used multiple times, make a copy of it since
    // it can be a destroyed the first time it is used.
    va_list tmpargs;
    va_copy(tmpargs, args);
#else
#define tmpargs args
#endif
    if (!logToEntry(*entry, timestamp, category, level, file, line, hasVaList, message, tmpargs))
    {
        // Log was too large for a regular entry so try and grab a bigger one
        mFreeLock.Lock();
        releaseEntry(*entry);
        entry = getEntry(true);
        mFreeLock.Unlock();
        if (entry == nullptr)
        {
            logDrop();
            return;
        }
        if (!logToEntry(*entry, timestamp, category, level, file, line, hasVaList, message, args))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].log_va_list: log entry truncated because it exceeds the max size");
        }
        trimEntry(*entry);
    }

    if (entry->mType == LogEntry::NORMAL)
    {
        // Tick the log count metrics
        if (sLoggerMetrics != nullptr)
            sLoggerMetrics->mCategoryMetrics.increment(1, entry->mEventCategory, entry->mLevel);
    }

    bool wake = false;
    mActiveLock.Lock();
    mActive[mQueueList].push_back(*entry);
    ++mActiveSizes[mQueueList];
    if (mActiveSizes[mQueueList] > ENTRY_DRAIN_BATCH_SIZE)
    {
        // The queue is more than a quarter full so wake up the write.  We don't wake the writer
        // every time a log entry is added because it already wakes up periodically to
        // drain the queue.  This should help reduce contention on the mutex.
        wake = true;
    }
    mActiveLock.Unlock();
    if (wake)
        mActiveCond.Signal(false);

    if (logSuppression)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Logger].log_va_list: the next " << (behaviours.getSuppressionThreshold() - 1) << " log entries from " << behaviours.getFilename() << ":" << behaviours.getLineNum() << " will be suppressed.");
    }
}


void LogOutputter::logEvent(int32_t category, const char8_t* event, size_t size)
{
    if (!isEventEnabled(category))
        return;

    mFreeLock.Lock();
    LogEntry* entry = getEntry(size >= REGULAR_ENTRY_SIZE);
    mFreeLock.Unlock();
    if (entry == nullptr)
    {
        logDrop();
        return;
    }

    if (size >= (entry->mSize - 1))
    {
        // Event is too large for the buffer so we have to drop it.
        mFreeLock.Lock();
        releaseEntry(*entry);
        mFreeLock.Unlock();
        return;
    }
    entry->mType = LogEntry::EVENT;
    memcpy(entry->mData, event, size);
    entry->mData[size] = '\0';
    entry->mCount = (uint32_t)size;
    entry->mEventCategory = category;
    trimEntry(*entry);

    bool wake = false;
    mActiveLock.Lock();
    mActive[mQueueList].push_back(*entry);
    ++mActiveSizes[mQueueList];
    if (mActiveSizes[mQueueList] > ENTRY_DRAIN_BATCH_SIZE)
    {
        // The queue is more than half full so wake up the write.  We don't wake the writer
        // every time a log entry is added because it already wakes up periodically to
        // drain the queue.  This should help reduce contention on the mutex.
        wake = true;
    }
    mActiveLock.Unlock();
    if (wake)
        mActiveCond.Signal(false);
}

void LogOutputter::logTdf(int32_t category, const EA::TDF::Tdf& tdf)
{
    // always increase the sequence counter
    uint32_t seq = mHeat2Logs[category].incSeqCounter();
    uint32_t size = 0;

    LogEntry* entry = nullptr;
    for (int a = 0; a < 2; ++a) // try twice, once with the regular entry and again with a large entry
    {
        mFreeLock.Lock();
        if (a == 0)
            entry = getEntry(false); // first try with a regular entry, should be fine most of the time
        else
            entry = getEntry(true); // in case a regular entry was too small or not available, try a larger one
        mFreeLock.Unlock();

        if (entry == nullptr)
        {
            // if we weren't able to get an entry for some reason, continue which will
            // try to get an entry from a different collection
            continue;
        }

        // initialize a RawBuffer with the memory provided by the LogEntry::mData,
        // but pad it a bit to leave room for the sequence counter and the size
        Blaze::RawBuffer raw((uint8_t *)entry->mData + sizeof(seq) + sizeof(size), entry->mSize - (sizeof(seq) + sizeof(size)));

        Heat2Encoder encoder;
        if (!encoder.encode(raw, tdf))
        {
            // If the encoding fails, it was probably because the buffer was too small.
            // So release it, and continue which will try with a larger one, or fail without logging the tdf
            mFreeLock.Lock();
            releaseEntry(*entry);
            mFreeLock.Unlock();

            entry = nullptr;
            continue;
        }

        size = raw.datasize();

        // we're all good, just break from the loop
        break;
    }

    if (entry == nullptr)
    {
        // If we've tried twice already, and still can't get a usable entry, we must return without logging the tdf
        logDrop();
        return;
    }

    // else everything should be fine, so just fill in the sequence counter and size and carry on.

    entry->mType = LogEntry::TDF;
    entry->mEventCategory = category;
    entry->mCount = sizeof(seq) + sizeof(size) + size;
    trimEntry(*entry);

    seq = htonl(seq);
    memcpy(entry->mData, &seq, sizeof(seq));
    size = htonl(size);
    memcpy(entry->mData + sizeof(seq), &size, sizeof(size));

    bool wake = false;
    mActiveLock.Lock();
    mActive[mQueueList].push_back(*entry);
    ++mActiveSizes[mQueueList];
    if (mActiveSizes[mQueueList] > ENTRY_DRAIN_BATCH_SIZE)
    {
        // The queue is more than half full so wake up the write.  We don't wake the writer
        // every time a log entry is added because it already wakes up periodically to
        // drain the queue.  This should help reduce contention on the mutex.
        wake = true;
    }
    mActiveLock.Unlock();
    if (wake)
        mActiveCond.Signal(false);
}


void LogOutputter::configureBehaviours(const LoggingConfig& config)
{
    mNumBehaviours = 0;

    // Populate new list based on latest config
    LoggingConfig::LogEntryBehavioursList::const_iterator itr = config.getBehaviours().begin();
    LoggingConfig::LogEntryBehavioursList::const_iterator end = config.getBehaviours().end();
    for (int32_t idx = 1; (itr != end) && (mNumBehaviours < MAX_BEHAVIOURS); ++itr, ++idx)
    {
        const LogEntryBehaviours* entry = *itr;

        bool accept = true;
        if (entry->getFilename()[0] == '\0')
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[LogOutputter].configureBehaviours: missing filename for entry " << idx << "; ignored.");
            accept = false;
        }
        if (entry->getLineNum() <= 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[LogOutputter].configureBehaviours: invalid line number for entry " << idx << "; ignored.");
            accept = false;
        }
        if (entry->getGenerateCore() && (entry->getMaxCores() == 0))
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[LogOutputter].configureBehaviours: invalid entry: generateCore=true but maxCores=0 for entry " << idx << "; ignored.");
            accept = false;
        }

        if (accept)
            mBehaviours[mNumBehaviours++].initialize(*entry);
    }
    if ((itr != end) && (mNumBehaviours == MAX_BEHAVIOURS))
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Logger].configureBehaviours: a maximum of " << MAX_BEHAVIOURS << " behaviour entries can be configured but " << config.getBehaviours().size() << " exist in the configuration; ignoring the excess entries.");
    }
}

void LogOutputter::setCategory(EventLoggingType type, bool state, TimeValue rotationPeriod, int32_t index)
{
    if (index >= 0)
    {
        switch (type)
        {
        case EVENT_LOG_TYPE_TEXT:
            mEventLogs[index].setState(state);
            mEventLogs[index].setRotationPeriod(rotationPeriod);
            break;
        case EVENT_LOG_TYPE_BINARY:
            mHeat2Logs[index].setState(state);
            mHeat2Logs[index].setRotationPeriod(rotationPeriod);
            break;
        }
    }
    else
    {
        for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
        {
            switch (type)
            {
            case EVENT_LOG_TYPE_TEXT:
                mEventLogs[idx].setState(state);
                mEventLogs[index].setRotationPeriod(rotationPeriod);
                break;
            case EVENT_LOG_TYPE_BINARY:
                mHeat2Logs[idx].setState(state);
                mHeat2Logs[index].setRotationPeriod(rotationPeriod);
                break;
            }
        }
    }
}

void LogOutputter::reconfigure()
{
    mReconfigure = false;
    rotateLogs(true);
}

void LogOutputter::rotateLogs(bool forced, TimeValue now)
{
    // rotate the main log (if applicable)
    rotateMainLog(forced, now);

    // rotate the event / binary event logs (if applicable)
    for (size_t idx = 0; idx < EAArrayCount(mEventLogs); ++idx)
    {
        rotateEvent(forced, &mEventLogs[idx], now);
        rotateEvent(forced, &mHeat2Logs[idx], now);
    }
}

void LogOutputter::rotateMainLog(bool forced, TimeValue now)
{
    if (forced || now > mNextDailyRotation)
    {
        mMainLog.rotate(now);
    }
}

void LogOutputter::rotateEvent(bool forced, LogFileInfo* event, TimeValue now)
{
    if (forced)
    {
        event->rotate(now);
        return;
    }

    TimeValue eventRotationPeriod = event->getRotationPeriod();
    TimeValue lastRotationTime = event->getLastRotation();

    // check to see if there is a rotation period and if so rotate when it's time, else rotate every midnight GMT only
    if (eventRotationPeriod > 0)
    {
        if (now > (lastRotationTime + eventRotationPeriod))
        {
            event->rotate(now);
        }
    }
    else
    {
        if (now > mNextDailyRotation)
        {
            event->rotate(now);
        }
    }
}

void LogOutputter::configureEventLogging(EventLoggingType type, const EventsList& events)
{
    if (events.size() == 0)
    {
        // No filter provided so enable all categories
        setCategory(type, true, 0, -1);
    }
    else
    {
        bool categoryStates[Log::CATEGORY_MAX];
        memset(categoryStates, 0, sizeof(categoryStates));

        TimeValue categoryRotationPeriod[Log::CATEGORY_MAX];
        eastl::fill_n(categoryRotationPeriod, Log::CATEGORY_MAX, 0);

        EventsList::const_iterator it = events.begin();
        for (; it != events.end(); ++it)
        {
            int32_t catIdx = -1;
            for (int32_t idx = 0; (idx < Log::CATEGORY_MAX) && (catIdx == -1); ++idx)
            {
                if (blaze_stricmp(it->get()->getCategory(), Logger::mCategories[idx].mName) == 0)
                    catIdx = idx;
            }

            if (catIdx != -1)
            {
                categoryStates[catIdx] = true;
                categoryRotationPeriod[catIdx] = it->get()->getRotationPeriod();
            }
            else
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].initialize: Ignoring unrecognized logging category specified for event filter: '" << it->get()->getCategory() << "'");
            }
        }

        // Enable/disable each category and set the period
        for (int32_t idx = 0; idx < Log::CATEGORY_MAX; ++idx)
            setCategory(type, categoryStates[idx], categoryRotationPeriod[idx], idx);
    }
}

void LogOutputter::getStatus(LoggerMetrics& metrics)
{
    LoggerMetrics::CategoriesMap& categories = metrics.getCategories();

    sLoggerMetrics->mCategoryMetrics.iterate([&categories](const Metrics::TagPairList& tags, const Metrics::Counter& value) {
        const char8_t* category = tags[0].second.c_str();
        LoggerMetrics::LevelsMetricsMap* levelMap;
        auto levelMapItr = categories.find(category);
        if (levelMapItr != categories.end())
        {
            levelMap = levelMapItr->second;
        }
        else
        {
            levelMap = categories.allocate_element();
            categories[category] = levelMap;
        }
        const char8_t* level = tags[1].second.c_str();
        (*levelMap)[level] = value.getTotal();
    });

    LoggerMetrics::BehaviourMetricsList& behaviourMetrics = metrics.getBehaviours();
    behaviourMetrics.reserve(mNumBehaviours);
    for (size_t idx = 0; idx < mNumBehaviours; ++idx)
    {
        LoggerMetrics::BehaviourMetrics* behaviour = behaviourMetrics.pull_back();
        mBehaviours[idx].getStatus(*behaviour);
    }
}


void LogOutputter::run()
{
    rotateLogs(true);
    mNextDailyRotation = getNextDailyRotationTime();

    mNextAuditLogRefresh = TimeValue::getTimeOfDay() + mAuditLogRefreshInterval;

    while (true)
    {
        mActiveLock.Lock();

        while (true)
        {
            if (!mActive[mQueueList].empty() || mShutdown)
            {
                // Got something to process to switch the queues and start draining
                int32_t tmp = mQueueList;
                mQueueList = mDrainList;
                mDrainList = tmp;
                break;
            }

            mActiveLock.Unlock();

            refreshAuditLogs(false, TimeValue::getTimeOfDay());

            mActiveLock.Lock();

            mActiveCond.Wait(&mActiveLock, EA::Thread::GetThreadTime() + 50);
        }

        mActiveLock.Unlock();

        TimeValue now = TimeValue::getTimeOfDay();

        rotateLogs(false, now);

        refreshAuditLogs(false, now);

        // update mNextDailyRotation when applicable after all log rotations
        if (now > mNextDailyRotation)
        {
            mNextDailyRotation = getNextDailyRotationTime();
        }

        drainEntries();

        if (mShutdown)
            break;

        if (mReconfigure)
            reconfigure();
    }

    refreshAuditLogs(true, 0); // force close all audit logs on this thread since it is the best way to do it without locking
}

void LogOutputter::refreshAuditLogs(bool force, TimeValue now)
{
    if (force || (now >= mNextAuditLogRefresh))
    {
        mNextAuditLogRefresh = now + mAuditLogRefreshInterval;

        for (auto& auditLog : mAuditLogs)
        {
            auditLog.second.close();
        }
        mAuditLogs.clear();
    }
}

TimeValue LogOutputter::getNextDailyRotationTime() const
{
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();

    struct tm tM;
    TimeValue::gmTime(&tM, t);
    tM.tm_hour = 23;
    tM.tm_min = 59;
    tM.tm_sec = 59;

    return TimeValue(TimeValue::mkgmTime(&tM, true) * 1000000 + 999999);
}

void LogOutputter::drainEntries()
{
    size_t freedEntriesCount = 0;
    EntryList freeEntriesList;
    LogFileInfoList flushList;
    bool flushStdout = false;
    EntryList::iterator itr = mActive[mDrainList].begin();
    EntryList::iterator end = mActive[mDrainList].end();
    for (; itr != end; )
    {
        LogEntry& entry = *itr;
        ++itr; //advance the iterator now in case we modify the list in place (see below in DROPPED)

        switch (entry.mType)
        {
        case LogEntry::DROPPED:
        {
            char8_t droppedMsg[256];
            CREATE_TIMESTAMP(timestamp);
            int32_t written = blaze_snzprintf(droppedMsg, sizeof(droppedMsg),
                "%s Log output rate exceeded; %d log entries dropped.\n", timestamp,
                mDropped.mSize);
            if (BlazeShouldLogToDebugger())
            {
                BlazeOutputDebugString(droppedMsg, 0);
            }
            if (Logger::mMainLogToStdout)
            {
                fwrite(droppedMsg, 1, written, stdout);
                flushStdout = true;
            }
            if (Logger::mMainLogToFile)
            {
                mMainLog.outputData(droppedMsg, written, flushList, true);
            }
            mActiveLock.Lock();
            EntryList::remove(mDropped);
            mDropped.mSize = 0;
            mActiveLock.Unlock();
            break;
        }

        case LogEntry::NORMAL:
        {
            if (mPrefixMultilineEnabled)
            {
                // The prefix of all the log lines is the first entry.mPrefixLength bytes of mData
                const auto actualPrefixLength = entry.mPrefixLength - 1; // prefix length, not including 0x0 byte
                auto* prefix = entry.mData;
                auto* token = entry.mData + entry.mPrefixLength; // token points to start of log line
                auto* dataEnd = entry.mData + entry.mCount - 1; // dataEnd points to last 0x0 byte in entry.mData
                do
                {
                    auto* tokenEnd = strchr(token, '\n');
                    if (tokenEnd == nullptr)
                        tokenEnd = dataEnd - 1; // NOTE: This should never happen, handle case if entry.mData contains internal 0x0 bytes.

                    auto actualTokenLength = (uint32_t)(tokenEnd - token + 1); // actual token length (includes newline)
                    if (actualTokenLength > 1)
                    {
                        if (BlazeShouldLogToDebugger())
                        {
                            BlazeOutputDebugString(prefix, actualPrefixLength);
                            BlazeOutputDebugString(token, actualTokenLength);
                        }
                        if (Logger::mMainLogToStdout)
                        {
                            fwrite(prefix, 1, actualPrefixLength, stdout);
                            fwrite(token, 1, actualTokenLength, stdout);
                            flushStdout = true;
                        }
                        if (Logger::mMainLogToFile)
                        {
                            mMainLog.outputData(prefix, actualPrefixLength, flushList, false);
                            mMainLog.outputData(token, actualTokenLength, flushList, true);
                        }
                        if (Logger::mOtherLogsToFile)
                        {
                            for (auto auditId : entry.mAuditIds)
                            {
                                auto* auditLog = getOrCreateAuditLogFileInfo(auditId);
                                if (auditLog != nullptr)
                                {
                                    auditLog->outputData(prefix, actualPrefixLength, flushList, false);
                                    auditLog->outputData(token, actualTokenLength, flushList, false); // Audit log rotation disabled for now
                                }
                            }
                        }
                    }
                    else if ((actualTokenLength == 1) && (*token == '\n'))
                    {
                        // NOTE: Deliberately skip repeated newlines in multiline log entries. 
                        //       This was the behavior of the previous logger implementation; 
                        //       however, we shall be adding a mode to insert a greppable warning
                        //       that shall expose the problem to developers and facilitate fixing
                        //       offending call sites in the code that generate unecessary newline
                        //       repeats.
                    }
                    else
                        break; // This should never happen unless log data becomes corrupt but handle it gracefully anyway.

                    token += actualTokenLength; // consume token

                } while (token < dataEnd);
            }
            else
            {
                if (BlazeShouldLogToDebugger())
                {
                    BlazeOutputDebugString(entry.mData, 0);
                }
                if (Logger::mMainLogToStdout)
                {
                    fwrite(entry.mData, 1, entry.mCount, stdout);
                    flushStdout = true;
                }
                if (Logger::mMainLogToFile)
                {
                    mMainLog.outputData(entry.mData, entry.mCount, flushList, true);
                }
                if (Logger::mOtherLogsToFile)
                {
                    for (auto auditId : entry.mAuditIds)
                    {
                        auto* auditLog = getOrCreateAuditLogFileInfo(auditId);
                        if (auditLog != nullptr)
                        {
                            auditLog->outputData(entry.mData, entry.mCount, flushList, false); // Audit log rotation disabled for now
                        }
                    }
                }
            }
            break;
        }

        case LogEntry::EVENT:
        {
            if (Logger::mOtherLogsToFile)
            {
                for (auto auditId : entry.mAuditIds)
                {
                    auto* auditLog = getOrCreateAuditLogFileInfo(auditId);
                    if (auditLog != nullptr)
                    {
                        auditLog->outputData(entry.mData, entry.mCount, flushList, false); // Audit log rotation disabled for now
                    }
                }
                mEventLogs[entry.mEventCategory].outputData(entry.mData, entry.mCount, flushList, true);
            }
            break;
        }

        case LogEntry::TDF:
        {
            if (Logger::mOtherLogsToFile)
            {
                mHeat2Logs[entry.mEventCategory].outputData(entry.mData, entry.mCount, flushList, true);
            }
            break;
        }

        case LogEntry::UNSET:
            break;
        }

        if (&entry != &mDropped)
        {
            entry.mAuditIds.clear();
            EntryList::remove(entry);
            freeEntriesList.push_back(entry);
            ++freedEntriesCount;
        }

        if ((freedEntriesCount % ENTRY_RELEASE_BATCH_SIZE) == 0)
        {
            // Release drained log entries incrementally to avoid starving the queueing side unnecessarily.
            // (We free a batch of entries at once to avoid paying the locking overhead for too few entries)
            auto freeEntryItr = freeEntriesList.begin();
            auto freeEntryEnd = freeEntriesList.end();
            mFreeLock.Lock();
            while (freeEntryItr != freeEntryEnd)
            {
                auto& freeEntry = *freeEntryItr;
                ++freeEntryItr;
                releaseEntry(freeEntry);
            }
            mFreeLock.Unlock();
            freeEntriesList.clear();
        }
    }

    EA_ASSERT(mActive[mDrainList].empty()); // all entries have been transferred to the freedEntryList
    mActiveSizes[mDrainList] = 0;

    // Release all the entries back to the pool
    auto freeEntryItr = freeEntriesList.begin();
    auto freeEntryEnd = freeEntriesList.end();
    mFreeLock.Lock();
    while (freeEntryItr != freeEntryEnd)
    {
        auto& freedEntry = *freeEntryItr;
        ++freeEntryItr;
        releaseEntry(freedEntry);
    }
    mFreeLock.Unlock();

    if (flushStdout)
        fflush(stdout);

    while (!flushList.empty())
    {
        auto& fileLog = flushList.front();
        flushList.pop_front();
        fileLog.flush();
    }
}

int32_t LogOutputter::log_snzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, ...) const
{
    int32_t iResult;
    va_list args;

    // make sure there's room for null termination
    if (uLength == 0) return(0);

    // format the text
    va_start(args, pFormat);
    iResult = vsnprintf(pBuffer, uLength, pFormat, args);
    va_end(args);

    return iResult;
}

int32_t LogOutputter::log_vsnzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, va_list Args) const
{
    int32_t iResult;

    // make sure there's room for null termination
    if (uLength == 0) return(0);

    // format the text
    iResult = vsnprintf(pBuffer, uLength, pFormat, Args);

    return iResult;
}

bool LogOutputter::logToEntry(LogEntry& entry, const char8_t* timestamp, int32_t category,
    Logging::Level level, const char* file, int32_t line, bool hasVaList,
    const char* message, va_list args) const
{
    level = Logger::getRemappedLevel(level);

    bool bIsTruncated = false;
    const char8_t* threadName = (gCurrentThread == nullptr) ? "<main>" : gCurrentThread->getName();

    int32_t written = blaze_snzprintf(
        entry.mData,
        entry.mSize,
        "%s %-6.6s %-12.16s %-*s [%16.16" PRIx64 "/%16.16" PRIx64 "/%20.20" PRId64 "]  ",
        timestamp,
        Logging::LevelToString(level),
        threadName,
        Logger::mMaxCategoryLength,
        Logger::mCategories[category].mName,
        gLogContext->getSlaveSessionId(),
        gLogContext->getUserSessionId(),
        gLogContext->getBlazeId()
    );

    entry.mPrefixLength = written;

    if (Logger::mIncludeLocation && (file != nullptr))
    {
        if (Logger::mEncodeLocation)
        {
            uint32_t filehash = EA::StdC::FNV1(file, strlen(file));
            written += blaze_snzprintf(entry.mData + written, entry.mSize - written, "[%8.8x:%4.4x] - ", filehash, line);
        }
        else
        {
            written += blaze_snzprintf(entry.mData + written, entry.mSize - written, "[%s:%d] - ", file, line);
        }
    }

    int32_t w;
    // use log_vsnzprintf() and log_snzprintf() instead of the blaze_vsnprintf() and blaze_snzprintf() methods
    // so that on overflow, we print the truncated log message on linux instead of no log message at all
    // on windows, _vsnprintf() will return negative values for buffer ovrerun as well as any failure in parameter validation
    // whereas on linux, negative values will only be returned on output error
    if (hasVaList)
    {
        w = log_vsnzprintf(entry.mData + written, entry.mSize - written, message, args);
    }
    else
    {
        if ((level & (Logging::FATAL | Logging::FAIL | Logging::ERR | Logging::WARN)) && Fiber::isCurrentlyCancelled())
        {
            // If we're logging a WARN or something more severe *AND* the current fiber is canceled, it's very likely that
            // the log entry being written here saying something about why or what just failed with ERR_TIMEOUT, for example.
            // In this case, we add a bunch of context to the log line to disambiguate it from other logs that might be
            // reporting other things that failed.  The intention is to let the user know exactly why this fiber was canceled.
            char8_t timeBuf[64] = "";
            TimeValue eventDuration = Fiber::getCurrentCancelTime() - Fiber::getCurrentCanceledEventStartTime();
            w = log_snzprintf(entry.mData + written, entry.mSize - written,
                "%s [fiber was canceled while calling \"%s\"; blocked event \"%s\" returned %s after %s]",
                message,
                Fiber::getCurrentCanceledWhileCallingContext(),
                Fiber::getCurrentCanceledEventContext(),
                Fiber::getCurrentCancelReason().c_str(),
                eventDuration.toIntervalString(timeBuf, sizeof(timeBuf)));
        }
        else
        {
            w = log_snzprintf(entry.mData + written, entry.mSize - written, "%s", message);
        }
    }

    if (w < 0)
    {
        // encoding error from snprintf() or vsnprintf(), just print without the message
        // or buffer overrun on windows
        w = 0;
        bIsTruncated = true;
    }

    written += w;
    if (written >= (int32_t)(entry.mSize - 1))
    {
        // message was larger than data buffer so it was truncated
        // printing out truncated log is better than no log at all
        // so let's just print out as much as we can
        bIsTruncated = true;
        // save room for the '\n' and '\0'
        written = entry.mSize - 2;
    }

    if (written > 0)
    {
        entry.mData[written++] = '\n';
        entry.mData[written++] = '\0';
    }
    entry.mCount = written;
    entry.mType = LogEntry::NORMAL;
    entry.mLevel = level;
    entry.mEventCategory = category;
    entry.mAuditIds = gLogContext->getAuditIds();
    return (written > 0 && !bIsTruncated);
}

void LogOutputter::logDrop()
{
    // The logger is backed up and there aren't any free entries.  Tick up the total
    // number of dropped messages.  The log outputter will generate an appropriate
    // message to the log file indicating the number of lost messages
    mActiveLock.Lock();
    mDropped.mSize++;
    if (mDropped.mSize == 1)
    {
        mActive[mQueueList].push_back(mDropped);
        ++mActiveSizes[mQueueList];
    }
    mActiveLock.Unlock();
}

LogOutputter::LogEntryBehavioursData& LogOutputter::getBehaviours(const char8_t* file, int32_t line)
{
    for (size_t idx = 0; idx < mNumBehaviours; ++idx)
    {
        if (mBehaviours[idx].isMatch(file, line))
            return mBehaviours[idx];
    }
    return mDefaultBehaviours;
}

LogFileInfo* LogOutputter::getOrCreateAuditLogFileInfo(AuditId auditId)
{
    LogFileInfo* logFileInfo = nullptr;
    // first find the audit log in the outputter's private loginfo cache (without locking).
    auto auditLogItr = mAuditLogs.find(auditId);
    if (auditLogItr != mAuditLogs.end())
    {
        logFileInfo = &auditLogItr->second;
    }
    else
    {
        // Otherwise get it form the Logger.
        auto ret = mAuditLogs.insert(auditId);
        auto& auditLog = ret.first->second;
        if (ret.second)
        {
            eastl::string fileName;
            Logger::getAuditLogFilename(auditId, fileName);
            auditLog.initialize(fileName.c_str());
            auditLog.setState(true);
        }
        logFileInfo = &auditLog;
    }
    return logFileInfo;
}


void ExternalThreadLogEntryJob::execute()
{
    switch (mLogEntry.mLevel)
    {
    case Blaze::Logging::SPAM:
        BLAZE_SPAM_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::TRACE:
        BLAZE_TRACE_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::INFO:
        BLAZE_INFO_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::WARN:
        BLAZE_WARN_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::ERR:
        BLAZE_ERR_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::FAIL:
        BLAZE_FAIL_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    case Blaze::Logging::FATAL:
        BLAZE_FATAL_LOG(mLogEntry.mCategory, mLogEntry.mMessage);
        break;

    default:
        BLAZE_ASSERT_COND_LOG(mLogEntry.mCategory, false, "Add support for unhandled core log category");
        break;
    }
}

void ExternalThreadLogOutputter::stop()
{
    if (!mShutdown)
    {
        mShutdown = true;
        mJobQueue.wakeAll();
    }
    waitForEnd();
}

void ExternalThreadLogOutputter::run()
{
    while (!mShutdown)
    {
        Job* job = mJobQueue.popOne(-1);
        if (!mShutdown && job != nullptr)
        {
            job->execute();
            delete job;
        }
    }
}

void ExternalThreadLogOutputter::pushLogEntry(const ExternalThreadLogEntry& logEntry)
{
    mJobQueue.push(BLAZE_NEW ExternalThreadLogEntryJob(logEntry));
}

}
