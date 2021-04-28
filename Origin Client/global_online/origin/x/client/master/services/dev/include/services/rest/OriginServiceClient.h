///////////////////////////////////////////////////////////////////////////////
// OriginServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _ORIGINSERVICECLIENT_H_INCLUDED_
#define _ORIGINSERVICECLIENT_H_INCLUDED_

#include <QHash>
#include "services/rest/OriginClientFactory.h"
#include "HttpServiceClient.h"
#include "services/session/SessionService.h"

#include "services/plugin/PluginAPI.h"

class QNetworkRequest;
class QUrl;
namespace Origin
{
    namespace Services
    {

        class ORIGIN_PLUGIN_API OriginServiceClient :
            public HttpServiceClient
        {
        public:
            virtual ~OriginServiceClient(void) = 0;
        protected:
            ///
            /// \brief CTOR
            /// \param nam Network access manager.
            ///
            explicit OriginServiceClient(NetworkAccessManager *nam = NULL);

            /// \brief debugs request header and data
            void debugRequest(const QNetworkRequest& request, QByteArray data = QByteArray());

        private:
            /// \brief Disables non-utilized members to avoid default generation by the compiler.
            explicit OriginServiceClient(const OriginServiceClient&);
            OriginServiceClient& operator=(const OriginServiceClient&);
        };

        /// \brief Returns a QBA with "true" or "false".
        ///
        /// \param val A boolean to "convert" to a string.
        ORIGIN_PLUGIN_API QByteArray boolStr(bool val);

        /// \brief Gets the current user's escaped Nucleus Id.
        ///
        /// \param session An opaque session handler.
        /// \return QString A QString containing the escaped Nucleus Id.
        ORIGIN_PLUGIN_API const QString escapedNucleusId(Session::SessionRef );

        /// \brief Gets the current user's Nucleus Id.
        ///
        /// \param session An opaque session handler.
        /// \return QString The non-escaped Nucleus Id.
        ORIGIN_PLUGIN_API const QString nucleusId(Session::SessionRef );

        /// \brief Gets the auth token.
        ///
        /// \param session An opaque session handler.
        /// \return QString The session token.
        ORIGIN_PLUGIN_API const QString sessionToken(Session::SessionRef );

        // const strings to form URL parameters
        const static QString loginIdUrlParam("ea_login_id");
        const static QString passwordUrlParam("ea_password");
        const static QString rememberMeUrlParam("remember_me");
        const static QString encryptedTokenUrlParam("encryptedToken");
        const static QString authTokenUrlParam("authToken");
        const static QString authTokenExpUrlParam("authTokenExpiration");
        const static QString SSOTokenUrlParam("ssoToken");
        const static QString genericAuthTokenExpUrlParam("genericAuthTokenExpiration");
        const static QString machineHashUrlParam("machine_hash");
        const static QString contentTypeHeader("application/x-www-form-urlencoded");
        const static QString XOriginUIDHeader("X-Origin-UID");
        const static QString localeURLparam("locale");
        const static QString environmentURLparam("environment");
        const static QString urlURLparam("url");

    }
}



#include "PropertiesMap.h"


#endif

