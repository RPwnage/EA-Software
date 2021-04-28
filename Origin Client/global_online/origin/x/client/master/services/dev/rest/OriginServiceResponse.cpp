///////////////////////////////////////////////////////////////////////////////
// OriginServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include <QTextStream>
#include <QNetworkReply>
#include "services/log/LogService.h"
#include "services/rest/OriginServiceResponse.h"
#include "TelemetryAPIDLL.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include <NodeDocument.h>

namespace Origin
{
    namespace Services
    {
        QMap<QString, restError> Nuke2Error::mMap;

        using namespace ApiValues;

        OriginServiceResponse::OriginServiceResponse(QNetworkReply *reply): HttpServiceResponse(reply)
        {
            //this will trigger a signal to call renewAccessToken if a response comes back with an authentication-type error (invalid_token, etc.)
            mFireAuthenticationErrorSignal = true;
        }

        // Error returned by EADP
//         {
//             "error" : {
//                 "code" : "Access is denied by unauthorized user"
//             }
//         }


        void OriginServiceResponse::decodeJSONError(const QByteArray& ba)
        {
            mNuke2Error.clear();
            QScopedPointer<INodeDocument> json(CreateJsonDocument());

            if(json && json->Parse(ba.constData()))
            {
                if (Nuke2Error::mMap.contains(json->GetAttributeValue("error")))
                {
                    mNuke2Error.mRestError = Nuke2Error::mMap[json->GetAttributeValue("error")]; // Error
                    mNuke2Error.mError = json->GetAttributeValue("error");
                    mNuke2Error.mErrorDescription = json->GetAttributeValue("error_description"); // Error description
                }
                else
                {
                    if (json->FirstChild())
                    {
                        QString error = json->GetValue();
                        if (error.contains("NO_SUCH_USER_PRIVACY_SETTINGS",Qt::CaseInsensitive))
                        {
                            mNuke2Error.mRestError = restErrorAccountNotExist;
                            mNuke2Error.mError = "error";
                            mNuke2Error.mErrorDescription = error; // Error description
                        }
                    }

                    mNuke2Error.mErrorDescription = ba;
                    ORIGIN_LOG_WARNING << "OriginServiceResponse received UNKNOWN response from Nucleus 2: error description: [" << ba.constData() << "]";   
                }
            }
        }

        restError OriginServiceResponse::extractErrorCode(QIODevice *body)
        {
            QDomDocument doc;
            if (doc.setContent(body->peek(reply()->bytesAvailable())))
            {
                QDomElement docElement = doc.documentElement();

                if ((docElement.tagName() == "error") && docElement.hasAttribute("code"))
                {
                    // Attempt to extract additional error information from error XML
                    if(docElement.hasChildNodes())
                    {
                        QDomElement errorDetailsElement = docElement.firstChildElement("failure");
                        if(!errorDetailsElement.isNull() && errorDetailsElement.hasAttribute("cause"))
                        {
                            mErrorString = "Cause: " + errorDetailsElement.attribute("cause");
                        }
                    }

                    bool codeOkay = false;
                    int errorCode = docElement.attribute("code").toInt(&codeOkay);

                    // We reserve codes <= ourselves; don't pass them through and
                    // treat it like a service error
                    if (codeOkay && (errorCode > 0))
                    {
                        return static_cast<restError>(errorCode);
                    }
                }
            }
            else
            {
                decodeJSONError(body->peek(reply()->bytesAvailable()));
                if (mNuke2Error.mRestError != restErrorUnknown)
                {
                    ORIGIN_LOG_ERROR << "OriginServiceResponse received Nucleus 2 error code: [" << mNuke2Error.mRestError << "] error [" << mNuke2Error.mError << "] description: [" << mNuke2Error.mErrorDescription << "]";  
                    return mNuke2Error.mRestError;
                }
            }

            QNetworkReply::NetworkError error = reply()->error();
            if (error < QNetworkReply::UnknownNetworkError)
                return (static_cast <restError> (restErrorNetwork + -(error)));   //use restErrorNework as the base, add value of QNetworkReply::NetworkError and negate it
            else
                return restErrorUnknown;
        }

        bool OriginServiceResponse::parseSuccessBody(QIODevice* body)
        {
            return true;
        }

