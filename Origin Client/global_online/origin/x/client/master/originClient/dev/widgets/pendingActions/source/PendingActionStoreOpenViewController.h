#ifndef PENDING_ACTION_STORE_OPEN_VIEW_CONTROLLER_H
#define PENDING_ACTION_STORE_OPEN_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrlQuery>
#include "PendingActionBaseViewController.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionStoreOpenViewController : public PendingActionBaseViewController
        {
            Q_OBJECT

            public:
                PendingActionStoreOpenViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
                virtual ~PendingActionStoreOpenViewController(){};

                /// \brief shows the store tab with the approriate url
                void performMainAction();

                /// \brief returns PendingAction::StartupTabStoreId ("store")
                QString startupTab();

                /// \brief returns the store url
                QUrl storeUrl();

                /// \brief checks the context to make sure its OFFER or MASTERTITLE
                bool parseAndValidateParams();

                /// \brief send out telemetry
                void sendTelemetry();

            private:
                QUrl mStoreUrl;
        };
    }
}

#endif