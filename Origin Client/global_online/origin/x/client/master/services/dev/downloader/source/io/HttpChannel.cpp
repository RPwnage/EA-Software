//
// Copyright 2011, Electronic Arts Inc
//

#include <limits>
#include <QDateTime>
#include <QEventLoop>
#include <QMetaEnum>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QVariant>
#include <QMutexLocker>
#include <QFile>
#include <QTimer>

#include "io/HttpChannel.h"

#include "services/network/NetworkAccessManager.h"

#include "services/downloader/Timer.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/log/LogService.h"
#include "services/downloader/DownloadService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"

#include "TelemetryAPIDLL.h"

#ifdef SIMULATE_RANDOM_NETWORK_ERRORS
#include <time.h>
#endif

namespace Origin
{
namespace Downloader
{

HttpChannel::QNetworkErrorMap HttpChannel::sQNetworkErrorTranslator = HttpChannel::initQNetworkErrorTranslator();

HttpChannel::QNetworkErrorMap HttpChannel::initQNetworkErrorTranslator()
{
    HttpChannel::QNetworkErrorMap errorMap;

    errorMap.insert(QNetworkReply::NoError, DownloadError::OK);
    errorMap.insert(QNetworkReply::ConnectionRefusedError, DownloadError::NetworkConnectionRefused);
    errorMap.insert(QNetworkReply::RemoteHostClosedError, DownloadError::NetworkRemoteHostClosed);
    errorMap.insert(QNetworkReply::HostNotFoundError, DownloadError::NetworkHostNotFound);
    errorMap.insert(QNetworkReply::TimeoutError, DownloadError::TimeOut);
    errorMap.insert(QNetworkReply::OperationCanceledError, DownloadError::NetworkOperationCanceled);
    errorMap.insert(QNetworkReply::SslHandshakeFailedError, DownloadError::SslHandshakeFailed);
    errorMap.insert(QNetworkReply::TemporaryNetworkFailureError, DownloadError::TemporaryNetworkFailure);
    errorMap.insert(QNetworkReply::ProxyConnectionRefusedError, DownloadError::NetworkProxyConnectionRefused);
    errorMap.insert(QNetworkReply::ProxyConnectionClosedError, DownloadError::NetworkProxyConnectionClosed);
    errorMap.insert(QNetworkReply::ProxyNotFoundError, DownloadError::NetworkProxyNotFound);
    errorMap.insert(QNetworkReply::ProxyTimeoutError, DownloadError::TimeOut);
    errorMap.insert(QNetworkReply::ProxyAuthenticationRequiredError,  DownloadError::NetworkAuthenticationRequired);
    errorMap.insert(QNetworkReply::ContentAccessDenied, DownloadError::HttpError);
    errorMap.insert(QNetworkReply::ContentOperationNotPermittedError, DownloadError::HttpError);
    errorMap.insert(QNetworkReply::UnknownContentError, DownloadError::HttpError);
    errorMap.insert(QNetworkReply::ContentNotFoundError, DownloadError::HttpError);
    errorMap.insert(QNetworkReply::AuthenticationRequiredError, DownloadError::NetworkAuthenticationRequired);

    return errorMap;
}


DownloadError::type HttpChannel::translateQNetworkError(HttpChannel* channel, QNetworkReply::NetworkError err)
{
    // TELEMETRY:  Need to get QNetworkReply error
    // TODO: possibly move the telemetry to onErrorReceived if it gets too noisy
    if (err != QNetworkReply::NoError)
    {
		QString hostname;
		int http_error = 0;
		if (channel)
		{
			// get IP/hostname from mUrl
			hostname = channel->GetURL().host();

			if (channel->mNetworkReply)
			{
				http_error = channel->mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			}
		}

		QString errorString = QString("QNetworkError=%1 HttpError=%2 HostIP=%3").arg(err).arg(http_error).arg(hostname);
        GetTelemetryInterface()->Metric_ERROR_NOTIFY("HttpChannelQNetworkError", errorString.toUtf8().data());
    }

    DownloadError::type error = DownloadError::UNKNOWN;
    QNetworkErrorMap::const_iterator i = sQNetworkErrorTranslator.find(err);
    if (i != sQNetworkErrorTranslator.constEnd())
    {
        error = i.value();
    }

    return error;
}

HttpChannel::HttpChannel(Services::Session::SessionWRef session) :
    mSession(session),
    mNetworkReply(NULL),
    mbAsyncMode(false),
    mbConnected(false),
    mbRequestForcedClosed(false)
{
    mNetworkAccessManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();
}

HttpChannel::~HttpChannel()
{
    CloseConnection();
}

QString HttpChannel::GetType() const
{
    return QString("http");
}

bool HttpChannel::Connect()
{
    // Technically, we aren't connected to the server until the first get/put/post/head request is made, which
    // is the same as the WinHTTP code we are replacing here.  The original code set up some necessary WinHTTP
    // structures in this method, but did not connect to the actual server.

    // Since some of the download logic expects the value of IsConnected to be based on whether those WinHTTP
    // structures were initialized successfully, for QT we'll just validate the URL for the channel here and get out.
    mbConnected = GetURL().isValid();
    return IsConnected();
}

bool HttpChannel::IsConnected() const
{
    return mbConnected;
}

void HttpChannel::AddRequestInfo(const QString& key, const QString& value)
{
    QString val = GetRequestInfo(key);
    if (!val.isEmpty())
    {
        if (!value.isEmpty())
        {
            val.append(", ");
            val.append(value);
        }
    }
    else
    {
        val = value;
    }

    ChannelInterface::AddRequestInfo(key, val);
}

#ifdef SIMULATE_RANDOM_NETWORK_ERRORS
QMutex HttpChannel::sGenerateMutex;
EA::StdC::Random HttpChannel::sRandom;

static quint64 generateAttempts = 0;
static quint64 errorGenerated = 0;

void HttpChannel::generateRandomError()
{
    QMutexLocker lock(&sGenerateMutex);

    generateAttempts++;

    mSimulateErrorType = DownloadError::OK;
    mSimulateErrorCode = 0;

    if((sRandom.RandomUint32Uniform(100)) < PERCENT_CHANCE_OF_SIMULATED_ERROR)
    {
#ifdef SIMULATE_ONLY_RETRYABLE_ERRORS
        switch(sRandom.RandomUint32Uniform(4))
        {
            case 0:
                mSimulateErrorType = DownloadError::TimeOut;
                break;
            case 1:
                mSimulateErrorType = DownloadError::NetworkOperationCanceled;
                break;
            case 2:
                mSimulateErrorType = DownloadError::TemporaryNetworkFailure;
                break;
            case 3:
                mSimulateErrorType = DownloadError::NetworkProxyConnectionClosed;
                break;
            default:
                break;
        }
#else
        mSimulateErrorType = (DownloadError::type)((sRandom.RandomUint32Uniform(12)) + (int)DownloadError::NetworkConnectionRefused);
#endif

        if(mSimulateErrorType == DownloadError::HttpError)
        {
            // TODO: Generate a random response code
            mSimulateErrorCode = Origin::Services::HttpStatusCode::Http403ClientErrorForbidden;
        }
    }

    if(mSimulateErrorType != DownloadError::OK)
    {
        errorGenerated++;

        ORIGIN_LOG_DEBUG << "Rolled a random simulated error of type: " << ErrorTranslator::Translate((ContentDownloadError::type)mSimulateErrorType);
        ORIGIN_LOG_DEBUG << "Percent random error generation is: [" << errorGenerated << " / " << generateAttempts << "]: " << ((double)errorGenerated/(double)generateAttempts);
    }
}

void HttpChannel::raiseSimulatedError()
{
    SetLastErrorType(mSimulateErrorType);
    SetErrorCode(-1);
    onError(mSimulateErrorType);
}
#endif

bool HttpChannel::SendRequest( const QString& verb, const QString& path,
                                const QString& addInfo,
                                const QByteArray& data,
                                const unsigned long timeout)
{
    //Clear state
    SetErrorCode(0);
    //SetResponseReason(QByteArray());
    mResponseInfo.clear();

    //Close previous request
    CloseRequest();

    //Reset error
    SetLastErrorType(DownloadError::OK);

#ifdef SIMULATE_RANDOM_NETWORK_ERRORS

    generateRandomError();

    if(mSimulateErrorType == DownloadError::NotConnectedError || mSimulateErrorType == DownloadError::RangeRequestInvalid)
    {
        SetLastErrorType(mSimulateErrorType);
        return false;
    }
    else if(mSimulateErrorType != DownloadError::OK)
    {
        if(!mbAsyncMode)
        {
            SetLastErrorType(mSimulateErrorType);
            return false;
        }
        else
        {
            // In the timeout case, we simply return true and the download worker timeout logic will catch the timeout
            // All other cases will raise a onError to the download worker in 50 to 150 ms
            if(mSimulateErrorType != DownloadError::TimeOut)
            {
                QTimer::singleShot((sRandom.RandomUint32Uniform(100) + 50), this, SLOT(raiseSimulatedError()));
            }

            return true;
        }
    }
#endif

    //scope it so that strongRef only remains during this short period of time
    {
        Services::Session::SessionRef session(mSession.toStrongRef());
        ORIGIN_ASSERT(session);
        if (!session || !IsConnected() || !Services::Connection::ConnectionStatesService::isUserOnline(session))
        {
            SetLastErrorType(DownloadError::NotConnectedError);
            return false;
        }
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

    QString szVerb = verb;
    if (szVerb.isEmpty())
    {
        //Default is GET
        szVerb = QString("GET");
    }

    QNetworkRequest request(GetURL());
    QVariant alwaysNetwork(QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, alwaysNetwork);

    // Do not enable HTTP pipelining
    // Qt has problems associating requests with response bodies when we're
    // heavily pipelined
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, QVariant(false));

    SetFormattedRequestHeaders(request);

    if (QString::compare(szVerb, QString("GET"), Qt::CaseInsensitive) == 0)
    {
        mNetworkReply = mNetworkAccessManager->get(request);
    }
    else if (QString::compare(szVerb, QString("PUT"), Qt::CaseInsensitive) == 0)
    {
        mNetworkReply = mNetworkAccessManager->put(request, data);
    }
    else if (QString::compare(szVerb, QString("POST"), Qt::CaseInsensitive) == 0)
    {
        mNetworkReply = mNetworkAccessManager->post(request, data);
    }
    else if (QString::compare(szVerb, QString("HEAD"), Qt::CaseInsensitive) == 0)
    {
        mNetworkReply = mNetworkAccessManager->head(request);
    }

    if (mbAsyncMode)
    {
        QObject::connect(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)), Qt::DirectConnection);
        QObject::connect(mNetworkReply, SIGNAL(finished()), this, SLOT(onFinished()));
    }
    else
    {
        QEventLoop loop;

        //create a timer set to the timeout passed in have it call quit since there doesn't seem to be a way to
        //change the default timeout for QtNetwork calls
        QTimer::singleShot(timeout, &loop, SLOT(quit()));

        QObject::connect(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)), Qt::DirectConnection);
        QObject::connect(mNetworkReply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        QObject::disconnect(mNetworkReply, SIGNAL(finished()), &loop, SLOT(quit()));
        QObject::disconnect(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)));
        
        if(!mbRequestForcedClosed && mNetworkReply != NULL)
        {
            ProcessStatusAndHeaders();
        }
        else
        {
            SetLastErrorType(DownloadError::NotConnectedError);
        }
    }

    return GetLastErrorType() == DownloadError::OK;
}

