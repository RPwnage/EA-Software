;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }

var ONE_DAY_MS = 1000*60*60*24;
var GAME_DETAILS_BASE_URL ='./game-details.html?id=';
var NEW_TIME_TRIAL = 20 * 60 * 1000;
var NEW_TIME = 72 * 60 * 60 * 1000;

// Helper function to determine if we should fallback isPlayable/play() to our parent entitlement
function shouldFallbackPlayToParent(entitlement)
{
	return (entitlement.displayLocation === 'EXPANSIONS') && entitlement.installed;
}

/**
 * CLASS: Game
 */
var Game = function(entitlement, loadChildContent)
{
	this.entitlement = entitlement;

	if (window.achievementManager){
		achievementManager.achievementSetAdded.connect( this.onAchievementUpdate.delegate(this) );
	}

	// Bind to the change signal
	this.entitlement.changed.connect(this.onEntitlementChanged.delegate(this));
	contentOperationQueueController.headBusy.connect(this.onEntitlementChanged.delegate(this));

	if (this.entitlement.cloudSaves)
	{
		this.entitlement.cloudSaves.changed.connect(this.onEntitlementCloudSavesChanged.delegate(this));
		this.entitlement.cloudSaves.currentUsageChanged.connect(this.onEntitlementCloudSavesCurrentUsageChanged.delegate(this));
	}

	// Events
	this.changeEvent = $.Callbacks();
	this.hideEvent = $.Callbacks();
	this.favoriteEvent = $.Callbacks();

	this.addonAddEvent = $.Callbacks();
	this.addonRemoveEvent = $.Callbacks();

	this.expansionAddEvent = $.Callbacks();
	this.expansionRemoveEvent = $.Callbacks();

	this.playEvent = $.Callbacks();

	this.cloudSavesChangeEvent = $.Callbacks();
	this.cloudSavesCurrentUsageChangeEvent = $.Callbacks();

	// Initialize PDLC
	this.expansions = [];
	this.addons = [];

	// Cache of filter groups we belong to
	this._cachedFilterGroups = null;

	if(loadChildContent)
	{
		$.each(this.entitlement.expansions, $.proxy(function(idx, expansion)
		{
            if(originUser.commerceAllowed || expansion.owned)
            {
                var expansionGame = new Origin.Game(expansion, false);
                this.expansions.push(expansionGame);
            }
		}, this));

		$.each(this.entitlement.addons, $.proxy(function(idx, addon)
		{
            if(originUser.commerceAllowed || addon.owned)
            {
                var addonGame = new Origin.Game(addon, false);
                this.addons.push(addonGame);
            }
		}, this));
	}

	this.entitlement.addonAdded.connect(this.onEntitlementAddonAdded.delegate(this));
	this.entitlement.addonRemoved.connect(this.onEntitlementAddonRemoved.delegate(this));

	this.entitlement.expansionAdded.connect(this.onEntitlementExpansionAdded.delegate(this));
	this.entitlement.expansionRemoved.connect(this.onEntitlementExpansionRemoved.delegate(this));

    this._notNewTimerId = null;
    if(this.isNewType)
    {
	    this._notNewTimerId = window.setTimeout($.proxy(function()
	    {
		    this.changeEvent.fire(this);
	    }, this), this.timeRemainingTilNotNew);
    }
};


/** ******************
 * Boxart/thumbnail/banner helpers built on top the BadImageCache
 */
Game.prototype.__defineGetter__('effectiveBoxartUrl', function()
{
	var configuredUrls = this.entitlement.boxartUrls;

	for(var i = 0; i < configuredUrls.length; i++)
	{
		if (Origin.BadImageCache.shouldTryUrl(configuredUrls[i]))
		{
			return configuredUrls[i];
		}
	}

	// Every URL is known bad
	return null;
});

Game.prototype.__defineGetter__('effectiveThumbnailUrl', function()
{
	var configuredUrls = this.entitlement.thumbnailUrls;

	for(var i = 0; i < configuredUrls.length; i++)
	{
		if (Origin.BadImageCache.shouldTryUrl(configuredUrls[i]))
		{
			return configuredUrls[i];
		}
	}

	// Every URL is known bad
	return null;
});

Game.prototype.platformCompatibleStatus = function(platform)
{
    var availableOnPlatform = '';
    if(!platform)
    {
        platform = Origin.views.currentPlatform();
    }
    availableOnPlatform = ($.inArray(platform, this.entitlement.platformsSupported) >= 0);

    if(!availableOnPlatform)
    {
        return 'INCOMPATIBLE_PLATFORM';
    }
    
    if(platform === 'MAC' && this.entitlement.isEntitledFromSubscription)
    {
        return 'INCOMPATIBLE_SUBS';
    }
    
    return 'COMPATIBLE';
};

Game.prototype.__defineGetter__('effectiveBannerUrl', function()
{
	var configuredUrls = this.entitlement.bannerUrls;

	for(var i = 0; i < configuredUrls.length; i++)
	{
		if (Origin.BadImageCache.shouldTryUrl(configuredUrls[i]))
		{
			return configuredUrls[i];
		}
	}

	// Every URL is known bad
	return null;
});

/** ******************
 * Downloading
 */
Game.prototype.startDownload = function()
{
	this.entitlement.startDownload();
};

