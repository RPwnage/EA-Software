;(function($){
"use strict";

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

/*
 * CLASS: ClientNavigator
 * Provides high-level methods in-client navigation
 */
var ClientNavigator = function()
{
};

// Shows the passed URL in the user's default browser. This URL must be globally accessible; widget:// URLs will not be reachable to an external browser
ClientNavigator.prototype.launchExternalBrowser = function(url)
{
    console.log("launchExternalBrowser: " + url);
    window.open(url, 'external-browser');
};

ClientNavigator.prototype.showIGOBrowser = function()
{
    console.log("showIGOBrowser");
};

ClientNavigator.prototype.showFriends = function()
{
    console.log("showFriends");
};

ClientNavigator.prototype.showOriginHelp = function()
{
    console.log("showOriginHelp");
};

ClientNavigator.prototype.showDownloadProgressDialog = function()
{
    console.log("showDownloadProgressDialog");
};

ClientNavigator.prototype.showGeneralSettingsPage = function()
{
    console.log("showGeneralSettingsPage");
};

ClientNavigator.prototype.closeIGO = function()
{
    console.log("closeIGO");
};

ClientNavigator.prototype.showBroadcast = function()
{
    console.log("showBroadcast");
};

// Create the Singleton, and expose to the world
window.clientNavigation = new ClientNavigator();

/*
 * CLASS: ClientNavigator
 * Provides high-level methods in-client navigation
 */
var Broadcast = function()
{
    this.isUserConnected = true;
    this.isBroadcasting = false;
    this.isBroadcastingPending = false;
    this.isBroadcastTokenValid = false;
    this.broadcastDuration = 3;
    this.broadcastViewers = 3;
    this.broadcastUserName = "Trixies";
    this.isBroadcastOptEncoderAvailable = false;
    this.currentBroadcastedProductId = "OFB:4543543";
    this.broadcastStarted = new Connector();
    this.broadcastStopped = new Connector();
    this.broadcastStartStopError = new Connector();
    this.broadcastUserNameChanged = new Connector();
    this.broadcastGameNameChanged = new Connector();
    this.broadcastDurationChanged = new Connector();
    this.broadcastChannelChanged = new Connector();
    this.broadcastStreamInfoChanged = new Connector();
    this.broadcastPermitted = new Connector();
    this.broadcastErrorOccurred = new Connector();
    this.broadcastStatusChanged = new Connector();
    this.broadcastingStateChanged = new Connector();
    this.broadcastStartPending = new Connector();
    this.userDisconnected = new Connector();
    this.broadcastOptEncoderAvailable = new Connector();
};

Broadcast.prototype.disconnectAccount = function()
{
    isUserConnected = false;
    console.log('attemptBroadcastStart');
    this.broadcastStarted.trigger();
};

Broadcast.prototype.attemptBroadcastStart = function()
{
    console.log('attemptBroadcastStart');
    this.isBroadcasting = true;
    this.broadcastStarted.trigger();
};

Broadcast.prototype.attemptBroadcastStop = function()
{
    console.log('attemptBroadcastStop');
    this.isBroadcasting = false;
    this.broadcastStopped.trigger();
};

Broadcast.prototype.isBroadcastingBlocked = function(id)
{
    console.log('isBroadcastingBlocked id: ' + id);
};

Broadcast.prototype.showLogin = function()
{
    console.log('Show broadcast login');
};

// Create the Singleton, and expose to the world
window.broadcast = new Broadcast();


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
    this._areServerUserSettingsLoaded = true;
    // Emitted whenever the setting's value changes
    this.updateSettings = new Connector();

    this._settings = [
        new Setting({ name : 'BroadcastMicVolume', value: 100, defaultValue: 50 }),
        new Setting({ name : 'GameMicVolume', value: 50, defaultValue: 50 }),
        new Setting({ name : 'BroadcastMicMute', value: true, defaultValue: false }),
        new Setting({ name : 'GameMicMute', value: true, defaultValue: false }),
        new Setting({ name : 'BroadcastSaveVideo', value: false, defaultValue: true }),
        new Setting({ name : 'video_broadcast_token', value: '', defaultValue: '' }),
        new Setting({ name : 'BroadcastGameName', value: '', defaultValue: '' }),
        new Setting({ name : 'BroadcastResolution', value: 2, defaultValue: 2 }),
        new Setting({ name : 'BroadcastFramerate', value: 4, defaultValue: 4 }),
        new Setting({ name : 'BroadcastAutoStart', value: false, defaultValue: false }),
        new Setting({ name : 'BroadcastUseOptimizedEncoder', value: true, defaultValue: true }),
        new Setting({ name : 'BroadcastShowViewersNum', value: true, defaultValue: true }),
        new Setting({ name : 'BroadcastQuality', value: 0, defaultValue: 0 }),
        new Setting({ name : 'BroadcastSaveVideoFix', value: 'BroadcastSaveVideoFix', defaultValue: '' }),
        new Setting({ name : 'BroadcastChannel', value: 'www.twitch.com/trixies1', defaultValue: '' }),
        new Setting({ name : 'EnvironmentName', value: 'production', defaultValue: '' })
    ];
};

ClientSettings.prototype.writeSetting = function(_name, _value)
{
    console.log(_name + " = " + _value);
    this.getSettingByName(_name).value = _value;
    this.updateSettings.trigger(_name, _value);
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

})(jQuery);