bool HttpChannel::QueryDataAvailable(ulong* avail)
{
    if (avail == NULL || mNetworkReply == NULL)
    {
        return false;
    }

    *avail = static_cast<ulong>(mNetworkReply->bytesAvailable());
    return (*avail)>0;
}

// Synchronous version
bool HttpChannel::ReadData(void* lpBuffer, qint64 iNumberOfBytesToRead, qint64* lpiNumberOfBytesRead,
                              volatile bool& stop, unsigned long ulTimeoutMs)
{
    qint64 lNumberOfBytesRead = 0;
    if ( lpiNumberOfBytesRead )
        *lpiNumberOfBytesRead = 0;

    if (mNetworkReply == 0 || mNetworkReply->isRunning() || lpBuffer == NULL )
        return false;

    qint64 nRead;
    qint64 nRemToRead = iNumberOfBytesToRead;
    ulong availableBytes;
    bool result;

    qlonglong llTimeoutAt = 0;
    if (ulTimeoutMs > 0)
        llTimeoutAt = GetTick() + ulTimeoutMs;

    do
    {
        availableBytes = 0;
        result = QueryDataAvailable(&availableBytes);

        if (result) // Check for data
        {
            //Limit to 8k chunks so we can report activity
            if (availableBytes > 8 * 1024)
                availableBytes = 8 * 1024;

            //Limit to required
            if (availableBytes > iNumberOfBytesToRead)
                availableBytes = static_cast<ulong>(iNumberOfBytesToRead);

            nRead = mNetworkReply->read(reinterpret_cast<char*>(lpBuffer), static_cast<qint64>(availableBytes));
            result = nRead != -1;

            if ( nRead > 0 )
            {
                lNumberOfBytesRead += nRead;
                if ( lpiNumberOfBytesRead )
                    *lpiNumberOfBytesRead += nRead;

                lpBuffer = (reinterpret_cast<char*>(lpBuffer)) + nRead;
                nRemToRead -= nRead;

                onDataReceived(static_cast<ulong>(nRead));
            }

            SetLastErrorType(translateQNetworkError(this, mNetworkReply->error()));
            SetErrorCode(mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        }
        else
        {
            SetLastErrorType(DownloadError::UNKNOWN);
        }

        // Handle the timeout (if enabled then ulTimeoutMS > 0)
        if (ulTimeoutMs > 0 && GetTick() > llTimeoutAt)
            break;
    }

    // One pass when lTimeoutMs < 0
    while ( !stop && result && availableBytes > 0 && nRemToRead > 0);

    if (!stop && result && (iNumberOfBytesToRead == 0 ||
            (lNumberOfBytesRead == iNumberOfBytesToRead)))
    {
        if (lpiNumberOfBytesRead)
            *lpiNumberOfBytesRead = lNumberOfBytesRead;
        return true;
    }

    return false;
}

void HttpChannel::CloseRequest(bool forceCanceled)
{
    if(mNetworkReply != NULL)
    {
        mbRequestForcedClosed = forceCanceled;

        // Disconnect all signal handling and abort the reply
        QObject::disconnect(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)));
        QObject::disconnect(mNetworkReply, SIGNAL(finished()), this, SLOT(onFinished()));
        mNetworkReply->abort();
        mNetworkReply->deleteLater();
        mNetworkReply = NULL;
    }

}

