///////////////////////////////////////////////////////////////////////////////
// UserInfoServiceClient.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _USERINFOSERVICECLIENT_H_INCLUDED_
#define _USERINFOSERVICECLIENT_H_INCLUDED_

#include "OriginAuthServiceClient.h"
#include "UserInfoServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API UserInfoServiceClient : public OriginAuthServiceClient
        {
            public:
                friend class OriginClientFactory<UserInfoServiceClient>;

                static UserInfoResponse* userInfo(Session::SessionRef session, const QString &pid);


            private:
                explicit UserInfoServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

                UserInfoResponse* userInfoPrivate(Session::SessionRef session, const QString &pid);
        };
    }
}


#endif

