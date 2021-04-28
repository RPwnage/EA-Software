/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/riverpinutil.h"
#include "framework/util/shared/rawbufferistream.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/system/threadlocal.h"
#include "framework/util/directory.h"
#include "framework/util/timer.h"

#include "EATDF/codec/tdfjsonencoder.h"

#include "EAIO/EAFileBase.h"
#include "EAIO/PathString.h"
#include "EAIO/EAFileUtil.h"

#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#endif
#include <fcntl.h>
#include <regex>


namespace Blaze
{

RiverPinUtil::RiverPinUtil()
    : BlazeThread("pinlog", BlazeThread::PINLOG, BUFFER_SIZE_1024K * 2)
    , mRequestJobQueue("RiverPinUtil::mRequestJobQueue")
    , mThreadId(EA::Thread::kThreadIdInvalid)
    , mStopping(false)
    , mFiberManager(gFiberManager)
    , mFileDescriptor(-1)
    , mResponseTimes(40)
    , mAverageResponseTime(0)
    , mMaxRequestsPerSecond(0)
    , mCurrentRequestsPerSecond(0)
{
}

eastl::string makeHeader(const char8_t* name, const char8_t* value)
{
    return eastl::string(eastl::string::CtorSprintf(), "%s: %s", name, value);
}
bool RiverPinUtil::configure(const PINConfig& pinConfig)
{
    HttpConnectionManagerPtr connMgr = gOutboundHttpService->getConnection("riverposter");
    if (connMgr == nullptr)
    {
        BLAZE_WARN_LOG(Log::PINEVENT, "[RiverPinUtil].configure: No HTTP service with the name 'river' is configured. PIN event data will not be sent to River.");
        return false;
    }

    mRiverConnManager = connMgr;

    mHttpHeader_x_ea_lint_level = makeHeader("x-ea-lint-level", "2");
    mHttpHeader_application_id = makeHeader("application-id", pinConfig.getApplicationId());
    mHttpHeader_x_ea_env = makeHeader("x-ea-env", pinConfig.getEnvironment());
    mHttpHeader_x_ea_taxv = makeHeader("x-ea-taxv", pinConfig.getPINTaxonomyVersion());
    mHttpHeader_x_ea_game_id = makeHeader("x-ea-game-id", pinConfig.getEaGameId());
    mHttpHeader_x_ea_game_id_type = makeHeader("x-ea-game-id-type", "edw_master_ttl_id");

    mHttpHeaders = {
        mHttpHeader_x_ea_lint_level.c_str(),
        mHttpHeader_application_id.c_str(),
        mHttpHeader_x_ea_env.c_str(),
        mHttpHeader_x_ea_taxv.c_str(),
        mHttpHeader_x_ea_game_id.c_str(),
        mHttpHeader_x_ea_game_id_type.c_str()
    };

    mMaxRequestsPerSecond = pinConfig.getMaxRequestsPerSecond();
    mCurrentRequestsPerSecond = 20;

    // Explicitly limit the number of requests that can be queued to
    // the same number of connections in the 'river-poster' HTTP service.
    mRequestJobQueue.setMaximumWorkerFibers(mRiverConnManager->maxConnectionsCount());

    //mRequestJobQueue.setJobQueueCapacity(mRiverConnManager->maxConnectionsCount() * 5); // shouldn't be needed

    return true;
}

void RiverPinUtil::sendPinData(RawBufferPtr pinDataBuffer)
{
    // Queue up the request
    mRequestJobQueue.queueFiberJob(this, &RiverPinUtil::sendPinDataInternal, pinDataBuffer, "RiverPinUtil::sendPinDataInternal");
}

void RiverPinUtil::checkCpuThresholds()
{
    if (TimeValue::getTimeOfDay() - mLastCheckCpuThresholds > (5 * 1000 * 1000))
    {
        mLastCheckCpuThresholds = TimeValue::getTimeOfDay();
        if (gFiberManager->getCpuUsageForProcessPercent() > 80)
        {
            if (mCurrentRequestsPerSecond > 15)
                mCurrentRequestsPerSecond -= 15;
        }
        else
        {
            if (mCurrentRequestsPerSecond < mMaxRequestsPerSecond)
                mCurrentRequestsPerSecond += 5;
        }
    }
}

void RiverPinUtil::sendPinDataInternal(RawBufferPtr pinDataBuffer)
{
    int32_t requestAttemptCounter = 0;
    bool shouldRetryRequest;
    Timer requestTimer;

    if (mRequestJobQueue.isJobQueueEmpty())
    {
        Fiber::signal(mRequestJobQueueWorkerAvailableEventHandle, ERR_OK);
    }

    checkCpuThresholds();

    // For sanity
    if (mCurrentRequestsPerSecond <= 0)
        mCurrentRequestsPerSecond = 1;

    const EA::TDF::TimeValue now = TimeValue::getTimeOfDay();
    const EA::TDF::TimeValue rpsDelay = mPreferredNextRequestTime - now;
    const EA::TDF::TimeValue requestRateInterval = (1 * 1000 * 1000) / mCurrentRequestsPerSecond;
    if (rpsDelay <= 0)
        mPreferredNextRequestTime = now + requestRateInterval;
    else
    {
        mPreferredNextRequestTime += requestRateInterval;
        Fiber::sleep(rpsDelay, "RiverPinUtil::sendPinDataInternal - extra delay");
    }

    do
    {
        if (mRiverConnManager == nullptr)
        {
            BLAZE_ASSERT_LOG(Log::PINEVENT, "[RiverPinUtil].sendPinDataInternal: There is no connection to River.");
            break;
        }

        const char8_t* resourcePathFormatted = "/pinEvents";
        const char8_t* resourcePathMetric = resourcePathFormatted+1; // The +1 is to exclude the '/'
        gController->getExternalServiceMetrics().incCallsStarted(mRiverConnManager->getName(), resourcePathMetric);
        gController->getExternalServiceMetrics().incRequestsSent(mRiverConnManager->getName(), resourcePathMetric);

        ++requestAttemptCounter;

        requestTimer.start();
        RawOutboundHttpResult result;
        BlazeError err = mRiverConnManager->sendRequest(HttpProtocolUtil::HTTP_POST, resourcePathFormatted,
            mHttpHeaders.data(), mHttpHeaders.size(), &result, "application/json",
            (const char8_t*)pinDataBuffer->data(), (uint32_t)pinDataBuffer->size());
        requestTimer.stop();

        if ((err == ERR_OK) && (result.getHttpStatusCode() == 0))
            err = ERR_COULDNT_CONNECT;

        gController->getExternalServiceMetrics().incCallsFinished(mRiverConnManager->getName(), resourcePathMetric);

        Logging::Level logLevel;
        eastl::string statusString;
        if (err == ERR_OK)
        {
            statusString.append_sprintf("HTTP_%" PRIu32, result.getHttpStatusCode());
            // Handle the HTTP status code as expected by River.
            switch (result.getHttpStatusCode())
            {
                case 200: // OK
                    logLevel = Logging::TRACE;
                    shouldRetryRequest = false;
                    break;
                case 429: // Too Many Requests
                case 500: // Internal Server Error
                case 503: // Service Unavailable
                    logLevel = Logging::WARN;
                    shouldRetryRequest = true;
                    break;
                default:
                    logLevel = Logging::ERR;
                    shouldRetryRequest = false;
                    break;
            }
        }
        else
        {
            statusString.append_sprintf("BLAZE_%s", LOG_ENAME(err));
            if ((err == ERR_TIMEOUT) || (err == ERR_COULDNT_CONNECT))
            {
                logLevel = Logging::WARN;
                shouldRetryRequest = true;
            }
            else
            {
                logLevel = Logging::ERR;
                shouldRetryRequest = false;
            }
        }

        gController->getExternalServiceMetrics().incResponseCount(mRiverConnManager->getName(), resourcePathMetric, statusString.c_str());
        gController->getExternalServiceMetrics().recordResponseTime(mRiverConnManager->getName(), resourcePathMetric, statusString.c_str(), requestTimer.getInterval().getMicroSeconds());

        mResponseTimes.push_back(requestTimer.getInterval().getMicroSeconds());
        mAverageResponseTime = 0;
        for (const auto responseTime : mResponseTimes)
            mAverageResponseTime += responseTime;
        mAverageResponseTime /= mResponseTimes.size();

        EA::TDF::TimeValue delay = (100 * 1000) * gRandom->getRandomNumber((1 << requestAttemptCounter) - 1);

        char8_t intervalBuf[64];
        BLAZE_LOG(logLevel, Log::PINEVENT, "[RiverPinUtil].sendPinDataInternal: River request " << (err == ERR_OK && result.getHttpStatusCode() == 200 ? "succeeded" : "failed") << ": "
            "attempt=" << requestAttemptCounter << "; "
            "err=" << err << "; "
            "response code=" << result.getHttpStatusCode() << "; "
            "next attempt=" << (shouldRetryRequest ? delay.toIntervalString(intervalBuf, sizeof(intervalBuf)) : "never") << "\n"
            "    request data: " << (const char8_t*)pinDataBuffer->data() << "\n"
            "    response data: " << (const char8_t*)result.getBuffer().data());

        if (shouldRetryRequest)
        {
            Fiber::sleep(delay, "RiverPinUtil::sendPinDataInternal");
        }

    } while (shouldRetryRequest && !Fiber::isCurrentlyCancelled());

    //// Code for a busy wait that intentionally uses CPU so we can debug this algorithm's auto-tunning functionality.
    //TimeValue now2 = TimeValue::getTimeOfDay();
    //while (TimeValue::getTimeOfDay() - now2 < 10 * 1000);
}

void RiverPinUtil::fillRequestQueue()
{
    mMutex.Lock();

    if (!mRequestJobQueue.isWorkerAvailable())
    {
        Fiber::getAndWait(mRequestJobQueueWorkerAvailableEventHandle, "RiverPinUtil::fillRequestQueue");
        mRequestJobQueueWorkerAvailableEventHandle = Fiber::INVALID_EVENT_ID;
    }

    if (!mStopping)
    {
        for (const RawBufferPtr& rawBuffer : mRawBufferPtrList)
            sendPinData(rawBuffer);
        mRawBufferPtrList.clear();
    }

    mCond.Signal();
    mMutex.Unlock();
}
void RiverPinUtil::awaitFillRequestQueue()
{
    mMutex.Lock();
    mFiberManager->scheduleCall(this, &RiverPinUtil::fillRequestQueue, "RiverPinUtil::fillRequestQueue");
    mCond.Wait(&mMutex);
    mMutex.Unlock();
}

void RiverPinUtil::drainRequestQueue()
{
    mMutex.Lock();

    if (!mStopping)
    {
        mRequestJobQueue.join();
    }

    mCond.Signal();
    mMutex.Unlock();
}
void RiverPinUtil::awaitDrainRequestQueue()
{
    mMutex.Lock();
    mFiberManager->scheduleCall(this, &RiverPinUtil::drainRequestQueue, "RiverPinUtil::drainRequestQueue");
    mCond.Wait(&mMutex);
    mMutex.Unlock();
}

EA::Thread::ThreadId RiverPinUtil::start()
{
    if (mThreadId == EA::Thread::kThreadIdInvalid)
    {
        mMutex.Lock();
        mThreadId = BlazeThread::start();
        if (mThreadId != EA::Thread::kThreadIdInvalid)
        {
            mCond.Wait(&mMutex);
        }
        mMutex.Unlock();
    }

    return mThreadId;
}

void RiverPinUtil::stop()
{
    if (mThreadId != EA::Thread::kThreadIdInvalid)
    {
        mThreadStoppedEventHandle = Fiber::getNextEventHandle();

        mStopping = true;
        mRequestJobQueue.cancelAllWork();
        Fiber::signal(mRequestJobQueueWorkerAvailableEventHandle, ERR_CANCELED);
        Fiber::wait(mThreadStoppedEventHandle, "RiverPinUtil::stop");
        waitForEnd();
        mStopping = false;

        mThreadStoppedEventHandle.reset();
        mThreadId = EA::Thread::kThreadIdInvalid;
    }
}

void RiverPinUtil::run()
{
    // Example path: /opt/home/fifa/fifa-2021-ps4/log/pin_events/blaze_mmSlave575_pinevent_20201122_205131-942.log
    // Regex: "^.*[/\\][\w\d]+?_pinevent_\d{8}_\d{6}-\d+\.log$"
    const std::regex pinLogFilenameRegex("^.*[/\\\\][\\w\\d]+?_pinevent_\\d{8}_\\d{6}-\\d+\\.log$");

    mMutex.Lock();
    mCond.Signal();
    // Main thread will continue executing after unlocking the mutex.^
    mMutex.Unlock();

    char8_t buffer[BUFFER_SIZE_1024K];

    while (!mStopping)
    {
        StringBuilder pinEventsFolder;
        pinEventsFolder << Logger::getLogDir() << EA_FILE_PATH_SEPARATOR_STRING_8 << Logger::BLAZE_LOG_PIN_EVENTS_DIR;
        Directory::FilenameList files;
        Directory::getDirectoryFilenames(pinEventsFolder.c_str(), Logger::BLAZE_LOG_FILE_EXTENSION, files);

        for (Directory::FilenameList::iterator it = files.begin(), end = files.end(); !mStopping && (it != end); it++)
        {
            const eastl::string& filename = *it;
            if (!std::regex_match(filename.c_str(), pinLogFilenameRegex))
                continue;

            if (openLogFile(filename.c_str()))
            {
                RawBuffer* line = nullptr;
                int32_t bufferOffset = -1;
                int32_t bytesRead = -1;
                int32_t counter = 0;
                char8_t c;

                while (!mStopping)
                {
                    if (bufferOffset == bytesRead)
                    {
                        bufferOffset = 0;
                        bytesRead = read(mFileDescriptor, buffer, sizeof(buffer));
                        if (bytesRead <= 0) // EOF or Error
                            break;
                    }

                    c = buffer[bufferOffset++];
                    if (c == '\n')
                    {
                        counter = 0;
                        if (line != nullptr)
                        {
                            mRawBufferPtrList.push_back(line);
                            if (mRawBufferPtrList.size() > 50)
                                awaitFillRequestQueue();
                            line = nullptr;
                        }
                    }
                    else if (c != '\r')
                    {
                        if (counter++ < BUFFER_SIZE_64K)
                        {
                            if (line == nullptr)
                                line = BLAZE_NEW RawBuffer(BUFFER_SIZE_16K);
                            *line->acquire(2) = c;
                            *line->put(1) = '\0';
                        }
                        else if (line != nullptr)
                        {
                            delete line;
                            line = nullptr;
                        }
                    }
                }

                delete line;

                if (!mRawBufferPtrList.empty())
                    awaitFillRequestQueue();
                awaitDrainRequestQueue();

                if (bytesRead == 0)
                {
                    if (!EA::IO::File::Remove(filename.c_str()))
                    {
                        // Handle error case
                    }
                    //eastl::string newFilename = filename.left(filename.size() - strlen(BLAZE_LOG_FILE_EXTENSION_ALT));
                    //newFilename.append(BLAZE_LOG_FILE_EXTENSION);
                    //if (!EA::IO::File::Rename(filename.c_str(), newFilename.c_str()))
                    //{
                    //    // Handle error case
                    //}
                }
                else
                {
                    // Handle error case
                }

                closeLogFile();
            }
        }

        if (!mStopping)
            EA::Thread::ThreadSleep(1000/*ms*/);
    }

    Fiber::signal(mThreadStoppedEventHandle, ERR_OK);
}

bool RiverPinUtil::openLogFile(const char8_t* filename)
{
#ifdef EA_PLATFORM_WINDOWS
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return false;

    mFileDescriptor = _open_osfhandle((intptr_t)fileHandle, O_RDWR);
    if (mFileDescriptor == -1)
    {
        CloseHandle(fileHandle);
        return false;
    }
#else
    flock fl;
    memset(&fl, 0, sizeof fl);
    //for the entire file
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    mFileDescriptor = open(filename, O_RDWR, S_IWOTH | S_IROTH | S_IWGRP | S_IRGRP | S_IWUSR | S_IRUSR);
    if (mFileDescriptor == -1)
        return false;

    // Try to get an exclusive (write) lock on the file so that we're guaranteed to be the only one processing it right now.
    // Failure to get the lock is an expected condition.  It simply means that another process is processing the file.
    fl.l_type = F_WRLCK;
    int rc = fcntl(mFileDescriptor, F_SETLK, &fl);
    if (rc == -1)
    {
        closeLogFile();
        return false;
    }
#endif
    // We've got exclusive access to the file pointed to by mFileDescriptor.
    return true;
}

void RiverPinUtil::closeLogFile()
{
    if (mFileDescriptor != -1)
    {
        // POSIX close() will cleanup all system resources associated with the fd, including the Windows file HANDLE and the Linux file lock.
        close(mFileDescriptor);
        mFileDescriptor = -1;
    }
}

} // Blaze
