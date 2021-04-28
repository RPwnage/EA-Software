#ifndef PENDING_ACTION_CURRENT_TAB_VIEW_CONTROLLER_H
#define PENDING_ACTION_CURRENT_TAB_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrlQuery>
#include "PendingActionBaseViewController.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionCurrentTabViewController : public PendingActionBaseViewController
        {
            Q_OBJECT

            public:
                PendingActionCurrentTabViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
                virtual ~PendingActionCurrentTabViewController(){};

                /// \brief kicks off a refresh if needed
                void performMainAction();
        };
    }
}

#endif