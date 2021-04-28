//
// Copyright 2008, Electronic Arts Inc
//

#include <QMutex>
#include "TransferStats.h"
#include <time.h>
#include "services/downloader/Timer.h"
#include "services/downloader/MovingAverage.h"
#include "services/log/LogService.h"
#include <limits>

#define DEFAULT_MAX_DL_HISTORY 20

#define UPDATE_INTERVAL 2000		//stats update interval

namespace Origin
{
namespace Downloader
{

DownloadRateSampler::DownloadRateSampler(int numSamples) :
    _circularSamplesBufferIndex(0),
    _oldestTimestamp(std::numeric_limits<qint64>::max()),
    _totalBytesInWindow(0)
{
    _circularSamplesBuffer.resize(numSamples);
}

DownloadRateSampler::~DownloadRateSampler()
{

}

qint64 DownloadRateSampler::now()
{
    return GetTick();
}

void DownloadRateSampler::clear()
{
    // Clear all the samples
    for (int i = 0; i < _circularSamplesBuffer.size(); i++)
	{
        _circularSamplesBuffer[i].bytes = 0;
        _circularSamplesBuffer[i].timestamp = 0;
    }

    // Reset the stats
    _oldestTimestamp = std::numeric_limits<qint64>::max();
    _totalBytesInWindow = 0;
}

void DownloadRateSampler::trimSamples(qint64 maxSampleAge, qint64 asOfTickMs)
{
    // Calculate the oldest sample we will retain
    qint64 oldestAllowableTimestamp = asOfTickMs - maxSampleAge;

    if (maxSampleAge < 0)
        return;
    if (oldestAllowableTimestamp < 0)
        return;

    // Calculate the stats again as we clear old samples
    _oldestTimestamp = std::numeric_limits<qint64>::max();
    _totalBytesInWindow = 0;

    // Clear old samples
    for (int i = 0; i < _circularSamplesBuffer.size(); i++)
	{
        Sample& sample = _circularSamplesBuffer[i];

        // If this sample is too old, erase it
        if (_circularSamplesBuffer[i].timestamp < oldestAllowableTimestamp)
        {
            _circularSamplesBuffer[i].bytes = 0;
            _circularSamplesBuffer[i].timestamp = 0;
        }

        if (sample.timestamp == 0)
            continue;

        // See if this is the oldest sample
        if (sample.timestamp < _oldestTimestamp)
            _oldestTimestamp = sample.timestamp;

        _totalBytesInWindow += sample.bytes;
    }
}

void DownloadRateSampler::addSample(qint64 bytes, qint64 timestampMs)
{
    // Update circular buffer
    Sample newSample;
    newSample.bytes = bytes;
    newSample.timestamp = timestampMs;

    _circularSamplesBuffer[_circularSamplesBufferIndex] = newSample;
    _circularSamplesBufferIndex++;
    _circularSamplesBufferIndex %= _circularSamplesBuffer.size();  // wrap around

    // Calculate the stats again
    _oldestTimestamp = std::numeric_limits<qint64>::max();
    _totalBytesInWindow = 0;

    // Find the total size and oldest timestamp
    for (int i = 0; i < _circularSamplesBuffer.size(); i++)
	{
        Sample sample = _circularSamplesBuffer[i];
        if (sample.timestamp == 0)
            continue;

        // See if this is the oldest sample
        if (sample.timestamp < _oldestTimestamp)
            _oldestTimestamp = sample.timestamp;

        _totalBytesInWindow += sample.bytes;
	}
}

quint64 DownloadRateSampler::averageBPS(qint64 asOfTickMs) const
{
	return theoreticalNewBPS(0, asOfTickMs);
}

quint64 DownloadRateSampler::theoreticalNewBPS(qint64 addedBytes, qint64 asOfTickMs) const
{
    quint64 bps = 0;
    qint64 bytesTotal = addedBytes + _totalBytesInWindow;
	if (_circularSamplesBuffer.size() > 0)
	{
        if (_oldestTimestamp == std::numeric_limits<qint64>::max())
        {
            // No samples, therefore no throughput
            return 0;
        }

        qint64 elapsedMs = asOfTickMs - _oldestTimestamp;
        if (elapsedMs <= 0)
        {
            // This should not happen, some time must have elapsed, but if none has, we'll just say 0 bps
            return 0;
        }

        double elapsedSeconds = elapsedMs / 1000.0f;
        bps = static_cast<quint64>(bytesTotal / (elapsedSeconds));
	}

	return bps;
}

quint64 DownloadRateSampler::estimatedCompletion(qint64 bytesRemaining, qint64 asOfTickMs) const
{
    quint64 bps = averageBPS(asOfTickMs);
    if (bps == 0)
        return 0;

    quint64 secsRemaining = bytesRemaining / bps;

    return secsRemaining;
}

typedef MovingAverage<double> BpsMovingAverage;

class TransferStats::Private
{
public:
	BpsMovingAverage bps_;		//!< Transfer rate in bytes per second, using MovingAverage to smooth the changes
	int bps_size_;				//!< History size
	double bps_avg_;			//!< History average

