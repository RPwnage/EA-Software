///////////////////////////////////////////////////////////////////////////////
// SubscriptionTrialEligibilityServiceResponse.cpp
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/subscription/SubscriptionTrialEligibilityServiceResponse.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include <NodeDocument.h>

namespace Origin
{
namespace Services
{
namespace Subscription
{

SubscriptionTrialEligibilityServiceResponse::SubscriptionTrialEligibilityServiceResponse(QNetworkReply* reply)
: OriginServiceResponse(reply)
, mIsTrialEligible(-1)
{
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
}


bool SubscriptionTrialEligibilityServiceResponse::parseSuccessBody(QIODevice* body)
{
    QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

    QByteArray data = body->readAll();
    if(pDoc->Parse(data.data()))
    {
        const QString isEligibleValue = pDoc->GetAttributeValue("isEligible");
        const QString errorValue = pDoc->GetAttributeValue("error");
        ORIGIN_LOG_ACTION << "Subscription trial status received. " << data.data();
        mIsTrialEligible = (isEligibleValue == "true") ? 1 : 0;
        return true;
    }

    ORIGIN_LOG_ERROR << "Subscription trial response doc can't be read";
    mIsTrialEligible = 0;
    return false;
}

}
}
}