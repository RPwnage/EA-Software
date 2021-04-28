///////////////////////////////////////////////////////////////////////////////
// ContentFlowController.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/ContentFlowController.h"
#include "engine/content/ContentController.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/PlayFlow.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "PlayFlow.h"
#include "ClientFlow.h"
#include "CloudSyncFlow.h"
#include "CustomizeBoxartFlow.h"
#include "InstallerFlow.h"
#include "NonOriginGameFlow.h"
#include "SaveBackupRestoreFlow.h"
#include "EntitleFreeGameFlow.h"
#include "widgets/client/source/LocalContentViewController.h"
#include "widgets/cloudsaves/source/RemoteSyncViewController.h"
#include "tasks/RestartGameTask.h"
#include <QFutureWatcher>

#if defined(ORIGIN_DEBUG)
#define ENABLE_FLOW_TRACE   0
#else
#define ENABLE_FLOW_TRACE   0
#endif

namespace Origin
{
namespace Client
{

ContentFlowController::ContentFlowController()
{
}


ContentFlowController::~ContentFlowController()
{
}


void ContentFlowController::endUserSpecificFlows()
{
    for(QMap<QString, CloudSyncFlow*>::iterator iter = mCloudSyncFlowMap.begin(); iter != mCloudSyncFlowMap.end(); iter++)
    {
        delete (*iter);
    }
    mCloudSyncFlowMap.clear();

    for(QMap<QString, InstallerFlow*>::iterator iter = mInstallerFlowMap.begin(); iter != mInstallerFlowMap.end(); iter++)
    {
        delete (*iter);
    }
    mInstallerFlowMap.clear();

    for (QMap<QString, EntitleFreeGameFlow*>::iterator iter = mEntitleFreeGameFlowMap.begin(); iter != mEntitleFreeGameFlowMap.end(); iter++)
    {
        delete (*iter);
    }
    mEntitleFreeGameFlowMap.clear();

    if(!mLocalContentViewController.isNull())
        delete mLocalContentViewController.take();
}


void ContentFlowController::endFlows()
{
    endUserSpecificFlows();

    for(QMap<QString, PlayFlow*>::iterator iter = mPlayFlowsMap.begin(); iter != mPlayFlowsMap.end(); iter++)
    {
        delete (*iter);
    }
    mPlayFlowsMap.clear();
}


void ContentFlowController::connectEntitlements()
{
    Engine::Content::ContentController* contentController = Engine::Content::ContentController::currentUserContentController();
    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
    Engine::Subscription::SubscriptionManager* sm = Engine::Subscription::SubscriptionManager::instance();

    mLocalContentViewController.reset(new LocalContentViewController());
    ORIGIN_VERIFY_CONNECT(mLocalContentViewController.data(), SIGNAL(restoreLocalBackupSave(const QString&)), this, SLOT(startSaveRestore(const QString&)));

    if(sm)
    {
        ORIGIN_VERIFY_CONNECT(sm, &Engine::Subscription::SubscriptionManager::entitleFailed,
            mLocalContentViewController.data(), &LocalContentViewController::showSubscriptionEntitleError);
        ORIGIN_VERIFY_CONNECT(sm, &Engine::Subscription::SubscriptionManager::upgradeFailed,
            mLocalContentViewController.data(), &LocalContentViewController::showSubscriptionUpgradeError);
        ORIGIN_VERIFY_CONNECT(sm, &Engine::Subscription::SubscriptionManager::downgradeFailed,
            mLocalContentViewController.data(), &LocalContentViewController::showSubscriptionDowngradeError);
        ORIGIN_VERIFY_CONNECT(sm, &Engine::Subscription::SubscriptionManager::revokeFailed,
            mLocalContentViewController.data(), &LocalContentViewController::showSubscriptionRemoveError);
    }
    else
    {
        ORIGIN_LOG_ERROR << "SubscriptionManager isn't valid. Can't set connections";
        ORIGIN_ASSERT(sm);
    }

    if(contentController)
    {
        ORIGIN_VERIFY_CONNECT(contentController->sharedNetworkOverrideManager(), &Engine::Shared::SharedNetworkOverrideManager::showConfirmWindow, 
            mLocalContentViewController.data(), &LocalContentViewController::showSharedNetworkOverridePrompt);
        ORIGIN_VERIFY_CONNECT(contentController->sharedNetworkOverrideManager(), &Engine::Shared::SharedNetworkOverrideManager::showFolderErrorWindow,
            mLocalContentViewController.data(), &LocalContentViewController::showSharedNetworkOverrideFolderError);
        ORIGIN_VERIFY_CONNECT(contentController->sharedNetworkOverrideManager(), &Engine::Shared::SharedNetworkOverrideManager::showInvalidTimeWindow,
            mLocalContentViewController.data(), &LocalContentViewController::showSharedNetworkOverrideInvalidTime);
        
        // kick of the initial content refresh cycle
        contentController->initUserGameLibrary();

        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(activeFlowsChanged(const QString&, const bool&)), this, SLOT(onActiveFlowsChanged(const QString&, const bool&)));

        //hook up signals to listen to here
        if(contentController->initialFullEntitlementRefreshed())
        {
            QList <Origin::Engine::Content::EntitlementRef> emptyList;
            onEntitlementsUpdated(contentController->entitlements(), emptyList);
        }

        //for future additions
        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));
        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(entitlementSignatureVerificationFailed()), mLocalContentViewController.data(), SLOT(showSignatureVerificationFailed()));
        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(autoDownloadUACFailed()), mLocalContentViewController.data(), SLOT(showUACFailed()));
        ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(brokenContentDetected()), mLocalContentViewController.data(), SLOT(showBrokenContentWarning()));
    }

    if(queueController)
    {
        ORIGIN_VERIFY_CONNECT_EX(queueController, SIGNAL(triedToAddGameWhileBusy(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), mLocalContentViewController.data(), SLOT(showQueueHeadBusyDialog(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(queueController, SIGNAL(headChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), this, SLOT(onOperationQueueHeadChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(queueController, SIGNAL(addedToComplete(Origin::Engine::Content::EntitlementRef)), mLocalContentViewController.data(), SLOT(onOperationQueueProgressChanged()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(queueController, SIGNAL(completeListCleared()), mLocalContentViewController.data(), SLOT(onOperationQueueProgressChanged()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(queueController, SIGNAL(removed(const QString&, const bool&)), mLocalContentViewController.data(), SLOT(onOperationQueueProgressChanged()), Qt::QueuedConnection);
    }
}


void ContentFlowController::onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> addedList, const QList<Origin::Engine::Content::EntitlementRef> deletedList)
{
    for(QList<Origin::Engine::Content::EntitlementRef>::ConstIterator it = addedList.constBegin();
        it != addedList.constEnd();
        it++)
    {
        Origin::Engine::Content::EntitlementRef entitlement = (*it);
        if (entitlement && entitlement->localContent())
        {
            mLocalContentViewController->connectEntitlement(entitlement);

            ORIGIN_VERIFY_CONNECT(entitlement->localContent()->cloudContent(), SIGNAL(remoteSyncStarted(Origin::Engine::Content::EntitlementRef)),
                this, SLOT(startCloudSyncFlow(Origin::Engine::Content::EntitlementRef)));

            if (entitlement->localContent()->installFlow() && entitlement->localContent()->installFlow()->isActive())
            {
                startInstallerFlow(entitlement->contentConfiguration()->productId());
            }

            if (entitlement->localContent()->playFlow())
            {
                ORIGIN_VERIFY_CONNECT(entitlement->localContent()->playFlow().data(), SIGNAL(startSaveBackup(const QString&)), this, SLOT(startSaveBackup(const QString&)));
            }
        }
    }
}


void ContentFlowController::onActiveFlowsChanged(const QString& productId, const bool& active)
{
    if(active)
    {
        startInstallerFlow(productId);
    }
    else
    {
        // We want to stop the flow from the flow itself.
    }
}

void ContentFlowController::startPlayFlow(const QString softwareId, const QString launchParameters)
{
    Origin::Engine::Content::EntitlementRef ent = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementBySoftwareId(softwareId);

    if (ent)
    {
        startPlayFlow(ent->contentConfiguration()->productId(), false, launchParameters);
    }
    else
    {
        ORIGIN_LOG_ERROR << "Unable to find entitlement by software ID: " << softwareId;
    }
}

void ContentFlowController::startPlayFlow(const QString& productId, const bool playnowwithoutupdate, const QString& launchParams, bool forceUpdateGame, bool /*forceUpdateDLC*/)
{
    Origin::Engine::Content::EntitlementRef ent = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);

    // Make sure productId is valid prior to play
    if(productId.isEmpty() || ent.isNull())
    {
        ORIGIN_LOG_ERROR << "Attempted to play invalid content. Product ID = " << productId;
        return;
    }

    if(!ent->localContent()->installed())
    {
        // For non-Origin content, give the user a chance to update the executable path before failing
        if(!ent->contentConfiguration()->isNonOriginGame())
        {
            mLocalContentViewController->showGameNotInstalled(ent);
        }
        else
        {
            if(mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpRepair))
            {
                mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpRepair)->focus();
            }
            else
            {
                NonOriginGameFlow* flow = new NonOriginGameFlow();
                if(flow)
                {
                    mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpRepair, flow);

                    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
                    flow->initialize(NonOriginGameFlow::OpRepair, ent);
                    flow->start();
                    ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Play flow started";
                }
            }
        }
        return;
    }

    Origin::Engine::Content::ContentConfigurationRef pContentConfig = ent->contentConfiguration();
    if(!pContentConfig)
    {
        ORIGIN_LOG_ERROR << "No content configuration found for ProductId = " << productId;
        return;
    }

    // Set the forced override for updating the game.
    pContentConfig->setTreatUpdatesAsMandatoryAlternate(forceUpdateGame);

    Origin::Engine::Content::LocalContent* pLocalContent = ent->localContent();
    if(!pLocalContent)
    {
        ORIGIN_LOG_ERROR << "No local content for ProductID = " << productId;
        return;
    }

    bool mandatoryUpdate = false;
    if(pLocalContent->treatUpdatesAsMandatory())
    {
        if(pLocalContent->updateAvailable())
        {
            ORIGIN_LOG_EVENT << "Mandatory update detected and available for ProductId = " << productId << ". Kicking off Install Flow instead.";
            mandatoryUpdate = true;
        }
    }


    PlayFlow* pFlow = NULL;

    QMap<QString, PlayFlow*>::const_iterator i = mPlayFlowsMap.find(productId);
    if(i == mPlayFlowsMap.constEnd())
    {
        pFlow = new PlayFlow(productId);
        mPlayFlowsMap.insert(productId, pFlow);

        ORIGIN_VERIFY_CONNECT(pFlow, SIGNAL(finished(PlayFlowResult)), this, SLOT(onPlayFlowFinished(PlayFlowResult)));

        if(mandatoryUpdate)
            pFlow->setPlayNowWithoutUpdates(false);
        else
            pFlow->setPlayNowWithoutUpdates(playnowwithoutupdate);

        pFlow->setLaunchParameters(launchParams);
        pFlow->start();
    }
}

void ContentFlowController::scheduleRelaunch(Origin::Engine::Content::EntitlementRef entitlement, bool forceUpdateGame)
{
    if(entitlement)
    {
        Origin::Client::Tasks::ScheduledTask * task = new Origin::Client::Tasks::RestartGameTask(entitlement, "", forceUpdateGame, false);

        task->run();
    }
}

PlayFlow* ContentFlowController::playFlow(const QString& productId)
{
    QMap<QString, PlayFlow*>::const_iterator i = mPlayFlowsMap.find(productId);
    return (i == mPlayFlowsMap.constEnd()) ? NULL : (*i);
}


CloudSyncFlow* ContentFlowController::cloudSyncFlow(const QString& productId)
{
    QMap<QString, CloudSyncFlow*>::const_iterator i = mCloudSyncFlowMap.find(productId);
    return (i == mCloudSyncFlowMap.constEnd()) ? NULL : (*i);
}


void ContentFlowController::modifyGameProperties(Origin::Engine::Content::EntitlementRef game)
{
    // Why are we coming all the way to the main flow to do this?
    // We should redo this and move it somewhere else.
    if(!game.isNull())
    {
        if(game->contentConfiguration()->isNonOriginGame())
        {
            if(mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpModify))
            {
                mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpModify)->focus();
            }
            else
            {
                NonOriginGameFlow* flow = new NonOriginGameFlow();
                if(flow)
                {
                    mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpModify, flow);

                    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
                    flow->initialize(NonOriginGameFlow::OpModify, game);
                    flow->start();
                }
            }
        }
        else
        {
            mLocalContentViewController->showGameProperties(game);
        }
    }
}


void ContentFlowController::customizeBoxart(Origin::Engine::Content::EntitlementRef game)
{
    if (!game.isNull())
    {
        if (!mCustomizeBoxartFlow.isNull())
        {
            mCustomizeBoxartFlow->focus();
        }
        else
        {
            mCustomizeBoxartFlow.reset(new CustomizeBoxartFlow());
            ORIGIN_VERIFY_CONNECT(mCustomizeBoxartFlow.data(), SIGNAL(finished(CustomizeBoxartFlowResult)), this, SLOT(onCustomizeBoxartFlowFinished(CustomizeBoxartFlowResult)));
            mCustomizeBoxartFlow->initialize(game);
            mCustomizeBoxartFlow->start();
        }
    }
}

void ContentFlowController::removeNonOriginGame(Origin::Engine::Content::EntitlementRef game)
{
    if(!game.isNull() && game->contentConfiguration()->isNonOriginGame())
    {
        if(mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpRemove))
        {
            mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpRemove)->focus();
        }
        else
        {
            NonOriginGameFlow* flow = new NonOriginGameFlow();
            if(flow)
            {
                mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpRemove, flow);

                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
                flow->initialize(NonOriginGameFlow::OpRemove, game);
                flow->start();
            }
        }
    }
}


