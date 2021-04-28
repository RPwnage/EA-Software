///////////////////////////////////////////////////////////////////////////////
// OriginCatalogPublicPrivateResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _OriginCatalogPublicPrivateResponse_H
#define _OriginCatalogPublicPrivateResponse_H

#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// Behaves exactly like OriginAuthServiceResponse, except with some
        /// customized telemetry reporting behavior to handle reporting of
        /// non-200 responses from the catalog CDN for /public and /private
        /// calls.
        class ORIGIN_PLUGIN_API OriginCatalogPublicPrivateResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            ///
            /// Creates an authenticated service response. The request contains a smart
            /// pointer, and is therefore be passed by value.
            ///
            OriginCatalogPublicPrivateResponse(AuthNetworkRequest reply);

        protected:	

            ///
            /// For public and private requests, 404 and 403 are expected 
            /// error responses from the CDN and shouldn't be reported to
            /// telemetry.
            ///
            virtual void reportTelemetry(
                const restError errorCode, 
                const QNetworkReply::NetworkError qtNetworkError, 
                const HttpStatusCode httpStatus, 
                const char* printableUrl, 
                const char* replyErrorString) const;
        };
    }
}

#endif