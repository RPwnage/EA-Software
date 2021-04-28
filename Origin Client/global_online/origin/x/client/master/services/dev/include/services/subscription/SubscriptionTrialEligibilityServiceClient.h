///////////////////////////////////////////////////////////////////////////////
// SubscriptionTrialEligibilityServiceClient.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SUBSCRIPTIONTRIALELIGIBILITYSERVICECLIENT_H
#define _SUBSCRIPTIONTRIALELIGIBILITYSERVICECLIENT_H

#include "services/plugin/PluginAPI.h"
#include "services/rest/OriginServiceClient.h"

namespace Origin
{
namespace Services
{
namespace Subscription
{

class SubscriptionTrialEligibilityServiceResponse;

///
/// \brief HTTP service client for the communication with the store endpoint.
///
class ORIGIN_PLUGIN_API SubscriptionTrialEligibilityServiceClient : public OriginServiceClient
{
public:
    friend class OriginClientFactory<SubscriptionTrialEligibilityServiceClient>;

    ///
    /// \brief Returns subscription trial eligibility
    ///
    static SubscriptionTrialEligibilityServiceResponse* subscriptionTrialEligibilityInfo()
    {
        return OriginClientFactory<SubscriptionTrialEligibilityServiceClient>::instance()->subscriptionTrialEligibilityInfoPriv();
    }


private:
    /// \brief The actual implementation of the info query.
    SubscriptionTrialEligibilityServiceResponse* subscriptionTrialEligibilityInfoPriv();

    /// 
    /// \brief Creates a new service client
    ///
    /// \param baseUrl       Base URL for the service API.
    /// \param nam           QNetworkAccessManager instance to send requests through.
    ///
    explicit SubscriptionTrialEligibilityServiceClient(const QUrl& baseUrl = QUrl(), NetworkAccessManager* nam = NULL);
};

}
}
}
#endif //_SUBSCRIPTIONTRIALELIGIBILITYSERVICECLIENT_H