void ContentFlowController::entitleFreeGame(Origin::Engine::Content::EntitlementRef game)
{
    if (!game.isNull())
    {
        const QString& productId = game->contentConfiguration()->productId();

        QMap<QString, EntitleFreeGameFlow*>::const_iterator i = mEntitleFreeGameFlowMap.find(productId);
        if (i == mEntitleFreeGameFlowMap.constEnd())
        {
            EntitleFreeGameFlow *pFlow = new EntitleFreeGameFlow(game);
            mEntitleFreeGameFlowMap.insert(productId, pFlow);

            ORIGIN_LOG_ACTION << "Attempting to entitle user to free game [" << productId << "]";
            ORIGIN_VERIFY_CONNECT(pFlow, &EntitleFreeGameFlow::finished, this, &ContentFlowController::onEntitleFreeGameFinished);
            pFlow->start();
        }
    }
}

void ContentFlowController::onEntitleFreeGameFinished(const EntitleFreeGameFlowResult& result)
{
    QMap<QString, EntitleFreeGameFlow*>::iterator iter = mEntitleFreeGameFlowMap.find(result.productId);
    if (iter != mEntitleFreeGameFlowMap.end())
    {
        EntitleFreeGameFlow* pFlow = *iter;
        mEntitleFreeGameFlowMap.erase(iter);
        pFlow->deleteLater();
    }

    if (result.result == FLOW_FAILED)
    {
        Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementById(result.productId, Engine::Content::AllContent);
        if (!ent.isNull())
        {
            mLocalContentViewController->showEntitleFreeGameError(ent);
        }
    }
}


