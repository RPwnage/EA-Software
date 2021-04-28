///////////////////////////////////////////////////////////////////////////////
// NonOriginGameServiceClient.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _NONORIGINGAMESERVICECLIENT_H_INCLUDED_
#define _NONORIGINGAMESERVICECLIENT_H_INCLUDED_

#include "OriginAuthServiceClient.h"
#include "NonOriginGameServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API NonOriginGameServiceClient : public OriginAuthServiceClient
        {
            public:
                friend class OriginClientFactory<NonOriginGameServiceClient>;

                static AppStartedResponse* playing(Session::SessionRef session, const QString& appId);
                static AppFinishedResponse* finishedPlaying(Session::SessionRef session, const QString& appId);
                static NonOriginGameFriendResponse* friendsWithSameContent(Session::SessionRef session, const QString& appId);
                static AppUsageResponse* usageTime(Session::SessionRef session, const QString& appId);

            private:
                explicit NonOriginGameServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

                AppStartedResponse* playingPrivate(Session::SessionRef session, const QString& appId);
                AppFinishedResponse* finishedPlayingPrivate(Session::SessionRef session, const QString& appId);
                NonOriginGameFriendResponse* friendsWithSameContentPrivate(Session::SessionRef session, const QString& appId);
                AppUsageResponse* usageTimePrivate(Session::SessionRef session, const QString& appId);
            
        };
    }
}


#endif

