///////////////////////////////////////////////////////////////////////////////
// OriginAuthServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include <QTextStream>
#include <QNetworkReply>
#include "services/log/LogService.h"
#include "services/rest/OriginAuthServiceResponse.h"
#include "TelemetryAPIDLL.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "services/session/SessionService.h"

namespace Origin
{
    namespace Services
    {
        using namespace ApiValues;

        OriginAuthServiceResponse::OriginAuthServiceResponse(AuthNetworkRequest reply) 
            : OriginServiceResponse(reply.first), mSession(reply.second)
        {
        }

        void OriginAuthServiceResponse::processReply()
        {
            if ( Session::SessionService::isValidSession(mSession.toStrongRef()) )
            {
                OriginServiceResponse::processReply();
            }
            else
            {
                setError(restErrorSessionExpired);
                emit error(restErrorSessionExpired);
            }
        }
    }
}
