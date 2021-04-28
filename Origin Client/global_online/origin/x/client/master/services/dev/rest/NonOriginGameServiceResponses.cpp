///////////////////////////////////////////////////////////////////////////////
// NonOriginGameServiceResponses.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QDomDocument>
#include "services/rest/NonOriginGameServiceResponses.h"

namespace Origin
{
    namespace Services
    {

        Origin::Services::AppUsedStats parseAppStats(QIODevice* body)
	    {
		    Origin::Services::AppUsedStats aus;
		    QDomDocument doc;

		    if (!doc.setContent(body))
			    // Not valid XML
			    return aus;

		    if (doc.documentElement().nodeName() != "usage")
			    return aus;

		    QDomElement root = doc.documentElement();

		    // add the individual elements to the settings
		    // TBD: we really need to strengthen the error handling
		    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
		    {
			    QDomElement e = n.toElement(); // try to convert the node to an element.
			    if (!e.isNull())
			    {
				    if (e.tagName() == QString("appId"))
					    aus.appId = e.text();
				    else if (e.tagName() == QString("total"))
					    aus.total = e.text().toULongLong();
				    else if (e.tagName() == QString("lastSession"))
					    aus.lastSession = e.text().toULongLong();
				    else if (e.tagName() == QString("lastSessionEndTimeStamp"))
                    {
                        qint64 sec = e.text().toULongLong();
                        if (sec)
                        {
                            aus.lastSessionEndTimeStamp = QDateTime::fromMSecsSinceEpoch(sec);
                        }
                        else
                        {
                            aus.lastSessionEndTimeStamp = QDateTime();
                        }
                    }
			    }
		    }

		    return aus;
	    }

        NonOriginGameFriendResponse::NonOriginGameFriendResponse(AuthNetworkRequest reply, const QString& appId)
            : OriginAuthServiceResponse(reply)
            ,mAppId(appId)
        {

        }

        // A user element contains the User ID and a list of Games.
        bool NonOriginGameFriendResponse::parseUser(const QDomElement& userElement, QList<quint64>& friends)
        {
            bool ret = false;

            QDomElement userIdElement = userElement.firstChildElement("userId");
            if (!userIdElement.isNull())
            {
                friends.push_back(userIdElement.text().toULongLong(&ret));
            }
           
            return ret;
        }

        bool NonOriginGameFriendResponse::parseUsers(const QDomElement& users)
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
                    ret = parseUser(userElement, mCommonUsers);
                }
            }
            return ret;
        }

        bool NonOriginGameFriendResponse::parseSuccessBody(QIODevice* body )
        {
            bool ret=false;
            QDomDocument doc;

            if ( doc.setContent(body) &&
                 doc.documentElement().nodeName() == "users" )
            {
                ret = parseUsers(doc.documentElement());
            }

            return ret;
        }

        NonOriginGameGetFriendsResponse::NonOriginGameGetFriendsResponse(QNetworkReply * reply)
            : OriginServiceResponse(reply)
        {
        }

        bool NonOriginGameGetFriendsResponse::parseSuccessBody(QIODevice *body)
        {
            return false;
        }

        AppStartedResponse::AppStartedResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

		bool AppStartedResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseAppStats(body);

			return mStats.isValid();
		}

		AppFinishedResponse::AppFinishedResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

		bool AppFinishedResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseAppStats(body);

			return mStats.isValid();
		}

		AppUsageResponse::AppUsageResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{}

        bool AppUsageResponse::parseSuccessBody(QIODevice *body)
		{
			mStats = parseAppStats(body);

			return mStats.isValid();
		}

    }
}