bool HttpChannel::RequestComplete()
{
    return mNetworkReply != NULL && mNetworkReply->isFinished();
}

void HttpChannel::CloseConnection()
{
    CloseRequest(true);
    mbConnected = false;
    onDisconnected();
}

/*QByteArray HttpChannel::GetResponseCodeDescription(unsigned code)
{
    return HttpResponseCode::getResponseDescription(code);
}*/

void HttpChannel::SetFormattedRequestHeaders(QNetworkRequest& request) const
{

    if (!mRequestInfo.empty())
    {
        for (QHash<QString,QString>::const_iterator it = mRequestInfo.begin(); it != mRequestInfo.end(); it++ )
        {
            request.setRawHeader(it.key().toLatin1(), it.value().toLatin1());
        }
    }
}

void HttpChannel::ProcessStatusAndHeaders()
{
    // The network will be null if a logout occurs or the network goes down.
    // We can just return because nobody cares about this reply anymore. A
    // new HttpChannel will be made on login or network re-connection.
    if (!mNetworkReply)
    {
        ORIGIN_LOG_EVENT << "Null network reply. Unable to process.";
        return;
    }

    //if the network response timed out
    if (!mNetworkReply->isFinished ())
    {
        onErrorReceived (QNetworkReply::TimeoutError);
    }

    QVariant responseCode = mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    SetErrorCode(responseCode.toInt());

    QVariant reasonPhrase = mNetworkReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    //if (reasonPhrase.toByteArray().isNull() || reasonPhrase.toByteArray().isEmpty())
    //{
        //QByteArray text = GetResponseCodeDescription(responseCode.toInt());
        //if (text.isEmpty())
        //{
            //text = (QString("Server response code ") + QString::number(responseCode.toInt())).toLatin1();
        //}
        //SetResponseReason(text);
    //}
    //else
    //{
        //SetResponseReason(reasonPhrase.toByteArray());
    //}

    ParseResponseHeaders(*mNetworkReply);
}