void ContentFlowController::onPlayFlowFinished(PlayFlowResult result)
{
    ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Play flow finished for " << result.productId;
    // Remove it from the Play Flow Map
    QMap<QString, PlayFlow*>::iterator iter = mPlayFlowsMap.find(result.productId);
    if(iter != mPlayFlowsMap.end())
    {
        PlayFlow* pFlow = *iter;
        mPlayFlowsMap.erase(iter);
        pFlow->deleteLater();
    }
}


void ContentFlowController::onNonOriginGameFlowFinished(NonOriginGameFlowResult result)
{
    ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "NOG flow finished";
    // Operation can be changed during the flow based on user action, so we can't use sender()->currentOperation().
    NonOriginGameFlow* flow = static_cast<NonOriginGameFlow*>(sender());

    if(flow)
    {
        for(QMap<quint32, NonOriginGameFlow*>::iterator it = mNonOriginGameFlowsMap.begin(); it != mNonOriginGameFlowsMap.end(); ++it)
        {
            if(flow == *it)
            {
                mNonOriginGameFlowsMap.erase(it);
                break;
            }
        }
        flow->deleteLater();
    }
}

void ContentFlowController::onCustomizeBoxartFlowFinished(CustomizeBoxartFlowResult result)
{
    CustomizeBoxartFlow* flow = mCustomizeBoxartFlow.take();
    mCustomizeBoxartFlow.reset(NULL);
    flow->deleteLater();
}


