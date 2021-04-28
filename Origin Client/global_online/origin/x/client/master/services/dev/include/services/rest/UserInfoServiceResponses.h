///////////////////////////////////////////////////////////////////////////////
// UserInfoServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _USERINFOSERVICERESPONSES_H_INCLUDED_
#define _USERINFOSERVICERESPONSES_H_INCLUDED_

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"
#include "AuthenticationData.h"

#include <QDomDocument>
#include <QList>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API UserInfoResponse : public OriginAuthServiceResponse
        {
            public:
                explicit UserInfoResponse(AuthNetworkRequest reply);

                User getUserInfo();

            protected:

                Origin::Services::User parseInfo(const QDomElement& userInfoElement);
                bool parseSuccessBody(QIODevice* body);

                User mUser;
        };
    }
}

#endif


