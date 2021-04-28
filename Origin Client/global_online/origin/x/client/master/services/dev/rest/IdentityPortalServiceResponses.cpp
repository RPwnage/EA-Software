///////////////////////////////////////////////////////////////////////////////
// IdentityPortalServiceResponses.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/common/JsonUtil.h"
#include "services/rest/IdentityPortalServiceResponses.h"
#include "services/rest/IdentityPortalServiceClient.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "NodeDocument.h"
#include "ReaderCommon.h"


namespace
{
    QString safe_map_get(const QMap<QString, QString> &map, const QString &key)
    {
        QString value = "";

        if (map.contains(key))
        {
            value = map[key];
        }

        return value;
    }
}

namespace Origin
{
    namespace Services
    {

        IdentityPortalPersonasResponse::IdentityPortalPersonasResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {
        }

        bool IdentityPortalPersonasResponse::parseSuccessBody(QIODevice* body )
        {
            QByteArray payload(body->readAll());

            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            if (json && json->Parse(payload.data()))
            {
            /*
            {
            "personas" : {
            "personaUri" : [ "/pids/2301532262/personas/182005848" ]
            }
            }
             **/
                if (json->FirstChild("personas"))
                {
                    if(json->FirstChild("personaUri"))
                    {
                        if(json->FirstValue())
                        {
                            mPersonaUri = json->GetValue();

                            return true;
                        }
                    }
                }
            }
            return false;
        }

        IdentityPortalPersonasResponse2::IdentityPortalPersonasResponse2(QNetworkReply* reply)
            : OriginServiceResponse(reply), mUserInfo(NULL)
        {
        }


        /*
        {
        "persona" : {
        "personaId" : 182005848,
        "pidId" : 2301532262,
        "displayName" : "testpc",
        "name" : "testpc",
        "namespaceName" : "cem_ea_id",
        "isVisible" : true,
        "status" : "ACTIVE",
        "statusReasonCode" : "",
        "showPersona" : "EVERYONE",
        "dateCreated" : "2013-01-03T5:35Z",
        "lastAuthenticated" : ""
        }
        }
         **/
        bool IdentityPortalPersonasResponse2::parseSuccessBody(QIODevice *body)
        {
            ORIGIN_ASSERT(mUserInfo);
            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            QByteArray payload(body->readAll());
            if (json && json->Parse(payload.data()))
            {
                if (json->FirstChild("persona") && json->FirstAttribute())
                {
                    do
                    {
                        QString key = json->GetAttributeName();
                        QString value = json->GetAttributeValue();
                        mUserInfo->insert(key.toUpper(), value);
                    }
                    while (json->NextAttribute());

                    return true;
                }
            }

            return false;
        }

        IdentityPortalExpandedPersonasResponse::IdentityPortalExpandedPersonasResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        /*
        {
            "personas" : {
                "persona" : [ {
                    "personaId" : 329278692,
                    "pidId" : 2395131246,
                    "displayName" : "jwade_gamesim",
                    "name" : "jwadegamesim",
                    "namespaceName" : "cem_ea_id",
                    "isVisible" : true,
                    "status" : "ACTIVE",
                    "statusReasonCode" : "",
                    "showPersona" : "EVERYONE",
                    "dateCreated" : "2011-06-30T16:52Z",
                    "lastAuthenticated" : "2013-04-01T14:23Z"
                } ]
            }
        }
        */
        bool IdentityPortalExpandedPersonasResponse::parseSuccessBody(QIODevice* body)
        {
            JsonUtil::JsonWrapper dom;
            QByteArray ba(body->readAll());
		    dom.parse(ba);

		    QVariant personaList = dom.search("persona");
            if (!personaList.toMap().isEmpty())
            {
                foreach (QVariant v, personaList.toMap().values())
                {
                    if (!v.toMap().isEmpty())
                    {
                        mPersonaList.push_back(v.toMap());
                    }
                }

                return !mPersonaList.isEmpty();
            }

            return false;
        }

        const QList<QVariantMap>& IdentityPortalExpandedPersonasResponse::personaList() const
        {
            return mPersonaList;
        }

        QString IdentityPortalExpandedPersonasResponse::originId() const
        {
            QString originId;

            if (!mPersonaList.isEmpty())
            {
                QVariantMap::const_iterator i = mPersonaList.at(0).find("displayName");
                if (i != mPersonaList.at(0).constEnd())
                {
                    originId = i.value().toString();
                }
            }

            return originId;
        }

        qlonglong IdentityPortalExpandedPersonasResponse::personaId() const
        {
            qlonglong personaId = -1;

            if (!mPersonaList.isEmpty())
            {
                QVariantMap::const_iterator i = mPersonaList.at(0).find("personaId");
                if (i != mPersonaList.at(0).constEnd())
                {
                    personaId = i.value().toLongLong();
                }
            }

            return personaId;
        }