void ContentFlowController::onAddNonOriginGame()
{
    if(mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpAdd))
    {
        mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpAdd)->focus();
    }
    else
    {
        NonOriginGameFlow* flow = new NonOriginGameFlow();
        if(flow)
        {
            mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpAdd, flow);

            ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
            flow->initialize(NonOriginGameFlow::OpAdd);
            flow->start();
        }
    }
}


bool ContentFlowController::startInstallerFlow(const QString& productId)
{
    bool success = false;
    Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementById(productId, Engine::Content::AllContent);
    // Make sure productId is valid prior to play
    if(productId.isEmpty() || ent.isNull())
    {
        ORIGIN_LOG_ERROR << "Attempted to download invalid content. Product ID = " << productId;
        return success;
    }

    InstallerFlow* pFlow = NULL;
    QMap<QString, InstallerFlow*>::const_iterator i = mInstallerFlowMap.find(productId);
    if(i == mInstallerFlowMap.constEnd())
    {
        pFlow = new InstallerFlow(ent);
        mInstallerFlowMap.insert(productId, pFlow);

        ORIGIN_VERIFY_CONNECT(pFlow, SIGNAL(finished(InstallerFlowResult)), this, SLOT(onInstallerFinished(InstallerFlowResult)));
        pFlow->start();
        success = true;
    }

    return success;
}