Game.prototype.pauseDownload = function()
{
	if (this.entitlement.downloadOperation)
	{
		// This is a setter method that will call setPaused on the QT object
		this.entitlement.downloadOperation.pause();
    }
};

Game.prototype.resumeDownload = function()
{
	if (this.entitlement.downloadOperation)
	{
		// This is a setter method that will call setPaused on the QT object
		this.entitlement.downloadOperation.resume();
    }
};

Game.prototype.cancelOperation = function()
{
	if (this.entitlement.currentOperation)
	{
        telemetryClient.sendQueueItemRemoved(this.entitlement.productId, "mygames-contextmenu");
		this.entitlement.currentOperation.cancel();
    }
};

/** ********************
 * Updating
 */
Game.prototype.checkForUpdateAndInstall = function()
{
	this.entitlement.checkForUpdateAndInstall();
};

Game.prototype.installUpdate = function()
{
	this.entitlement.installUpdate();
};

Game.prototype.pauseUpdate = function()
{
	if (this.entitlement.updateOperation)
	{
		this.entitlement.updateOperation.pause();
    }
};

Game.prototype.resumeUpdate = function()
{
	if (this.entitlement.updateOperation)
	{
		this.entitlement.updateOperation.resume();
    }
};

/** **********************
 * Repairing
 */
Game.prototype.repair = function()
{
	this.entitlement.repair();
};

Game.prototype.pauseRepair = function()
{
	if (this.entitlement.repairOperation)
	{
		this.entitlement.repairOperation.pause();
    }
};

Game.prototype.resumeRepair = function()
{
	if (this.entitlement.repairOperation)
	{
		this.entitlement.repairOperation.resume();
    }
};

/** *****************
 * Cloud Syncing
 */
Game.prototype.cancelCloudSync = function()
{
	if (this.entitlement.cloudSaves && this.entitlement.cloudSaves.syncOperation)
	{
		this.entitlement.cloudSaves.syncOperation.cancel();
    }
};

Game.prototype.removeFromLibrary = function()
{
    if (this.entitlement.nonOriginGame)
    {
        this.entitlement.nonOriginGame.removeFromLibrary();
    }
    else if(this.entitlement.isEntitledFromSubscription)
    {
        entitlementManager.dialogs.showRemoveEntitlementPrompt(this.entitlement.productId);
    }
};

/** *****************
 * Playing
 */
Game.prototype.play = function()
{
	if (this.entitlement.playable)
	{
		this.entitlement.play();
	}
	else if (shouldFallbackPlayToParent(this.entitlement))
	{
		// Proxy the play to our parent
		this.entitlement.parent.play();
	}
	else
	{
		// Not playable
		return this;
	}

	this.playEvent.fire(this);
	return this;
};

/** ******************
 * Installing
 */
Game.prototype.install = function()
{
	this.entitlement.install();
};

Game.prototype.pauseInstall = function()
{
	if (this.entitlement.installOperation)
	{
		this.entitlement.installOperation.pause();
    }
};

Game.prototype.resumeInstall = function()
{
	if (this.entitlement.installOperation)
	{
		this.entitlement.installOperation.resume();
    }
};

/** ******************
 * Uninstalling
 */
Game.prototype.uninstall = function()
{
	this.entitlement.uninstall();
};

/** ******************
 * Restoring
 */
Game.prototype.restoreFromTrash = function()
{
	this.entitlement.restoreFromTrash();
};


/** **********************
 * Notifications
 */
