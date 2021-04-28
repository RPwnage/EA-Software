;(function($){
"use strict";

if (window.entitlementManager) { return; }

// If there is no entitlementManager, then let's create dummy data and simulate events

/**
 * CLASS: Connector
 */
var Connector = function()
{
    this.connections = [];
};
Connector.prototype.connect = function(callback)
{
    this.connections.push(callback);
};
Connector.prototype.trigger = function()
{
    var args = $.makeArray(arguments);
    $.each(this.connections, function(idx, callback)
    {
        callback.apply(null, args);
    });
};

/**
 * EntitlementCloudSaveBackup
 * Represents a backed up version of an entitlement's local save files
 */
var EntitlementCloudSaveBackup = function(data)
{
    data = $.extend(
    {
        created: null
    }, data);

    // Date the backup was performed
    this.created = data.created;
};

// Starts an interactive flow to restore the entitlement's local save files from this backup.
// The flow will confirm the restore operation with the user before proceeding
EntitlementCloudSaveBackup.prototype.restore = function()
{
    alert('Restoring...');
};

/**
 * EntitlementCloudSaves
 * Helper class for an entitlement's cloud save related functionality
 */
var EntitlementCloudSaves = function(data)
{
    data = $.extend(
    {
        autoSyncEnabled: false,
        lastBackup: null
    }, data);

    // Indicates if the entitlement's local save files are automatically synchronized with the cloud. Entitlements with autosync disabled are different from entitlements without cloud save support as they can still have associated remote usage and backups.
    this.autoSyncEnabled = Boolean(data.autoSyncEnabled);

    // Last successful backup of the local save files for the entitlement.
    // A backup is automatically created before pulling any changes from the remote cloud saves area.
    // This property will be null if no backup exists.
    this.lastBackup = (data.lastBackup)? new EntitlementCloudSaveBackup({ created: data.lastBackup }) : null;

    // The current usage of the remote cloud saves area in bytes.
    // This value locally is cached an updated from the server whenever a cloud sync is performed or pollCurrentUsage() is called.
    this.currentUsage = (!isNaN(data.currentUsage))? data.currentUsage : 0; // 20,971,520 = 20 MB - this is entitlement specific

    // The maximum usage of the remote cloud saves area for the entitlement in bytes.
    this.maximumUsage = (!isNaN(data.maximumUsage))? data.maximumUsage : 104857600; // 100MB - this is per entitlement

    // TODO - simulate
    // Current remote sync operation if one is in progress, null otherwise
    this.syncOperation = null; //(Math.floor(Math.random()*11) === 5)? new EntitlementOperation('sync', this, false, true) : null;
    this._simulateSyncTimer = null;

    // Emitted whenever any property of this object changes value
    this.changed = new Connector();

    // Emitted when the currentUsage property changes
    this.currentUsageChanged = new Connector();
};

// Requests a single asynchronous update of the remote cloud saves usage from the server.
// This should be called when the user is actively looking at cloud save usage and not for passive things such as warning badges.
EntitlementCloudSaves.prototype.pollCurrentUsage = function()
{
    // Randomly add storage
    if (Math.floor(Math.random()*11) === 5 && (this.currentUsage + 1048576) < this.maximumUsage)
    {
        this.currentUsage += 1048576; // Add a MB
        this.changed.trigger();
        this.currentUsageChanged.trigger(this.currentUsage);
    }
};
EntitlementCloudSaves.prototype._startSimulateSync = function()
{
    this.syncOperation = new EntitlementOperation('sync', this, false, true);
    this.changed.trigger();
    window.setTimeout($.proxy(this, '_simulateSync'), 2000);
};
EntitlementCloudSaves.prototype._simulateSync = function()
{
    this.syncOperation._phase = 'RUNNING';
    this.changed.trigger();
    this._simulateSyncTimer = window.setInterval($.proxy(this, '_simulateSyncInterval'), 110);
};

EntitlementCloudSaves.prototype._simulateSyncInterval = function()
{
    if (this.syncOperation.progress < 1.0)
    {
        this.syncOperation.progress += 0.01;
        this.syncOperation._secondsRemaining -= 10;
        this.syncOperation._bytesDownloaded  += 0.5;
    }
    else
    {
        this._clearSimulateSyncInterval();
        this.syncOperation._cancellable = false;
        this.syncOperation._phase = 'FINALIZING';
        this.syncOperation.progress = null;
        window.setTimeout($.proxy(this, '_finalizeSimulateSync'), 2000);
    }
    this.changed.trigger();
};

EntitlementCloudSaves.prototype._finalizeSimulateSync = function()
{
    this.syncOperation = null;
    this.changed.trigger();
};

EntitlementCloudSaves.prototype._clearSimulateSyncInterval = function()
{
    if (this._simulateSyncTimer)
    {
        window.clearInterval(this._simulateSyncTimer);
    }
    this._simulateSyncTimer = null;
};



/**
 * EntitlementOperation
 * Object describing an in-progress operation on an entitlement
 */
var EntitlementOperation = function(type, entitlement, supportsPauseResume, cancellable)
{
    this._type = type;
    this._entitlement = entitlement;
    this._supportsPauseResume = Boolean(supportsPauseResume); // Indicates if this operation supports pausing and resuming
    this._cancellable = Boolean(cancellable); // Indicates if this operation can be cancelled

    this._pauseTimer = null;
    this._paused = false; // Indicates if the current operation is paused. Assigning true or false to this property will pause and resume the operation respectively. Not all operations support pausing; those operations will have this property permanently set to false.
    this.progress = null; // Progress of the current operation. Between 0.0 (0%) and 1.0 (100%)
    this._phase = 'PREPARING'; // Current phase of the operation
    this._totalFileSize = 100000;
    this._bytesPerSecond = 100000;
    this._bytesDownloaded = 100000;
    this._secondsRemaining = 10000;
    this._playableAt = 0.2;
    this._progressState = 'State-Active';
	this._isDynamicOperation = true;
    /*
        "CANCELLING"    Preparing to cancel. This phase may not be entered for operations that do not take significant time to cancel.
        "PAUSED"        Paused due to a user request
        "PAUSING"        Preparing to pause. This phase may not be entered for operations that do not take significant time to pause.
        "PREPARING"        Performing initial preperation before the main transfer can begin. This phase can also apply to transfers resuming from a paused state
        "RUNNING"    Downloading or uploading the main payload. This is main phase for an operation.
        "FINALIZING"
    */
};

EntitlementOperation.prototype.__defineGetter__('isDynamicOperation', function()
{
    return this._isDynamicOperation;
});
EntitlementOperation.prototype.__defineGetter__('type', function()
{
    return this._type;
});
EntitlementOperation.prototype.__defineGetter__('typeDisplay', function()
{
    return this._type;
});
EntitlementOperation.prototype.__defineGetter__('progressState', function()
{
    switch (this.phase)
     {
        case 'PAUSING':
        case 'PAUSED':
            this._progressState = "State-Paused";
            break;
        case 'RESUMING':
        case 'PREPARING':
        case 'CANCELLING':
        case 'INITIALIZING':
            this._progressState = "State-Indeterminate";
            break;
        default:
            this._progressState = "State-Active";
            break;
        }
    return this._progressState;
});
EntitlementOperation.prototype.specificPhaseDisplay = function(phase)
{
    return this._type+"-"+phase;
};
EntitlementOperation.prototype.shouldLightFlag = function(flagName)
{
    return this._entitlement.installed && (this._entitlement.playable || this._entitlement.playing);
};
EntitlementOperation.prototype.__defineGetter__('phaseDisplay', function()
{
    return this._type+"-"+this._phase;
});
EntitlementOperation.prototype.__defineGetter__('totalFileSize', function()
{
    return this._totalFileSize;
});
EntitlementOperation.prototype.__defineGetter__('bytesPerSecond', function()
{
    return this._bytesPerSecond;
});
EntitlementOperation.prototype.__defineGetter__('bytesDownloaded', function()
{
    return this._bytesDownloaded;
});
EntitlementOperation.prototype.__defineGetter__('secondsRemaining', function()
{
    return this._secondsRemaining;
});
EntitlementOperation.prototype.__defineGetter__('playableAt', function()
{
    return this._playableAt;
});
EntitlementOperation.prototype.__defineGetter__('phase', function()
{
    return this._phase;
});
EntitlementOperation.prototype.__defineGetter__('pausable', function()
{
    return this._supportsPauseResume &&
        (this.phase !== 'PAUSED') &&
        (this.phase !== 'PAUSING') &&
        (this.phase !== 'ENQUEUED');
});
EntitlementOperation.prototype.__defineGetter__('resumable', function()
{
    return this._supportsPauseResume &&
        ((this.phase === 'PAUSED') || (this.phase === 'ENQUEUED'));
});
EntitlementOperation.prototype.__defineGetter__('cancellable', function()
{
    return this._cancellable;
});

EntitlementOperation.prototype.pause = function()
{
    if (!this.pausable)
    {
        console.log('EntitlementOperation.pause: This operation is not pausable');
        return;
    }

    if (this._pauseTimer)
    {
        window.clearTimeout(this._pauseTimer);
        this._pauseTimer = null;
    }

    switch (this._type)
    {
        case ('download'):
        {
            this._entitlement._clearSimulateDownloadInterval();
            break;
        }
        case ('install'):
        {
            this._entitlement._clearSimulateInstallInterval();
            break;
        }
        case ('update'):
        {
            this._entitlement._clearSimulateUpdateInterval();
            break;
        }
        case ('repair'):
        {
            this._entitlement._clearSimulateRepairInterval();
            break;
        }
    }

    this._phase = 'PAUSING';
    this._entitlement.changed.trigger();

    // Simulate a delayed pause
    this._pauseTimer = window.setTimeout(this._delayedPause.bind(this), 2000);
};

EntitlementOperation.prototype._delayedPause = function()
{
    this._paused = true;

    var entitlement = this._entitlement;

    this._phase = 'PAUSED';
    entitlement.changed.trigger();
};

EntitlementOperation.prototype.resume = function()
{
    if (!this.resumable)
    {
        console.log('EntitlementOperation.resume: This operation is not resumable');
        return;
    }

    switch (this._type)
    {
        case ('download'):
        {
            this._entitlement._simulateDownload();
            break;
        }
        case ('install'):
        {
            this._entitlement._simulateInstall();
            break;
        }
        case ('update'):
        {
            this._entitlement._simulateUpdate();
            break;
        }
        case ('repair'):
        {
            this._entitlement._simulateRepair();
            break;
        }
    }
}

// Cancels the current operation
EntitlementOperation.prototype.cancel = function()
{
    if (!this.cancellable)
    {
        console.log('EntitlementOperation.cancel:: This operation does not support cancelling - exiting.');
        return;
    }

    if (this.phase === 'CANCELLING') { return; }

    if (this._pauseTimer)
    {
        window.clearTimeout(this._pauseTimer);
        this._pauseTimer = null;
    }

    switch (this._type)
    {
        case ('download'):
        {
            this._entitlement._clearSimulateDownloadInterval();
            break;
        }
        case ('install'):
        {
            this._entitlement._clearSimulateInstallInterval();
            break;
        }
        case ('update'):
        {
            this._entitlement._clearSimulateUpdateInterval();
            break;
        }
        case ('repair'):
        {
            this._entitlement._clearSimulateRepairInterval();
            break;
        }
        case ('sync'):
        {
            this._entitlement._clearSimulateSyncInterval();
            break;
        }
    }

    this._phase = 'CANCELLING';
    this._entitlement.changed.trigger();

    // Simulate a delayed cancel
    window.setTimeout(this._delayedCancel.bind(this), 2000);
};

EntitlementOperation.prototype._delayedCancel = function()
{
    if (!this.cancellable)
    {
        console.log('EntitlementOperation.cancel:: This operation does not support cancelling - exiting.');
        return;
    }

    var entitlement = this._entitlement;

    // Delete the reference to the operation
    this._entitlement = null;
    entitlement[this._type + 'Operation'] = null;

    switch (this._type)
    {
        case ('download'):
        case ('install'):
        {
            entitlement._state = 'READY_TO_DOWNLOAD';
            entitlement.downloadable = true;
            break;
        }
        case ('update'):
        case ('repair'):
        {
            entitlement._state = 'READY_TO_PLAY';
            break;
        }

        default:
        {
        }
    }

    entitlement.changed.trigger();
    contentOperationQueueController.remove(entitlement.productId, true);
};


/**
 * CLASS: Entitlement
 * Object describing a game or addon an user is entitled to
 */
var Entitlement = function(data)
{
    data = $.extend(
    {
        addons: [],
        autoUpdateEnabled: false,
        boxartUrls: [],
        bannerUrls: [],
        cloudSaves: null,
        contentId: '',
        debugInfo: null,
        downloadOperation: null,
        displayLocation: 'GAMES',
        displayDownloadStartDate: null,
        displayExpirationDate: null,
        displayUnlockStartDate: null,
        downloadable: false,
        entitleDate: null,
        expansions: [],
        id: null,
        installed: false,
        installOperation: null,
        itemSubType: 'NORMAL_GAME',
        lastPlayedDate: null,
        latestStartPurchaseDate: null,
        longDescription: '',
        manualUrl: null,
        nonOriginGame: null,
        boxart: null,
        parent: null,
        platformsSupported: [],
        multiLaunchTitles: [],
        multiLaunchDefault: '',
        owned: true,
        playable: false,
        playableBitSet: true,
        playing: false,
        releaseStatus: 'UNRELEASED',
        registrationCode: null,
        repairOperation: null,
        repairSupported: false,
        dynamicContentSupportEnabled: false,
        _state: 'WAITING_TO_DOWNLOAD',
        shortDescription: '',
        testCode: null,
        thumbnailUrls: [],
        title: null,
        totalSecondsPlayed: 0,
        type: 'GAME',
        availableUpdateVersion: null,
        updateOperation: null,
        updateSupported: false,
        unownedContentAvailable: false,
        newUnownedContentAvailable: false,
        hasPDLCStore: false,
        debugActions: [],
        _legacyPackage: false,
        _uninstallableIfInstalled: true,
        preAnnouncementDisplayString: ''
    }, data);

    this.addons = data.addons; // Array of addons for this game that the user is entitled to
    this._autoUpdateEnabled = Boolean(data.autoUpdateEnabled); // Indicates if the entitlement will automatically check for and apply updates before the entitlement is played
    this.boxartUrls = data.boxartUrls; // URL for the entitlement's box art
    this.bannerUrls = data.bannerUrls; // URL for the entitlement's banner image
    this.cloudSaves = data.cloudSaves; // Cloud saves related information for the entitlement. If the entitlement does not support cloud saves this property will be null.
    this.contentId = data.contentId; // Content ID of the entitlement. This roughly corresponds to a game but various editions and regional versions of a game may have different content IDs. Some addons or expansions may not have associated content IDs
    this.debugInfo = data.debugInfo; // Dictionary of arbitrary key-value pairs to be shown as debug information for the entitlement or null if none should be shown. These values are internal and    may change at any time.
    this.downloadOperation = data.downloadOperation; // The current download operation if one is in progress, null otherwise
    this.displayDownloadStartDate = data.displayDownloadStartDate; // Date the entitlement is available to download. This is referred to as the preload date in the user interface.
    this.displayLocation = data.displayLocation; // Description of where the entitlement should be displayed
    this.displayExpirationDate = data.displayExpirationDate; // Date the entitlement expires
    this.displayUnlockStartDate = data.displayUnlockStartDate; // Date the entitlement is available to play. This is referred to as the release date in the user interface.
    this.downloadable = Boolean(data.downloadable); // Indicates if the entitlement is downloadable. This means that the entitlement isn't already downloaded and no date-based restrictions are active.
    this.entitleDate = data.entitleDate; // Date the entitlement was granted to the user. This is referred to as the purchase date in the user interface.
    this.expansions = data.expansions;
    this.id = data.id; // Content ID of the entitlement. This an internal unique identifier for the content. This roughly corresponds to a game but various editions and regional versions of a game may have different content IDs. A user may only have one entitlement for a given content ID so it can also be used as a stable identifier for the user's entitlement.
    this.installed = data.installed;
    this.installOperation = data.installOperation; // The current install operation if one is in progress, null otherwise
    this.itemSubType = data.itemSubType; // Entitlement's game type
    /*
         "ALPHA"                    Game is the alpha demo version
         "BETA"                    Game is the beta demo version
         "DEMO"                    Game is a demo
         "FREE_WEEKEND_TRIAL"    Game is a free for a weekend
         "LIMITED_TRIAL"            Game is free for a limited time
         "LIMITED_WEEKEND_TRIAL"    Game is free for a limited weekend
         "NON_ORIGIN"             Game was added to the library but wasn't bought via Origin
         "NORMAL_GAME"            Origin game
     */
    this.lastPlayedDate = data.lastPlayedDate; // Date the user last played the entitlement on this machine. This property will be null if the user has never played the entitlement.
    this.latestStartPurchaseDate = data.latestStartPurchaseDate; // Latest 'start purchase date' for unowned and purchasable content
    this.manualUrl = data.manualUrl; // URL for the entitlement's manual
    this.nonOriginGame = data.nonOriginGame; // Information tied to the non-Origin game
    this.boxart = data.boxart; // Information tied to an entitlements boxart
    this.parent = data.parent;
    this.currentPrice = data.currentPrice;
    this.shortDescription = data.shortDescription;
    this.longDescription = data.longDescription;
    this.thumbnailUrls = data.thumbnailUrls;
    this.platformsSupported = data.platformsSupported; // Which platforms are supported by this entitlement
    this.multiLaunchTitles = data.multiLaunchTitles; // What multi launch options there are
    this.multiLaunchDefault = data.multiLaunchDefault; // What multi launch perferred option is
    this.owned = data.owned; // Indicates if the entitlement owned by the current user.
    this.playable = Boolean(data.playable); // Indicates if the entitlement can be played. This means that entitlement is installed, no date-based restrictions are active, and isn't currently being played
    this.playableBitSet = Boolean(data.playableBitSet); // Indicates if the game can be played the user's machine bitset.
    this.playing = Boolean(data.playing); // Indicates if the entitlement is currently being played
    this.preAnnouncementDisplayString = data.preAnnouncementDisplayString;
    this.releaseStatus = data.releaseStatus; // The current release status of the entitlement
    /*
        "AVAILABLE"        Available for download and can be played. May transition to EXPIRED at some point in the future.
        "EXPIRED"        No longer available for download and cannot be played.
        "PRELOAD"        Available for download but cannot be played. Will transition to AVAILABLE at some point in the future.
        "UNRELEASED"    Unreleased and unavailable for download. Will be transition to PRELOAD or AVAILABLE at some point in the future.
    */
    this.registrationCode = data.registrationCode; // Registration code (also known as product key) for the entitlement
    this.repairOperation = data.repairOperation; // The current repair operation if one is in progress, null otherwise
    this.repairSupported = Boolean(data.repairSupported); // Indicates if this entitlement supports repair operations. Legacy titles typically do not support repair unless they have been recently repackaged.
    this.dynamicContentSupportEnabled = data.dynamicContentSupportEnabled;
    this.testCode = data.testCode; // Publishing test code associated with the test code associated with the entitlement or null if no test code is associated. Possible values: 1102 − 1105
    this._state = data._state; // State of the entitlement
    /*
        "DOWNLOADING"     A download is in progress
        "INSTALLING"     The game's installer is running
        "PLAYING"     Currently being played
        "READY_TO_DOWNLOAD"     Available to download but not installed
        "READY_TO_PLAY"     Installed and ready to play
        "REPAIRING"     A repair operation is in progress
        "UPDATING"     An update is being installed
        "WAITING_TO_DOWNLOAD"     Waiting until the displayDownloadStartDate to be downloadable
        "UNPACKING"  A game packaged in a legacy format is unpacking to its final destination.
        "READY_TO_INSTALL"  The downloaded game is waiting for its installer to be invoked with Entitlement.install
    */
    this.title = data.title; // User visible localized title of the entitlement
    this.totalSecondsPlayed = data.totalSecondsPlayed; // Total number of seconds the game has been played for
    this.displayLocation = data.displayLocation; // Where to display the entitlement
    /*
        "ADDON"     Add-on. These differ from expasions only in their UI presentation
        "EXPANSION"     Expansions. These different from addons only in their UI presentation.
        "GAME"    Standalone game
    */
    this.availableUpdateVersion = data.availableUpdateVersion;
    this.updateOperation = data.updateOperation; // The current update operation if one is in progress, null otherwise
    this.updateSupported = Boolean(data.updateSupported); // Indicates if this entitlement supports update operations. Legacy titles typically do not support repair unless they have been recently repackaged.
    this.unownedContentAvailable = Boolean(data.unownedContentAvailable); // Indicates if this entitlement has unowned content available
    this.newUnownedContentAvailable = Boolean(data.newUnownedContentAvailable); // Indicates if there is new unowned content for this title (1 week from release date)
    // Signal: Emitted whenever any property of the entitlement changes value
    this.changed = new Connector();

    // Signal: Emitted when the application wants to ensure an entitlement is visible to the user. This is typically emitted after a new entitlement has been added.
    // This is only a hint to the primary entitlement view; other users of this object can safely ignore this signal.
    this.ensureVisible = new Connector();

    // Signal: Emitted whenever an addon is added
    this.addonAdded = new Connector();
    this.addonRemoved = new Connector();

    // Signal: Emitted whenever an expansion is added
    this.expansionAdded = new Connector();
    this.expansionRemoved = new Connector();

    // Indicates if certain commerce features are available
    this.hasPDLCStore = Boolean(data.hasPDLCStore);

    // Any debug actions available on the entitlement
    this.debugActions = data.debugActions;

    // Private flag that determines if we simulate a legacy package an unpack phase
    this._legacyPackage = Boolean(data._legacyPackage);

    // Private flag indicating if we're uninstallable if we're installed
    this._uninstallableIfInstalled = Boolean(data._uninstallableIfInstalled);

    // Set our PDLC's parent to ourselves
    $.each(this.addons, $.proxy(function(idx, addon)
    {
        addon.parent = this;
    }, this));

    $.each(this.expansions, $.proxy(function(idx, expansion)
    {
        expansion.parent = this;
    }, this));

    // Simulated timers
    this._simulateDownloadTimer = null;
    this._simulateUpdateTimer = null;
    this._simulateRepairTimer = null;
    this._simulateUnpackTimer = null;
};

// DONT USE! For QT C++
Entitlement.prototype.deleteLater = function(){};

// This is almost the same in C++ except for some unowned madness which should
// be removed
Entitlement.prototype.__defineGetter__('productId', function()
{
    return this.id;
});

// Implement favoriting and hiding on top of local storage
Entitlement.prototype.__defineGetter__('favorite', function()
{
    return $.jStorage.get('favorite-' + this.id) || false;
});

Entitlement.prototype.__defineSetter__('favorite', function(val)
{
    // Store in local storage
    $.jStorage.set('favorite-' + this.id, Boolean(val));
    this.changed.trigger();
});

Entitlement.prototype.__defineGetter__('hidden', function()
{
    return $.jStorage.get('hidden-' + this.id) || false;
});

Entitlement.prototype.__defineSetter__('hidden', function(val)
{
    // Store in local storage
    $.jStorage.set('hidden-' + this.id, Boolean(val));
    this.changed.trigger();
});


// Indicates if the entitlement will automatically check for and apply updates before the entitlement is played
Entitlement.prototype.__defineGetter__('autoUpdateEnabled', function()
{
    return this._autoUpdateEnabled;
});

Entitlement.prototype.__defineGetter__('currentOperation', function()
{
	if (this.cloudSaves && this.cloudSaves.syncOperation)
    {
        return this.cloudSaves.syncOperation;
    }
    else if (this.updateOperation)
    {
        return this.updateOperation;
    }
    else if (this.downloadOperation)
    {
        return this.downloadOperation;
    }
    else if (this.installOperation)
    {
        return this.installOperation;
    }
    else if (this.repairOperation)
    {
        return this.repairOperation;
    }
    else if (this.unpackOperation)
    {
        return this.unpackOperation;
    }
});



/** ******************
 * Downloading
 */

// Starts a download for the entitlement
Entitlement.prototype.startDownload = function()
{

    if (!this.downloadable)
    {
        console.log('Entitlement.startDownload:: Warning! Entitlement is not currently downloadable.');
        return;
    }
    if (this.downloadOperation)
    {
        console.log('Entitlement.startDownload:: Warning! Download operation already exists');
        return;
    }

    this._state = 'DOWNLOADING';
    this.downloadOperation = new EntitlementOperation('download', this, true, true);
    this.downloadOperation._phase = "ENQUEUED";
    this.downloadable = false;
    contentOperationQueueController.enqueue(this);
    this.changed.trigger();
};

Entitlement.prototype._simulateDownload = function()
{
    this.downloadOperation._phase = 'RUNNING';
    this.downloadOperation.progress = 0.0;
    this.changed.trigger();

    this._simulateDownloadTimer = window.setInterval($.proxy(this, '_simulateDownloadInterval'), 110);
};

Entitlement.prototype._simulateDownloadInterval = function()
{
    if (this.downloadOperation.progress < 1.0)
    {
        if(this.downloadOperation.isDynamicOperation && this.downloadOperation.progress > this.downloadOperation.playableAt)
        {
            this.installed = true;
            this.playable = true;
        }
        this.downloadOperation.progress += 0.03;
    }
    else
    {
        this._clearSimulateDownloadInterval();
        this.downloadable = false;
        this.downloadOperation._cancellable = false;
        this.downloadOperation._phase = 'FINALIZING';
        this.downloadOperation.progress = null;
        window.setTimeout($.proxy(this, '_finalizeSimulateDownload'), 2000);
    }
    this.changed.trigger();
};

Entitlement.prototype._finalizeSimulateDownload = function()
{
    console.log('_finalizeSimulateDownload');
    this.downloadOperation = null;

    if (this._legacyPackage)
    {
        // Start unpack
        this._simulateUnpack();
    }
    else
    {
        // Automatically install
        this.install();
    }
};

Entitlement.prototype._clearSimulateDownloadInterval = function()
{
    if (this._simulateDownloadTimer)
    {
        window.clearInterval(this._simulateDownloadTimer);
    }
    this._simulateDownloadTimer = null;
};

/** ******************
 * Installing
 */

Entitlement.prototype.install = function()
{
    console.log('install');

    if (this.installOperation)
    {
        console.log('Entitlement._startInstall:: Warning! Install operation already exists');
        return;
    }

    this._state = 'INSTALLING';
    this.installOperation = new EntitlementOperation('install', this, false, false);
    this.installOperation._bytesPerSecond = 0;
    this.changed.trigger();
    contentOperationQueueController.headBusy.trigger(true);

    window.setTimeout($.proxy(this, '_simulateInstall'), 2000);
};
Entitlement.prototype.__defineGetter__('installable', function()
{
    return this._state == "READY_TO_INSTALL";
});
Entitlement.prototype._simulateInstall = function()
{
    this.installOperation._phase = 'RUNNING';
    this.changed.trigger();

    this._simulateInstallTimer = window.setInterval($.proxy(this, '_simulateInstallInterval'), 110);
};
Entitlement.prototype._simulateInstallInterval = function()
{
    if (this.installOperation.progress < 1.0)
    {
        this.installOperation.progress += 0.04;
    }
    else
    {
        this._clearSimulateInstallInterval();
        this._state = 'READY_TO_PLAY';

        if ((this.releaseStatus !== 'PRELOAD') &&
            (this.displayLocation !== 'EXPANSIONS'))
        {
            this.playable = true;
        }

        this.installed = true;
        this.installOperation = null;

        // Mark all of our subentitlements as downloadable
        var subentitlements = $.merge([], this.addons);
        $.merge(subentitlements, this.expansions);
        $.each(subentitlements, function(idx, subEnt)
        {
            if (subEnt.releaseStatus === 'RELEASED')
            {
                subEnt.downloadable = true;
                subEnt.changed.trigger();
            }
        });
        contentOperationQueueController.removeFromQueueAndAddToComplete(this);
    }
    this.changed.trigger();
};

Entitlement.prototype._clearSimulateInstallInterval = function()
{
    if (this._simulateInstallTimer)
    {
        window.clearInterval(this._simulateInstallTimer);
    }
    this._simulateInstallTimer = null;
};


/** ******************
 * Updating
 */

// Checks for new updates from the server while showing a native progress dialog.
// If an update is available it will be automatically installed.
Entitlement.prototype.checkForUpdateAndInstall = function()
{
    if (this.updateSupported && this._state === 'READY_TO_PLAY')
    {
        // Random
        if (Math.floor(Math.random()*11) === 5)
        {
            // Give it a random version number
            this.availableUpdateVersion = String(Math.floor(Math.random() * 10));
            this.installUpdate();
        }
    }
};

// Installs an update if one is available. This will not check for an update from the server;
// it should only be called if the availableUpdateVersion property is not null.
Entitlement.prototype.installUpdate = function()
{
    if (this.updateSupported &&
        (this.availableUpdateVersion !== null) &&
        this.installed &&
        !this.updateOperation)
    {
        this._state = 'UPDATING';
        this.updateOperation = new EntitlementOperation('update', this, true, true);
        this.changed.trigger();
    }
};

Entitlement.prototype._simulateUpdate = function()
{
    this.updateOperation._phase = 'RUNNING';
    this.updateOperation.progress = 0.0;
    this.changed.trigger();

    this._simulateUpdateTimer = window.setInterval($.proxy(this, '_simulateUpdateInterval'), 110);
};

Entitlement.prototype._simulateUpdateInterval = function()
{
    if (this.updateOperation.progress < 1.0)
    {
        this.updateOperation.progress += 0.01;
    }
    else
    {
        this._state = 'READY_TO_PLAY';
        this.updateOperation = null;
        this.availableUpdateVersion = null;
        this._clearSimulateUpdateInterval();
    }
    this.changed.trigger();
};

Entitlement.prototype._clearSimulateUpdateInterval = function()
{
    if (this._simulateUpdateTimer)
    {
        window.clearInterval(this._simulateUpdateTimer);
    }
    this._simulateUpdateTimer = null;
};


/** ******************
 * Repairing
 */

// Starts repairing the entitlement and displays a native dialog informing the user of the repair progress
Entitlement.prototype.repair = function()
{
    if (this.repairSupported && this._state === 'READY_TO_PLAY' && !this.repairOperation)
    {
        this._state = 'REPAIRING';
        this.repairOperation = new EntitlementOperation('repair', this, true, true);
        this.changed.trigger();
    }
};

Entitlement.prototype._simulateRepair = function()
{
    this._state = 'REPAIRING';

    this.repairOperation._phase = 'RUNNING';
    this.repairOperation.progress = 0.0;
    this.changed.trigger();

    this._simulateRepairTimer = window.setInterval($.proxy(this, '_simulateRepairInterval'), 110);
};

Entitlement.prototype._simulateRepairInterval = function()
{
    if (this.repairOperation.progress < 1.0)
    {
        this._state = 'REPAIRING';
        this.repairOperation.progress += 0.01;
    }
    else
    {
        this._state = 'READY_TO_PLAY';
        this.repairOperation = null;
        this._clearSimulateRepairInterval();
    }
    this.changed.trigger();
};

Entitlement.prototype._clearSimulateRepairInterval = function()
{
    if (this._simulateRepairTimer)
    {
        window.clearInterval(this._simulateRepairTimer);
    }
    this._simulateRepairTimer = null;
};

/** *****************
 * Unpacking
 */
Entitlement.prototype._simulateUnpack = function()
{
    this._state = 'UNPACKING';

    this.unpackOperation = new EntitlementOperation('unpack', this, false, true);
    this.unpackOperation._phase = 'RUNNING';
    this.changed.trigger();
};

Entitlement.prototype._simulateUnpackInterval = function()
{
    if (this.unpackOperation.progress < 1.0)
    {
        this._state = 'UNPACKING';
        this.unpackOperation.progress += 0.02;
    }
    else
    {
        this._state = 'READY_TO_INSTALL';
        this.unpackOperation = null;
        this._clearSimulateUnpackInterval();

        // Don't automatically install
    }
    this.changed.trigger();
};

Entitlement.prototype._clearSimulateUnpackInterval = function()
{
    if (this._simulateUnpackTimer)
    {
        window.clearInterval(this._simulateUnpackTimer);
    }
    this._simulateUnpackTimer = null;
};



/** ******************
 * Uninstalling
 */

Entitlement.prototype.__defineGetter__('uninstallable', function()
{
    return this.installed && this._uninstallableIfInstalled;
});

// Prompts the user to uninstall the entitlement.
// The entitlement will be returned to the READY_TO_DOWNLOAD state if the uninstall completes.
Entitlement.prototype.uninstall = function()
{
    if (this._state === 'READY_TO_PLAY')
    {
        this._state = 'READY_TO_DOWNLOAD';
        this.changed.trigger();

        window.setTimeout($.proxy(this, '_simulateUninstall'), 3000);
    }
};

Entitlement.prototype._simulateUninstall = function()
{
    this._state = 'READY_TO_DOWNLOAD';
    this.changed.trigger();
};


/** ******************
 * Playing
 */

// Launches the entitlement
Entitlement.prototype.play = function()
{
    if (!this.playable)
    {
        console.log('Entitlement.play:: Warning! Entitlement is not currently playable.');
        return;
    }
    if (this._state !== 'READY_TO_PLAY' || this.releaseStatus !== 'AVAILABLE')
    {
        console.log('Entitlement.play:: Not ready to play');
        return;
    }
    $.jStorage.set('recently-played-' + this.id, true);

    this._state = 'PLAYING';
    this.playing = true;
    this.playable = false;
    this.changed.trigger();

    if (this.cloudSaves)
    {
        this.cloudSaves._startSimulateSync();
    }
    window.setTimeout($.proxy(this, '_delayedPlayExit'), 20000);
};

Entitlement.prototype._delayedPlayExit = function()
{
    this._state = 'READY_TO_PLAY';
    this.playing = false;
    this.playable = true;
    this.changed.trigger();

    if (this.cloudSaves)
    {
        this.cloudSaves._startSimulateSync();
    }
};


var ContentOperationQueueController = function()
{
    this.enqueued = new Connector();
    this.removed = new Connector();
    this.addedToComplete = new Connector();
    this.completeListCleared = new Connector();
    this.headBusy = new Connector();

    this._entitlementsQueued = [];
    this._entitlementsCompleted = [];
}

// Returns a list of entitlement objects belonging to the current user
ContentOperationQueueController.prototype.__defineGetter__('entitlementsQueued', function()
{
    return this._entitlementsQueued;
});

ContentOperationQueueController.prototype.__defineGetter__('entitlementsCompleted', function()
{
    return this._entitlementsCompleted;
});

ContentOperationQueueController.prototype.__defineGetter__('head', function()
{
    return this._entitlementsQueued[0];
});

ContentOperationQueueController.prototype.isHeadBusy = function(entitlement)
{
    return this._entitlementsQueued.length
    && (this.head.currentOperation.pausable || this.head.currentOperation.resumable) === false;
}

ContentOperationQueueController.prototype.enqueue = function(entitlement)
{
    this._entitlementsQueued = $.merge($.merge([], this._entitlementsQueued),[entitlement]);
    this.enqueued.trigger(entitlement);

    if(entitlement.productId === this.head.productId)
    {
        entitlement.downloadOperation.resume();
    }
}

ContentOperationQueueController.prototype.pushToTop = function(id)
{
    var newHead = entitlementManager.getEntitlementById(id, true),
        oldhead = this.head,
        newHeads = this.baseGroup(newHead),
        oldHeads = this.baseGroup(oldhead),
        i = 0,
        iEnt;

    oldhead.currentOperation.pause();
    newHeads = $.merge($.merge([], newHeads), oldHeads);

    for(i = 0; i < newHeads.length; i++)
    {
        iEnt = newHeads[i];
        this._entitlementsQueued = $.grep(this._entitlementsQueued, function(entitlement) {return entitlement.productId != iEnt.productId;});
        this.removed.trigger(iEnt.productId, true, false);
    }

    this._entitlementsQueued = $.merge($.merge([], newHeads), this._entitlementsQueued);

    for(i = 0; i < newHeads.length; i++)
    {
        this.enqueued.trigger(newHeads[i]);
    }

    if(newHead.downloadOperation)
    {
        newHead.downloadOperation.resume();
    }
    else
    {
        newHead.startDownload();
        newHead.downloadOperation.resume();
    }
}

ContentOperationQueueController.prototype.baseGroup = function(baseEnt)
{
    var arr = [],
        i = 0,
        iEnt = null;
    for(i = this.index(baseEnt.id); i < this._entitlementsQueued.length; i++)
    {
        iEnt = this._entitlementsQueued[i];
        if(iEnt &&
           (iEnt.parent === baseEnt || iEnt === baseEnt))
        {
            arr = $.merge($.merge([], arr), [iEnt]);
        }
        else
        {
            break;
        }
    }
    return arr;
}

ContentOperationQueueController.prototype.removeFromQueueAndAddToComplete = function(entitlement)
{
    console.log("removeFromQueueAndAddToComplete: " + entitlement.productId);
    this.remove(entitlement.productId, false);
    this._entitlementsCompleted = $.merge($.merge([], this._entitlementsCompleted), [entitlement]);
    this.addedToComplete.trigger(entitlement);
}

ContentOperationQueueController.prototype.parentInQueue = function(_id)
{
    var ent = entitlementManager.getEntitlementByProductId(_id, true);
    if(ent == null)
        return null;
    return ent.parent;
}


ContentOperationQueueController.prototype.remove = function(_id, removeChildren)
{
    console.log("QueueController::remove " + _id);
    var ent = entitlementManager.getEntitlementByProductId(_id, true);
    var wasHead = ent === this.head;

    if(removeChildren)
    {
        for(var i = this.index(_id) + 1; i < this._entitlementsQueued.length;)
        {
            if(this._entitlementsQueued[i] && this._entitlementsQueued[i].parent === ent)
            {
                console.log("removing child " + this._entitlementsQueued[i].productId);
                this.remove(this._entitlementsQueued[i].productId);
            }
            else
            {
                break;
            }
        }
    }

    this._entitlementsQueued = $.grep(this._entitlementsQueued, function(entitlement) {return entitlement.productId != _id;});
    this._entitlementsCompleted = $.grep(this._entitlementsCompleted, function(entitlement) {return entitlement.productId != _id;});
    this.removed.trigger(_id, removeChildren, true);

    if(wasHead && this.head)
    {
        if(this.head.downloadOperation)
        {
            this.head.downloadOperation.resume();
        }
        else
        {
            this.head.startDownload();
            this.head.downloadOperation.resume();
        }
        this.headBusy.trigger(false);
    }
};


ContentOperationQueueController.prototype.clearCompleteList = function()
{
    this._entitlementsCompleted = [];
    this.completeListCleared.trigger();
};


ContentOperationQueueController.prototype.index = function(_id)
{
    for(var i=0;i<this._entitlementsQueued.length;i++)
    {
        if(this._entitlementsQueued[i].productId === _id)
            return i;
    }
    return -1;
};


ContentOperationQueueController.prototype.isInQueue = function(_id)
{
    for(var i=0;i<this._entitlementsQueued.length;i++)
    {
        if(this._entitlementsQueued[i].productId === _id)
            return true;
    }
    return false;
};


ContentOperationQueueController.prototype.isInQueueOrCompleted = function(_id)
{
    for(var i=0;i<this._entitlementsQueued.length;i++)
    {
        if(this._entitlementsQueued[i].productId === _id)
            return true;
    }
    for(var i=0;i<this._entitlementsCompleted.length;i++)
    {
        if(this._entitlementsCompleted[i].productId === _id)
            return true;
    }
    return false;
};

ContentOperationQueueController.prototype.queueSkippingEnabled = function(id)
{
    var operation = null;

    if(this.head)
    {
        if (this.head.cloudSaves && this.head.cloudSaves.syncOperation)
        {
            operation = this.head.cloudSaves.syncOperation;
        }
        else if (this.head.updateOperation)
        {
            operation = this.head.updateOperation;
        }
        else if (this.head.downloadOperation)
        {
            operation = this.head.downloadOperation;
        }
        else if (this.head.installOperation)
        {
            operation = this.head.installOperation;
        }
        else if (this.head.repairOperation)
        {
            operation = this.head.repairOperation;
        }
        else if (this.head.unpackOperation)
        {
            operation = this.head.unpackOperation;
        }
    }

	return (!(this.head) || this.head.productId !== id)
            && this.index(id) !== -1
            && operation
            && (operation.pausable || operation.resumable);
}

ContentOperationQueueController.prototype._findEntitlement = function(filterFunc)
{
    for (var topLevelId in this._entitlementsQueued)
    {
        var topLevel = this._entitlementsQueued[topLevelId];

        if (filterFunc(topLevel))
        {
            return topLevel;
        }
    }

    for (var topLevelId in this._entitlementsCompleted)
    {
        var topLevel = this._entitlementsCompleted[topLevelId];

        if (filterFunc(topLevel))
        {
            return topLevel;
        }
    }

    return null;
};

// Create the Singleton, and expose to the world
window.contentOperationQueueController = new ContentOperationQueueController();;


/**
 * CLASS: EntitlementDialog
 *
 */
var EntitlementDialog = function()
{
};
EntitlementDialog.prototype.showDownloadDebug = function(offerID)
{
    console.log("Origin will open " + offerID + " debug window.");
};


/**
 * CLASS: EntitlementManager
 * Root object for accessing the current user's entitlement information
 */
var EntitlementManager = function()
{
    // Emitted when an entitlement has been added to the current user
	this.added = new Connector();
    this.removed = new Connector();
    this.refreshCompleted = new Connector();
    this.refreshStarted = new Connector();
    this.removedFromGlobalProgressList = new Connector();
    this.initialRefreshCompleted = true;
    this.refreshInProgress = false;
    this.dialogs = new EntitlementDialog();

    this._entitlements = [
        new Entitlement(
        {
            id: "dragonage_na",
            productId: "da",
            contentId: "dragonage_na",
            type: 'GAME',
            playing: false,
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70377/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/70377/images/en_US/banner_1200x730.jpg"],
            title: "Dragon Age(TM): Origins",
            _state: "WAITING_TO_DOWNLOAD",
            releaseStatus: 'UNRELEASED',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC","MAC"],
            multiLaunchTitles: ["Shank 1- Multiplayer  fdsk ljk jfds gfdsuhfak jdhsjal fdsuay hfudkl jsaf ufhdusahfi","Shank 1 - Single player","Shank 2- Multiplayer","Shank 2 - Single player j fds gfdsu hfak jdh sja j fds gfdsu hfak jdh sja", "Shank 3- Multiplayer hfak jdh j fds gfdsu hfak jdh sja sjal fd suayh fudkl jsaf ufhd us ahfi","Shank 3 - Single player fd skl jkj fds gfdsu hfak jdh sjal fd suayh fudkl jsaf ufhd us ahfi"],
            entitleDate: new Date(2011, 7, 11),
            displayDownloadStartDate: new Date(2012, 7, 26),
            totalSecondsPlayed: 0,
            registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
            testCode: 1102,
            downloadable: true,
            cloudSaves: new EntitlementCloudSaves({ autoSyncEnabled: true, lastBackup: new Date(2, 2, 2012), currentUsage: 54857600 }),
            debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } }
        }),
        new Entitlement(
        {
            id: "pn",
            productId: "pn",
            playing: false,
            title: "Painter's Nightmare",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true,
            //downloadOperation: new EntitlementOperation('download', this, true, true)
            expansions:
            [
                new Entitlement(
                {
                    id: "aaaa",
                    productId: "aaaa",
                    contentId: "aaaa",
                    type: 'EXPANSION',
                    title: "Painter's Nightmare  EXPANSION",
                    _state: "WAITING_TO_DOWNLOAD",
                    repairSupported: false,
                    updateSupported: false,
                    downloadable: true
                }),
                        new Entitlement(
        {
            id: "sb2",
            productId: "sb2",
            playing: false,
            title: "Star Bound 2",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb3",
            productId: "sb3",
            playing: false,
            title: "Star Bound 3",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb4",
            productId: "sb4",
            playing: false,
            title: "Star Bound 4",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        })
            ],
            addons:
            [
                new Entitlement(
                {
                    id: "143",
                    productId: "143",
                    contentId: "143",
                    type: 'ADDON',
                    title: "Painter's Nightmare  ADDON: Game Soundtrack A Very Very Very Long Title",
                    _state: "WAITING_TO_DOWNLOAD",
                    repairSupported: false,
                    updateSupported: false,
                    downloadable: true
                })
            ]
        }),
        new Entitlement(
        {
            id: "nl",
            productId: "nl",
            playing: false,
            title: "Nightlight",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb",
            productId: "sb",
            playing: false,
            title: "Star Bound",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb2",
            productId: "sb2",
            playing: false,
            title: "Star Bound 2",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb3",
            productId: "sb3",
            playing: false,
            title: "Star Bound 3",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "sb4",
            productId: "sb4",
            playing: false,
            title: "Star Bound 4",
            _state: "WAITING_TO_DOWNLOAD",
            downloadable: true,
        }),
        new Entitlement(
        {
            id: "ss",
            playing: false,
            productId: "ss",
            playable: true,
            title: "Sun Shine",
            _state: "READY_TO_PLAY",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: false,
        }),
        new Entitlement(
        {
            id: "exss",
            productId: "exss",
            playing: false,
            title: "Expansion ",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true,
            parent: new Entitlement(
                {
                    id: "exp",
                    productId: "exp",
                    contentId: "exp",
                    title: "Expansion Top level entitlement",
                    _state: "WAITING_TO_DOWNLOAD",
                    repairSupported: false,
                    updateSupported: false,
                    downloadable: true
                })
        })
    ];
};

