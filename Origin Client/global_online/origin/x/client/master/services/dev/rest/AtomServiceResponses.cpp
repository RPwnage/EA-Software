///////////////////////////////////////////////////////////////////////////////
// AtomServiceResponses.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include <QDomElement>
#include "services/rest/AtomServiceResponses.h"
#include "services/common/XmlUtil.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"


namespace Origin
{
    namespace Services
    {

        UserResponse::UserResponse(AuthNetworkRequest reply )
            : OriginAuthServiceResponse(reply)
        {

        }

        // The UserResponse used to only return information for a single user.  It is being updated to 
        // return the info for up to 5 users(batch size may increase at a later time).
        bool UserResponse::parseSuccessBody(QIODevice* body)
        {
            ORIGIN_ASSERT(body);

            // Empty response, failure!
            if(body == NULL || body->bytesAvailable() == 0)
            {
                return false;
            }

            QDomDocument doc;
            if (!doc.setContent(body->readAll()))
            {
                // Not valid XML
                return false;
            }

            QDomElement root = doc.documentElement();

            // More than one user's profile is included
            if (root.tagName() == "users")
            {
                for (QDomElement elem = root.firstChildElement("user"); !elem.isNull(); elem = elem.nextSiblingElement("user"))
                {
                    User user;
                    user.userId = Origin::Util::XmlUtil::getLongLong(elem, "userId");
                    user.email = Origin::Util::XmlUtil::getString(elem, "email");
                    user.personaId = Origin::Util::XmlUtil::getLongLong(elem, "personaId");
                    user.originId = Origin::Util::XmlUtil::getString(elem, "EAID");
                    user.firstName = Origin::Util::XmlUtil::getString(elem, "firstName");
                    user.lastName = Origin::Util::XmlUtil::getString(elem, "lastName");

                    if (user.userId > 0)
                    {
                        mUsers.append(user);
                    }
                }

                return true;
            }
            // Just one user
            else
            {
                mUser.userId = Origin::Util::XmlUtil::getLongLong(doc, "userId");
                mUser.email = Origin::Util::XmlUtil::getString(doc, "email");
                mUser.personaId = Origin::Util::XmlUtil::getLongLong(doc, "personaId");
                mUser.originId = Origin::Util::XmlUtil::getString(doc, "EAID");
                mUser.firstName = Origin::Util::XmlUtil::getString(doc, "firstName");
                mUser.lastName = Origin::Util::XmlUtil::getString(doc, "lastName");

                return mUser.userId > 0;
            }
        }

        AppSettingsResponse::AppSettingsResponse(QNetworkReply * reply )
            : OriginServiceResponse(reply)
        {

        }

        bool AppSettingsResponse::parseSuccessBody(QIODevice* body)
        {
            ORIGIN_ASSERT(body);

            // Empty response, failure!
            if(body == NULL || body->bytesAvailable() == 0)
            {
                return false;
            }

            QDomDocument doc;
            if (!doc.setContent(body->readAll()))
            {
                // Not valid XML
                return false;
            }

            mUserSettings.dt = Origin::Util::XmlUtil::getString(doc, "dt", "mygames");
            mUserSettings.overrideTab = Origin::Util::XmlUtil::getBool(doc, "overrideTab", mUserSettings.overrideTab);

            return mUserSettings.isValid();

        }

        SearchOptionsResponseBase::SearchOptionsResponseBase(AuthNetworkRequest reply )
            : OriginAuthServiceResponse(reply)
        {
        }

        bool SearchOptionsResponseBase::parseSuccessBody( QIODevice* body )
        {
            ORIGIN_ASSERT(body);

            if(body == NULL || body->bytesAvailable() == 0)
            {
                // This is not a valid response
                return false;
            }

            // We read into a byte array instead of calling setContent(body) directly because 
            // QDomDocument::setContent(body) is returning a fail in some cases
            QByteArray msg = body->readAll();
            QDomDocument doc;

            if (!doc.setContent(msg))
            {
                // Not valid XML
                return false;
            }

            if (doc.documentElement().nodeName() != "options")
            {
                // Not the XML document we're looking for ? 
                return false;
            }

            QDomNodeList rootChildren = doc.documentElement().childNodes();

            for(int elementIndex = 0; elementIndex < rootChildren.length(); elementIndex++)
            {
                QDomElement profileElement = rootChildren.at(elementIndex).toElement();

                if (profileElement.isNull() || (profileElement.tagName() != "value"))
                    // Invalid XML document
                    return false;

                mValues.append(profileElement.nodeValue());

            }

            return mValues.size() > 0;
        }

        SearchOptionsResponse::SearchOptionsResponse( AuthNetworkRequest reply ) :
        SearchOptionsResponseBase(reply)
        {
        }

        SetSearchOptionsResponse::SetSearchOptionsResponse( AuthNetworkRequest reply ) :
        SearchOptionsResponseBase(reply)
        {
        }


