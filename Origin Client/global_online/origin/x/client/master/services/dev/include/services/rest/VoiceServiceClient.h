///////////////////////////////////////////////////////////////////////////////
// VoiceServiceClient.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _VOICE_SERVICE_CLIENT_H
#define _VOICE_SERVICE_CLIENT_H

#include "OriginAuthServiceClient.h"
#include "VoiceServiceResponse.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// \brief HTTP service client for the Voice server
        ///
        class VoiceServiceClient : public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<VoiceServiceClient>;

            static ChannelExistsResponse* channelExists(Session::SessionRef session, const QString& operatorId, const QString& channelId)
            {
                return OriginClientFactory<VoiceServiceClient>::instance()->channelExistsPriv(session, operatorId, channelId);
            }

            static CreateChannelResponse* createChannel(Session::SessionRef session, const QString& operatorId, const QString& channelId, const QString& channelName)
            {
                return OriginClientFactory<VoiceServiceClient>::instance()->createChannelPriv(session, operatorId, channelId, channelName);
            }

            static AddChannelUserResponse* addUserToChannel(Session::SessionRef session, const QString& operatorId, const QString& channelId)
            {
                return OriginClientFactory<VoiceServiceClient>::instance()->addUserToChannelPriv(session, operatorId, channelId);
            }

            static GetChannelTokenResponse* getChannelToken(Session::SessionRef session, const QString& operatorId, const QString& channelId)
            {
                return OriginClientFactory<VoiceServiceClient>::instance()->getChannelTokenPriv(session, operatorId, channelId);
            }

        private:
            /// 
            /// \brief Creates a new voice service client
            ///
            /// \param baseUrl       Base URL for the friends service API.
            /// \param nam           QNetworkAccessManager instance to send requests through.
            ///
            explicit VoiceServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager* nam = NULL);

            ///
            /// \brief Checks if a channel exists
            ///
            ChannelExistsResponse* channelExistsPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId);

            ///
            /// \brief Creates a voice channel
            ///
            CreateChannelResponse* createChannelPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId, const QString& channelName);
            
            ///
            /// \brief Adds a user to a voice channel
            ///
            AddChannelUserResponse* addUserToChannelPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId);
           
            ///
            /// \brief Gets a voice channel token
            ///
            GetChannelTokenResponse* getChannelTokenPriv(Session::SessionRef session, const QString& operatorId, const QString& channelId);
        };
    }
}

#endif //_VOICE_SERVICE_CLIENT_H