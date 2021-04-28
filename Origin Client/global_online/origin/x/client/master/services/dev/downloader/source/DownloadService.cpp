///////////////////////////////////////////////////////////////////////////////
// DownloadService.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QObject>
#include <QMetaType>
#include <QThread>
#include <QThreadPool>
#include <QDebug>
#include <QTimer>
#include <QLinkedList>

#include "io/DownloadWorker.h"
#include "io/DownloadServiceInternal.h"
#include "io/ChannelInterface.h"

#include "services/network/NetworkAccessManager.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/downloader/LogHelpers.h"
#include "services/downloader/TransferStats.h"
#include "services/downloader/Timer.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/common/NetUtil.h"
#include "TelemetryAPIDLL.h"

#include <QCoreApplication>
#include <QReadLocker>
#include <QWriteLocker>
#include <QSemaphore>
#include <QHostInfo>
#include <QNetworkProxy>

#include <limits>

#define SESSIONLOG_PREFIX "[ID:" << id() << "] "
#define SYNCREQUEST_PREFIX "[ID: " << mRequest->getSession()->id() << "]"

//#define MULTIPLE_WORKERS_STRESS_TEST    // forces multiple workers by limiting the initial set of workers to two leaving two to be extra workers
//#define TURN_OFF_MULTIPLE_CONNECTIONS
//#define TURN_OFF_MULTIPLE_WORKERS_PER_FILE

using namespace Origin::Downloader::LogHelpers;

namespace Origin
{
namespace Downloader
{

const qint64 kDelayBetweenRequestRetries = 4000;
const qint64 kDelayBetweenInitializationRetries = 1000;

const int kDefaultMaxWorkersPerFileSession = 2;
const int kDefaultMaxWorkersPerHttpSession = 4;
const int kMaxPossibleWorkers = 4;	// greater of the two above
const int kDefaultMinRequestSize = (32*1024);                               // 32k - Minimum request size, all other request sizes must be multiple of this
const int kDefaultIdealFileRequestSize = (kDefaultMinRequestSize*128);      // 4MB - Must be a multiple of min request size
const int kDefaultIdealHttpRequestSize = (kDefaultMinRequestSize*8);        // 256k - Must be a multiple of min request size
const int kDefaultMaxRequestSize = (kDefaultMinRequestSize*512);           // 16MB - Must be a multiple of min request size
const long kDefaultSessionBaseTickRate = 100;
const long kDefaultRateDecreasePerActiveSession = 25;
const int kGameStartDownloadSuppressionTimeout = 120000;        // The amount of time in ms we suppress the download at game start. (increased to 2 minutes per Alex Z)

const long kMaxPendingChunks = 10;
const int kSemaphoreMaxWait = 5000;
const int kMaxRetryRequests = 3;

// Timeout for HEAD request to get content length, in millis
const unsigned long kContentLengthTimeout = 15000; 

// Allow up to this many sliver bytes between requests when clustering them.
const quint64 kAcceptableSliverBetweenClusteredRequests = 16*1024; // 16KB

static qint64 g_lastRequestId = 0;
static bool g_registeredMetatypes = false;
static qint16 g_activeSessionCount = 0;
static QSemaphore* g_WorkerSemaphore;
static int g_SessionBaseTickRate;
static int g_RateDecreasePerActiveSession;
static QReadWriteLock g_OnlineReadWriteLock;
static int g_minRequestSize = kDefaultMinRequestSize;
static int g_maxRequestSize = kDefaultMaxRequestSize;
bool g_logRequestResponseHeader = false;

static const float kDefaultGameRequestedDownloadUtilization = 1.0f; // 1.0 means let Origin manage
static const float kDefaultDownloadUtilization = 2.0f; // 2.0 is unlimited
static const float kDefaultPlayingDownloadUtilization = 1.0f; // 1.0 corresponds to roughly 10 MB/s
static const float kDefaultGameStartingDownloadUtilization = 0.0f; // Pause downloading while a game starts

volatile float g_downloadUtilization = kDefaultDownloadUtilization;
volatile float g_gameRequestedDownloadUtilization = kDefaultGameRequestedDownloadUtilization; 

const int kMaxIPErrorCount = 4; // max number of errors encountered before IP is removed from list

DNSResMap::DNSResMap(QString url, DownloadSession *session) :
mSession(session),
mMultiSessionEnabled(true),
mTotalBytesDownloaded(0),
mTotalTimeElapsed_ms(0),
mNumChunks(0),
mTotalWorkers(0),
mTotalDownloadWorkers(0),
mTotalActiveDownload(0)
{
    mUrl = url;
    mHostName = QUrl(mUrl).host();

    bool downloaderSafeMode = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloaderSafeMode);
    if (downloaderSafeMode)
    {
        ORIGIN_LOG_WARNING << "Disabling multi-session downloading while in Downloader Safe Mode.";
        mMultiSessionEnabled = false;
        return;
    }

    // Don't use multi-session for file overrides or SSL connections!
    if (url.startsWith("file:", Qt::CaseInsensitive) || url.startsWith("https:", Qt::CaseInsensitive))
    {
        ORIGIN_LOG_WARNING << "Disabling multi-session downloading for file/https transfer.";
        mMultiSessionEnabled = false;
        return;
    }
    
    mCurrentFastestIP = QString();

    QString proxyQueryUrl = QString("http://%1").arg(mHostName);
    QNetworkProxyQuery npq(proxyQueryUrl);
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    for (QList<QNetworkProxy>::Iterator iter = listOfProxies.begin(); iter != listOfProxies.end(); ++iter)
    {
        QNetworkProxy proxy = *iter;
        QNetworkProxy::ProxyType type = proxy.type();
        if (type != QNetworkProxy::NoProxy)
        {
            ORIGIN_LOG_EVENT << "Found a proxy: " << proxy.hostName() << " Type: " << (int)type;
            ORIGIN_LOG_EVENT << "Disabling multi-session downloading due to use of a proxy.";

            GetTelemetryInterface()->Metric_DL_PROXY_DETECTED();

            mMultiSessionEnabled = false;
            return;
        }
    }