        PrivacyGetSettingResponse::PrivacyGetSettingResponse( AuthNetworkRequest reply )
            : OriginAuthServiceResponse(reply)
        {
        }

        bool PrivacyGetSettingResponse::parseSuccessBody( QIODevice *body )
        {
            QDomDocument doc;

            // This is a valid response if no setting has been set for the requested option
            if(body == NULL || body->bytesAvailable() == 0)
            {
                return true;
            }

            // We read into a byte array instead of calling setContent(body) directly because 
            // QDomDocument::setContent(body) is returning a fail in some cases
            QByteArray msg = body->readAll();
            if (!doc.setContent(msg))
                // Not valid XML
                return false;

            if (doc.documentElement().nodeName() != "privacySettings")
                // Not the XML document we're looking for ? 
                return parseSuccessBodySingle( body ); //...maybe is a single privacy response

            QDomNodeList rootChildren = doc.documentElement().childNodes();

            for(int id = 0; id < rootChildren.length(); id++)
            {
                QDomElement privacySetting = rootChildren.at(id).toElement();

                if (privacySetting.isNull() || (privacySetting.tagName() != "privacySetting"))
                    // Invalid XML document
                    return false;

                UserPrivacySetting ps;
                ps.userId = privacySetting.firstChildElement("userId").text().toULongLong();
                ps.category = privacySetting.firstChildElement("category").text();
                ps.payload = privacySetting.firstChildElement("payload").text();
                // Add the received info to our list
                m_privacySettings.append(ps);
            }
#ifdef _DEBUG
            foreach (UserPrivacySetting ups, m_privacySettings)
            {
                ORIGIN_LOG_EVENT << "USERID: " << ups.userId << "CATEGORY: " << ups.category << "PAYLOAD: " << ups.payload;
            }
#endif // _DEBUG
            return m_privacySettings.size() >= 0;
        }

        bool PrivacyGetSettingResponse::parseSuccessBodySingle( QIODevice *body )
        {
            QDomDocument doc;

            if (!doc.setContent(body))
                // Not valid XML
                return false;

            // check that the element galleryName is "default" and that
            // the element typeName belongs to one of the four official type names
            if (doc.documentElement().nodeName() != "privacySetting")
                return false;

            QDomElement root = doc.documentElement();

            // add the individual elements to the settings
            // TBD: we really need to strengthen the error handling
            for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
            {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if (!e.isNull())
                {
                    UserPrivacySetting ps;
                    if (e.tagName() == QString("userId"))
                        ps.userId = e.text().toULongLong();
                    else if (e.tagName() == QString("category"))
                        ps.category = e.text();
                    else if (e.tagName() == QString("payload"))
                        ps.payload = e.text();
                    // Add the received info to our map
                    m_privacySettings.append(ps);
                }
            }
            return m_privacySettings.size() > 0;
        }

        UserGamesResponse::UserGamesResponse(AuthNetworkRequest req)
            : UserGamesResponseBase(req)
        {
        }

        bool UserGamesResponse::parseSuccessBody(QIODevice* body)
        {
            ORIGIN_ASSERT(body);

            if (body == NULL)
            {
                return false;
            }

            QDomDocument doc;
            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            QDomElement rootEl = doc.documentElement();

            if (rootEl.tagName() != "productInfoList")
            {
                ORIGIN_LOG_WARNING << "Unexpected root element name: " << rootEl.tagName();
                return false;
            }
            
            for(QDomElement productInfoEl = rootEl.firstChildElement("productInfo");
                !productInfoEl.isNull();
                productInfoEl = productInfoEl.nextSiblingElement("productInfo"))
            {
                QDomElement productIdEl = productInfoEl.firstChildElement("productId");
                QDomElement displayProductNameEl = productInfoEl.firstChildElement("displayProductName");
                QDomElement cdnAssetRootEl = productInfoEl.firstChildElement("cdnAssetRoot");
                QDomElement softwareListEl = productInfoEl.firstChildElement("softwareList");
                QDomElement imageServer = productInfoEl.firstChildElement("imageServer");
                QDomElement backgroundImage = productInfoEl.firstChildElement("backgroundImage");
                QDomElement packArtSmall = productInfoEl.firstChildElement("packArtSmall");
                QDomElement packArtMedium = productInfoEl.firstChildElement("packArtMedium");
                QDomElement packArtLarge = productInfoEl.firstChildElement("packArtLarge");

                if (productIdEl.isNull() || displayProductNameEl.isNull() || (imageServer.isNull() && cdnAssetRootEl.isNull()))
                {
                    mUserGames.clear();
                    return false;
                }

                UserGameData userGame;

                userGame.productId = productIdEl.text();
                userGame.displayProductName = displayProductNameEl.text();
                userGame.cdnAssetRoot = cdnAssetRootEl.text();
                userGame.imageServer = imageServer.text();
                userGame.backgroundImage = backgroundImage.text();
                userGame.packArtSmall = packArtSmall.text();
                userGame.packArtMedium = packArtMedium.text();
                userGame.packArtLarge = packArtLarge.text();

                if (!softwareListEl.isNull())
                {
                    // Get the achievement set for each platform
                    QMap<QString, QString> platformAchievementSetIds;

#ifdef ORIGIN_MAC
                    const QString preferredPlatform("MAC");
#else
                    const QString preferredPlatform("PCWIN");
#endif

                    for(QDomElement softwareEl = softwareListEl.firstChildElement("software");
                        !softwareEl.isNull();
                        softwareEl = softwareEl.nextSiblingElement("software"))
                    {
                        QDomElement achievementSetEl = softwareEl.firstChildElement("achievementSetOverride");

                        if (!achievementSetEl.isNull())
                        {
                            platformAchievementSetIds[softwareEl.attribute("softwarePlatform")] = achievementSetEl.text();
                        }
                    }

                   
                    // Try our preferred platform then fallback to PCWIN
                    if (platformAchievementSetIds.contains(preferredPlatform))
                    {
                        userGame.achievementSetId = platformAchievementSetIds[preferredPlatform];
                    }
                    else if (platformAchievementSetIds.contains("PCWIN"))
                    {
                        userGame.achievementSetId = platformAchievementSetIds["PCWIN"];
                    }
                }

                mUserGames.append(userGame);
            }

            return true;
        }

