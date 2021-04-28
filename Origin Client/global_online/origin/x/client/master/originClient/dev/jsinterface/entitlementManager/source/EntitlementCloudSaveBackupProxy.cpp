#include "engine/cloudsaves/LocalStateBackup.h"
#include "EntitlementCloudSaveBackupProxy.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "flows/CloudRestoreFlow.h"
#include "services/debug/DebugService.h"
#include "originwindow.h"

using namespace Origin::Engine::CloudSaves;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementCloudSaveBackupProxy::EntitlementCloudSaveBackupProxy(QObject *parent, Engine::Content::EntitlementRef entitlement) :
	QObject(parent),
	mEntitlement(entitlement),
    mRestoreFlow(NULL)
{
}

QDateTime EntitlementCloudSaveBackupProxy::created()
{
	LocalStateBackup lastBackup(LocalStateBackup::lastBackupForEntitlement(mEntitlement));

	if (!lastBackup.isValid())
	{
		return QDateTime();
	}

	return lastBackup.created();
}

void EntitlementCloudSaveBackupProxy::restore()
{
    if(mRestoreFlow == NULL)
    {
        LocalStateBackup lastBackup = LocalStateBackup::lastBackupForEntitlement(mEntitlement);

        mRestoreFlow = new CloudRestoreFlow(lastBackup);
        ORIGIN_VERIFY_CONNECT(mRestoreFlow, SIGNAL(finished(bool)), this, SLOT(onRestoreFlowFinished()));

        mRestoreFlow->start();
    }
    // If the user clicks the Restore button again
    // show the current window if there is one
    else
    {
        if(mRestoreFlow->currentWindow())
        {
            mRestoreFlow->currentWindow()->showWindow();
        }
    }
}

void EntitlementCloudSaveBackupProxy::onRestoreFlowFinished()
{
    if(mRestoreFlow)
    {
        disconnect(mRestoreFlow, SIGNAL(finished(bool)), this, SLOT(onRestoreFlowFinished()));
        mRestoreFlow->deleteLater();
        mRestoreFlow = NULL;
    }
}


}
}
}
