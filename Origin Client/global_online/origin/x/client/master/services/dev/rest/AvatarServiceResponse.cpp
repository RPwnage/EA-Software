///////////////////////////////////////////////////////////////////////////////
// AvatarServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/AvatarServiceResponse.h"

namespace Origin
{
	namespace Services
	{
		AvatarDefaultAvatarResponse::AvatarDefaultAvatarResponse( AuthNetworkRequest reply)
			: OriginAuthServiceResponse(reply)
		{
		}

		void setAvatarInfo( const QDomElement &avatar, AvatarInformation &ai )
		{

			QDomElement avatarEl = avatar.toElement();
			for (QDomNode ee = avatarEl.firstChild(); !ee.isNull(); ee=ee.nextSibling())
			{
				QDomElement e = ee.toElement();
				if (e.tagName() == QString("avatarId"))
					ai.avatarId = e.text().toULongLong();
				else if (e.tagName() == QString("orderNumber"))
					ai.orderNumber = e.text().toULongLong();
				else if (e.tagName() == QString("link"))
					ai.link = e.text();
				else if (e.tagName() == QString("isRecent"))
					ai.isRecent = QString("true") == e.text() ? true: false;
				else if (e.tagName() == QString("statusName"))
					ai.statusName = e.text();
				else if (e.tagName() == QString("statusId"))
				{
					imageStatus is = static_cast<imageStatus>(e.text().toULongLong());
					if (ImageStatus().exists(is))
						ai.statusId = is;
				}
				else if (e.tagName() == QString("typeName"))
				{
					if (avatarTypes().valExists(e.text()))
						ai.typeName = e.text();
				}
				else if (e.tagName() == QString("typeId"))
				{
					avatarType at = static_cast<avatarType>(e.text().toULongLong());
					if (avatarTypes().exists(at))
						ai.typeId = at;
				}
				else if (e.tagName() == QString("galleryId"))
					ai.galleryId = e.text().toULongLong();
				else if (e.tagName() == QString("galleryName"))
					ai.galleryName = e.text();
			}

		}


		static bool parseAvatarInfoBody(QIODevice *body, QHash<quint64, AvatarInformation>& hash)
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			// check that the element galleryName is "default" and that
			// the element typeName belongs to one of the four official type names
			if (doc.documentElement().nodeName() != "avatars")
				return false;

			QDomElement root = doc.documentElement();

			// add the individual elements to the settings
			bool res(false);
			for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				QDomElement avatar = n.toElement(); // try to convert the node to an element.
				if (avatar.isNull() || (avatar.tagName() != "avatar"))
					// Invalid XML document
					return false;

