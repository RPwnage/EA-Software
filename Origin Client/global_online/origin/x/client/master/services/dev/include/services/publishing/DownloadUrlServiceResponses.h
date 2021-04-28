///////////////////////////////////////////////////////////////////////////////
// DownloadUrlServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DownloadUrlResponse_H_INCLUDED_
#define _DownloadUrlResponse_H_INCLUDED_

#include <limits>
#include <QDateTime>

#include "services/rest/OriginAuthServiceResponse.h"
#include "services/rest/ServerUserData.h"

namespace Origin
{
	namespace Services
	{
        namespace Publishing
        {
		    struct ORIGIN_PLUGIN_API DownloadURL : public ServerUserData
		    {
                QString mOfferId;
			    QString mURL;
                QString mSyncURL;
			    virtual bool isValid() const { return mURL.size() > 0;}
			    void reset() { mURL.clear();}
                QDateTime mValidStartTime;
			    QDateTime mValidEndTime;
		    };
		    /// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/loginregistration/

		    class ORIGIN_PLUGIN_API DownloadUrlResponse : public OriginAuthServiceResponse
		    {

		    public:
			    explicit DownloadUrlResponse(AuthNetworkRequest, bool buildIdSpecified);

			    const DownloadURL& downloadUrl() const { return m_downloadUrl;}
			    ///
			    /// DTOR
			    ///
			    virtual ~DownloadUrlResponse() {}
		    protected:
			    bool parseSuccessBody(QIODevice *body);
                void processReply();
		    protected:
			    DownloadURL m_downloadUrl;
                bool m_buildIdSpecified;
		    };
        }
	}
}





#endif


