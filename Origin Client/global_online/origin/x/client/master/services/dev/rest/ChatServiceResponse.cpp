///////////////////////////////////////////////////////////////////////////////
// ChatServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/ChatServiceResponse.h"
#include "services/rest/HttpStatusCodes.h"

namespace Origin
{
    namespace Services
    {
        UserOnlineResponse::UserOnlineResponse(AuthNetworkRequest reply) : OriginAuthServiceResponse(reply), m_userOnline(false)
        {
        }

        void UserOnlineResponse::processReply()
        {
            // Check the status
            m_userOnline = Http200Ok == reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }	
    }
}

