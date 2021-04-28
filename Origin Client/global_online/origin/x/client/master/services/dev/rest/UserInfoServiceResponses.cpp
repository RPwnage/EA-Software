///////////////////////////////////////////////////////////////////////////////
// UserInfoServiceResponses.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/UserInfoServiceResponses.h"
#include "services/log/LogService.h"

#include "NodeDocument.h"
#include "ReaderCommon.h"

namespace
{

    // Okay, so this should actually be an XML fragment - yeah.
    // The "info" JSON should look something like the following:
    //
    // { "info" : {
    //
    //      "eaid" : eaid, 
    //      "first_name" : first_name,
    //      "last_name" : last_name,
    //      "country" : country,
    //      "email" : email,
    //      "underage" : underage,
    //      "user_id" : user_id,  
    //      "persona_id" : persona_id
    //      }
    // }
    //

    const static QString UserId("user_id");
    const static QString PersonaId("persona_id");
    const static QString EAID("eaid");
    const static QString FirstName("first_name");
    const static QString LastName("last_name");
    const static QString Country("country");
    const static QString Email("email");
    const static QString Underage("underage");
}

namespace Origin
{
    namespace Services
    {

        Origin::Services::User UserInfoResponse::parseInfo(const QDomElement& userInfoElement)
        {
            Origin::Services::User user;

            for (QDomNode ee = userInfoElement.firstChild(); !ee.isNull(); ee=ee.nextSibling())
            {
                QDomElement e = ee.toElement();

                if (e.tagName() == EAID)
                {
                    user.originId = e.text();
                }
                else if (e.tagName() == FirstName)
                    user.firstName = e.text();
                else if (e.tagName() == LastName)
                    user.lastName = e.text();
                else if (e.tagName() == Country)
                    user.country = e.text();
                else if (e.tagName() == Email)
                    user.email = e.text();
                else if (e.tagName() == Underage)
                    user.isUnderage = e.text()=="true";
                else if (e.tagName() == UserId)
                {
                    user.userId = e.text().toULongLong();
                }
                else if (e.tagName() == PersonaId)
                {
                    user.personaId = e.text().toULongLong();
                }
            }

            return user;
        }

        bool UserInfoResponse::parseSuccessBody(QIODevice* body )
        {
            QDomDocument doc;

            if (!doc.setContent(body))
                // Not valid XML
                return false;

            if (doc.documentElement().nodeName() != "userInfo")
                return false;

            mUser = parseInfo(doc.documentElement());

            return mUser.isValid();
        }


        UserInfoResponse::UserInfoResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {

        }

    }
}
