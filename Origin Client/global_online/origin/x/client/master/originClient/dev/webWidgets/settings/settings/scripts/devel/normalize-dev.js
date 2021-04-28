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
 * CLASS: OriginVoice
 *
 */
 var OriginVoice = function()
 {
    this.deviceAdded = new Connector();
    this.deviceRemoved = new Connector();
    this.audioInputDevices = [];
    this.audioOutputDevices = [];
    this.voiceLevel = new Connector();
    this.overThreshold = new Connector();
    this.underThreshold = new Connector();
 };

 OriginVoice.prototype.changeInputDevice = function(val)
 {
 };

 OriginVoice.prototype.changeOutputDevice = function(val)
 {
 };

 OriginVoice.prototype.startVoiceChannel = function()
 {
 };

 OriginVoice.prototype.stopVoiceChannel = function()
 {
 };

 OriginVoice.prototype.testMicrophoneStart = function()
 {
 };

 OriginVoice.prototype.testMicrophoneStop = function()
 {
 };

 // Expose to the world
window.originVoice = new OriginVoice();


/**
 * CLASS: ClientSettings
 *
 */
var ClientSettings = function()
{
    this.supportedlanguages = new Array("cs_cz", "da_dk", "de_DE", "el_gr", "en_GB", "en_US", "es_es", "es_mx", "fi_fi", "fr_fr", "hu_hu", "it_it", "ja_JP", "ko_kr", "nl_nl", "no_no", "pl_pl", "pt_br", "pt_pt", "ru_ru", "sv_se", "th_th", "zh_ch", "zh_tw");
    this._areServerUserSettingsLoaded = true;
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
        new Setting({ name : 'IncomingChatRoomMessageNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'IncomingTextChatNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendSigningInNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendSigningOutNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendStartsGameNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendQuitsGameNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendStartBroadcastNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendStartBroadcastNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FriendStopBroadcastNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'GameInviteNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'GroupChatInviteNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'DownloadErrorNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FinishedDownloadNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'FinishedInstallNotification', value: 3, defaultValue: 3 }),
        new Setting({ name : 'TelemetryCrashDataOptOut', value: 2, defaultValue: 2 }),
        new Setting({ name : 'TelemetryHWSurveyOptOut', value: true, defaultValue: false }),
        new Setting({ name : 'DownloadInPlaceDir', value: 'C:/OriginGames', defaultValue: 'C:/OriginGames' }),
        new Setting({ name : 'CacheDir', value: 'C:/cache/cache', defaultValue: 'C:/cache/' }),
        new Setting({ name : 'OriginFaqURL', value: 'http://www.origin.com/%1/faq', defaultValue: 'http://www.origin.com/%1/faq' }),
        new Setting({ name : 'KeepInstallers', value: true, defaultValue: false }),
        new Setting({ name : 'HotKey', value: 224415, defaultValue: 224415 }),
        new Setting({ name : 'HotKeyString', value: '', defaultValue: 'Shift+F1' }),
        new Setting({ name : 'EnableIgo', value: true, defaultValue: true }),
        new Setting({ name : 'telemetry_enabled', value: true, defaultValue: true }),
        new Setting({ name : 'in_game_all_games', value: false, defaultValue: false }),
        new Setting({ name : 'cloud_saves_enabled', value: true, defaultValue: true }),
        new Setting({ name : 'SaveConversationHistory', value: true, defaultValue: true }),
        new Setting({ name : 'DisablePromoManager', value: true, defaultValue: false }),
        new Setting({ name : 'IgnoreNonMoandatoryGameUpdates', value: true, defaultValue: false }),
        new Setting({ name : 'ShutDownOriginOnGameFinished', value: true, defaultValue: false }),
        new Setting({ name : 'EnablePushToTalk', value: false, defaultValue: false }),
        new Setting({ name : 'EnableVoiceChatIndicator', value: 2, defaultValue: 2 }),
        new Setting({ name : 'DeclineIncomingVoiceChatRequests', value: false, defaultValue: false }),
        new Setting({ name : 'NoWarnAboutVoiceChatConflict', value: false, defaultValue: false }),
        new Setting({ name : 'EnableDownloaderSafeMode', value: true, defaultValue: false }),
        new Setting({ name : 'EnablePushToTalk', value: false, defaultValue: false })
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
ClientSettings.prototype.__defineGetter__('areServerUserSettingsLoaded', function()
{
    return this._areServerUserSettingsLoaded;
});

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


/**
 * CLASS: OnlineStatus
 * Object for watching the online status of the Origin client
 */
var OnlineStatus = function()
{
    // Online state of the Origin client.
    // The online status is affected both by the users' network connection
    // and user actions such as taking the client in to offline mode.
    this._onlineState = true;

    // Emitted whenever the onlineState property changes
    this.onlineStateChanged = new Connector();
};
OnlineStatus.prototype.__defineGetter__('onlineState', function()
{
    return this._onlineState;
});
OnlineStatus.prototype.__defineSetter__('onlineState', function(online)
{
    if (typeof(online) !== 'boolean') { console.log('OnlineStatus value must be a boolean'); return; }
    if (online !== this._onlineState)
    {
        this._onlineState = online;
        this.onlineStateChanged.trigger(online);
    }
});

// Create the Singleton
window.onlineStatus = new OnlineStatus();

/**
 * CLASS: Helper
 *
 */
var Helper = function()
{
};
Helper.prototype.context = function()
{
    // 0 = normal, 1 = OIG
    return 0;
};

Helper.prototype.hotkeyInputHasFocus = function(val)
{
    // 0 = normal, 1 = OIG
    return 0;
};
Helper.prototype.pinHotkeyInputHasFocus = function(val)
{
    // 0 = normal, 1 = OIG
    return 0;
};

Helper.prototype.pushToTalkHotKeyInputHasFocus = function(val)
{
}

// Expose to the world
window.helper = new Helper();


})(jQuery);
