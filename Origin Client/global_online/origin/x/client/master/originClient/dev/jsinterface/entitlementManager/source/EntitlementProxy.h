/////////////////////////////////////////////////////////////////////////////
// EntitlementProxy.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef _ENTITLEMENTPROXY_H
#define _ENTITLEMENTPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>

#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/downloader/ContentInstallFlowState.h"

#include "widgets/client/source/PDLCViewController.h"

class QTimer;

namespace Origin
{
namespace Client
{

class CloudSaveDebugActions;

namespace JsInterface
{

class EntitlementCloudSaveProxy;
class EntitlementInstallFlowProxy;
class NonOriginGameProxy;
class BoxartProxy;

class ORIGIN_PLUGIN_API EntitlementProxy : public QObject
{
    friend class EntitlementManager;
	Q_OBJECT
public:
	// Date the entitlement was entitled t:o the user
	Q_PROPERTY(QDateTime entitleDate READ entitleDate)
	QDateTime entitleDate();
	
	// Key of the entitlement
	Q_PROPERTY(QString id READ id)
	QString id();

	// Content ID of the entitlement
	Q_PROPERTY(QString contentId READ contentId)
	QString contentId();

    // Content ID of the entitlement
    Q_PROPERTY(QString expansionId READ financeId)
    QString financeId();
    
    // Product ID of the entitlement
	Q_PROPERTY(QString productId READ productId)
	QString productId();

    // list of related game ids (used for determining games playing in common)
    Q_PROPERTY(QStringList relatedGameContentIds READ relatedGameContentIds)
    QStringList relatedGameContentIds();

	// Localized title of the entitlement
	Q_PROPERTY(QString title READ title)
	QString title();

	// Box art URL
	Q_PROPERTY(QStringList boxartUrls READ boxartUrls)
	QStringList boxartUrls();

    // Return list of launch options
    Q_PROPERTY(QStringList multiLaunchTitles READ multiLaunchTitles)
    QStringList multiLaunchTitles();

	// Return the platforms supported
	Q_PROPERTY(QStringList platformsSupported READ platformsSupported)
	QStringList platformsSupported();

	// Return the locales supported
	Q_PROPERTY(QStringList localesSupported READ localesSupported)
	QStringList localesSupported();
	
    Q_PROPERTY(QStringList bannerUrls READ bannerUrls)
	QStringList bannerUrls();

	// Manual URL
	Q_PROPERTY(QVariant manualUrl READ manualUrl)
	QVariant manualUrl();

	// Last played date
	Q_PROPERTY(QDateTime lastPlayedDate READ lastPlayedDate)
	QDateTime lastPlayedDate();

	// Origin Release Control - download/preload start date
	Q_PROPERTY(QVariant displayDownloadStartDate READ displayDownloadStartDate)
	QVariant displayDownloadStartDate();

	// Origin Release Control - displayUnlock start date
	Q_PROPERTY(QVariant displayUnlockStartDate READ displayUnlockStartDate)
	QVariant displayUnlockStartDate();
	
	// Origin Release Control - expiration date
	Q_PROPERTY(QVariant displayExpirationDate READ displayExpirationDate)
	QVariant displayExpirationDate();
	
    // Origin Release Control - the pre announcement string
    Q_PROPERTY(QVariant preAnnouncementDisplayString READ preAnnouncementDisplayString)
    QVariant preAnnouncementDisplayString();

	// Cloud saves sub-object
	Q_PROPERTY(QVariant cloudSaves READ cloudSaves)
	QVariant cloudSaves();

    // Non-Origin game sub-object
    Q_PROPERTY(QVariant nonOriginGame READ nonOriginGame)
    QVariant nonOriginGame();

    // Boxart sub-object
    Q_PROPERTY(QVariant boxart READ boxart)
    QVariant boxart();

	// The registration code
	Q_PROPERTY(QString registrationCode READ registrationCode);
	QString registrationCode();

