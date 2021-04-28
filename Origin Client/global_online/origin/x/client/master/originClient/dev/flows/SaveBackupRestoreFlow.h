///////////////////////////////////////////////////////////////////////////////
// SaveBackupRestoreFlow.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTSAVEBACKUPRESTOREFLOW_H
#define CLIENTSAVEBACKUPRESTOREFLOW_H

#include "flows/AbstractFlow.h"
#include "engine/content/ContentTypes.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include <QFutureWatcher>

namespace Origin
{
namespace Client
{

class SaveBackupRestoreViewController;
/// \brief Handles all high-level actions related to playing a game.
class ORIGIN_PLUGIN_API SaveBackupRestoreFlow : public AbstractFlow
{
    Q_OBJECT

public:
    enum SaveBackupRestoreAction
    {
        BackupRestoreLocalSave,
        BackupLocalSave,
        RestoreLocalSave
    };
    /// \brief The SaveBackupRestoreFlow constructor.
    /// \param productId The product ID of the game to play.
    SaveBackupRestoreFlow(Engine::Content::EntitlementRef entitlement, const SaveBackupRestoreAction& saveBackupRestoreAction);

    /// \brief The SaveBackupRestoreFlow destructor.
    virtual ~SaveBackupRestoreFlow();

    /// \brief Public interface to start the play flow.
    virtual void start();

    /// \brief Slot that is triggered when user wants to restore a local save backup
    void backupLocalAndRestoreLocalBackupSave();
    // Entitlement have default backfilenames.
    void restoreLocalBackupSave();
    bool backupLocalSaveFile();
    static bool isUpgradeBackupSaveAvailable(Engine::Content::EntitlementRef entitlement);

signals:
    void finished(const SaveBackupRestoreFlowResult& result);

private slots:
    /// \brief Slot that is triggered when user wants to restore a local save backup
    void onRestoreLocalBackupComplete();
    void onLocalSaveStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot* calculatedState);
    void onSaveBackupRestoreFlowFailed();
    void onSaveBackupRestoreFlowSuccess();

private:
    Engine::Content::EntitlementRef mEntitlement;
    QFutureWatcher<bool> mBackupWatcher;
    Engine::CloudSaves::LocalStateSnapshot mEmptyState;
    SaveBackupRestoreViewController* mVC;
    const SaveBackupRestoreAction mSaveBackupRestoreAction;
    QString mSecondaryBackupWeNeedToRestore;
};

}
}
#endif