///////////////////////////////////////////////////////////////////////////////
// SubscriptionTrialEligibilityServiceResponse.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SUBSCRIPTIONTRIALELIGIBILITYSERVICERESPONSE_H
#define _SUBSCRIPTIONTRIALELIGIBILITYSERVICERESPONSE_H

#include "services/plugin/PluginAPI.h"
#include "services/rest/OriginServiceResponse.h"

namespace Origin
{
namespace Services
{
namespace Subscription
{

class ORIGIN_PLUGIN_API SubscriptionTrialEligibilityServiceResponse : public OriginServiceResponse
{
public:
    explicit SubscriptionTrialEligibilityServiceResponse(QNetworkReply*);

    /// \brief indicates if the user has access to the subscription trial.
    /// -1 = not received yet, 0 = false, 1 = true
    const int isTrialEligible() const {return mIsTrialEligible;}


protected:
    bool parseSuccessBody(QIODevice*);


private:
    int mIsTrialEligible;
};

}
}
}
#endif //_SUBSCRIPTIONTRIALELIGIBILITYSERVICERESPONSE_H