	// Determines if repairs are supported
	Q_PROPERTY(bool repairSupported READ repairSupported);
	bool repairSupported();
	
	// Determines if updates are supported
	Q_PROPERTY(bool updateSupported READ updateSupported);
	bool updateSupported();

	// Determines if an update is available
	Q_PROPERTY(QVariant availableUpdateVersion READ availableUpdateVersion);
	QVariant availableUpdateVersion();

	// Indicates if the game is being updated
	Q_PROPERTY(bool updating READ updating);
	bool updating();

    Q_PROPERTY(bool repairing READ repairing);
    bool repairing();

	// Returns the current download operation
	Q_PROPERTY(QVariant downloadOperation READ downloadOperation);
	QVariant downloadOperation();
	
	// Returns the current update operation
	Q_PROPERTY(QVariant updateOperation READ updateOperation);
	QVariant updateOperation();

    // Returns the current unpack operation
    Q_PROPERTY(QVariant unpackOperation READ unpackOperation);
    QVariant unpackOperation();
    
	// Returns the current repair operation
	Q_PROPERTY(QVariant repairOperation READ repairOperation);
	QVariant repairOperation();
	
    // Returns the current install operation
	Q_PROPERTY(QVariant installOperation READ installOperation);
	QVariant installOperation();

    // Returns the current operation
    Q_PROPERTY(QVariant currentOperation READ currentOperation);
    QVariant currentOperation();

	// Returns if auto update is enabled for the entitlement
	Q_PROPERTY(bool autoUpdateEnabled READ autoUpdateEnabled);
	bool autoUpdateEnabled();
	
	// Returns the achievement set id
	Q_PROPERTY(QString achievementSet READ achievementSet);
	QString achievementSet();

    // Returns a long description of the entitlement
    Q_PROPERTY(QString longDescription READ longDescription);
    QString longDescription();
    
    // Returns a brief description of the entitlement
    Q_PROPERTY(QString shortDescription READ shortDescription);
    QString shortDescription();
    
    // Returns the OFB-configured product type
    Q_PROPERTY(QString productType READ productType);
    QString productType();
    
    // Returns the download package type (DiP, Unpacked, etc.)
    Q_PROPERTY(QString packageType READ packageType);
    QString packageType();
    
    // Returns true if the game does not support Twitch broadcasting.
    Q_PROPERTY(bool twitchBlacklisted READ twitchBlacklisted);
    bool twitchBlacklisted();

	// Play the content
	Q_INVOKABLE void play();

	// Start the download
	Q_INVOKABLE void startDownload();

	// Uninstall the game
	Q_INVOKABLE void uninstall();

	// Repair the game's install
	Q_INVOKABLE void repair();

	// Checks for an available update
	Q_INVOKABLE void checkForUpdateAndInstall();

	// Installs an available update
	Q_INVOKABLE void installUpdate();

    // Indicates if the game is eligible for a progressive install (can play while download/installing)
    Q_PROPERTY(bool dynamicContentSupportEnabled READ dynamicContentSupportEnabled);
    bool dynamicContentSupportEnabled();

	// Indicates if the game can be played
	Q_PROPERTY(bool playable READ playable);
	bool playable();

    // Indicates if the game can be played the user's machine bitset.
    Q_PROPERTY(bool playableBitSet READ playableBitSet);
    bool playableBitSet();

	// Indicates if the game's path is in the 'Trash'
	Q_PROPERTY(bool inTrash READ inTrash);
	bool inTrash();
	
	// Indicates if the game can be downloaded
	Q_PROPERTY(bool downloadable READ downloadable);
	bool downloadable();

	// Indicates if the game has been published
	Q_PROPERTY(bool published READ published);
	bool published();
    
	// Indicates if the game has been published in the last 28 days.
	Q_PROPERTY(bool newlyPublished READ newlyPublished);
	bool newlyPublished();
    
	// Indicates if the game's download date has been bypassed
	Q_PROPERTY(bool downloadDateBypassed READ downloadDateBypassed);
	bool downloadDateBypassed();
    