        IdentityPortalUserResponse::IdentityPortalUserResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        /* Example JSON
        {
        "pid": {
            "pidId": 4684250331,
            "email": "test1@ea.com",
            "emailStatus": "UNKNOWN",
            "strength": "WEAK",
            "dob": "1981-02-02",
            "country": "US",
            "language": "en",
            "locale": "en_US",
            "status": "ACTIVE",
            "reasonCode": "",
            "tosVersion": "1.0",
            "parentalEmail": "",
            "thirdPartyOptin": "false",
            "globalOptin": "true",
            "dateCreated": "2012-12-11T23:43Z",
            "dateModified": "2012-12-11T23:43Z",
            "lastAuthDate": "",
            "registrationSource": "",
            "authenticationSource": "",
            "showEmail": "NO_ONE",
            "discoverableEmail": "NO_ONE",
            "anonymousPid": "false",    //USE THIS ONE TO DETERMINE UNDERAGE
            "underagePid": "false",     //DO NOT USE THIS TO DETERMINE UNDERAGE, it is based on current date - DOB
            "defaultBillingAddressUri": "",
            "defaultShippingAddressUri": ""
         } */

        bool IdentityPortalUserResponse::parseSuccessBody(QIODevice *body)
        {
            QScopedPointer<INodeDocument> json(CreateJsonDocument());
            QByteArray payload(body->readAll());

            if (json && json->Parse(payload.data()))
            {
                if (json->FirstChild("pid") && json->FirstAttribute())
                {
                    do
                    {
                        QString key = json->GetAttributeName();
                        QString value = json->GetAttributeValue();
                        mUserInfo.insert(key, value);
                    }
                    while (json->NextAttribute());

                    return true;
                }
            }

            return false;
        }

        QString IdentityPortalUserResponse::token(const QString& token_name) const
        {
            return safe_map_get(mUserInfo, token_name);
        }

        QString IdentityPortalUserResponse::pidId() const
        {
            return token("pidId");
        }

        QString IdentityPortalUserResponse::email() const
        {
            return token("email");
        }

        QString IdentityPortalUserResponse::emailStatus() const 
        {
            return token("emailStatus");
        }

        QString IdentityPortalUserResponse::strength() const 
        {
            return token("strength");
        }

        QString IdentityPortalUserResponse::dob() const 
        {
            return token("dob");
        }

        QString IdentityPortalUserResponse::country() const 
        {
            return token("country");
        }

        QString IdentityPortalUserResponse::language() const
        {
            return token("language");
        }

        QString IdentityPortalUserResponse::locale() const
        {
            return token("locale");
        }

        QString IdentityPortalUserResponse::status() const  
        {
            return token("status");
        }

        QString IdentityPortalUserResponse::reasonCode() const 
        {
            return token("reasonCode");
        }

        QString IdentityPortalUserResponse::tosVersion() const 
        {
            return token("tosVersion");
        }

        QString IdentityPortalUserResponse::parentalEmail() const 
        {
            return token("parentalEmail");
        }

        QString IdentityPortalUserResponse::thirdPartyOptin() const 
        {
            return token("thirdPartyOptin");
        }

        QString IdentityPortalUserResponse::globalOptin() const 
        {
            return token("globalOptin");
        }

        QString IdentityPortalUserResponse::dateCreated() const 
        {
            return token("dateCreated");
        }

        QString IdentityPortalUserResponse::dateModified() const 
        {
            return token("dateModified");
        }

        QString IdentityPortalUserResponse::lastAuthDate() const 
        {
            return token("lastAuthDate");
        }

        QString IdentityPortalUserResponse::registrationSource() const 
        {
            return token("registrationSource");
        }

        QString IdentityPortalUserResponse::authenticationSource() const
        {
            return token("authenticationSource");
        }

        QString IdentityPortalUserResponse::showEmail() const 
        {
            return token("showEmail");
        }

        QString IdentityPortalUserResponse::discoverableEmail() const
        {
            return token("discoverableEmail");
        }

        QString IdentityPortalUserResponse::anonymousPid() const 
        {
            return token("anonymousPid");
        }

        QString IdentityPortalUserResponse::underagePid() const 
        {
            return token("underagePid");
        }

        QString IdentityPortalUserResponse::defaultBillingAddressUri() const
        {
            return token("defaultBillingAddressUri");
        }

        QString IdentityPortalUserResponse::defaultShippingAddressUri() const
        {
            return token("defaultShippingAddressUri");
        }

        QMap<IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory, QString> IdentityPortalUserProfileUrlsResponse::sProfileInfoCategoryToStringMap;

