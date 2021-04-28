//
// Copyright 2008, Electronic Arts Inc
//
#ifndef HTTPCHANNEL_H
#define HTTPCHANNEL_H

#include "ChannelInterface.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/session/AbstractSession.h"
#include <QNetworkReply>
#include <QMutex>
#include <QSslConfiguration>
#include <QThreadStorage>

class QNetworkRequest;
class QNetworkReply;

// If this is defined, the http channel will generate random errors
// at the percentage specified in PERCENT_CHANCE_OF_SIMULATED_ERROR
// #define SIMULATE_RANDOM_NETWORK_ERRORS 1

#ifdef SIMULATE_RANDOM_NETWORK_ERRORS
#include <EAStdC/EARandom.h>
#define SIMULATE_ONLY_RETRYABLE_ERRORS 1
#define PERCENT_CHANCE_OF_SIMULATED_ERROR 2
#endif

namespace Origin
{

namespace Services
{
    class NetworkAccessManager;
}

namespace Downloader
{

/**
Channel interface for HTTP protocol
*/
class HttpChannel : public QObject, public ChannelInterface
{
    Q_OBJECT

public:
    typedef QMap<QNetworkReply::NetworkError, DownloadError::type> QNetworkErrorMap;
    static QNetworkErrorMap sQNetworkErrorTranslator;
    static QNetworkErrorMap initQNetworkErrorTranslator();
    static DownloadError::type translateQNetworkError(HttpChannel* channel, QNetworkReply::NetworkError err);

    HttpChannel(Services::Session::SessionWRef session);
    virtual	~HttpChannel();

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
    Override: If the header already exists we append using ', '
    */
    virtual void AddRequestInfo( const QString& key, const QString& value );

    /**
    Override: Request the given resource.
    If the request was sent and the response received, it returns true even if 
    the server response is an error.
    Server responses are the callers responsibility.
    You can still read data the same as with a 200 OK response.
    */
    virtual bool SendRequest( const QString& verb, const QString& path, 
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
                  volatile bool& stop, unsigned long ulTimeoutMs /* = 0 for infinite */ );

    
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
    Request response
    */
    bool RequestComplete();

    bool IsTransferring();

    /**
    Get the default description for a given HTTP response code
    */
    //static QByteArray GetResponseCodeDescription(unsigned code);

public slots:
    void onErrorReceived(QNetworkReply::NetworkError err);
    void onFinished();

private:

    void SetFormattedRequestHeaders(QNetworkRequest& request) const;
    void ProcessStatusAndHeaders();
    void ParseResponseHeaders(const QNetworkReply& response);
    bool validateFinished(int bytesRead);

    Services::Session::SessionWRef mSession;
    Origin::Services::NetworkAccessManager* mNetworkAccessManager;
    QNetworkReply*		   mNetworkReply;
    bool				   mbAsyncMode;
    bool                   mbConnected;
    bool                   mbRequestForcedClosed;


#ifdef SIMULATE_RANDOM_NETWORK_ERRORS
public slots:
    void raiseSimulatedError();

private:
    void generateRandomError();
    static EA::StdC::Random sRandom;
    static QMutex       sGenerateMutex;
    DownloadError::type mSimulateErrorType;
    qint32              mSimulateErrorCode;
#endif
};

} // namespace Downloader
} // namespace Origin

#endif
