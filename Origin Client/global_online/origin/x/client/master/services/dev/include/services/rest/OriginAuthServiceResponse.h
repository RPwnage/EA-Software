///////////////////////////////////////////////////////////////////////////////
// OriginAuthServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _OriginServiceAuthResponse_H
#define _OriginServiceAuthResponse_H

#include "OriginServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// This response class is initialized with the current session at the time
        /// of the request. If the session is no longer valid at the time the reply
        /// comes back, the reply reports a failure. The caller is still responsible
        /// for deleting the response object. If the caller no longer exists (because
        /// the user is logged out), then the response will likely leak.
        class ORIGIN_PLUGIN_API OriginAuthServiceResponse : public OriginServiceResponse
        {
            Q_OBJECT
        public:
            ///
            /// Creates an authenticated service response. The request contains a smart
            /// pointer, and is therefore be passed by value.
            ///
            OriginAuthServiceResponse(AuthNetworkRequest reply);

            Session::SessionWRef userSession() { return mSession; }

        protected:	

            ///
            /// Handles the completion of the QNetworkReply
            ///
            /// If the session is no longer valid, the reply is never
            /// parsed, the signals error() and finished() are emitted.
            ///
            virtual void processReply();

        private:
            Session::SessionWRef mSession;
        };
    }
}

#endif