// Returns a list of entitlement objects belonging to the current user
EntitlementManager.prototype.__defineGetter__('topLevelEntitlements', function()
{
    return this._entitlements;
});

// Gets an entitlement based on its ID. Returns null if no entitlement is found
EntitlementManager.prototype.getEntitlementById = function(id, includeUnownedContent)
{
    return this._findEntitlement(function(entitlement){ return (entitlement.id == id); });
};

EntitlementManager.prototype.getEntitlementByProductId = function(id, includeUnownedContent)
{
    return this._findEntitlement(function(entitlement){ return (entitlement.productId == id); });
};

// Gets an entitlement based on its content ID. Returns null if no entitlement is found
EntitlementManager.prototype.getEntitlementByContentId = function(contentId, includeUnownedContent)
{
    return this._findEntitlement(function(entitlement){ return (entitlement.contentId == contentId); });
};

EntitlementManager.prototype._findEntitlement = function(filterFunc)
{
    for (var topLevelId in this._entitlements)
    {
        var topLevel = this._entitlements[topLevelId];

        if (filterFunc(topLevel))
        {
            return topLevel;
        }

        var subentitlements = $.merge([], topLevel.addons);
        $.merge(subentitlements, topLevel.expansions);

        for(var subEntId in subentitlements)
        {
            var subEnt = subentitlements[subEntId];
            if (filterFunc(subEnt))
            {
                return subEnt;
            }
        }
    }

    return null;
};