        IdentityPortalUserProfileUrlsResponse::IdentityPortalUserProfileUrlsResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {
            if (sProfileInfoCategoryToStringMap.isEmpty())
            {
                sProfileInfoCategoryToStringMap.insert(MobileInfo, "MOBILE_INFO");
                sProfileInfoCategoryToStringMap.insert(Name, "NAME");
                sProfileInfoCategoryToStringMap.insert(Address, "ADDRESS");
                sProfileInfoCategoryToStringMap.insert(SecurityQA, "SECURITY_QA");
                sProfileInfoCategoryToStringMap.insert(Gender, "GENDER");
                sProfileInfoCategoryToStringMap.insert(BillingAddress, "BILLING_ADDRESS");
                sProfileInfoCategoryToStringMap.insert(ShippingAddress, "SHIPPING_ADDRESS");
                sProfileInfoCategoryToStringMap.insert(UserDefaults, "USER_DEFAULTS");
                sProfileInfoCategoryToStringMap.insert(SafeChildInfo, "SAFE_CHILD_INFO");
            }

            for (QMap<IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory, QString>::iterator i = sProfileInfoCategoryToStringMap.begin();
                    i != sProfileInfoCategoryToStringMap.end(); ++i)
            {
                mProfileUrls.insert(i.key(), QStringList());
            }
        }

        /* Example JSON
        {
          "pidProfiles": {
  	        "pidProfileUri" : [
    	        "/pids/19056/profileinfo/category/mobile_info/entryid/216350731/",
    	        "/pids/19056/profileinfo/category/mobile_info/entryid/1898928754/",
    	        "/pids/19056/profileinfo/category/ADDRESS/entryid/18989/"
  	        ]
          }
        }*/

        bool IdentityPortalUserProfileUrlsResponse::parseSuccessBody(QIODevice *body)
        {
            JsonUtil::JsonWrapper dom;
		    dom.parse(body->readAll());

		    QVariant pidProfileUri = dom.search("pidProfileUri");
            if (!pidProfileUri.toMap().isEmpty())
            {
                foreach (QVariant v, pidProfileUri.toMap().values())
                {
                    if (!v.toString().isEmpty())
                    {
                        for (QMap<IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory, QString>::iterator i = sProfileInfoCategoryToStringMap.begin();
                            i != sProfileInfoCategoryToStringMap.end(); ++i)
                        {
                            if (v.toString().contains(i.value(), Qt::CaseInsensitive))
                            {
                                mProfileUrls[i.key()].push_back(v.toString());
                                break;
                            }
                        }
                    }
                }

                return true;
            }

            return false;
        }

        void IdentityPortalUserProfileUrlsResponse::parseUrlArray(const QString& jsonArray)
        {
            QStringList urls = jsonArray.split(",", QString::SkipEmptyParts);
            foreach (QString url, urls)
            {
                url.remove("\"");

                for (QMap<IdentityPortalUserProfileUrlsResponse::ProfileInfoCategory, QString>::iterator i = sProfileInfoCategoryToStringMap.begin();
                    i != sProfileInfoCategoryToStringMap.end(); ++i)
                {
                    if (url.contains(i.value(), Qt::CaseInsensitive))
                    {
                        mProfileUrls[i.key()].push_back(url);
                        break;
                    }
                }
            }
        }

        IdentityPortalUserNameResponse::IdentityPortalUserNameResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
            , mFirstName("UNKNOWN")
            , mLastName("UNKNOWN")
        {

        }

        /* Example JSON
        {
            "pidProfileEntry" : {
                "pidUri" : "/pids/2301532262/",
                "entryId" : 2301532262,
                "entryCategory" : "NAME",
                "entryElement" : [ {
                    "profileType" : "FIRST_NAME",
                    "value" : "John"
                }, {
                    "profileType" : "LAST_NAME",
                    "value" : "Smith"
                } ]
            }
        } */

        bool IdentityPortalUserNameResponse::parseSuccessBody(QIODevice *body)
        {
            JsonUtil::JsonWrapper dom;
            QByteArray ba(body->readAll());
		    dom.parse(ba);

		    QVariant entryElement = dom.search("entryElement");
            if (!entryElement.toMap().isEmpty())
            {
                foreach (QVariant v, entryElement.toMap().values())
                {
                    if (!v.toMap().isEmpty())
                    {
                        QString profileType = v.toMap()["profileType"].toString();
                        if (profileType.compare("FIRST_NAME", Qt::CaseInsensitive) == 0)
                        {
                            mFirstName = v.toMap()["value"].toString();
                        }
                        else if (profileType.compare("LAST_NAME", Qt::CaseInsensitive) == 0)
                        {
                            mLastName = v.toMap()["value"].toString();
                        }

                    }
                }

                return true;
            }

            return false;
        }

