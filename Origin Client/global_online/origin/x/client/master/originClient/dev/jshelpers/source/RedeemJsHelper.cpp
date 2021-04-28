///////////////////////////////////////////////////////////////////////////////
// RedeemJsHelper.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "RedeemJsHelper.h"
#include "engine/ito/InstallFlowManager.h"
#include <flows/MainFlow.h>
#include <flows/RTPFlow.h>

#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"

#include <QUrl>
#include <QWidget>

namespace Origin
{
    namespace Client
    {
        RedeemJsHelper::RedeemJsHelper(RedeemType redeemType, QObject *parent)
            : MyGameJsHelper(parent)
            , mRedeemType(redeemType)
        {
        }

        void RedeemJsHelper::codeRedeemed()
        {
            ORIGIN_LOG_EVENT << "Code successfully redeemed.  Redeem type = " << mRedeemType;
            GetTelemetryInterface()->Metric_ACTIVATION_CODE_REDEEM_SUCCESS();

            if(mRedeemType == ITE)
            {
                Engine::InstallFlowManager::GetInstance().SetVariable("CodeRedeemed",true);
                // set this to false because we are about to refresh the entitlements.  then in the ITOFlow it will go to
                // the pRedeemWaitForRefreshEntitlements state which will wait for the refresh to complete before checking
                // if the newly redeemed entitlement is in the library.
                Engine::InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", false);
            }
            else if(mRedeemType == RTP)
            {
                Origin::Client::MainFlow::instance()->rtpFlow()->setCodeRedeemed(true);
            }

            refreshEntitlements();
        }

        //	Called when the user clicked on the FAQ link.
        void RedeemJsHelper::faqRequested()
        {
            ORIGIN_LOG_ACTION << "User viewing code redemption FAQ.";
            GetTelemetryInterface()->Metric_ACTIVATION_FAQ_PAGE_REQUEST();
        }

        //	Called when code redemption results in error
        void RedeemJsHelper::redeemError(int errorCode)
        {
            ORIGIN_LOG_ERROR << "Code redemption error.  Error code = " << errorCode;
            GetTelemetryInterface()->Metric_ACTIVATION_CODE_REDEEM_ERROR(errorCode);
        }

        void RedeemJsHelper::next()
        {
            if(mRedeemType == RTP)
            {
                Origin::Client::MainFlow::instance()->rtpFlow()->attemptPendingLaunch();
            }

            close();
        }

        void RedeemJsHelper::close()
        {
            emit(windowCloseRequested());
        }

    }
}