Game.prototype.getNotifications = function()
{
	var notifications = [],
        currentOperation = this.entitlement.currentOperation,
        showNotYetReleasedText = true,
        secondsUntilSubsMembershipExpiration = 0,
        secondsUntilSubsEntitlementRetires = 0,
        secondsUntilSubsOfflinePlayExpiration = 0;

	var addNotification = function(data)
	{
		// Use the notification type as the key by default
		if (typeof data.key === 'undefined')
		{
			data.key = data.type;
		}

		notifications.push(data);
	};

    var displayReleaseDate = function(releaseDate)
    {
        var millisecondsPerYear = 365 * 24 * 60 * 60 * 1000;
        var display = false;

        if (releaseDate !== null)
        {
            var releaseDateMS = releaseDate.getTime();
            var concealDateMS = (new Date().getTime()) + millisecondsPerYear;

            // check to see if the release date is less than a year out
            display = (releaseDateMS < concealDateMS);
        }

        return display;
    };

	// Is this a non-Origin game
	if (this.isNonOriginGame)
	{
		addNotification({ type: 'non-origin-game', dismissable: false })
	}

	// Hack to try to determine if we could download if we were online
	if (!onlineStatus.onlineState && !this.isInstalled && !this.isInstallable)
	{
		addNotification({ type: 'offline', dismissable: false });
	}

	// Internal Debug (1102, Download override, etc.)
	if (this.entitlement.debugInfo)
	{
		var debugInfo = this.entitlement.debugInfo.summary;

		if (this.entitlement.displayLocation === 'ADDONS')
		{
			// Addons don't have game detail pages so they need everything on the hovercard
			debugInfo = $.extend({}, debugInfo, this.entitlement.debugInfo.details);
		}

		addNotification({ type: 'internal-debug', data: debugInfo, dismissable: false });
	}

	// Platform information if game isn't available on current platform
	if (!this.isPlayable)
    {
        switch(this.platformCompatibleStatus())
        {
            case ('INCOMPATIBLE_SUBS'):
            {
                var owned = this.entitlement.owned;
                // This is the true state of owned. However, it's too risky to fix it at this point.
                if(owned && !subscriptionManager.isActive)
                {
                    owned = false;
                }
                if(this.entitlement.purchasable && !owned)
                {
                    addNotification({ type: 'subs-windows-only-purchasable', dismissable: false });
                }
                else
                {
                    addNotification({ type: 'subs-windows-only', dismissable: false });
                }
                break;
            }
            
            case ('INCOMPATIBLE_PLATFORM'):
            {
                var platformSupported = "platform-none";
    
                if (this.entitlement.platformsSupported.length > 0)
                {
                    platformSupported = "platform-" + this.entitlement.platformsSupported[0].toLowerCase();
                }
    
                addNotification({ type: platformSupported, dismissable: false });
    
                // If not available on this platform, not yet released was showing
                // Will need to revisit if we do have a version coming for this platform
                showNotYetReleasedText = false;
                break;
            }
            
            case ('COMPATIBLE'):
            default:
            {
                // If the platform is supported, make sure the 'bitness' is supported
                if(!this.entitlement.playableBitSet)
                {
                    addNotification({ type: "64-only", dismissable: false });
                }
                break;
            }
        }
	}

	// Expiration date (expiring from vault)
	// We only show this 30 days before actual expiration
	var secondsUntilExpiration = this.secondsUntilExpiration;
    var isTrialType = this.isTrialType;
    var isTimedTrialType = this.entitlement.itemSubType === 'TIMED_TRIAL_ACCESS' || this.entitlement.itemSubType === 'TIMED_TRIAL_GAMETIME';
    var isAlphaType = this.isAlphaType;
    var isBetaType = this.isBetaType;
    var isDemoType = this.isDemoType;
    var secondTerminationRemaining = this.secondsUntilTermination;
	if ((secondsUntilExpiration !== -1) && (secondsUntilExpiration < (30 * 24 * 60 * 60)) && (!isTimedTrialType) && (!isTrialType) && (!isDemoType) && (!isAlphaType) && (!isBetaType))
	{
		addNotification({ type: 'expires', seconds: secondsUntilExpiration, dismissable: false });
	}

    if(isTimedTrialType)
    {
        if(!onlineStatus.onlineState)
        {
            addNotification({ type: 'timed-trial-offline', dismissable: false });
        }
        else if(secondTerminationRemaining < 0)
        {
            addNotification({ type: 'timed-trial-admin-disable', dismissable: false });
        }
        else if (secondTerminationRemaining === 0)
        {
            addNotification({ type: 'timed-trial-expired', seconds: secondTerminationRemaining, dismissable: false });
        }
        else
        {
            addNotification({ type: 'early-access', seconds: secondTerminationRemaining, dismissable: false });
        }
    }
    // Free trials
    // Show the expired violator if the trial is expired
    // If trial has not expired, show trial violator regardless of how distant the expiration date is
    else if (isTrialType)
    {
        if (secondTerminationRemaining === 0)
        {
            addNotification({ type: 'trial-expired', seconds: secondTerminationRemaining, dismissable: false });
        }
        else
        {
            addNotification({ type: 'trial', seconds: secondTerminationRemaining, dismissable: false });
        }
    }

    // Alpha/Beta
    // Only show expired notification
    if (isAlphaType || isBetaType)
    {
    	if (secondsUntilExpiration === 0)
    	{
    		addNotification({ type: 'alpha-beta-expired', seconds: secondsUntilExpiration, dismissable: false });
    	}
    }

    if(this.entitlement.previewContent && !this.entitlement.owned && this.entitlement.purchasable)
    {
        addNotification({ type: "preview-content", dismissable: false });
    }

    // Demo
    // Always show the demo violator if this is a demo
    if (isDemoType)
    {
    	if (secondsUntilExpiration === 0)
    	{
    		addNotification({ type: 'demo-expired', seconds: secondsUntilExpiration, dismissable: false });
    	}
    	else
    	{
    		addNotification({ type: 'demo', seconds: secondsUntilExpiration, dismissable: false });
    	}
    }
    
    // Subscription Upgrade available
    // Don't show for DLC
    if (!this.entitlement.parent)
    {
        switch (this.entitlement.upgradeTypeAvailable)
        {
            case ('BASEGAME_ONLY'):
            {
                addNotification({ type: 'upgrade-available-base-only', dismissable: false });   
                break;
            }
            case ('DLC_ONLY'):
            {
                addNotification({ type: 'upgrade-available-dlc-only', dismissable: false });
                break;
            }
            case ('BASEGAME_AND_DLC'):
            {
                addNotification({ type: 'upgrade-available-base-dlc', dismissable: false });
                break;
            }
            case ('NONE'):
            default:
            {
                break;
            }
        }
    }

    if (this.entitlement.isEntitledFromSubscription && Origin.views.currentPlatform() !== 'MAC')
    {
        // - Subscription Membership Ending -
        switch (subscriptionManager.status)
        {
            case ('EXPIRED'):
            case ('DISABLED'):
            {
                addNotification({ type: 'subs-membership-expired', seconds: 0, dismissable: false });   
                break;
            }
            case ('PENDINGEXPIRED'):
            {
                secondsUntilSubsMembershipExpiration = subscriptionManager.timeRemaining;
                // Time left is 5 days (432000 seconds)
                if (secondsUntilSubsMembershipExpiration < 432000)
                {
                    addNotification({ type: 'subs-membership-expiring', seconds: secondsUntilSubsMembershipExpiration, dismissable: false });
                }
                break;
            }
            default:
            {
                break;
            }
        }
        
        if(this.entitlement.subscriptionRetiring)
        {
            // - Is this game retiring from the vault? -
            secondsUntilSubsEntitlementRetires = this.entitlement.subscriptionRetiringTime;
            if (secondsUntilSubsEntitlementRetires <= 0)
            {
                addNotification({ type: 'subs-entitlement-retired', seconds: secondsUntilSubsEntitlementRetires, dismissable: false });
            }
            // Time left is 30 days (2592000 seconds)
            else if (secondsUntilSubsEntitlementRetires < 2592000)
            {
                addNotification({ type: 'subs-entitlement-retiring', seconds: secondsUntilSubsEntitlementRetires, dismissable: false });
            }
        }
        
        if(!onlineStatus.onlineState)
        {
            // - Is offline play expiring? -
            secondsUntilSubsOfflinePlayExpiration = subscriptionManager.offlinePlayRemaining;
            if (secondsUntilSubsOfflinePlayExpiration <= 0)
            {
                addNotification({ type: 'subs-offline-play-expired', seconds: secondsUntilSubsOfflinePlayExpiration, dismissable: false });
            }
            // Time left is 5 days (432000 seconds)
            else if (secondsUntilSubsOfflinePlayExpiration < 432000)
            {
                addNotification({ type: 'subs-offline-play-expiring', seconds: secondsUntilSubsOfflinePlayExpiration, dismissable: false });
            }
        }
    }

	// Preload
	// Custom Icon? No
	var downloadStartDate = this.entitlement.displayDownloadStartDate;
	var unlockStartDate = this.entitlement.displayUnlockStartDate;
    var preAnnouncementDate = this.entitlement.preAnnouncementDisplayString;
    var hasPreannouncement = (preAnnouncementDate != 'undefined' && preAnnouncementDate.length > 0);

    if (hasPreannouncement)
    {
        addNotification({ type: 'preannouncement-date', date: preAnnouncementDate , dismissable: false });
        showNotYetReleasedText = false;
    }
    else
    {
        // Title is unreleased
        if (this.entitlement.releaseStatus === 'UNRELEASED')
        {
            if (displayReleaseDate(unlockStartDate))
            {
                if (displayReleaseDate(downloadStartDate) && downloadStartDate.getTime() !== unlockStartDate.getTime())
                {
                    // Show both preload and available dates. (code 1102 - "Release date is overridden"?)
                    addNotification({ type: 'preload-date', date: downloadStartDate, dismissable: false });
                }

                // Available date
                addNotification({ type: 'available-date', date: unlockStartDate, dismissable: false });
                showNotYetReleasedText = false;
            }
        }
        // Title is in preload period
        else if (this.entitlement.releaseStatus === 'PRELOAD')
        {
            // Only show available date
            if (displayReleaseDate(unlockStartDate))
            {
                // Available date
                addNotification({ type: 'available-date', date: unlockStartDate, dismissable: false });
                showNotYetReleasedText = false;
            }
        }
    }

    if (this.entitlement.releaseStatus === 'PRELOAD')
    {
        if (this.isPreloaded && !this.isNotificationDismissed('preloaded'))
        {
            // Already preloaded
            addNotification({ type: 'preloaded', dismissable: true });
        }
        else if (this.isDownloadable && !this.isNotificationDismissed('preload-available'))
        {
            // Preload available
            addNotification({ type: 'preload-available', dismissable: true });
            showNotYetReleasedText = false;
        }
    }

	// Update available
	// Custom Icon? No
	// Custom Icon? No
	// If auto-patching is disabled as a user setting, and updates are supported and available
	var availableUpdateVersion = this.entitlement.availableUpdateVersion;

	if (onlineStatus.onlineState &&
		(this.entitlement.availableUpdateVersion !== null) &&
		!this.entitlement.currentOperation)
	{
		// Key the notification to the update version so the use will get
		// notified again if a new version is available
		var notificationKey = 'update-available-' + availableUpdateVersion;

		if (!this.isNotificationDismissed(notificationKey))
		{
			addNotification({ type: 'update-available', dismissable: true, key: notificationKey });
		}
	}

	// New DLC for this base game is available for purchase
    if (this.entitlement.newUnownedContentAvailable)
    {
        var latestChildPublishedDate = this.entitlement.latestChildPublishedDate;
        if (latestChildPublishedDate && !this.isNotificationDismissedSince('new-dlc-available', latestChildPublishedDate.getTime() ))
        {
            addNotification({ type: 'new-dlc-available', dismissable: true });
        }
    }

    // If this entitlement is an expansion, is unowned, and is newly published (TODO: maybe add addons here too?)
    if (this.entitlement.newlyPublished && !this.entitlement.owned)
    {
        var isExpansion = this.entitlement.displayLocation === 'EXPANSIONS' || this.entitlement.displayLocation === 'GAMES|EXPANSIONS';
	    if (isExpansion && !this.isNotificationDismissed('new-dlc'))
	    {
		    addNotification({ type: 'new-dlc', dismissable: true });
	    }
    }

	// TODO - Just released
	// Custom Icon? No
	if (0 && !this.isNotificationDismissed('just-released'))
	{
		addNotification({ type: 'just-released', dismissable: false });
	}

    if (this.entitlement.inTrash)
    {
        addNotification({ type: 'trashed', dismissable: false});
    }

    if (currentOperation && currentOperation.isDynamicOperation)
    {
        if(currentOperation.shouldLightFlag("playableAt"))
        {
            addNotification({ type: 'playable', dismissable: false});
        }
        else if(currentOperation.progress > 0 && currentOperation.playableAt > 0)
        {
            addNotification({ type: 'dynamic-content-playable-at', dismissable: false});
        }
        else
        {
            addNotification({ type: 'dynamic-content', dismissable: false});
        }
    }
    else if(this.entitlement.dynamicContentSupportEnabled)
    {
        addNotification({ type: 'dynamic-content', dismissable: false});
    }

    if (!hasPreannouncement && this.isUnreleased && showNotYetReleasedText)
    {
        // Not yet released
        addNotification({ type: 'release-coming', dismissable: false });
    }

	return notifications;
};