void HttpChannel::ParseResponseHeaders(const QNetworkReply& response)
{
    mResponseInfo.clear();

    const QList<QNetworkReply::RawHeaderPair> rawHeaderInfo = response.rawHeaderPairs();
    for (QList<QNetworkReply::RawHeaderPair>::const_iterator i = rawHeaderInfo.constBegin();
        i != rawHeaderInfo.constEnd(); ++i)
    {
        QString key(i->first);
        QString value(i->second);
        AddResponseInfo(key, value);
    }
}

bool HttpChannel::IsTransferring()
{
    if (mNetworkReply != NULL)
    {
        return !mNetworkReply->isFinished();
    }

    return false;
}

void HttpChannel::onErrorReceived(QNetworkReply::NetworkError err)
{
    DownloadError::type e = translateQNetworkError(this, err);
    SetLastErrorType(e);
    SetErrorCode(mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    onError(e);
}

void HttpChannel::onFinished()
{
    ProcessStatusAndHeaders();

    if((int)mErrorType == (int)QNetworkReply::NoError)
    {
        qint64 bytesRead = 1;
        while (bytesRead != 0)
        {
            bytesRead = 0;
            ulong count = static_cast<ulong>(mNetworkReply->bytesAvailable());
            int actual_bytes_read = static_cast<int>(count);
            if(count > 0)
            {
                void* buffer = NULL;
                onDataAvailable(&count, &buffer);
                if (buffer != NULL)
                {
                    bytesRead = mNetworkReply->read(reinterpret_cast<char*>(buffer), count);
                    SetLastErrorType(translateQNetworkError(this, mNetworkReply->error()));
                    SetErrorCode(mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
                    onDataReceived(bytesRead);

                    if (!validateFinished(bytesRead) || (bytesRead != actual_bytes_read))
                    {
                        onErrorReceived(QNetworkReply::TemporaryNetworkFailureError);
                    }
                }
            }
        }
    }
}

bool HttpChannel::validateFinished(int bytesRead)
{
    QString range  = GetRequestInfo("Range");
    int expectedNumberOfBytes = 0;
    if (!range.isEmpty())
    {
        int idx1 = range.indexOf('=');
        int idx2 = range.indexOf('-');
        if (idx1 != -1 && idx2 != -1)
        {
            uint64_t startByte = range.mid(idx1 + 1, idx2 - idx1 - 1).toULongLong();
            uint64_t endByte = range.mid(idx2 + 1).toULongLong();
            expectedNumberOfBytes = endByte - startByte + 1;
        }
    }

    if (expectedNumberOfBytes == 0)
        return true;    // can't verify so just pass.

    return bytesRead == expectedNumberOfBytes;
}

} // namespace Downloader
} // namespace Origin

