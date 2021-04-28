//
// Copyright 2008, Electronic Arts Inc
//
#ifndef FILECHANNEL_H
#define FILECHANNEL_H

#include "ChannelInterface.h"
#include "services/downloader/DownloaderErrors.h"

#include <QFile>
#include <QRunnable>
#include <QThreadPool>
#include <QThreadStorage>
#include <QMutex>

namespace Origin
{
namespace Downloader
{

/**
Channel interface for local file system
*/
class FileChannel : public ChannelInterface
{
public:

    typedef QMap<QFile::FileError, DownloadError::type> QFileErrorMap;
    static QFileErrorMap mQFileErrorTranslator;
    static void initQFileErrorTranslator();
    static DownloadError::type translateQFileError(QFile::FileError err);

	FileChannel();
	virtual	~FileChannel();

	/**
	Override: Channel type
	*/
	virtual QString GetType() const;

	/**
	Connect with the "server"
	*/
	bool Connect();

	/**
	Check if the connection established
	*/
	virtual bool IsConnected() const;

	/**
	Override: Request the given resource.
	If the request was sent and the response received, it returns true even if 
	the server response is an error.
	Server responses are the callers responsibility.
	You can still read data the same as with a 200 OK response.
	*/
	virtual bool SendRequest(const QString& verb, const QString& path, 
		const QString& addInfo = QString(), 
		const QByteArray& data = QByteArray(),
		const unsigned long timeout = CHANNEL_DEFAULT_TIMEOUT);
	/**
	Override: Query the amount of available data ready to be read
	*/		
	virtual bool QueryDataAvailable(ulong* avail);

	/**
	Override: Read data from the channel
	*/		
	bool ReadData(void* lpBuffer, qint64 iNumberOfBytesToRead, qint64* lpiNumberOfBytesRead,
		volatile bool& stop, ulong ulTimeoutMs /* = 0 for infinite */);

	
	/**
	Override: Close the ongoing request
	*/
	void CloseRequest(bool forceCanceled = false);

	/**
	Override: Close the active connection
	*/
	void CloseConnection();

	/*
	Set asynchronous mode
	*/
	bool SetAsyncMode(bool bAsyncMode) { mbAsyncMode = bAsyncMode; return true; }

	/*
	Is busy transferring an asynchronous request?
	*/
	virtual bool IsTransferring() { return (mAsyncWorker.isProcessing()); }

	/*
	Request response
	*/
	bool RequestComplete() { return false;  /* not supported */ }

	void run();

protected:

private:
	bool OpenFile();
	void FillResponseHeaders();

	qint64 Tell();
	qint64 Size();
	bool GetRange(qint64& start, qint64& end);

private:
	static QThreadStorage<QThreadPool*> sIoThreadPool;
	static QThreadPool* GetIoThreadPool();
	static QMutex sIoThreadMutex;
	
	class AsyncWorker : public QRunnable
	{
		public:
			AsyncWorker(FileChannel& parent);
			~AsyncWorker();
			void reset();
			void run();
			void abort();
			bool isProcessing();

		private:
			FileChannel& mParent;
			bool         mAbort;
			bool         mProcessing;
	};

    ChannelInfo mCachedResponseHeaders;
    qint64      mFileSize;

	QString		mszPath;
	QFile		mhFile;
	QString		mszMethod;
	bool		mbAsyncMode;
	AsyncWorker mAsyncWorker;
};

} // namespace Downloader
} // namespace Origin

#endif
