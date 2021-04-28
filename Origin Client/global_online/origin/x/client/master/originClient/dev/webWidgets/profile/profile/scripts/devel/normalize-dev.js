;(function($){
"use strict";

if (window.entitlementManager) { return; }

var ExampleProductDatabase = {}, hushedUserIds = [], pokeInToJoinable = {}, 
simulateSubcriptionCreation = {}, pokeNicknameLoad = {}, RemoteUsers;

// Create ExampleProductDatabase
(function()
{
    var key, entry, cdnAssetRoot, basicInfo = {
        "dragonage_na": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/1007785",
            title: "Dragon Age(TM): Origins"
        },
        "darkspore_dd": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/70561",
            title: "DARKSPORE(TM)"
        },
        "71067": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71067",
            title: "Battlefield 3 Limited Edition"
        },
        "71304": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71304",
            title: "Batman: Arkham City"
        },
        "71308": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71308",
            title: "Plants vs. Zombies"
        },
        "71078": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71078",
            title: "The Sims 3 Teaser"
        },
        "71278": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71278",
            title: "The Sims 3 Pets Create a Pet Alpha Demo"
        },
        "bfbc2_le": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/70662",
            title: "Battlefield: Bad Company 2 Digital Deluxe Edition"
        },
        "moh_le_eastore_dd": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/71002",
            title: "Medal of Honor\u2122 Digital Deluxe Edition"
        },
        "lil_pet_shop_na": {
            cdnAssetRoot: "http://static.cdn.ea.com/ebisu/u/f/products/70395",
            title: "LITTLEST PET SHOP\u2122"
        },
        "nog_calculator": {
            title: "Non-Origin Game - Calculator"
        }
    };

    // Expand the product DB in to proper ProductInformation dicts
    for(key in basicInfo)
    {
        if (basicInfo.hasOwnProperty(key))
        {
            entry = basicInfo[key];
            cdnAssetRoot = entry.cdnAssetRoot;

            entry.productId = null;
            entry.contentId = null;

            entry.thumbnailUrls = [cdnAssetRoot + '/images/en_US/boxart_50x71.jpg'];
            entry.boxartUrls = [cdnAssetRoot + '/images/en_US/boxart_262x372.jpg'];
            entry.boxartUrls.push(cdnAssetRoot + '/images/en_US/boxart_172x240.jpg');
            entry.bannerUrls = [cdnAssetRoot + '/images/en_US/banner_800x487.jpg'];

            delete entry.cdnAssetRoot;

            ExampleProductDatabase[key] = entry;
        }
    }
}());

function randIntBetween(min, max)
{
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

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
        parent: null,
        platformsSupported: [],
        multiLaunchTitles: [],
        multiLaunchDefault: '',
        playable: false,
        playing: false,
        releaseStatus: 'UNRELEASED',
        registrationCode: null,
        repairOperation: null,
        repairSupported: false,
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
        _uninstallableIfInstalled: true
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
    this.parent = data.parent;
    this.currentPrice = data.currentPrice;
    this.shortDescription = data.shortDescription;
    this.longDescription = data.longDescription;
    this.thumbnailUrls = data.thumbnailUrls;
    this.platformsSupported = data.platformsSupported; // Which platforms are supported by this entitlement
    this.multiLaunchTitles = data.multiLaunchTitles; // What multi launch options there are
    this.multiLaunchDefault = data.multiLaunchDefault; // What multi launch perferred option is
    this.playable = Boolean(data.playable); // Indicates if the entitlement can be played. This means that entitlement is installed, no date-based restrictions are active, and isn't currently being played
    this.playing = Boolean(data.playing); // Indicates if the entitlement is currently being played
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
    this.testCode = data.testCode; // Publishing test code associated with the test code associated with the entitlement or null if no test code is associated. Possible values: 1102 - 1105
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

/**
 * CLASS: EntitlementManager
 * Root object for accessing the current user's entitlement information
 */
var EntitlementManager = function()
{
    // Emitted when an entitlement has been added to the current user
	this.added = new Connector();
    this.removed = new Connector();
    
    this._entitlements = [
        new Entitlement(
        {
            id: "dragonage_na",
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
            debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } }
        }),
        new Entitlement(
        {
            id: "non_origin_game",
            contentId: "non_origin_game",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/7102_262x372.jpg"], 
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/7102/images/en_US/banner_1200x730.jpg"],
            title: "Hansoft Wars",
            downloadable: false,
            playable: true,
            installed: true,
            _state: "READY_TO_PLAY",
            shortDescription: "",
            longDescription: "",
            platformsSupported: ["PC"],
            totalSecondsPlayed: 31,
            entitleDate: new Date(2012, 7, 14),
            displayExpirationDate: null,
            itemSubType: "NON_ORIGIN",
            nonOriginGame: { 'objectName' : ''},
            currentPrice: null
        
        })
    ];
};