Game.prototype.isNotificationDismissed = function(type)
{
	return ($.jStorage.get('notification-dismissed-' + type + '-' + this.entitlement.id) !== null);
};

Game.prototype.isNotificationDismissedSince = function(type, since)
{
    var dismissTime = $.jStorage.get('notification-dismissed-' + type + '-' + this.entitlement.id);
    return dismissTime && (dismissTime > since);
};

Game.prototype.dismissNotification = function(type)
{
	$.jStorage.set('notification-dismissed-' + type + '-' + this.entitlement.id, Date.now());
	this.changeEvent.fire(this);
};


/** **********************
 * Misc Methods
 */

Game.prototype.getDetailsUrl = function()
{
	return GAME_DETAILS_BASE_URL + this.entitlement.id;
};

Game.prototype.gotoDetails = function()
{
	var detailsUrl = this.getDetailsUrl();

	// QtWebKit will just freeze up trying to load the page next without
	// finishing the redraw for the current one. This can cause our window
	// to get caught in awkward half drawn state until the new page renders.
	// Work around that by setting a timeout here and doing the load later
	setTimeout(function()
	{
		window.location.href = detailsUrl;
	}, 0);
};

Game.prototype.performPrimaryAction = function(source)
{
	if ((this.isTrialType && this.secondsUntilTermination === 0) || (this.entitlement.purchasable && !this.entitlement.owned))
    {
        this.purchase('primary_action');
    }
	else if (this.isPlayable)
	{
        telemetryClient.sendGamePlaySource(this.entitlement.productId, source);
		this.play();
	}
	else if (this.isDownloadable)
	{
		this.startDownload();
	}
	else if (this.isInstallable)
	{
		this.install();
	}
	else if (this.entitlement.owned && !this.entitlement.currentOperation && !this.entitlement.isPULC &&
        (this.entitlement.releaseStatus !== 'UNRELEASED' || this.entitlement.downloadDateBypassed) &&
        this.entitlement.parent && (this.entitlement.parent.downloadable || this.entitlement.parent.installable))
	{
	    // Prompt the user to install the base game
	    entitlementManager.dialogs.promptForParentInstallation(this.entitlement.productId);
	}
};