// Create the Singleton, and expose to the world
window.entitlementManager = new EntitlementManager();


/**
 * CLASS: Setting
 *
 */
 var Setting = function(data)
{
    data = $.extend(
    {
        name: '', // Unique name for the setting. This value shoudl be treated as opaque and not exposed to the user.
        value : '', // Value of the setting
		defaultValue : '', // Default value of the setting
    }, data);

    this._value = data.value;
    this.name = data.name;
	this.defaultValue = data.defaultValue;
};

Setting.prototype.__defineGetter__('value', function()
{
    return this._value;
});
Setting.prototype.__defineSetter__('value', function(val)
{
    if (typeof(val) !== 'string' && typeof(val) !== 'boolean' && typeof(val) !== 'number') { console.log('Setting value must be a string, boolean or integer'); return; }
    if (val !== this._value)
    {
        this._value = val;
    }
});


/**
 * CLASS: ClientSettings
 *
 */
var ClientSettings = function()
{
    // Emitted whenever the setting's value changes
    this.updateSettings = new Connector();

    this._settings = [
        new Setting({ name : 'DownloadDebugEnabled', value: false, defaultValue: false }),
        new Setting({ name : 'EnableDownloaderSafeMode', value: false, defaultValue: false })
    ];
};

ClientSettings.prototype.writeSetting = function(_name, _value)
{
    console.log(_name + " = " + _value);
    this.getSettingByName(_name).value = _value;
};

