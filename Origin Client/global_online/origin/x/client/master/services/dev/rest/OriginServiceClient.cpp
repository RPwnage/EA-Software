///////////////////////////////////////////////////////////////////////////////
// OriginServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/OriginServiceClient.h"
#include <QNetworkReply>
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {

        OriginServiceClient::OriginServiceClient(NetworkAccessManager *nam)
            : HttpServiceClient(nam)
        {
        }

        OriginServiceClient::~OriginServiceClient(void)
        {
        }

        void OriginServiceClient::debugRequest( const QNetworkRequest& request, QByteArray data /*= QByteArray()*/ )
        {
#ifdef _DEBUG
            ORIGIN_LOG_DEBUG << "[information] Request: " << request.url().toString();
            const QList<QByteArray>& rawHeaderList(request.rawHeaderList());
            foreach (QByteArray rawHeader, rawHeaderList) {
                ORIGIN_LOG_DEBUG << "[information]\tRaw Header: " << request.rawHeader(rawHeader);
            }
            if (!data.isEmpty())
            {
                ORIGIN_LOG_DEBUG << "[information] Request data: " << data;
            }
#endif // _DEBUG
        }

        const QString escapedNucleusId(Session::SessionRef session)
        {
            return QUrl::toPercentEncoding(Session::SessionService::nucleusUser(session));
        }

        const QString nucleusId(Session::SessionRef session)
        {
            return Session::SessionService::nucleusUser(session);
        }

        const QString sessionToken( Session::SessionRef session)
        {
            return Session::SessionService::accessToken(session);
        }

        QByteArray boolStr(bool val) { return val? "true" : "false";}

    }
}