Game.prototype.viewStoreProductPage = function()
{
    clientNavigation.showStoreProductPage(this.entitlement.productId);
};

Game.prototype.viewStoreMasterTitlePage = function()
{
    clientNavigation.showStoreMasterTitlePage(this.entitlement.masterTitleId);
};

Game.prototype.purchase = function(context)
{
    // send telemetry when purchasing through game details page
    telemetryClient.sendContentSalesPurchase(this.entitlement.productId, this.entitlement.productType, context);

    // Just until the server can open lockbox pages for NUCLEUSDD offers.
    if (this.entitlement.purchasable && this.entitlement.productType !== 'NUCLEUSDD')
    {
        this.entitlement.purchase();
    }
    else
    {
        this.viewStoreMasterTitlePage();
    }
};

Game.prototype.getAddonById = function(id)
{
	var addons = $.grep(this.addons, function(addon, idx){ return (addon.entitlement.id === id); });
	return (addons && addons.length !== 0)? addons[0] : null;
};

Game.prototype.getExpansionById = function(id)
{
	var expansions = $.grep(this.expansions, function(expansion, idx){ return (expansion.entitlement.id === id); });
	return (expansions && expansions.length !== 0)? expansions[0] : null;
};

Game.prototype.addBoxartTransformations = function(containerWidth, containerHeight, $img)
{
	if (this.entitlement.boxart && this.entitlement.boxart.croppedBoxart) {

        var dx = this.entitlement.boxart.imageLeft(containerWidth),
        	dy = this.entitlement.boxart.imageTop(containerHeight),
        	scale = this.entitlement.boxart.imageScale(containerWidth);

        $img.css({'width' : 'auto'});
        $img.css({'height' : 'auto'});
        $img.css({'-webkit-transform-origin' : this.entitlement.boxart.cropCenterX + 'px ' + this.entitlement.boxart.cropCenterY + 'px'});
        $img.css({'-webkit-transform' : 'translate(' + dx + 'px,' + dy + 'px) scale(' + scale + ')'});
    } else {
    	$img.css({'width' : 'inherit'});
        $img.css({'height' : 'inherit'});
        $img.css({'-webkit-transform-origin' : ''});
        $img.css({'-webkit-transform' : 'none'});
    }
};

