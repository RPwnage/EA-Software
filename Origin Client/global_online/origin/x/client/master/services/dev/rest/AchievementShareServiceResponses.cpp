///////////////////////////////////////////////////////////////////////////////
// SearchFriendServiceResponses.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/AchievementShareServiceResponses.h"
#include "services/common/XmlUtil.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {
        AchievementShareServiceResponses::AchievementShareServiceResponses(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
        }

        bool AchievementShareServiceResponses::parseSuccessBody(QIODevice* body)
        {
            mPayload = body->readAll();
            // test if we have this setting set already
            if (!mPayload.contains("shareAchievements"))
            {
                ORIGIN_LOG_EVENT << "RECEIVED payload does not contain shareAchievements - adding it for completion:\n" << mPayload;
                //...we don't, make it PENDING
                mPayload.replace("\n  }\n}", ",\n    \"shareAchievements\" : \"PENDING\"\n  }\n}");
            }
            return mPayload.size() > 0;
        }

        QByteArray AchievementShareServiceResponses::payload() const
        {
            return mPayload;
        }

    }
}
