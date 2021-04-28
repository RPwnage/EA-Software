///////////////////////////////////////////////////////////////////////////////
// SubscriptionVaultServiceResponse.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/subscription/SubscriptionVaultServiceResponse.h"
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

            SubscriptionVaultInfoResponse::SubscriptionVaultInfoResponse( QNetworkReply* reply ) :
                OriginServiceResponse(reply)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
            }

            bool SubscriptionVaultInfoResponse::parseSuccessBody( QIODevice *body )
            {
                QScopedPointer<INodeDocument> pDoc(CreateXmlDocument());

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
