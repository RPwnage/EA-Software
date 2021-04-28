#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include "services/downloader/Common.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QUrl>
#include <QMutex>

namespace Origin
{
namespace Downloader
{

class MemBuffer;
class IDownloadSession;
class IDownloadRequest;
class DownloadSession;

    typedef qint64 RequestId;

    class ORIGIN_PLUGIN_API DownloadService : public QObject
    {
        Q_OBJECT
            // Intended to be a factory which creates DownloadSession objects

        public:
            static IDownloadSession* CreateDownloadSession( QString const& url, const QString& id, bool isPhysicalMedia, Services::Session::SessionWRef session );

            static void DestroyDownloadSession(IDownloadSession* session);

            static int GetActiveDownloadSessionCount();

            static bool IsNetworkDownload(const QString& url);

            static void init();
            static void release();

            static float GetDownloadUtilization();
            static void SetGameRequestedDownloadUtilization(float utilization);
            static void SetDownloadUtilization(float utilization);

            static float GetDefaultGameRequestedDownloadUtilization();
            static float GetDefaultDownloadUtilization();
            static float GetDefaultPlayingDownloadUtilization();
            static float GetDefaultGameStartingDownloadUtilization();
            static int GetGameStartDownloadSuppressionTimeout();
    };

    // this interface coordinates all downloads from one URL, maintains
    // references to all active requests against this URL, and all
    // workers working on these requests. The benefit of this approach
    // is that this class has the overview of all the requests, and can
    // combine requests of small adjacent byte blocks into a larger
    // request, etc.
    class ORIGIN_PLUGIN_API IDownloadSession : public QObject
    {
        Q_OBJECT
    public:
        virtual ~IDownloadSession() {}

        virtual QString const& getUrl() const = 0;
        virtual QString const& getHostName() const = 0; 
        virtual bool isReady() const = 0;
		virtual bool isRunning() const = 0;

        // async calls
        Q_INVOKABLE virtual void initialize() = 0;

        Q_INVOKABLE virtual void submitRequest(RequestId reqId, QSet<int> priorityGroupIds = QSet<int>(), bool priorityGroupEnabled = true) = 0;
        Q_INVOKABLE virtual void submitRequests(QVector<RequestId> reqIds, int priorityGroupId = 0, bool priorityGroupEnabled = true) = 0;
        Q_INVOKABLE virtual void closeRequest(RequestId reqId) = 0;

        Q_INVOKABLE virtual void priorityMoveToTop(int priorityGroupId) = 0;
        Q_INVOKABLE virtual void prioritySetQueueOrder(QVector<int> priorityGroupIds) = 0;
        Q_INVOKABLE virtual void prioritySetGroupEnableState(int priorityGroupId, bool enabled) = 0;

        // sync calls
        virtual bool requestAndWait(RequestId reqId, int timeout = 0) = 0;
        virtual void cancelSyncRequests() = 0;

        virtual QVector<int> priorityGetQueueOrder() = 0;

        // create/destroy requests
        virtual RequestId createRequest(qint64 byteStart, qint64 byteEnd) = 0;
        virtual IDownloadRequest* getRequestById(RequestId reqId) = 0;

        virtual qint64 getIdealRequestSize() const = 0;
        virtual qint64 getMinRequestSize() const = 0;
        virtual qint64 getRequestSize() const = 0;
        virtual qint64 getContentLength() = 0;

        Q_INVOKABLE virtual void shutdown() = 0;

        virtual const QString& id() const = 0;
        virtual const QUuid& uuid() const = 0;

        virtual void sendHostIPTelemetry(int errorType, int errorCode) = 0;
    signals:
        void initialized(qint64 totalSize);
		void shutdownComplete();

        void IDownloadSession_error(Origin::Downloader::DownloadErrorInfo errorInfo);