    onResolveIP();

#ifndef TURN_OFF_MULTIPLE_CONNECTIONS    // to turn off and just use the first IP (but still track the rate)
    ORIGIN_VERIFY_CONNECT(&mCheckDNSTimer, SIGNAL(timeout()), this, SLOT(onResolveIP()));
    mCheckDNSTimer.start(60 * 1000);  // once a minute
#endif
}

DNSResMap::~DNSResMap()
{
}
    
void DNSResMap::cleanup()
{
    if (mSession->isFileOverride())
        return;

    ORIGIN_VERIFY_DISCONNECT(&mCheckDNSTimer, SIGNAL(timeout()), this, SLOT(onResolveIP()));
    mCheckDNSTimer.stop();

    QMutexLocker lock(&mSyncLock);

    mTotalTimeElapsed_ms = GetTick() - mTotalTimeElapsed_ms;
    float download_rate = 0.0f;
    if (mTotalTimeElapsed_ms > 0)
        download_rate = (float) mTotalBytesDownloaded / (1024.0f * 1024.0f) / ((float) mTotalTimeElapsed_ms / 1000.0f);

    ORIGIN_LOG_EVENT << "Final download rate: " << download_rate << " MB/sec <--- " << (float) mTotalBytesDownloaded / (1024.0f * 1024.0f) << " MB downloaded | Time: " << TransferStats::timeIntervalString(mTotalTimeElapsed_ms / 1000, true) << "  (" << mIPResMap.size() << " IPs encountered)";

    quint64 worker_total_bytes = 0, worker_total_time = 0, average_chunk_size_total = 0;
    quint16 ip_count = 0, ip_error_count = 0;

    for(QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
        if (it->total_time_elapsed_ms)
        {
            ip_count++;

            worker_total_bytes += it->total_bytes_downloaded; 
            worker_total_time += it->total_time_elapsed_ms;

            if (it->total_time_elapsed_ms > 0)
                it->download_rate = (float) it->total_bytes_downloaded / (1024.0f * 1024.0f) / ((float) it->total_time_elapsed_ms / 1000.0f);
            else
                it->download_rate = 0.0f;

            quint64 avg_chunk_size = 0;
            if (it->total_times_used > 0)
            {
                avg_chunk_size = it->total_bytes_downloaded / it->total_times_used;
                average_chunk_size_total += avg_chunk_size;
            }

            ip_error_count += it->error_count;

            ORIGIN_LOG_EVENT << it.key() << " download rate: " << it->download_rate << " MB/sec <--- " << it->total_bytes_downloaded << " bytes dwldd " << (float) it->total_time_elapsed_ms / 1000.0f << " secs  Avg chunk size: " << avg_chunk_size << " bytes";
                        
            GetTelemetryInterface()->Metric_DL_CONNECTION_STATS(mSession->id().toLocal8Bit().constData()
                                                                , mSession->uuid().toString().remove('{').remove('}').toLocal8Bit().constData()
                                                                , it.key().toLocal8Bit().constData()
                                                                , QUrl(mSession->getUrl()).host().toLocal8Bit().constData()
                                                                , it->total_bytes_downloaded
                                                                , it->total_time_elapsed_ms
                                                                , it->total_times_used
                                                                , it->error_count);
        }
        else
        {
            ORIGIN_LOG_EVENT << it.key() << " download rate: 0 MB/sec <--- 0 bytes dwldd 0 secs";
        }
    }

    float average_worker_rate = 0.0f;
    if (worker_total_time > 0)
        average_worker_rate = (float) worker_total_bytes / (1024.0f * 1024.0f) / ((float) worker_total_time /  1000.0f);
    float worker_saturation = 0.0f, active_worker_saturation = 0.0f, download_saturation = 0.0f;
    if (mNumChunks)
    {
        worker_saturation = (float) mTotalWorkers / mNumChunks;
        active_worker_saturation = (float) mTotalDownloadWorkers / mNumChunks;
        download_saturation = (float) mTotalActiveDownload / mNumChunks;
    }
    float rate_factor = 0.0f;
    if (average_worker_rate > 0.0f)
        rate_factor = download_rate / average_worker_rate;

    ORIGIN_LOG_EVENT << "Worker saturation: " << worker_saturation << "  Average download rate: " << average_worker_rate << "  (" << rate_factor << "x)  Active workers: " << active_worker_saturation << "  Download saturation: " << download_saturation;

    // send telemetry for optimization data
    char float_string[4][255];
    sprintf(float_string[0], "%.3f", download_rate);
    sprintf(float_string[1], "%.3f", average_worker_rate);
    sprintf(float_string[2], "%.3f", worker_saturation);
    sprintf(float_string[3], "%.3f", rate_factor);
    if (ip_count > 0)
        average_chunk_size_total /= ip_count;
    GetTelemetryInterface()->Metric_DL_OPTI_DATA(mSession->id().toLocal8Bit().constData(), float_string[0], float_string[1], float_string[2], float_string[3], 
        average_chunk_size_total, QUrl(mSession->getUrl()).host().toLocal8Bit().constData(), ip_count, ip_error_count, mSession->getClusterRequestCount(), mSession->getSingleFileChunkCount() );
}

void DNSResMap::AddIPToMap(QString ip)
{
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
    ORIGIN_LOG_EVENT << "Adding Host IP: " << ip;
#endif
    QUrl url("http://"+ip);
    // test for possible bad/unsupported IPs (e.g. IPv6)
    if (!url.isValid())
    {
        ORIGIN_LOG_WARNING << "Host IP not valid (possible IPv6, not supported in Qt4): " << ip;
        return;
    }

    // add it
    IP_RateTrackingType track;

    track.bytes_downloaded = 0;
    track.time_elapsed_ms = 0;
    track.total_bytes_downloaded = 0;
    track.total_time_elapsed_ms = 0;
    track.total_times_used = 0;
    track.last_download_time = 0;
    track.used_count = 0;
    track.error_count = 0;
    track.download_rate = 0.0f;
    track.removed = false;
    mIPResMap[ip] = track;
}

void DNSResMap::onResolveIP()
{
    QMutexLocker lock(&mSyncLock);

    // Perform the lookup
    QHostInfo hostInfo(QHostInfo::fromName(mHostName));

    if (hostInfo.error() != QHostInfo::NoError)
    {
        ORIGIN_LOG_EVENT << hostInfo.errorString();
        return;
    }

    // mark all IPs in pool as removed, put them back if in DNS (per Akamai's request - https://developer.origin.com/support/browse/EBIBUGS-23531)
    for(QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
        it->removed = true;
    }

    if (!mCurrentFastestIP.isEmpty())
        mIPResMap[mCurrentFastestIP].removed = false;    // but save the current fastest ip

    QList<QHostAddress> iplist = hostInfo.addresses();

    for(QList<QHostAddress>::iterator it = iplist.begin(); it != iplist.end(); it++)
    {
        QHostAddress address = *it;
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
    ORIGIN_LOG_EVENT << "DNS IP: " << address.toString();
#endif

        if (!mIPResMap.contains(address.toString()))
        {
            AddIPToMap(address.toString());
        }
        else
        {
            mIPResMap[address.toString()].removed = false;    // restore existing
        }
    }

#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
    ORIGIN_LOG_EVENT << "RESETTING *********************";
#endif
    for(QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
        QString active;

        if (it->removed)
            active = QString("  ");
        else
            active = QString(">>");

        if (it->time_elapsed_ms)
        {
            it->download_rate = it->bytes_downloaded / (it->time_elapsed_ms * 1000.0f);

            ORIGIN_LOG_EVENT << active << it.key() << " download rate: " << it->download_rate << " MB/sec <--- " << it->bytes_downloaded << " bytes dwldd " << (float) it->time_elapsed_ms / 1000.0f << " secs";
        }
        else
        {
            ORIGIN_LOG_EVENT << active << it.key() << " download rate: 0 MB/sec <--- 0 bytes dwldd 0 secs";
        }
#endif
    
        it->used_count = 0;
        it->bytes_downloaded = 0;
        it->time_elapsed_ms = 0;
        it->last_download_time = 0;
        it->download_rate = 0.0f;
    }
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
    ORIGIN_LOG_EVENT << "*******************************";
#endif
}

void DNSResMap::RecordDownloadRate(QString ip, quint64 bytes_downloaded, quint64 time_elapsed_ms, int num_active_workers, int num_active_download_workers, int num_active_downloads)
{
    QMutexLocker lock(&mSyncLock);

    mTotalBytesDownloaded += bytes_downloaded;
    if (mTotalTimeElapsed_ms == 0)
        mTotalTimeElapsed_ms = GetTick() - time_elapsed_ms;

    mTotalWorkers += num_active_workers;
    mTotalDownloadWorkers += num_active_download_workers;
    mTotalActiveDownload += num_active_downloads;
    mNumChunks++;

    if (mIPResMap.contains(ip))
    {
        mIPResMap[ip].last_download_time = GetTick();
        mIPResMap[ip].total_bytes_downloaded += bytes_downloaded;
        mIPResMap[ip].total_time_elapsed_ms += time_elapsed_ms;
        mIPResMap[ip].bytes_downloaded += bytes_downloaded;
        mIPResMap[ip].time_elapsed_ms += time_elapsed_ms;
        mIPResMap[ip].total_times_used++;
        
        mIPResMap[ip].download_rate = mIPResMap[ip].bytes_downloaded / (mIPResMap[ip].time_elapsed_ms * 1000.0f);
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
        ORIGIN_LOG_EVENT << ip << " download rate: " << mIPResMap[ip].download_rate << " <--- " << mIPResMap[ip].bytes_downloaded << " bytes dwldd " << (float) mIPResMap[ip].time_elapsed_ms / 1000.0f << " secs  workers: " << num_active_workers << " (" << num_active_download_workers << ")  downloads: " << num_active_downloads;
#endif
    }
}

// assumption is that this is only called for retry-able errors
void DNSResMap::HandleError(QString ip, DownloadError::type errorType, qint32 errorCode)
{
    QMutexLocker lock(&mSyncLock);

    if (mIPResMap.contains(ip))
    {
        if (mIPResMap[ip].removed)  // already flagged as out?
            return;

        mIPResMap[ip].error_count++;

        // reset stats so this IP won't be used again until 1 minute resolve timer expires
        mIPResMap[ip].bytes_downloaded = 0;
        mIPResMap[ip].time_elapsed_ms = 0;
        mIPResMap[ip].last_download_time = 0;
        mIPResMap[ip].download_rate = 0.0f;

        // Automatically remove HTTP 400/404 errors from the host list immediately
        if (errorType == DownloadError::HttpError && (errorCode == 400 || errorCode == 404))
        {
            ORIGIN_LOG_ERROR << "Received a HTTP " << errorCode << " response from " << ip << ".  Disabling multi-session downloading.";
            mIPResMap[ip].removed = true;
            mMultiSessionEnabled = false;
        }

        if (mIPResMap[ip].error_count > kMaxIPErrorCount)
        {
            ORIGIN_LOG_ERROR << "Too many errors reported on IP: " << ip << " removing from DNS list";
            mIPResMap[ip].removed = true;
        }
    }
}

void DNSResMap::sendHostIPTelemetry(const QString& id, int errorType, int errorCode)
{
    QMutexLocker lock(&mSyncLock);

    QString ipList;

    // Build a list of all our IPs
    for(QMap<QString,IP_RateTrackingType>::const_iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
		QString ip = it.key();

        if (!ipList.isEmpty())
            ipList.append(",");

        ipList.append(ip);
	}

    // Send the telemetry
    GetTelemetryInterface()->Metric_DL_ERROR_VENDOR_IP(qPrintable(id), qPrintable(ipList), errorType, errorCode);
}

bool DNSResMap::GetBestIP(QString &url)
{
    QMutexLocker lock(&mSyncLock);

    // find next IP to use
    url = mUrl;

    if (!mMultiSessionEnabled)
        return true;

#ifdef TURN_OFF_MULTIPLE_CONNECTIONS    // to turn off and just use the first IP (but still track the rate)

#if defined(ORIGIN_MAC)
    return true;
#else
    QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin();
    it->used_count++;
    int index = url.indexOf(mHostName);
    url.replace(index, mHostName.size(), it.key());   // replace host name with raw ip address

    return true;   // turn off
#endif
#endif

    if (mIPResMap.size() == 0)  
        return false;   // nothing to check

    if (mIPResMap.size() == 1)  // if there is only one, just grab the first
    {
        QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin();

        if (it->removed)    // but if it is an IP not in rotation anymore, then just use the host name
            return false;

        int index = url.indexOf(mHostName);
        url.replace(index, mHostName.size(), it.key());   // replace host name with raw ip address
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
        ORIGIN_LOG_EVENT << it.key() << " chosen(default) (" << mIPResMap.size() << ")  <-------------------------- download rate: " << mIPResMap[it.key()].download_rate;
#endif
        return true;
    }

    // first try one that hasn't been used
    for(QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
        if ((it->used_count <= 2) && (!it->removed))
        {
            it->used_count++;
            int index = url.indexOf(mHostName);
            url.replace(index, mHostName.size(), it.key());   // replace host name with raw ip address
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
            ORIGIN_LOG_EVENT << it.key() << " chosen(a) (" << mIPResMap.size() << ")  <-------------------------- download rate: " << mIPResMap[it.key()].download_rate;
#endif
            return true;
        }
    }

    float best_rate = 0.0f;
    QString best_ip("");

    for(QMap<QString,IP_RateTrackingType>::iterator it = mIPResMap.begin(); it != mIPResMap.end(); it++)
    {
        if (it->removed)
            continue;   // marked as too many errors - don't use

        if (best_rate < it->download_rate)
        {
            best_rate = it->download_rate;
            best_ip = it.key();
        }
    }

    if (mIPResMap.contains(best_ip))
    {
        mCurrentFastestIP = best_ip;

        mIPResMap[best_ip].used_count++;

        int index = url.indexOf(mHostName);
        url.replace(index, mHostName.size(), best_ip);   // replace host name with raw ip address
#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
        ORIGIN_LOG_EVENT << best_ip << " chosen(b) (" << mIPResMap.size() << ")  <-------------------------- download rate: " << mIPResMap[best_ip].download_rate;
#endif
        return true;
    }

    return false;   // didn't find one in map so just return the original (most likely because all the IPs have too many errors - unlikely)
}


ChunkSizeSpeedTracker::ChunkSizeSpeedTracker(quint64 circularBPSSize): mCircularBPSBufferIndex(0)
{
	ORIGIN_ASSERT(circularBPSSize < 10*1024);		// really this should be something like 10, 50 or maybe even 100
	mCircularBPSBuffer.resize(circularBPSSize);

	// Initialize the buffer with the minimum sizes
	for (quint32 i = 0; i < circularBPSSize; i++)
		mCircularBPSBuffer[i] = g_minRequestSize;
}


void ChunkSizeSpeedTracker::addSample(quint64 nSizeInBytes, quint64 nSpeedInMs)
{
    if(nSpeedInMs > 0)
    {
        quint64 bps = (nSizeInBytes*1000) / nSpeedInMs;

        // Update circular buffer
        mCircularBPSBuffer[mCircularBPSBufferIndex] = bps;
        mCircularBPSBufferIndex++;
        mCircularBPSBufferIndex %= mCircularBPSBuffer.size();  // wrap around
    }
}

quint64 ChunkSizeSpeedTracker::averageRecentBPS()
{
	quint64 bps = 0;
	if (mCircularBPSBuffer.size() > 0)
	{
		for (quint64 i = 0; i < mCircularBPSBuffer.size(); i++)
		{
			bps += mCircularBPSBuffer[i];
		}

		bps /= mCircularBPSBuffer.size();
	}

	return bps;
}

quint64 ChunkSizeSpeedTracker::slowestRecentBPS()
{
	quint64 bps = g_maxRequestSize;

	if (mCircularBPSBuffer.size() > 0)
	{
		for (quint64 i = 0; i < mCircularBPSBuffer.size(); i++)
		{
			if (bps > mCircularBPSBuffer[i])
				bps = mCircularBPSBuffer[i];
		}
	}

	return bps;
}

void DownloadService::init()
{
    // TODO not thread safe
    if (!g_registeredMetatypes)
    {
        g_registeredMetatypes = true;
        qRegisterMetaType<RequestId> ("RequestId");
        qRegisterMetaType<QVector<RequestId> >("QVector<RequestId>");
        qRegisterMetaType<QVector<int> >("QVector<int>");
        qRegisterMetaType<QSet<int> >("QSet<int>");
    }
    
    g_minRequestSize = Origin::Services::readSetting(Origin::Services::SETTING_MinRequestSize);

    if(g_minRequestSize < 1)
    {
        g_minRequestSize = kDefaultMinRequestSize;
    }
        
    g_maxRequestSize = Origin::Services::readSetting(Origin::Services::SETTING_MaxRequestSize);

    if(g_maxRequestSize < 1)
    {
        g_maxRequestSize = kDefaultMaxRequestSize;
    }

    g_SessionBaseTickRate = Origin::Services::readSetting(Origin::Services::SETTING_SessionBaseTickRate);

    if(g_SessionBaseTickRate < 1)
    {
        g_SessionBaseTickRate = kDefaultSessionBaseTickRate;
    }
    else
    {
        //TODO: We can't log in service initialization because logging hasn't been initialized yet. revisit
        //ORIGIN_LOG_EVENT << "[DownloaderTuning] Using session base tick rate [" << g_SessionBaseTickRate << "]";
    }

    g_RateDecreasePerActiveSession = Origin::Services::readSetting(Origin::Services::SETTING_RateDecreasePerActiveSession);

    if(g_RateDecreasePerActiveSession < 1)
    {
        g_RateDecreasePerActiveSession = kDefaultRateDecreasePerActiveSession;
    }
    else
    {
        //TODO: We can't log in service initialization because logging hasn't been initialized yet. revisit
        //ORIGIN_LOG_EVENT << "[DownloaderTuning] Using rate decrease per active session [" << g_RateDecreasePerActiveSession << "]";
    }

    g_logRequestResponseHeader = Origin::Services::readSetting(Origin::Services::SETTING_LogDownload);

    g_WorkerSemaphore = new QSemaphore(kMaxPossibleWorkers);
}

void DownloadService::release()
{
#if defined(_DEBUG)
    DownloadRequest::checkForMemoryLeak();
#endif

    // TODO: Cleanup active downloads - may cause issues if flows are not being properly shut down during application exit
    delete g_WorkerSemaphore;
}

IDownloadSession* DownloadService::CreateDownloadSession( QString const& url, const QString& id, bool isPhysicalMedia, Services::Session::SessionWRef session )
{
    int maxWorkersForSession;
    int idealRequestSize;

    bool downloaderSafeMode = false;

    if(url.startsWith("file:"))
    {    
        if(isPhysicalMedia)
        {
            maxWorkersForSession = 1;
            ORIGIN_LOG_DEBUG << "[ID:" << id << "] [DownloaderTuning] Using 1 worker for physical media download.";
        }
        else
        {
            maxWorkersForSession = Origin::Services::readSetting(Origin::Services::SETTING_MaxWorkersPerFileSession);

            // Signed 16-bit integer, so we must clamp.
            if (maxWorkersForSession > 32767)
            {
                ORIGIN_LOG_DEBUG << "ID:" << id << "] [DownloadTuning] Using too many workers (" << maxWorkersForSession << ").";
                maxWorkersForSession = 32767;
            }

            if(maxWorkersForSession < 1)
            {
                maxWorkersForSession = kDefaultMaxWorkersPerFileSession;
            }
            else
            {
                ORIGIN_LOG_EVENT << "[ID:" << id << "] [DownloaderTuning] Using max worker per file session [" << maxWorkersForSession << "]";
            }
        }

        idealRequestSize = Origin::Services::readSetting(Origin::Services::SETTING_IdealFileRequestSize);

        if(idealRequestSize < 1)
        {
            idealRequestSize = kDefaultIdealFileRequestSize;
        }
        else
        {
            ORIGIN_LOG_EVENT << "[ID:" << id << "] [DownloaderTuning] Using ideal request size for file session [" << idealRequestSize << "]";
        }
    }
    else
    {
        maxWorkersForSession = Origin::Services::readSetting(Origin::Services::SETTING_MaxWorkersPerHttpSession);

        if(maxWorkersForSession < 1)
        {
            maxWorkersForSession = kDefaultMaxWorkersPerHttpSession;
        }
        else
        {
            ORIGIN_LOG_EVENT << "[ID:" << id << "] [DownloaderTuning] Using max worker per HTTP session [" << maxWorkersForSession << "]";
        }

        downloaderSafeMode = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloaderSafeMode);
        if (downloaderSafeMode)
        {
            ORIGIN_LOG_WARNING << "Downloader Safe Mode enabled, maxWorkersForSession = 1.";

            maxWorkersForSession = 1;
        }

        idealRequestSize = Origin::Services::readSetting(Origin::Services::SETTING_IdealHttpRequestSize);

        if(idealRequestSize < 1)
        {
            idealRequestSize = kDefaultIdealHttpRequestSize;
        }
        else
        {
            ORIGIN_LOG_EVENT << "[ID:" << id << "] [DownloaderTuning] Using ideal request size for HTTP session [" << idealRequestSize << "]";
        }
    }

    if((idealRequestSize % g_minRequestSize != 0))
    {
        ORIGIN_ASSERT(false);
        ORIGIN_LOG_WARNING << "[ID:" << id << "] Request size is not evenly divisible by minimum request size, download tracking may be impacted negatively.";
    }

    g_activeSessionCount++;

    return new DownloadSession(url, id, isPhysicalMedia, idealRequestSize, static_cast<qint16>(maxWorkersForSession), downloaderSafeMode, session);
}

void DownloadService::DestroyDownloadSession(IDownloadSession* session)
{
    QObject::disconnect(session, 0, 0, 0);
    session->shutdown();
    delete session;

    g_activeSessionCount--;
}

int DownloadService::GetActiveDownloadSessionCount()
{
    return (g_activeSessionCount > 0) ? 1 : 0;	// because we are now using a download queue this return value is either 1 or 0.  kept this in case we go back to multiple downloads.
}

bool DownloadService::IsNetworkDownload(const QString& url)
{
    return ChannelInterface::IsNetworkUrl(url);
}

