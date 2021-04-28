///////////////////////////////////////////////////////////////////////////////
// IdentityPortalServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _IDENTITYPORTALSERVICECLIENT_H_INCLUDED_
#define _IDENTITYPORTALSERVICECLIENT_H_INCLUDED_

#include "services/rest/IdentityPortalServiceResponses.h"
#include "services/plugin/PluginAPI.h"
#include "OriginServiceClient.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API IdentityPortalServiceClient :	public OriginServiceClient
        {

        public:
            friend class OriginClientFactory<IdentityPortalServiceClient>;
          
            static IdentityPortalUserResponse* retrieveUserInfoByToken(const QString& accessToken);
            static IdentityPortalUserResponse* retrieveUserInfoByPid(const QString& accessToken, qint64 userPid);

            static IdentityPortalUserProfileUrlsResponse* retrieveUserProfileUrls(const QString& accessToken, qint64 userPid,
                IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category = IdentityPortalUserProfileUrlsResponse::All);
            static IdentityPortalExpandedNameProfilesResponse* retrieveExpandedNameProfiles(const QString& accessToken, qint64 userPid);
            static IdentityPortalUserNameResponse* retrieveUserName(const QString& servicePath, const QString& accessToken);
            static IdentityPortalUserProfileUrlsResponse* updateUserName(const QString& servicePath, const QString& accessToken,
                const QString& firstName, const QString& lastName);

            static IdentityPortalUserIdResponse* pidByEmail(const QString& accessToken, const QString& email);
            static IdentityPortalUserIdResponse* pidByEAID(const QString&  accessToken, const QString& eaid);

            /// \brief returns EAID and other info for user
            /// \brief based on https://gateway.int.ea.com/swagger/ui/?baseUrl=https://gateway.int.ea.com/proxy/api-docs/index.json#!/personas
            /// \brief static wrapper for the member function
            static IdentityPortalPersonasResponse* personas(const QString& accessToken, qint64 userPid);
            static IdentityPortalPersonasResponse2* personas2(const QString& accessToken, const QString& servicePath);
            static IdentityPortalExpandedPersonasResponse* expandedPersonas(const QString& accessToken, qint64 userPid);
            static IdentityPortalPersonasResponse* personasByEAID(const QString& accessToken, const QString& eaId);


        private:

            QString mClientId;
            QString mClientSecret;


            explicit IdentityPortalServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            IdentityPortalUserResponse* retrieveUserInfoByTokenPriv(const QString& servicePath, const QString& accessToken);

            IdentityPortalUserProfileUrlsResponse* retrieveUserProfileUrlsPriv(const QString& servicePath, const QString& accessToken,
                IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category);

            IdentityPortalPersonasResponse* personasPriv(const QString& servicePath, const QString& accessToken);
            IdentityPortalPersonasResponse2* personasPriv2(const QString& servicePath, const QString& accessToken);
            IdentityPortalExpandedPersonasResponse* expandedPersonasPriv(const QString& servicePath, const QString& accessToken);
            IdentityPortalPersonasResponse* personasByEAIDPriv(const QString& servicePath, const QString& accessToken, const QString& eaId);

            IdentityPortalUserNameResponse* retrieveUserNamePriv(const QString& servicePath, const QString& accessToken);
            IdentityPortalExpandedNameProfilesResponse* retrieveExpandedNameProfilesPriv(const QString& servicePath, const QString& accessToken);
            IdentityPortalUserProfileUrlsResponse* updateUserNamePriv(const QString& servicePath, const QString& accessToken,
                const QString& firstName, const QString& lastName);

            IdentityPortalUserIdResponse* pidByEmailPriv(const QString& servicePath, const QString& accessToken, const QString& email);
            IdentityPortalUserIdResponse* pidByEAIDPriv(const QString& servicePath, const QString& accessToken, const QString& eaid);

            /// \fn setHeader
            /// \brief helper to set the access token for HTTP GET requests headers.
            void setHeader( const QString& accessToken, QNetworkRequest &request );

            QString profileInfoCategoryToString(IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory category) const;

        };
    }
}

#endif

