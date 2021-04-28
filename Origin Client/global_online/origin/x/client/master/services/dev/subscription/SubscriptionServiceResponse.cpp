///////////////////////////////////////////////////////////////////////////////
// SubscriptionInfoResponse.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/subscription/SubscriptionServiceResponse.h"
#include "services/debug/DebugService.h"

#include <QDomDocument>
#include <NodeDocument.h>
#include <ReaderCommon.h>
#include "serverReader.h"

namespace Origin
{
    namespace Services
    {
        namespace Subscription
        {

            SubscriptionInfoResponse::SubscriptionInfoResponse( QNetworkReply* reply ) :
                OriginServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool SubscriptionInfoResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    Read(pDoc.data(), mInfo);
                    return true;
                }

                return false;
            }

            SubscriptionDetailInfoResponse::SubscriptionDetailInfoResponse( QNetworkReply* reply ) :
                OriginServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool SubscriptionDetailInfoResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    Read(pDoc.data(), mInfo);
                    return true;
                }

                return false;
            }
        }
    }
}