float DownloadService::GetDownloadUtilization()
{
    // If the game has requested turbo mode, use that
    if (g_gameRequestedDownloadUtilization > 1.0)
        return g_gameRequestedDownloadUtilization;

    // If the game has requested a speed slower than max
    if (g_gameRequestedDownloadUtilization < 1.0)
    {
        // Clamp the client utilization at 1.0
        float clientUtilization = g_downloadUtilization;
        clientUtilization = std::min<float>(clientUtilization, 1.0f);

        // If the game has requested a normal speed, combine that with the Origin-managed value
        return clientUtilization*g_gameRequestedDownloadUtilization;
    }

    // The game requested 1.0 exactly, so just return the Origin-managed value
    return g_downloadUtilization;
}

void DownloadService::SetGameRequestedDownloadUtilization(float utilization)
{
    if (utilization < 0.0 || utilization > 2.0)
    {
        ORIGIN_LOG_ERROR << "[Download Service] SetGameRequestedDownloadUtilization value out of bounds (0-2): " << utilization;
        return;
    }

    QString description;

    if (utilization == 1.0)
        description = " (Let Origin Manage)";
    else if (utilization == 2.0)
        description = " (Maximum Throughout)";
    else if (utilization == 0.0)
        description = " (Full Stop)";
    else
        description = " (Throttled)";

    g_gameRequestedDownloadUtilization = utilization;
    ORIGIN_LOG_EVENT << "[Download Service] SetGameRequestedDownloadUtilization value: " << utilization << description << " Effective utilization: " << GetDownloadUtilization();
}

void DownloadService::SetDownloadUtilization(float utilization)
{
    if (utilization < 0.0 || utilization > 2.0)
    {
        ORIGIN_LOG_ERROR << "[Download Service] SetDownloadUtilization value out of bounds (0-2): " << utilization;
        return;
    }

    g_downloadUtilization = utilization;
    ORIGIN_LOG_EVENT << "[Download Service] SetDownloadUtilization value: " << utilization << " Effective utilization: " << GetDownloadUtilization();
}

float DownloadService::GetDefaultGameRequestedDownloadUtilization()
{
    return kDefaultGameRequestedDownloadUtilization;
}

float DownloadService::GetDefaultDownloadUtilization()
{
    return kDefaultDownloadUtilization;
}

float DownloadService::GetDefaultPlayingDownloadUtilization()
{
    return kDefaultPlayingDownloadUtilization;
}

float DownloadService::GetDefaultGameStartingDownloadUtilization()
{
    return kDefaultGameStartingDownloadUtilization;
}

int DownloadService::GetGameStartDownloadSuppressionTimeout()
{
    return kGameStartDownloadSuppressionTimeout;
}

class SyncRequestHelper : public QRunnable
{
public:
    SyncRequestHelper(DownloadRequest* request, Services::Session::SessionWRef session) : mSyncChannel(NULL), mRequest(request), mSession(session), mBarrier(1), mCanceled(false), mResult(false)
    {
        setAutoDelete(false);
    }

    void run()
    {    
        qint64 start = mRequest->getStartOffset();
        qint64 end = mRequest->getEndOffset();
        qint64 len = end - start;

        if (len <= 0)
        {
            ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << "Invalid length specified [" << len << "] for request " << mRequest->getRequestId();
            mRequest->setErrorState(DownloadError::RangeRequestInvalid);
        }
        else if(!mCanceled && !mSession.isNull())
        {
            mSyncChannel = ChannelInterface::Create(mRequest->getSession()->getUrl(), mSession);

            if(mSyncChannel == NULL || mSyncChannel->Connect() == false)
            {
                ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << "Failed to connect to URL: " << logSafeDownloadUrl(mRequest->getSession()->getUrl());
                mRequest->setErrorState(DownloadError::ContentUnavailable);
            }
            else
            {
                //Create the range. Exclude the last byte!!!
                QString sRequest = QString("bytes=%1-%2").arg(QString::number(start)).arg(QString::number(end-1));
                QString rangeStr = "Range";
                mSyncChannel->RemoveRequestInfo(rangeStr);
                mSyncChannel->AddRequestInfo(rangeStr, sRequest);
                mSyncChannel->RemoveRequestInfo("Host");
                mSyncChannel->AddRequestInfo("Host", mRequest->getSession()->getHostName());

                if(g_logRequestResponseHeader)
                {
                    ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << " Sending GET request with URL (" << mRequest->getSession()->getUrl() << ").";

                    QHash<QString, QString> headerSet = mSyncChannel->GetAllRequestInfo();
                    if (!headerSet.empty())
                    {
                        int idx = 0;
                        for (QHash<QString,QString>::const_iterator it = headerSet.begin(); it != headerSet.end(); it++)
                        {
                            ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << " GET request key[" << idx << "] = " << it.key() << "; value[" << idx << "] = " << it.value();
                            ++idx;
                        }
                    }
                }

                int retryCount = 0;

                while(retryCount < kMaxRetryRequests && 
                     !mSyncChannel->SendRequest("GET", QString(), QString(), QByteArray()) && 
                      mRequest->getSession()->isErrorRetryable(mSyncChannel->GetLastErrorType(), mSyncChannel->GetErrorCode()))
                {
                    if(mCanceled)
                    {
                        break;
                    }

                    Origin::Services::PlatformService::sleep(kDelayBetweenInitializationRetries);
                    retryCount++;
                }

                // If enabled in EACore INI file, we wwant to record the GET response header
                // (if any response) regardless of the success
                if(g_logRequestResponseHeader)
                {
                    ChannelInfo ci = mSyncChannel->GetAllResponseInfo();
                    QString key, value;
                    int idx = 0;
                    QHash<QString, QString>::iterator i;
                    for (i = ci.begin(); i != ci.end(); ++i)
                    {
                        ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << " GET response key[" << idx << "] = " << i.key().toStdString().c_str() << "; value[" << idx << "] = " << i.value().toStdString().c_str();
                        ++idx;
                    }
                }

                if (mSyncChannel->GetLastErrorType() != DownloadError::OK )
                {
                    mRequest->setErrorState(mSyncChannel->GetLastErrorType(), mSyncChannel->GetErrorCode());
                    ORIGIN_LOG_ERROR << SYNCREQUEST_PREFIX << "Failed to send synchronous request for url [" << logSafeDownloadUrl(mRequest->getSession()->getUrl()) << "] last error: " << ErrorTranslator::Translate((ContentDownloadError::type)mSyncChannel->GetLastErrorType());
                }
                else if(!mCanceled)
                {
                    mRequest->queueDataContainer(mRequest->getAvailableDataContainer());
                    MemBuffer* readBuf = mRequest->getDataBuffer();
                    qint64 bytesRead;

                    if(readBuf->TotalSize() < mRequest->getTotalBytesRequested())
                    {
                        readBuf->Resize(mRequest->getTotalBytesRequested());
                    }

                    if(mSyncChannel->ReadData(readBuf->GetBufferPtr(), mRequest->getTotalBytesRequested(), &bytesRead, mCanceled, 0) && bytesRead == mRequest->getTotalBytesRequested())
                    {
                        mRequest->setRequestState(bytesRead, true, true, 0, 0);
                        mResult = true;
                    }
                    else
                    {
                        // make sure proper error data is reported while still lumping download errors in ITO into the FileRead type.
                        QString errorString = QString("Type=%1,Code=%2").arg(mSyncChannel->GetLastErrorType()).arg(mSyncChannel->GetErrorCode());
                        if (mRequest->getSession()->isPhysicalMediaSession())
                        {
                            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mRequest->getSession()->id().toLocal8Bit().constData(), "SyncRequestITO", errorString.toUtf8().data(), __FILE__, __LINE__);
                            mRequest->setErrorState(DownloadError::FileRead, mSyncChannel->GetErrorCode());
                        }
                        else
                        {
                            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mRequest->getSession()->id().toLocal8Bit().constData(), "SyncRequest", errorString.toUtf8().data(), __FILE__, __LINE__);
                            mRequest->setErrorState(mSyncChannel->GetLastErrorType(), mSyncChannel->GetErrorCode());
                        }
                    }
                }
                else
                {
                    mRequest->setErrorState(DownloadError::DownloadWorkerCancelled);
                }
            }

            if(mSyncChannel)
            {
                delete mSyncChannel;
                mSyncChannel = NULL;
            }
        }

        mBarrier.release();
    }

    bool requestAndWait(unsigned long timeout)
    {
        mBarrier.acquire(1);

        sSyncRequestThreadPool.start(this);

        if(timeout == 0)
        {
            mBarrier.acquire(1);
            return mResult;
        }
        else if(mBarrier.tryAcquire(1, timeout))
        {
            return mResult;
        }
        else
        {
            // We've timed out, cancel it and return false
            mCanceled = true;
            mSyncChannel->CloseRequest();
            return false;
        }
    }

    void cancel()
    {
        mCanceled = true;

        if(mSyncChannel)
        {
            mSyncChannel->CloseRequest(true);
        }
    }

    bool result()
    {
        return mResult;
    }

private:
    static QThreadPool sSyncRequestThreadPool;
    
    ChannelInterface* mSyncChannel;
    DownloadRequest* mRequest;
    Services::Session::SessionWRef mSession;
    QSemaphore mBarrier;
    bool mCanceled;
    bool mResult;
};

QThreadPool SyncRequestHelper::sSyncRequestThreadPool;

// IDownloadSession members

DownloadSession::DownloadSession(const QString& url, const QString& id, bool isPhysicalMedia, qint64 idealRequestSize, qint16 maxWorkersForSession, bool safeMode, Services::Session::SessionWRef session ) :
    mSyncRequestsCanceled(false),
    mEventThread(NULL),    
    mHeartbeatRunning(false),
    mShuttingDownSession(false),
    mPhysicalMedia(isPhysicalMedia),
    mRateLimiter(this),
    mUrl(url),
    mId(id),
    mUuid(QUuid::createUuid()),
    mCompletedWorkers(0),
    mSafeMode(safeMode),
    mIdealRequestSize(idealRequestSize),
    mCurRequestSize(idealRequestSize),
    mMaxWorkers(maxWorkersForSession),
    mWorkerErrorLevel(0),
    mConsecutiveSuccessfulWorkers(0),
    mIoDeviceReady(false),
    mSession(session),
    mDNSResMap(url, this),
    mMemoryHighWatermark(0),
    mMemoryAllocated_bytes(0),
    mClusterRequestCount(0),
    mSingleFileChunkCount(0)
{
    mFileOverride = !isPhysicalMedia && mUrl.startsWith("file:");
}

#ifdef DEBUG_DOWNLOAD_MEMORY_USE
static qint64 gMaxDownloadMemoryAllocated_perSession = 0;
#endif

DownloadSession::~DownloadSession()
{
    if(mEventThread)
    {
        mEventThread->exit();

        while(mEventThread->isRunning())
        {
            Origin::Services::PlatformService::sleep(10);
        }

        delete mEventThread;
    }

    QMutexLocker lock(&mSyncRequestLock);

    if(!mActiveSyncRequests.empty())
    {
        ORIGIN_LOG_WARNING << "Destroying download session while active synchronous requests are still working.  Canceling them.";
        foreach(SyncRequestHelper* request, mActiveSyncRequests)
        {
            request->cancel();
        }
        mActiveSyncRequests.clear();
    }
    ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Download session destroyed for url: " << logSafeDownloadUrl(mUrl);
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
    if (gMaxDownloadMemoryAllocated_perSession < mMemoryHighWatermark)
        gMaxDownloadMemoryAllocated_perSession = mMemoryHighWatermark;
    ORIGIN_LOG_EVENT << "Memory Allocated High: " << (float) mMemoryHighWatermark / (1024.0f * 1024.0f) << "  Overall highest: " << (float) gMaxDownloadMemoryAllocated_perSession / (1024.0f * 1024.0f);
#endif
    
    mDNSResMap.cleanup();
}

const QString& DownloadSession::id() const
{
    return mId;
}

const QUuid& DownloadSession::uuid() const
{
    return mUuid;
}
    
void DownloadSession::sendHostIPTelemetry(int errorType, int errorCode)
{
    mDNSResMap.sendHostIPTelemetry(id(), errorType, errorCode);
}

void DownloadSession::CleanupThreadData()
{
    // Cancel all active workers
    ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Deleting all active download workers...";
    foreach(DownloadWorker* worker, mActiveDownloads)
    {
        g_WorkerSemaphore->release();
        delete worker;
    }

    // Clear the workers list
    mActiveDownloads.clear();

    ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Deleting all cached single download workers...";
    foreach(DownloadWorker* worker, mWorkerCache[0])
    {
        delete worker;
    }

    // Clear the workers list
    mWorkerCache[0].clear();

    ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Deleting all cached cluster download workers...";
    foreach(DownloadWorker* worker, mWorkerCache[1])
    {
        delete worker;
    }

    // Clear the workers list
    mWorkerCache[1].clear();

    if (!mDownloadRequests.empty())
    {
        ORIGIN_LOG_WARNING << "We aren't properly cleaning up all DownloadRequests during normal download operations. Deleting them.";
    }

    foreach(DownloadRequest* request, mDownloadRequests)
    {
        delete request;
    }

    mDownloadRequests.clear();
}

qint16 DownloadSession::getActiveDownloadWorkers()
{
#if 1
    return (qint16) mActiveDownloads.size();
#else
    qint16 active_downloads = 0;

    foreach(DownloadWorker* worker, mActiveDownloads)
    {
        switch (worker->getState())
        {
        case DownloadWorker::kStRunning:
        case DownloadWorker::kStInitialized:    // single req workers can be put into this state before grabbing their next chunk
            active_downloads++;
            break;
        case DownloadWorker::kStDownloadComplete:
            if (worker->getType() == DownloadWorker::kTypeSingle)           // make sure we consider a single request worker as active even when the chunk
                active_downloads++;    // has completed downloading since another may be on its way.
            break;
        default:
            break;
        }
    }

    return active_downloads;
#endif
}

qint16 DownloadSession::getActiveDownloadChannels()
{
    qint16 active_downloads = 0;

    foreach(DownloadWorker* worker, mActiveDownloads)
    {
        if (worker->isTransferring())
            active_downloads++;
    }

    return active_downloads;
}

