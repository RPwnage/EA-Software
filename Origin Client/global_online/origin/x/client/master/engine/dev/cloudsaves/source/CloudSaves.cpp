#include <QMetaType>

#include "CloudSaves.h"
#include "engine/cloudsaves/RemoteSync.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "engine/cloudsaves/FilenameBlacklistManager.h"
#include "services/log/LogService.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    void init()
    {
        // Set up cloud saves
        ORIGIN_LOG_EVENT << "Cloud save setup";

        qRegisterMetaType<SyncFailure>("Origin::Engine::CloudSaves::SyncFailure");

        RemoteUsageMonitor::instance();
        FilenameBlacklistManager::instance()->startNetworkUpdate();
    }
}
}
}