	// Indicates if the game's release date has been bypassed
	Q_PROPERTY(bool releaseDateBypassed READ releaseDateBypassed);
	bool releaseDateBypassed();

	// Indicates if the game is installed
	Q_PROPERTY(QVariant installed READ installed);
	QVariant installed();
	
	// Indicates if the game can be uninstalled
	Q_PROPERTY(bool uninstallable READ uninstallable);
	bool uninstallable();
    
	// Indicates if the game has a custom uninstaller specified in the DiP manifest
	Q_PROPERTY(bool hasUninstaller READ hasUninstaller);
	bool hasUninstaller();
	
	// Indicates if the game is being played
	Q_PROPERTY(bool playing READ playing);
	bool playing();

    // Dictionary of debug information about the entitlement
    Q_PROPERTY(QVariant debugInfo READ debugInfo);
	QVariant debugInfo();

    // Total time played
	Q_PROPERTY(qint64 totalSecondsPlayed READ totalSecondsPlayed);
	qint64 totalSecondsPlayed();

	// Parent entitlement
	Q_PROPERTY(QVariant parent READ parent);
	QVariant parent();

	// Addons
	Q_PROPERTY(QObjectList addons READ addons);
	QObjectList addons();
	
	// Expansions
	Q_PROPERTY(QObjectList expansions READ expansions);
	QObjectList expansions();
	
	// Type
	Q_PROPERTY(bool isPULC READ isPULC);
	bool isPULC();
    
	// Display location
	Q_PROPERTY(QString displayLocation READ displayLocation);
	QString displayLocation();

    // the last action of the install flow (downloading, preloading, updating, repairing, installing)
    Q_PROPERTY(QString lastInstallFlowAction READ lastInstallFlowAction);
    QString lastInstallFlowAction();

    // the total data size associated with the last action of the install flow
    Q_PROPERTY(qint64 lastInstallFlowActionTotalDataSize READ lastInstallFlowActionTotalDataSize);
    qint64 lastInstallFlowActionTotalDataSize();

    // ORC status
    Q_PROPERTY(QString releaseStatus READ releaseStatus);
    QString releaseStatus();

    // Whether entitlement is a pre-order entitlement
    Q_PROPERTY(bool isPreorder READ isPreorder);
    bool isPreorder();
    
    // Whether entitlement is a browser game
    Q_PROPERTY(bool isBrowserGame READ isBrowserGame);
    bool isBrowserGame();

    // Game Type (normal, free trial etc)
    Q_PROPERTY(QString itemSubType READ itemSubType);
    QString itemSubType();

    // Test code or null if there is none
    Q_PROPERTY(QVariant testCode READ testCode)
    QVariant testCode();

    // Indicates if this entitlement has any purchasable addons
    Q_PROPERTY(bool hasPDLCStore READ hasPDLCStore);
    bool hasPDLCStore();

    // Favorite flag
    Q_PROPERTY(bool favorite READ favorite WRITE setFavorite)
    bool favorite();
    void setFavorite(bool);
    
    // Favorite flag
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden)
    bool hidden();
    void setHidden(bool);

    Q_PROPERTY(bool isCompletelySuppressed READ isCompletelySuppressed)
    bool isCompletelySuppressed();

    // Multi Launch Default flag
    Q_PROPERTY(QString multiLaunchDefault READ multiLaunchDefault)
    QString multiLaunchDefault();
    
    // Indicates if the entitlement is installable
    Q_PROPERTY(bool installable READ installable);
    bool installable();

    // Manually kicks off an installer
    Q_INVOKABLE void install();

	// Moves application to 'Applications' folders
	Q_INVOKABLE void restoreFromTrash()

    // Debug actions associated with the entitlement
    Q_PROPERTY(QObjectList debugActions READ debugActions);
    QObjectList debugActions();

    // Unowned Content
    // Indicates if the entitlement has extra content that is not owned
    Q_PROPERTY(bool unownedContentAvailable READ unownedContentAvailable);
    bool unownedContentAvailable();

