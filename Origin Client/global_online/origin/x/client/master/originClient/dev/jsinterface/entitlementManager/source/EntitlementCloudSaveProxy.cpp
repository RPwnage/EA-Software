#include "engine/content/Entitlement.h"
#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"

#include "EntitlementProxy.h"
#include "EntitlementCloudSaveProxy.h"
#include "EntitlementCloudSaveBackupProxy.h"
#include "EntitlementRemoteSyncProxy.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#include "engine/cloudsaves/RemoteSyncManager.h"

using namespace Origin::Engine::CloudSaves;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementCloudSaveProxy::EntitlementCloudSaveProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement)
    : QObject(parent),
    mEntitlement(entitlement),
    mLastBackup(new EntitlementCloudSaveBackupProxy(this, entitlement)),
    mRemoteSync(new EntitlementRemoteSyncProxy(this, entitlement))
{
    ORIGIN_VERIFY_CONNECT(RemoteUsageMonitor::instance(), SIGNAL(usageChanged(Origin::Engine::Content::EntitlementRef, qint64)),
                            this, SLOT(usageChanged(Origin::Engine::Content::EntitlementRef, qint64)));

    ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                            this, SLOT(settingsChanged()))

    ORIGIN_VERIFY_CONNECT(mRemoteSync, SIGNAL(changed()), this, SIGNAL(changed()));

    // Track this so we know when it changes. Otherwise we end up firing changed() for every entitlement on every
    // settings change
    mLastAutoSyncValue = autoSyncEnabled();
}

QVariant EntitlementCloudSaveProxy::currentUsage()
{
    qint64 currentUsage = RemoteUsageMonitor::instance()->usageForEntitlement(mEntitlement);

    if (currentUsage == RemoteUsageMonitor::UsageUnknown)
    {
        // Unknown usage
        return QVariant();
    }

    return QVariant(currentUsage);
}

qint64 EntitlementCloudSaveProxy::maximumUsage()
{
    return mEntitlement->localContent()->cloudContent()->maximumCloudSaveLocalSize();
}

void EntitlementCloudSaveProxy::pollCurrentUsage()
{
    RemoteUsageMonitor::instance()->checkCurrentUsage(mEntitlement);
}

void EntitlementCloudSaveProxy::usageChanged(Origin::Engine::Content::EntitlementRef entitlement, qint64 newUsage)
{
    if (entitlement == mEntitlement)
    {
        emit currentUsageChanged(newUsage);
        emit changed();
    }
}

QVariant EntitlementCloudSaveProxy::lastBackup()
{
    if (mLastBackup->created().isNull())
    {
        return QVariant();
    }

    return QVariant::fromValue<QObject*>(mLastBackup);
}

bool EntitlementCloudSaveProxy::autoSyncEnabled()
{
    return Services::readSetting(Services::SETTING_EnableCloudSaves, Services::Session::SessionService::currentSession());
}

void EntitlementCloudSaveProxy::settingsChanged()
{
    bool autoSync = autoSyncEnabled();

    if (autoSync != mLastAutoSyncValue)
    {
        // The value for autosync changed
        mLastAutoSyncValue = autoSync;
    }
}

EntitlementRemoteSyncProxy *EntitlementCloudSaveProxy::syncOperation()
{
    if (!RemoteSyncManager::instance()->remoteSyncByEntitlement(mEntitlement))
    {
        // No sync in progress
        return NULL;
    }

    return mRemoteSync;
}

EntitlementCloudSaveBackupProxy *EntitlementCloudSaveProxy::lastBackupObj()
{
    if (mLastBackup->created().isNull())
    {
        return NULL;
    }

    return mLastBackup;
}

}
}
}
