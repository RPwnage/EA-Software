//
// Copyright 2008, Electronic Arts Inc
//
#ifndef TransferStats_H
#define TransferStats_H

#include <QString>
#include <QVector>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

// Provides a simple moving-window download rate calculation
class DownloadRateSampler
{
    struct Sample
    {
        Sample() : 
            bytes(0), 
            timestamp(0)
        {
            // Nothing to do here
        }
        qint64 bytes;
        qint64 timestamp;
    };
public:
    DownloadRateSampler(int numSamples = 10);
    ~DownloadRateSampler();

    static qint64 now();

    void    clear();
    void    trimSamples(qint64 maxSampleAge, qint64 asOfTickMs = DownloadRateSampler::now());
    void	addSample(qint64 bytes, qint64 timestampMs = DownloadRateSampler::now());
	quint64 averageBPS(qint64 asOfTickMs = DownloadRateSampler::now()) const;		// averate BPS of most recent n samples
    quint64 theoreticalNewBPS(qint64 addedBytes, qint64 asOfTickMs = DownloadRateSampler::now()) const;
    quint64 estimatedCompletion(qint64 bytesRemaining, qint64 asOfTickMs = DownloadRateSampler::now()) const;

private:
    int             _circularSamplesBufferIndex;
    QVector<Sample> _circularSamplesBuffer;
    qint64          _oldestTimestamp;
    qint64          _totalBytesInWindow;
};

/**
TransferStats manages statistics for a protocol session.
Simply set the expected download size with setTotalSize()
and call onStart() onDataReceived() and onFinished()
*/
class ORIGIN_PLUGIN_API TransferStats
{
public:
	TransferStats(const QString& transferType, const QString& logId);
	virtual ~TransferStats();

	/**
	Return the bytes per second.
	This value is dynamically calculated on every OnProgress() report
	*/
	double bps() const;

	/**
	Return the bytes per second of this session.
	This is not an instant value, but the session average
	*/
	double historicBps() const;

	/**
	Get the bps as a user friendly string, choosing the appropriate units according to the value
	NOTE: This is for internal (debug) use, no localization
	*/
	QString bpsString() const;

	/**
	Returns the time elapsed for this session in milliseconds
	*/
	qint64 elapsed() const;

	/**
	Return tick of last bps update
	*/
	qint64 getLastUpdateTick() const;

	/**
	Get the time elapsed since this session started as a user friendly string, 
	choosing the appropriate units according to the value
	NOTE: This is for internal (debug) use, no localization
	*/	
	QString elapsedString() const;

	/**
	Return the total transferred size up to now in bytes
	This value is the total received data
	*/
	qint64 totalReceived() const;
	
	/**
	Return the total transferred size as a user friendly string
	NOTE: This is for internal (debug) use, no localization
	*/
	QString totalReceivedString() const;

	/**
	Return the transferred size for this session
	*/
	qint64 sessionReceived() const;

	/**
	Return the transferred size for this session as a user friendly string
	*/
	QString sessionReceivedString() const;

	/**
	Return the total size of the resource being downloaded in bytes
	*/
	qint64 totalSize() const;

	/**
	Return the total size as a user friendly string
	NOTE: This is for internal (debug) use, no localization
	*/
	QString totalSizeString() const;

	/**
	Return the current disc size of the resource being downloaded in bytes
	*/
	qint64 currentDiscSize() const;


	/**
	Get the estimated time to completion in seconds.
	If the info is insufficient (no stats yet, total size is unknown) it will return -1.
	If finished, it will return 0
	if useHistoricBps is true, then historic bps instead of instant will be used in the calculation
	*/		
	qint64 estimatedTimeToCompletion(bool useHistoricBps = false) const;

	/**
	Get the estimated time to completion of the disc in seconds.
	If the info is insufficient (no stats yet, total size is unknown) it will return -1.
	If finished, it will return 0
	if useHistoricBps is true, then historic bps instead of instant will be used in the calculation
	*/		
	qint64 estimatedTimeToCompletionDisc(bool useHistoricBps = false) const;

	/**
	Return a representation of a time interval in seconds using 
	the appropriate units that better suit for readability
	NOTE: This is for internal (debug) use, no localization
	*/
	static QString timeIntervalString( qint64 seconds, bool short_format = false );

	/**
	Return a representation of a transfer speed in bps using 
	the appropriate units that better suit for readability
	NOTE: This is for internal (debug) use, no localization
	*/
	static QString speedString( long bps );

	/**
	Return a representation of a size in bytes using 
	the appropriate units that better suit for readability
	NOTE: This is for internal (debug) use, no localization
	*/
	static QString sizeString( qint64 bytes );

public:
	void setLogId(const QString& id);

	/**
	Call when the transmission starts
	*/
	void onStart(qint64 totalSize, qint64 totalPreviouslyDownloaded);

	/**
	Data received
	*/
	void onDataReceived( quint32 bytes );

	/**
	Call when finished
	*/
	void onFinished();

	/**
	This call resets the stats for this session. This can be useful when pausing
	*/
	void resetStats();

	/**
	Set the total estimated size, plus whats already completed
	*/
	void setTotalSize( qint64 size,  qint64 completed = 0 );

	/**
	Set the current disc estimated size, plus whats already completed
	*/
	void setCurrentDiscSize( qint64 size,  qint64 completed = 0 );
	
	/**
	Set historic samples size (moving average)
	*/
	void setHistorySize( int size );

	/**
	Seed the history bps with a sample
	*/
	void seedBps( double bps );

protected:
	/**
	Updates the Bps. This is done automatically on each onDataReceived() 
	if the time since the last update is greater than 1/2 second.
	The onFinished() call also updates.
	*/
	void updateStats();

	void logStats(bool complete = false);

private:
	QString mTransferType;
	QString mLogId;
	double mLoggedProgress; // For a little extra logging when in debug mode

	class Private;
	Private* d;
};

}

}

#endif
