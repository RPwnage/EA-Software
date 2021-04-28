#ifndef PENDING_ACTION_FACTORY_H
#define PENDING_ACTION_FACTORY_H

#include <QUrl>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class PendingActionBaseViewController;

        namespace PendingActionFactory
        {
            /// \brief Creates a pending action given a url
            ORIGIN_PLUGIN_API PendingActionBaseViewController *createPendingActionViewController(const QUrl &url, bool launchedFromBrowser, QString uuid);

            /// \brief Register all the pending actions the origin client accepts.
            ORIGIN_PLUGIN_API void registerPendingActions();

            /// \brief Given a url obtain the lookup id. 
            ORIGIN_PLUGIN_API QString lookupIdFromUrl(const QUrl &url);
        }
    }
}

#endif