void ContentFlowController::onInstallerFinished(InstallerFlowResult result)
{
    // Remove it from the download flow map
    QMap<QString, InstallerFlow*>::iterator iter = mInstallerFlowMap.find(result.productId);
    if(iter != mInstallerFlowMap.end())
    {
        InstallerFlow* flow = *iter;
        mInstallerFlowMap.erase(iter);
        flow->deleteLater();
    }
}


bool ContentFlowController::startCloudSyncFlow(Origin::Engine::Content::EntitlementRef entRef)
{
    bool success = false;
    // Make sure productId is valid prior to cloud sync
    if(entRef.isNull())
    {
        ORIGIN_LOG_ERROR << "Attempted to start cloud sync on invalid content. ";
        return success;
    }

    CloudSyncFlow* pFlow = NULL;
    const QString productId = entRef->contentConfiguration()->productId();
    QMap<QString, CloudSyncFlow*>::const_iterator i = mCloudSyncFlowMap.find(productId);
    if(i == mCloudSyncFlowMap.constEnd())
    {
        pFlow = new CloudSyncFlow(entRef);
        mCloudSyncFlowMap.insert(productId, pFlow);

        ORIGIN_VERIFY_CONNECT(pFlow, SIGNAL(finished(CloudSyncFlowResult)), this, SLOT(onCloudSyncFinished(CloudSyncFlowResult)));
        pFlow->start();
        success = true;
    }

    return success;
}