ClientSettings.prototype.readSetting = function(name)
{
    return this.getSettingByName(name).value;
};

ClientSettings.prototype.settingIsDefault = function(name)
{
	return this.getSettingByName(name).value == this.getSettingByName(name).defaultValue;
};

ClientSettings.prototype.getSettingByName = function(_name)
{
    for (var iSetting in this._settings)
    {
        var setting = this._settings[iSetting];
        if (setting.name == _name)
        {
            return setting;
        }
    }
    return 0;
};

ClientSettings.prototype.restoreDefaultHotkey = function()
{
};

// Expose to the world
window.clientSettings = new ClientSettings();


/**
 * CLASS: OriginUser
 *
 */
var OriginUser = function()
{
    // Global setting to enable or disable the automatic synchronization of local game saves with the cloud
    this.commerceAllowed = true;
    this.socialAllowed = true;
};

// Expose to the world
window.originUser = new OriginUser();


/**
 * CLASS: OriginUser
 *
 */
var OriginPageVisibility = function()
{
    this.hidden = false;
    this.visibilityState = true;
    this.visibilityChanged = new Connector();
};

// Expose to the world
window.originPageVisibility = new OriginPageVisibility();


/**
 * CLASS: OnlineStatus
 * Object for watching the online status of the Origin client
 */
