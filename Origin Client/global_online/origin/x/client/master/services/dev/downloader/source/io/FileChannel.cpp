//
// Copyright 2008, Electronic Arts Inc
//
#include <limits>
#include <QDateTime>
#include <QFileInfo>
#include <QMetaEnum>
#include <QThreadPool>

#include "io/FileChannel.h"
#include "services/downloader/FilePath.h"

#include "services/downloader/Common.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "TelemetryAPIDLL.h"

#define DEFAULT_ASYNC_XFER_BUFFER_SIZE		32768

namespace Origin
{
namespace Downloader
{

// #define FILECHANNEL_UNIT_TEST

#ifdef FILECHANNEL_UNIT_TEST

bool FileChannelUnitTest()
{
	FileChannel channel;

	channel.SetURL("file:///C:/eula.1031.txt");

	if ( channel.Connect() )
	{
		QString sRequest = QString("bytes=%1-%2").arg(0, 15);
		channel.AddRequestInfo("Range", sRequest);

		if ( channel.SendRequest("GET", QString()) )
		{
			DWORD avail = 0;
			while ( channel.QueryDataAvailable(&avail) && avail )
			{
				char* buffer = new char[avail+1];
				qint64 read = 0;
				bool stop = false;
				if ( !channel.ReadData(buffer, avail, &read, stop, 0) )
					break;

				buffer[read] = '\0';
				::OutputDebugStringA(buffer);
				delete [] buffer;
			}
		}
		else
		{
			QString szErr = QString("Send request error. %d").arg(channel.GetLastErrorType());
			OutputDebugString(szErr.utf16());
		}
	}
	else
	{
		QString szErr = QString("Connect error. %d").arg(channel.GetLastErrorType());
		OutputDebugString(szErr.utf16());
	}

	return channel.GetLastErrorType() == DownloadError::OK;
}

static bool testResult = FileChannelUnitTest();

#endif

#define LOCAL_MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define LOCAL_MAX(a,b)            (((a) > (b)) ? (a) : (b))

#ifndef _count_array_members
#define _count_array_members(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

//RFC1123 date format specifier
#define RFC1123_FORMAT L"%a, %d %b %Y %H:%M:%S GMT"

typedef struct
{
	const char* ext;
	const char* ctype;

} NameToContentType_t;

static const NameToContentType_t Name_To_ContentType[] = {
	{"gif", "image/gif"},
	{"jpeg", "image/jpeg"},
	{"jpg", "image/jpeg"},
	{"tiff", "image/tiff"},
	{"wav", "audio/x-wav"},
	{"mpg", "video/mpeg"},
	{"mpeg", "video/mpeg"},
	{"mpe", "video/mpeg"},
	{"mov", "video/quicktime"},
	{"avi", "video/x-msvideo"},
	{"zip", "application/x-zip-compressed"},
	{"gz", "application/x-gzip"},
	{"html", "text/html"},
	{"htm", "text/html"},
	{"txt", "text/plain"},
	{"pdf", "application/pdf"},
	{"rtf", "application/rtf"}
};

static QString GetContentType(const QString& extension)
{
	for ( size_t i = 0; i < _count_array_members(Name_To_ContentType); i++ )
	{
		if (extension.compare(Name_To_ContentType[i].ext, Qt::CaseInsensitive) == 0)
		{
			return Name_To_ContentType[i].ctype;
		}
	}

	return QString(QObject::tr("application/octet-stream"));
}

FileChannel::QFileErrorMap FileChannel::mQFileErrorTranslator;

void FileChannel::initQFileErrorTranslator()
{
	mQFileErrorTranslator.insert(QFile::NoError, DownloadError::OK);
	mQFileErrorTranslator.insert(QFile::ReadError, DownloadError::FileRead);
	mQFileErrorTranslator.insert(QFile::WriteError, DownloadError::FileWrite);
	//map.insert(QFile::FatalError, DownloadError::);
	//map.insert(QFile::ResourceError, DownloadError::);
	mQFileErrorTranslator.insert(QFile::OpenError, DownloadError::FileOpen);
	//map.insert(QFile::AbortError, DownloadError::);
	mQFileErrorTranslator.insert(QFile::TimeOutError, DownloadError::TimeOut);
	//map.insert(QFile::UnspecifiedError, DownloadError::);
	//map.insert(QFile::RemoveError, DownloadError::);
	//map.insert(QFile::RenameError, DownloadError::);
	//map.insert(QFile::PositionError, DownloadError::FileSeek);
	//map.insert(QFile::ResizeError, DownloadError::);
	mQFileErrorTranslator.insert(QFile::PermissionsError, DownloadError::FilePermission);
	//map.insert(QFile::CopyError, DownloadError::);
}


DownloadError::type FileChannel::translateQFileError(QFile::FileError err)
{
    if (mQFileErrorTranslator.isEmpty())
    {
        initQFileErrorTranslator();
    }

    // TELEMETRY:  Need to get QFile error
    if (err != QFile::NoError)
    {
        QString errorString = QString("QFileError=%1").arg(err);
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelQFileError", errorString.toUtf8().data());
    }

	DownloadError::type error = DownloadError::UNKNOWN;
	QFileErrorMap::iterator i = mQFileErrorTranslator.find(err);
	if (i != mQFileErrorTranslator.constEnd())
	{
		error = i.value();
	}

	return error;
}

// Thread local thread pools mean that each download session will have a threadpool with 2 threads in it.  
QThreadStorage<QThreadPool*> FileChannel::sIoThreadPool;
QMutex FileChannel::sIoThreadMutex;

QThreadPool* FileChannel::GetIoThreadPool()
{
	QMutexLocker lock(&sIoThreadMutex);

	if (sIoThreadPool.hasLocalData())
	{
		return sIoThreadPool.localData();
	}
	else
	{
		QThreadPool* newThreadPool = new QThreadPool();
		newThreadPool->setMaxThreadCount(2);
		sIoThreadPool.setLocalData(newThreadPool);
		return newThreadPool;
	}
}

FileChannel::FileChannel() : 
	mhFile(NULL)
	, mbAsyncMode(false)
	, mAsyncWorker(*this)
{

}

FileChannel::~FileChannel()
{
	CloseConnection();
}

QString FileChannel::GetType() const
{
	return QString("file");
}

bool FileChannel::Connect()
{
	//Close previous connection
	CloseConnection();

	const QUrl& url = GetURL();

	if(url.scheme().compare("file") == 0)
	{
		mszPath = url.toLocalFile();

		if (!mszPath.isEmpty())
		{
			// Open the file
			SetLastErrorType(DownloadError::OK);
			SetErrorCode(0);
			//SetResponseReason(QByteArray());
			mResponseInfo.clear();

			if (!OpenFile())
			{
				CloseRequest();
                SetLastErrorType(DownloadError::FileOpen);
			}
			else
			{
				onConnected();
			}
		}
	}

	return IsConnected();
}

bool FileChannel::IsConnected() const
{
	return (mhFile.isReadable());
}

bool FileChannel::SendRequest(const QString& verb,
							   const QString& path,
							   const QString& addInfo,
							   const QByteArray& data,
							   const unsigned long timeout)
{
	int asyncWorkerWaitTotal = 10000;
	int asyncWorkerWaitSleep = 1000;
	int totalWaited = 0;

	// Not still processing some request are we?  Give it some time to complete
	while(mAsyncWorker.isProcessing())
	{
		if(totalWaited >= asyncWorkerWaitTotal)
		{
            // TELEMETRY:  Thread exit prematurely?
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelSendRequest", "ThreadExit");

			ORIGIN_LOG_ERROR << "FileChannel::SendRequest() - error thread currently exiting";
			return false;
		}

		Origin::Services::PlatformService::sleep(asyncWorkerWaitSleep);
		totalWaited += asyncWorkerWaitSleep;
	}

	//Clear state
	SetErrorCode(0);
	mResponseInfo.clear();

	//Reset error
	SetLastErrorType(DownloadError::OK);

	if (!IsConnected())
	{
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelSendRequest", "NotConnected");

		SetLastErrorType(DownloadError::FileRead);
		ORIGIN_LOG_ERROR << "FileChannel::SendRequest() - error not connected";
		return false;
	}

	if (!path.isEmpty())
	{
        if (!path.startsWith('/'))
        {
            QString tmp = "/" + path;
            mUrl.setPath(tmp);
        }
        
        else
            mUrl.setPath(path);
	}

	QString mszMethod = verb;
	if (mszMethod.isEmpty())
	{
		//Default is GET
		mszMethod = QString("GET");
	}

	QString getStr("GET");
	QString headStr("HEAD");
	if ( mszMethod.compare(getStr, Qt::CaseInsensitive) != 0 &&
		 mszMethod.compare(headStr, Qt::CaseInsensitive) != 0 )
	{
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelSendRequest", "BadRequest");

		//Bad request
        SetLastErrorType(DownloadError::InvalidDownloadRequest);
		CloseRequest();
		return true;
	}

	//Now check if the range is satisfiable
	qint64 start = 0, end = 0;
	QString rangeStr("Range");
	if (!GetRequestInfo(rangeStr).isEmpty())
	{
		qint64 size = Size();
		bool valid = GetRange(start, end) && start < size && end <= size && end >= start && mhFile.seek(start);

		if (!valid)
		{
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelSendRequest", "InvalidRange");

			//Requested range not satisfiable
			SetLastErrorType(DownloadError::RangeRequestInvalid);

			CloseRequest();
			return false;
		}
	}

	//TODO: we could also handle If-Modified-Since header here

	//ulong code = HTTP_RESPONSE_OK;
	if (GetRange(start, end))
	{
		//code = HTTP_RESPONSE_PARTIAL_CONTENT;
	}
	
	//SetResponseCode(code);
	//SetResponseReason(HttpResponseCode::getResponseDescription(code));

	//Fill response headers
	FillResponseHeaders();

	if (GetLastErrorType() != DownloadError::OK)
	{
        // TELEMETRY:  Need to get QFile error
        QString errorString = QString("ResponseHeader=%1").arg(GetLastErrorType());
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelSendRequest", errorString.toUtf8().data());

		ORIGIN_LOG_ERROR << "FileChannel::SendRequest() - error code " << GetLastErrorType();
		return false;
	}

	// Exit if synchronous mode xfer
	if (!mbAsyncMode)
		return true;

	// Start async thread
	mAsyncWorker.reset();
	GetIoThreadPool()->start(&mAsyncWorker, QThread::LowPriority);

	return true;
}

bool FileChannel::QueryDataAvailable(ulong* avail)
{
	if (avail)
		*avail = 0;

	if (!mhFile.exists())
	{
        // TELEMETRY:  What file does not exist?
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelQueryDataAvailable", mhFile.fileName().toUtf8().data());

		return false;
	}

	//For head, no content is returned
	QString headStr("HEAD");
	if ( mszMethod.compare(headStr, Qt::CaseInsensitive) == 0)
		return true;

	qint64 pos = Tell();
	qint64 size = Size();
	qint64 start = 0, end = 0;

	if (!GetRange(start, end))
	{
		start = 0;
		end = size;
	}

	start = LOCAL_MAX(start, pos);
	end = LOCAL_MIN(end, size);

	if (end > start + 8 * 1024)
	{
		end = start + 8 * 1024;
	}

	if (avail)
		*avail = static_cast<ulong>(end - start);

	return true;
}

bool FileChannel::ReadData(void* lpBuffer, qint64 iNumberOfBytesToRead, qint64* lpiNumberOfBytesRead,
							  volatile bool& stop, unsigned long ulTimeoutMs )
{
	if (lpiNumberOfBytesRead)
		*lpiNumberOfBytesRead = 0;

	if (!mhFile.exists())
    {
        // TELEMETRY:  What file does not exist?
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelReadDataNotExist", mhFile.fileName().toUtf8().data());
		return false;
    }

	qint64 nRead;
	ulong availableBytes;
	bool result;

	do
	{
		availableBytes = 0;

		result = QueryDataAvailable(&availableBytes);

		if (result) // Check for data
		{
			//Limit to 8k chunks so we can report activity
			if ( availableBytes > 8 * 1024 )
			{
				availableBytes = 8 * 1024;
			}

			//Limit to required
			if ( availableBytes > iNumberOfBytesToRead )
			{
				availableBytes = static_cast<ulong>(iNumberOfBytesToRead);
			}

			nRead = mhFile.read(reinterpret_cast<char*>(lpBuffer), static_cast<qint64>(availableBytes));
			result = nRead != -1;

			if ( nRead > 0 )
			{
				if (lpiNumberOfBytesRead)
					*lpiNumberOfBytesRead += nRead;

				lpBuffer = (reinterpret_cast<char*>(lpBuffer)) + nRead;
				iNumberOfBytesToRead -= nRead;

				onDataReceived(static_cast<ulong>(nRead));
			}
		}

	} while ( !stop && result && availableBytes > 0 && iNumberOfBytesToRead > 0 );
	
	SetLastErrorType(translateQFileError(mhFile.error()));

	return !stop && result && iNumberOfBytesToRead == 0;
}

void FileChannel::CloseRequest(bool forceClosed)
{
	if(mAsyncWorker.isProcessing())
	{
		mAsyncWorker.abort();
	}
	else
	{
		mszMethod.clear();
		mhFile.close();
		mhFile.setFileName(QString());
	}
}

void FileChannel::CloseConnection()
{
	CloseRequest();
}

bool FileChannel::OpenFile()
{
	//ORIGIN_LOG_DEBUG << "Trying to open file [" << mszPath << "]";

	mhFile.setFileName(mszPath);

	if(!mhFile.open(QIODevice::ReadOnly) || !mhFile.isReadable())
	{
        // TELEMETRY:  File open failure
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("FileChannelOpenFile", mszPath.toUtf8().data());

		SetLastErrorType(translateQFileError(mhFile.error()));
	}

    if(mhFile.isReadable())
    { 
        mFileSize = mhFile.size();

        mCachedResponseHeaders.clear();

	    //Date
        mCachedResponseHeaders["Date"] = (QDateTime::currentDateTime().toUTC().toString("ddd, d MMM yyyy hh:mm:ss") + " GMT");

	    //Last modified
	    //BY_HANDLE_FILE_INFORMATION fi;
	    //memset(&fi, 0, sizeof(fi));
	    QFileInfo fileInfo(mhFile);
	    if (fileInfo.exists())
	    {
		    QDateTime lastModDateTime = fileInfo.lastModified().toUTC();
		    QString lastModifiedKey("Last Modified");
		    QString lastModifiedValue = lastModDateTime.toString("ddd, d MMM yyyy hh:mm:ss");
		    lastModifiedValue.append(" GMT");
		    mCachedResponseHeaders[lastModifiedKey] = lastModifiedValue;

		    QString contentTypeKey(QString("Content-Type"));
		    QString contentTypeValue = GetContentType(fileInfo.suffix());
		    mCachedResponseHeaders[contentTypeKey] = contentTypeValue;
	    }
    }

	return mhFile.isReadable();
}

void FileChannel::FillResponseHeaders()
{
	/*
	HTTP/1.1 206 Partial content
	Date: Wed, 15 Nov 1995 06:25:24 GMT
	Last-Modified: Wed, 15 Nov 1995 04:58:08 GMT
	Content-Range: bytes 21010-47021/47022
	Content-Length: 26012
	Content-Type: image/gif
	*/
    foreach(const QString& key, mCachedResponseHeaders.keys())
    {
        AddResponseInfo(key, mCachedResponseHeaders[key]);
    }

	qint64 start = 0, end = 0, size = 0;
	size = Size();

	QString contentRangeKey(QString("Content-Range"));
	QString contentLengthKey(QString("Content-Length"));

	//Range if indicated
	if  (GetRange(start, end))
	{
		QString contentRangeValue = QString("%1-%2/%3").arg(QString::number(start)).arg(
			QString::number(end-1)).arg(QString::number(size));
		AddResponseInfo(contentRangeKey, contentRangeValue);

		//Prepare for content length
		size = end - start;
	}

	//Content length
	QString contentLengthValue = QString::number(size);
	AddResponseInfo(contentLengthKey, contentLengthValue);
}

qint64 FileChannel::Tell()
{
	qint64 position = 0;

	if (mhFile.exists())
	{
		position = mhFile.pos();
	}

	return position;
}

qint64 FileChannel::Size()
{
    return mFileSize;
}

bool FileChannel::GetRange(qint64& start, qint64& end)
{
	bool result = false;
	QString rangeStr("Range");
	QString szRange = GetRequestInfo(rangeStr);

	if (!szRange.isEmpty())
	{
		//We only accept bytes= specifier
		if (szRange.left(6) == "bytes=")
		{
			szRange = szRange.mid(6);
			int pos = szRange.indexOf(QChar('-'));
			if (pos >= 0)
			{
				QString szStart = szRange.left(pos);
				QString szEnd = szRange.mid(pos+1);

				szStart = szStart.trimmed();
				szEnd = szEnd.trimmed();

				//Support "bytes=-500" and "bytes=9500-" formats
				if ( szStart.isEmpty() )
				{
					start = 0;
				}
				else
				{
					start = szStart.toLongLong();
				}

				if ( szEnd.isEmpty() )
				{
					end = Size();
				}
				else
				{
					end = szEnd.toLongLong();

					if ( end != 0 )
						end++; //Includes last byte by spec
				}

				result = true;
			}
		}
	}

	return result;
}

FileChannel::AsyncWorker::AsyncWorker(FileChannel& parent) :
	mParent(parent)
{
	setAutoDelete(false);
	reset();
}

FileChannel::AsyncWorker::~AsyncWorker()
{

}

void FileChannel::AsyncWorker::reset()
{
	mProcessing = false;
	mAbort = false;
}

void FileChannel::AsyncWorker::abort()
{
	mAbort = true;
}

bool FileChannel::AsyncWorker::isProcessing()
{
	return mProcessing;
}

void FileChannel::AsyncWorker::run()
{
	mProcessing = true;

	if (mParent.IsConnected())
	{
		// Determine range of file to return (Note: byte at iEnd is included)
		qint64 iStart = 0, iEnd = 0;
		mParent.GetRange(iStart, iEnd);

		// Push until stream is exhausted
		qint64 llCurOffset = iStart;
		while(!mAbort && mParent.IsConnected() && llCurOffset < iEnd)
		{
			ulong ulBytesToRead = LOCAL_MIN(DEFAULT_ASYNC_XFER_BUFFER_SIZE, (iEnd-llCurOffset));
			ORIGIN_ASSERT(ulBytesToRead);

			// Post a data ready notification to get a buffer (and possibly a new read size)
			void* pBuffer = NULL;
			mParent.onDataAvailable(&ulBytesToRead, &pBuffer);

			// Get a buffer of data
			if (pBuffer)
			{
				ulong dwBytesRead = 0;
				mParent.mhFile.seek(llCurOffset);
				dwBytesRead = mParent.mhFile.read(reinterpret_cast<char*>(pBuffer), ulBytesToRead);

				if (dwBytesRead == (ulong)-1)
				{
					mParent.SetLastErrorType(translateQFileError(mParent.mhFile.error()));
					break;
				}
				ORIGIN_ASSERT(dwBytesRead == ulBytesToRead);
				if (dwBytesRead != ulBytesToRead)
				{
					ORIGIN_LOG_ERROR << "File " << mParent.GetURL().path() << " stream ended prematurely at byte " << (llCurOffset + dwBytesRead) << " of expected " << ((iEnd-iStart)+1);
					mParent.SetLastErrorType(translateQFileError(mParent.mhFile.error()));
					break;
				}
			}
			llCurOffset += ulBytesToRead;

			// Let observers know the data was retrieved (or skipped if no buffer provided)
			mParent.onDataReceived(ulBytesToRead);

			// Take a breather between blocks
			if(llCurOffset < iEnd)
			{
				Origin::Services::PlatformService::sleep(0);
			}
		}
		if(mAbort)
		{
			mParent.mszMethod.clear();
			mParent.mhFile.close();
			mParent.mhFile.setFileName(QString());
		}
	}

	mProcessing = false;
}

} // namespace Downloader
} // namespace Origin