void ContentFlowController::onCloudSyncFinished(CloudSyncFlowResult result)
{
    PlayFlow* playFlow = this->playFlow(result.productId);
    if(playFlow)
    {
        playFlow->setCloudSyncFinished(result.result == FLOW_SUCCEEDED);
    }

    // Remove it from the download flow map
    QMap<QString, CloudSyncFlow*>::iterator iter = mCloudSyncFlowMap.find(result.productId);
    if(iter != mCloudSyncFlowMap.end())
    {
        CloudSyncFlow* flow = *iter;
        flow->disconnect();
        mCloudSyncFlowMap.erase(iter);
        flow->deleteLater();
    }
}


bool ContentFlowController::startSaveRestore(const QString& productId)
{
    return startSaveBackupRestoreFlow(productId, SaveBackupRestoreFlow::RestoreLocalSave);
}


bool ContentFlowController::startSaveBackup(const QString& productId)
{
    return startSaveBackupRestoreFlow(productId, SaveBackupRestoreFlow::BackupLocalSave);
}


bool ContentFlowController::startSaveBackupRestoreFlow(const QString& productId, const SaveBackupRestoreFlow::SaveBackupRestoreAction& action)
{
    bool success = false;
    Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementById(productId, Engine::Content::AllContent);
    // Make sure productId is valid prior to backup
    if (ent.isNull())
    {
        ORIGIN_LOG_ERROR << "Attempted to start backup restore flow on invalid content. ";
        return success;
    }

    SaveBackupRestoreFlow* pFlow = NULL;
    QMap<QString, SaveBackupRestoreFlow*>::const_iterator i = mSaveBackupRestoreFlowMap.find(productId);
    if (i == mSaveBackupRestoreFlowMap.constEnd())
    {
        pFlow = new SaveBackupRestoreFlow(ent, action);
        mSaveBackupRestoreFlowMap.insert(productId, pFlow);

        ORIGIN_VERIFY_CONNECT(pFlow, SIGNAL(finished(const SaveBackupRestoreFlowResult&)), this, SLOT(onSaveBackupRestoreFinished(const SaveBackupRestoreFlowResult&)));
        switch (action)
        {
        case SaveBackupRestoreFlow::BackupRestoreLocalSave:
            pFlow->backupLocalAndRestoreLocalBackupSave();
            success = true;
            break;
        case SaveBackupRestoreFlow::BackupLocalSave:
            pFlow->backupLocalSaveFile();
            success = true;
            break;
        case SaveBackupRestoreFlow::RestoreLocalSave:
            pFlow->restoreLocalBackupSave();
            success = true;
            break;
        default:
            break;
        }
    }

    return success;
}


void ContentFlowController::onSaveBackupRestoreFinished(const SaveBackupRestoreFlowResult& result)
{
    // Remove it from the flow map
    QMap<QString, SaveBackupRestoreFlow*>::iterator iter = mSaveBackupRestoreFlowMap.find(result.productId);
    if (iter != mSaveBackupRestoreFlowMap.end())
    {
        PlayFlow* playFlow = this->playFlow(result.productId);
        if (playFlow)
        {
            Origin::Engine::Content::EntitlementRef ent = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(result.productId);
            emit ent->localContent()->playFlow()->cloudSaveBackupComplete();
        }
        SaveBackupRestoreFlow* flow = *iter;
        flow->disconnect();
        mSaveBackupRestoreFlowMap.erase(iter);
        flow->deleteLater();
    }
}


void ContentFlowController::onOperationQueueHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead)
{
    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
    if (oldHead.isNull() == false && oldHead->localContent() && oldHead->localContent()->installFlow())
    {
        ORIGIN_VERIFY_DISCONNECT(oldHead->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), mLocalContentViewController.data(), SLOT(onOperationQueueProgressChanged()));
    }
    if (newHead.isNull() == false && newHead->localContent() && newHead->localContent()->installFlow() && queueController && queueController->entitlementsQueued().length())
    {
        ORIGIN_VERIFY_CONNECT(newHead->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), mLocalContentViewController.data(), SLOT(onOperationQueueProgressChanged()));
    }

    if (mLocalContentViewController)
    {
        mLocalContentViewController->onOperationQueueProgressChanged();
    }
}


LocalContentViewController* ContentFlowController::localContentViewController() const
{
    return mLocalContentViewController.data();
}

} // namespace Client
} // namespace Origin