        IdentityPortalExpandedNameProfilesResponse::IdentityPortalExpandedNameProfilesResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
        {

        }

        const QList<QVariantMap>& IdentityPortalExpandedNameProfilesResponse::profileList() const
        {
            return mProfileList;
        }

        QString IdentityPortalExpandedNameProfilesResponse::firstName() const
        {
            QString firstname;

            if (!mProfileList.isEmpty())
            {
                QVariantMap::const_iterator entryElement = mProfileList.at(0).find("entryElement");
                if (entryElement != mProfileList.at(0).constEnd())
                {
                    if (!entryElement->toMap().isEmpty())
                    {
                        foreach (QVariant v, entryElement->toMap().values())
                        {
                            if (!v.toMap().isEmpty())
                            {
                                QString profileType = v.toMap()["profileType"].toString();
                                if (profileType.compare("FIRST_NAME", Qt::CaseInsensitive) == 0)
                                {
                                    firstname = v.toMap()["value"].toString();
                                    break;
                                }

                            }
                        }
                    }
                }
            }

            return firstname;
        }

        QString IdentityPortalExpandedNameProfilesResponse::lastName() const
        {
            QString lastname;

            if (!mProfileList.isEmpty())
            {
                QVariantMap::const_iterator entryElement = mProfileList.at(0).find("entryElement");
                if (entryElement != mProfileList.at(0).constEnd())
                {
                    if (!entryElement->toMap().isEmpty())
                    {
                        foreach (QVariant v, entryElement->toMap().values())
                        {
                            if (!v.toMap().isEmpty())
                            {
                                QString profileType = v.toMap()["profileType"].toString();
                                if (profileType.compare("LAST_NAME", Qt::CaseInsensitive) == 0)
                                {
                                    lastname = v.toMap()["value"].toString();
                                    break;
                                }

                            }
                        }
                    }
                }
            }

            return lastname;
        }

        /*
        {
            "pidProfiles" : {
                "pidProfileEntry" : [ {
                    "pidUri" : "/pids/2395131246/",
                    "entryId" : 2395131246,
                    "entryCategory" : "NAME",
                    "entryElement" : [ {
                        "profileType" : "FIRST_NAME",
                        "value" : " "
                    }, {
                        "profileType" : "LAST_NAME",
                        "value" : " "
                    } ]
                } ]
            }
        }
        */
        bool IdentityPortalExpandedNameProfilesResponse::parseSuccessBody(QIODevice* body)
        {
            JsonUtil::JsonWrapper dom;
            QByteArray ba(body->readAll());
		    dom.parse(ba);

		    QVariant personaList = dom.search("pidProfileEntry");
            if (!personaList.toMap().isEmpty())
            {
                foreach (QVariant v, personaList.toMap().values())
                {
                    if (!v.toMap().isEmpty())
                    {
                        mProfileList.push_back(v.toMap());
                    }
                }

                return !mProfileList.isEmpty();
            }

            return false;
        }

        IdentityPortalUserIdResponse::IdentityPortalUserIdResponse(QNetworkReply* reply) :
            OriginServiceResponse(reply)
        {

        }
        

        bool IdentityPortalUserIdResponse::parseSuccessBody(QIODevice *body)
        {
            JsonUtil::JsonWrapper dom;
            QByteArray ba(body->readAll());
            QString s(ba);
		    dom.parse(ba);

            /* pidByEAID JSON response
            {
                "personas": [
                    {
                        "personaUri": ["/pids/123/personas/789"]
                    }
                ]
            }*/

            /* pidByEmail JSON response
            {
                "pidUri" : [ "/pids/2308416878" ]
            }*/

            bool success = false;
            QVariant v = dom.search("personas");
            if (!v.toMap().isEmpty())
            {
                v = v.toMap()["personaUri"];
                if (!v.toMap().isEmpty())
                {
                    QString uri = v.toMap()["0"].toString();
                    uri.replace("\\", "/");
                    QStringList tokens = uri.split("/", QString::SkipEmptyParts);
                    if (tokens.size() > 1)
                    {
                        mPid = tokens.at(1).toLongLong(&success);
                    }
                }

            }
            else
            {
                v = dom.search("pidUri");
                if (!v.toMap().isEmpty())
                {
                    QString uri = v.toMap()["0"].toString();
                    if (!uri.isEmpty())
                    {
                        uri.replace("\\", "/");
                        QStringList tokens = uri.split("/", QString::SkipEmptyParts);
                        if (tokens.size() > 1)
                        {
                            mPid = tokens.at(1).toLongLong(&success);
                        }
                    }
                }
            }

            return success;

        }


    }
}
