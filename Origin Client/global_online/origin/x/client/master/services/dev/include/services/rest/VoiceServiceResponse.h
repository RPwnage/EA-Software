///////////////////////////////////////////////////////////////////////////////
// VoiceServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _VOICE_SERVICE_RESPONSE_H
#define _VOICE_SERVICE_RESPONSE_H

#include <QDateTime>

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"

namespace Origin
{
    namespace Services
    {
        class ChannelExistsResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit ChannelExistsResponse(AuthNetworkRequest);

            bool channelExists() const
            {
                return mExists;
            }

        protected:
            void processReply();

            bool mExists;
            int mErrorCode;
            QString mErrorMessage;
        };

        class CreateChannelResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit CreateChannelResponse(AuthNetworkRequest);

            const QString& getChannelId() const
            {
                return mChannelId;
            }

        protected:
            void processReply();

            QString mChannelId;
            int mErrorCode;
            QString mErrorMessage;
        };

        class AddChannelUserResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit AddChannelUserResponse(AuthNetworkRequest);

            const QString& getChannelId() const
            {
                return mChannelId;
            }

        protected:
            void processReply();

            QString mChannelId;
            int mErrorCode;
            QString mErrorMessage;
        };

        class GetChannelTokenResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GetChannelTokenResponse(AuthNetworkRequest);

            const QString& getToken() const
            {
                return mToken;
            }

        protected:
            void processReply();

            QString mToken;
            int mErrorCode;
            QString mErrorMessage;
        };
    }
}

#endif //_VOICE_SERVICE_RESPONSE_H


