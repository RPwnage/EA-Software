///////////////////////////////////////////////////////////////////////////////
// OriginAuthServiceClient.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _ORIGINAUTNSERVICECLIENT_H_INCLUDED_
#define _ORIGINAUTNSERVICECLIENT_H_INCLUDED_

#include "services/rest/OriginServiceClient.h"
#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API OriginAuthServiceClient : public OriginServiceClient
        {
        public:
            virtual ~OriginAuthServiceClient(void);

            ///
            /// \brief Requests fail immediately if the given session is currently forced offline,
            /// otherwise all are forwarded to the network access manager.
            ///
            AuthNetworkRequest headAuth(Session::SessionRef session, const QNetworkRequest &request) const;
            AuthNetworkRequest getAuth(Session::SessionRef session, const QNetworkRequest &request) const;
            AuthNetworkRequest postAuth(Session::SessionRef session, const QNetworkRequest &request, QIODevice *data) const;
            AuthNetworkRequest postAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &data) const;
            AuthNetworkRequest postAuth(Session::SessionRef session, const QNetworkRequest &request, QHttpMultiPart *multiPart) const;
            AuthNetworkRequest putAuth(Session::SessionRef session, const QNetworkRequest &request, QIODevice *data) const;
            AuthNetworkRequest putAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &data) const;
            AuthNetworkRequest putAuth(Session::SessionRef session, const QNetworkRequest &request, QHttpMultiPart *multiPart) const;
            AuthNetworkRequest deleteResourceAuth(Session::SessionRef session, const QNetworkRequest &request) const;
            AuthNetworkRequest sendCustomRequestAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &verb, QIODevice *data = 0) const;

            ///
            /// Make a get request regardless of current offline state, used for the single login check 
            /// which may happen as part of going online. 
            ///
            /// This function should be used only for VERY SPECIAL CASES!!!
            ///
            AuthNetworkRequest getAuthOffline(Session::SessionRef session, const QNetworkRequest &request) const;

        protected:
            explicit OriginAuthServiceClient(NetworkAccessManager *nam = NULL);

        private:
            /// \brief Disables non-utilized members to avoid default generation by the compiler.
            explicit OriginAuthServiceClient(const OriginAuthServiceClient&);
            OriginAuthServiceClient& operator=(const OriginAuthServiceClient&);
        };
    }
}

#endif