qint16 DownloadSession::getWorkerIndex(DownloadWorker* workerToFind)
{
	qint16 nIndex = 0;

	foreach(DownloadWorker* worker, mActiveDownloads)
	{
		if (worker == workerToFind)
			return nIndex;

		nIndex++;
	}

	return 0;
}

void DownloadSession::servicePendingRequestQueue()
{
    // We have a local cap for simultaneous requests for sessions, but we also have a global max we want to allow allocated across all download sessions
    qint16 maxWorkersAvailable = std::max<qint16>(1, static_cast<qint16>(std::min<float>(DownloadService::GetDownloadUtilization(),1.0f)*(mMaxWorkers / DownloadService::GetActiveDownloadSessionCount())));

#ifdef MULTIPLE_WORKERS_STRESS_TEST
    if (maxWorkersAvailable > 2)
        maxWorkersAvailable -= 2;
    else
    if (maxWorkersAvailable == 2)
        maxWorkersAvailable = 1;
#endif
    qint16 active_downloads = getActiveDownloadWorkers();

    // See if we can add any new download requests workers from the queue
    if (true)// TODO: Clean this indent block up, for now do this so the diff isn't ridiculous --- active_downloads < maxWorkersAvailable)
    {
        qint64 requestChunksPending = 0;

        QMutexLocker lock(&mDownloadRequestMapLock);

#ifdef DYNAMIC_POSTPONEMENT
        // We only want to postpone requests there are actually multiple groups in use
        bool enablePostpone = (mPriorityQueue.GetQueue().size() > 1);

        RequestId highestPriorityActiveRequest = -1;
        RequestId highestPriorityAvailableRequest = -1;
        bool higherPriorityRequestsBlocked = false;
#endif
        DownloadPriorityQueue::DownloadPriorityQueueIterator iter = mPriorityQueue.constIterator();
        while (iter.valid())
        {
            // This is the request we will be working on
            RequestId reqId = iter.current();

            // Retrieve the download request
            DownloadRequest *req = mDownloadRequests[reqId];
            if (!req)
            {
                iter.next();
                continue;
            }

            // Don't process canceled or completed requests, also skip request that are queued for retry but haven't waited long enough
            if (req->isCanceled() || req->isComplete() || req->isDownloadingComplete() || req->waitBeforeRetry() || req->isHandlingWorkerError())
            {
                iter.next();
                continue;
            }

#ifdef DYNAMIC_POSTPONEMENT
            // If the request is in progress/has an active worker, and we haven't yet set the highest priority active request marker
            if ((req->isInProgress() || req->getWorker()) && highestPriorityActiveRequest == -1)
            {
                highestPriorityActiveRequest = reqId;
            }

            // If we get here, the request isn't complete, isn't canceled, and isn't in an error state
            if (req->isSubmitted()  // Is the request submitted yet?  If not, it isn't ready to work
                && (!req->isInProgress() && !req->getWorker() && !req->isHandlingWorkerError()) // Is the request not already in progress/has a worker?
                && highestPriorityActiveRequest == -1 // If there is no higher priority active request set yet 
                && highestPriorityAvailableRequest == -1 // If there is no higher priority available request set yet
                && active_downloads >= maxWorkersAvailable) // If this request would be blocked because there are no available workers
            {
                // Set the flag indicating that we're starving higher priority requests
                highestPriorityAvailableRequest = reqId;
                higherPriorityRequestsBlocked = true;

                if (enablePostpone)
                {
                    ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Detected request id = " << reqId << " may be starved by lower priority work.";
                }

                // Move to the next request
                iter.next();
                continue;
            }

            // This request is blocking a higher priority request, so postpone it and break
            if (higherPriorityRequestsBlocked && req->isInProgress())
            {
                DownloadWorker *worker = req->getWorker();

                // If there is no valid worker or it's a clustered worker, move on, not worth the trouble of postponing clustered work
                if (!worker || worker->getType() == DownloadWorker::kTypeCluster)
                {
                    iter.next();
                    continue;
                }

                if (enablePostpone && !req->isPostponed())
                {
                    // This is an active single request, postpone it and stop processing, our higher priority request will get re-scheduled the next trip around
                    req->markPostponed(true);
                    ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Postponing request id = " << reqId << " because higher priority work exists, request id = " << highestPriorityAvailableRequest;
                }
                
                break;
            }
#endif

            // No work is starved, but there's no available workers.  Nothing left to schedule
            if (active_downloads >= maxWorkersAvailable)
                break;

            // This check throttles the downloader from making more requests if the protocol 
            // isn't keeping up with the data that has already been downloaded. 
            if (req->isInProgress() && req->chunkIsAvailable())
            {
                ++requestChunksPending;
            }

            // If too many chunks are pending, give up!
            if (requestChunksPending >= kMaxPendingChunks)
            {
#ifdef DEBUG_DOWNLOAD_SERVICE
                ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "[Downloader Pulse] Too many pending chunks! - " << requestChunksPending << " [max: " << kMaxPendingChunks << "]";
#endif
                break;
            }

            if (req->isSubmitted() && !req->isInProgress())
            {
                // If we don't have any active downloads, we block here until one frees up elsewhere.
                // Otherwise we try to reserve a worker and if we can't we wait until next time.
                // This guarantees everyone keeps at gets one worker in fair order and has at least 1
                // worker in most cases
                if(mActiveDownloads.empty())
                {
                    // We block for a little while but then head to the back of the line.  It's not fair
                    // but it guarantees we aren't blocking if someone cancels or destroys the session
                    if(g_WorkerSemaphore->tryAcquire(1, kSemaphoreMaxWait))
                    {
                        if(mShuttingDownSession)
                        {
                            g_WorkerSemaphore->release();
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else if(!g_WorkerSemaphore->tryAcquire()) // if we already got a worker, try half heartedly to get more.
                {
                    // There are no workers available.
                    break;
                }

                bool skipnext = false;
                bool clusteredRequestSubmitted = false;

                // Check for cluster optimal request sizes, and see if requests are contiguous
                if((quint64)req->getTotalBytesRequested() < mCurRequestSize)
                {
                    quint64 clusterSize = req->getTotalBytesRequested();
                    QVector<DownloadRequest*> clusteredRequests;
                    clusteredRequests.push_back(req);

                    // Look for potential clusterable requests following this one.
                    iter.next();
                    while(iter.valid())
                    {
                        // Retrieve the download request
                        DownloadRequest *innerReq = mDownloadRequests[iter.current()];
                        if (!innerReq)
                        {
                            // Don't know what happened here, this should never happen
                            break;
                        }

                        if(!(innerReq->isComplete() || innerReq->isInProgress()) &&
                            ((quint64)(innerReq->getStartOffset() - clusteredRequests.back()->getEndOffset()) < kAcceptableSliverBetweenClusteredRequests) && 
                            ((quint64)(clusterSize + innerReq->getTotalBytesRequested()) < (quint64)mCurRequestSize))
                        {
                            clusterSize += innerReq->getTotalBytesRequested();
                            clusterSize += (innerReq->getStartOffset() - clusteredRequests.back()->getEndOffset());
                            clusteredRequests.push_back(innerReq);
                            iter.next();
                        }
                        else
                        {
                            // This last one took us over cluster size limit or wasn't contiguous, set a flag so
                            // it can be serviced next loop and not bypassed
                            skipnext = true;
                            break; // Break the inner loop
                        }
                    }

                    // in case we are at the end, we don't want to iterate past the end at the enclosing for loop above.
                    if (!iter.valid())
                    { 
                        skipnext = true;
                    }

                    if(clusteredRequests.count() > 1)
                    {
                        clusteredRequestSubmitted = true;

#ifdef DEBUG_DOWNLOAD_SERVICE
                        ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "Submitting clustered task with" << clusteredRequests.count() << "requests bundled into request of size " << clusterSize;
#endif

                        // Schedule our task, try to reuse a cached worker
                        if(mWorkerCache[DownloadWorker::kTypeCluster].isEmpty())
                        {
                            DownloadWorker * worker = new DownloadClusterWorker(this, mSession, clusteredRequests, clusterSize);
                            if((worker->getState() != DownloadWorker::kStError) && (!mSession.isNull()))
                            {
                                mActiveDownloads.push_back(worker);
                                active_downloads++;

                                if (!mPhysicalMedia && !mFileOverride)
                                {
                                    QString url;
                                    mDNSResMap.GetBestIP(url);
                                    worker->setUrl(url);
                                }
                            }
                            else
                            {
                                delete worker;
                                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                                errorInfo.mErrorType = DownloadError::NotConnectedError;
                                errorInfo.mErrorCode = 0;
    
                                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to initialize download worker, not connected.";
                                emit (IDownloadSession_error(errorInfo));
                            }
                        }
                        else
                        {
                            DownloadClusterWorker* worker = static_cast<DownloadClusterWorker*>(mWorkerCache[DownloadWorker::kTypeCluster].front());
                            mWorkerCache[DownloadWorker::kTypeCluster].pop_front();
                            worker->init(clusteredRequests, clusterSize);
                            mActiveDownloads.push_back(worker);
                            active_downloads++;

                            if (!mPhysicalMedia && !mFileOverride)
                            {
                                QString url;
                                mDNSResMap.GetBestIP(url);
                                worker->setUrl(url);
                            }
                        }
                    }
                }

                if(!clusteredRequestSubmitted)
                {
                    req->markInProgress(true);

#ifdef DEBUG_DOWNLOAD_SERVICE
                    ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "Submitting single request task for request of size " << req->getTotalBytesRequested();
#endif
                    // Schedule our task, try to reuse a cached worker
                    if(mWorkerCache[DownloadWorker::kTypeSingle].isEmpty())
                    {
                        DownloadWorker * worker = new DownloadSingleRequestWorker(this, mSession, req);
                          if((!mSession.isNull()) && (worker->getState() != DownloadWorker::kStError))
                        {
                            mActiveDownloads.push_back(worker);
                            active_downloads++;

                            if (!mPhysicalMedia && !mFileOverride)
                            {
                                QString url;
                                mDNSResMap.GetBestIP(url);
                                worker->setUrl(url);
                            }
                        }
                        else
                        {
                            delete worker;
                            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                            errorInfo.mErrorType = DownloadError::NotConnectedError;
                            errorInfo.mErrorCode = 0;
    
                            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to initialize download worker, not connected.";
                            emit (IDownloadSession_error(errorInfo));
                        }
                    }
                    else
                    {
                        DownloadSingleRequestWorker* worker = static_cast<DownloadSingleRequestWorker*>(mWorkerCache[DownloadWorker::kTypeSingle].front());
                        mWorkerCache[DownloadWorker::kTypeSingle].pop_front();
                        worker->init(req);
                        mActiveDownloads.push_back(worker);
                        active_downloads++;

                        if (!mPhysicalMedia && !mFileOverride)
                        {
                            QString url;
                            mDNSResMap.GetBestIP(url);
                            worker->setUrl(url);
                        }
                    }
                }

                // If we already moved the iterator and don't want to move it again
                if (skipnext)
                {
                    continue;
                }
            }

            // Move to the next request
            iter.next();
        }
    }
}

void DownloadSession::servicePendingRequestQueueExtraWorkers()
{
    // We have a local cap for simultaneous requests for sessions, but we also have a global max we want to allow allocated across all download sessions
	qint16 maxWorkersAvailable = std::max<qint16>(1, static_cast<qint16>(std::min<float>(DownloadService::GetDownloadUtilization(),1.0f)*(mMaxWorkers / DownloadService::GetActiveDownloadSessionCount())));

    qint16 active_downloads = getActiveDownloadWorkers();

    if (active_downloads < maxWorkersAvailable)    // See if we can add any new download requests workers from the queue
    {
        QMutexLocker lock(&mDownloadRequestMapLock);

        // find an active download to attach another worker to
        for (DownloadPriorityQueue::DownloadPriorityQueueIterator iter = mPriorityQueue.constIterator(); iter.valid(); iter.next())
        {
            // This is the request we will be working on
            RequestId reqId = iter.current();

            if (active_downloads >= maxWorkersAvailable)
                break;

            DownloadRequest *req = mDownloadRequests[reqId];
            if (!req)
                continue;

            // Don't process canceled or completed requests, also skip request that are queued for retry but haven't waited long enough
            if (req->isCanceled() || req->isComplete() || req->isDownloadingComplete() || req->waitBeforeRetry() || req->isHandlingWorkerError())
                continue;

            if (req->isSubmitted() && req->isInProgress() && req->hasMoreDataToDownload())
            {
                // If we don't have any active downloads, we block here until one frees up elsewhere.
                // Otherwise we try to reserve a worker and if we can't we wait until next time.
                // This guarantees everyone keeps at gets one worker in fair order and has at least 1
                // worker in most cases
                if(mActiveDownloads.empty())
                {
                    // We block for a little while but then head to the back of the line.  It's not fair
                    // but it guarantees we aren't blocking if someone cancels or destroys the session
                    if(g_WorkerSemaphore->tryAcquire(1, kSemaphoreMaxWait))
                    {
                        if(mShuttingDownSession)
                        {
                            g_WorkerSemaphore->release();
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else if(!g_WorkerSemaphore->tryAcquire()) // if we already got a worker, try half heartedly to get more.
                {
                    // There are no workers available.
                    break;
                }

#ifdef DEBUG_DOWNLOAD_SERVICE
                ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "Adding worker to active single request task for request of size " << req->getTotalBytesRequested();
#endif
                // Schedule our task, try to reuse a cached worker
                if(mWorkerCache[DownloadWorker::kTypeSingle].isEmpty())
                {
                    DownloadWorker * worker = new DownloadSingleRequestWorker(this, mSession, req);
                      if((!mSession.isNull()) && (worker->getState() != DownloadWorker::kStError))
                    {
                        mActiveDownloads.push_back(worker);
                        active_downloads++;

                        if (!mPhysicalMedia && !mFileOverride)
                        {
                            QString url;
                            mDNSResMap.GetBestIP(url);
                            worker->setUrl(url);
                        }
                    }
                    else
                    {
                        delete worker;
                        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                        errorInfo.mErrorType = DownloadError::NotConnectedError;
                        errorInfo.mErrorCode = 0;
    
                        ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to initialize download worker, not connected.";
                        emit (IDownloadSession_error(errorInfo));
                    }
                }
                else
                {
                    DownloadSingleRequestWorker* worker = static_cast<DownloadSingleRequestWorker*>(mWorkerCache[DownloadWorker::kTypeSingle].front());
                    mWorkerCache[DownloadWorker::kTypeSingle].pop_front();
                    worker->init(req);
                    mActiveDownloads.push_back(worker);
                    active_downloads++;

                    if (!mPhysicalMedia && !mFileOverride)
                    {
                        QString url;
                        mDNSResMap.GetBestIP(url);
                        worker->setUrl(url);
                    }
                }

            }
        }
    }
}


void DownloadSession::adjustRequestSize(DownloadWorker* worker)
{
	if ((worker->getState() == DownloadWorker::kStDownloadComplete) ||
        (worker->getState() == DownloadWorker::kStIdle))
	{     
        quint64 nTimeExecuted = worker->getRunTime();

        if (nTimeExecuted == 0)
        {
            // Something isn't right
            mCurRequestSize = g_minRequestSize;
            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Worker reporting 0ms time to complete.  Resetting request size to:" << mCurRequestSize;
            return;
        }

		// We want only one worker to be responding within a second for UI responsiveness
		// Each successive worker can be trying for bigger chunk sizes
		quint64 workerIndex = getWorkerIndex(worker);

		if (workerIndex == 0)
			mCurRequestSize = mChunkSpeedTracker.slowestRecentBPS();
		else
			mCurRequestSize = mChunkSpeedTracker.averageRecentBPS();

        // Range adjustments
        if (mCurRequestSize < g_minRequestSize) // floor it
            mCurRequestSize = g_minRequestSize;

		mCurRequestSize = mCurRequestSize * pow((float) 2, (long) workerIndex);

        float utilization = DownloadService::GetDownloadUtilization();
        int maxRequestSizeBasedOnUtilization = g_maxRequestSize;

        // If we are tweaking the utilization we need a bit more granularity
        if (utilization <= 1.0)
        {
            // We need the granularity to match the nominal rate ; i.e. ideally two chunks per second
            maxRequestSizeBasedOnUtilization = DownloadRateLimiter::getIdealMaxChunkSize();

            if (maxRequestSizeBasedOnUtilization < g_minRequestSize)
                maxRequestSizeBasedOnUtilization = g_minRequestSize;
        }

        int maxRequestSize = maxRequestSizeBasedOnUtilization;
        if (mCurRequestSize > maxRequestSize) // cap it
            mCurRequestSize = maxRequestSize;

        mCurRequestSize = ((mCurRequestSize/ g_minRequestSize)*g_minRequestSize);    // ensure it's a multiple of minimum size (for the chunk map)

		//ORIGIN_LOG_DEBUG << "mRequestSize:" << mCurRequestSize << " nLatestBPS:" << nLatestBPS;

#ifdef DEBUG_DOWNLOAD_MULTI_CONNECTION
        ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "Worker reporting:" << nTimeExecuted << "ms to complete. percentage:" << percentage << " Updating request size from:" << oldSize << " to:" << mCurRequestSize << " difference:" << (qint64) mCurRequestSize- (qint64) oldSize << " workers:" << (qint64) mActiveDownloads.size();
#endif
        ORIGIN_ASSERT((mCurRequestSize % g_minRequestSize) == 0);
        ORIGIN_ASSERT(mCurRequestSize <= g_maxRequestSize);
        ORIGIN_ASSERT(mCurRequestSize >= g_minRequestSize);
	}
	else if(worker->getState() == DownloadWorker::kStError)
	{
		if(mCurRequestSize > (quint64)g_minRequestSize)
		{
			ORIGIN_LOG_DEBUG << SESSIONLOG_PREFIX << "Transfer experiencing errors [error level = " << mWorkerErrorLevel << "], decreasing request size from " << mCurRequestSize << " to " << std::max<quint64>(mCurRequestSize/2, g_minRequestSize);
			mCurRequestSize = std::max<quint64>(mCurRequestSize/2, g_minRequestSize);

            //  TELEMETRY:  Track the request size changes
            QString errorCode = QString("mCurRequestSize=%1").arg(mCurRequestSize);
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("DownloadSessionAdjustRequestSize", errorCode.toUtf8().data());
        }
    }
}
bool DownloadSession::isErrorRetryable(DownloadError::type errorType, qint32 errorCode) const
{
    if(mPhysicalMedia)
    {
        // We don't really retry anything on physical media at this time.
        return false;
    }
    else
    {
        switch(errorType)
        {
        case DownloadError::UNKNOWN:
        case DownloadError::TimeOut:
        case DownloadError::NetworkOperationCanceled:
        case DownloadError::NetworkConnectionRefused:
        case DownloadError::TemporaryNetworkFailure:
        case DownloadError::NetworkProxyConnectionClosed:
        case DownloadError::NetworkRemoteHostClosed:
        case DownloadError::NetworkHostNotFound:
            return true;
        case DownloadError::HttpError:
            {
                switch(errorCode)
                {
                case Origin::Services::Http400ClientErrorBadRequest:
                case Origin::Services::Http404ClientErrorNotFound:
                case Origin::Services::Http408ClientErrorRequestTimeout:
                case Origin::Services::Http500InternalServerError:
                case Origin::Services::Http501NotImplemented:
                case Origin::Services::Http502BadGateway:
                case Origin::Services::Http503ServiceUnavailable:
                case Origin::Services::Http504GatewayTimeout:
                case Origin::Services::Http206PartialContent:
                    return true;
                default:
                    return false;
                }
            }
            default:
                return false;
        }
    }
}

void DownloadSession::checkRetryOnError(DownloadWorker* worker)
{
    if(isErrorRetryable(worker->getCurrentErrorType(), worker->getCurrentErrorCode()))
    {
        switch(worker->getType())
        {
        case DownloadWorker::kTypeCluster:
            {
                DownloadClusterWorker* clusterWorker = static_cast<DownloadClusterWorker*>(worker);
                foreach(DownloadRequest* request, clusterWorker->getRequests())
                {
                    if(!request->isComplete())
                    {
                        if(mShuttingDownSession || !request->retry())
                        {
                            emit(requestError(request->getRequestId(), worker->getCurrentErrorType(), worker->getCurrentErrorCode()));
                        }
                    }
                }
            }
            break;
        case DownloadWorker::kTypeSingle:
            {
                DownloadSingleRequestWorker* singleWorker = static_cast<DownloadSingleRequestWorker*>(worker);
                DownloadRequest *request = singleWorker->getRequest();
                if(mShuttingDownSession || (request && !request->retry(worker, false)))
                {
                    qint64 req_id = -1;
                    if (request)
                        req_id = request->getRequestId();
                    emit(requestError(req_id, worker->getCurrentErrorType(), worker->getCurrentErrorCode()));
                }
            }
            break;
        default:
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Unrecognized worker type [" << worker->getType() << "] cannot retry";
            break;
        }
    }
}

void DownloadSession::workerChunkCompleted(DownloadWorker* worker)
{
    if (!mPhysicalMedia && !mFileOverride)
    {
        QString host_ip(QUrl(worker->getUrl()).host());

        if (worker->getState() == DownloadWorker::kStDownloadComplete)
            mDNSResMap.RecordDownloadRate(host_ip, worker->getEndOffset() - worker->getStartOffset(), worker->getRunTime(), mActiveDownloads.size(), getActiveDownloadWorkers(), getActiveDownloadChannels());

        mDNSResMap.GetBestIP(host_ip);
        worker->setUrl(host_ip);
    }

	mChunkSpeedTracker.addSample(worker->getEndOffset() - worker->getStartOffset(), worker->getRunTime());

    adjustRequestSize(worker);
}

void DownloadSession::tickDownloadWorkers()
{
    // Tick all existing download workers, remove completed workers from active downloads
    QMutableListIterator<DownloadWorker*> iter(mActiveDownloads);
    while(iter.hasNext())
    {
        iter.next();
        if(iter.value()->getState() != DownloadWorker::kStError)
        {
            iter.value()->tick();
        }

        if (iter.value()->getState() == DownloadWorker::kStCompleted || 
            iter.value()->getState() == DownloadWorker::kStPostponed || 
            iter.value()->getState() == DownloadWorker::kStError ||
            iter.value()->getState() == DownloadWorker::kStCanceled)
        {
            if(iter.value()->getState() == DownloadWorker::kStError)
            {
                adjustRequestSize(iter.value());
                checkRetryOnError(iter.value());

                if (!iter.value()->canRemoveOnError())
                    continue;
            }

            iter.value()->reset();
            mWorkerCache[iter.value()->getType()].push_back(iter.value());
            iter.remove();
            g_WorkerSemaphore->release();
        }
    }
}

void DownloadSession::fastForwardDownloadWorkers()
{
    // Tick existing download workers in early stages of request to get the channel active sooner, remove completed workers from active downloads
    for (int i = 0; i < 2; i++)
    {
        QMutableListIterator<DownloadWorker*> iter(mActiveDownloads);
        while(iter.hasNext())
        {
            iter.next();
            if ((iter.value()->getState() == DownloadWorker::kStUninitialized) ||
                (iter.value()->getState() == DownloadWorker::kStInitialized))
            {
                iter.value()->tick();
            }
            else
                continue;

            if (iter.value()->getState() == DownloadWorker::kStCompleted || 
                iter.value()->getState() == DownloadWorker::kStPostponed || 
                iter.value()->getState() == DownloadWorker::kStError ||
                iter.value()->getState() == DownloadWorker::kStCanceled)
            {
                if(iter.value()->getState() == DownloadWorker::kStError)
                {
                    adjustRequestSize(iter.value());
                    checkRetryOnError(iter.value());

                    if (!iter.value()->canRemoveOnError())
                        continue;
                }

                iter.value()->reset();
                mWorkerCache[iter.value()->getType()].push_back(iter.value());
                iter.remove();
                g_WorkerSemaphore->release();
            }
        }
    }
}

QString DownloadSession::getWorkerReportString()
{
    QString report("");

    qint16 active_downloads = 0;

    foreach(DownloadWorker* worker, mActiveDownloads)
    {
        bool cluster_type = worker->getType() == DownloadWorker::kTypeCluster;

        switch (worker->getState())
        {
        case DownloadWorker::kStRunning:
        case DownloadWorker::kStInitialized:    // single req workers can be put into this state before grabbing their next chunk
            active_downloads++;
            if (cluster_type)
                report.append("C");
            else
                report.append("S");
            break;
        case DownloadWorker::kStDownloadComplete:
            if (!cluster_type)
                active_downloads++;    // has completed downloading since another may be on its way.
            if (cluster_type)
                report.append("c");
            else
                report.append("S");
            break;
        default:
            if (cluster_type)
                report.append("c");
            else
                report.append("s");
            break;
        }

        if (worker->isTransferring())
            report.append("+");
        else
            report.append("-");
    }

    return report;
}

void DownloadSession::Heartbeat()
{
    tickDownloadWorkers();

    // Service pending cache and schedule the next heartbeat, or clear all pending requests and exit event thread if session is ended
    if(mShuttingDownSession)
    {
        QMutexLocker lock(&mDownloadRequestMapLock);

        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Canceling and clearing all download requests on event thread exit.";

        // Mark all requests canceled
        for (QMap<RequestId, DownloadRequest*>::Iterator iter = mDownloadRequests.begin(); iter != mDownloadRequests.end(); iter++)
        {
            DownloadRequest *req = *iter;

            // Cancel all requests that haven't been serviced yet, the workers will cancel the ones in progress.
            if((req != NULL) && (req->getWorker() == NULL))
            {
                req->markCanceled();
                emit(requestError(req->getRequestId(), DownloadError::DownloadWorkerCancelled, 0));        
            }
        }

        // Tick all the active workers until they complete
        while(!mActiveDownloads.empty())
        {
            tickDownloadWorkers();
        }

        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Cancel and exit complete.";
        mHeartbeatRunning = false;
        emit(shutdownComplete());
    }
    else
    {
        // Tick the rate limiter so chunks get released if they are available
        mRateLimiter.tick();

        // See if we have any pending requests we can add to the active downloads
        servicePendingRequestQueue();

#ifdef IMPROVED_PIPELINING
        fastForwardDownloadWorkers();   // if any new chunks/requests are setup, make sure we zip past the initial stages to get the http/file channel going immediately
#endif
#ifndef TURN_OFF_MULTIPLE_WORKERS_PER_FILE
        servicePendingRequestQueueExtraWorkers();    // if workers are still free see if there are active requests that need an extra worker 
#ifdef IMPROVED_PIPELINING
        fastForwardDownloadWorkers();
#endif
#endif
        // At this location we have an opportunity to scale up or down the frequency of the heartbeat
        // for now, we will we use the same algorithm as AbstractJob in 8.5
        int nSleep = g_SessionBaseTickRate + (g_RateDecreasePerActiveSession * DownloadService::GetActiveDownloadSessionCount()); // In milliseconds.  Increase sleep significantly for each concurrent session.  
        QTimer::singleShot(nSleep, this, SLOT(Heartbeat()));
    }
}

QString const& DownloadSession::getUrl() const
{
    return mUrl;
}

QString const& DownloadSession::getHostName() const
{
    return mHostName;
}

QString DownloadSession::getPrimaryHostIP() const
{
    return mPrimaryHostIP;
}

bool DownloadSession::isReady() const
{
    return mIoDeviceReady;
}

bool DownloadSession::isRunning() const
{
    return mHeartbeatRunning;
}

void DownloadSession::adjustMemoryAllocated(qint64 bytes)
{
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
    static int sCounter = 0;

    mMemoryAllocated_bytes += bytes;

    if (bytes > 0)
    {
        sCounter++;
        ORIGIN_LOG_DEBUG << "Allocated: " << (float) bytes / (1024.0f * 1024.0f) << " MB  Total: " << (float) mMemoryAllocated_bytes / (1024.0f * 1024.0f) << " MB  High: " << (float) mMemoryHighWatermark / (1024.0f * 1024.0f) << " MB  " << sCounter;
    }
    else
    {
        sCounter--;
        ORIGIN_LOG_DEBUG << "Freed: " << (float) -bytes / (1024.0f * 1024.0f) << " MB  Total: " << (float) mMemoryAllocated_bytes / (1024.0f * 1024.0f) << " MB  High: " << (float) mMemoryHighWatermark / (1024.0f * 1024.0f) << " MB  " << sCounter;
    }
    if (mMemoryHighWatermark < mMemoryAllocated_bytes)
        mMemoryHighWatermark = mMemoryAllocated_bytes;
#endif
}

void DownloadSession::OnEventThreadStartup()
{
    mHeartbeatRunning = true;
    mShuttingDownSession = false;

    // Set the network request access manager to downloader source hint so that we aren't tracked by Heartbeat.cpp and don't incur that overhead
    Origin::Services::NetworkAccessManager::threadDefaultInstance()->setRequestSource(Origin::Services::NetworkAccessManager::DownloaderSourceHint);

    getContentLength();

    QTimer::singleShot(10, this, SLOT(Heartbeat()));
}

void DownloadSession::initialize()
{
    // Ensure this can only be called asynchronously
    ORIGIN_ASSERT(!mHeartbeatRunning);

    if (!mPhysicalMedia && !mFileOverride)
    {
        QString hostName(QUrl(mUrl).host());

        mHostName = hostName;

        // Perform the lookup
        QHostInfo hostInfo(QHostInfo::fromName(hostName));

        if (hostInfo.error() != QHostInfo::NoError)
        {
            ORIGIN_LOG_EVENT << hostInfo.errorString();
        }

        QList<QHostAddress> iplist = hostInfo.addresses();

        if (!iplist.empty())
        {
            ORIGIN_LOG_EVENT << "Host IP: " << iplist.first().toString();

            mPrimaryHostIP = iplist.first().toString();
        }
    }
    else
    {
        mHostName = QString();
    }

    if(mEventThread == NULL)
    {
        mEventThread = new QThread();
        mEventThread->setObjectName("Download Session Event Thread");
        this->moveToThread(mEventThread);
        QObject::connect(mEventThread, SIGNAL(started()), this, SLOT(OnEventThreadStartup()));
        QObject::connect(mEventThread, SIGNAL(finished()), this, SLOT(CleanupThreadData()));
    }

    mEventThread->start();
    Q_ASSERT(mEventThread->isRunning());

    // Wait for event thread to exist
    while(!mHeartbeatRunning)
        Origin::Services::PlatformService::sleep(10);

    mEventThread->setPriority(QThread::LowPriority);

    mIoDeviceReady = true;
}

qint64 DownloadSession::getContentLength()
{
    qint64 len = -1;

    ChannelInterface* syncChannel = NULL;

    if(!mSyncRequestsCanceled && !mSession.isNull())
    {
        syncChannel = ChannelInterface::Create(mUrl, mSession);
        if(syncChannel)
        {
            syncChannel->SetAsyncMode(false);
            syncChannel->Connect();
        }
    }

    if(syncChannel == NULL)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = 0;

        ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Invalid content URL detected: " << logSafeDownloadUrl(mUrl);
        emit (IDownloadSession_error(errorInfo));
    }
    else
    {
        //We will first try with a get request, if that fails we will try with head

        // This range is to request headers
        QString sRequest = "bytes=0-0";
        QString rangeStr = "Range";
        syncChannel->RemoveRequestInfo(rangeStr);
        syncChannel->AddRequestInfo(rangeStr, sRequest);
        syncChannel->RemoveRequestInfo("Host");
        syncChannel->AddRequestInfo("Host", getHostName());

        if(g_logRequestResponseHeader)
        {
            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " Sending GET request with URL (" << mUrl << ").";

            QHash<QString, QString> headerSet = syncChannel->GetAllRequestInfo();
            if (!headerSet.empty())
            {
                int idx = 0;
                for (QHash<QString,QString>::const_iterator it = headerSet.begin(); it != headerSet.end(); it++)
                {
                    ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " GET request key[" << idx << "] = " << it.key() << "; value[" << idx << "] = " << it.value();
                    ++idx;
                }
            }
        }

        int retryCount = 0;
        bool bUseHeadConnection = false;

        while(retryCount < kMaxRetryRequests && 
            !syncChannel->SendRequest("GET", QString(), QString(), QByteArray(), kContentLengthTimeout) && 
            isErrorRetryable(syncChannel->GetLastErrorType(), syncChannel->GetErrorCode()))
        {
            if(mSyncRequestsCanceled || mSession.isNull())
            {
                break;
            }

            Origin::Services::PlatformService::sleep(kDelayBetweenInitializationRetries);
            retryCount++;
        }

        // If enabled in EACore INI file, we wwant to record the GET response header
        // (if any response) regardless of the success
        if(g_logRequestResponseHeader)
        {
            ChannelInfo ci = syncChannel->GetAllResponseInfo();
            QString key, value;
            int idx = 0;
            QHash<QString, QString>::iterator i;
            for (i = ci.begin(); i != ci.end(); ++i)
            {
                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " GET response key[" << idx << "] = " << i.key().toStdString().c_str() << "; value[" << idx << "] = " << i.value().toStdString().c_str();
                ++idx;
            }
        }

        // Get the CDN vendor IP address that was used for the GET request for telemetry and logging
        QString cdnIpAddress(Util::NetUtil::ipFromUrl(mUrl));

        // Used for client-log specifically
        QString loggableUrl(logSafeDownloadUrl(mUrl));

        if (syncChannel->GetLastErrorType() != DownloadError::OK)
        {
            bUseHeadConnection = true;

            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to get content length with error [GET] [" << 
                Downloader::ErrorTranslator::Translate((ContentDownloadError::type)syncChannel->GetLastErrorType()) << 
                "] error code [" << syncChannel->GetErrorCode()<< "] " << loggableUrl << " (" << cdnIpAddress << ")";

            GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "GET", "FAILURE", "0-0", "0", cdnIpAddress.toLocal8Bit().constData());
        }
        else if ( syncChannel->HasResponseInfo("Content-Range") )        // Content-Range: bytes 0-0/#
        {
            QString szLen = syncChannel->GetResponseInfo("Content-Range");

            ORIGIN_LOG_EVENT << "Server Responded with Content-Range: \"" << szLen << "\"";

            int nIndexOfSlash = szLen.lastIndexOf(L'/');
            QString tmpBytesStr = szLen.right(szLen.length() - (nIndexOfSlash + 1));
            qint64 nTmpTotalBytes = tmpBytesStr.toLongLong();

            if(nTmpTotalBytes < 0)
            {
                bUseHeadConnection = true;

                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to get content length with error [GET] Content-Range [" << 
                    szLen << "] calculated as negative number: " << tmpBytesStr << ", " << loggableUrl << " (" << cdnIpAddress << ")";

                GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "GET", "FAILURE", szLen.toUtf8().data(), tmpBytesStr.toUtf8().data(), cdnIpAddress.toLocal8Bit().constData());
            }
            else
            {
                emit ( initialized(nTmpTotalBytes));

                QStringList contentLengthSplit = szLen.split("/",QString::SkipEmptyParts); // split on / : "bytes 0-0", "212121"
                QString startEnd = "0";

                if (contentLengthSplit.size() == 2)
                {
                    QStringList startEndSplit = contentLengthSplit.at(0).split("bytes",::QString::SkipEmptyParts); // split on bytes : "0-0"
                    if(startEndSplit.size() == 1)
                        startEnd = startEndSplit.join("");
                }

                GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "GET", "SUCCESS", startEnd.toUtf8().data(), tmpBytesStr.toUtf8().data(), cdnIpAddress.toLocal8Bit().constData());
            }
        }
        else
        {
            bUseHeadConnection = true;
            ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to determine content length for URL [GET]: " << loggableUrl << " (" << cdnIpAddress << ")";
            GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "GET", "FAILURE", "0-0", "0", cdnIpAddress.toLocal8Bit().constData());

        }
        //we failed with the get so try with the HEAD connection
        if(bUseHeadConnection && !mSyncRequestsCanceled && !mSession.isNull())
        {
            int retryCount = 0;

            //remove the request info we set for the Get Request
            syncChannel->RemoveRequestInfo(rangeStr);

            if(g_logRequestResponseHeader)
            {
                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " Sending HEAD request with URL (" << mUrl << ").";

                QHash<QString, QString> headerSet = syncChannel->GetAllRequestInfo();
                if (!headerSet.empty())
                {
                    int idx = 0;
                    for (QHash<QString,QString>::const_iterator it = headerSet.begin(); it != headerSet.end(); it++)
                    {
                        ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " HEAD request key[" << idx << "] = " << it.key() << "; value[" << idx << "] = " << it.value();
                        ++idx;
                    }
                }
            }

            while(retryCount < kMaxRetryRequests && 
                !syncChannel->SendRequest("HEAD", QString(), QString(), QByteArray(), kContentLengthTimeout) && 
                isErrorRetryable(syncChannel->GetLastErrorType(), syncChannel->GetErrorCode()))
            {
                if(mSyncRequestsCanceled || mSession.isNull())
                {
                    break;
                }

                Origin::Services::PlatformService::sleep(kDelayBetweenInitializationRetries);
                retryCount++;
            }

            // If enabled in EACore INI file, we wwant to record the HEAD response header
            // (if any response) regardless of the success
            if(g_logRequestResponseHeader)
            {
                ChannelInfo ci = syncChannel->GetAllResponseInfo();
                QString key, value;
                int idx = 0;
                QHash<QString, QString>::iterator i;
                for (i = ci.begin(); i != ci.end(); ++i)
                {
                    ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << " HEAD response key[" << idx << "] = " << i.key().toStdString().c_str() << "; value[" << idx << "] = " << i.value().toStdString().c_str();
                    ++idx;
                }
            }

            // Since GET request failed, lookup CDN IP address again that was used for HEAD request,
            // in case it changed.
            cdnIpAddress = Util::NetUtil::ipFromUrl(mUrl);

            if (syncChannel->GetLastErrorType() != DownloadError::OK)
            {
                // :WARN: we must report content length info before reporting the download error due
                // to the way download sessions are reported in telemetry. When a download error is reported,
                // the session tracking in telemetry is removed (internally) until the download is resumed.
                GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "HEAD", "FAILURE", "0-0", "0", cdnIpAddress.toLocal8Bit().constData());

                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = syncChannel->GetLastErrorType();
                errorInfo.mErrorCode = syncChannel->GetErrorCode();

                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to get content length with error [HEAD] [" << 
                    Downloader::ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType) << 
                    "] error code [" << errorInfo.mErrorCode << "] " << loggableUrl << " (" << cdnIpAddress << ")";

                emit (IDownloadSession_error(errorInfo));                  
            }

            else if (syncChannel->HasResponseInfo("Content-Length") )
            {
                QString lengthStr = syncChannel->GetResponseInfo("Content-Length"); // typically this value is always the content length
                int nIndexOfSlash = lengthStr.lastIndexOf(L'/');
                lengthStr = lengthStr.right(lengthStr.length() - (nIndexOfSlash + 1));
                len = lengthStr.toULongLong();

                if(len < 0)
                {
                    QString errorHeader = syncChannel->GetResponseInfo("Content-Length");
                    ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to get content length with error [HEAD] Content-Length [" << 
                        errorHeader << "] calculated as negative number: " << lengthStr << ", " << loggableUrl << " (" << cdnIpAddress << ")";

                    // :WARN: we must report content length info before reporting the download error due
                    // to the way download sessions are reported in telemetry. When a download error is reported,
                    // the session tracking in telemetry is removed (internally) until the download is resumed.
                    GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "HEAD", "FAILURE", errorHeader.toUtf8().data(), lengthStr.toUtf8().data(), cdnIpAddress.toLocal8Bit().constData());

                    CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                    errorInfo.mErrorType = DownloadError::ContentLength;
                    errorInfo.mErrorCode = 0;
                    emit (IDownloadSession_error(errorInfo));
                }
                else
                {
                    emit ( initialized(len));
                    GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "HEAD", "SUCCESS", "0-0", lengthStr.toUtf8().data(), cdnIpAddress.toLocal8Bit().constData());
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Failed to determine content length for URL  [HEAD] : " << loggableUrl <<
                    " (" << cdnIpAddress << ")";

                // :WARN: we must report content length info before reporting the download error due
                // to the way download sessions are reported in telemetry. When a download error is reported,
                // the session tracking in telemetry is removed (internally) until the download is resumed.
                GetTelemetryInterface()->Metric_DL_CONTENT_LENGTH(mId.toUtf8().data(), mUrl.toLocal8Bit().constData(), "HEAD", "FAILURE", "0-0", "0", cdnIpAddress.toLocal8Bit().constData());

                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = DownloadError::ContentLength;
                errorInfo.mErrorCode = 0;
                emit (IDownloadSession_error(errorInfo));
            }
        }

        delete syncChannel;
    }

    return len;
}

void DownloadSession::submitRequest(RequestId reqId, QSet<int> priorityGroupIds, bool priorityGroupEnabled)
{
    // Ensure this can only be called asynchronously
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(RequestId, reqId), Q_ARG(QSet<int>, priorityGroupIds), Q_ARG(bool, priorityGroupEnabled));

    // If we aren't initialized
    if (!mIoDeviceReady)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;
        emit (IDownloadSession_error(errorInfo));
        return;
    }

    if (!mDownloadRequests.contains(reqId))
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;
        emit (IDownloadSession_error(errorInfo));
        return;
    }

    // Make sure we can't do this while the queue is being accessed
    QMutexLocker lock(&mDownloadRequestMapLock);

    DownloadRequest *ireq = mDownloadRequests[reqId];
    if (ireq)
    {
        // Insert the request into all associated priority groups
        for (QSet<int>::iterator groupIdIter = priorityGroupIds.begin(); groupIdIter != priorityGroupIds.end(); ++groupIdIter)
        {
            mPriorityQueue.Insert(reqId, *groupIdIter, priorityGroupEnabled);
        }

        ireq->markSubmitted();
    }
}

