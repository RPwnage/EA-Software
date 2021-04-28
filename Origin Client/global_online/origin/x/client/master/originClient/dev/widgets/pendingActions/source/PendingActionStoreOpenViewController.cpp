#include "PendingActionStoreOpenViewController.h"
#include "ClientFlow.h"
#include "OriginApplication.h"
#include "StoreUrlBuilder.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Client
    {
        using namespace PendingAction;

        PendingActionStoreOpenViewController::PendingActionStoreOpenViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            :PendingActionBaseViewController(queryParams, launchedFromBrowser, uuid, lookupId)
        {
        }

        QUrl PendingActionStoreOpenViewController::storeUrl()
        {
            return mStoreUrl;
        }

        void PendingActionStoreOpenViewController::performMainAction()
        {
            ensureClientVisible();
            ClientFlow::instance()->showUrlInStore(storeUrl());
            mComponentsCompleted |= MainAction;
        }

        QString PendingActionStoreOpenViewController::startupTab()
        {
            return PendingAction::StartupTabStoreId;
        }

        bool PendingActionStoreOpenViewController::parseAndValidateParams()
        {
            bool valid = PendingActionBaseViewController::parseAndValidateParams();

            QString context = mQueryParams.queryItemValue(ParamContextTypeId);
            QString id = mQueryParams.queryItemValue(ParamIdId);

            StoreUrlBuilder builder;
            if(!context.isEmpty())
            {
                if(context.compare(ContextTypeOfferId, Qt::CaseInsensitive) == 0)
                {
                    mStoreUrl = builder.productUrl(id);
                }
                else
                if(context.compare(ContextTypeMasterTitleId, Qt::CaseInsensitive) == 0)
                {
                    mStoreUrl = builder.masterTitleUrl(id);
                }
                else
                {
                    valid = false;
                }
            }
            else
            {
                mStoreUrl = builder.homeUrl();
            }

            return valid;
        }

        void PendingActionStoreOpenViewController::sendTelemetry()
        {
            if (mFromJumpList)
            {
                //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                GetTelemetryInterface()->Metric_JUMPLIST_ACTION("store", "jumplist", "" );
            }
            //call the baseclass one
            PendingActionBaseViewController::sendTelemetry();
        }
    }
}