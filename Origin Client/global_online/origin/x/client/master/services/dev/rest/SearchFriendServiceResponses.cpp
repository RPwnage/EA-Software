///////////////////////////////////////////////////////////////////////////////
// SearchFriendServiceResponses.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/SearchFriendServiceResponses.h"
#include "services/common/XmlUtil.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {
        SearchFriendServiceResponse::SearchFriendServiceResponse(AuthNetworkRequest reply) : 
            OriginAuthServiceResponse(reply)
            ,mTotalCount(-1)
        {
        }

        bool SearchFriendServiceResponse::parseSuccessBody(QIODevice *body)
        {
            QDomDocument doc;

            if (!doc.setContent(body))
            {
                // Not valid XML
                return false;
            }

            if (doc.documentElement().nodeName() != "searchInfos")
            {
                // Not the XML document we're looking for 
                return false;
            }

            QDomElement root = doc.documentElement();

            for (QDomElement elem = root.firstChildElement("searchInfo"); !elem.isNull(); elem = elem.nextSiblingElement("searchInfo"))
            {
                SearchInfo searchInfo;
                searchInfo.mAvatarLink = Origin::Util::XmlUtil::getString(elem, "avatarLink");
                searchInfo.mEaid = Origin::Util::XmlUtil::getString(elem, "eaid");
                searchInfo.mFriendUserId = Origin::Util::XmlUtil::getString(elem, "friendUserId");
                searchInfo.mFullName = Origin::Util::XmlUtil::getString(elem, "fullName");
                if (searchInfo.mFullName.contains("$private$"))
                {
                    searchInfo.mFullName = tr("ebisu_client_private");
                }
                mList.append(searchInfo);
            }

            QDomElement x = doc.documentElement().firstChildElement("totalCount");

            if (!x.isNull())
            {
                mTotalCount = x.text().toInt();
            }

            return mList.size() > 0;
        }

        const QList<SearchInfo> SearchFriendServiceResponse::list() const
        {
            return mList;
        }

        int SearchFriendServiceResponse::totalCount() const
        {
            return mTotalCount;
        }

    }
}
