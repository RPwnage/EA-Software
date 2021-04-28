///////////////////////////////////////////////////////////////////////////////
// IdentityPortalServiceResponses.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/common/JsonUtil.h"
#include "services/rest/SignInPortalServiceResponses.h"
#include "services/rest/SignInPortalServiceClient.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {

        SignInPortalEmailVerificationResponse::SignInPortalEmailVerificationResponse(QNetworkReply *reply)
            : OriginServiceResponse(reply)
            , mSuccess(false)
        {
        }

        bool SignInPortalEmailVerificationResponse::parseSuccessBody(QIODevice* body )
        {
            QJsonParseError jsonResult;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body->readAll(), &jsonResult);

            //check if its a valid JSON response
            if(jsonResult.error == QJsonParseError::NoError)
            {
                QJsonValue statusJsonValue = jsonDoc.object().value("status");

                //check if it has a "status" key
                if(!statusJsonValue.isUndefined())
                {
                    bool status = statusJsonValue.toBool();
                    if(status)
                    {
                        QJsonValue messageJsonValue = jsonDoc.object().value("message");
                        if(!messageJsonValue.isUndefined())
                        {
                            QString message = messageJsonValue.toString();
                            if(message.compare("success", Qt::CaseInsensitive) == 0)
                            {
                                mSuccess = true;
                            }
                        }
                    }
                }
            }
            return mSuccess;
        }

    }
}
