///////////////////////////////////////////////////////////////////////////////
// OriginCatalogPublicPrivateResponse.cpp
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/OriginCatalogPublicPrivateResponse.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Services
    {
        using namespace ApiValues;

        OriginCatalogPublicPrivateResponse::OriginCatalogPublicPrivateResponse(AuthNetworkRequest reply) 
            : OriginAuthServiceResponse(reply)
        {
        }

        void OriginCatalogPublicPrivateResponse::reportTelemetry(
            const restError errorCode, 
            const QNetworkReply::NetworkError qtNetworkError, 
            const HttpStatusCode httpStatus, 
            const char* printableUrl, 
            const char* replyErrorString) const
        {
            // We expect 200/400/403 for /private and /public, so don't
            // report this to telemetry
            const bool reportError = 
                httpStatus != Http200Ok && 
                httpStatus != Http403ClientErrorForbidden && 
                httpStatus != Http404ClientErrorNotFound;

            if (reportError) 
            {
                OriginServiceResponse::reportTelemetry(errorCode, qtNetworkError, httpStatus, printableUrl, replyErrorString);
            }
        }
    }
}
