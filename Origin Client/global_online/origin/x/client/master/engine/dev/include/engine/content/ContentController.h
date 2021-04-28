//    Copyright (c) 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef CONTENTCONTROLLER_H
#define CONTENTCONTROLLER_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QPixmap>
#include <QtNetwork/QNetworkReply>
#include <QFutureWatcher>
#include <QMap>
#include <QString>

#include "engine/content/LocalContent.h"
#include "engine/content/Entitlement.h"
#include "engine/content/CustomBoxartController.h"
#include "engine/content/NonOriginContentController.h"
#include "engine/content/NucleusEntitlementController.h"
#include "engine/content/CatalogDefinitionController.h"
#include "engine/content/BaseGamesController.h"
#include "engine/config/SharedNetworkOverrideManager.h"

#include "services/platform/PlatformService.h"
#include "services/plugin/PluginAPI.h"

#include "../controller/source/ContentFolders.h"

namespace Origin 
{
    namespace Engine
    {
        class User;

        namespace Content
        {
            /// \class ContentController
            /// \brief TBD.
            class ORIGIN_PLUGIN_API ContentController : public QObject
            {
                Q_OBJECT
            public:
                /// \brief The ContentController destructor; releases the resources used by the ContentController.
                virtual ~ContentController();

                /// \brief Creates a new entitlement controller.
                ///
                /// \param  user  The user associated with these entitlements. 
                /// \returns ContentController Returns a pointer to the newly created entitlement controller.
                static ContentController * create(Origin::Engine::UserRef user);

                /// \brief TBD.
                ///
                /// \returns ContentController Returns a pointer to the newly created entitlement controller.
                static ContentController * currentUserContentController();
            public:

                /// \enum EntitlementLookupType
                /// \brief TBD.
                enum EntitlementLookupType
                {
                    EntByOfferId,       ///< Interpret passed ID(s) as offer ID(s).
                    EntByContentId,     ///< Interpret passed ID(s) as content ID(s).
                    EntByMasterTitleId, ///< Interpret passed ID(s) as master title ID(s).
                    EntById             ///< Interpret passed ID(s) as content, master title, or offer ID(s).
                };

            public:
                /// \brief Initializes the user's game library.
                void initUserGameLibrary();

                /// \brief Create shared network override objects
                void setupSharedNetworkOverrides();

                /// \brief Refreshes the user's game library according to the given mode
                Q_INVOKABLE void refreshUserGameLibrary(Origin::Engine::Content::ContentRefreshMode mode = Origin::Engine::Content::RenewAll);

                /// \brief Returns the user associated with this set of entitlements.
                ///
                /// \return UserRef The user associated with the entitlements. 
                Origin::Engine::UserRef user() const;

                /// \brief Returns the number of non-Origin game entitlements
                ///
                /// \return int The number of entitlements.
                int numberOfNonOriginEntitlements();

                /// \brief Returns the list of non-Origin game ids
                /// 
                /// \return A list of the non Origin game ids
                QStringList nonOriginEntitlementIds();

                /// \brief A singular place to initialize the IGO for a title.
                /// \param config The entitlement to run.
                void prepareIGO(const Origin::Engine::Content::ContentConfigurationRef config);

                ///
                /// \brief Returns the entitlements as a list of QSharedPointers to Entitlements
                /// \param filter Filter enum describing which content should be included in the operation.
                ///
                EntitlementRefList entitlements(EntitlementFilter filter = OwnedContent) const;

                /// \brief Returns the list of all entitlements filtered on states.
                ///
                /// Returns entitlements that have at least one of the states given in any,
                /// all of the states given in all, and none of the states given in none.
                /// Each condition is disabled by passing in zero for any, all or none, so
                /// that by default all entitlements are returned.
                EntitlementRefList entitlementByState(LocalContent::States any = 0, LocalContent::States all = 0, LocalContent::States none = 0, EntitlementFilter filter = OwnedContent) const;