// Returns -1 if no expiration, 0 if expired, otherwise, number of days
Game.prototype.__defineGetter__('secondsUntilExpiration', function()
{
	if (!this.entitlement.displayExpirationDate)
	{
		return -1;
	}
	var now = Date.now();
	var exp = this.entitlement.displayExpirationDate.getTime();

	return (now < exp)? ((exp - now) / 1000) : 0;
});

// Returns -1 if no expiration, 0 if expired, otherwise, number of days
Game.prototype.__defineGetter__('secondsUntilTermination', function()
{
    return this.entitlement.timeRemainingTilTerminationInSecs;
});

Game.prototype.__defineGetter__('isDownloadable', function()
{
	return this.entitlement.downloadable;
});
    
Game.prototype.__defineGetter__('isPurchasable', function()
{
    var purchasable = true;
    // Subscriptions: For standard edition DLC - we don't want user to see the price of games they haven't self-entitled yet.
    if(subscriptionManager.isActive && this.entitlement.isAvailableFromSubscription && this.entitlement.parent)
    {
        purchasable = false;
    }
    // User has a license or it's not purchasable
    else if (!this.entitlement.hasPricingData || !this.entitlement.purchasable || this.entitlement.owned)
    {
        purchasable = false;
    }
    return purchasable;
});

Game.prototype.__defineGetter__('isDownloading', function()
{
	return Boolean(this.entitlement.downloadOperation); //this.entitlement.state === 'DOWNLOADING');
});

Game.prototype.__defineGetter__('downloadNoun', function()
{
	return (this.entitlement.releaseStatus === 'PRELOAD')? tr('ebisu_client_preload_noun') : tr('ebisu_client_download_noun');
});

Game.prototype.__defineGetter__('downloadVerb', function()
{
    var index = contentOperationQueueController.index(this.entitlement.productId),
        isPreload = this.entitlement.releaseStatus === 'PRELOAD',
        str = '',
        locale = '';
    if(contentOperationQueueController.entitlementsQueued.length === 0)
    {
        str = isPreload ? tr('ebisu_client_preload') : tr('ebisu_client_download');
    }
    else
    {
        switch(index)
        {
            case -1:
                locale = clientSettings.readSetting("locale");
                // EBIBUGS-27488 In some locales Add to Downloads doesn't fit.
                // Instead just say "Download". TODO: Fix the addon tables to be an
                // ok size. Right now it's too risky.
                if (locale
                   && (locale === "th_TH"
                       || locale === "nl_NL"
                       || locale === "fr_FR"
                       || locale === "hu_HU"
                       || locale === "de_DE"
                       || locale === "pt_PT"
                       || locale === "pt_BR"
                       || locale === "sv_SE"))
                {
                    str = tr('ebisu_client_download');
                }
                else
                {
                    str = tr('ebisu_client_add_to_downloads');
                }
                break;
            case 0:
            default:
                str = isPreload ? tr('ebisu_client_preload_now') : tr('ebisu_client_download_now');
                break;
        }
    }

    return str;
});


Game.prototype.__defineGetter__('buyNowVerb', function()
{
    var str = ''
    if (this.entitlement.freeProduct)
    {
        if (this.entitlement.itemSubType === 'ON_THE_HOUSE')
        {
            str = tr('ebisu_client_download_now');
        }
        else
        {
            str = tr('ebisu_client_free_games_download_client_button');
        }
    }
    else if (this.entitlement.purchasable && !this.entitlement.owned)
    {
        str = tr('ebisu_client_buy_now');
    }
    else
    {
        str = tr('ebisu_client_free_trial_purchase_game');
    }
    return str;
});

Game.prototype.__defineGetter__('inTrash', function()
{
  return Boolean(this.entitlement.inTrash);
});

Game.prototype.__defineGetter__('isPlayable', function()
{
	if (this.entitlement.playable)
	{
		return true;
	}
    // 'parent' isn't totally reliable in this case. With subs, the parent could be the owned game, and this was redeemed from subs.
    else if (this.entitlement.isEntitledFromSubscription && !subscriptionManager.isActive)
    {
        return false;
    }
	else if (shouldFallbackPlayToParent(this.entitlement))
	{
		// Proxy the playability of our parent
		// This play() function has the same test inside it
		return this.entitlement.parent.playable && (!this.isUnreleased || this.entitlement.releaseDateBypassed) && (this.entitlement.owned || this.entitlement.previewContent);
	}

	return false;
});

Game.prototype.__defineGetter__('isCloudSyncInProgress', function()
{
    return this.entitlement.cloudSaves && this.entitlement.cloudSaves.syncOperation;
});

Game.prototype.__defineGetter__('isInstalled', function()
{
	return this.entitlement.installed;
});
Game.prototype.__defineGetter__('isInstallable', function()
{
	return this.entitlement.installable;
});

Game.prototype.__defineGetter__('isUninstallable', function()
{
	return this.entitlement.uninstallable &&
        (this.entitlement.displayLocation === 'GAMES' || this.entitlement.displayLocation === 'GAMES|EXPANSIONS' || this.entitlement.hasUninstaller);
});

