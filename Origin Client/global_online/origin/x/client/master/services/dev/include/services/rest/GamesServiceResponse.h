///////////////////////////////////////////////////////////////////////////////
// GamesServicedResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _GAMESERVICERESPONSE_H_INCLUDED_
#define _GAMESERVICERESPONSE_H_INCLUDED_

#include <limits>
#include <QDateTime>
#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"
#include <QDomDocument>
#include <QList>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct CommonGame
        {
            // The content ID / product ID of the game in common
            QString masterTitleId;
            QString multiPlayerId;

            inline bool operator==(const CommonGame& lhs) const
            {
                return (masterTitleId == lhs.masterTitleId) && (multiPlayerId == lhs.multiPlayerId);
            }
        };
        
        inline uint qHash(const Origin::Services::CommonGame &game)
        {
            return qHash(game.masterTitleId) ^ qHash(game.multiPlayerId);
        }

        struct FriendCommonGames
        {
            /// User Identifier of the friend
            quint64 userId;
            // List of games that are in common
            QList<CommonGame>   games;
        };

        struct FriendsCommonGames
        {
            //////////////////////////////////////////////////////////////////////////
            /// List of users that own common games
            ///
            QList<FriendCommonGames> friends;
        };

        struct GamePlayedStats
		{
			QString gameId;
            QString multiplayerId;
			quint64 total;
			quint64 lastSession;
			QDateTime lastSessionEndTimeStamp;

			bool isValid() const { return !gameId.isEmpty(); }
			void reset() { gameId = ""; multiplayerId = ""; total = lastSession =  0; lastSessionEndTimeStamp = QDateTime();}
		};

        class ORIGIN_PLUGIN_API GameStartedResponse : public OriginAuthServiceResponse
        {
            public:

			    /// Sample response
			    /*
			
				    <?xml version="1.0" encoding="UTF-8" ?>
					    <usage>
					    <gameId>4343</gameId>
					    <MultiplayerId>4545</MultiplayerId>
					    <total>0</total>
					    <lastSession>0</lastSession>
					    <lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				    </usage>

			    */

                /// CTOR
                explicit GameStartedResponse(AuthNetworkRequest);
			    const GamePlayedStats& gameStats() const {return mStats;}

            protected:


                bool parseSuccessBody(QIODevice* body);

			    GamePlayedStats mStats;
        };

        class ORIGIN_PLUGIN_API GameFinishedResponse : public OriginAuthServiceResponse
        {
        public:

			/// Sample response
			/*
			
				<?xml version="1.0" encoding="UTF-8" ?>
					<usage>
					<gameId>4343</gameId>
					<MultiplayerId>4545</MultiplayerId>
					<total>0</total>
					<lastSession>0</lastSession>
					<lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				</usage>

			*/

            /// CTOR
            explicit GameFinishedResponse(AuthNetworkRequest);
			const GamePlayedStats& gameStats() const {return mStats;}

        protected:


            bool parseSuccessBody(QIODevice* body);

			GamePlayedStats mStats;

        };

        class ORIGIN_PLUGIN_API GameUsageResponse : public OriginAuthServiceResponse
        {
        public:

			/// Sample response
			/*
			
				<?xml version="1.0" encoding="UTF-8" ?>
					<usage>
					<gameId>4343</gameId>
					<MultiplayerId>4545</MultiplayerId>
					<total>0</total>
					<lastSession>0</lastSession>
					<lastSessionEndTimeStamp>0</lastSessionEndTimeStamp>
				</usage>

			*/

            /// CTOR
            explicit GameUsageResponse(AuthNetworkRequest);
			const GamePlayedStats& gameStats() const {return mStats;}

        protected:


            bool parseSuccessBody(QIODevice* body);

			GamePlayedStats mStats;

        };

        class ORIGIN_PLUGIN_API GamesServiceFriendResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit GamesServiceFriendResponse(AuthNetworkRequest);
            const FriendsCommonGames & getFriendGames() const { return m_commonGames;}

        protected:
            Origin::Services::FriendsCommonGames m_commonGames;

            bool parseSuccessBody(QIODevice *body);
            bool parseGames(QDomElement const &gamesElement, QList<CommonGame> &games); 
            bool parseUser (QDomElement const &userElement, QList<FriendCommonGames> &friends);
            bool parseUsers(QDomElement const &users);
        };
    }
}

#endif