                /// \brief Returns the entitlements that are downloading/paused as a map of QSharedPointers indexed by folder paths.
                ///
                const QMultiHash<QString, EntitlementRef> & entitlementsWithActiveFlows() const { return m_productIdToEntitlementRefHash; }

                /// \brief Returns the entitlement given a generic ID.
                /// 
                /// Only use this function if you are given an ID with no context.
                /// 
                /// \param id The ID.  Can be a content ID or product ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that ID.
                EntitlementRef entitlementById(const QString &id, EntitlementFilter filter = OwnedContent);

                /// \brief Returns a list of entitlements given a generic ID.
                /// 
                /// Only use this function if you are given an ID with no context.
                /// 
                /// \param id The ID.  Can be a content ID or product ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return A list of EntitlementRefs that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsById(const QString &id, EntitlementFilter filter = OwnedContent);

                /// \brief Returns a list of entitlements that have this id in the manifest as the full game.
                /// 
                /// Only use this function if you are given an ID with no context.
                /// 
                /// \param contentId The contentId to search against.  
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return A list of EntitlementRefs that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsByTimedTrialAssociation(const QString &contentId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement given a content ID.
                /// \param contentId The content ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that content ID.
                EntitlementRef entitlementByContentId(const QString &contentId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlements given a content ID.
                /// \param contentId The content ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return A list of QSharedPointers that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsByContentId(const QString &contentId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement base game with given master title ID.
                /// \param contentId The master title ID.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that master title ID.

                /// \brief Returns the entitlement base games with given master title ID.
                /// \param contentId The master title ID.
                /// \return A list of EntitlementRefs that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsBaseByMasterTitleId(const QString& masterTitleID);

                /// \brief Returns the entitlement base games with given master title ID. Excludes entitlements entitled by subscriptions
                /// \param contentId The master title ID.
                /// \return A list of EntitlementRefs that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsBaseByMasterTitleId_ExcludeSubscription(const QString& masterTitleID);

                /// \brief Returns the entitlements given a master title id.
                /// \param masterTitleId The master title id.
                /// \param includeAlternateMatches If true, this search will also include entitlements with the given master title id as an alternate master title.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return A list of EntitlementRefs that match the entitlement. An empty list if no entitlements are found.
                EntitlementRefList entitlementsByMasterTitleId(const QString &masterTitleId, bool includeAlternateMatches = false, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement given a multiplayer ID.
                /// \param multiplayerId TBD.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that content ID.
                ///
                EntitlementRef entitlementByMultiplayerId(const QString &multiplayerId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement given a product ID.
                /// \param productId The product ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that product ID.
                EntitlementRef entitlementByProductId(const QString &productId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement given a software ID.
                /// \param softwareId The software ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that software ID.
                EntitlementRef entitlementBySoftwareId(const QString &softwareId, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the best matching entitlement given a list of IDs.  If multiple matches are found, this function will 
                ///        return the most relevant function using the following algorithm:
                ///        Playable entitlement > Installed entitlement > Any matching entitlement
                /// \param ids The given IDs.
                /// \param lookupType A description of the given ID.
                /// \param baseGamesOnly If true, this function will only consider base games in its calculations.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that product ID.
                EntitlementRef bestMatchedEntitlementByIds(const QStringList &ids, EntitlementLookupType lookupType = EntById, bool baseGamesOnly = false, EntitlementFilter filter = OwnedContent);

                /// \brief Returns the entitlement or a child entitlement given a product ID.
                /// \param productId The product ID.
                /// \param filter Filter enum describing which content should be included in the operation.
                /// \return QSharedPointer A QSharedPointer to the entitlement. NULL if no entitlement is found with that product ID.
                EntitlementRef entitlementOrChildrenByProductId(const QString &productId, EntitlementFilter filter = OwnedContent);

                /// \brief Checks to see if a folder path is in use.
                ///
                /// \param folderPath The folder path you want to check to see if something is downloading into it.
                /// \return bool True if the specified folder path is in use.
                bool folderPathInUse(const QString &folderPath);

                /// \brief Checks to see if a any non-DiP flow is active.
                ///
                /// Checks to see if content folder is valid, emits invalidContentFolders and returns false if not
                ///
                /// \param validateDipFolder If true it validates the download in place folder, if false it validates cache folder
                /// \return true if folder is valid, false otherwise
                ///
                bool validateContentFolder(bool validateDipFolder);

                ///
                /// Retrieves the entitlements currently using a specified folder.
                ///
                /// \param folderPath the folder path you want to check to see if something is downloading into it
                /// \return A list of the entitlements using the folder
                EntitlementRefList getEntitlementsAccessingFolder(const QString &folderPath);

                ///
                ///
                /// \return bool True if a non-DiP flow is active.
                ///
                bool isNonDipFlowActive();

                /// \brief Checks to see if a any DiP flow is active.
                ///
                /// \return bool True if a DiP flow is active.
                bool isDipFlowActive();

                /// \brief Similar to isDipFlowActive() but handles a special case check for ITO installs
                /// so that we can allow the user to change the DipInstallLocation from the ITO Prepare Download dialog.
                ///
                /// \return bool True if the only active flow is an ITO install AND it is in the pendingInstallInfo state.
                bool canChangeDipInstallLocation();

                /// \brief Stops all active dip flows.
                void stopAllNonDipFlows();

                /// \brief Stops all active dip flows that have non-changeable folders.
                ///
                /// That is, makes a special case check for the ITO case. If the user chose to change the
                /// folder during the ITO Prepare Download dialog, we allow them to.
                void stopAllDipFlowsWithNonChangeableFolders();

                /// \brief Attempts to pause all active install flows.
                ///
                /// If a timeout is specified, this function will emit lastActiveFlowPaused when all flows are
                /// paused or the operation times out.
                /// \param autoresume If set to true, all paused flows will begin automatically at next startup.
                /// \param timeout The number of milliseconds after which to time out; only valid if > 0.
                void suspendAllActiveFlows(bool autoresume, int timeout = 0);

                /// \brief Properly clean up all the install flows.
                void deleteAllContentInstallFlows();

                /// \brief Indicates whether auto-patching is enabled.
                ///
                /// \return bool True is auto-patching is enabled.
                bool autoPatchingEnabled() const;

                /// \brief Takes in QString ID to get game title
                ///
                /// \return QString gametitle
                QString getDisplayNameFromId(QString const& id);

                /// \brief Collects/Starts updates for all qualifying entitlements.
                ///                
                QStringList collectAutoPatchInstalledContent();
                void processAutoPatchInstalledContent(const QStringList& pendingDownloads, bool uacFailed);

                /// \brief Check whether we have pending PDLCs/Starts auto PDLC downloads
                ///
                QSet<QString> collectAutoStartedPDLC();
                void processAutoStartedPDLC(QSet<QString>& pendingDownloads, bool uacFailed);

                QStringList collectBrokenInstalledContent(bool sendTelemetry);
                void processBrokenInstalledContent(const QStringList& pendingBrokenContent, bool uacFailed);

                /// \brief Starts updates for a single entitlement.
                ///                
                bool autoPatchInstalledContent(bool startDownload, bool &permissionsRequested, bool &uacFailed, EntitlementRef entitlement, QStringList& autoUpdatingList);

                /// \brief Starts repair for a single entitlement.
                ///
                bool autoRepairInstalledContent(bool startRepair, bool &permissionsRequested, bool &uacFailed, EntitlementRef entitlement, QStringList& autoRepairList, bool sendNeedRepairTelemetry);

                /// \brief Deletes all installers in the cache folder.
                ///  
                void deleteAllInstallerCacheFiles();

                /// \brief Checks if the initial base games refresh has been completed.
                ///
                /// \return bool True if the initial base games refresh has been completed.
                bool initialEntitlementRefreshCompleted() const;

                /// \brief Checks if the initial full (extra content included) entitlement refresh has been completed.
                ///
                /// \return bool True if the initial full entitlement refresh has been completed.
                bool initialFullEntitlementRefreshed() { return m_InitialFullEntitlementRefreshed; }

                /// \brief Download some content specified by the content id.
                /// \param id The content or offer id of the content to download
                /// \param installLocale The locale to use for the install. Defaults to the user's current locale.
                ///
                /// \return bool True if the download was able to start
                bool downloadById(const QString& id, const QString& installLocale = "");

                /// \brief On the next refresh, the specified content will automatically begin downloading.
                /// \param id The content or offer id of the content to download
                void autoStartDownloadOnNextRefresh(const QString& id);

                /// \brief On the next refresh, the specified DLC content will begin downloading.
                /// \param id The content or offer id of the DLC content to download
                void forceDownloadPurchasedDLCOnNextRefresh(const QString& id);

                /// \brief Indicates whether an entitlement update is in progress.
                ///
                /// \return bool True if an entitlement update is in progress.
                bool updateInProgress() const;

                /// \brief Is the user playing a game.
                bool isPlaying() const;

                /// \brief Sets the play and ready to play state of externally started
                /// content that is not wrapped in RtP.
                Q_INVOKABLE void sdkConnectionEstablished(const QString &productId, const QString& name);

                /// \brief TBD.
                Q_INVOKABLE void sdkConnectionLost(const QString &productId);

                /// \brief TBD.
                ACCESSOR(ContentFolders, contentFolders);

                /// \brief A controller for non Origin content
                ACCESSOR(NonOriginContentController, nonOriginController);

                /// \brief A controller for customized boxart
                ACCESSOR(CustomBoxartController, boxartController);

                ACCESSOR(NucleusEntitlementController, nucleusEntitlementController);

                ACCESSOR(BaseGamesController, baseGamesController);

                const QStringList&  entitlementsConnectedToSDK() { return mEntitlementsConnectedToSDK; }

                bool isLoadingEntitlementsSlow();

                /// \brief Performs any necessary download preflighting and returns the result of the preflight.
                enum DownloadPreflightingResult { ABORT_DOWNLOAD, CONTINUE_DOWNLOAD, USER_OVERRIDE };
                void doDownloadPreflighting(Origin::Engine::Content::LocalContent const& content, DownloadPreflightingResult& result);

                void checkIfOfferVersionIsUpToDate(Origin::Engine::Content::EntitlementRef entitlement);

                void updateExtraContentInfo(bool requestRefreshFromServer);

                bool hasExtraContent();

                bool isAutoStartOK() { return m_autoStartItemsOK; }

                void extraContentRequestReturned(const QString& masterTitleID);

                /// \brief EBIBUGS-27997: Used during the InstallerFlow when we want to change the install path of an entitlement
                /// when there isn't enough disc space. We need to remove the old path from the entitlement so if the
                /// user cancels and changes it back - we won't say that the path is busy.
                bool removeInstallPathToEntitlement(const QString& productId, QString& oldDownloadPath);

                Shared::SharedNetworkOverrideManager* sharedNetworkOverrideManager() { return m_sharedNetworkOverrideManager; }

                /// \brief Returns true if the user is entitled to an upgrade version of passed in entitlement.
                /// \param entitlement The entitlement to check.
                /// \param suppressingEntitlement The entitlement upgraded version of the entitlement.
                bool hasEntitlementUpgraded(EntitlementRef entitlement, EntitlementRef* upgradedEntitlement = NULL);

                /// \brief Returns true if the given base game offer should be suppressed because a subscription-based
                ///        base game entitlement exists with the same master title id.
                /// \param entitlement The entitlement to check.
                /// \param suppressingEntitlement The entitlement that is suppressing the passed in entitlement.
                bool isEntitlementSuppressed(EntitlementRef entitlement, EntitlementRef * suppressingEntitlement = NULL);

            signals:
                /// \brief Signal emitted when base games have been loaded and initialized
                void entitlementsInitialized();

                /// \brief Signal emitted when *all* content (base games and extracontent) has been loaded and initialized
                void initialFullUpdateCompleted();

                void offerVersionUpdateCheckComplete(QString offerId, bool isOfferUpToDate);
                
                /// \brief Emits a signal after the auto update check is done.
                ///
                /// \param started Set to true if auto update started on any entitlement.
                /// \param autoUpdateList A list of the contentIds that are being auto-updated.
                void autoUpdateCheckCompleted(bool started, QStringList autoUpdateList);

                /// \brief Emits a signal before attempting to update entitlements.
                void updateStarted();
                
                /// \brief Emits a signal when the entitlements are ready to be accessed and
                /// all processing has been completed.
                ///
                /// We pass a list of entitlements that have been added since the last refresh,
                /// as well as a list of entitlements that have been deleted.
                void updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>);
                
                /// \brief Emits a signal when updating the entitlements has failed.
                void updateFailed();
                
                /// \brief Emits a signal when entitlement signatures fail to verify.
                void entitlementSignatureVerificationFailed();
                
                /// \brief Emits a signal when the user has started playing a game.
                void playStarted(Origin::Engine::Content::EntitlementRef, bool externalNonRtp);
                
                /// \brief Emits a signal when the user has finished playing the game.
                void playFinished(Origin::Engine::Content::EntitlementRef entitlement);

                /// \brief Emits a signal when a game has connection connected to Origin via the SDK
                void sdkConnected(Origin::Engine::Content::EntitlementRef);

                /// \brief Emits a signal when a flow has started or stopped.
                void activeFlowsChanged(const QString &productId, const bool &active);

                /// \brief Emits a signal when the last active install flow is paused.
                ///
                /// This signal is only emitted if pauseAllActiveDownloads() is called with a valid timeout.
                /// \param success True if all flows were successfully paused, false if they timed out.
                void lastActiveFlowPaused(bool success);
                
                /// \brief Emits a signal when the last install flow has been wiped out.
                void lastContentInstallFlowDeleted();

                /// \brief Emits a signal when loading entitlements takes too long due to a slow network
                void loadingGamesTooSlow();

                /// \brief Signal is emitted when content folders are invalid.  Causes the client to load the settings page,
                /// so the user can change the content folder.
                void invalidContentFolders();

                /// \brief Signal is emitted for an invalid content folder.  Causes an error message pop-up.
                void folderError(int error);

                /// \brief Signal is emitted when UAC failed or was denied.
                void autoDownloadUACFailed();

                /// \brief Signal is emitted when the client detects content that is in need of repair
                void brokenContentDetected();

                /// \brief Signal emitted to request any necessary download preflighting be performed.
                void preflightDownload(
                        Origin::Engine::Content::LocalContent const& content,
                        Origin::Engine::Content::ContentController::DownloadPreflightingResult&);

            public slots:
                /// \brief Slot to mark that the entitlement update check is completed.
                void onOfferVersionUpdateCheckComplete();

                ///
                /// \brief slot to listen to connection change.
                ///
                void onConnectionStateChanged(bool onlineAuth);

                ///
                /// \brief slot to listen for valid content folders to kick off auto-patching/downloads
                ///
                void onContentFoldersValidForAutoDownloads();

                /// \brief slot called when a SDK-enabled game is being played
                /// \param productId
                void onConnectedToSDK(QString);

                /// \brief slot called when a SDK-enabled game is stopped being played
                /// \param productId
                void onHungupFomSDK(const QString&);

                /// \brief Starts all automatic downloader processing (updates, downloads, PDLC)
                ///
                void processDownloaderAutostartItems();

                /// \brief Starts auto-repair of broken content
                ///
                void processBrokenContentItems();

                /// \brief Declines auto-repair of broken content
                ///
                void processBrokenContentItemsRejected();

            private slots:
                void onBaseGamesLoaded();

                void onBaseGameAdded(Origin::Engine::Content::BaseGameRef baseGame);
                void onBaseGameRemoved(Origin::Engine::Content::BaseGameRef baseGame);
                void onBaseGameExtraContentUpdated(Origin::Engine::Content::BaseGameRef baseGame);

                void onNonOriginGameAdded(Origin::Engine::Content::EntitlementRef nog);
                void onNonOriginGameRemoved(Origin::Engine::Content::EntitlementRef nog);

                void onFlowActiveStateCheck(const QString &key, const bool &active);

                void onInstallFlowSuspendComplete(const QString &key);
                void onInstallFlowStateChange(Origin::Downloader::ContentInstallFlowState state);
                void onInstallFlowDeleted();

                void onPauseAllTimeout();

                void onUserLoggedOut(Origin::Engine::UserRef user);

                void onRefreshGamesTimedOut();
                void onDelayedUpdateFinished();

                void refreshAchievements(EntitlementRefList refreshEntitlements, bool refreshUserPoints);

                void onProcessDownloaderAutoStartItemsCallback(bool uacSuccess);
                void onCollectBrokenContentItemsCallback();
                void onProcessBrokenContentItemsCallback(bool uacSuccess);

                void onPlayStarted(Origin::Engine::Content::EntitlementRef e);
                void onPlayFinished(Origin::Engine::Content::EntitlementRef e);
                void onRestoreDownloadUtilizationTimout();

            protected:
                QSet<QString> m_processedExtraContentProductIds;

                QList<Downloader::IContentInstallFlow*> m_flowsWaitingToPause;
                QList<Engine::Content::LocalContent*> m_contentFlowsToDelete;

                QMultiHash<QString, EntitlementRef> m_productIdToEntitlementRefHash;
                QMultiHash<QString, EntitlementRef> m_installFolderToEntitlementRefHash;

            private:
                QStringList mEntitlementsConnectedToSDK;
                /// \brief Returns the session of the user associated with the entitlements.
                Services::Session::SessionRef session();

                UserWRef m_User;

                mutable QMutex m_entitlementInitialCheckLock;
                mutable QReadWriteLock m_unownedChildContentLock;
                mutable QReadWriteLock m_initialExtraEntitlementListAccessLock;

                EntitlementRef addOwnedContent(EntitlementRef ownedContent);
                void processExtraContent(BaseGameRef baseGame, EntitlementRefList& addedEntitlementList);

                void purgeLicenses(const EntitlementRefList &entitlementList);

                bool m_InitialEntitlementRefreshed;
				bool m_InitialFullEntitlementRefreshed;	// add initial extra content load completed as well
                bool m_refreshInProgress;

                QSet<QString> mInitialExtraContentRequestList;    // on initial refresh for extra content make list of master ids that we are waiting for a response

                explicit ContentController(Origin::Engine::UserRef user);

                QTimer *m_refreshGamesTimeout;
                const qint32 m_refreshTimeout;

                bool mIsLoadingEntitlementsSlow;

                QSet<QString> mAutoStartOnRefreshList;
                QSet<QString> mPendingPurchasedDLCDownloadList;  // list of DLC that were just purchased but are pending a refresh (EBIBUGS-28701)

                bool m_autostartPermissionsRequested;
                bool m_autostartUACFailed;
				bool m_autoStartItemsOK;	// because we need to delay this now for children loading in order to restore the download queue properly
                bool m_autostartUACPending;
                bool m_autostartAllowOnExtraDownloads;  // this is to stop the onExtraContentAvailable() automatic download of extra content until we have initially checked all the pending patches/updates (in processDownloaderAutoStartItems())
                                                        // so that we don't keep on asking UAC permissions (if the user refuses at first) - afterwards, we simply disable the same onExtraContentAvailable() checks while running the processDownloaderAutoStartItems()
                QSet<QString> m_processedExtraContent;

                QTimer m_downloadUtilizationRestoreTimer;

                Shared::SharedNetworkOverrideManager *m_sharedNetworkOverrideManager;
            }; //ContentController

        } //Content        
    } //Engine
} //Origin

#endif // CONTENTCONTROLLER_H
