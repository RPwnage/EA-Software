#include "EntitlementRemoteSyncProxy.h"
#include "engine/cloudsaves/RemoteSyncManager.h"
#include "engine/content/ContentConfiguration.h"
#include "services/debug/DebugService.h"

using namespace Origin::Engine::CloudSaves;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementRemoteSyncProxy::EntitlementRemoteSyncProxy(QObject *parent, Engine::Content::EntitlementRef entitlement) :
	QObject(parent),
	mProgress(0.0f),
	mEntitlement(entitlement)
{
	ORIGIN_VERIFY_CONNECT(RemoteSyncManager::instance(), SIGNAL(remoteSyncCreated(Origin::Engine::CloudSaves::RemoteSync*)),
		this, SLOT(remoteSyncCreated(Origin::Engine::CloudSaves::RemoteSync*)));
}
	
RemoteSync* EntitlementRemoteSyncProxy::remoteSync()
{
	return RemoteSyncManager::instance()->remoteSyncByEntitlement(mEntitlement);
}

void EntitlementRemoteSyncProxy::pause()
{
}

void EntitlementRemoteSyncProxy::resume()
{
}

void EntitlementRemoteSyncProxy::cancel()
{
	if (remoteSync())
	{
		remoteSync()->cancel();
	}
}

float EntitlementRemoteSyncProxy::progress()
{
	return mProgress;
}

void EntitlementRemoteSyncProxy::remoteSyncProgressed(float progress, qint64, qint64)
{
	mProgress = progress;
	emit changed();
}

void EntitlementRemoteSyncProxy::remoteSyncCreated(RemoteSync *sync)
{
    if (sync->entitlement()->contentConfiguration()->productId() != 
        mEntitlement->contentConfiguration()->productId())
	{
		return;
	}

	mProgress = 0.0f;

	ORIGIN_VERIFY_CONNECT(sync, SIGNAL(progress(float, qint64, qint64)), 
		this, SLOT(remoteSyncProgressed(float, qint64, qint64)));
	
	ORIGIN_VERIFY_CONNECT(sync, SIGNAL(finished(const QString&)),
		this, SIGNAL(changed()));

	emit changed();
}

QString EntitlementRemoteSyncProxy::phase()
{
	return "RUNNING";
}

}
}
}