void DownloadSession::submitRequests(QVector<RequestId> reqIds, int priorityGroupId, bool priorityGroupEnabled)
{
    // Ensure this can only be called asynchronously
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QVector<RequestId>, reqIds), Q_ARG(int, priorityGroupId), Q_ARG(bool, priorityGroupEnabled));

    // If we aren't initialized
    if (!mIoDeviceReady)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;
        return;
    }

    // Make sure we can't do this while the queue is being accessed
    QMutexLocker lock(&mDownloadRequestMapLock);

    for (int i = 0; i < reqIds.count(); ++i)
    {
        RequestId reqId = reqIds[i];

        if (!mDownloadRequests.contains(reqId))
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
            errorInfo.mErrorCode = -1;
            return;
        }

        DownloadRequest *ireq = mDownloadRequests[reqId];
        if (ireq)
        {
            // Note: Some requests may be submitted multiple times if they appear in multiple priority groups
            // This is acceptable.
            ireq->markSubmitted();
        }
    }

    // Add them to the priority queue
    mPriorityQueue.Insert(reqIds, priorityGroupId, priorityGroupEnabled);
}

void DownloadSession::priorityMoveToTop(int priorityGroupId)
{
    // Ensure this can only be called asynchronously
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(int, priorityGroupId));

    // If we aren't initialized
    if (!mIoDeviceReady)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;

        // TODO do something with this
        ORIGIN_LOG_ERROR << "Invalid call to priorityMoveToTop, session not ready.";

        return;
    }
    else
    {
        // Make sure we can't do this while the queue is being accessed
        QMutexLocker lock(&mDownloadRequestMapLock);

        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Move to top: " << priorityGroupId;

        mPriorityQueue.MoveToTop(priorityGroupId);

        priorityQueueSendUpdatedOrder();

        // Postpone active download workers to allow higher priority queue items to move ahead
        priorityQueuePostponeActiveWorkers();
    }
}

