#include "DownloadRateLimiter.h"
#include "DownloadServiceInternal.h"
#include "services/log/LogService.h"
#include "services/downloader/Timer.h"

#include <limits>

namespace Origin
{
namespace Downloader
{
    // Constants used by the rate limiting algorithms
    const qint64 kRateLimiterNominalRate = static_cast<qint64>(2.5*1024*1024); // 2.5 MB/s - Nominal 'max' rate, utilization will be a percentage of this
    const qint64 kRateLimiterUnlimitedRate = 1024*1024*1024; // 1 GB/s - Effectively unlimited rate
    const qint64 kRateLimiterMaxQueueAge = 2000;    // since we are setting our chunk sizes to half the nominal rate that works out to basically 2 chunks per second but
                                                    // we add a buffer of 1 second for the occasional bunched up chunks that can get released in consecutive ticks.

    DownloadRateLimiter::DownloadRateLimiter(DownloadSession *session) : 
        _session(session),
        _lock(QMutex::Recursive),
        _lastChunkTime(GetTick()),
        _lastDesiredBps(0),
        _rateSampler(10)
    {
    
    }

    DownloadRateLimiter::~DownloadRateLimiter()
    {

    }

    void DownloadRateLimiter::chunkAvailable(RequestId reqId, qint64 chunkSize)
    {
        QMutexLocker lock(&_lock);

        // Stick it in the queue
        DownloadRateLimiter::AvailableChunk chunk;
        chunk.reqId = reqId;
        chunk.chunkSize = chunkSize;
        chunk.tickReceived = GetTick();

        _availableChunks.enqueue(chunk);

        // Tick the queue
        tick();
    }

    void DownloadRateLimiter::tick()
    {
        QMutexLocker lock(&_lock);

    #ifdef DEBUG_RATE_LIMITER
        ORIGIN_LOG_DEBUG << "[Download Rate Limiter] Tick";
    #endif

        // General algorithm:
        //    Look at the head of the available chunk queue.  If sending out this chunk
        //    would push us over our desired Bps, we will do it next time around.
        //    Otherwise, add it to the dispatch list and keep trying to add chunks
        //    until we push ourselves over our desired Bps limit.
        //    After we build a list of chunks to dispatch, send them out and update
        //    our internal timers and counters.

        // Call this before our early return so we get logging of the changes in download utilization even if nothing was queued up
        qint64 desiredBps = getIdealBps();

        if (_availableChunks.isEmpty())
            return;

        qint64 thisTick = GetTick();
        float utilization = DownloadService::GetDownloadUtilization();
        qint64 oldestAllowableQueueTimestamp = 0;
        if (utilization > 0.0 && utilization <= 1.0)
        {
            oldestAllowableQueueTimestamp = thisTick - kRateLimiterMaxQueueAge;
        }
        double newBps = std::numeric_limits<double>::max();

        qint64 bytesToDispatch = 0;
        QQueue<DownloadRateLimiter::AvailableChunk> chunksToDispatch;
        do
        {
            qint64 combinedChunkSize = _availableChunks.head().chunkSize + bytesToDispatch;

            // Figure out what the blended average would be if we included this new chunk
            newBps = _rateSampler.theoreticalNewBPS(combinedChunkSize, thisTick);
#ifdef DEBUG_RATE_LIMITER
            ORIGIN_LOG_DEBUG << "[Download Rate Limiter] newBps = " << (newBps / double(1024*1024)) << " desiredBps: " << (desiredBps / double(1024*1024))  << " bytesToDispatch: " << (bytesToDispatch)  << " combinedChunkSize: " << (combinedChunkSize) << " tick revd: " << _availableChunks.head().tickReceived << " oldest Allowable: " << oldestAllowableQueueTimestamp;
#endif  

            // If we're transferring slower than we would like OR if the chunk exceeds the max queue duration, we must release it
            if (newBps <= desiredBps || _availableChunks.head().tickReceived < oldestAllowableQueueTimestamp)
            {
                // Dequeue a chunk and move it to the dispatch list
                DownloadRateLimiter::AvailableChunk chunk = _availableChunks.dequeue();
                chunksToDispatch.enqueue(chunk);

                bytesToDispatch += chunk.chunkSize;
            }
        } while (newBps < desiredBps && !_availableChunks.empty());

        if (!chunksToDispatch.isEmpty())
        {
            // Update the last chunk time
            _lastChunkTime = thisTick;

            // Add the combined sample to the bps tracker
            _rateSampler.addSample(bytesToDispatch, thisTick);
        }

        while (!chunksToDispatch.isEmpty())
        {
            // Dequeue a chunk and dispatch it
            DownloadRateLimiter::AvailableChunk chunk = chunksToDispatch.dequeue();

            // Dispatch it to the listeners
            _session->limiterChunkAvailable(chunk.reqId);

    #ifdef DEBUG_RATE_LIMITER
        ORIGIN_LOG_DEBUG << "[Download Rate Limiter] Dispatched chunk, size in MB = " << (chunk.chunkSize / double(1024*1024)) << " Average MBps: " << (_rateSampler.averageBPS(thisTick) / double(1024*1024));
    #endif   
        }
    }

    qint64 DownloadRateLimiter::getIdealMaxChunkSize()
    {
        // We need the granularity to match the nominal rate ; i.e. ideally two chunks per second
        float utilization = std::min<float>(DownloadService::GetDownloadUtilization(),1.0f);
        qint64 maxChunkSize = utilization*0.5*kRateLimiterNominalRate;
        return maxChunkSize;
    }

    qint64 DownloadRateLimiter::getIdealBps()
    {
        float utilization = DownloadService::GetDownloadUtilization();

        bool unlimited = false;
        qint64 desiredBps = 0;

        // Sanity check, for values outside [0,1] we don't want to limit
        if (utilization > 1.0f || utilization < 0.0f)
        {
            unlimited = true;
            desiredBps = kRateLimiterUnlimitedRate; // 1 GB/s (unlimited)
        }
        else
        {
            desiredBps = utilization*kRateLimiterNominalRate; // 10MB/s * utilization (0.0-1.0)
        }

        // Did the desired BPS change?
        if (desiredBps != _lastDesiredBps)
        {
            if (unlimited)
            {
                ORIGIN_LOG_EVENT << "[Download Rate Limiter] New desired MBPS: UNLIMITED";
            }
            else
            {
                ORIGIN_LOG_EVENT << "[Download Rate Limiter] New desired MBPS: " << (desiredBps / double(1024*1024));
            }
        }

        // If the desired BPS goes down, we need to clear out the samples buffer so it can go down gracefully
        if (desiredBps < _lastDesiredBps)
        {
            _rateSampler.clear();
        }
        _lastDesiredBps = desiredBps;

        return desiredBps;
    }

}//Downloader
}//Origin