var OnlineStatus = function()
{
    // Online state of the Origin client.
    // The online status is affected both by the users' network connection
    // and user actions such as taking the client in to offline mode.
    this.onlineState = true;

    // Emitted whenever the onlineState property changes
    this.onlineStateChanged = new Connector();
};


// Create the Singleton
window.onlineStatus = new OnlineStatus();

/**
 * CLASS: DateFormatter
 * Helper for formatting localized dates and times
 */
var DateFormatter = function()
{
};

var HOUR_IN_SECONDS = 60 * 60;
var DAY_IN_SECONDS = HOUR_IN_SECONDS * 24;


DateFormatter.prototype.formatShortInterval = function(seconds)
{
    var hour = seconds / 3600;
    var minute = (seconds % 3600) / 60;
    var second = seconds % 60;
    return Math.round(hour)+':'+Math.round(minute)+':'+Math.round(second);
};

// Formats the passed interval in seconds. The result is in the form "1 day", "2 hours", "less than an hour", etc
DateFormatter.prototype.formatInterval = function(seconds)
{
    if (seconds > DAY_IN_SECONDS)
    {
        var days = seconds / DAY_IN_SECONDS;
        var daysRounded = Math.floor(days);
        return daysRounded + ' days'; // + this.formatInterval(days - daysRounded)
    }
    else if (seconds === DAY_IN_SECONDS)
    {
        return '1 day';
    }
    else if (seconds > HOUR_IN_SECONDS)
    {
        return Math.ceil(seconds / HOUR_IN_SECONDS) + ' hours';
    }
    else if (seconds === HOUR_IN_SECONDS)
    {
        return '1 hour';
    }
    else
    {
        return 'less than an hour';
    }
};