        void requestChunkAvailable(RequestId reqId);
        void requestError(RequestId reqId, qint32 errType, qint32 errcode);
        void requestPostponed(RequestId reqId);

        void priorityQueueOrderUpdate(QVector<int> queueOrder);
    };

    class ORIGIN_PLUGIN_API IDownloadRequest : public QObject
    {
        Q_OBJECT
    public:
        virtual RequestId getRequestId() const = 0;
        virtual qint64 getStartOffset() const = 0;
        virtual qint64 getEndOffset() const = 0;
        virtual qint64 getTotalBytesRead() const = 0;
        virtual qint64 getTotalBytesRequested() const = 0;

        virtual bool isSubmitted() const = 0;
        virtual bool isInProgress() const = 0;
        virtual bool isComplete() const = 0;
        virtual bool isCanceled() const = 0;
        virtual bool isPostponed() const = 0;
        virtual bool isDiagnosticMode() const = 0;
        virtual void markCanceled() = 0;
        virtual void setDiagnosticMode(bool val) = 0;
        virtual qint32 getErrorState() const = 0;
        virtual qint32 getErrorCode() const = 0;

        virtual bool chunkIsAvailable() = 0;
        virtual MemBuffer* getDataBuffer() = 0;
        virtual void chunkMarkAsRead() = 0;
        virtual QSharedPointer<DownloadChunkDiagnosticInfo> getChunkDiagnosticInfo() = 0;
    };


    class ORIGIN_PLUGIN_API DNSResMap : public QObject
    {
        Q_OBJECT

        typedef struct 
        {
			quint64 bytes_downloaded;
			quint64 time_elapsed_ms;
            quint64 total_bytes_downloaded;
            quint64 total_time_elapsed_ms;
            quint64 total_times_used;
            quint64 last_download_time; // track how long it has been since last tried
            float download_rate;    // in MB/sec
            int used_count; // how many times used
            quint16 error_count; // how many errors encountered
			bool removed;	// either too many errors or IP is no longer on the DNS and not currently used
        } IP_RateTrackingType;

    public:
        DNSResMap(QString url, DownloadSession *session);
        void cleanup();
        ~DNSResMap();

        void RecordDownloadRate(QString ip, quint64 bytes_downloaded, quint64 time_elapsed_ms, int num_active_workers = 0, int num_active_download_workers = 0, int num_active_downloads = 0);
        bool GetBestIP(QString &url);
        void HandleError(QString ip, DownloadError::type errorType, qint32 errorCode);
        void sendHostIPTelemetry(const QString& id, int errorType, int errorCode);
    public slots:
        void onResolveIP();    // test and see if current DNS has new addresses to try add to pool

    private:
        void AddIPToMap(QString ip);

        DownloadSession *mSession;

        bool mMultiSessionEnabled;
        QString mUrl;
        QString mHostName;
        QMap<QString, IP_RateTrackingType> mIPResMap;
		QString mCurrentFastestIP;	// used to preserve this IP even if the IP is no longer in the current DNS list

        QTimer mCheckDNSTimer;

        quint64 mTotalBytesDownloaded;
        quint64 mTotalTimeElapsed_ms;
		quint64 mNumChunks;
		quint64 mTotalWorkers;
        quint64 mTotalDownloadWorkers;
        quint64 mTotalActiveDownload;

		QMutex mSyncLock;

    };

	class ORIGIN_PLUGIN_API ChunkSizeSpeedTracker
	{
	public:

		ChunkSizeSpeedTracker(quint64 circularBPSSize = 10);

		void	addSample(quint64 nSizeInBytes, quint64 nSpeedInMs);
		quint64 averageRecentBPS();		// averate BPS of most recent n samples
		quint64 slowestRecentBPS();		// slowest BPS of recent n samples

	private:
		QVector<quint64>    mCircularBPSBuffer;
		quint64				mCircularBPSBufferIndex;

	};

} // namespace Downloader
} // namespace Origin

#endif

