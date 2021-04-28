///////////////////////////////////////////////////////////////////////////////
// SearchFriendServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SEARCHFRIENDSERVICECLIENT_H
#define _SEARCHFRIENDSERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "SearchFriendServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// HTTP service client for the friends component of the Friends GCS API
        ///
        class ORIGIN_PLUGIN_API SearchFriendServiceClient : public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<SearchFriendServiceClient>;

            ///
            /// \brief Searches friend 
            ///
            /// \param cookies The session's cookies.
            /// \param source The search keyword.
            /// \param pageNumber The current page number.
            ///
            static SearchFriendServiceResponse *searchFriend(const QByteArray& at, const QString &source, const QString& pageNumber = QString("1"))
            {
                return OriginClientFactory<SearchFriendServiceClient>::instance()->searchFriendPriv(at, source, pageNumber);
            }

        private:
            /// 
            /// \brief Creates a new friend service client
            ///
            /// \param baseUrl       Base URL for the friends service API
            /// \param nam           QNetworkAccessManager instance to send requests through
            ///
            explicit SearchFriendServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            ///
            /// \brief Searches friend (Private version).
            ///
            /// \param source The search keyword.
            /// \param pageNumber The current page number.
            ///
            SearchFriendServiceResponse *searchFriendPriv(const QByteArray& at,const QString &source, const QString& pageNumber);

        };
    }
}

#endif