// Formats the date component of the passed Date object as a long localized string. This is typically includes the full month name eg October 12, 2012.
DateFormatter.prototype.formatLongDate = function(date)
{
    return date.format('mmmm dS, yyyy');
};

// Formats the passed Date object as a localized datetime string using the long date format.
DateFormatter.prototype.formatLongDateTime = function(date)
{
    return date.format('mmmm dS, yyyy h:MM tt');
};

// Formats the date component of the passed Date object as a short localized string. This is typically a short numerical representation eg 12/10/2012.
DateFormatter.prototype.formatShortDate = function(date)
{
    return date.format('mm/d/yyyy');
};

// Formats the passed Date object as a localized datetime string using the short date format.
DateFormatter.prototype.formatShortDateTime = function(date)
{
    return date.format('mm/d/yyyy h:MM tt');
};

// Formats the time component of the passed Date object as a localized string
DateFormatter.prototype.formatTime = function(date)
{
    if (includeSeconds)
    {
        return date.format('h:MM:ss tt');
    }
    else
    {
        return date.format('h:MM tt');
    }
};

// Formats the passed Date object as a localized datetime string using the long date format and includes the day of the week.
DateFormatter.prototype.formatLongDateTimeWithWeekday = function(date)
{
    return date.format('dddd mmmm dS, yyyy h:MM tt');
};