	qint64	total_received_;			//!< Total read and write size
	qint64	received_this_session_;		//!< Total read this session
	qint64	total_size_;				//!< Total expected size (may be 0 if unknown in advance)

	qint64	current_disc_received_;			//!< current disc read and write size
	qint64	current_disc_size_;				//!< current disc expected size (may be 0 if unknown in advance)

	qint64	time_start_;				 //!< Total when this session started
	qint64	time_finished_;				 //!< Total when this session finished (or -1)
	qint64	time_of_last_update_;		 //!< Time elapsed since last update
	qint64	received_since_last_update_; //!< Bytes received since last update

	Private()
	{
		reset();
	}

	~Private()
	{

	}

	void reset()
	{
		bps_size_ = 8;
		bps_avg_ = 0.0;
		bps_.SetHistorySize(0);
		bps_.SetHistorySize(bps_size_);
		received_this_session_ = total_received_ = total_size_ = current_disc_received_ = current_disc_size_ = 0;
		time_finished_ = time_start_ = time_of_last_update_ = received_since_last_update_ = 0;
	}
};

// ------------------------------------------------------------------
TransferStats::TransferStats(const QString& transferType, const QString& logId) :
	mTransferType(transferType), mLogId(logId), mLoggedProgress(0.0)
{
	d = new Private;
}

// ------------------------------------------------------------------
TransferStats::~TransferStats()
{
	delete d;
}

void TransferStats::setLogId(const QString& id)
{
	mLogId = id;
}

// ------------------------------------------------------------------
double TransferStats::bps() const
{ 
	double result = d->bps_avg_;
	return result;
}

// ------------------------------------------------------------------
double TransferStats::historicBps() const
{
	double bps = 0.0;
	qint64 session_time = elapsed();


	if ( session_time > 0 ) 
	{
		bps = (1000.0 * d->received_this_session_) / session_time;
	}

	return bps;
}

// ------------------------------------------------------------------
QString TransferStats::bpsString() const 
{ 
	return speedString( (quint32) (bps() + 0.5) );
}

// ------------------------------------------------------------------
qint64 TransferStats::elapsed() const
{
	qint64 result = 0;

	if ( d->time_start_ > 0 )
	{
		qint64 end = d->time_finished_ > 0 ? d->time_finished_ : GetTick();

		result = end - d->time_start_;
	}

	return result;
}

qint64 TransferStats::getLastUpdateTick() const  
{ 
	if (!d) return 0;
	return d->time_of_last_update_; 
}

// ------------------------------------------------------------------
QString TransferStats::elapsedString() const
{
	return timeIntervalString( elapsed() / 1000 );
}

// ------------------------------------------------------------------
qint64 TransferStats::totalReceived() const 
{ 
	qint64	result = d->total_received_;
	return result;
}

// ------------------------------------------------------------------
QString TransferStats::totalReceivedString() const 
{
	return sizeString( totalReceived() );
}

// ------------------------------------------------------------------
qint64 TransferStats::sessionReceived() const
{
	qint64	result = d->received_this_session_;
	return result;
}

// ------------------------------------------------------------------
QString TransferStats::sessionReceivedString() const
{
	return sizeString( sessionReceived() );
}

// ------------------------------------------------------------------
qint64 TransferStats::totalSize() const 
{ 
	qint64	result = d->total_size_;
	return result;
}

// ------------------------------------------------------------------
QString TransferStats::totalSizeString() const 
{
	return sizeString( totalSize() );
}

// ------------------------------------------------------------------
qint64 TransferStats::currentDiscSize() const 
{ 
	qint64	result = d->current_disc_size_;
	return result;
}

// ------------------------------------------------------------------
qint64 TransferStats::estimatedTimeToCompletion(bool useHistoricBps) const
{
	//Insufficient data
	if ( d->total_size_ <= 0 || d->bps_avg_ == 0. )
		return -1;

	//Finished
	if ( d->total_received_ >= d->total_size_ )
		return 0;

	//Get the remaining 
	qint64 rem = d->total_size_ - d->total_received_;

	double bps = useHistoricBps ? historicBps() :  d->bps_avg_;

	if ( bps < 0.1 )
		return -1;

	return (qint64) ((rem / bps) + 0.5);
}

// ------------------------------------------------------------------
qint64 TransferStats::estimatedTimeToCompletionDisc(bool useHistoricBps) const
{
	//Insufficient data
	if ( d->current_disc_size_ <= 0 || d->bps_avg_ == 0. )
		return -1;

	//Finished
	if ( d->current_disc_received_ >= d->current_disc_size_ )
		return 0;

	//Get the remaining 
	qint64 rem = d->current_disc_size_ - d->current_disc_received_;

	double bps = useHistoricBps ? historicBps() :  d->bps_avg_;

	if ( bps < 0.1 )
		return -1;

	return (qint64) ((rem / bps) + 0.5);
}
// ------------------------------------------------------------------
void TransferStats::onStart(qint64 totalSize, qint64 totalPreviouslyDownloaded)
{
	double dlSeed = bps();

	setHistorySize(DEFAULT_MAX_DL_HISTORY);
	setTotalSize(totalSize, totalPreviouslyDownloaded);
	resetStats();

	if(dlSeed > 0.0) 
	{
		seedBps(dlSeed);
	}
}

// ------------------------------------------------------------------
void TransferStats::onDataReceived( quint32 bytes )
{
	d->current_disc_received_ += bytes;
	d->total_received_ += bytes;
	d->received_since_last_update_ += bytes;
	d->received_this_session_ += bytes;

	if ( GetTick() - d->time_of_last_update_ >= UPDATE_INTERVAL )
	{
		updateStats();

#if _DEBUG
		if(d->total_size_ > 0)
		{
			// Log roughly every 5% up to 90%
			double currentProgress = ((double)d->total_received_ / (double)d->total_size_);
			if(currentProgress < .9 && ((currentProgress - mLoggedProgress) > .05))
			{
				mLoggedProgress = currentProgress;
				logStats();
			}
		}
#endif

	}
}

void TransferStats::logStats(bool complete)
{
	qint64 nBytesThisSession = sessionReceived();
	qint64 nTimeThisSession = elapsed();
	if (nTimeThisSession > 0)
	{
		qint64 nSpeedThisSession = (nBytesThisSession * 1000) / nTimeThisSession;
		QString logProgress = "100";
		QString eta = "";

#ifdef _DEBUG
		if(!complete)
		{
			logProgress = QString::number((int) floor(mLoggedProgress*100));
			eta = "  |  ETA: " + timeIntervalString(estimatedTimeToCompletion(true), true);
		}
#endif

		ORIGIN_LOG_EVENT << "[" << mLogId << "] " << mTransferType << " " << logProgress << "% Complete  |  Session Bytes: " << sizeString(nBytesThisSession) << "  |  Session Time: " << timeIntervalString(nTimeThisSession/1000, true) << "  |  Session Speed: " << speedString(nSpeedThisSession) << eta;
	}
}

// ------------------------------------------------------------------
void TransferStats::onFinished()
{
	if ( d->received_since_last_update_ > 0 )
	{
		updateStats();
	}

	d->time_finished_ = GetTick();
	logStats(true);
}

// ------------------------------------------------------------------
void TransferStats::updateStats()
{
	long long now = GetTick();

	if ( now == d->time_of_last_update_ )
		return;

	//Calculate this bps
	double bps = (1000.0 * d->received_since_last_update_) / (now - d->time_of_last_update_);

	//Add to the average
	d->bps_.Add(bps);
	d->bps_avg_ = d->bps_.GetAverage();

	d->received_since_last_update_ = 0;
	d->time_of_last_update_ = now;
}

// ------------------------------------------------------------------
void TransferStats::resetStats()
{
	long long now = GetTick();

	d->time_start_ = now;
	d->time_finished_ = 0;
	d->received_since_last_update_ = 0;
	d->received_this_session_ = 0;
	d->time_of_last_update_ = now;

	d->bps_.SetHistorySize(0);
	d->bps_.SetHistorySize(d->bps_size_);
	d->bps_avg_ = 0;
}

// ------------------------------------------------------------------
void TransferStats::setTotalSize( qint64 size, qint64 completed )
{
	d->total_size_ = size;
	d->total_received_ = completed;
	Q_ASSERT(d->total_size_==0 || d->total_size_ >= d->total_received_);
}

// ------------------------------------------------------------------
void TransferStats::setCurrentDiscSize( qint64 size, qint64 completed )
{
	d->current_disc_size_ = size;
	d->current_disc_received_ = completed;
	Q_ASSERT(d->current_disc_size_==0 || d->current_disc_size_ >= d->current_disc_received_);
}
// ------------------------------------------------------------------
void TransferStats::setHistorySize( int size )
{
	if  ( d->bps_size_ != size )
	{
		d->bps_size_ = size;
		d->bps_.SetHistorySize(d->bps_size_);
		d->bps_avg_ = d->bps_.GetAverage();
	}
}

// ------------------------------------------------------------------
void TransferStats::seedBps( double bps )
{
	//Add to the average
	d->bps_.Add(bps);
	d->bps_avg_ = d->bps_.GetAverage();
}

// ------------------------------------------------------------------
QString TransferStats::sizeString( qint64 length )
{
	QString result;

	if ( length > (1 * 1024 * 1024) )
	{
		result = QString("%1 MB").arg(QString::number(length/(1024.f*1024.f), 'f', 2));
	}
	else if ( length > 1024 )
	{
		result = QString("%1 KB").arg(QString::number(length/1024.f, 'f', 2));
	}
	else if ( length > 0 )
	{
		result = QString("%1 Bytes").arg(QString::number(length));
	}

	return ( result );
}

// ------------------------------------------------------------------
QString TransferStats::speedString( long bps )
{
	QString result = sizeString( bps );

	if ( result.isEmpty() )
	{
		result = "0 Bytes/sec";
	}
	else
	{
		result.append("/sec");
	}

	return ( result );
}

// ------------------------------------------------------------------
QString TransferStats::timeIntervalString( qint64 seconds, bool short_format )
{
	QString result;

	if ( seconds < 0 )
		return QString("unknown");

	quint32 hours = (quint32) (seconds / 3600);
	quint32 minutes = (quint32) ((seconds / 60) % 60);
	quint32 secs = (quint32) (seconds % 60);

	if ( short_format )
	{
		if ( hours > 0 )
		{
			result = QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
		}
		else
		{
			result = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
		}
	}
	else
	{
		if ( hours > 0 )
		{
			result = QString("%1 ").arg(hours);
			result.append(hours > 1 ? "hours" : "hour");
			result.append(" ");
		}

		if ( hours > 0 || minutes > 0 )
		{
			result.append(QString("%1 min ").arg(minutes));
		}

		result.append(QString("%1 sec").arg(secs));
	}

	return ( result );
}

}

}
