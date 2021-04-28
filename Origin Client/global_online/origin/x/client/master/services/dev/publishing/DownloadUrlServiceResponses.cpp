///////////////////////////////////////////////////////////////////////////////
// DownloadUrlResponse.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <limits>
#include <QDateTime>
#include <QDomDocument>
#include "services/publishing/DownloadUrlServiceResponses.h"
#include "services/common/XmlUtil.h"

namespace Origin
{
	namespace Services
	{
        namespace Publishing
        {
		    DownloadUrlResponse::DownloadUrlResponse(AuthNetworkRequest reply, bool buildIdSpecified)
			    : OriginAuthServiceResponse(reply),
                  m_buildIdSpecified(buildIdSpecified)
		    {
		    }

		    bool DownloadUrlResponse::parseSuccessBody(QIODevice* body )
		    {
			    QDomDocument doc;

			    QByteArray bodyContents = body->readAll();

			    if (!doc.setContent(bodyContents))
				    // Not valid XML
				    return false;

                QUrlQuery query(reply()->url());
                m_downloadUrl.mOfferId = query.queryItemValue("productId");
			    m_downloadUrl.mURL = Origin::Util::XmlUtil::getString(doc,"url").trimmed();
                m_downloadUrl.mSyncURL = Origin::Util::XmlUtil::getString(doc, "syncurl").trimmed();
			    m_downloadUrl.mValidStartTime = Origin::Util::XmlUtil::getDateTime(doc, "startTime");
			    m_downloadUrl.mValidEndTime = Origin::Util::XmlUtil::getDateTime(doc, "endTime");

			    return m_downloadUrl.isValid();
		    }

            void DownloadUrlResponse::processReply()
            {
                OriginAuthServiceResponse::processReply();

                // If we specified a build ID and server returned "NO_DOWNLOAD_URL", we 
                // want special developer-only error messaging.
                if(m_buildIdSpecified && error() == restErrorNoDownloadUrl)
                {
                    setError(restErrorInvalidBuildIdOverride);
                }
            }
        }
	}
}
