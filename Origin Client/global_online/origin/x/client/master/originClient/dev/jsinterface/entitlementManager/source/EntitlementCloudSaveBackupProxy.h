#ifndef _ENTITLEMENTCLOUDSAVEBACKUPPROXY_H
#define _ENTITLEMENTCLOUDSAVEBACKUPPROXY_H

#include <QObject>

#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class CloudRestoreFlow;
}
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API EntitlementCloudSaveBackupProxy : public QObject
{
	Q_OBJECT
public:
	EntitlementCloudSaveBackupProxy(QObject *parent, Engine::Content::EntitlementRef entitlement);

	Q_PROPERTY(QDateTime created READ created);
	QDateTime created();

	Q_INVOKABLE void restore();

protected slots:
    void onRestoreFlowFinished();

protected:
    Engine::Content::EntitlementRef mEntitlement;

    // Prevent users from seeing multiple restore dialogs.
    CloudRestoreFlow* mRestoreFlow;
};

}
}
}

#endif