void DownloadSession::prioritySetQueueOrder(QVector<int> priorityGroupIds)
{
    // Ensure this can only be called asynchronously
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QVector<int>, priorityGroupIds));

    // If we aren't initialized
    if (!mIoDeviceReady)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;

        // TODO do something with this
        ORIGIN_LOG_ERROR << "Invalid call to prioritySetQueueOrder, session not ready.";

        return;
    }
    else
    {
        // Make sure we can't do this while the queue is being accessed
        QMutexLocker lock(&mDownloadRequestMapLock);

        QString queue;
        for (QVector<int>::iterator groupIter = priorityGroupIds.begin(); groupIter != priorityGroupIds.end(); ++groupIter)
        {
            if (!queue.isEmpty())
                queue.append(", ");
            queue.append(QString("%1").arg(*groupIter));
        }
        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Set Queue Order: " << queue;

        mPriorityQueue.SetQueueOrder(priorityGroupIds);

        priorityQueueSendUpdatedOrder();

        if (priorityGroupIds.count() > 0)
        {
            ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Postponing all active downloads for queue order change.";

            // Postpone active download workers to allow higher priority queue items to move ahead
            priorityQueuePostponeActiveWorkers();
        }
    }
}

void DownloadSession::prioritySetGroupEnableState(int priorityGroupId, bool enabled)
{
    // Ensure this can only be called asynchronously
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(int, priorityGroupId), Q_ARG(bool, enabled));

    // If we aren't initialized
    if (!mIoDeviceReady)
    {
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = DownloadError::InvalidDownloadRequest;
        errorInfo.mErrorCode = -1;

        // TODO do something with this
        ORIGIN_LOG_ERROR << "Invalid call to prioritySetGroupEnableState, session not ready.";

        return;
    }
    else
    {
        // Make sure we can't do this while the queue is being accessed
        QMutexLocker lock(&mDownloadRequestMapLock);

        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Set Queue Group " << priorityGroupId << " Enabled Status: " << enabled;

        mPriorityQueue.SetGroupEnabledStatus(priorityGroupId, enabled);
    }
}