// Returns a list of entitlement objects belonging to the current user
EntitlementManager.prototype.__defineGetter__('topLevelEntitlements', function()
{
    return this._entitlements;
});

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
    this.supportedlanguages = new Array("cs_cz", "da_dk", "de_DE", "el_gr", "en_GB", "en_US", "es_es", "es_mx", "fi_fi", "fr_fr", "hu_hu", "it_it", "ja_JP", "ko_kr", "nl_nl", "no_no", "pl_pl", "pt_br", "pt_pt", "ru_ru", "sv_se", "th_th", "zh_ch", "zh_tw");
    
    // Emitted whenever the setting's value changes
    this.updateSettings = new Connector();
    
    this._settings = [
        new Setting({ name : 'Locale', value: 'en_US', defaultValue: '' }),
        new Setting({ name : 'DefaultTab', value: 'mygames', defaultValue: 'decide' }),
        new Setting({ name : 'ShowMyOrigin', value: true, defaultValue: false }),
        new Setting({ name : 'ShowTimeStamps', value: true, defaultValue: true }),
        new Setting({ name : 'AutoPatch', value: true, defaultValue: true }),
        new Setting({ name : 'AutoUpdate', value: true, defaultValue: false }),
        new Setting({ name : 'RunOnSystemStart', value: true, defaultValue: false }),
        new Setting({ name : 'BetaOptIn', value: true, defaultValue: false }),
        new Setting({ name : 'ShowIGONux', value: true, defaultValue: true }),
        new Setting({ name : 'EnableInGameLogging', value: true, defaultValue: false }),
        new Setting({ name : 'IncomingTextChatNotification', value: 2, defaultValue: 3 }),
        new Setting({ name : 'FriendSigningInNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendSigningOutNotification', value: 1, defaultValue: 3 }),
        new Setting({ name : 'FriendStartsGameNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendQuitsGameNotification', value: 0, defaultValue: 3 }),
        new Setting({ name : 'FriendStartBroadcastNotification', value: 1, defaultValue: 3 }),
        new Setting({ name : 'FriendStopBroadcastNotification', value: 2, defaultValue: 3 }),
        new Setting({ name : 'GroupChatInviteNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'DownloadErrorNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FinishedDownloadNotification', value: 2, defaultValue: 3 }),
        new Setting({ name : 'FinishedInstallNotification', value: 2, defaultValue: 3 }),
        new Setting({ name : 'TelemetryCrashDataOptOut', value: 2, defaultValue: 2 }),
        new Setting({ name : 'TelemetryHWSurveyOptOut', value: true, defaultValue: false }),
        new Setting({ name : 'DownloadInPlaceDir', value: 'C:/OriginGames', defaultValue: 'C:/OriginGames' }),
        new Setting({ name : 'CacheDir', value: 'C:/cache/cache', defaultValue: 'C:/cache/' }),
        new Setting({ name : 'OriginFAQ', value: 'http://www.origin.com/%1/faq', defaultValue: 'http://www.origin.com/%1/faq' }),
        new Setting({ name : 'KeepInstallers', value: true, defaultValue: false }),
        new Setting({ name : 'HotKey', value: 224415, defaultValue: 224415 }),
        new Setting({ name : 'HotKeyString', value: '', defaultValue: 'Shift+F1' }),
        new Setting({ name : 'EnableIgo', value: true, defaultValue: true }),
        new Setting({ name : 'telemetry_enabled', value: true, defaultValue: true }),
        new Setting({ name : 'in_game_all_games', value: false, defaultValue: false }),
        new Setting({ name : 'cloud_saves_enabled', value: true, defaultValue: true }),
        new Setting({ name : 'SaveConversationHistory', value: true, defaultValue: true })
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
 * CLASS: InstallDirectoryManager
 * 
 */
var InstallDirectoryManager = function()
{
};

InstallDirectoryManager.prototype.chooseDownloadInPlaceLocation = function()
{
    console.log("Choose download place = chooseDownloadInPlaceLocation");
};

InstallDirectoryManager.prototype.resetDownloadInPlaceLocation = function()
{
    console.log("Reset download place location clicked = resetDownloadInPlaceLocation");
};

InstallDirectoryManager.prototype.browseInstallerCache = function()
{
    console.log("Open game installers location clicked = browseInstallerCache");
};

InstallDirectoryManager.prototype.deleteInstallers = function()
{
    console.log("Delete game installers clicked = deleteInstallerCache");
};

InstallDirectoryManager.prototype.chooseInstallerCacheLocation = function()
{
    console.log("Browse Installer cache location clicked = chooseInstallerCacheLocation");
};

InstallDirectoryManager.prototype.resetInstallerCacheLocation = function()
{
    console.log("Reset Installer cache location clicked = resetInstallerCacheLocation");
};

// Expose to the world
window.installDirectoryManager = new InstallDirectoryManager();


/**
 * CLASS: Helper
 * 
 */
var ClientNavigation = function()
{
};
ClientNavigation.prototype.launchExternalBrowser = function(url)
{
console.log("launching url: " + url);
};
window.clientNavigation = new ClientNavigation();

var OnlineStatus = function()
{
    this.onlineStateChanged = new Connector();
    this.onlineState = true;
};
window.onlineStatus = new OnlineStatus();




/**
 * CLASS: PlayingGame
 * 
 */
var PlayingGame = function(productId, joinable, broadcastUrl)
{
    if (productId.indexOf('nog_') !== 0)
    {
        this.productId = productId;
    }
    else
    {
        this.productId = null;
    }

    this.gameTitle = ExampleProductDatabase[productId].title;
    this.joinable = joinable;
    this.broadcastUrl = broadcastUrl;
};

/**
 * ENUM: Availability
 * Describes the advertised availability of a user
 */
var Availability =
{
    'ONLINE': 'ONLINE',        // User is online and available
    'CHAT': 'CHAT',        // User is looking to chat
    'AWAY' : 'AWAY',        // User is away
    'XA' : 'XA',        // User is extended away
    'DND' : 'DND',        // User is do not disturb
    'UNAVAILABLE': 'UNAVAILABLE',    // User is offline or invisible
};

/**
 * ENUM: Visibility
 * Describes the visibility of the current user
 */
var Visibility =
{
    'VISIBLE': 'VISIBLE',        // User is online and available
    'INVISIBLE': 'INVISIBLE',        // User is looking to chat
};

/**
 * CLASS: RemoteUser
 * Object for representing a remote user
 */
var RemoteUser = function(data)
{
    var availability = null, selectRandomPresence, realNameLoaded = false,
        nicknameLoaded = true, avatarLoaded = true, blocked = false;

    data = $.extend(
    {
        avatarUrl : null, // URL of the user's avatar or null if the avatar hasn't been determined yet. This may be a data: URI.
        id: '', // Unique ID for the user. This value should be treated as opaque and not exposed to the user.
        // ESCAPE USING INNER TEXT
        nickname: null, // Plain text nickname to be displayed for the user. This is typically the user's Origin ID. This value is remotely provided and must not be trusted. In particular, it may contain malicious HTML special characters
        realName: null,
        subscriptionState: {
            pendingContactApproval: false, 
            pendingCurrentUserApproval: false,
            direction: 'BOTH'
        }
    }, data);
    
    this.id = data.id;
    this.nickname = data.nickname;
    this._owns = data.owns;
    this.subscriptionState = data.subscriptionState;
    this.playingGame = null;
    
    // Emitted whenever the user's avatar changes.
    this.avatarChanged = new Connector();
    
    this.largeAvatarChanged = new Connector();
    
    // Emitted whenever the user's availability or playingGame properties change
    this.presenceChanged = new Connector();

    // Emitted whenever the user's subscriptionState property changes
    this.subscriptionStateChanged = new Connector();

    // Emitted whenever the user's name changes
    this.nameChanged = new Connector();

    this.addedToRoster = new Connector();
    this.removedFromRoster = new Connector();

    this.blockChanged = new Connector();

    if (data.delayMetadataBy)
    {
        // Simulate a nickname load
        nicknameLoaded = false;
        avatarLoaded = false;

        window.setTimeout(function()
        {
            nicknameLoaded = true;
            avatarLoaded = true;

            this.nameChanged.trigger();
            this.avatarChanged.trigger();
        }.bind(this), data.delayMetadataBy);
    }

    pokeNicknameLoad[this.id] = function()
    {
        nicknameLoaded = true;
    };

    this.__defineGetter__('availability', function()
    {
        return availability;
    });

    this.__defineGetter__('nickname', function()
    {
        if (nicknameLoaded)
        {
            return data.nickname;
        }

        return null;
    });

    this.__defineGetter__('avatarUrl', function()
    {
        if (avatarLoaded) 
        {
            return data.avatarUrl;
        }
        
        return null;
    });

    this.__defineGetter__('realName', function()
    {
        if (realNameLoaded)
        {
            return data.realName;
        }

        return null;
    });

    this.__defineGetter__('blocked', function()
    {
        return blocked;
    });

    this.block = function()
    {
        blocked = true;
        this.blockChanged.trigger(true);

        if (this.subscriptionState.pendingCurrentUserApproval)
        {
            this.rejectSubscriptionRequest();
        }

        selectRandomPresence(false);
    };

    this.unblock = function()
    {
        blocked = false;
        this.blockChanged.trigger(false);
        selectRandomPresence(false);
    };

    this.requestRealName = function()
    {
        if (realNameLoaded)
        {
            // Nothing to do
            return;
        }

        window.setTimeout(function()
        {
            realNameLoaded = true;
            this.nameChanged.trigger();
        }.bind(this), randIntBetween(0, 500));
    };

    selectRandomPresence = function(forceJoinable)
    {
        var allowedAvailability, randomIndex, randomJoinable,
            randomAvailability, shouldPlay, shouldBroadcast;
        
        if ((hushedUserIds.indexOf(this.id) !== -1) || 
            (this.subscriptionState.direction !== 'BOTH'))
        {
            // We're either hushed or not subscribed
            return;
        }

        if (blocked)
        {
            availability = Availability.UNAVAILABLE;
            this.playingGame = null;
            this.presenceChanged.trigger();
            return;
        }

        // Determine if we're playing a game
        shouldPlay = (forceJoinable || (randIntBetween(0, 8) === 0)) &&
                     this._owns.length;
                     
        // Determine if we're broadcasting this game
        shouldBroadcast = (shouldPlay && (randIntBetween(0, 2) === 0)) && this._owns.length;

        if (shouldBroadcast)
        {
            randomIndex = randIntBetween(0, this._owns.length - 1);
            randomJoinable = forceJoinable || !!randIntBetween(0, 1);

            this.playingGame = new PlayingGame(this._owns[randomIndex], randomJoinable, "http://www.twitch.tv/phantoml0rd");
            allowedAvailability = ['ONLINE'];
        }
        else if (shouldPlay)
        {
            randomIndex = randIntBetween(0, this._owns.length - 1);
            randomJoinable = forceJoinable || !!randIntBetween(0, 1);

            this.playingGame = new PlayingGame(this._owns[randomIndex], randomJoinable, "");
            allowedAvailability = ['ONLINE'];
        }
        else
        {
            this.playingGame = null;
            allowedAvailability = ['ONLINE', 'AWAY', 'UNAVAILABLE'];
        }

        // Pick a random availablity
        randomIndex = randIntBetween(0, allowedAvailability.length - 1);
        availability = allowedAvailability[randomIndex];

        // Notify that our presence has changed
        this.presenceChanged.trigger();

        window.setTimeout(selectRandomPresence, randIntBetween(0, 128 * 1000)); 

        // This is for the pokeInToJoinable wrapper below
        return shouldPlay;
    }.bind(this);

    // This lets Conversation poke us in to a joinable state to test invites
    pokeInToJoinable[data.id] = function()
    {
        return selectRandomPresence(true);
    };

    
    // Used for "accept me" command and acceptSubscriptionRequest
    simulateSubcriptionCreation[data.id] = function()
    {
        this.subscriptionState = {
            direction: 'BOTH',
            pendingContactApproval: false,
            pendingCurrentUserApproval: false
        };
        this.subscriptionStateChanged.trigger();
        
        // Switch to unavailable immediately
        availability = Availability.UNAVAILABLE;
        this.playingGame = null;

        this.presenceChanged.trigger();

        // Emulate receiving initial presence afterwards
        window.setTimeout(selectRandomPresence, randIntBetween(100, 200));
    }.bind(this);

    this.acceptSubscriptionRequest = function()
    {
        if ((this.subscriptionState.direction === 'NONE') && this.subscriptionState.pendingCurrentUserApproval)
        {
            simulateSubcriptionCreation[this.id]();
        }
        else
        {
            console.log("Tried to accept subscription for a non-request");
            return;
        }
    }
   

    this.requestSubscription = function()
    {
        window.setTimeout(function()
        {
            if ((this.subscriptionState.direction === 'NONE') &&
                !this.subscriptionState.pendingCurrentUserApproval &&
                !this.subscriptionState.pendingContactApproval)
            {
                this.subscriptionState.direction = 'NONE';
                this.subscriptionState.pendingContactApproval = true;
                this.subscriptionStateChanged.trigger();
            }
            else
            {
                console.log("Tried to request subscription for existing contact");
                return;
            }
        }.bind(this), randIntBetween(100, 300));
    }
    
    this.queryContacts = function()
    {
        return new UserContactsQueryOperation;
    };
    
    this.achievementSharingState = function()
    {
       return new AchievementShareQueryOperationProxy;
    };
    
    this.updateAchievements = function()
    {
       console.log('Updating achievements for RemoteUser: "' + this.nickname + '"');
    };
    
    this.queryOwnedProducts = function()
    {
       return new UserProductsQueryOperationProxy;
    };
    
    this.reportTosViolation = function(contentType, reason)
    {
        console.log('"' + this.nickname + '"' + "Reported for: " + reason + " on/at: " + contentType);
    };

    selectRandomPresence();
};

RemoteUser.prototype.showProfile = function(arg)
{
    console.log('Showing profile for RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.updateAchievements = function()
{
    console.log('Updating achievements for RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.requestLargeAvatar = function()
{
    console.log('Requesting large avatar for RemoteUser: "' + this.nickname + '"');
};


RemoteUser.prototype.startConversation = function()
{
    openConversation(this.id);
};

RemoteUser.prototype.inviteToGame = function()
{
    console.log('Inviting to game for RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.joinGame = function()
{
    console.log('Joining game of RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.rejectSubscriptionRequest = function()
{
    if ((this.subscriptionState.direction === 'NONE') && !this.subscriptionState.pendingContactApproval)
    {
        originSocial.roster.removeContact(this);
    }
    else
    {
        console.log("Tried to reject subscription for a non-request");
    }
}

RemoteUser.prototype.cancelSubscriptionRequest = function()
{
    if ((this.subscriptionState.direction === 'NONE') && this.subscriptionState.pendingContactApproval)
    {
        originSocial.roster.removeContact(this);
    }
    else
    {
        console.log("Tried to cancel subscription request for a non-request");
        return;
    }
}

RemoteUser.prototype.__defineSetter__('avatarUrl', function(val)
{
    if (val !== this._avatarUrl)
    {
        this._avatarUrl = val;
        this.avatarChanged.trigger();
    }
});

RemoteUser.prototype.__defineGetter__('statusText', function()
{
    if (this.playingGame)
    {
        return this.playingGame.gameTitle;
    }

    return null;
});

// Create RemoteUsers
RemoteUsers = [
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg',
        id: '2409845695',
        nickname: 'CarlSpoon',
        realName: {firstName: 'Bob', lastName: 'Loblaw'},
        owns: ['dragonage_na'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : null,
        id: '4758292481',
        nickname: 'TheQueen',
        realName: {firstName: 'Amber', lastName: 'Macx'},
        owns: ['nog_calculator'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh5.googleusercontent.com/-93eORAcEkAk/AAAAAAAAAAI/AAAAAAAAAAA/nHIkBlogF2E/s48-c-k/photo.jpg',
        id: '2258992822',
        nickname: 'eircon',
        realName: {firstName: 'Eric', lastName: 'Neugebauer'},
        owns: ['dragonage_na', '71067'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg',
        id: '2258992823',
        nickname: 'CrashOverride',
        realName: {firstName: 'Dade', lastName: 'Murphy'},
        owns: ['dragonage_na', 'darkspore_dd', '71304'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-c3Wq0wg8Q80/AAAAAAAAAAI/AAAAAAAAAAA/10h-AAHXFig/s48-c-k/photo.jpg',
        id: '2258992824',
        nickname: 'AcidBurn',
        realName: {firstName: 'Kate', lastName: 'Libby'},
        owns: ['dragonage_na', '71308'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-jlEexHhc7G4/AAAAAAAAAAI/AAAAAAAAAAA/doOn2x5RLok/s48-c-k/photo.jpg',
        id: '2258992825',
        nickname: 'ZeroCool',
        realName: null,
        owns: ['dragonage_na', 'darkspore_dd', '71067', 'moh_le_eastore_dd'],
        subscriptionState: {pendingContactApproval: false, pendingCurrentUserApproval: true, direction: 'NONE'}
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg',
        id: '2258992826',
        nickname: 'CerealKiller',
        realName: {firstName: 'Emmanuel', lastName: 'Goldstein '}, 
        owns: [],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-NHhVz7mBzP8/AAAAAAAAAAI/AAAAAAAAAAA/m_Uu94sFaLI/s48-c-k/photo.jpg',
        id: '2258992827',
        nickname: 'PaulAtreides',
        realName: {firstName: 'Muad', lastName: 'Dib'},
        owns: ['dragonage_na', 'darkspore_dd', '71304'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-l9AijjgWVZM/AAAAAAAAAAI/AAAAAAAAAAA/yTRk98j2Wo0/s48-c-k/photo.jpg',
        id: '2258992828',
        nickname: 'DukeLeto',
        realName: {firstName: 'Leto', lastName: 'Ateides'},
        owns: ['dragonage_na', '71067'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-IxGHfn0otBg/AAAAAAAAAAI/AAAAAAAAAAA/lj1f0Sl5Wbo/s48-c-k/photo.jpg',
        id: '2258992829',
        nickname: 'GurneyHalleck',
        owns: ['dragonage_na', 'darkspore_dd', '71308'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '2408965432',
        nickname: 'BleysAmber',
        owns: ['dragonage_na'],
        subscriptionState: {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false }
    })
];

/**
 * CLASS: connectedUser
 * Object for representing a connected user
 */
var connectedUser = function()
{
    var user, joinableBf3PlayingGame, playingGame, availability, visibility,
        requestedAvailability, requestedVisibility, broadcasting;

    broadcasting = randIntBetween(0, 4) === 0;
        
    if (broadcasting)
    {
        joinableBf3PlayingGame = new PlayingGame("71067", true, "http://www.twitch.tv/phantoml0rd");
    }
    else
    {
        joinableBf3PlayingGame = new PlayingGame("71067", true, null);
    }

    user = new Object();
    user.avatarUrl = 'https://lh3.googleusercontent.com/-l9AijjgWVZM/AAAAAAAAAAI/AAAAAAAAAAA/yTRk98j2Wo0/s48-c-k/photo.jpg';
    user.largeAvatarUrl = 'https://lh3.googleusercontent.com/-l9AijjgWVZM/AAAAAAAAAAI/AAAAAAAAAAA/yTRk98j2Wo0/s208-c-k/photo.jpg';
    user.id = 0;
    user.nickname = 'hiro' 
    user.realName = {firstName: 'Hiro', lastName: 'Protagonist'};

    playingGame = joinableBf3PlayingGame;
    availability = Availability.ONLINE;
    requestedAvailability = Availability.ONLINE;
    user.subscriptionState = {direction: 'BOTH', pendingContactApproval: false, pendingCurrentUserApproval: false };
    
    visibility = Visibility.VISIBLE;
    requestedVisibility = Visibility.VISIBLE;

    // Switch our playingGame every 45 seconds
    window.setInterval($.proxy(function()
    {
        if (playingGame)
        {
            // Drop out of game
            playingGame = null;

            user.allowedAvailabilityTransitionsChanged.trigger();
            user.presenceChanged.trigger();
        }
        else
        {
            // Re-enter the game
            playingGame = joinableBf3PlayingGame;

            if (availability === 'AWAY')
            {
                availability = 'ONLINE';
            }

            user.allowedAvailabilityTransitionsChanged.trigger();
            user.presenceChanged.trigger();
        }
    }, this), 45000);
    
    // Emitted whenever the user's avatar changes.
    user.avatarChanged = new Connector();
    
    // Emitted whenever the user's large avatar changes.
    user.largeAvatarChanged = new Connector();
    
    // Emitted whenever the user's availability or playingGame properties change
    user.presenceChanged = new Connector();

    // Emitted whenever the user's visibility changes
    user.visibilityChanged = new Connector();
    
    // Emitted whenever the user's name changes
    user.nameChanged = new Connector();

    user.allowedAvailabilityTransitionsChanged = new Connector();

    user.__defineGetter__('availability', function()
    {
        return availability;
    });

    user.__defineGetter__('requestedAvailability', function()
    {
        return requestedAvailability;
    });

    user.__defineSetter__('requestedAvailability', function(newAvailability)
    {
        if (this.allowedAvailabilityTransitions.indexOf(newAvailability) === -1)
        {
            console.log('Attempted an invalid availability transition');
            return;
        }

        requestedAvailability = newAvailability;

        // Change the actual availability after a delay
        window.setTimeout($.proxy(function()
        {
            availability = newAvailability;
            this.presenceChanged.trigger();
        }, this), randIntBetween(0, 1000));
    });
    
    user.__defineGetter__('visibility', function()
    {
        return visibility;
    });

    user.__defineGetter__('requestedVisibility', function()
    {
        return requestedVisibility;
    });

    user.__defineSetter__('requestedVisibility', function(newVisibility)
    {
        if (!Visibility.hasOwnProperty(newVisibility))
        {
            console.log('Attempted to set an unknown visibility');
            return;
        }

        requestedVisibility = newVisibility;

        // Change the actual visibility after a delay
        window.setTimeout($.proxy(function()
        {
            visibility = newVisibility;
            this.visibilityChanged.trigger();
        }, this), randIntBetween(0, 1000));
    });

    user.__defineGetter__('allowedAvailabilityTransitions', function()
    {
        if (playingGame !== null)
        {
            return ['ONLINE'];
        }
            
        return ['ONLINE', 'AWAY'];
    });

    user.__defineGetter__('playingGame', function()
    {
        return playingGame;
    });

    user.__defineGetter__('statusText', function()
    {
        if (playingGame)
        {
            return playingGame.gameTitle;
        }

        return null;
    });

    user.showProfile = function(arg)
    {
        console.log('Showing connected user\'s profile');
    };
    
    user.queryContacts = function()
    {
        return new UserContactsQueryOperation;
    };
    
    user.achievementSharingState = function()
    {
       return new AchievementShareQueryOperationProxy;
    };
    
    user.updateAchievements = function()
    {
       console.log('Updating achievements for RemoteUser: "' + this.nickname + '"');
    };
    
    user.queryOwnedProducts = function()
    {
       return new UserProductsQueryOperationProxy;
    };

    return user;
}();


/**
 * CLASS: UserContactsQueryOperation
 */
var UserContactsQueryOperation = function()
{
    this.succeeded = new Connector();
    this.queryError = new Connector();
    this.failed = new Connector();
};
UserContactsQueryOperation.prototype.onUserFriendsSuccess = function()
{
    console.log('onUserFriendsSuccess');
};
UserContactsQueryOperation.prototype.sendCurrentUserRoster = function()
{
    console.log('sendCurrentUserRoster');
};

/**
 * CLASS: AchievementShareQueryOperationProxy
 */
var AchievementShareQueryOperationProxy = function()
{
    this.succeeded = new Connector();
    this.queryError = new Connector();
    this.failed = new Connector();
};
AchievementShareQueryOperationProxy.prototype.onError = function()
{
    console.log('onError');
};

/**
 * CLASS: UserProductsQueryOperationProxy
 */
var UserProductsQueryOperationProxy = function()
{
    this.succeeded = new Connector();
    this.queryError = new Connector();
    this.failed = new Connector();
};
UserProductsQueryOperationProxy.prototype.onError = function()
{
    console.log('onError');
};



/**
 * CLASS: OriginSocial
 * Root object of Origin social support
 */
var OriginSocial = function()
{
    this.currentUser = connectedUser;
    //this.connection = new Connection;
    //this.roster = new RosterView();
    //this.roster.hasLoaded = true;
    //this.roster.loaded = new Connector();
};

// Creates a dynamic roster view of all contacts owning a specific game
//    productId: Game product ID to filter on
OriginSocial.prototype.createOwningRosterView = function(productId)
{
    var contacts = this.roster.contacts.filter(function(contact){
        return (contact._owns.indexOf(productId) !== -1); 
    });

    var rosterView = new RosterView();
    rosterView.contacts = contacts;
    return rosterView;
};

// Creates a dynamic roster view of all contacts playing a game
//    productId: Game product ID to filter on or null for all contacts currently playing a game
OriginSocial.prototype.createPlayingRosterView = function(productId)
{
    var contacts = this.roster.contacts.filter(function(contact)
    { 
        return (contact.playingGame && contact.playingGame.productId === productId); 
    });

    var rosterView = new RosterView();
    rosterView.contacts = contacts;
    return rosterView;
};

// Returns the user with the passed ID or null if no such contact exists
OriginSocial.prototype.getUserById = function(userId)
{
    if (userId === this.currentUser.id)
    {
        return this.currentUser;
    }

    var users = RemoteUsers.filter(function(user)
    {
        return (user.id === userId);
    });

    return (users.length > 0) ? users[0] : null;
};

// Returns the roster contact with the passed ID or null if no such contact exists
//    id: contact id
OriginSocial.prototype.getRosterContactById = function(contactId)
{
    var contacts = this.roster.contacts.filter(function(contact)
    { 
        return (contact.id === contactId); 
    });

    return (contacts.length > 0)? contacts[0] : null;
};

OriginSocial.prototype.startMultiUserConversation = function(users)
{
    var userIds = users.map(function(user)
    {
        return user.id;
    });

    openConversation("muc|" + userIds.join(";"));
}

window.originSocial = new OriginSocial();

/**
 * CLASS: Helper
 * 
 */
var Helper = function()
{
};
Helper.prototype.context = function()
{
    // 0 = ClientScope, 3 = IGOScope
    return 0;
};
// Expose to the world
window.helper = new Helper();

})(jQuery);