				AvatarInformation ai;
				setAvatarInfo(avatar, ai);
				hash[ai.avatarId] = ai;
				res = true;
			}
			return res;
		}

		static bool parseAvatarInfoBodySingle(QIODevice *body, AvatarInformation& ai)
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			// check that the element galleryName is "default" and that
			// the element typeName belongs to one of the four official type names
			if (doc.documentElement().nodeName() != "avatar")
				return false;

			QDomElement avatar = doc.documentElement();
			if (avatar.isNull() || (avatar.tagName() != "avatar"))
				// Invalid XML document
				return false;

			setAvatarInfo(avatar, ai);

			return true;
		}

		bool AvatarDefaultAvatarResponse::parseSuccessBody( QIODevice *body )
		{
			return parseAvatarInfoBodySingle(body, m_defaultAvatarInfo);
		}

		AvatarTypeResponse::AvatarTypeResponse( AuthNetworkRequest reply )
			: OriginAuthServiceResponse(reply)
		{

		}

		bool AvatarTypeResponse::parseSuccessBody( QIODevice *body )
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			// check that the element galleryName is "default" and that
			// the element typeName belongs to one of the four official type names
			if (doc.documentElement().nodeName() != "avatarType")
				return false;

			QDomElement root = doc.documentElement();

			// add the individual elements to the settings
			bool res(false);
			for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				QDomElement e = n.toElement(); // try to convert the node to an element.
				if (!e.isNull())
				{
					if (e.tagName() == "id")
						m_avatarType.id = e.text().toULongLong();
					if (e.tagName() == "name")
						m_avatarType.name = e.text();
					res = true;
				}
			}
			return res;

		}

		AvatarsByGalleryIdResponse::AvatarsByGalleryIdResponse( AuthNetworkRequest reply )
			: AvatarDefaultAvatarResponse(reply )
		{
		}

		bool AvatarsByGalleryIdResponse::parseSuccessBody( QIODevice *body )
		{
			return parseAvatarInfoBody(body,m_Hash);
		}

		AvatarAllAvatarTypesResponse::AvatarAllAvatarTypesResponse( AuthNetworkRequest reply )
			: OriginAuthServiceResponse(reply)
		{
		}

		bool AvatarAllAvatarTypesResponse::parseSuccessBody( QIODevice *body )
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			if (doc.documentElement().nodeName() != "avatarTypes")
				// Not the XML document we're looking for 
				return false;

			QDomElement root = doc.documentElement();

			for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				QDomElement avatarTypeEl = n.toElement(); // try to convert the node to an element.
				if (avatarTypeEl.isNull() || (avatarTypeEl.tagName() != "avatarType"))
					// Invalid XML document
					return false;
				if (!avatarTypeEl.firstChildElement("name").text().isEmpty() 
					&& !avatarTypeEl.firstChildElement("id").text().isEmpty())
				{
					// We are retrieving all the avatar types.
					// We already have an official set, but if any of the retrieved
					// elements is not in the set, we add it. If it comes with in this set,
					// it must be official.
					QString Name = avatarTypeEl.firstChildElement("name").text();
					if (!avatarTypes().valExists(Name))
					{
						avatarType index = static_cast<avatarType>(avatarTypeEl.firstChildElement("id").text().toULongLong());;
						// Add the received avatar type info to our avatar types map.
						avatarTypes().add(index, Name);
					}
				}
			}

			return true;
		}

		AvatarsByUserIdsResponse::AvatarsByUserIdsResponse( AuthNetworkRequest reply, const QList<quint64> &requestedUserIds )
			: AvatarDefaultAvatarResponse(reply),
			  m_requestedUserIds(requestedUserIds)
		{

		}

		bool AvatarsByUserIdsResponse::parseSuccessBody( QIODevice *body )
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			// check that the element galleryName is "default" and that
			// the element typeName belongs to one of the four official type names
			if (doc.documentElement().nodeName() != "users")
				return false;

			QDomElement root = doc.documentElement();

			// add the individual elements to the settings
			for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				QDomElement e = n.toElement(); // try to convert the node to an element.
				if (e.isNull() || (e.tagName() != "user"))
					// Invalid XML document
					return false;

				QDomElement rt = n.toElement();
				// add the individual elements to the settings
				for (QDomNode nn = rt.firstChild(); !nn.isNull(); nn = nn.nextSibling())
				{
					QDomElement ee = nn.toElement(); // try to convert the node to an element.
					if (ee.isNull() || (ee.tagName() != "userId"))
						// Invalid XML document
						return false;

					UserAvatarsInfo uai;
					uai.userId = ee.text().toULongLong();

					nn = nn.nextSibling();
					ee = nn.toElement(); // try to convert the node to an element.

					QDomElement avatar  = ee.toElement();
					if (avatar.isNull() || (avatar.tagName() != "avatar"))
						// Invalid XML document
						return false;

					AvatarInformation ai;
					setAvatarInfo(avatar, ai);
					uai.info = ai;
					m_Hash[uai.userId] = uai;

				}
			}
			return true;

		}

		AvatarGetRecentResponse::AvatarGetRecentResponse( AuthNetworkRequest reply )
			: AvatarsByGalleryIdResponse(reply)
		{
		}


		AvatarSupportedDimensionsResponse::AvatarSupportedDimensionsResponse( AuthNetworkRequest reply )
			: OriginAuthServiceResponse(reply)
		{

		}

		bool AvatarSupportedDimensionsResponse::parseSuccessBody( QIODevice *body )
		{
			QDomDocument doc;

			if (!doc.setContent(body))
				// Not valid XML
				return false;

			if (doc.documentElement().nodeName() != "dimensions")
				// Not the XML document we're looking for 
				return false;

			QDomElement root = doc.documentElement();

			bool res(false);
			for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				QDomElement dimension = n.toElement(); // try to convert the node to an element.
				if (dimension.isNull() || (dimension.tagName() != "dimension"))
					// Invalid XML document
					return false;

				m_list.append(dimension.text());
				res = true;
			}

			return res;

		}

		bool AvatarBooleanResponse::parseSuccessBody( QIODevice *body )
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

			QString resultText = doc.documentElement().text();

			m_bResult = resultText == "true";

			return m_bResult;

		}

		AvatarBooleanResponse::AvatarBooleanResponse( AuthNetworkRequest reply )
			:OriginAuthServiceResponse(reply), m_bResult(false)
		{

		}
	}
}
