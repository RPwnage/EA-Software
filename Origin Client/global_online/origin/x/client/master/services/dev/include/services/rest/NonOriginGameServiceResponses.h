///////////////////////////////////////////////////////////////////////////////
// NonOriginGameServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _NONORIGINGAMESERVICERESPONSES_H_INCLUDED_
#define _NONORIGINGAMESERVICERESPONSES_H_INCLUDED_

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"

#include <QDomDocument>
#include <QList>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct AppUsedStats
		{
			QString appId;
			quint64 total;
			quint64 lastSession;
			QDateTime lastSessionEndTimeStamp;

			bool isValid() const { return !appId.isEmpty(); }
			void reset() { appId = ""; total = lastSession = 0; lastSessionEndTimeStamp = QDateTime(); }
		};

        class ORIGIN_PLUGIN_API NonOriginGameFriendResponse : public OriginAuthServiceResponse
        {
            public:
                explicit NonOriginGameFriendResponse(AuthNetworkRequest reply, const QString& appId);
                const QList<quint64>& friendIds() const { return mCommonUsers; }
                QString appId() const { return mAppId; }

            protected:
                QList<quint64> mCommonUsers;
                QString mAppId;

                bool parseSuccessBody(QIODevice* body);
                bool parseUser (const QDomElement& userElement, QList<quint64>& friends);
                bool parseUsers(const QDomElement& users);
        };

        class ORIGIN_PLUGIN_API NonOriginGameGetFriendsResponse : public OriginServiceResponse
        {
            Q_OBJECT
            public:
                /// \brief The NonOriginGameGetFriendsResponse constructor.
                /// \param reply Object containing the server's response.
                explicit NonOriginGameGetFriendsResponse(QNetworkReply * reply);
            
            protected:
                /// \brief Parses a successful response body
                /// \param body IO device containing the content to parse
                /// \return True if parse was successful, false if not
                bool parseSuccessBody(QIODevice *body);
            
        };


        class ORIGIN_PLUGIN_API AppStartedResponse : public OriginAuthServiceResponse
        {
            public:

			    /// Sample response
			    /*
			
				    <?xml version="1.0" encoding="UTF-8" ?>
					    <usage>
					    <gameId>4343</gameId>
					    <total>0</total>
					    <lastSession>0</lastSession>
					    <lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				    </usage>

			    */
			
                /// CTOR
                explicit AppStartedResponse(AuthNetworkRequest);
			    const AppUsedStats& appStats() const {return mStats;}

            protected:


                bool parseSuccessBody(QIODevice* body);

			    AppUsedStats mStats;

        };

		// Responses for setting game / app status
        class ORIGIN_PLUGIN_API AppFinishedResponse : public OriginAuthServiceResponse
        {
            public:

			    /// Sample response
			    /*
			
				    <?xml version="1.0" encoding="UTF-8" ?>
					    <usage>
					    <gameId>4343</gameId>
					    <total>0</total>
					    <lastSession>0</lastSession>
					    <lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				    </usage>

			    */

                /// CTOR
                explicit AppFinishedResponse(AuthNetworkRequest);
			    const AppUsedStats& appStats() const {return mStats;}

            protected:


                bool parseSuccessBody(QIODevice* body);

			    AppUsedStats mStats;

        };


		// Responses for setting game / app status
        class ORIGIN_PLUGIN_API AppUsageResponse : public OriginAuthServiceResponse
        {
            public:

			    /// Sample response
			    /*
			
				    <?xml version="1.0" encoding="UTF-8" ?>
					    <usage>
					    <gameId>4343</gameId>
					    <total>0</total>
					    <lastSession>0</lastSession>
					    <lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				    </usage>
			    */
			
                /// CTOR
                explicit AppUsageResponse(AuthNetworkRequest);
			    const AppUsedStats& appStats() const {return mStats;}

            protected:


                bool parseSuccessBody(QIODevice* body);

			    AppUsedStats mStats;

        };

    }
}

#endif


