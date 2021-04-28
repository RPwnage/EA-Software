///////////////////////////////////////////////////////////////////////////////
// SignInPortalServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SIGNIN_PORTALSERVICERESPONSES_H_INCLUDED_
#define _SIGNIN_PORTALSERVICERESPONSES_H_INCLUDED_

#include "OriginServiceResponse.h"
//#include "OriginServiceMaps.h"

#include <QMap>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API SignInPortalEmailVerificationResponse  : public OriginServiceResponse
		{
		    public:
			    explicit SignInPortalEmailVerificationResponse(QNetworkReply *reply);
                bool requestSent() { return mSuccess; }
		    protected:
			    bool parseSuccessBody(QIODevice *body);
                bool mSuccess;

		};
    }
}

#endif


