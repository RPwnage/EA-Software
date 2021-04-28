#include "PendingActionCurrentTabViewController.h"
#include "engine/content/ContentController.h"

namespace Origin
{
    namespace Client
    {
        using namespace PendingAction;

        PendingActionCurrentTabViewController::PendingActionCurrentTabViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            :PendingActionBaseViewController(queryParams, launchedFromBrowser, uuid, lookupId)
        {
        }

        void PendingActionCurrentTabViewController::performMainAction()
        {
            ensureClientVisible();
            refreshEntitlementsIfRequested();
            mComponentsCompleted |= MainAction;
        }
    }
}