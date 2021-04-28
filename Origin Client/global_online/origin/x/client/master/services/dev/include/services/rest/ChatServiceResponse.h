///////////////////////////////////////////////////////////////////////////////
// ChatServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _CHATERVICERESPONSE_H
#define _CHATERVICERESPONSE_H

#include "OriginAuthServiceResponse.h"
#include "services/session/AbstractSession.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// Encapsulate responses for if a user is online
        ///
        class ORIGIN_PLUGIN_API UserOnlineResponse : public OriginAuthServiceResponse
        {
        public:
            UserOnlineResponse(AuthNetworkRequest);

            ///
            /// Returns if the user is online
            ///
            bool userOnline() const { return m_userOnline; }

        protected:
            bool m_userOnline;

            void processReply();
        };
    }
}

#endif //_CHATERVICERESPONSE_H
