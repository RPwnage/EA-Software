/////////////////////////////////////////////////////////////////////////////
// CloudSyncFlow.CPP
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "CloudSyncFlow.h"
#include "services/settings/SettingsManager.h"
#include "engine/content/ContentTypes.h"
#include "engine/content/ContentController.h"
#include "engine/content/CloudContent.h"
#include "engine/content/PlayFlow.h"
#include "engine/cloudsaves/RemoteSyncManager.h"
#include "widgets/cloudsaves/source/CloudSyncViewController.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{

CloudSyncFlow::CloudSyncFlow(Engine::Content::EntitlementRef entitlement)
: mEntitlement(entitlement)
, mVC(NULL)
{
}


CloudSyncFlow::~CloudSyncFlow()
{
    finish();
}


void CloudSyncFlow::start()
{
    mVC = new CloudSyncViewController(mEntitlement, false, this);
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(retryPush()), this, SLOT(startPush()));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(retryPull()), this, SLOT(startPull()));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(continueFlow()), this, SLOT(flowSuccess()));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(stopFlow()), this, SLOT(flowFailed()));
    ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(disableCloudStorage()), this, SLOT(onDisableCloud()));

    ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(pullFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString)), this, SLOT(onPullFailed(Origin::Engine::CloudSaves::SyncFailure)));
    ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(externalChangesDetected()), mVC, SLOT(showExternalSavesWarning()));
    ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(pushFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString)), this, SLOT(onPushFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString)));
    ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(remoteSyncFinished(Origin::Engine::Content::EntitlementRef)), mVC, SLOT(onRemoteSyncFinished()));
    ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->cloudContent(), SIGNAL(cloudSaveProgress(float, qint64, qint64)), mVC, SLOT(onCloudProgressChanged(const float&, const qint64&, const qint64&)));

    Engine::CloudSaves::RemoteSync* remotePull = Engine::CloudSaves::RemoteSyncManager::instance()->remoteSyncByEntitlement(mEntitlement);
    if(remotePull)
    {
        // Show the dialog if the game uses cloud save, or if the game was started from the client 
        // (as opposed to RTP)
        // http://online.ea.com/documentation/display/EBI/Removing+Launch+Dialog+from+RTP+Game+Launch+PRD
        if(remotePull->direction() == Engine::CloudSaves::RemoteSync::PullFromCloud && !mEntitlement->localContent()->playFlow()->launchedFromRTP())
            mVC->showCloudSync();
        ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(succeeded()), this, SLOT(flowSuccess()));
        ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(syncConflictDetected(const QDateTime&, const QDateTime&)), mVC, SLOT(showSyncConflictDetected(const QDateTime&, const QDateTime&)));
        ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(untrustedRemoteFilesDetected()), mVC, SLOT(showUntrustedRemoteFilesDetected()));
        ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(emptyCloudWarning()), mVC, SLOT(showEmptyCloudWarning()));
        ORIGIN_VERIFY_CONNECT(mVC, SIGNAL(stateChosen(const int&)), remotePull, SLOT(handleDelegateResponse(const int&)));
    }
}


void CloudSyncFlow::flowFailed()
{
	CloudSyncFlowResult result;
	result.productId = mEntitlement->contentConfiguration()->productId();
	result.result = FLOW_FAILED;
	emit finished(result);
    finish();
}


void CloudSyncFlow::flowSuccess()
{
	CloudSyncFlowResult result;
	result.productId = mEntitlement->contentConfiguration()->productId();
	result.result = FLOW_SUCCEEDED;
	emit finished(result);
    finish();
}


void CloudSyncFlow::finish()
{
    disconnect();
    Engine::CloudSaves::RemoteSync* remotePull = Engine::CloudSaves::RemoteSyncManager::instance()->remoteSyncByEntitlement(mEntitlement);
    if(remotePull)
    {
        remotePull->cancel();
    }

    if(mVC)
    {
        mVC->disconnect();
        delete mVC;
        mVC = NULL;
    }
}


void CloudSyncFlow::startPush()
{
    mEntitlement->localContent()->cloudContent()->pushToCloud();
}


void CloudSyncFlow::startPull()
{
    mEntitlement->localContent()->cloudContent()->pullFromCloud();
}


bool CloudSyncFlow::canPullCloudSync(const QString& productID)
{
    bool shouldCloudSync = true;
    bool waitForServerUserSetting = false;

    // Check and see if we've gotten the privacyprofile already.
    // If not, we need to hide the cloudsync portion until we find out.
    // Also, if we're offline, then dont' show cloudsync.
    if (!Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::currentUser()->getSession())) //offline
    {
        ORIGIN_LOG_EVENT << "Launching... offline";
        shouldCloudSync = false;
        waitForServerUserSetting = false;
    }
    else
    {
        Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productID);
        if(!entRef.isNull() && entRef->localContent()->cloudContent()->hasCloudSaveSupport())
        {
            if (Services::SettingsManager::instance()->areServerUserSettingsLoaded())
            {
                shouldCloudSync = entRef->localContent()->cloudContent()->shouldCloudSync();
                waitForServerUserSetting = false;
                ORIGIN_LOG_EVENT << "Launching... privacy setting already set: sync =  " << shouldCloudSync;
            }
            else
            {
                ORIGIN_LOG_EVENT << "Launching... waiting for server user settings";
                waitForServerUserSetting = true;
                shouldCloudSync = true;        //for now
            }
        }
        else
        {
            shouldCloudSync = false;
            waitForServerUserSetting = false;
        }
    }

	return waitForServerUserSetting || shouldCloudSync;
}


void CloudSyncFlow::onPullFailed(Origin::Engine::CloudSaves::SyncFailure failureReason)
{
    switch(failureReason.reason())
    {
    case Engine::CloudSaves::DelegateCancelledFailure:
    case Engine::CloudSaves::SystemCancelledFailure:
        flowFailed();
        break;

    case Engine::CloudSaves::QuotaExceededFailure:
        mVC->showQuotaExceededFailure(failureReason);
        break;

    case Engine::CloudSaves::NoSpaceFailure:
        mVC->showNoSpaceFailure(failureReason);
        break;

    case Engine::CloudSaves::ManifestLockedFailure:
        mVC->showManifestLockedFailure();
        break;

    default:
        mVC->showErrorPlay();
        break;
    }
}


void CloudSyncFlow::onPushFailed(Origin::Engine::CloudSaves::SyncFailure failureReason, QString /*displayName*/, QString offerId)
{
    switch(failureReason.reason())
    {
    case Engine::CloudSaves::DelegateCancelledFailure:
    case Engine::CloudSaves::SystemCancelledFailure:
        ORIGIN_LOG_EVENT << "Canceled: failureReason: " << failureReason.reason();
        break;

    case Engine::CloudSaves::QuotaExceededFailure:
        mVC->showQuotaExceededFailure(failureReason);
        break;

    default:
        GetTelemetryInterface()->Metric_CLOUD_ERROR_SYNC_LOCKING_UNSUCCESFUL(offerId.toUtf8().data());
        mVC->showErrorRetry();
        break;
    }
}


void CloudSyncFlow::onDisableCloud()
{
    mEntitlement->localContent()->cloudContent()->disableCloudStorage();
    flowSuccess();
}

}
}