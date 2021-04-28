///////////////////////////////////////////////////////////////////////////////
// OriginServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ORIGINSERVICERESPONSE_H
#define _ORIGINSERVICERESPONSE_H

#include "HttpServiceResponse.h"
#include "OriginServiceValues.h"
#include "HttpStatusCodes.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct Nuke2Error
        {
            restError mRestError;
            QString mErrorDescription;
            QString mError;
            Nuke2Error() : mRestError(restErrorUnknown), mErrorDescription("Unknown Nuke2 error"), mError("error" )
            {
                if (0==Nuke2Error::mMap.size())
                {
                    Nuke2Error::mMap["invalid_request"] = restErrorNucleus2InvalidRequest;
                    Nuke2Error::mMap["invalid_client"] = restErrorNucleus2InvalidClient;
                    Nuke2Error::mMap["invalid_grant"] = restErrorNucleus2InvalidGrant;
                    Nuke2Error::mMap["unauthorized_client"] = restErrorNucleus2UnauthorizedClient;
                    Nuke2Error::mMap["unsupported_grant_type"] = restErrorNucleus2UnsupportedGrantType;
                    Nuke2Error::mMap["invalid_scope"] = restErrorNucleus2InvalidScope;
                    Nuke2Error::mMap["invalid_token"] = restErrorNucleus2InvalidToken;
                }
            }
            void clear() {mErrorDescription.clear();mError.clear();mRestError=restErrorUnknown;}
            static QMap<QString, restError> mMap;
        };
        using namespace ApiValues;
        ///
        /// Base class for responses from Global Common Services
        ///
        /// This handles parsing of the common error format from GCS responses. This 
        /// class can be used directly for services that return no content on success
        ///
        class ORIGIN_PLUGIN_API OriginServiceResponse : public HttpServiceResponse
        {
            Q_OBJECT
        public:
            ///
            /// Creates a GCS service response
            ///
            OriginServiceResponse(QNetworkReply *reply);

            virtual ~OriginServiceResponse();

            ///
            /// Returns the GCS error code associated with the response
            ///
            /// Only valid after the request has finished
            ///
            restError error() const { return m_error; }

            ///
            /// Returns the GCS error code associated with the response
            ///
            /// Only valid after the request has finished
            ///
            HttpStatusCode httpStatus() const { return m_httpStatus; }

            /// \brief returns last error string
            QString errorString() const { return mErrorString; }

            /// \brief returns nuke 2 error struct
            Nuke2Error nuke2Error() const;

signals:
            ///
            /// Emitted when the request has successfully completed
            ///
            void success();

            ///
            /// Emitted when the request has failed
            ///
            void error(Origin::Services::restError code);

            ///
            /// Emitted when the request has failed
            ///
            void error(Origin::Services::HttpStatusCode code);

        protected:  

            ///
            /// Attempts to extract a ORS-style error code from a reply body
            ///
            /// \param body  IO device contains the error response
            /// \return Error code or restUnknownError on parse failure
            ///
            restError extractErrorCode(QIODevice *body);

            ///
            /// Called to parse a successful response body
            ///
            /// Called by the default implementation of processReply()
            ///
            /// \param body  IO device containing the content to parse
            /// \return True if parse was successful, false if not
            ///
            virtual bool parseSuccessBody(QIODevice *body);

            ///
            /// Handles the completion of the QNetworkReply
            ///
            /// Calls parseSuccessBody for 200 responses and extractGcsErrorCode for
            /// 500 responses. success() is emitted if parseSuccessBody() returns true
            ///
            /// The default implementation should be appropriate for the vast
            /// majority of GCS services
            ///
            virtual void processReply();

            ///
            /// Simple method that reports the given information to telemetry. Child
            /// classes may use this method to customize their telemetry reporting.
            ///
            virtual void reportTelemetry(
                const restError errorCode, 
                const QNetworkReply::NetworkError qtNetworkError, 
                const HttpStatusCode httpStatus, 
                const char* printableUrl, 
                const char* replyErrorString) const;

            ///
            /// Sets the error code for the request
            ///
            /// \param errorCode ORS Error code 
            /// 
            void setError(restError errorCode) { m_error = errorCode; }

            ///
            /// Sets the error code for the request
            ///
            /// \param statusCode  Http status code
            ///
            void setHttpStatus(HttpStatusCode statusCode) { m_httpStatus = statusCode; }

            /// \fn decodeJSONError
            /// \brief decodes Nucleus 2 JSON errors
            /// \param ba payload to decode
            void decodeJSONError(const QByteArray& ba);

            bool mFireAuthenticationErrorSignal;         //keeps track of whether to emit a signal to indicate error was due to authentication

        private:
            restError m_error;
            HttpStatusCode m_httpStatus;

            /// stores the last error string
            QString mErrorString;

            Nuke2Error mNuke2Error;
        };

        //////////////////////////////////////////////////////////////////////////
        ///
        /// \brief RestAuthenticationNotifier : public QObject
        ///
        /// This class provides authentication failure notifications from failed
        /// REST calls to whoever cares, usually the session that maintains
        /// authentication integrity.
        ///

        class ORIGIN_PLUGIN_API RestAuthenticationFailureNotifier : public QObject
        {
            Q_OBJECT

        public:

            static void init();
            static void release();

            static RestAuthenticationFailureNotifier* instance();

            static void fireAuthenticationError(Origin::Services::restError code)
            {
                emit instance()->authenticationError(code);
            }

        signals:

            ///
            /// Emitted when an authentication error has occurred.
            ///
            void authenticationError(Origin::Services::restError code);

        private:

            RestAuthenticationFailureNotifier();
            RestAuthenticationFailureNotifier(const RestAuthenticationFailureNotifier&);
            const RestAuthenticationFailureNotifier& operator=(const RestAuthenticationFailureNotifier&);
        };
    }
}

#endif