    // Indicates if this entitlement has extra content that has been released within the past 7 days
    Q_PROPERTY(bool newUnownedContentAvailable READ newUnownedContentAvailable);
    bool newUnownedContentAvailable();

    // Latest 'published date' for unowned and purchasable child content
    Q_PROPERTY(QDateTime latestChildPublishedDate READ latestChildPublishedDate)
    QDateTime latestChildPublishedDate();

    // master title Id, a id that is shared between related entitlements
    Q_PROPERTY(QString masterTitleId READ masterTitleId)
    QString masterTitleId();

    // franchise Id, an id that is shared between titles from the same franchise
    Q_PROPERTY(QString franchiseId READ franchiseId)
    QString franchiseId();

    // termination date for free trial
    Q_PROPERTY(QDateTime terminationDate READ terminationDate)
    QDateTime terminationDate();
    
    // Gets sort order value for extra content.  Note that this does not apply to base games.
    Q_PROPERTY(QVariant sortOrderDescending READ sortOrderDescending);
    QVariant sortOrderDescending();

    // Gets dictionary of display group info for extra content.  Note that this does not apply to base games.
    Q_PROPERTY(QVariant extraContentDisplayGroupInfo READ extraContentDisplayGroupInfo);
    QVariant extraContentDisplayGroupInfo();

    Q_PROPERTY(QVariant timeRemainingTilTerminationInSecs READ timeRemainingTilTerminationInSecs);
    QVariant timeRemainingTilTerminationInSecs();

    // A list of thumbnail image urls for this entitlement. None of the urls are guaranteed to exist, and image retrieval should be
    // attmepted using the urls in the order in which they appear in the list.
    Q_PROPERTY(QStringList thumbnailUrls READ thumbnailUrls);
    QStringList thumbnailUrls();

    // Is the entitlement owned by the current user.
    Q_PROPERTY(bool owned READ owned);
    bool owned();

    // Can the current user purchase this entitlement. Will return false if already purchased.
    Q_PROPERTY(bool purchasable READ purchasable);
    bool purchasable();

    // Can the current user download this content without an entitlement.
    Q_PROPERTY(bool previewContent READ previewContent);
    bool previewContent();

    // For bundled entitlements, returns a list of entitlement proxys for content contained within the bundle. Returns an empty list
    // for entitlements which are not bundles.
    Q_PROPERTY(QObjectList associations READ associations);
    QObjectList associations();
    
    // Has pricing data been loaded?
    Q_PROPERTY(bool hasPricingData READ hasPricingData);
    bool hasPricingData();

    // Is the entitlement free?
    Q_PROPERTY(bool freeProduct READ freeProduct);
    bool freeProduct();

    // If this entitlement is purchasable, returns the current price of the entitlement. The string will be localized.
    Q_PROPERTY(QVariant currentPrice READ currentPrice);
    QVariant currentPrice();

    // If this entitlement is purchasable and has had a price reduction, returns the orignal price of the entitlement. The string will be localized.
    Q_PROPERTY(QVariant originalPrice READ originalPrice);
    QVariant originalPrice();

    // If this entitlement is purchasable and the price is in virtual currency, returns a localized descripition of the currency with html tags.
    Q_PROPERTY(QVariant priceDescription READ priceDescription);
    QVariant priceDescription();

    // Is this entitlement a free trial?
    Q_PROPERTY(bool freeTrial READ freeTrial);
    bool freeTrial();

    // Purchase this entitlement for the current user. If the entitlement is not purchasable or already purchased, the request for
    // purchase is ignored.
    Q_INVOKABLE void purchase();

    Q_INVOKABLE void fetchUnownedContentPricing();

    //////////////////////////////////////////////////////////////////////////
    /// Subscription related parameters/functions.
    ///

    /// \brief Indicates whether the entitlement was granted by a subscription.
    Q_PROPERTY(bool isEntitledFromSubscription READ isEntitledFromSubscription);
    bool isEntitledFromSubscription() const;

