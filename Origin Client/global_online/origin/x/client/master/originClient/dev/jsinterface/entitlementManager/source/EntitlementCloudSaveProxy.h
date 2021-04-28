#ifndef _ENTITLEMENTCLOUDSAVEPROXY_H
#define _ENTITLEMENTCLOUDSAVEPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QVariant>
#include <QObject>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class EntitlementCloudSaveBackupProxy;
class EntitlementRemoteSyncProxy;

class ORIGIN_PLUGIN_API EntitlementCloudSaveProxy : public QObject
{
    Q_OBJECT
public:
    explicit EntitlementCloudSaveProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement);

    Q_PROPERTY(QVariant currentUsage READ currentUsage)
    QVariant currentUsage();

    Q_PROPERTY(qint64 maximumUsage READ maximumUsage)
    qint64 maximumUsage();

    Q_INVOKABLE void pollCurrentUsage();

    Q_PROPERTY(QVariant lastBackup READ lastBackup);
    QVariant lastBackup();

    Q_PROPERTY(bool autoSyncEnabled READ autoSyncEnabled);
    bool autoSyncEnabled();

    EntitlementRemoteSyncProxy *syncOperation();
    EntitlementCloudSaveBackupProxy *lastBackupObj();

signals:
    void currentUsageChanged(qint64);
    void changed();

private slots:
    void usageChanged(Origin::Engine::Content::EntitlementRef entitlement, qint64 newUsage);
    void settingsChanged();

private:
    Origin::Engine::Content::EntitlementRef mEntitlement;
    EntitlementCloudSaveBackupProxy *mLastBackup;
    EntitlementRemoteSyncProxy *mRemoteSync;

    bool mLastAutoSyncValue;
};

}
}
}

#endif