QVector<int> DownloadSession::priorityGetQueueOrder()
{
    // Make sure we can't do this while the queue is being accessed
    QMutexLocker lock(&mDownloadRequestMapLock);

    return priorityQueueGetUpdatedOrder();
}

// Lock before calling this function
void DownloadSession::priorityQueuePostponeActiveWorkers()
{
    for (QList<DownloadWorker*>::iterator workerIter = mActiveDownloads.begin(); workerIter != mActiveDownloads.end(); ++workerIter)
    {
        // Only single requests are postponable
        if ((*workerIter)->getType() == DownloadWorker::kTypeSingle)
        {
            DownloadSingleRequestWorker* singleWorker = static_cast<DownloadSingleRequestWorker*>(*workerIter);

            // Postpone the request
            DownloadRequest* req = singleWorker->getRequest();
            if (req)
            {
                ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Postpone active request id: " << req->getRequestId();
                req->markPostponed(true);
            }
        }
    }
}

// Lock before calling this function
QVector<int> DownloadSession::priorityQueueGetUpdatedOrder()
{
    QLinkedList<Origin::Downloader::DownloadPriorityGroup>& queue = mPriorityQueue.GetQueue();
    
    QVector<int> queueOrder(queue.count());

    QString queueStr;
    int idx = 0;
    for (QLinkedList<Origin::Downloader::DownloadPriorityGroup>::iterator iter = queue.begin(); iter != queue.end(); ++iter)
    {
        if (!queueStr.isEmpty())
            queueStr.append(", ");

        int groupId = (*iter).mPriorityGroupId;

        queueOrder[idx++] = groupId;

        queueStr.append(QString("%1").arg(groupId));
    }

    ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "[PRIORITY QUEUE] Queue Order: " << queueStr;

    return queueOrder;
}

void DownloadSession::priorityQueueSendUpdatedOrder()
{
    emit priorityQueueOrderUpdate(priorityQueueGetUpdatedOrder());
}

void DownloadSession::cancelSyncRequests()
{
    QMutexLocker lock(&mSyncRequestLock);
    foreach(SyncRequestHelper* request, mActiveSyncRequests)
    {
        request->cancel();
    }
}

void DownloadSession::addActiveSyncRequest(SyncRequestHelper* request)
{
    QMutexLocker lock(&mSyncRequestLock);
    mActiveSyncRequests.insert(request);
}

void DownloadSession::removeActiveSyncRequest(SyncRequestHelper* request)
{
    QMutexLocker lock(&mSyncRequestLock);
    mActiveSyncRequests.remove(request);
}

// This method is intended to be executed synchronously and so we do NOT fire signals
bool DownloadSession::requestAndWait(RequestId reqId, int timeout)
{
    // If we aren't initialized
    if (!mIoDeviceReady || !mDownloadRequests.contains(reqId))
    {
        ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "Invalid request.";
        return false;
    }

    DownloadRequest *ireq = mDownloadRequests[reqId];

    SyncRequestHelper helper(ireq, mSession);
    addActiveSyncRequest(&helper);
    bool result = helper.requestAndWait(timeout);
    removeActiveSyncRequest(&helper);
    return result;
}

RequestId DownloadSession::createRequest(qint64 byteStart, qint64 byteEnd)
{
    QMutexLocker lock(&mDownloadRequestMapLock);

    if(byteStart < 0 || byteEnd < byteStart)
    {
        ORIGIN_LOG_ERROR << SESSIONLOG_PREFIX << "CreateRequest called with invalid range [" << byteStart << " to " << byteEnd << "].";
    }

    DownloadRequest *req = new DownloadRequest(this, byteStart, byteEnd);
    mDownloadRequests[req->getRequestId()] = req;
    return req->getRequestId();
}

IDownloadRequest* DownloadSession::getRequestById(RequestId reqId)
{
    QMutexLocker lock(&mDownloadRequestMapLock);

    if (mDownloadRequests.contains(reqId))
        return mDownloadRequests[reqId];
    return NULL;
}

void DownloadSession::closeRequest(RequestId reqId)
{
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(RequestId, reqId));

    QMutexLocker lock(&mDownloadRequestMapLock);

    if (mDownloadRequests.contains(reqId))
    {
        DownloadRequest *creq = mDownloadRequests[reqId];
        mDownloadRequests.remove(reqId);

        // Sanity check
        if (creq)
        {
            int worker_count = creq->numWorkers();

            if(worker_count)
            {
                DownloadWorker* worker = NULL;

                // Worker(s) must delete the request
                for (int i = 0; i < worker_count; i++)
                {
                    worker = creq->getWorker();
                    if (worker)
                        worker->requestClosed(reqId, mShuttingDownSession);
                }
            }
            else
            {
                creq->deleteLater();
            }
        }
        else
        {
            ORIGIN_LOG_WARNING << "Closing a NULL request: [" << reqId << "]";
        }
    }
    else
    {
        ORIGIN_LOG_WARNING << "Closing a request that isn't being tracked: [" << reqId << "]";
    }
}

void DownloadSession::shutdown()
{
    ASYNC_INVOKE_GUARD;

    // Shutdown the event thread
    if(!mShuttingDownSession)
    {
        // Cancel all active workers
        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Canceling all active download workers...";
        foreach(DownloadWorker* worker, mActiveDownloads)
        {
            worker->cancel();
        }

        ORIGIN_LOG_EVENT << SESSIONLOG_PREFIX << "Shutting down event thread...";

        mShuttingDownSession = true;
    }
}

qint64 DownloadSession::getIdealRequestSize() const 
{ 
    return mIdealRequestSize; 
}

qint64 DownloadSession::getRequestSize() const
{
    return mCurRequestSize;
}

qint64 DownloadSession::getMinRequestSize() const
{
    return g_minRequestSize;
}

void DownloadSession::workerChunkAvailable(RequestId reqId)
{
    // Only engage the rate limiter if the download utilization is being capped (reduce overhead)
    if (DownloadService::GetDownloadUtilization() <= 1.0f)
    {
        // If we can get the data we need, pass it to the rate limiter
        IDownloadRequest* req = getRequestById(reqId);
        if (req && req->chunkIsAvailable())
        {
            mRateLimiter.chunkAvailable(reqId, req->getDataBuffer()->TotalSize());
            return;
        }
    }

    // Just do the default behavior
    emit(requestChunkAvailable(reqId));
}

void DownloadSession::limiterChunkAvailable(RequestId reqId)
{
    emit(requestChunkAvailable(reqId));
}

void DownloadSession::workerError(QVector<DownloadRequest*> requests, qint32 errorType, qint32 errorCode, DownloadWorker* worker)
{
    RequestId reqId;
    bool sendDownloadSessionError = false;
    bool retryable = isErrorRetryable((DownloadError::type)errorType, errorCode);

    if(requests.isEmpty())
    {
        if (worker && !mPhysicalMedia && !mFileOverride && retryable)
        {
            QString host_ip(QUrl(worker->getUrl()).host());
            mDNSResMap.HandleError(host_ip, (DownloadError::type)errorType, errorCode);
        }

        if(errorType != DownloadError::DownloadWorkerCancelled)
            sendDownloadSessionError = true;
    }
    else
    {
        foreach(DownloadRequest* req, requests)
        {
            reqId = req->getRequestId();
            if (worker && !mPhysicalMedia && !mFileOverride && retryable)
            {
                QString host_ip(QUrl(worker->getUrl()).host());
                mDNSResMap.HandleError(host_ip, (DownloadError::type)errorType, errorCode);
            }

            if(!retryable)
            {
                if(errorType != DownloadError::DownloadWorkerCancelled)
                    sendDownloadSessionError = true;

                emit(requestError(reqId, (mPhysicalMedia ? DownloadError::FileRead : errorType), errorCode));
            }
        }
    }

    if(sendDownloadSessionError)
    {
        // TELEMETRY:  Send raw error code
        QString errorString = QString("Type=%1,Code=%2").arg(errorType).arg(errorCode);
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("WorkerErrorRaw", errorString.toUtf8().data());

        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = (mPhysicalMedia ? DownloadError::FileRead : errorType);
        errorInfo.mErrorCode = errorCode;

        emit(IDownloadSession_error(errorInfo));
    }
}

void DownloadSession::workerPostponed(RequestId reqId)
{
    if (reqId == -1)
        return;

    emit requestPostponed(reqId);
}

#if defined(_DEBUG)

QMutex DownloadRequest::mInstanceDebugMapMutex;
QSet<DownloadRequest*> DownloadRequest::mInstanceDebugMap;
void DownloadRequest::checkForMemoryLeak()
{
    QMutexLocker locker(&mInstanceDebugMapMutex);
    // If this debug fires, all responses in the map are leaking
    ORIGIN_ASSERT_MESSAGE(mInstanceDebugMap.isEmpty(), "We are leaking DownloadRequest instances");
    // a bit of help with debugging...
    int count = mInstanceDebugMap.size();
    Q_UNUSED(count);
    for ( QSet<DownloadRequest*>::const_iterator i = mInstanceDebugMap.begin(); i != mInstanceDebugMap.end(); ++i )
    {
        DownloadRequest* curr = *i;
        Q_UNUSED(curr);
    }
}
void DownloadRequest::insertMe()
{
    QMutexLocker locker(&mInstanceDebugMapMutex);
    mInstanceDebugMap.insert(this);
}
void DownloadRequest::removeMe()
{
    QMutexLocker locker(&mInstanceDebugMapMutex);
    mInstanceDebugMap.remove(this);
}

#endif

// IDownloadRequest members

DownloadRequest::DownloadRequest(DownloadSession* session, qint64 byteStart, qint64 byteEnd) : 
    mRequestId(g_lastRequestId++),
    mSession(session),
    _isSubmitted(false),
    _isComplete(false),
    _isCanceled(false),
    _isPostponed(false),
    _isDiagnosticMode(false),
    _retryCount(0),
    _lastAttempt(0),
    _errorState(0),
    _errorCode(0),
    _byteStart(byteStart),
    _byteEnd(byteEnd),
    mBytesRead(0),
    mIsRemoveWorkers(false),
    mIsDownloadingComplete(false),
    mIsHandlingWorkerError(false),
    mDataContainersProcessing(false),
    mBytesRemaining(0),
    mCurrentBytesRequested(0),
    mProcessingIndex(0),
    mOrderIndex(0),
    mDataContainersLock(QMutex::Recursive),
    _chunkDataReady(false),
    _isInProgress(false)
{
    insertMe();

    mRequestWorkerList.clear();

    for (int i = 0; i < kMaxChunkDataContainers; i++)
    {
        mDataContainers[i].used = false;
        mDataContainers[i].index = -1;
        mAvailableDataContainers.push_back(&mDataContainers[i]);
    }

    mQueuedDataContainers.clear();

    // Automatically run in diagnostic mode when safe mode is enabled
    if (session->isSafeMode())
        _isDiagnosticMode = true;
}