Game.prototype.__defineGetter__('isFavorite', function()
{
	return this.entitlement.favorite;
});
Game.prototype.__defineSetter__('isFavorite', function(val)
{
	this.entitlement.favorite = val;
	this.favoriteEvent.fire(this);
});

Game.prototype.__defineGetter__('isEntitledFromSubscription', function()
{
    return this.entitlement.isEntitledFromSubscription;
});

Game.prototype.__defineGetter__('isHidden', function()
{
	return this.entitlement.hidden;
});
Game.prototype.__defineSetter__('isHidden', function(val)
{
	this.entitlement.hidden = val;
	this.hideEvent.fire(this);
});

Game.prototype.__defineGetter__('isUnreleased', function()
{
	return (this.entitlement.releaseStatus === 'UNRELEASED' || this.entitlement.releaseStatus === 'PRELOAD');
});
Game.prototype.__defineGetter__('isPreloaded', function()
{
    // EBIBUGS-27936 If it's a PI game - only show preloaded after the game is done downloading.
	// We can't use this.entitlement.currentOperation.isDynamicOperation in this case...
    var operation = this.entitlement.currentOperation,
	    isPreloading = operation && operation.type === "PRELOAD";
    return ((this.entitlement.releaseStatus === 'PRELOAD') && this.isInstalled && !isPreloading);
});

Game.prototype.__defineGetter__('wasPlayedThisWeek', function()
{
	return (this.entitlement.lastPlayedDate && Math.floor((Date.now() - this.entitlement.lastPlayedDate.getTime()) / ONE_DAY_MS) < 7);
});

Game.prototype.__defineGetter__('isNonOriginGame', function()
{
	return this.entitlement.itemSubType === 'NON_ORIGIN';
});

Game.prototype.__defineGetter__('totalSecondsPlayed', function()
{
	return this.entitlement.totalSecondsPlayed;
});

Game.prototype.__defineGetter__('hasPDLCStore', function()
{
	return this.entitlement.hasPDLCStore;
});

Game.prototype.__defineGetter__('filterGroups', function()
{
	if (this._cachedFilterGroups === null)
	{
		// Determine which filter groups we belong to
		var groups = [];

        if(this.entitlement.isCompletelySuppressed)
        {
            groups.push('internal_hidden');
        }
		else if (this.isHidden)
		{
			groups.push('hidden');
		}
        else if (this.entitlement.isSuppressed)
        {
            groups.push('purchased');
        }
		else
		{
			groups.push('all');

			if (this.wasPlayedThisWeek)
			{
				groups.push('played_this_week');
			}

			if (this.isFavorite)
			{
				groups.push('favorites');
			}
            
            // ORSUBS-1678: only add vault and purchase filter if the user is/was a subscriber
            if(subscriptionManager.firstSignupDate)
			{
				// ORSUBS-1639: Show trial games under Origin Access filter
			    if (this.entitlement.isEntitledFromSubscription || this.entitlement.itemSubType === 'TIMED_TRIAL_ACCESS')
    			{
					groups.push('vault');
				}
				else if (this.entitlement.owned)
				{
					groups.push('purchased');
				}
			}

			if (this.isDownloading)
			{
				groups.push('downloading');
			}

			if (this.isInstalled)
			{
				groups.push('installed');
			}

			if (this.isNonOriginGame)
			{
				groups.push('non_origin_games');
			}
		}

		this._cachedFilterGroups = groups;
	}

	return this._cachedFilterGroups;
});

Game.prototype.__defineGetter__('hoursPlayedString', function()
{
	var hoursPlayed = this.totalSecondsPlayed / (60 * 60);

	if (hoursPlayed === 0)
	{
		// 0 hours played
		// This is considered more PC than "never played"
		return tr("ebisu_client_hours_played").replace("%1", "0");
	}
	else
	{
		var displayHoursPlayed = Math.floor(hoursPlayed);

		if (displayHoursPlayed === 0)
		{
			// Less than an hour played
			return tr("ebisu_client_interval_less_than_hour_uppercase");
		}
		else if (displayHoursPlayed === 1)
		{
			// 1 hour played
			return tr("ebisu_client_hour_played").replace("%1", displayHoursPlayed);
		}
		else
		{
			// <x> hours played
			return tr("ebisu_client_hours_played").replace("%1", displayHoursPlayed);
		}
	}
});

Game.prototype.__defineGetter__('timeRemainingTilNotNew', function()
{
    var newTime;
    //for trials the 'new' duration is only 20 minutes since we want them to see the calendar violator
    if(this.isTrialType || this.entitlement.itemSubType === 'TIMED_TRIAL_ACCESS' || this.entitlement.itemSubType === 'TIMED_TRIAL_GAMETIME')
    {
        newTime = NEW_TIME_TRIAL;
    }
    else
    {
        newTime = NEW_TIME;
    }

    var timeRemain;
    if(this.entitlement.entitleDate)
    {
        timeRemain = this.entitlement.entitleDate.getTime() + newTime - Date.now();
    }
    else
    {
        timeRemain = 0;
    }
    return timeRemain;
});

Game.prototype.__defineGetter__('isNewType', function()
{
    return this.timeRemainingTilNotNew > 0;
});

Game.prototype.__defineGetter__('isTrialType', function()
{
    return this.entitlement.freeTrial;
});

