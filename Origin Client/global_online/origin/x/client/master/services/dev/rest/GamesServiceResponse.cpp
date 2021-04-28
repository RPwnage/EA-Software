///////////////////////////////////////////////////////////////////////////////
// GamesServicedResponse.cpp
// This code is responsible for parsing the response from the REST call.
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/GamesServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/common/XmlUtil.h"

namespace Origin
{
    namespace Services
    {
        Origin::Services::GamePlayedStats parseGameStats(QIODevice* body)
	    {
		    Origin::Services::GamePlayedStats gps;
		    QDomDocument doc;

		    if (!doc.setContent(body))
			    // Not valid XML
			    return gps;

		    if (doc.documentElement().nodeName() != "usage")
			    return gps;

		    QDomElement root = doc.documentElement();

		    // add the individual elements to the settings
		    // TBD: we really need to strengthen the error handling
		    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
		    {
			    QDomElement e = n.toElement(); // try to convert the node to an element.
			    if (!e.isNull())
			    {
				    if (e.tagName() == QString("gameId"))
					    gps.gameId = e.text();
				    else if (e.tagName() == QString("MultiplayerId"))
                        gps.multiplayerId = e.text();
				    else if (e.tagName() == QString("total"))
					    gps.total = e.text().toULongLong();
				    else if (e.tagName() == QString("lastSession"))
					    gps.lastSession = e.text().toULongLong();
				    else if (e.tagName() == QString("lastSessionEndTimeStamp"))
                    {
                        qint64 secs = e.text().toULongLong();
                        if (secs)
                        {
                            //if we have a valid time, lets figure out the timestamp
					        gps.lastSessionEndTimeStamp = QDateTime::fromMSecsSinceEpoch(secs);
                        }
                        else
                        {
                            //else set the time stamp to be invalid
                            gps.lastSessionEndTimeStamp = QDateTime();
                        }
                    }
			    }
		    }

		    return gps;
	    }

        GamesServiceFriendResponse::GamesServiceFriendResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
        }

        // A Game simply contains a content ID at the moment
        bool GamesServiceFriendResponse::parseGames(QDomElement const &gamesElement, QList<CommonGame> &games)
        {
            QDomNodeList rootChildren = gamesElement.childNodes();
            for(int i = 0; i < rootChildren.length(); i++)
            {
                CommonGame game;
                // Grab a game element from the list
                QDomElement gameElement = rootChildren.at(i).toElement();

                game.masterTitleId = Origin::Util::XmlUtil::getString(gameElement, "masterTitleId", "");
                game.multiPlayerId = Origin::Util::XmlUtil::getString(gameElement, "multiPlayerId", "");
                games.append(game);
            }
            return true;
        }

        // A user element contains the User ID and a list of Games.
        bool GamesServiceFriendResponse::parseUser(QDomElement const &userElement, QList<FriendCommonGames> &friends)
        {
            bool ret=false;
            FriendCommonGames fcg;

            QDomElement userIdElement = userElement.firstChildElement("userId");
            fcg.userId = Origin::Util::XmlUtil::getLongLong(userElement, "userId", 0);
            
            QDomElement gamesElement = userElement.firstChildElement("games");
            if ( !gamesElement.isNull() && (ret=parseGames( gamesElement, fcg.games ))==true )
            {
                friends.append(fcg);
            }

            return ret;
        }

        bool GamesServiceFriendResponse::parseUsers(QDomElement const &users)
        {
            bool ret=true;
            QDomNodeList rootChildren = users.childNodes();
            // Loop through each User element
            for(int i = 0; i < rootChildren.length() && ret; i++)
            {
                // Grab a user element from the list
                QDomElement userElement = rootChildren.at(i).toElement();

                if (!userElement.isNull() && 
                   (userElement.tagName() == "user"))
                {
                    ret = parseUser(userElement, m_commonGames.friends);
                }
            }
            return ret;
        }

        bool GamesServiceFriendResponse::parseSuccessBody(QIODevice* body )
        {
            bool ret=false;
            QDomDocument doc;

            if ( doc.setContent(body) &&
                 doc.documentElement().nodeName() == "users" )
            {
                // This function will parse each user 
                ret = parseUsers( doc.documentElement() );
            }

            return ret;
        }


        GameStartedResponse::GameStartedResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

		bool GameStartedResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseGameStats(body);

			return mStats.isValid();
		}

		GameFinishedResponse::GameFinishedResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

		bool GameFinishedResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseGameStats(body);

			return mStats.isValid();
		}

		GameUsageResponse::GameUsageResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

		bool GameUsageResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseGameStats(body);

			return mStats.isValid();
		}
    }
}
