///////////////////////////////////////////////////////////////////////////////
// SignInPortalServiceClient.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SIGNINPORTALSERVICECLIENT_H_INCLUDED_
#define _SIGNINPORTALSERVICECLIENT_H_INCLUDED_

#include "services/rest/SignInPortalServiceResponses.h"
#include "services/plugin/PluginAPI.h"
#include "OriginAuthServiceClient.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API SignInPortalServiceClient :	public OriginServiceClient
        {
            public:
                friend class OriginClientFactory<SignInPortalServiceClient>;
                static SignInPortalEmailVerificationResponse* sendEmailVerificationRequest(Session::SessionRef session, const QString &accessToken);


            private:
                explicit SignInPortalServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
                SignInPortalEmailVerificationResponse* sendEmailVerificationRequestPriv(Session::SessionRef session, const QString &accessToken);
        };
    }
}

#endif