        UserFriendsResponse::UserFriendsResponse(AuthNetworkRequest req)
            : OriginAuthServiceResponse(req)
        {
        }

        bool UserFriendsResponse::parseSuccessBody(QIODevice* body)
        {
            ORIGIN_ASSERT(body);

            if (body == NULL)
            {
                return false;
            }

            QDomDocument doc;
            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            QDomElement rootEl = doc.documentElement();

            if (rootEl.tagName() != "users")
            {
                ORIGIN_LOG_WARNING << "Unexpected root element name: " << rootEl.tagName();
                return false;
            }
            
            for(QDomElement userEl = rootEl.firstChildElement("user");
                !userEl.isNull();
                userEl = userEl.nextSiblingElement("user"))
            {
                QDomElement userIdEl = userEl.firstChildElement("userId");
                QDomElement personaIdEl = userEl.firstChildElement("personaId");
                QDomElement eaidEl = userEl.firstChildElement("EAID");
                QDomElement firstNameEl = userEl.firstChildElement("firstName");
                QDomElement lastNameEl = userEl.firstChildElement("lastName");

                if (userIdEl.isNull() || personaIdEl.isNull() || eaidEl.isNull())
                {
                    mUserFriends.clear();
                    return false;
                }

                UserFriendData userFriend;

                userFriend.userId = userIdEl.text().toULongLong();
                userFriend.personaId = personaIdEl.text().toULongLong();
                userFriend.EAID = eaidEl.text();

                if (!firstNameEl.isNull() && !firstNameEl.text().isEmpty() && !lastNameEl.isNull() && !lastNameEl.text().isEmpty())
                {
                    userFriend.firstName = firstNameEl.text();
                    userFriend.lastName = lastNameEl.text();
                }

                mUserFriends.append(userFriend);
            }

            return true;
        }

        ShareAchievementsResponse::ShareAchievementsResponse(AuthNetworkRequest req)
            : OriginAuthServiceResponse(req)
            ,mIsShareAchievements(false)
        {
        }

        bool ShareAchievementsResponse::isShareAchievements() const
        {
            return mIsShareAchievements;
        }

        QString ShareAchievementsResponse::userId() const
        {
            return mUserId;
        }

        // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
        //      <privacySetting>
        //          <userId>12296549260</userId>
        //          <category>shareAchievements</category>
        //          <payload>true</payload>
        //      </privacySetting>

        bool ShareAchievementsResponse::parseSuccessBody(QIODevice* body)
        {
            ORIGIN_ASSERT(body);
            if (body == NULL)
            {
                return false;
            }

            QDomDocument doc;
            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            // you could check the root tag name here if it matters
            //Get the root element
            QDomElement rootEl = doc.documentElement(); 
            QString rootTag = rootEl.tagName(); // == privacySetting

            ORIGIN_ASSERT(rootTag=="privacySetting");
            ORIGIN_ASSERT(Origin::Util::XmlUtil::getString(rootEl, "category")=="shareAchievements");

            mUserId = Origin::Util::XmlUtil::getString(rootEl, "userId");
            mIsShareAchievements = Origin::Util::XmlUtil::getString(rootEl, "payload")=="true";

            return true;
        }

        UserGamesResponseBase::UserGamesResponseBase(AuthNetworkRequest reply )
            : OriginAuthServiceResponse(reply)
        {

        }

        QList<UserGameData> UserGamesResponseBase::userGames() const
        {
            return mUserGames;
        }

        UserGamesResponseBase::~UserGamesResponseBase()
        {

        }

        bool UserGamesResponseBase::parseSuccessBody( QIODevice* body )
        {
            return true;
        }

    }
}
