#include "engine/cloudsaves/ClearRemoteStateManager.h"
#include "engine/cloudsaves/ClearRemoteStateRequest.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "engine/content/ContentConfiguration.h"
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    static ClearRemoteStateManager *g_clearSavesAreaInstance;

    void ClearRemoteStateManager::clearRemoteStateForEntitlement(Content::EntitlementRef entitlement, const bool& clearOldPath /* = false */)
    {
        ClearRemoteStateRequest *req = new ClearRemoteStateRequest(entitlement, clearOldPath);

        ORIGIN_VERIFY_CONNECT(req, SIGNAL(failed()), req, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(req, SIGNAL(succeeded()), this, SLOT(succeeded()));

        req->start();
    }

    void ClearRemoteStateManager::succeeded()
    {
        ClearRemoteStateRequest *req = dynamic_cast<ClearRemoteStateRequest*>(sender());

        ORIGIN_ASSERT(req);
        req->deleteLater();

        // The area is now empty
        RemoteUsageMonitor::instance()->setUsageForEntitlement(req->entitlement(), 0);
    }

    ClearRemoteStateManager* ClearRemoteStateManager::instance()
    {
        if (g_clearSavesAreaInstance == NULL)
        {
            g_clearSavesAreaInstance = new ClearRemoteStateManager;
        }

        return g_clearSavesAreaInstance;
    }
}
}
}