        void OriginServiceResponse::processReply()
        {
#ifdef _DEBUG
            // this is only here to allow us to easily inspect the reply in the debugger
            QString resp(QTextStream(reply()->peek(reply()->bytesAvailable())).readAll());
#endif

            QNetworkReply::NetworkError networkError = reply()->error();
            mErrorString = reply()->errorString();
            HttpStatusCode statusCode = static_cast<HttpStatusCode>(reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
            setHttpStatus(statusCode);

            if (networkError == QNetworkReply::NoError)
            {
                setError(restErrorSuccess);

                if (parseSuccessBody(reply()))
                {
                    // It worked!
                    emit success();
                }
                else
                {
                    // Can't parse the body
                    setError(restErrorUnknown);
                    emit error(restErrorUnknown);
                }
            }
            else
            {
                QUrl replyUrl(reply()->url());
                QString path = replyUrl.path();
                QString censoredPath = Logger::CensorUrl(path);
                replyUrl.setPath(censoredPath);
                QString printableUrl(replyUrl.toString());

#ifndef _DEBUG
                // Censor the string if in release build.
                int index = printableUrl.indexOf('?');
                if (index >= 0)
                    printableUrl = printableUrl.left(index);
#endif

                //  Censor the nucleus id's out of url
                Logger::AddStringToCensorList(printableUrl);               
                if( Connection::ConnectionStatesService::isGlobalStateOnline() )
                {
                    ORIGIN_LOG_ERROR << "OriginServiceResponse QNetworkReply error code [" << networkError << "] error string : [" << mErrorString << "] HTTP status code [" << statusCode << "] X-Origin-Ops [" << reply()->rawHeader("X-Origin-Ops") << "] " << printableUrl; 
                }
                else
                {
                    ORIGIN_LOG_EVENT << "global state: offline: " << printableUrl;
                }
 
                // search for Server error code
                restError errorCode = extractErrorCode(reply());
                reportTelemetry(errorCode, networkError, statusCode, printableUrl.toUtf8().constData(), reply()->errorString().toUtf8().constData());

                switch(errorCode)
                {
                    //nucleus 2 errors
                case restErrorNucleus2InvalidToken:

                    //nucleus 1 errors but leave them here just in case something remaps nucleus 2 erors to nucleus 1 errors
                case restErrorRequestAuthtokenInvalidLoginRegistration:
                case restErrorTokenExpired:
                case restErrorTokenInvalid:
                case restErrorTokenUseridDifferLoginRegistration:
                    {
                        if (mFireAuthenticationErrorSignal)
                            emit RestAuthenticationFailureNotifier::fireAuthenticationError(errorCode);
                    }
                    break;

                default:
                    break;
                }

                setError(errorCode);
                emit error(errorCode);

                // Get the HTTP status code
                emit error(statusCode);
            }
        }

        void OriginServiceResponse::reportTelemetry(
            const restError errorCode, 
            const QNetworkReply::NetworkError qtNetworkError, 
            const HttpStatusCode httpStatus, 
            const char* printableUrl, 
            const char* replyErrorString) const
        {
            ORIGIN_LOG_ERROR << "Failed with REST error code " << static_cast<qint32>(errorCode) << " error string: " << errorString();           
            GetTelemetryInterface()->Metric_ERROR_REST(errorCode, qtNetworkError, httpStatus, printableUrl, replyErrorString);
        }

        Origin::Services::Nuke2Error OriginServiceResponse::nuke2Error() const
        {
            return mNuke2Error;
        }

        OriginServiceResponse::~OriginServiceResponse()
        {

        }

        //////////////////////////////////////////////////////////////////////////
        ///
        /// \class RestAuthenticationNotifier
        ///
        /// \brief This class provides authentication failure notifications from failed
        /// REST calls to whoever cares, usually the session that maintains
        /// authentication integrity.
        ///

        static RestAuthenticationFailureNotifier* sAuthenticationNotifier = NULL;

        void RestAuthenticationFailureNotifier::init()
        {
            if (sAuthenticationNotifier == NULL)
            {
                sAuthenticationNotifier = new RestAuthenticationFailureNotifier();
                ORIGIN_ASSERT(sAuthenticationNotifier);
            }
        }

        void RestAuthenticationFailureNotifier::release()
        {
            if (sAuthenticationNotifier != NULL)
            {
                delete sAuthenticationNotifier;
                sAuthenticationNotifier = NULL;
            }
        }

        RestAuthenticationFailureNotifier* RestAuthenticationFailureNotifier::instance()
        {
            return sAuthenticationNotifier;
        }

        RestAuthenticationFailureNotifier::RestAuthenticationFailureNotifier()
        {
        }
    }

}
