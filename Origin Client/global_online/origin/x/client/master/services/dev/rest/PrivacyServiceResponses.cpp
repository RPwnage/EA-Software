///////////////////////////////////////////////////////////////////////////////
// PrivacyServiceClientResponses.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>

#include "services/rest/PrivacyServiceResponses.h"

// requires definition here
template <typename T, typename U, typename V>
QHash<U, V> PropertiesMap<T, U, V>::m_Hash;

namespace Origin
{
	namespace Services
	{
		PrivacyVisibilityResponse::PrivacyVisibilityResponse(AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply) , m_visibilitySetting(visibilityNoOne)
		{
		}

		bool PrivacyVisibilityResponse::parseSuccessBody(QIODevice *body)
		{
			QDomDocument doc;

			if (!doc.setContent(body))
			{
				// Not valid XML
				return false;
			}

			if (doc.documentElement().nodeName() != "result")
			{
				// Not the XML document we're looking for 
				return false;
			}

			QByteArray resultText = doc.documentElement().text().toUtf8();

			if (Visibility().valExists(resultText))
			{
				m_visibilitySetting = Visibility().keyForValue(resultText);
				return true;
			}

			return false;
		}

		PrivacyFriendVisibilityResponse::PrivacyFriendVisibilityResponse( AuthNetworkRequest reply )
			: OriginAuthServiceResponse(reply)
		{

		}

		bool PrivacyFriendVisibilityResponse::parseSuccessBody( QIODevice *body )
		{
			m_visibilityFriend = OriginServiceResponse::parseSuccessBody(body);
			return m_visibilityFriend;
		}

		PrivacyFriendsVisibilityResponse::PrivacyFriendsVisibilityResponse( AuthNetworkRequest reply )
			: OriginAuthServiceResponse(reply)
		{

		}

		bool PrivacyFriendsVisibilityResponse::parseSuccessBody( QIODevice *body )
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			if (doc.documentElement().nodeName() != "users")
				// Not the XML document we're looking for 
				return false;

			QDomNodeList rootChildren = doc.documentElement().childNodes();

			for(int id = 0; id < rootChildren.length(); id++)
			{
				QDomElement friendProfile = rootChildren.at(id).toElement();

				if (friendProfile.isNull() || (friendProfile.tagName() != "user"))
					// Invalid XML document
					return false;

				QString userId = friendProfile.firstChildElement("userId").text();
				bool isVisible = friendProfile.firstChildElement("visibility").text() == "true";
				// Add the received avatar type info to our avatar types map.
				friendsProfiles().add(userId, isVisible);
			}

			return true;
		}
	}
}

