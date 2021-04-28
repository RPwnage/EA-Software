//
// Copyright 2008, Electronic Arts Inc
//
#ifndef CHANNELINTERFACE_H
#define CHANNELINTERFACE_H

#include "services/downloader/DownloaderErrors.h"
#include "services/session/AbstractSession.h"

#include <list>

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace Origin
{
namespace Downloader
{

class ChannelInterface;

/**
A channel spy is an observer for channel activity.
*/
class ChannelSpy
{
public:
    virtual void onConnected(ChannelInterface*) {}
    virtual void onDataAvailable(ChannelInterface*, ulong* pBytes, void** ppBuffer_ret = NULL) {}
    virtual void onDataReceived(ChannelInterface*, ulong bytes) {}
    virtual void onConnectionClosed(ChannelInterface*) {}
    virtual void onError(ChannelInterface*, DownloadError::type error) {}
};

typedef std::list< ChannelSpy* > ChannelSpyList;

typedef QHash<QString, QString> ChannelInfo;

/**
This is a very simple interface to a connection and request driven channel
Possible implementations include HTTP, FTP or proprietary P2P
*/
class ChannelInterface
{
public:
    ChannelInterface();
    virtual	~ChannelInterface();

    static const unsigned long CHANNEL_DEFAULT_TIMEOUT = 2L*60L*1000L;	//2 min 'cuz QT's default is 2 min.
    /**
    Factory, create a ChannelInterface by type, determined by format of URL. The only current implementation is http (allows https too)
    */
    static ChannelInterface* Create(const QString& urlString, Services::Session::SessionWRef session);

    /**
     * Helper method for channel to cleanup URL string.  Attempts to handle sloppy url input
     * such as file://d:\\autorun.iso which QUrl considers invalid.
     */
    static QString CleanupUrl(const QString& urlString);

    /**
     * Returns whether the specified URL is a network URL, which requires an online
     * connection state.
     */
    static bool IsNetworkUrl(const QString& urlString);

    /**
    This channel type
    */
    virtual QString GetType() const = 0;

    /**
    Channel ID
    */
    void SetId( const QString& );
    const QString& GetId() const;

    /**
    Resource fully qualified URL
    */
    bool SetURL( const QString& );
    const QUrl& GetURL() const;

    /**
    Get the URL as a string
    */
    QString GetUrlString() const;

    /**
    Get the URL path and extra information such as parameters '?' and/or fragments '#'
    */
    QString GetUrlPathAndExtra() const;

    /**
    User for authentication purposes
    */
    void SetUserName(const QString&);
    QString getUserName() const;

    /**
    Password for authentication purposes
    */
    void SetPassword(const QString&);
    QString GetPassword() const;

    /**
    Add extra information to the request.
    This information is sent according to the protocol implementation (i.e: headers for HTTP).
    This information is sent for every request, so use RemoveAllRequestInfo() to remove headers
    */
    virtual void AddRequestInfo(const QString& key, const QString& value);

    /**
    Get request information by key
    */
    virtual QString GetRequestInfo(const QString& key) const;

    /**
    Remove a particular request information
    */
    void RemoveRequestInfo(const QString& key);

    /**
    Remove all request information
    */
    void RemoveAllRequestInfo();

    /**
    Connect with the "server"
    */
    virtual bool Connect() = 0;

    /**
    Check if the connection established
    */
    virtual bool IsConnected() const = 0;

    /**
    Request the given resource.
    Verb indicates the action, such as GET, POST, LIST etc Meaningful for the protocol.
    If a path is indicated is used instead of the current path.
    Any additional info indicated is sent to the server along with the additional
    info previously added with AddRequestInfo().
    (This is so to allow setting common headers and URL information and them performing successive requests)
    */
    virtual bool SendRequest(const QString& verb, const QString& path = QString(),
                             const QString& addInfo = QString(),
                             const QByteArray& data = QByteArray(),
                             const unsigned long timeout = CHANNEL_DEFAULT_TIMEOUT) = 0;


    virtual qint32 GetErrorCode() const;

    /**
    Get the response reason phrase for the last request, if any.
    This may not have sense on some protocols.
    */
    //virtual QByteArray GetResponseReason() const;

    /**
    Test response is 2XX. This class of status code indicates that the client's
    request was successfully received, understood, and accepted.
    */
    //bool IsResponseCodeSuccess() const;

    /**
    Query a particular response information value
    */
    virtual QString GetResponseInfo(const QString& key) const;

    /**
    Test the existence of a particular response information value
    */
    virtual bool HasResponseInfo(const QString& key) const;

    /**
    Get all received response information. Some implementations may opt
    to override GetResponseInfo() to provide info on some keys so this call
    discouraged and should only be used for informational purposes
    */
    const ChannelInfo& GetAllResponseInfo() const;

    /**
    Get all sent request information.
    */
    const ChannelInfo& GetAllRequestInfo() const;

    /**
    Returns the amount of data, in bytes, available to be read.
    */
    virtual bool QueryDataAvailable(ulong* avail) = 0;

    /**
    Read data from the channel. A previous request had to be made with SendRequest()
    dwNumberOfBytesToRead: Requested nr of bytes, lpBuffer must point to a buffer at least this size.
    lpdwNumberOfBytesRead: On exit contains number of bytes actually read.
    stop: flag to be signaled if the operation has to be interrupted
    ulTimeoutMs: Timeout in milliseconds
    */
    virtual bool ReadData(void* lpBuffer, qint64 iNumberOfBytesToRead, qint64* lpiNumberOfBytesRead,
                          volatile bool& stop, ulong ulTimeoutMs /* = 0 for infinite */) = 0;

    /**
    Close the ongoing request
    */
    virtual void CloseRequest(bool forceCanceled = false) = 0;

    /**
    Close the active connection
    */
    virtual void CloseConnection() = 0;

    /*
    Set asynchronous mode
    */
    virtual bool SetAsyncMode(bool bAsyncMode) = 0;

    /*
    Is busy transferring an asynchronous request?
    */
    virtual bool IsTransferring() { return false; }

#ifdef _DEBUG
    /*
    Debugging mode thread naming
    */
    virtual void SetAsyncThreadName(const char* sName) {}  // Not supported by default
#endif

    /*
    Request response
    */
    virtual bool RequestComplete() = 0;

    /**
    Get the last error code. Error code are system errors.
    */
    DownloadError::type GetLastErrorType() const;

    /**
    Add a spy
    */
    void AddSpy(ChannelSpy*);
    void RemoveSpy(ChannelSpy*);

protected:
    /**
    Set the last request response code. Use HTTP response codes.
    */
    void SetErrorCode(qint32 code);

    /**
    Set the last request response reason (if any)
    */
    //void SetResponseReason(const QByteArray& data);

    /**
    To be called by derived classes to provide response information
    */
    void AddResponseInfo(const QString& key, const QString& value);
    
    /**
    Set last error code, description will be set from system.
    */
    virtual void SetLastErrorType(DownloadError::type err);

    /**
    To be called by derived classes. Base class notifies spies
    */
    virtual void onConnected();
    virtual void onDisconnected();
    virtual void onDataAvailable(ulong* pCount, void** ppBuffer_ret = NULL);
    virtual void onDataReceived(ulong count);
    virtual void onError(DownloadError::type error);

protected:
    QString							mId;
    QUrl							mUrl;
    ChannelInfo						mRequestInfo;

    //QByteArray						mResponseReason;

    DownloadError::type	            mErrorType;
    qint32							mErrorCode;
    QString							mErrorDescription;
    ChannelInfo						mResponseInfo;

    ChannelSpyList					mSpies;
};

} // namespace Downloader
} // namespace Origin

#endif
