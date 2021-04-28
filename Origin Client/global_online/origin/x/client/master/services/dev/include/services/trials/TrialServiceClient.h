///////////////////////////////////////////////////////////////////////////////
// TrialServiceClient.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _TRIALSERVICECLIENT_H
#define _TRIALSERVICECLIENT_H

#include "services/rest/OriginAuthServiceClient.h"
#include "services/trials/TrialServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Trials
        {

            ///
            /// \brief HTTP service client for the communication with the achievement service.
            ///
            class ORIGIN_PLUGIN_API TrialServiceClient : public OriginAuthServiceClient
            {
            public:
                friend class OriginClientFactory<TrialServiceClient>;


                ///
                /// \brief Returns the amount of time available in the trial.
                ///
                /// \param contentId The contentId of the game where we try to burn time for.
                /// \param requestToken The optional Denuvo time request token.
                static TrialBurnTimeResponse *burnTime(const QString& contentId, const QString &requestToken)
                {
                    return OriginClientFactory<TrialServiceClient>::instance()->burnTimePriv(contentId, requestToken);
                }

                ///
                /// \brief Returns the amount of time available in the trial.
                ///
                /// \param contentId The contentId of which we want to know how much trial time is left.
                static TrialCheckTimeResponse *checkTime(const QString& contentId)
                {
                    return OriginClientFactory<TrialServiceClient>::instance()->checkTimePriv(contentId);
                }

                ///
                /// \brief Grants the user additional time. This only works in Int environments.
                ///
                /// \param contentId The contentId of which we want to add more time.
                static TrialGrantTimeResponse *grantTime(const QString& contentId)
                {
                    return OriginClientFactory<TrialServiceClient>::instance()->grantTimePriv(contentId);
                }

            private:


                TrialBurnTimeResponse *burnTimePriv(const QString& contentId, const QString &requestTicket);
                TrialCheckTimeResponse *checkTimePriv(const QString& contentId);
                TrialGrantTimeResponse *grantTimePriv(const QString& contentId);


                /// 
                /// \brief Creates a new trial service client
                ///
                /// \param baseUrl       Base URL for the trail service API.
                /// \param nam           QNetworkAccessManager instance to send requests through.
                ///
                explicit TrialServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
            };
        }
    }
}

#endif //_TRIALSERVICECLIENT_H