    /// \brief Indicates whether the entitlement has an upgraded version
    Q_PROPERTY(bool hasEntitlementUpgraded READ hasEntitlementUpgraded);
    bool hasEntitlementUpgraded() const;

    /// \brief Indicates whether the offer is available via subscription.
    Q_PROPERTY(bool isAvailableFromSubscription READ isAvailableFromSubscription);
    bool isAvailableFromSubscription() const;

    // Indicates that this entitlement should be suppressed in the normal views.
    Q_PROPERTY(bool isSuppressed READ isSuppressed);
    bool isSuppressed() const;

    // Determines what kind of upgrade is available, if any
    Q_PROPERTY(QString upgradeTypeAvailable READ upgradeTypeAvailable);
    QString upgradeTypeAvailable() const;

    // Returns the QDateTime on when the game expires.
    Q_PROPERTY(QVariant subscriptionRetiring READ subscriptionRetiring);
    QVariant subscriptionRetiring() const;

    // Returns the number of seconds until the expiration time.
    Q_PROPERTY(QVariant subscriptionRetiringTime READ subscriptionRetiringTime);
    QVariant subscriptionRetiringTime() const;

    // File of pre-subscription local save backup
    Q_PROPERTY(bool isUpgradeBackupSaveAvailable READ isUpgradeBackupSaveAvailable)
    bool isUpgradeBackupSaveAvailable();

    // Restores the pre-upgraded save file.
    Q_INVOKABLE void restoreSubscriptionUpgradeBackupSave();

    // Requests a subscription entitlement for this title.
    Q_INVOKABLE void entitle();

    // Removes the subscription version of this game/DLC.
    Q_INVOKABLE void revoke();

    // Upgrades to the best version of the game available
    Q_INVOKABLE void upgrade();

    // Removes the subscription version of the game
    Q_INVOKABLE void downgrade();

    ///
    /// Subscription related parameters/functions.
    //////////////////////////////////////////////////////////////////////////

private slots:
    void settingChanged(const QString& settingName);
    
    void childAdded(Origin::Engine::Content::EntitlementRef);
    void childRemoved(Origin::Engine::Content::EntitlementRef);

    void displayDateRevealTriggered();
    void armDisplayDateRevealTimer();
    void onContentConfigurationChanged();

	void playingGameTimeUpdated(const QString &masterTitleId, const QString &multiplayerId);

    void onStateChanged();

    void onBoxartChanged(const QString& productId);

    void onUpdateCheckCompletedForRepair();
    void onUpdateCheckCompletedForInstall();

    void onUnownedContentPricingAvailable();
    void timedTrialGrantTime();

    void onEntitleFailed(const QString &offerId);
    void onRevokeFailed(const QString &offerId);

signals:
	void changed();
    void boxartUrlsChanged();
    void unownedContentChanged();
	void ensureVisible();

	void addonAdded(QObject*);
	void addonRemoved(QObject*);

	void expansionAdded(QObject*);
	void expansionRemoved(QObject*);

    void entitleFailed();
    void revokeFailed();

private:
	void updateFavoriteAndHidden();
    void updateMultiLaunchDefault();

	explicit EntitlementProxy(Origin::Engine::Content::EntitlementRef, QObject *parent);

	bool dipUpdateSupported();

	Origin::Engine::Content::EntitlementRef mEntitlement;
	EntitlementCloudSaveProxy *mCloudSaves;
	EntitlementInstallFlowProxy *mInstallFlowProxy;
    NonOriginGameProxy *mNonOriginGame;
    BoxartProxy *mBoxart;

    CloudSaveDebugActions *mCloudDebugActions;

    QTimer *mDisplayDateRevealTimer;

    bool mCurrentFavoriteValue;
    bool mCurrentHiddenValue;
    QString mCurrentMultiLaunchDefault;

    Origin::Client::PDLCViewController mPdlcViewController;
};

}
}
}

#endif
