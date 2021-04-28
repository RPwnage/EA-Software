#include <QHostAddress>
#include <QTCPSocket>
#include "HttpRequestHandler.h"
#include "LocalHost/LocalHostServiceHandler.h"
#include "LocalHost/LocalHostServices/IService.h"
#include "services/log/LogService.h"
#include "services/config/OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            const int sHttpMajorVersionNumber = 1;
            const int sHttpMinorVersionNumber = 1;

            const char sHeaderHttpVer[] = "HTTP/%1.%2";
            const char sHeaderOrigin[] = "Origin";
            const char sHeaderContentLength[] = "Content-Length";
            const char sHeaderConnection[] = "Connection";
            const char sHeaderDate[] = "Date";
            const char sHeaderCacheControl[] = "Cache-Control";
            const char sHeaderServer[] = "Server";
            const char sHeaderAccessControlAllowOrigin[] = "Access-Control-Allow-Origin";
            const char sHeaderHost[] = "Host";

            const char sServerId[] = "OriginClient-LocalHost";

            // from PendingActionDefines.h
            const char ParamAuthCodeId[] = "authCode";
            const char ParamAuthTokenId[] = "authToken";
            const char ParamRedeemCodeId[] = "redeemCode";

            HttpRequestHandler::HttpRequestHandler(QTcpSocket *socket, QObject *parent)
                : QObject(parent)
                , mHandlerState(ParseRequestLine)
                , mRequestDataPos(0)
                , mParseRequestLineResult(HttpStatus::OK)
                , mService(NULL)
                , mTcpSocket(socket)
                , mSslSocket(NULL)
                , mIsAsychronous(false)
            {
                ORIGIN_VERIFY_CONNECT(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketErrors()));
                ORIGIN_VERIFY_CONNECT(mTcpSocket, SIGNAL(readyRead()), this, SLOT(onMessageReceived()));
                mRequestTimer.start();
            }

            HttpRequestHandler::HttpRequestHandler(QSslSocket *socket, QObject *parent)
                : QObject(parent)
                , mHandlerState(ParseRequestLine)
                , mRequestDataPos(0)
                , mParseRequestLineResult(HttpStatus::OK)
                , mService(NULL)
                , mTcpSocket(socket)
                , mSslSocket(socket)
                , mIsAsychronous(false)
            {
                ORIGIN_VERIFY_CONNECT(mSslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketErrors()));
                ORIGIN_VERIFY_CONNECT(mSslSocket, SIGNAL(readyRead()), this, SLOT(onMessageReceived()));
                mRequestTimer.start();
            }

            HttpRequestHandler::~HttpRequestHandler()
            {
                if (mSslSocket)
                {
                    ORIGIN_VERIFY_DISCONNECT(mSslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketErrors()));
                    ORIGIN_VERIFY_DISCONNECT(mSslSocket, SIGNAL(readyRead()), this, SLOT(onMessageReceived()));
                }
                else
                if (mTcpSocket)
                {
                ORIGIN_VERIFY_DISCONNECT(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketErrors()));
                ORIGIN_VERIFY_DISCONNECT(mTcpSocket, SIGNAL(readyRead()), this, SLOT(onMessageReceived()));
            }
            }

            void HttpRequestHandler::onMessageReceived()
            {
                const int maxMessageSize = 4096;  //4k
                const int maxTimeAllowedForRequest = 10000; //10 secs

                //we cannot guarantee how much of the message we get at a time, so we process as much as we can
                //while we wait for more data

                if(mRawRequestData.open(QBuffer::ReadWrite | QBuffer::Append | QBuffer::Unbuffered))
                {
                    mRawRequestData.write(mTcpSocket->readAll());
                    mRawRequestData.seek(mRequestDataPos);

                    //if we've exceeded the max message size or max time allowed for a request to finish sending, send an error, we should never see this
                    //this is to protect against denial of service attacks
                    if(mRawRequestData.size() > maxMessageSize)
                    {
                        sendErrorHttpResponse(HttpStatus::RequestEntityTooLarge);
                        return;
                    }


                    if(mRequestTimer.elapsed() > maxTimeAllowedForRequest)
                    {
                        sendErrorHttpResponse(HttpStatus::RequestTimeout);
                        return;
                    }

                    bool readMore = true;

                    while(readMore)
                    {
                        switch (mHandlerState)
                        {
                        case ParseRequestLine:
                            {
                                readMore = parseRequestLine();
                                break;
                            }

                        case ParseRequestHeaders:
                            {
                                readMore = parseRequestHeaders();
                                break;
                            }

                        case CheckRequestLineErrors:
                            {
                                readMore = checkRequestLineErrors();
                                break;
                            }
                        case ProcessRequest:
                            {
                                readMore = processRequest();
                                break;
                            }
                        default:
                            readMore = false;
                            break;
                        }
                    }

                    if (!mIsAsychronous)
                    {
                        mRequestDataPos = mRawRequestData.pos();
                        mRawRequestData.close();
                    }
                }

            }

            void HttpRequestHandler::onSocketErrors()
            {
                QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
                if(socket)
                {
                    ORIGIN_LOG_DEBUG << QString("Socket Error (from %1:%2) -- %3").arg(socket->peerAddress().toString()).arg(socket->peerPort()).arg(socket->errorString());
                    socket->disconnect();
                }
            }

            QString HttpRequestHandler::getRequestHeaderValue(const QString &name)
            {
                return mRequestHeaders[name.toLower()];
            }

            void HttpRequestHandler::appendLineToResponseHeader(QString &header, const QString &name, const QString &value)
            {
                header += (name + ": " + value + "\r\n");
            }

            void HttpRequestHandler::sendErrorHttpResponse(int errorCode)
            {
                mHandlerState = ErrorResponse;

                ResponseInfo responseInfo;
                responseInfo.errorCode = errorCode;
                sendResponse(responseInfo);
            }

            bool HttpRequestHandler::checkVersion(QString httpVersionString)
            {
                bool success = false;

                //split up the http/1.1 string
                QStringList httpVersionParts = httpVersionString.split('/');

                int majorVersion = 0;
                int minorVersion = 0;

                if(httpVersionParts.size() == 2)
                {
                    //just get the number part
                    QString versionStr = httpVersionParts.at(1);

                    //split that to get the major/minor version
                    QStringList versionParts = versionStr.split('.');

                    if(versionParts.size() == 2)
                    {
                        majorVersion = versionParts.at(0).toLong();
                        minorVersion = versionParts.at(1).toLong();

                        if(majorVersion > sHttpMajorVersionNumber)
                        {
                            success = true;
                        }

                        if(majorVersion == sHttpMajorVersionNumber)
                        {
                            if(minorVersion >= sHttpMinorVersionNumber)
                            {
                                success = true;
                            }
                        }
                    }
                }

                return success;
            }

            void HttpRequestHandler::removeQueryParam(QUrl &url, QString param_key)
            {
                QUrlQuery queryParams(url.query());

                queryParams.removeQueryItem(param_key);

                url.setQuery(queryParams);
            }

            bool HttpRequestHandler::parseRequestLine()
            {
                //based off info here
                //http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html

                if(mRawRequestData.canReadLine())
                {
                    QList<QByteArray> requestLineElements = mRawRequestData.readLine().split(' ');
                    //check the number of elements in the request line
                    if(requestLineElements.size() == 3)
                    {
                        QString method = requestLineElements.at(0).trimmed();
                        mRequestUrl = QString(requestLineElements.at(1).trimmed());
                        QString httpVersion = requestLineElements.at(2).trimmed();

                        // if this is localhost via HTTPS, remove any auth params (part of OFM-8301)
                        if (mSslSocket)
                        {
                            removeQueryParam(mRequestUrl, ParamAuthCodeId);
                            removeQueryParam(mRequestUrl, ParamAuthTokenId);
                            // for localhost call via HTTPS, remove prepopulation of redemption codes (part of OFM-8301) - (changed from any localhost call per Aki - 3/19/15)
                            removeQueryParam(mRequestUrl, ParamRedeemCodeId);
                        }
                        
                        //check if http/1.1
                        if(checkVersion(httpVersion))
                        {
                            //look for a service mapped to the request
                            mService = LocalHostServiceHandler::findService(mRequestUrl.path());
                            if(mService)
                            {
                                //make sure the method in the request matches what is available for the service
                                if(method.compare(mService->method(), Qt::CaseInsensitive) == 0)
                                {
                                    mHandlerState = ParseRequestHeaders;
                                }
                                else
                                {
                                    //we only support GET and POST for the server, if the method is not either, return 501 code
                                    //if we support the method but it doesn't match what we use for the service return 405
                                    if((method.compare("GET", Qt::CaseInsensitive) != 0 ) &&
                                        (method.compare("POST", Qt::CaseInsensitive) != 0 ))
                                    {
                                        mParseRequestLineResult = HttpStatus::NotImplemented;
                                    }
                                    else
                                    {
                                        mParseRequestLineResult = HttpStatus::MethodNotAllowed;  
                                    }
                                }
                            }
                            else
                            {
                                //we can't find a service registered to that request
                                mParseRequestLineResult = HttpStatus::NotFound;  
                            }
                        }
                        else
                        {
                            //don't support the http version
                            mParseRequestLineResult = HttpStatus::HttpVersionNotSupported;  
                        }
                        //move on to the next state to parse params, we'll handle any errors after we verify the Origin header
                        mHandlerState = ParseRequestHeaders;
                    }
                    else
                    {
                        //malformed request
                        sendErrorHttpResponse(HttpStatus::BadRequest);
                    }
                    return true;

                }
                return false;

            }

            bool HttpRequestHandler::parseRequestHeaders()
            {
                while(mRawRequestData.canReadLine())
                {
                    QString headerLine = mRawRequestData.readLine();

                    //we found the line break in between the header and body
                    if(headerLine.trimmed().isEmpty())
                    {
                        //we check the requestline errors now, because we need the Origin from the headers to verify cors when sending the errors back
                        mHandlerState = CheckRequestLineErrors;
                        return true;
                    }

                    int separatorLocation = headerLine.indexOf(':');
                    QPair<QString, QString> headerPair;
                    mRequestHeaders[headerLine.mid(0, separatorLocation).trimmed().toLower()] =  headerLine.mid(separatorLocation + 1).trimmed();
                }

                sendErrorHttpResponse(HttpStatus::BadRequest);
                return false;

            }

            bool HttpRequestHandler::checkRequestLineErrors()
            {
                if(mParseRequestLineResult != HttpStatus::OK)
                {
                    sendErrorHttpResponse(mParseRequestLineResult);
                }
                else
                {
                    mHandlerState = ProcessRequest;
                }

                return true;
            }

            QString HttpRequestHandler::accessControlAllowOriginUrl()
            {
                //check Origin url against our list of urls from config service
                const std::vector<QString> &whitelistedUrls = Origin::Services::OriginConfigService::instance()->clientAccessWhiteListUrls().url;

                const QUrl &originHeader = getRequestHeaderValue(sHeaderOrigin);

				//if user has ODT and is in developer mode, turn off cors for development purposes
				if(Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled).toQVariant().toBool())
				{
                    QString sOriginHeader = originHeader.toString();
                    if (sOriginHeader.isEmpty())
                    {
                        QString hostHeader = getRequestHeaderValue(sHeaderHost);
                        if (!hostHeader.isEmpty())
                        {
                            sOriginHeader = hostHeader;
                        }
                    }
                    return sOriginHeader;
				}
				
                for(std::vector<QString>::size_type i = 0; i < whitelistedUrls.size(); i++)
                {
                    QUrl whiteListUrl = whitelistedUrls[i];

					if((whiteListUrl.host().toLower() == originHeader.host().toLower()) &&
						(whiteListUrl.scheme().toLower() == originHeader.scheme().toLower()) &&
						whiteListUrl.port()== originHeader.port())
                    {
                        return originHeader.toString();
                    }
                }
                return QString();
            }

            QString HttpRequestHandler::generateResponseHeader(int responseCode, int contentLength)
            {
                QString header;
                QString httpVerStr = QString(sHeaderHttpVer).arg(sHttpMajorVersionNumber).arg(sHttpMinorVersionNumber);

                header += QString("%1 %2 %3\r\n").arg(httpVerStr).arg(responseCode).arg(getMessageFromCode(responseCode));

                appendLineToResponseHeader(header, sHeaderConnection, "close");
                appendLineToResponseHeader(header, sHeaderDate, QDateTime::currentDateTimeUtc().toString("ddd, d MMM yyyy hh:mm:ss") + " GMT");
                appendLineToResponseHeader(header, sHeaderContentLength, QString::number(contentLength));
                appendLineToResponseHeader(header, sHeaderCacheControl, "no-cache");
                appendLineToResponseHeader(header, sHeaderServer, sServerId);

                QString allowedURI = accessControlAllowOriginUrl();
                if(!allowedURI.isEmpty())
                {
                    appendLineToResponseHeader(header, sHeaderAccessControlAllowOrigin, allowedURI);
                }

                //always need this extra line
                header += "\r\n";
                return header;
            }

            void HttpRequestHandler::sendResponse(ResponseInfo responseInfo)
            {
                QString header;
                header = generateResponseHeader(responseInfo.errorCode, responseInfo.response.toUtf8().length());
                QString rawResponse = header + responseInfo.response;

                mTcpSocket->write(rawResponse.toUtf8());
                mTcpSocket->waitForBytesWritten();
                mTcpSocket->disconnectFromHost();
            }

            bool HttpRequestHandler::processRequest()
            {
                QString msg;
                ResponseInfo responseInfo;

                mCallingDomainName = accessControlAllowOriginUrl();

                if (mService && !mCallingDomainName.isEmpty())
                {
                    // remove leading "http" or "https"
                    mCallingDomainName = mCallingDomainName.remove("http://", Qt::CaseInsensitive);
                    mCallingDomainName = mCallingDomainName.remove("https://", Qt::CaseInsensitive);

                    mServiceSecurityBit = mService->getSecurityBitPosition();
                    if (mService->isSecureService() && !LocalHostSecurity::isCurrentlyUnlocked(mCallingDomainName, mServiceSecurityBit))
                    {
                        // initiate asynchronous https validation call 
                        mSecurityRequest = LocalHostSecurity::validateConnection(mCallingDomainName);
                        if (mSecurityRequest == NULL)
                            onSecurityValidationComplete(false);
                        else
                        {
                            ORIGIN_VERIFY_CONNECT(mSecurityRequest, SIGNAL(validationCompleted(bool)), this, SLOT(onSecurityValidationComplete(bool)));
                        }
                    }
                    else
                        onProcessRequest(); // call this directly
                }
                else
                {
                    if (mService && mService->isSecureService()) // if we had a service and it was secure but we are here, then we must have failed the CORS test
                    {
                        const QUrl &originHeader = getRequestHeaderValue(sHeaderOrigin);
                        GetTelemetryInterface()->Metric_LOCALHOST_SECURITY_FAILURE(QString("Failed whitelist : %1").arg(originHeader.toString()).toLatin1());
                    }

                    responseInfo.errorCode = HttpStatus::NotFound;
                    sendResponse(responseInfo);
                }
                
                return false;
            }

            void HttpRequestHandler::onSecurityValidationComplete(bool valid)
            {
                ORIGIN_VERIFY_DISCONNECT(mSecurityRequest, SIGNAL(validationCompleted(bool)), this, SLOT(onSecurityValidationComplete(bool)));

                mSecurityRequest = NULL;    // was only needed for disconnecting

                if (!valid || !LocalHostSecurity::isCurrentlyUnlocked(mCallingDomainName, mServiceSecurityBit))
                {
                    ResponseInfo responseInfo;

                    responseInfo.response = QString("{\"error\":\"NOT_AUTHORIZED\"}");
                    responseInfo.errorCode = HttpStatus::Forbidden;
                    sendResponse(responseInfo);

                    return;
                }

                onProcessRequest();
            }

            void HttpRequestHandler::onProcessRequest()
            {
                mIsAsychronous = mService->isAsynchronousService();
                if (mIsAsychronous)
                {
                    ORIGIN_VERIFY_CONNECT(mService, SIGNAL(processCompleted(ResponseInfo)), this, SLOT(onProcessCompleted(ResponseInfo)));
                }

                ResponseInfo responseInfo;
                responseInfo = mService->processRequest(mRequestUrl);

                if (!mIsAsychronous)    // if asynchronous, don't call this now as it will result in this handler being deleted
                    sendResponse(responseInfo);
            }

            void HttpRequestHandler::onProcessCompleted(ResponseInfo response)
            {
                ORIGIN_VERIFY_DISCONNECT(mService, SIGNAL(processCompleted(ResponseInfo)), this, SLOT(onProcessCompleted(ResponseInfo)));

                sendResponse(response);

                mRequestDataPos = mRawRequestData.pos();
                mRawRequestData.close();
            }
        }
    }
}
