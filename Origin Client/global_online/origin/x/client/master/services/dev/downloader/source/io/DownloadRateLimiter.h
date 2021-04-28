#ifndef DOWNLOADSVC_RATELIMITER_H
#define DOWNLOADSVC_RATELIMITER_H

#include "services/downloader/DownloadService.h"
#include "services/downloader/TransferStats.h"

#include <QVector>
#include <QQueue>
#include <QTimer>
#include <QMutex>

namespace Origin
{
namespace Downloader
{
    class DownloadSession;

    // Provides rate-based limiting of download chunks
    class DownloadRateLimiter
    {
        struct AvailableChunk
        {
            AvailableChunk() :
                reqId(-1),
                chunkSize(0),
                tickReceived(0)
            {
            }

            RequestId reqId;
            qint64 chunkSize;
            qint64 tickReceived;
        };

    public:
        DownloadRateLimiter(DownloadSession *session);
        ~DownloadRateLimiter();

        void chunkAvailable(RequestId reqId, qint64 chunkSize);
        void tick();

        static qint64 getIdealMaxChunkSize();

    private:
        qint64  getIdealBps();
        
    private:
        DownloadSession*        _session;
        QMutex                  _lock;
        qint64                  _lastChunkTime;
        qint64                  _lastDesiredBps;
        QQueue<AvailableChunk>  _availableChunks;
        DownloadRateSampler     _rateSampler;
    };

}//Downloader
}//Origin

#endif //DOWNLOADSVC_RATELIMITER_H