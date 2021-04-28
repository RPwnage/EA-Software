///////////////////////////////////////////////////////////////////////////////
// SaveBackupRestoreFlow.cpp
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "SaveBackupRestoreFlow.h"

#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/cloudsaves/LocalStateCalculator.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "SaveBackupRestoreViewController.h"

namespace Origin
{
namespace Client
{

SaveBackupRestoreFlow::SaveBackupRestoreFlow(Engine::Content::EntitlementRef entitlement, const SaveBackupRestoreAction& saveBackupRestoreAction)
: mEntitlement(entitlement)
, mVC(new SaveBackupRestoreViewController(entitlement))
, mSaveBackupRestoreAction(saveBackupRestoreAction)
{
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(saveBackupRestoreFlowSuccess()), this, SLOT(onSaveBackupRestoreFlowSuccess()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mVC, SIGNAL(saveBackupRestoreFlowFailed()), this, SLOT(onSaveBackupRestoreFlowFailed()), Qt::QueuedConnection);
    // Safety bail out
    if (entitlement.isNull() || !entitlement->contentConfiguration())
    {
        ORIGIN_LOG_ERROR << "NOT VALID ENTITLEMENT";
    }
}


SaveBackupRestoreFlow::~SaveBackupRestoreFlow()
{
    if (mVC)
    {
        mVC->deleteLater();
        mVC = NULL;
    }
}


void SaveBackupRestoreFlow::start()
{
}


void SaveBackupRestoreFlow::backupLocalAndRestoreLocalBackupSave()
{
    // Confuse the backup functionality by renaming the current file to something else
    Engine::CloudSaves::LocalStateBackup localStateBackup(mEntitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    const bool fileExists = QFile::exists(localStateBackup.backupPath());
    if (fileExists)
    {
        QFile file(localStateBackup.backupPath());
        mSecondaryBackupWeNeedToRestore = localStateBackup.backupPath() + ".bk";
        file.rename(mSecondaryBackupWeNeedToRestore);
    }

    // Ok, now that we have tricked the code into thinking there isn't a backup, create a backup of current save.
    backupLocalSaveFile();

    // Restore the secondary file. 
    restoreLocalBackupSave();
}

            
bool SaveBackupRestoreFlow::backupLocalSaveFile()
{
    bool runBackup = false;
    Engine::CloudSaves::LocalStateBackup localStateBackup(mEntitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    runBackup = !QFile::exists(localStateBackup.backupPath());

    if (runBackup)
    {
        Engine::CloudSaves::LocalStateCalculator* localStateCalculator = new Engine::CloudSaves::LocalStateCalculator(mEntitlement, mEmptyState);
        ORIGIN_VERIFY_CONNECT_EX(localStateCalculator, SIGNAL(finished(Origin::Engine::CloudSaves::LocalStateSnapshot*)), this, SLOT(onLocalSaveStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot*)), Qt::QueuedConnection);
        localStateCalculator->start();
    }
    // Only kill the flow when we are JUST doing a backup. When we are getting here from backupLocalAndRestoreLocalBackupSave() we still need to do a restore
    else if (mSaveBackupRestoreAction == BackupLocalSave)
    {
        ORIGIN_LOG_ACTION << "Save backup: Not backing up current save.";
        onSaveBackupRestoreFlowFailed();
    }

    return runBackup;
}


bool SaveBackupRestoreFlow::isUpgradeBackupSaveAvailable(Engine::Content::EntitlementRef entitlement)
{
    //SHOULD WE PUT THIS BACK? SUBSCRIPTION LOCAL BACKUP https://developer.origin.com/support/browse/ORSUBS-1523
    //if (entitlement->contentConfiguration()->isEntitledFromSubscription())
    //{
    //    ORIGIN_LOG_ACTION << "isUpgradeBackupSaveAvailable: Entitlement is in upgraded state";
    //    return false;
    //}
    //if (entitlement->contentController()->hasEntitlementUpgraded(entitlement))
    //{
    //    ORIGIN_LOG_ACTION << "isUpgradeBackupSaveAvailable: Upgraded state in game library";
    //    return false;
    //}
    //Engine::CloudSaves::LocalStateBackup localStateBackup(entitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    //const bool debug_fileExists = QFile::exists(localStateBackup.backupPath());
    //return debug_fileExists;
    return false;
}


void SaveBackupRestoreFlow::restoreLocalBackupSave()
{
    Engine::CloudSaves::LocalStateBackup localStateBackup = Engine::CloudSaves::LocalStateBackup(mEntitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    if (mSecondaryBackupWeNeedToRestore.count())
    {
        localStateBackup.setBackupPath(mSecondaryBackupWeNeedToRestore);
    }
    QFutureWatcher<bool>* futureWatcher = new QFutureWatcher<bool>();
    ORIGIN_VERIFY_CONNECT(futureWatcher, SIGNAL(finished()), this, SLOT(onRestoreLocalBackupComplete()));

    futureWatcher->setFuture(localStateBackup.restore());
}


void SaveBackupRestoreFlow::onLocalSaveStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot* calculatedState)
{
    sender()->deleteLater();
    SaveBackupRestoreFlowResult result;
    result.productId = mEntitlement->contentConfiguration()->productId();

    if (!calculatedState)
    {
        if (mVC)
        {
            mVC->showSaveBackupRestoreFailed();
        }
        return;
    }

    // EA_TODO("jonkolb", "Preflight available space and bail if disk space is low")

    Engine::CloudSaves::LocalStateBackup localStateBackup(mEntitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    QFuture<bool> backup = localStateBackup.createBackup(calculatedState);

    // Only kill the flow when we are JUST doing a backup. When we are getting here from backupLocalAndRestoreLocalBackupSave() we still need to do a restore
    if (mSaveBackupRestoreAction == BackupLocalSave)
    {
        ORIGIN_VERIFY_CONNECT(&mBackupWatcher, SIGNAL(finished()), this, SLOT(onSaveBackupRestoreFlowSuccess()));
    }
    mBackupWatcher.setFuture(backup);
}


void SaveBackupRestoreFlow::onRestoreLocalBackupComplete()
{
    QFutureWatcher<bool>* futureWatcher = dynamic_cast<QFutureWatcher<bool>*>(sender());

    if (mVC == NULL)
    {
        sender()->deleteLater();
        return;
    }

    // Always delete the temp backup file (this is for the back/restore combo)
    if (mSecondaryBackupWeNeedToRestore.count() && QFile::exists(mSecondaryBackupWeNeedToRestore))
    {
        QFile file(mSecondaryBackupWeNeedToRestore);
        file.remove();
    }
    mSecondaryBackupWeNeedToRestore = "";

    if (futureWatcher)
    {
        const bool restoreSuccess = static_cast<bool>(futureWatcher->result());
        if (restoreSuccess)
        {
            ORIGIN_LOG_ACTION << "RESTORE SAVE BACKUP: Success";
            mVC->showSaveBackupRestoreSuccessful();

            // Only if we were successful and this is just a restore path should we delete the backed up file
            if (mSaveBackupRestoreAction == RestoreLocalSave)
            {
                Engine::CloudSaves::LocalStateBackup localStateBackup(mEntitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
                if (QFile::exists(localStateBackup.backupPath()))
                {
                    QFile file(localStateBackup.backupPath());
                    file.remove();
                }
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << "RESTORE SAVE BACKUP: Failed";
            mVC->showSaveBackupRestoreFailed();
        }
    }
    else
    {
        ORIGIN_LOG_ERROR << "RESTORE SAVE BACKUP: FUTURE WATCHER INVALID";
        mVC->showSaveBackupRestoreFailed();
    }

    if (sender())
    {
        sender()->deleteLater();
    }

    if (mEntitlement && mEntitlement->localContent())
    {
        // This is a way to refresh the my games page info.
        mEntitlement->localContent()->stateChanged(mEntitlement);
    }
}


void SaveBackupRestoreFlow::onSaveBackupRestoreFlowFailed()
{
    SaveBackupRestoreFlowResult result;
    result.productId = mEntitlement->contentConfiguration()->productId();
    result.result = FLOW_FAILED;
    finished(result);
}


void SaveBackupRestoreFlow::onSaveBackupRestoreFlowSuccess()
{
    SaveBackupRestoreFlowResult result;
    result.productId = mEntitlement->contentConfiguration()->productId();
    result.result = FLOW_SUCCEEDED;
    finished(result);
}

}
}