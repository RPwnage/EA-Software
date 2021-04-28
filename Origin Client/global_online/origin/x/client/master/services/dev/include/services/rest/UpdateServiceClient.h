///////////////////////////////////////////////////////////////////////////////
// UpdateServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef UPDATESERVICECLIENT_H
#define UPDATESERVICECLIENT_H

#include "OriginServiceClient.h"
#include "UpdateServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API UpdateServiceClient : public OriginServiceClient
        {
        public:
            friend class OriginClientFactory<UpdateServiceClient>;

            ///
            /// \brief Checks if an update is available
            ///
            static UpdateQueryResponse* isUpdateAvailable(QString locale, QString type, QString platform)
            {
                return OriginClientFactory<UpdateServiceClient>::instance()->isUpdateAvailablePriv(locale, type, platform);
            }

        private:
            /// 
            /// \brief Creates a new chat service client
            ///
            /// \param nam           QNetworkAccessManager instance to send requests through.
            ///
            explicit UpdateServiceClient(NetworkAccessManager *nam = NULL);

            ///
            /// \brief Checks if an update is available
            ///
            UpdateQueryResponse* isUpdateAvailablePriv(QString locale, QString type, QString platform);
        };
    }
}

#endif