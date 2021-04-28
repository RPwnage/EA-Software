///////////////////////////////////////////////////////////////////////////////
// ContentFlowController.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CONTENTFLOWCONTROLLER_H
#define CONTENTFLOWCONTROLLER_H

#include <QObject>
#include <QMap>
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"
#include "AbstractFlow.h"
#include "SaveBackupRestoreFlow.h"

namespace Origin
{
namespace Client
{

class PlayFlow;
class NonOriginGameFlow;
class InstallerFlow;
class CloudSyncFlow;
class LocalContentViewController;
class CustomizeBoxartFlow;
class EntitleFreeGameFlow;

class ORIGIN_PLUGIN_API ContentFlowController : public QObject
{
	Q_OBJECT

public:
    /// \brief The ContentFlowController constructor.
	ContentFlowController();

    ~ContentFlowController();



    /// \brief Gets the PlayFlow object for the given entitlement.
    /// \param productId The product ID of the entitlement.
    /// \return PlayFlow The PlayFlow for the given entitlement.
    PlayFlow* playFlow(const QString& productId);

    /// \brief Returns matching cloud sync flow from given product Id.
    CloudSyncFlow* cloudSyncFlow(const QString& productId);

    /// \brief Starts the non-Origin game modify properties flow
    ///
    /// \param game The Origin game whose properties will be modified
    void modifyGameProperties(Origin::Engine::Content::EntitlementRef game);

    /// \brief Starts the non-Origin customize boxart flow
    ///
    /// \param game The non-Origin game whose boxart will be modified
    void customizeBoxart(Origin::Engine::Content::EntitlementRef game);

    /// \brief Starts the non-Origin game remove flow.
    ///
    /// \param game The non-Origin game whose properties need to be removed.
    void removeNonOriginGame(Origin::Engine::Content::EntitlementRef game);

    /// \brief Attempts to entitle the user to a free game.
    ///
    /// \param game The game to be entitled.
    void entitleFreeGame(Origin::Engine::Content::EntitlementRef game);

    /// \brief Starts the download flow for the given entitlement.
    /// \param productId The product ID of the entitlement to download.
    bool startInstallerFlow(const QString& productId);

    void connectEntitlements();

    void endUserSpecificFlows();

    void endFlows();

    LocalContentViewController* localContentViewController() const;

public slots:

    /// \brief Overloaded method that allows passing of software ID instead.
    void startPlayFlow(const QString softwareId, const QString launchParameters);

    /// \brief Starts the play flow for the given entitlement.
    /// \param productId The product ID of the entitlement to play.
    /// \param playnowwithoutupdate True if the entitlement should play without updating.
    /// \param launchParameters Any launch parameters to pass to the game executable.
    /// \param forceUpdateGame Do not give the user the option to skip game updates.
    /// \param forceUpdateDLC Do not give the user the option to skip updates to DLC (not implemented yet).
    void startPlayFlow(const QString& productId, const bool playnowwithoutupdate, const QString& launchParameters = QString(), bool forceUpdateGame = false, bool forceUpdateDLC = false);

    /// \brief Activates cloud sync Ui
    bool startCloudSyncFlow(Origin::Engine::Content::EntitlementRef entRef);

    /// \brief Start save backup flow
    bool startSaveRestore(const QString& productId);
    bool startSaveBackup(const QString& productId);

    /// \brief Schedule a relaunch for the game after the current running instance terminates.
    /// \param entitlement The entitlement associated with the current running game.
    /// \param forceUpdateGame When this is true perform an update on the entitlement.
    /// \param forceUpdateDLC When this is true, all the installed DLC will be checked for updates, and updated if necessary.
    /// \param commandLineArgs Additional command line parameters for when the game is restarted.
    void scheduleRelaunch(Origin::Engine::Content::EntitlementRef entitlement, bool forceUpdateGame);


protected slots:
    /// \brief Slot that is triggered when the play flow finishes.
    /// \param result The result of the play flow.
    void onPlayFlowFinished(PlayFlowResult result);

    /// \brief Slot that is triggered when the non-Origin Game flow finishes.
    /// \param result The result of the non-Origin Game flow.
    void onNonOriginGameFlowFinished (NonOriginGameFlowResult result);

    /// \brief Slot that is triggered when the customize boxart flow finishes.
    /// \param result The result of the customize boxart flow
    void onCustomizeBoxartFlowFinished(CustomizeBoxartFlowResult result);

    /// \brief Slot that is triggered when the user requests to add a non origin game to the library.
    void onAddNonOriginGame();

    /// \brief Slot that is triggered when the download flow finishes.
    /// \param result The result of the download flow.
    void onInstallerFinished(InstallerFlowResult result);

    /// \brief Slot that is triggered when the entitlements have been up
    void onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> addedList, const QList<Origin::Engine::Content::EntitlementRef> deletedList);

    /// \brief Slot that is triggered when the user's entitlements are updated.
    /// \param addedList List of entitlements that were newly added.
    /// \param deletedList List of entitlements that were removed.
    void onActiveFlowsChanged(const QString& productId, const bool& active);

    /// \brief Slot that is triggered when the cloud sync flow finishes.
    /// \param result The result of the cloud sync flow.
    void onCloudSyncFinished(CloudSyncFlowResult result);

    /// \brief Slot that is triggered when the save backup restore flow finishes.
    /// \param result The result of the cloud sync flow.
    void onSaveBackupRestoreFinished(const SaveBackupRestoreFlowResult& result);

    /// \brief Slot that is triggered when the entitle free game flow finishes.
    /// \param result The result of the entitle free game flow.
    void onEntitleFreeGameFinished(const EntitleFreeGameFlowResult& result);

    /// \brief Slot that is triggered when content queue controller's head changes.
    void onOperationQueueHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead);

private:
    bool startSaveBackupRestoreFlow(const QString& productId, const Origin::Client::SaveBackupRestoreFlow::SaveBackupRestoreAction& action = SaveBackupRestoreFlow::BackupRestoreLocalSave);

    QMap<QString, PlayFlow*>                    mPlayFlowsMap; ///< Map of all play flows.
    QMap<quint32, NonOriginGameFlow*>           mNonOriginGameFlowsMap; ///< Map of all non-Origin game flows by operation.
    QMap<QString, InstallerFlow*>               mInstallerFlowMap; ///< Map of all install flows.
    QMap<QString, CloudSyncFlow*>               mCloudSyncFlowMap; ///< Map of all cloud sync flows.
    QMap<QString, SaveBackupRestoreFlow*>       mSaveBackupRestoreFlowMap; ///< Map of all save backup restore flows.
    QMap<QString, EntitleFreeGameFlow*>         mEntitleFreeGameFlowMap; ///< Map of all entitle free game flows.
    QScopedPointer<LocalContentViewController>  mLocalContentViewController; ///< Pointer to the local content error catch all view controller.
    QScopedPointer<CustomizeBoxartFlow>         mCustomizeBoxartFlow; ///< Pointer to the CustomizeBoxartFlow object.
};
}
}
#endif