DateFormatter.prototype.formatBytes = function(bytes)
{
    var KB = 1024,
        MB = 1048576,
        GB = 1073741824,
        TB = 1099511627776;
    if (bytes < KB)
    {
        return ("%1 B").replace('%1', bytes.toFixed(2));
    }
    else if (bytes < MB)
    {
        return ("%1 KB").replace('%1', (bytes / KB).toFixed(2));
    }
    else if (bytes < GB)
    {
        return ("%1 MB").replace('%1', (bytes / MB).toFixed(2));
    }
    else if (bytes < TB)
    {
        return ("%1 GB").replace('%1', (bytes / GB).toFixed(2));
    }
    else
    {
        return ("%1 TB").replace('%1', (bytes / TB).toFixed(2));
    }
};

// Expose to the world
window.dateFormat = new DateFormatter();

/**
 * CLASS: TelemetryClient
 * Object for accessing Telemetry API
 */
var TelemetryClient = function()
{
};

TelemetryClient.prototype.sendGamePlaySource = function(productId, source)
{
    console.log('Send Metric_GUI_MYGAMES_PLAY_SOURCE telemetry (productId: ' + productId + ' source: ' + source + ')');
};
TelemetryClient.prototype.sendQueueOrderChanged = function(old_product_id, new_product_id, source)
{
    console.log('Send Metric_QUEUE_ORDER_CHANGED telemetry (old_product_id: ' + old_product_id + ' new_product_id: ' + new_product_id + ' source: ' + source + ')');
};
TelemetryClient.prototype.sendQueueItemRemoved = function(product_id, source)
{
    console.log('Send Metric_QUEUE_ITEM_REMOVED telemetry (product_id: ' + product_id + ' source: ' + source + ')');
};

// Create the Singleton, and expose to the world
window.telemetryClient = new TelemetryClient();

})(jQuery);
