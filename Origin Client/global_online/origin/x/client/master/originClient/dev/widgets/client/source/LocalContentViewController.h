/////////////////////////////////////////////////////////////////////////////
// LocalContentViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef LOCALCONTENTVIEWCONTROLLER_H
#define LOCALCONTENTVIEWCONTROLLER_H

#include <QObject>
#include <QMap>
#include "engine/content/Entitlement.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
namespace Client
{
class ORIGIN_PLUGIN_API LocalContentViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the LocalContentViewController.
    LocalContentViewController(QObject* parent = 0);

    /// \brief Destructor
    ~LocalContentViewController();

    void connectEntitlement(Engine::Content::EntitlementRef entitlement);

    /// \brief Shows the error dialog that the user is trying to launch a game that isn't installed
    void showGameNotInstalled(Engine::Content::EntitlementRef entitlement);

    /// \brief Show a window that lets the user change a games properties.
    void showGameProperties(Engine::Content::EntitlementRef entitlement);

    /// \brief Tells the user to install base game before installing addon content.
    void showParentNotInstalledPrompt(Engine::Content::EntitlementRef entitlement);

    /// \brief Tells the user that they can't save their server settings at this time. Server is most likely down.
    void showServerSettingsUnsavable();

    /// \brief Verifies that the user actually wants to remove a game from their catalog.
    void showRemoveEntitlementPrompt(Engine::Content::EntitlementRef entitlement, const bool& showUninstall);

    /// \brief Informs user that we can not restore their Trashed game.
    void showCanNotRestoreFromTrash(Engine::Content::EntitlementRef entitlement);

    /// \brief Handles quick uninstall of entitlement.
    void showUninstallConfirmation(Engine::Content::EntitlementRef entitlement);

    /// \brief Handles quick uninstall of a DLC entitlement.
    void showUninstallConfirmationForDLC(Engine::Content::EntitlementRef entitlement);

    /// \brief Show download debug information on a specific entitlement
    void showDownloadDebugInfo(Engine::Content::EntitlementRef entitlement);

    /// \brief Show update debug information on a specific entitlement
    void showUpdateDebugInfo(Engine::Content::EntitlementRef entitlement);

    /// \brief Show warning about save games getting messed up by upgrading game.
    void showUpgradeWarning(Engine::Content::EntitlementRef entitlement);

    /// \brief Show message about upgrading game. Messaging is different from DLC.
    void showDLCUpgradeStandard(Engine::Content::EntitlementRef entitlement, const QString& dlcName);

    /// \brief Show message about restoring backed up save game.
    void showSaveBackupRestoreWarning(Engine::Content::EntitlementRef entitlement);

    /// \brief Show warning about save games getting messed up by upgrading game. Messaging is different from DLC.
    void showDLCUpgradeWarning(Engine::Content::EntitlementRef entitlement, const QString& childOfferID, const QString& dlcName);

    /// \brief Show error when user tries to entitle a free game but the server request fails.
    void showEntitleFreeGameError(Engine::Content::EntitlementRef entitlement);

    /// \brief Show floating (error) window. Please avoid using this. It's most for when the flow
    /// that should be owning the error is killed before the window is closed.
    void showFloatingErrorWindow(const QString& title, const QString& text, const QString& key, const QString& source);

    static Downloader::ContentInstallFlowState possibleMaskInstallState(Engine::Content::EntitlementRef entitlement);

public slots:
    /// \brief Slot that is triggered when the user's entitlement signature could not be verified.
    void showSignatureVerificationFailed();

    /// \brief Slot that is triggered when an Free Trial expiration notification is sent
    /// \param timeRemainingInMinutes The number of minutes still remaining in the free trial
    void showTrialEnding(int);

    /// \brief Slot that is triggered when an Timed Trial expiration notification is sent
    /// \param timeRemainingInMinutes The number of minutes still remaining in the free trial
    void showTimedTrialEnding(const int&);

    /// \brief Slot that is triggered when user went offline while playing a trial game
    /// \param entitlement trial that was active
    void showTimedTrialOffline(const Origin::Engine::Content::EntitlementRef ent);

    /// \brief Slot that is triggered when clients goes online. Dialogs that were opened with showTimedTrialOffline are closed.
    void closeAllTimedTrialOffline();

    /// \brief Slot that is triggered when sdk tells us trial is over for an entitlement.
    void closeTimedTrialEnding(const Origin::Engine::Content::EntitlementRef entitlement);

    /// \brief Slot that is triggered there is an error from an entitlement's Local Content.
    /// \param int/LocalContent::Error The number that is the error sent to us.
    void showError(Origin::Engine::Content::EntitlementRef, int);

    /// \brief Handles when UAC failed or was denied.
    void showUACFailed();

    /// \brief Handles when Origin has detected broken content.
    void showBrokenContentWarning();

    /// \brief Shows when the content operation queue head is busy and we tried to push to the top
    void showQueueHeadBusyDialog(Origin::Engine::Content::EntitlementRef head, Origin::Engine::Content::EntitlementRef ent);

    /// \brief Shows a confirmation prompt for setting a new shared network override.
    void showSharedNetworkOverridePrompt(Engine::Content::EntitlementRef ent, const QString overridePath, const QString lastModifiedTime);

    /// \brief Shows an error window when the shared network override is set to a directory that is invalid.
    void showSharedNetworkOverrideFolderError(Engine::Content::EntitlementRef ent, const QString& path);

    /// \brief Shows an error window when the shared network override has an invalid scheduled time.
    void showSharedNetworkOverrideInvalidTime(Engine::Content::EntitlementRef ent);

    /// \brief Shows error which tells the user there was an issue with entitling a subscription entitlement.
    void showSubscriptionEntitleError(const QString& offerId, Engine::Subscription::SubscriptionRedemptionError error);

    /// \brief Shows error which tells the user there was an issue with downgrading/removing entitlement.
    void showSubscriptionRemoveError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error);

    /// \brief Slot that is triggered when content queue controller's head progress changes.
    void onOperationQueueProgressChanged();

    /// \brief Shows error which tells the user the downgrade for a given masterTitle is not available.
    void showSubscriptionDowngradeError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error);

    /// \brief Shows error which tells the user there was an issue with upgrading their subscription entitlement.
    void showSubscriptionUpgradeError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error);

signals:
    void restoreLocalBackupSave(const QString& productId);


protected slots:
    void onRemoveWindow();
    void onUninstall(const QString& productId);
    void onUninstallCancelled(const QString& productId);
    void onRemoveEntitlement(const QString& masterTitleId);
    void onUpgrade(const QString& productId);

    /// \brief Enqueues the parent to update and then enqueues entitlement for download
    void updateParentAndDownload(QJsonObject);
    void onRetryDownloads(QJsonObject);
    void onDownloadOrInstallParent(QJsonObject);
    void onUninstallConfirmationDone(QJsonObject);

private:
    /// \brief Returns true if the window with that key is found and shows it. If no window is found - false.
    bool containsAndShowWindowIfAlive(const QString& key);
    UIToolkit::OriginWindow* pushAndShowWindow(UIToolkit::OriginWindow* window, const QString& key, const bool& showInOIG = false);

    QMap<QString, UIToolkit::OriginWindow*> mWindowMap;
};
}
}
#endif
