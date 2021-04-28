#ifndef LOCALHOST_HttpRequestHandler_H
#define LOCALHOST_HttpRequestHandler_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QBuffer>
#include <QUrl>
#include <QElapsedTimer>
#include "LocalHost/HttpStatusCodes.h"
#include "LocalHost/LocalHostServices/IService.h"
#include "LocalHostSecurity.h"

class QTcpSocket;

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class HttpRequestHandler : public QObject
            {
                Q_OBJECT

                public:
                    HttpRequestHandler(QTcpSocket *socket, QObject *parent);
                    HttpRequestHandler(QSslSocket *socket, QObject *parent);
                    ~HttpRequestHandler();

                private slots:
                    ///
                    /// handles socket errors
                    ///
                    void onSocketErrors();
                    ///
                    /// slot gets called any time a new message comes through the socket
                    ///
                    void onMessageReceived();
                    ///
                    /// slot that gets called when the security validation is completed the bool param is true if the connection is valid
                    ///
                    void onSecurityValidationComplete(bool);
                    ///
                    /// slot that gets called when the process is ready to execute (synchronous or asynchronous)
                    ///
                    void onProcessRequest();
                    ///
                    /// slot that gets called when the process is completed and ready to send a response
                    ///
                    void onProcessCompleted(ResponseInfo response);

                private:

                    enum HandlerState
                    {
                        ParseRequestLine,
                        ParseRequestHeaders,
                        CheckRequestLineErrors,
                        ProcessRequest,
                        ErrorResponse,
                    };
                    
                    ///
                    /// sends response back to the requestor (browser)
                    ///
                    void sendResponse(ResponseInfo responseInfo);

                    ///
                    /// gets the value of for an entry in the http header
                    ///
                    QString getRequestHeaderValue(const QString &name);

                    ///
                    /// create a response header to send back to the requester
                    ///
                    QString generateResponseHeader(int responseCode,  int contentLength = 0);

                    ///
                    /// adds a line to the response header
                    ///
                    void appendLineToResponseHeader(QString &header, const QString &name, const QString &value);

                    ///
                    /// sends an error reponse back to requestor
                    /// \param error the http status code
                    ///
                    void sendErrorHttpResponse(int error);

                    /// 
                    /// parse the first line of the http header
                    ///
                    bool parseRequestLine();

                    /// 
                    /// parse the http request headers
                    ///
                    bool parseRequestHeaders();

                    /// 
                    /// this makes a call to the actual service (e.g. progress, authentication) which does the work 
                    ///
                    bool processRequest();
    
                    ///
                    /// checks to see if there were any errors when parsing the request line
                    ///
                    bool checkRequestLineErrors();

                    ///
                    /// checks to see if the version matches one we support (currently 1.1 and higher)
                    /// \param httpVersionString the version string from the http header (HTTP/1.1) 
                    ///
                    bool checkVersion(QString httpVersionString);

                    ///
                    /// returns the uri to send back in the response header for CORS, empty if the request is coming from a site that we did not white list
                    ///
                    QString accessControlAllowOriginUrl();

                    ///
                    /// takes a url and removes the query param that matches the param_key
                    ///
                    void removeQueryParam(QUrl &url, QString param_key);

                    ///
                    /// a map of the http header ids to values
                    ///
                    QMap<QString, QString> mRequestHeaders;

                    ///
                    /// the request Url
                    ///
                    QUrl mRequestUrl;

                    /// 
                    /// the current state of processing the request
                    ///
                    HandlerState mHandlerState;

                    ///
                    /// buffer that holds the incoming msg data
                    ///
                    QBuffer mRawRequestData;

                    ///
                    /// the current position of the pointer to mRawRequestData
                    ///
                    qint64 mRequestDataPos;

                    ///
                    /// holds any errors we found when parsing the request line, we must store it and return the error after we parse the rest of the headers other wise CORS will block the actual error from being sent to requester
                    /// 
                    int mParseRequestLineResult;

                    /// 
                    /// pointer the the service that the request uses (ping, progress, auth, etc)
                    IService *mService;

                    ///
                    /// the socket we are using to pass information through
                    ///
                    QTcpSocket *mTcpSocket;

                    ///
                    /// the ssl socket we are using to pass information through
                    ///
                    QSslSocket *mSslSocket;

                    /// 
                    /// timer used to timeout request
                    QElapsedTimer mRequestTimer;

                    ///
                    /// the response information to send back to caller
                    ///
                    ResponseInfo mResponseInfo;

                    ///
                    /// is the process request to be handled asynchronously - set by service
                    ///
                    bool mIsAsychronous;

                    ///
                    /// the domain name of the calling host - used to do possible https security handshake
                    ///
                    QString mCallingDomainName;

                    ///
                    /// the bit identifier for the secure service
                    ///
                    int mServiceSecurityBit;

                    ///
                    /// the LocalHostSecurityRequest object needed for https assisted validation of connection
                    ///
                    LocalHostSecurityRequest *mSecurityRequest;
            };
        }
    }
}

#endif