Game.prototype.__defineGetter__('isAlphaType', function()
{
    return this.entitlement.itemSubType === 'ALPHA';
});

Game.prototype.__defineGetter__('isBetaType', function()
{
    return this.entitlement.itemSubType === 'BETA';
});

Game.prototype.__defineGetter__('isDemoType', function()
{
    return this.entitlement.itemSubType === 'DEMO';
});

Game.prototype.__defineGetter__('unownedContentAvailable', function()
{
    return this.entitlement.unownedContentAvailable;
});

Game.prototype.__defineGetter__('isUnownedNonNucleusDDExpansion', function()
{
    return (this.entitlement.displayLocation === 'EXPANSIONS' || this.entitlement.displayLocation === 'GAMES|EXPANSIONS') && this.entitlement.productType !== 'NUCLEUSDD' && !this.entitlement.owned && this.entitlement.purchasable;
});

Game.prototype.__defineGetter__('isUnownedNucleusDDExpansion', function()
{
    return (this.entitlement.displayLocation === 'EXPANSIONS' || this.entitlement.displayLocation === 'GAMES|EXPANSIONS') && this.entitlement.productType === 'NUCLEUSDD' && !this.entitlement.owned && this.entitlement.purchasable;
});

// Events
Game.prototype.onEntitlementChanged = function()
{
	// Update the context menu
	Origin.GameContextMenuManager.updateGameIfVisible(this);

	// Our filter group cache is invalid now
	this._cachedFilterGroups = null;

	this.changeEvent.fire(this);


    //reset our new timer on refresh/updates
    if(this._notNewTimerId !== null)
    {
        window.clearTimeout(this._notNewTimerId);
        this._notNewTimerId = null;
    }

    if(this.isNewType)
    {
	    this._notNewTimerId = window.setTimeout($.proxy(function()
	    {
		    this.changeEvent.fire(this);
	    }, this), this.timeRemainingTilNotNew);
    }

};

Game.prototype.onEntitlementCloudSavesChanged = function()
{
	// Update the context menu
	Origin.GameContextMenuManager.updateGameIfVisible(this);

	this.cloudSavesChangeEvent.fire(this);
};
Game.prototype.onEntitlementCloudSavesCurrentUsageChanged = function(currentUsage)
{
	this.cloudSavesCurrentUsageChangeEvent.fire(this, currentUsage);
};

Game.prototype.onEntitlementExpansionAdded = function(expansion)
{
	var expansionGame = new Origin.Game(expansion, false);
	this.expansions.push(expansionGame);
	this.expansionAddEvent.fire(this, expansionGame);
};

Game.prototype.onEntitlementExpansionRemoved = function(expansion)
{
	$.each(this.expansions, $.proxy(function(idx, expansionGame)
	{
		if (expansionGame.entitlement.id === expansion.id)
		{
			this.expansions.splice(idx, 1);
			this.expansionRemoveEvent.fire(this, expansionGame);
			return false;
		}
	}, this));
};

Game.prototype.onEntitlementAddonAdded = function(addon)
{
	var addonGame = new Origin.Game(addon, false);
	this.addons.push(addonGame);
	this.addonAddEvent.fire(this, addonGame);
};

Game.prototype.onEntitlementAddonRemoved = function(addon)
{
	$.each(this.addons, $.proxy(function(idx, addonGame)
	{
		if (addonGame.entitlement.id === addon.id)
		{
			this.addons.splice(idx, 1);
			this.addonRemoveEvent.fire(this, addonGame);
			return false;
		}
	}, this));
};

Game.prototype.onAchievementUpdate = function(achievementSet)
{


     if ( this.entitlement.achievementSet && this.entitlement.achievementSet.indexOf( achievementSet.id ) > -1  )
     {
         this.achievementSet = achievementSet;
     }

};


Game.prototype.__defineGetter__('hasAchievements', function()
{

    var gameAchievementSet = null, 
        entitlement = this.entitlement;

    // Workaround for EBIBUGS-28720
    if (entitlement.achievementSet && entitlement.achievementSet.length && (this.achievementSet === undefined))
    {
        $.each( achievementManager.achievementSets, function( index, achievementSet ){
            if ( entitlement.achievementSet && entitlement.achievementSet.indexOf( achievementSet.id ) !== -1 ){
                gameAchievementSet = achievementSet;
                return false;
            }
        });
        if ( gameAchievementSet ){
            this.achievementSet = gameAchievementSet;
        }
    }

    return ( this.achievementSet && this.achievementSet.achievements && this.achievementSet.achievements.length > 0 );
});

Game.prototype.__defineGetter__('earnedRp', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.earnedRp;
    } else {
        return null;
    }
});


Game.prototype.__defineGetter__('achievementId', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.id;
    } else {
        return null;
    }
});

Game.prototype.__defineGetter__('earnedXp', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.earnedXp;
    } else {
        return null;
    }

});

Game.prototype.__defineGetter__('totalXp', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.totalXp;
    } else {
        return null;
    }

});

Game.prototype.__defineGetter__('achievements', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.achievements;
    } else {
        return null;
    }
});

Game.prototype.__defineGetter__('total', function()
{
    if ( this.achievementSet ){
        return this.achievementSet.total;
    } else {
        return null;
    }

});
// Expose to the world
Origin.Game = Game;

})(jQuery);