DownloadRequest::~DownloadRequest()
{
    removeMe();
}

const DownloadSession* DownloadRequest::getSession() const
{
    return mSession;
}

qint64 DownloadRequest::getRequestId() const
{
    return mRequestId;
}

qint64 DownloadRequest::getStartOffset() const
{
    return _byteStart;
}

qint64 DownloadRequest::getEndOffset() const
{
    return _byteEnd;
}

qint64 DownloadRequest::getTotalBytesRead() const
{
    return mBytesRead;
}

qint64 DownloadRequest::getTotalBytesRequested() const
{
    return (_byteEnd - _byteStart);
}

DownloadWorker* DownloadRequest::getWorker()
{
    QMutexLocker lock(&mRequestWorkerListLock);

    if (mRequestWorkerList.empty())
        return NULL;
    return mRequestWorkerList[0];
}

void DownloadRequest::setWorker(DownloadWorker* worker)
{
    QMutexLocker lock(&mRequestWorkerListLock);

    if (worker == NULL)
        mRequestWorkerList.clear();
    else
    if (!mRequestWorkerList.contains(worker))
        mRequestWorkerList.push_back(worker);
}

// remove worker if in list, return true if list is not empty (false otherwise).
bool DownloadRequest::removeWorker(DownloadWorker* worker)
{
    QMutexLocker lock(&mRequestWorkerListLock);

    if (mRequestWorkerList.contains(worker))
    {
        mRequestWorkerList.removeOne(worker);
    }

    return !mRequestWorkerList.empty();
}

int DownloadRequest::numWorkers()
{ 
    QMutexLocker lock(&mRequestWorkerListLock);

    return mRequestWorkerList.count();
}

bool DownloadRequest::isSubmitted() const
{
    return _isSubmitted;
}

bool DownloadRequest::isComplete() const
{
    return _isComplete;
}

bool DownloadRequest::isCanceled() const
{
    return _isCanceled;
}

bool DownloadRequest::isPostponed() const
{
    return _isPostponed;
}

bool DownloadRequest::isDiagnosticMode() const
{
    return _isDiagnosticMode;
}

qint32 DownloadRequest::getErrorState() const
{
    return _errorState;
}

qint32 DownloadRequest::getErrorCode() const
{
    return _errorCode;
}

bool DownloadRequest::chunkIsAvailable()
{
    QMutexLocker lock(&mDataContainersLock);

    if (mQueuedDataContainers.empty())
        return false;

    ChunkDataContainerType *container = mQueuedDataContainers.first();

    return container->index == mProcessingIndex;//_chunkDataReady;
}

bool DownloadRequest::isDataContainerAvailable(qint32 index)
{
    QMutexLocker lock(&mDataContainersLock);

#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
    if (mAvailableDataContainers.empty())
    {   // display queue
        int n = mQueuedDataContainers.size();

        QString index_string("(");

        for (int i = 0; i < n; i++)
        {
            ChunkDataContainerType *container = mQueuedDataContainers[i];
            if (container)
            {
                index_string.append(QString("%1").arg(container->index));
                if (i != (n-1))
                    index_string.append(QString("->"));
            }
        }
        index_string.append(QString(")"));

        ORIGIN_LOG_EVENT << "Req: " << mRequestId << "  QUEUED DATA CONTAINERS: " << mProcessingIndex << "  Asking: " << index << "  " << index_string;
    }
#endif
    // test for case where we have one space left, the queue is full, and we are waiting for one particular index

    bool ret = false;

    if ((index != -1) && (mAvailableDataContainers.size() == 1))
    {
        if (mDataContainersProcessing)
            return false;   // hold off on queuing new containers for this case

        ChunkDataContainerType *container = mQueuedDataContainers.first();
        ret = ((container->index > mProcessingIndex) && (mProcessingIndex == index)); // only give this last container to the index we are waiting for

        return ret;
    }

    ret = !mAvailableDataContainers.empty();

    return ret;
}

MemBuffer* DownloadRequest::getDataBuffer()
{
    QMutexLocker lock(&mDataContainersLock);

    if (mQueuedDataContainers.empty())
        return NULL;    // error

    ChunkDataContainerType *container = mQueuedDataContainers.first();

    ORIGIN_ASSERT(mProcessingIndex == container->index);    // should not be calling getDataBuffer() unless the proper processing index matches the first queued container

    return &container->data_buffer;
}

ChunkDataContainerType* DownloadRequest::getAvailableDataContainer(qint32 order_index)
{
    QMutexLocker lock(&mDataContainersLock);

    if (mAvailableDataContainers.empty())
        return NULL;

    ChunkDataContainerType *container = mAvailableDataContainers[0];
    container->used = true;
    container->index = order_index;
    container->clearDiagnosticInfo(); // If any was in use
    mAvailableDataContainers.pop_front();
    return container;
}

qint32 DownloadRequest::incrementOrderIndex()
{
    return mOrderIndex++;
}

void DownloadRequest::setProcessingDataFlag(bool processing) 
{ 
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
    ORIGIN_LOG_DEBUG << "[" << mRequestId << "] processing first queued chunk " << mProcessingIndex << "    **********************************************";
#endif
    mDataContainersProcessing = processing;
}

void DownloadRequest::queueDataContainer(ChunkDataContainerType *container)
{
    QMutexLocker lock(&mDataContainersLock);

    // insert in order of index
    int num = mQueuedDataContainers.size();
    if (num == 0)
    {
        mQueuedDataContainers.push_back(container);
    }
    else
    {
        for (int i = 0; i < num; i++)
        {
            if (container->index < mQueuedDataContainers[i]->index)
            {
                mQueuedDataContainers.insert(i, container);
                return;
            }
        }

        mQueuedDataContainers.append(container);
    }
}

void DownloadRequest::chunkMarkAsRead()
{
    mDataContainersLock.lock();
    ChunkDataContainerType *container = mQueuedDataContainers.first();
    mQueuedDataContainers.pop_front();
    container->used = false;
    container->clearDiagnosticInfo(); // If any was in use
    mProcessingIndex = container->index + 1;    // set the next chunk index to wait for
    container->index = -1;
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
    if (container->data_buffer.IsOwner())
        mSession->adjustMemoryAllocated(-(qint64) container->data_buffer.TotalSize());
#endif
    container->data_buffer.Destroy();

    mAvailableDataContainers.push_back(container);

    mDataContainersLock.unlock();

    // check for any queued up data after this
    if (chunkIsAvailable())
    {
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
        ORIGIN_LOG_DEBUG << "[" << mRequestId << "] processing queued chunk " << mProcessingIndex << "    **********************************************";
#endif
        mSession->workerChunkAvailable(getRequestId());
    }
    else
    {
        mDataContainersProcessing = false;  // need to make thread safe
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
        ORIGIN_LOG_DEBUG << "[" << mRequestId << "] processing done. " << mProcessingIndex << "    **********************************************";
#endif

        if (isDownloadingComplete())
        {
            _isComplete = true;
        }
    }
//    _chunkDataReady = false;
}

QSharedPointer<DownloadChunkDiagnosticInfo> DownloadRequest::getChunkDiagnosticInfo()
{
    QMutexLocker lock(&mDataContainersLock);

    if (mQueuedDataContainers.empty())
        return QSharedPointer<DownloadChunkDiagnosticInfo>(NULL);    // error

    ChunkDataContainerType *container = mQueuedDataContainers.first();

    ORIGIN_ASSERT(mProcessingIndex == container->index);    // should not be calling getDataBuffer() unless the proper processing index matches the first queued container

    return container->diagnostic_info;
}


// internal members
void DownloadRequest::markSubmitted()
{
    _isSubmitted = true;
}

bool DownloadRequest::isInProgress() const
{
    return _isInProgress;
}

bool DownloadRequest::hasMoreDataToDownload() const
{
    return (mBytesRemaining > 0);
}

void DownloadRequest::setDownloadingComplete(bool complete)
{ 
    mIsDownloadingComplete = complete; 

    if (complete)
    {   // mark all workers attached to this request to completed to prevent uninitialized workers from getting set to an error state
        QMutexLocker lock(&mRequestWorkerListLock);

        int num = mRequestWorkerList.size();
        for (int i = 0; i < num; i++)
            mRequestWorkerList[i]->setToIdle();
    }
}

void DownloadRequest::setBytesRemaining(qint64 bytes_remaining)
{
    mBytesRemaining = bytes_remaining;
}

void DownloadRequest::markCanceled()
{
    _isCanceled = true;
}

void DownloadRequest::setDiagnosticMode(bool val)
{
    _isDiagnosticMode = val;
}

void DownloadRequest::markInProgress(bool val)
{
    _isInProgress = val;
}

void DownloadRequest::markPostponed(bool val)
{
    _isPostponed = val;
}

bool DownloadRequest::waitBeforeRetry()
{
    if(_lastAttempt == 0)
    {
        return false;
    }
    else 
    {
        bool retry_val = (GetTick() - _lastAttempt) < kDelayBetweenRequestRetries;

        if (retry_val == false)
            _lastAttempt = 0;    // just so we don't keep calling GetTick()
        return (retry_val);
    }
}

bool DownloadRequest::retry(bool resetBytesRead)
{
    if(_retryCount < kMaxRetryRequests)
    {
        _retryCount++;
        _isInProgress = false;
        _chunkDataReady = false;
        _isSubmitted = true;
        _isComplete = false;
        _isCanceled = false;
        _errorState = 0;
        _errorCode = 0;
        _isInProgress = 0;
        _lastAttempt = GetTick();
        mIsDownloadingComplete = false;

        // remove worker(s)
        QMutexLocker lock(&mRequestWorkerListLock);
        mRequestWorkerList.clear();


        if(resetBytesRead)
            mBytesRead = 0;

        return true;
    }
    else
    {
        return false;
    }
}

// version of retry for single requests with possible multiple workers assigned
// this is tricky, but the basic idea is to wait until we are in the state where
// no data is being processed for this request, then clear out the workers and reset
// the indexes and data containers to the state we last processed.
bool DownloadRequest::retry(DownloadWorker* singleWorker, bool resetBytesRead)
{
    mIsHandlingWorkerError = true;
 
    if (!isProcessingData() && areAllWorkersIdle())
    {
        bool retry = _retryCount < kMaxRetryRequests;
        handleWorkerError(singleWorker, retry);
        return (retry);
    }

    return true;
}

bool DownloadRequest::areAllWorkersIdle()
{
    QMutexLocker lock(&mRequestWorkerListLock);

    int num = mRequestWorkerList.size();

    for (int i = 0; i < num; i++)
    {
        if ((mRequestWorkerList[i]->getState() != DownloadWorker::kStDownloadComplete ) && (mRequestWorkerList[i]->getState() != DownloadWorker::kStError ))
            return false;
    }
    return true;
}

// clear out data and reset indices to attempt a retry
void DownloadRequest::handleWorkerError(DownloadWorker *worker, bool retrying)
{
    mDataContainersLock.lock();

    int num = mQueuedDataContainers.size();

#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
    ORIGIN_LOG_ERROR << "[" << mRequestId << "] Multi-worker Error Handling:  workers: " << mRequestWorkerList.size() << " # queued containers: " << num << "  current order index: " << mOrderIndex << "  bytes read: " << mBytesRead << " remaining: " << mBytesRemaining;
#endif

    // now clear any data already read by the workers
    for(int i = 0; i < num; i++)
    {
        ChunkDataContainerType *container = mQueuedDataContainers.first();
        mQueuedDataContainers.pop_front();
        container->used = false;
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
        ORIGIN_LOG_ERROR << "  " << (i+1) << ") index: " << container->index << " size: " << container->data_buffer.TotalSize();
#endif
        container->index = -1;
        mBytesRead -= container->data_buffer.TotalSize();

        container->data_buffer.Destroy();   // free the data

        mAvailableDataContainers.push_back(container);
    }

    mOrderIndex = mProcessingIndex; // set it back

    mDataContainersLock.unlock();

    if (retrying)   // if we are retrying, have workers remove themselves from the active list, otherwise let the CloseRequest() call do it.
    {
        QMutexLocker lock(&mRequestWorkerListLock);
        mRequestWorkerList.clear();
        mIsHandlingWorkerError = false; 

        _retryCount++;
        _isInProgress = false;
        _chunkDataReady = false;
        _isSubmitted = true;
        _isComplete = false;
        _isCanceled = false;
        _errorState = 0;
        _errorCode = 0;
        _isInProgress = 0;
        _lastAttempt = GetTick();
        mIsDownloadingComplete = false;
    }
    else
    {
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
        ORIGIN_LOG_ERROR << "Not Retrying";
#endif
        mIsRemoveWorkers = true;

        // remove the error state worker from the requests list as we all letting the closeRequest() from ContentProtocol remove the extra workers
        removeWorker(worker);
    }

    mCurrentBytesRequested = mBytesRead;
    mBytesRemaining = getTotalBytesRequested() - mBytesRead;

#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
    ORIGIN_LOG_ERROR << "  New order index: " << mOrderIndex << "  bytes read: " << mBytesRead << " remaining: " << mBytesRemaining;
#endif
}

bool DownloadRequest::containsWorker(DownloadWorker *worker)
{
    QMutexLocker lock(&mRequestWorkerListLock);

    return(mRequestWorkerList.contains(worker));
}

void DownloadRequest::setErrorState(qint32 errorState, qint32 errorCode)
{
    _errorState = errorState;
    _errorCode = errorCode;
}

void DownloadRequest::setRequestState(qint64 bytesRead, bool chunkAvailable, bool isComplete, qint32 errorState, qint32 errorCode)
{
    mBytesRead = bytesRead;
    _chunkDataReady = chunkAvailable;
    _isComplete = isComplete;
    _errorState = errorState;
    _errorCode = errorCode;
}

} // namespace Downloader
} // namespace Origin

