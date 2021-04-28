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
 * CLASS: DateFormatter
 * Helper for formatting localized dates and times
 */
var DateFormatter = function()
{
};

var HOUR_IN_SECONDS = 60 * 60;
var DAY_IN_SECONDS = HOUR_IN_SECONDS * 24;


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
 * CLASS: Setting
 *
 */
 var Setting = function(data)
{
    data = $.extend(
    {
        name: '', // Unique name for the setting. This value shoudl be treated as opaque and not exposed to the user.
        value : '', // Value of the setting
    }, data);

    this._value = data.value;
    this.name = data.name;
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
        new Setting({ name : 'cloud_saves_enabled', value: true }),
        new Setting({ name : 'OnTheHouseVersionsShown', value: "" }),
        new Setting({ name : 'OverrideOnTheHouseQueryIntervalMS', value: 21600000 })
    ];
};

ClientSettings.prototype.writeSetting = function(_name, _value)
{
    console.log(_name + " = " + _value);
    this.getSettingByName(_name).value = _value;
};
ClientSettings.prototype.appendStringSetting = function(_name, _value)
{
    console.log(_name + " = " + _value);
    this.getSettingByName(_name).value = _value;
};

ClientSettings.prototype.readSetting = function(name)
{
    return this.getSettingByName(name).value;
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

ClientSettings.prototype.__defineGetter__('areServerUserSettingsLoaded', function()
{
    return this._areServerUserSettingsLoaded;
});

// Expose to the world
window.clientSettings = new ClientSettings();


/**
 * CLASS: OnlineStatus
 * Object for watching the online status of the Origin client
 */
var OnlineStatus = function()
{
    // Online state of the Origin client.
    // The online status is affected both by the users' network connection
    // and user actions such as taking the client in to offline mode.
    this._onlineState = false;

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
OnlineStatus.prototype.requestOnlineMode = function()
{
    console.log("requesting to go online");
};

// Create the Singleton
window.onlineStatus = new OnlineStatus();



/**
 * CLASS: OnlineStatus
 * Object for watching the online status of the Origin client
 */
var SubscriptionManager = function()
{
    // Is the subscription enabled
    this._enabled = true;
    
    // The user has a subscription
    this._isActive = true;
    
    // The users subscription has expired
    this._isExpired = true;
    
    // The users subscription has expired
    this._status = "DISABLED";
    
    // The subscription expiration date YYYY/MM/DD
    this._expirationDate = new Date(2014, 11, 17);
    
    // The amount of time in seconds remaining till subscription membership expires.
    this._timeRemaining = 10000;
    
    // The amount of time in seconds remaining for offline play.
    this._offlinePlayRemaining = 10000;
    
    // The amount of time in seconds remaining for offline play.
    this._firstSignupDate = "fddfd";

    // This signal is fired when the subscription is updated.
    this.updated = new Connector();
    
    // Upgrade Completed
    this.entitlementUpgraded = new Connector();
    // Remove Completed
    this.entitlementRemoved = new Connector();
};
SubscriptionManager.prototype.__defineGetter__('enabled', function()
{
    return this._enabled;
});
SubscriptionManager.prototype.__defineGetter__('isActive', function()
{
    return this._isActive;
});
SubscriptionManager.prototype.__defineGetter__('status', function()
{
    return this._status;
});
SubscriptionManager.prototype.__defineGetter__('isExpired', function()
{
    return this._isExpired;
});
SubscriptionManager.prototype.__defineGetter__('expirationDate', function()
{
    return this._expirationDate;
});
SubscriptionManager.prototype.__defineGetter__('timeRemaining', function()
{
    return this._timeRemaining;
});
SubscriptionManager.prototype.__defineGetter__('offlinePlayRemaining', function()
{
    return this._offlinePlayRemaining;
});
SubscriptionManager.prototype.__defineGetter__('firstSignupDate', function()
{
    return this._firstSignupDate;
});

// Check whether the current entitlement has an upgrade.
SubscriptionManager.prototype.hasUpgrade = function(_masterTitleId)
{
    console.log("checking for upgrade for " + _masterTitleId);
    return true;
};

// Create the Singleton
window.subscriptionManager = new SubscriptionManager();


/**
 * CLASS: MenuAction
 * Represents a native popup menu action
 */
var MenuAction = function(text, idx, menu)
{
    this._idx = idx || 0;
    this._menu = menu;

    // Text of the action
    this.text = text || '';

    // Enable state of the action. Disabled actions are greyed out and unselectable
    this._enabled = true;
    this._visible = true;

    // Fired whenever the action has been selected from the menu
    this.triggered = new Connector();
};
MenuAction.prototype.__defineGetter__('visible', function()
{
    return this._visible;
});
MenuAction.prototype.__defineSetter__('visible', function(val)
{
    if (this._visible != val)
    {
        this._visible = val;

        // Redraw the menu
        this._menu._draw();
    }
});
MenuAction.prototype.__defineGetter__('enabled', function()
{
    return this._enabled;
});
MenuAction.prototype.__defineSetter__('enabled', function(val)
{
    if (this._enabled != val)
    {
        this._enabled = Boolean(val);

        // Redraw the menu
        this._menu._draw();
    }
});
MenuAction.prototype.__defineGetter__('_html', function()
{
    var html = '<li style="';
    if (!this.visible)
    {
        html += 'display:none;';
    }
    html += 'margin:0;border-radius:3px;padding:5px 10px;font-size:11px;';
    html += (this.enabled)? 'cursor:pointer;color:#000;' : 'cursor:default;color:#CCC;';
    html += '"';
    if (!this.enabled)
    {
        html += ' class="disabled"';
    }
    html += ' data-idx="' + this._idx + '">' + this.text + '</li>';
    return html;
});

/**
 * CLASS: SeparatorAction
 * Represents a native menu separator
 */
var SeparatorAction = function(menu, idx)
{
    this._menu = menu;
    this._visible = true;

    // Stub out the rest of our API
    // I don't believe these are hooked up in Qt either
    this.enabled = true;
    this.triggered = new Connector();
};
SeparatorAction.prototype.__defineGetter__('visible', function()
{
    return this._visible;
});
SeparatorAction.prototype.__defineSetter__('visible', function(val)
{
    if (this._visible != val)
    {
        this._visible = val;

        // Redraw the menu
        this._menu._draw();
    }
});
SeparatorAction.prototype.__defineGetter__('_html', function()
{
    if (!this.visible)
    {
        return '';
    }

    return '<li style="height:1px;margin:5px 0;border-bottom:1px solid #CCC;padding:0;background-color:#EEE;"></li>';
});

/**
 * CLASS: Menu
 * Represents a native popup menu
 */
var Menu = function()
{
    this._id = 'menu-' + (++Menu._count);
    Menu._instances[this._id] = this;

    this._actions = [];
    this._html = '';

    this.aboutToHide = new Connector();
    this.aboutToShow = new Connector();
};

// Adds a new action to the bottom of the menu
Menu.prototype.addAction = function(toAdd)
{
    if (typeof toAdd === "string")
    {
        // Create a new menu action
        var action = new MenuAction(toAdd, this._actions.length, this);
        this._actions.push(action);
        return action;
    }
    else
    {
        // Reuse an existing menu action
        toAdd._idx = this._actions.length;
        toAdd._menu = this;
        this._actions.push(toAdd);
    }
};

Menu.prototype.removeAction = function(action)
{
    var actionIndex = this._actions.indexOf(action);

    if (actionIndex !== -1)
    {
        this._actions.splice(actionIndex, 1);

        // Re-index our actions
        $.each(this._actions, function(idx, action)
        {
            action._idx = idx;
        });
    }
    else
    {
        console.log("Attempted to remove a nonexistent action from a menu");
    }
};

// Adds a separator to the bottom of the menu
Menu.prototype.addSeparator = function()
{
    var action = new SeparatorAction(this);
    this._actions.push(action);
    return action;
};

// Pops the menu up at the current cursor position
Menu.prototype.popup = function()
{
    this.aboutToShow.trigger();

    var $menu = this._getElm(false, false).attr('data-id', this._id);
    this._html = '';
    $menu.find('> ul').empty();
    this._draw($menu);
    $menu
        .hide()
        .css({ 'top': ((nativeMenu.mouseY - 20) + 'px'), 'left': ((nativeMenu.mouseX - 20) + 'px') })
        .fadeIn('fast');
};

//
Menu.prototype.clear = function()
{
    if (this._actions.length === 0) { return; }
    this._actions = [];
};

Menu.prototype._getElm = function(filterVisible, filterActive)
{
    var selector = '#context-menu';
    if (filterActive)
    {
        selector += '[data-id="' + this._id +'"]';
    }
    if (filterVisible)
    {
        selector += ':visible';
    }
    return $(selector);
};

Menu.prototype._draw = function($menu)
{
    var $menu = $menu || this._getElm(true, true);
    if ($menu.length === 0) { return; }

    var html = '';
    $.each(this._actions, function(idx, action)
    {
        html += action._html;
    });

    if (html && html !== this._html)
    {
        this._html = html;
        $menu.find('> ul').html(this._html);
    }
};

Menu._get = function(id)
{
    return Menu._instances[id];
};

Menu._count = 0;

Menu._instances = {};

Menu._init = function()
{
    var $body = $(document.body);

    $('<div id="context-menu" data-id="" style="position:absolute;display:none;z-index:99999;min-width:100px;border-radius:3px;padding:3px;background-color:#FFF;box-shadow:0 0 5px rgba(0,0,0,0.4);"><ul style="list-style-type:none;margin:0;padding:0;"></ul></div>')
        .appendTo($body);

    $(document).on('click', function(evt)
    {
        Menu._close();
    });

    $body
        .on('click', '#context-menu > ul > li', function(evt)
        {
            evt.stopImmediatePropagation();

            var $this = $(this);
            var idx = $this.data('idx');
            if ($this.hasClass('disabled') || isNaN(idx)) { return; }

            var menuId = $this.parent().parent().attr('data-id');
            var menu = Menu._get(menuId);
            var action = menu._actions[idx];

            Menu._close();

            action.triggered.trigger();
        })
        .on('hover', '#context-menu > ul > li', function(evt)
        {
            var $this = $(this);
            if (isNaN($this.data('idx')) || $this.hasClass('disabled')) { return; }
            switch (evt.type)
            {
                case ('mouseenter'):
                {
                    $this.css({ 'color': '#666', 'background-color': '#EEE' });
                    break;
                }
                case ('mouseleave'):
                {
                    $this.css({ 'color': '#000', 'background-color': 'transparent' });
                    break;
                }
            }
        });
};

Menu._close = function()
{
    var $menu = $('#context-menu:visible');
    if ($menu.length === 0) { return; }
    var menu = Menu._get($menu.attr('data-id'));
    if (!menu) { return; }

    menu.aboutToHide.trigger();
    $menu.fadeOut('fast');
};

Menu.prototype.hide = Menu._close;

Menu._init();


/**
 * CLASS: MenuFactory
 * Creates new native popup menus
 */
window.nativeMenu =
{
    // Creates a new native popup menu
    createMenu: function()
    {
        return new Menu();
    },
    mouseX: 0,
    mouseY: 0
};

$(document).on('mousemove', function(evt)
{
    nativeMenu.mouseX = evt.pageX;
    nativeMenu.mouseY = evt.pageY;
});


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
    this._progressState = 'State-Active';
    this._playableAt = .25;
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
EntitlementOperation.prototype.__defineGetter__('playableAt', function()
{
    return this._playableAt;
});
EntitlementOperation.prototype.__defineGetter__('phaseDisplay', function()
{
    return this._type+"-"+this._phase;
});
EntitlementOperation.prototype.specificPhaseDisplay = function(phase)
{
    return this._type+"-"+phase;
};
EntitlementOperation.prototype.shouldLightFlag = function(flagName)
{
    return this._entitlement.installed && (this._entitlement.playable || this._entitlement.playing);
};
EntitlementOperation.prototype.__defineGetter__('phase', function()
{
    return this._phase;
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
    this._progressState = 'State-Paused';
    this._entitlement.changed.trigger();

    // Simulate a delayed pause
    this._pauseTimer = window.setTimeout(this._delayedPause.bind(this), 2000);
};

EntitlementOperation.prototype._delayedPause = function()
{
    this._paused = true;

    var entitlement = this._entitlement;

    this._phase = 'PAUSED';
    this._progressState = 'State-Paused';
    entitlement.changed.trigger();
    contentOperationQueueController.headBusy.trigger(false);
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
    window.setTimeout(this._delayedCancel.delegate(this), 2000);
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
    contentOperationQueueController.remove(entitlement.productId);
};


/**
 * CLASS: EntitlementDebugInfo
 *
 */
var EntitlementDebugInfo = function()
{
    // Key-value pairs of detailed debug information. This does not include the keys already contained in summary; these objects should be combined when presented to the user.
    this.details = {};

    // Key-value pairs of summary debug information. This only contains the most basic information and will typically have 2 or 3 entries
    this.summary = {};
};


/**
 * CLASS: Boxart
 * Object describing a game's boxart
 */
var Boxart = function(data)
{
    data = $.extend(
    {
        cropCenterX: 0,
        cropCenterY: 0,
        cropWidth: 0,
        cropHeight: 0,
        cropLeft: 0,
        cropTop: 0,
        croppedBoxart: false,
        customizedBoxart: null,
        boxartUrls: []
    }, data);

    this.cropCenterX = data.cropCenterX;
    this.cropCenterY = data.cropCenterY;
    this.cropWidth = data.cropWidth;
    this.cropHeight = data.cropHeight;
    this.cropLeft = data.cropLeft;
    this.cropTop = data.cropTop;
    this.croppedBoxart = Boolean(data.croppedBoxart);
    this.customizedBoxart = data.customizedBoxart;
    this.boxartUrls = data.boxartUrls;
};

Boxart.prototype.imageScale = function(tileWidth)
{
    return 1.0;
};

Boxart.prototype.imageTop = function(tileHeight)
{
    return 0;
};

Boxart.prototype.imageLeft = function(tileWidth)
{
    return 0;
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
        masterTitleId: '',
        debugInfo: null,
        downloadOperation: null,
        displayLocation: 'GAMES',
        displayDownloadStartDate: null,
        displayExpirationDate: null,
        displayUnlockStartDate: null,
        downloadable: false,
        entitleDate: null,
        expansions: [],
        extraContentDisplayGroupInfo: null,
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
        isEntitledFromSubscription: false,
        isAvailableFromSubscription: false,
        hasEntitlementUpgraded: false,
        playable: false,
        playableBitSet: true,
        playing: false,
        purchasable: false,
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
        upgradeTypeAvailable: false,
        isSuppressed: false,
        subscriptionRetiring: null,
        subscriptionRetiringTime: 0,
        updateOperation: null,
        updateSupported: false,
        unownedContentAvailable: false,
        newUnownedContentAvailable: false,
        hasPDLCStore: false,
        timeRemainingTilTerminationInSecs: -1,
        debugActions: [],
        _legacyPackage: false,
        _uninstallableIfInstalled: true,
        hasUninstaller: false,
        preAnnouncementDisplayString: '',
        previewContent: false
    }, data);

    this.addons = data.addons; // Array of addons for this game that the user is entitled to
    this._autoUpdateEnabled = Boolean(data.autoUpdateEnabled); // Indicates if the entitlement will automatically check for and apply updates before the entitlement is played
    this.boxartUrls = data.boxartUrls; // URL for the entitlement's box art
    this.masterTitleId = data.masterTitleId;
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
    this.extraContentDisplayGroupInfo = data.extraContentDisplayGroupInfo; // Dictionary containing info on how the extra content should be displayed.  If null, the extracontent will be displayed in the standard "Addons" and "Expansions" tables.
    this.id = data.id; // Content ID of the entitlement. This an internal unique identifier for the content. This roughly corresponds to a game but various editions and regional versions of a game may have different content IDs. A user may only have one entitlement for a given content ID so it can also be used as a stable identifier for the user's entitlement.
    this.installed = data.installed;
    this.installOperation = data.installOperation; // The current install operation if one is in progress, null otherwise
    this.itemSubType = data.itemSubType; // Entitlement's game type
    /*
         "ALPHA"                    Game is the alpha demo version
         "TIMED_TRIAL_ACCESS"      Game use an timed trial version with access
         "TIMED_TRIAL_GAMETIME"      Game use an timed trial version with game time
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
    this.owned = Boolean(data.owned); // Indicates if the entitlement owned by the current user.
    this.isEntitledFromSubscription = Boolean(data.isEntitledFromSubscription);
    this.isAvailableFromSubscription = Boolean(data.isAvailableFromSubscription);
    this.hasEntitlementUpgraded = Boolean(data.hasEntitlementUpgraded);
    this.playable = Boolean(data.playable); // Indicates if the entitlement can be played. This means that entitlement is installed, no date-based restrictions are active, and isn't currently being played
    this.playableBitSet = Boolean(data.playableBitSet); // Indicates if the game can be played the user's machine bitset.
    this.playing = Boolean(data.playing); // Indicates if the entitlement is currently being played
    this.preAnnouncementDisplayString = data.preAnnouncementDisplayString;
    this.purchasable = Boolean(data.purchasable); // Indicates if this content is purchasable by the current user.  Note that consumables may be both owned and purchasable.
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
    this.testCode = data.testCode; // Publishing test code associated with the test code associated with the entitlement or null if no test code is associated. Possible values: 1102 − 1103
    this.dynamicContentSupportEnabled= data.dynamicContentSupportEnabled;
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
    this.upgradeTypeAvailable = data.upgradeTypeAvailable;
    this.isSuppressed = data.isSuppressed;
    this.subscriptionRetiring = data.subscriptionRetiring;
    this.subscriptionRetiringTime = data.subscriptionRetiringTime;
    this.updateOperation = data.updateOperation; // The current update operation if one is in progress, null otherwise
    this.updateSupported = Boolean(data.updateSupported); // Indicates if this entitlement supports update operations. Legacy titles typically do not support repair unless they have been recently repackaged.
    this.unownedContentAvailable = Boolean(data.unownedContentAvailable); // Indicates if this entitlement has unowned content available
    this.newUnownedContentAvailable = Boolean(data.newUnownedContentAvailable); // Indicates if there is new unowned content for this title (1 week from release date)
    this.timeRemainingTilTerminationInSecs = data.timeRemainingTilTerminationInSecs;
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

    // Indicates if the expansion/addon is downloadable without an entitlement
    this.previewContent = Boolean(data.previewContent);

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
        this.downloadOperation.progress += 0.01;
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
        this.installOperation.progress += 0.01;
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

Entitlement.prototype.__defineGetter__('hasUninstaller', function()
{
    return this.hasUninstaller;
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

Entitlement.prototype.fetchUnownedContentPricing = function()
{
};

/** ******************
 * Purchasing
 */

// Launches the entitlement
Entitlement.prototype.purchase = function()
{
    console.log("Opening PDLC store for offer " + this.id);
};
    
Entitlement.prototype.upgrade = function()
{
    console.log("upgrading offer " + this.id);
};
    
Entitlement.prototype.entitle = function()
{
    console.log("Requesting a subscription entitlement for " + this.id);
};

/**
 * CLASS: EntitlementDialog
 *
 */
var EntitlementDialog = function()
{
};
EntitlementDialog.prototype.showProperties = function(offerID)
{
    console.log("Origin will open " + offerID + " properties window.");
};

EntitlementDialog.prototype.showRemoveEntitlementPrompt = function(offerID)
{
    console.log("Origin will open " + offerID + " remove entitlement prompt.");
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
    this.initialRefreshCompleted = true;
    this.refreshInProgress = false;
    this.dialogs = new EntitlementDialog();

    this._entitlements = [
        new Entitlement(
        {
            id: "dragonage_na",
            masterTitleId: "55906",
            contentId: "dragonage_na",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70377/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/70377/images/en_US/banner_1200x730.jpg"],
            manualUrl: 'http://news.ycombinator.com/',
            title: "Dragon Age(TM): Origins",
            _state: "WAITING_TO_DOWNLOAD",
            releaseStatus: 'UNRELEASED',
            isEntitledFromSubscription: true,
            isAvailableFromSubscription: true,
            hasEntitlementUpgraded: true,
            upgradeTypeAvailable: "BASEGAME_ONLY",
            isSuppressed: false,
            subscriptionRetiring: new Date(2014, 4, 26),
            subscriptionRetiringTime: 691200,
            repairSupported: false,
            updateSupported: false,
            playableBitSet: false,
            platformsSupported: ["PC","MAC"],
            multiLaunchTitles: ["Shank 1- Multiplayer  fdsk ljk jfds gfdsuhfak jdhsjal fdsuay hfudkl jsaf ufhdusahfi","Shank 1 - Single player","Shank 2- Multiplayer","Shank 2 - Single player j fds gfdsu hfak jdh sja j fds gfdsu hfak jdh sja", "Shank 3- Multiplayer hfak jdh j fds gfdsu hfak jdh sja sjal fd suayh fudkl jsaf ufhd us ahfi","Shank 3 - Single player fd skl jkj fds gfdsu hfak jdh sjal fd suayh fudkl jsaf ufhd us ahfi"],
            entitleDate: new Date(2011, 7, 11),
            displayDownloadStartDate: new Date(2014, 4, 26),
            displayUnlockStartDate: new Date(2014, 7, 26),
            preAnnouncementDisplayString: 'totally man',
            totalSecondsPlayed: 0,
            registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
            testCode: 1102,
            debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } },
            boxart: { 'objectName' : ''}
        }),
        new Entitlement(
        {
            id: "darkspore_dd",
            contentId: "darkspore_dd",
            type: 'GAME',
            isEntitledFromSubscription: true,
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70561_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70561/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/70561/images/en_US/banner_1200x730.jpg"],
            title: "DARKSPORE(TM)",
            downloadable: true,
            multiLaunchTitles: ["Shank The Game For Realzies", "Just Another Launcher", "Launcher Number Three"],
            playable: false,
            _state: "READY_TO_DOWNLOAD",
            releaseStatus: 'AVAILABLE',
            installed: false,
            repairSupported: false,
            dynamicContentSupportEnabled: true,
            platformsSupported: ["PC"],
            updateSupported: false,
            entitleDate: new Date(2012, 2, 1),
            displayDownloadStartDate: new Date(2012, 1, 1),
            totalSecondsPlayed: 0,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: '',
            expansions:
            [
                new Entitlement(
                {
                    id: "dragonage_na_expand",
                    contentId: "dragonage_na_expand",
                    type: 'EXPANSION',
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
                    title: "Dragon Age(TM): Origins",
                    _state: "WAITING_TO_DOWNLOAD",
                    releaseStatus: 'UNRELEASED',
                    repairSupported: false,
                    dynamicContentSupportEnabled: true,
                    updateSupported: false,
                    currentPrice: 30,
                    shortDescription: "Discover the groundbreaking RPG, winner of more than 50 awards including more than 30 'Game of the Year' awards!",
                    longDescription: "Discover the groundbreaking RPG, winner of more than 50 awards including more than 30 'Game of the Year' awards! This package contains most terrain specific textures in TGA format which Tiberium Crystal War utilizes. For use in 3DSMax Studio 8.",
                    entitleDate: null,
                    platformsSupported: ["PC"],
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2015, 10, 10),
                    latestStartPurchaseDate: new Date(2012, 10, 7),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    testCode: 1102,
                    debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } },
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                }),
                new Entitlement(
                {
                    id: "darkspore_dd_expand",
                    contentId: "darkspore_dd_expand",
                    type: 'EXPANSION',
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70561_262x372.jpg"],
                    title: "DARKSPORE(TM)",
                    downloadable: true,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "Combat from Jerusalem to Beirut with a new perspective, using this exclusive Add-on for MEC2.",
                    longDescription: "Combat from Jerusalem to Beirut with a new perspective, using this exclusive Add-on for MEC2.",
                    releaseStatus: 'AVAILABLE',
                    platformsSupported: ["PC"],
                    repairSupported: false,
                    updateSupported: false,
                    latestStartPurchaseDate: new Date(2012, 10, 7),
                    displayUnlockStartDate: new Date(2012,10,11),
                    totalSecondsPlayed: 0,
                    installed: false,
                    dynamicContentSupportEnabled: true,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    previewContent: true,
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                })
            ],
            addons:
            [
                new Entitlement(
                {
                    id: "43",
                    contentId: "43",
                    type: 'ADDON',
                    title: "Game Soundtrack A Very Very Very Long Title",
                    _state: "WAITING_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'UNRELEASED',
                    dynamicContentSupportEnabled: true,
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2011, 7, 11),
                    platformsSupported: ["PC"],
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2012, 5, 26),
                    latestStartPurchaseDate: new Date(2012, 10, 10),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } },
                    testCode: 1102,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: 'coming soon to a place near you',
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                }),
                new Entitlement(
                {
                    id: "45",
                    contentId: "45",
                    dynamicContentSupportEnabled: true,
                    type: 'ADDON',
                    title: "Micro Content",
                    downloadable: false,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "The BlackHawk helicopter from the Wreckage mod. Including the high/mid and low poly models. Several rotors and a prefab for an easy usage of the helicopter.",
                    longDescription: "The BlackHawk helicopter from the Wreckage mod. Including the high/mid and low poly models. Several rotors and a prefab for an easy usage of the helicopter. You could use it in your creations by giving the author credits. Created by Jaco_E.",
                    releaseStatus: 'AVAILABLE',
                    repairSupported: false,
                    thumbnailUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/1008371/images/en_US/boxart_50x71.jpg"],
                    platformsSupported: ["PC"],
                    updateSupported: false,
                    entitleDate: new Date(2011, 4, 11),
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2012, 5, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                }),
                new Entitlement(
                {
                    id: "61",
                    contentId: "61",
                    type: 'ADDON',
                    title: "Unlockable Content",
                    downloadable: false,
                    installed: null,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'AVAILABLE',
                    dynamicContentSupportEnabled: true,
                    platformsSupported: ["PC"],
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2011, 4, 10),
                    displayDownloadStartDate: new Date(2012, 3, 12),
                    displayUnlockStartDate: new Date(2012, 3, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                }),
                new Entitlement(
                {
                    id: "62",
                    contentId: "62",
                    type: 'ADDON',
                    title: "Unlockable Content 2",
                    downloadable: false,
                    installed: null,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'AVAILABLE',
                    dynamicContentSupportEnabled: true,
                    platformsSupported: ["PC"],
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2011, 4, 10),
                    displayDownloadStartDate: new Date(2012, 3, 12),
                    displayUnlockStartDate: new Date(2012, 3, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZE',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                })
            ]
        }),
        new Entitlement(
        {
            id: "111",
            contentId: "111",
            type: 'GAME',
            boxartUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70619/images/en_US/boxart_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71067/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71067/images/en_US/banner_1200x730.jpg"],
            title: "timed trial",
            playable: true,
            _state: "READY_TO_PLAY",
            releaseStatus: 'AVAILABLE',
            installed: true,
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            entitleDate: new Date(2012, 0, 1),
            displayDownloadStartDate: new Date(2012, 1, 1),
            totalSecondsPlayed: 2342340,
            lastPlayedDate: new Date(2012, 2, 25),
            cloudSaves: new EntitlementCloudSaves({ autoSyncEnabled: true, lastBackup: new Date(2, 2, 2012), currentUsage: 54857600 }),
            debugInfo: { summary: { 'Offer ID': '70619', 'Content ID': 'DR:225064100' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Battlefield 3\\Install Dir]\\bf3.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Battlefield 3\\Install Dir]\\bf3.exe' } },
            hasPDLCStore: true,
            newUnownedContentAvailable: true,
            unownedContentAvailable: true,
            timeRemainingTilTerminationInSecs: 0,
            boxart: { 'objectName' : ''},
            itemSubType: "TIMED_TRIAL_ACCESS",
            preAnnouncementDisplayString: ''
        }),
        new Entitlement(
        {
            id: "71067",
            contentId: "71067",
            type: 'GAME',
            boxartUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70619/images/en_US/boxart_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71067/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71067/images/en_US/banner_1200x730.jpg"],
            title: "Battlefield 3 Limited Edition",
            playable: true,
            _state: "READY_TO_PLAY",
            releaseStatus: 'AVAILABLE',
            installed: true,
            repairSupported: false,
            updateSupported: true,
            platformsSupported: ["PC"],
            availableUpdateVersion: "0",
            entitleDate: new Date(2012, 0, 1),
            displayDownloadStartDate: new Date(2012, 1, 1),
            totalSecondsPlayed: 2342340,
            lastPlayedDate: new Date(2012, 2, 25),
            cloudSaves: new EntitlementCloudSaves({ autoSyncEnabled: true, lastBackup: new Date(2, 2, 2012), currentUsage: 54857600 }),
            debugInfo: { summary: { 'Offer ID': '70619', 'Content ID': 'DR:225064100' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Battlefield 3\\Install Dir]\\bf3.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Battlefield 3\\Install Dir]\\bf3.exe' } },
            debugActions: [new MenuAction("Clear Remote Save Area")],
            hasPDLCStore: true,
            newUnownedContentAvailable: true,
            unownedContentAvailable: true,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: '',
            isEntitledFromSubscription: true,
            isAvailableFromSubscription: true,
            upgradeTypeAvailable: "DLC_ONLY",
            subscriptionRetiring: new Date(2014, 4, 26),
            subscriptionRetiringTime: 20000,
            expansions:
            [
                new Entitlement(
                {
                    id: "dragonage_na_expand",
                    contentId: "dragonage_na_expand",
                    type: 'EXPANSION',
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
                    title: "Dragon Age(TM): Origins",
                    _state: "WAITING_TO_DOWNLOAD",
                    releaseStatus: 'UNRELEASED',
                    repairSupported: false,
                    updateSupported: false,
                    currentPrice: 30,
                    shortDescription: "Discover the groundbreaking RPG, winner of more than 50 awards including more than 30 'Game of the Year' awards!",
                    longDescription: "Discover the groundbreaking RPG, winner of more than 50 awards including more than 30 'Game of the Year' awards! This package contains most terrain specific textures in TGA format which Tiberium Crystal War utilizes. For use in 3DSMax Studio 8.",
                    entitleDate: null,
                    platformsSupported: ["PC"],
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2015, 10, 10),
                    latestStartPurchaseDate: new Date(2012, 10, 7),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    testCode: 1102,
                    debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } },
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    isEntitledFromSubscription: false,
                    isAvailableFromSubscription: true,
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                }),
                new Entitlement(
                {
                    id: "darkspore_dd_expand",
                    contentId: "darkspore_dd_expand",
                    type: 'EXPANSION',
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70561_262x372.jpg"],
                    title: "DARKSPORE(TM)",
                    downloadable: true,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "Combat from Jerusalem to Beirut with a new perspective, using this exclusive Add-on for MEC2.",
                    longDescription: "Combat from Jerusalem to Beirut with a new perspective, using this exclusive Add-on for MEC2.",
                    releaseStatus: 'AVAILABLE',
                    platformsSupported: ["PC"],
                    repairSupported: false,
                    updateSupported: false,
                    latestStartPurchaseDate: new Date(2012, 10, 7),
                    displayUnlockStartDate: new Date(2012,10,11),
                    totalSecondsPlayed: 0,
                    installed: false,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                }),
                new Entitlement(
                {
                    id: "OFB-EAST:55171",
                    contentId: "71554",
                    type: 'EXPANSION',
                    boxartUrls: ["https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/50182/231.0x326.0/71554_LB_231x326_en_US_%5E_2013-05-28-15-04-29_32369250b56b4e8ddb0196031885bb6a68c8c5c3e1c75989ee37aca48c9b16cd.jpg"],
                    title: "Battlefield 3™: Aftermath",
                    downloadable: true,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "Survive amid devastation.",
                    longDescription: "Survive amid devastation.",
                    releaseStatus: 'AVAILABLE',
                    platformsSupported: ["PC"],
                    repairSupported: true,
                    updateSupported: true,
                    owned: false,
                    purchasable: true,
                    latestStartPurchaseDate: new Date(2012, 10, 7),
                    displayUnlockStartDate: new Date(2012,10,11),
                    totalSecondsPlayed: 1653870,
                    installed: false,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    previewContent: true,
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                })
            ],
            addons:
            [
                new Entitlement(
                {
                    id: "43",
                    contentId: "43",
                    type: 'ADDON',
                    title: "Game Soundtrack A Very Very Very Long Title",
                    _state: "WAITING_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'UNRELEASED',
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2011, 7, 11),
                    platformsSupported: ["PC"],
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2012, 5, 26),
                    latestStartPurchaseDate: new Date(2012, 10, 10),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    debugInfo: { summary: { "Test code": "1102", 'Offer ID': '31435', 'Content ID': 'dragonage_na' }, details: { 'Install Check Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe', 'Launch Path Key': '[HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Games\\Dragon Age\\Install Dir]\\da.exe' } },
                    testCode: 1102,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: 'coming soon to a place near you',
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_2", "displayName": "CUSTOM GROUP 2", "sortOrderAscending": 200 }
                }),
                new Entitlement(
                {
                    id: "45",
                    contentId: "45",
                    type: 'ADDON',
                    title: "Micro Content",
                    downloadable: true,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "The BlackHawk helicopter from the Wreckage mod. Including the high/mid and low poly models. Several rotors and a prefab for an easy usage of the helicopter.",
                    longDescription: "The BlackHawk helicopter from the Wreckage mod. Including the high/mid and low poly models. Several rotors and a prefab for an easy usage of the helicopter. You could use it in your creations by giving the author credits. Created by Jaco_E.",
                    releaseStatus: 'AVAILABLE',
                    repairSupported: false,
                    thumbnailUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/1008371/images/en_US/boxart_50x71.jpg"],
                    platformsSupported: ["PC", "MAC"],
                    updateSupported: false,
                    entitleDate: new Date(2011, 4, 11),
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2012, 5, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    isEntitledFromSubscription: false,
                    isAvailableFromSubscription: true,
                    subscriptionRetiringTime: 0,
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                }),
                new Entitlement(
                {
                    id: "61",
                    contentId: "61",
                    type: 'ADDON',
                    title: "Unlockable Content",
                    downloadable: false,
                    installed: null,
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'AVAILABLE',
                    platformsSupported: ["PC"],
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2011, 4, 10),
                    displayDownloadStartDate: new Date(2012, 3, 12),
                    displayUnlockStartDate: new Date(2012, 3, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_1", "displayName": "CUSTOM GROUP 1", "sortOrderAscending": 100 }
                })
            ]
        }),
        new Entitlement(
        {
            id: "71304",
            contentId: "71304",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/71304_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71304/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71304/images/en_US/banner_1200x730.jpg"],
            title: "Batman: Arkham City",
            isEntitledFromSubscription: true,
            downloadable: true,
            _state: "READY_TO_DOWNLOAD",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'PRELOAD',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            entitleDate: new Date(2012, 2, 11),
            totalSecondsPlayed: 0,
            displayDownloadStartDate: new Date(2012, 1, 1),
            displayUnlockStartDate: new Date(2012, 8, 1),
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: 'zero'
        }),
        new Entitlement(
        {
            id: "sb",
            productId: "test3",
            playing: false,
            title: "Star Bound",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true
        }),
        new Entitlement(
        {
            id: "pn",
            playing: false,
            title: "Painter's Nightmare",
            _state: "WAITING_TO_DOWNLOAD",
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC","MAC"],
            downloadable: true
        }),
        new Entitlement(
        {
            id: "71308",
            contentId: "71308",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/71308_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71308/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71308/images/en_US/banner_1200x730.jpg"],
            title: "Plants vs. Zombies",
            downloadable: true,
            _state: "READY_TO_DOWNLOAD",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'AVAILABLE',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            entitleDate: new Date(2011, 4, 15),
            totalSecondsPlayed: 0,
            displayDownloadStartDate: new Date(2012, 1, 1),
            // Test for un-uninstallable entitlements
            _uninstallableIfInstalled: false,
            boxart: { 'objectName' : ''}
        }),
        // Unreleased
        new Entitlement(
        {
            id: "71078",
            contentId: "71078",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/pr/oduct/box/71078_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71078/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71078/images/en_US/banner_1200x730.jpg"],
            title: "The Sims 3 Teaser",
            _state: "WAITING_TO_DOWNLOAD",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'UNRELEASED',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: [],
            entitleDate: new Date(2015, 7, 25),
            totalSecondsPlayed: 0,
            boxart: { 'objectName' : ''},
            displayDownloadStartDate: new Date(2014, 9, 1),
            preAnnouncementDisplayString: 'Coming next summer'
        }),
        new Entitlement(
        {
            id: "71278",
            contentId: "71278",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/71278_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71278/images/en_US/banner_800x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71278/images/en_US/banner_1200x730.jpg"],
            title: "The Sims 3 Pets Create a Pet Alpha Demo",
            isEntitledFromSubscription: true,
            isAvailableFromSubscription: true,
            upgradeTypeAvailable: "DLC_ONLY",
            subscriptionRetiring: new Date(2014, 4, 26),
            subscriptionRetiringTime: 2000000,
            playable: true,
            _state: "READY_TO_PLAY",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'EXPIRED',
            installed: true,
            repairSupported: true,
            updateSupported: true,
            platformsSupported: ["PC"],
            entitleDate: new Date(2012, 1, 14),
            totalSecondsPlayed: 423324,
            itemSubType: "ALPHA",
            displayExpirationDate: new Date(2012, 7, 28),
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: ''
        }),
        new Entitlement(
        {
            id: "bfbc2_le",
            contentId: "bfbc2_le",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70662_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70662/images/en_US/banner_80x487.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/70662/images/en_US/banner_120x730.jpg"],
            title: "Battlefield: Bad Company 2 Digital Deluxe Edition",
            playable: true,
            _state: "READY_TO_PLAY",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'AVAILABLE',
            installed: true,
            repairSupported: false,
            updateSupported: true,
            platformsSupported: ["PC"],
            availableUpdateVersion: "45a",
            totalSecondsPlayed: 0,
            hasPDLC: false,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: '',
            addons:
            [
                new Entitlement(
                {
                    id: "56",
                    contentId: "56",
                    type: 'ADDON',
                    title: "Unreleased Game Soundtrack With A Very Very Very Long Title",
                    _state: "WAITING_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'UNRELEASED',
                    repairSupported: false,
                    updateSupported: false,
                    entitleDate: new Date(2014, 7, 11),
                    platformsSupported: ["PC"],
                    displayDownloadStartDate: new Date(2013, 9, 26),
                    displayUnlockStartDate: new Date(2013, 9, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: 'Coming soon...'
                }),
                new Entitlement(
                {
                    id: "57",
                    contentId: "57",
                    type: 'ADDON',
                    title: "Preloadable Addon",
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'PRELOAD',
                    downloadable: true,
                    repairSupported: false,
                    updateSupported: false,
                    platformsSupported: ["PC"],
                    entitleDate: new Date(2011, 7, 11),
                    displayDownloadStartDate: new Date(2012, 5, 26),
                    displayUnlockStartDate: new Date(2012, 6, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                }),
                new Entitlement(
                {
                    id: "58",
                    contentId: "58",
                    type: 'ADDON',
                    title: "Expired Addon",
                    _state: "READY_TO_PLAY",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'EXPIRED',
                    installed: true,
                    repairSupported: false,
                    updateSupported: false,
                    platformsSupported: ["PC"],
                    entitleDate: new Date(2011, 7, 11),
                    displayDownloadStartDate: new Date(2011, 5, 26),
                    displayUnlockStartDate: new Date(2011, 5, 26),
                    displayExpirationDate: new Date(2012, 5, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                }),
                new Entitlement(
                {
                    id: "59",
                    contentId: "59",
                    type: 'ADDON',
                    title: "Non-DiP Addon",
                    _state: "READY_TO_DOWNLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'RELEASED',
                    installed: false,
                    downloadable: true,
                    repairSupported: false,
                    updateSupported: false,
                    platformsSupported: ["PC"],
                    entitleDate: new Date(2011, 7, 11),
                    displayDownloadStartDate: new Date(2011, 5, 26),
                    displayUnlockStartDate: new Date(2011, 5, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    _legacyPackage: true,
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: ''
                })
            ],
            entitleDate: new Date(2011, 6, 13),
            displayExpirationDate: new Date(2012, 5, 28)
        }),
        new Entitlement(
        {
            id: "moh_le_eastore_dd",
            contentId: "moh_le_eastore_dd",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/71002_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/71002/images/en_US/banner_80x80.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/71002/images/en_US/banner_90x90.jpg"],
            title: "Medal of Honor\u2122 Digital Deluxe Edition",
            downloadable: true,
            playable: false,
            _state: "READY_TO_DOWNLOAD",
            shortDescription: "",
            dynamicContentSupportEnabled: true,
            longDescription: "",
            releaseStatus: 'AVAILABLE',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            availableUpdateVersion: "4592",
            totalSecondsPlayed: 0,
            entitleDate: new Date(2011, 6, 13),
            displayExpirationDate: null,
            hasPDLCStore: true,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: '',
            addons:
            [
                new Entitlement(
                {
                    id: "60",
                    contentId: "60",
                    type: 'ADDON',
                    title: "Medal Of Honor: Breakthrough",
                    _state: "READY_TO_DWONLOAD",
                    shortDescription: "",
                    longDescription: "",
                    releaseStatus: 'RELEASED',
                    repairSupported: false,
                    updateSupported: false,
                    downloadable: false,
                    platformsSupported: ["PC"],
                    entitleDate: new Date(2011, 7, 11),
                    displayDownloadStartDate: new Date(2011, 9, 26),
                    displayUnlockStartDate: new Date(2011, 9, 26),
                    totalSecondsPlayed: 0,
                    registrationCode: '000000-2VRTJK-H65H4N-3605KB-5XRMQR-KCFU4Z-7AH0HX-GU78ZF',
                    boxart: { 'objectName' : ''},
                    preAnnouncementDisplayString: '',
                    extraContentDisplayGroupInfo: { "groupKey": "cust_group_3", "displayName": "CUSTOM GROUP 3", "sortOrderAscending": 300 }
                }),
            ],
            _legacyPackage: true
        }),
        new Entitlement(
        {
            id: "lil_pet_shop_na",
            contentId: "lil_pet_shop_na",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70395_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70395/images/en_US/banner_1200x730.jpg"],
            title: "LITTLEST PET SHOP\u2122",
            // Third party titles can be playable but not installed (!)
            downloadable: false,
            playable: true,
            installed: false,
            _state: "READY_TO_DOWNLOAD",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'AVAILABLE',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            totalSecondsPlayed: 0,
            entitleDate: new Date(2013, 3, 13),
            displayExpirationDate: null,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: ''
        }),
        new Entitlement(
        {
            id: "mac_game",
            contentId: "mac_game",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70195_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70195/images/en_US/banner_1200x730.jpg"],
            title: "MAC ONLY GAME",
            downloadable: false,
            playable: false,
            installed: false,
            _state: "READY_TO_DOWNLOAD",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'UNAVAILABLE',
            repairSupported: false,
            updateSupported: false,
            playableBitSet: false,
            platformsSupported: ["MAC"],
            totalSecondsPlayed: 0,
            entitleDate: new Date(2012, 6, 13),
            displayExpirationDate: null,
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: ''
        }),
        new Entitlement(
        {
            id: "alpha_game",
            contentId: "alpha_game",
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/7102_262x372.jpg"],
            bannerUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/7102/images/en_US/banner_100x730.jpg", "http://static.cdn.ea.com/ebisu/u/f/products/7102/images/en_US/banner_1100x730.jpg"],
            title: "Spore Alpha",
            downloadable: true,
            playable: true,
            installed: true,
            _state: "READY_TO_PLAY",
            shortDescription: "",
            longDescription: "",
            releaseStatus: 'AVAILABLE',
            repairSupported: false,
            updateSupported: false,
            platformsSupported: ["PC"],
            totalSecondsPlayed: 0,
            entitleDate: new Date(2012, 7, 14),
            displayExpirationDate: null,
            itemSubType: "ALPHA",
            boxart: { 'objectName' : ''},
            preAnnouncementDisplayString: ''
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
            boxart: { 'objectName' : ''},
            currentPrice: null,
            preAnnouncementDisplayString: ''

        }),
        new Entitlement(
        {
            id: "OFB-EAST:109549069",
            contentId: "1010966",
            type: 'GAME',
            boxartUrls: ["https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/23�6_en_US_^_2013-08-27-10-13-24_25ae1337ad7f06923dac28f07284d2d2f7de23e3.png", "https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/14�0_en_US_^_2013-08-27-10-13-19_a27ee757a6051e8673b68c5af75c4b968d704e3e.png"],
            bannerUrls: [],
            title: "Battlefield 4(TM) Exclusive Beta (x64)",
            downloadable: false,
            playable: true,
            installed: true,
            _state: "READY_TO_PLAY",
            shortDescription: "The Beta features all-out war in the multiplayer map Siege of Shanghai. Please note that the Beta is still undergoing testing before its official release and does not reflect the quality of the final product.",
            longDescription: "The Beta features all-out war in the multiplayer map Siege of Shanghai.&nbsp;<br><br>Battlefield 4 is the genre-defining action blockbuster made of moments that blur the line between game and glory. Fueled by the next-generation power and fidelity of Frostbite� 3, Battlefield 4 provides a visceral, dramatic experience unlike any other.<br><br>Only in Battlefield will you blow the foundations of a dam or reduce an entire skyscraper to rubble. Only in Battlefield will you lead a naval assault from the back of a gun boat. Battlefield grants you the freedom to do more and be more while playing to your strengths and carving your own path to victory.&nbsp;<br><br>Please note that the Beta is still undergoing testing before its official release and does not reflect the quality of the final product.",
            platformsSupported: ["PC"],
            totalSecondsPlayed: 31,
            entitleDate: new Date(2013, 7, 14),
            displayExpirationDate: null,
            itemSubType: "BETA",
            nonOriginGame: { 'objectName' : ''},
            boxart: { 'objectName' : ''},
            currentPrice: null,
            preAnnouncementDisplayString: 'Coming Soon!'

        }),
        
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
    return this._findEntitlement(function(entitlement){ return (entitlement.productId == id); });
};

EntitlementManager.prototype.getEntitlementByProductId =
    EntitlementManager.prototype.getEntitlementById;

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

// Refreshes the entitlement list from the copy stored on the server
EntitlementManager.prototype.refresh = function()
{
    window.setTimeout(this._delayedRefresh.delegate(this), 2000);
    this.refreshInProgress = true;
    this.refreshStarted.trigger();
};

EntitlementManager.prototype._delayedRefresh = function()
{
    this.refreshInProgress = false;
    this.refreshCompleted.trigger();

    // Add a new entitlement
    if (this._entitlements.length === 10)
    {
        var entitlement = new Entitlement(
        {
            id: "dragonage2_dd",
            title: "Dragon Age (TM) II",
            _state: "READY_TO_DOWNLOAD",
            repairSupported: false,
            updateSupported: false,
            type: 'GAME',
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70784_262x372.jpg"],
            downloadable: true,
            playable: false,
            releaseStatus: 'AVAILABLE',
            installed: false,
            totalSecondsPlayed: 0,
            entitleDate: new Date()
        });

        this._entitlements.push(entitlement);
        this.added.trigger(entitlement);
    }
};

// Create the Singleton, and expose to the world
window.entitlementManager = new EntitlementManager();

//window.setInterval(function(){ window.entitlementManager.simulateChange(); }, 1000);


/**
 * CLASS: TelemetryClient
 * Object for accessing Telemetry API
 */
var TelemetryClient = function()
{
};

// Sends METRIC_FREETRIAL_PURCHASE telemetry event with passed content ID, expired flag, and source
TelemetryClient.prototype.sendFreeTrialPurchase = function(contentId, expired, source)
{
    console.log('Send METRIC_FREETRIAL_PURCHASE telemetry (contentId: ' + contentId + ' expired: ' + expired + ' source: ' + source + ')');
};

TelemetryClient.prototype.sendContentSalesPurchase = function(id, productType, context)
{
    console.log('Send Metric_CONTENTSALES_PURCHASE(id: ' + id + ' productType: ' + productType + ' context: ' + context + ')' );
};

TelemetryClient.prototype.sendContentSalesViewInStore = function(id, productType, context)
{
    console.log('Send Metric_CONTENTSALES_VIEW_IN_STORE(id: ' + id + ' productType: ' + productType + ' context: ' + context + ')' );
};

TelemetryClient.prototype.sendAchievementWidgetPageView = function(pageName, productId, inIgo)
{
    console.log('Send Metric_ACHIEVEMENT_WIDGET_PAGE_VIEW(pageName: ' + pageName + ' productId: ' + productId + ' inIgo: ' + inIgo + ')' );
};

TelemetryClient.prototype.sendAchievementWidgetPurchaseClick = function(productId)
{
    console.log('Send Metric_ACHIEVEMENT_WIDGET_PURCHASE_CLICK(productId: ' + productId + ')' );
};

TelemetryClient.prototype.sendMyGamesCloudStorageTabView = function (productId)
{
    console.log('Send METRIC_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW(productId: ' + productId + ')');
};

TelemetryClient.prototype.sendMyGamesManualLinkClick = function (productId)
{
    console.log('Send METRIC_GUI_MYGAMES_MANUAL_LINK_CLICK(productId: ' + productId + ')');
};

TelemetryClient.prototype.sendBroadcastJoined = function (source)
{
    console.log('Send METRIC_BROADCAST_JOINED(source:' + source + ')');
};

TelemetryClient.prototype.sendAchievementSharing = function (myVal, source)
{
    if (myVal == "show")
        console.log('Send METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW(source:' + source + ')');
    else
        console.log('Send METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED(source:' + source + ')');

};

TelemetryClient.prototype.sendGamePlaySource = function (productId, source)
{
    console.log('Send METRIC_GUI_MYGAMES_PLAY_SOURCE(contentId: ' + productId + ' source: ' + source + ')');
};

TelemetryClient.prototype.sendProfileGameSource = function (source)
{
    console.log('Send METRIC_USER_PROFILE_VIEWGAMESOURCE(source: ' + source + ')');
};

TelemetryClient.prototype.sendAddonDescriptionExpanded = function (source)
{
    console.log('Send METRIC_GAMEPROPERTIES_ADDON_DESCR_EXPANDED(source: ' + source + ')');
};

TelemetryClient.prototype.sendHovercardBuyNowClick = function (gameId)
{
    console.log('Send METRIC_HOEVERCARD_BUYNOW_CLICK (gameid: ' + gameId + ')');
};

TelemetryClient.prototype.sendHovercardBuyClick = function (gameId)
{
    console.log('Send METRIC_HOEVERCARD_BUY_CLICK (gameid: ' + gameId + ')');
};
TelemetryClient.prototype.sendHovercardDownloadClick = function (gameId)
{
    console.log('Send METRIC_HOEVERCARD_DOWNLOAD_CLICK (gameid: ' + gameId + ')');
};
TelemetryClient.prototype.sendHovercardDetailsClick = function (gameId)
{
    console.log('Send METRIC_HOEVERCARD_DETAILS_CLICK (gameid: ' + gameId + ')');
};
TelemetryClient.prototype.sendHovercardAchievementsClick = function (gameId)
{
    console.log('Send METRIC_HOEVERCARD_ACHIEVEMENTS_CLICK (gameid: ' + gameId + ')');
};
TelemetryClient.prototype.sendQueueOrderChanged = function(old_product_id, new_product_id, source)
{
    console.log('Send Metric_QUEUE_ORDER_CHANGED telemetry (old_product_id: ' + old_product_id + ' new_product_id: ' + new_product_id + ' source: ' + source + ')');
};
TelemetryClient.prototype.sendQueueItemRemoved = function(product_id, source)
{
    console.log('Send Metric_QUEUE_ITEM_REMOVED telemetry (product_id: ' + product_id + ' source: ' + source + ')');
};
TelemetryClient.prototype.sendSubscriptionUpsell = function(context)
{
    console.log('Send Metric_SUBSCRIPTION_UPSELL telemetry (context: ' + context + ')');
};
// Create the Singleton, and expose to the world
window.telemetryClient = new TelemetryClient();

/**
 * CLASS: ClientNavigator
 * Provides high-level methods in-client navigation
 */
var ClientNavigator = function()
{
};

var GAME_DETAILS_BASE_URL ='./game-details.html?id=';

// Shows the passed URL in the user's default browser. This URL must be globally accessible; widget:// URLs will not be reachable to an external browser
ClientNavigator.prototype.launchExternalBrowser = function(url)
{
    window.open(url, 'external-browser');
};

// Show a window containing detailed progress information for all in-progress downloads
ClientNavigator.prototype.showDownloadProgressDialog = function()
{
    console.log('Showing Download Progress Dialog');
};

// Shows in-client detailed information for a content ID owned by the current user. This method does nothing if the user does not own the passed content ID.
ClientNavigator.prototype.showGameDetails = function(contentId) // Content ID of the game details page to show. This is exposed as Entitlement.contentId
{
    window.location.href = GAME_DETAILS_BASE_URL + contentId;
};

// Shows the PDLC store for the passed generic ID. This allows the user to buy expansions, addons, unlockable content, etc
ClientNavigator.prototype.showPDLCStore = function(genericId) // Content ID of the product to show. This is exposed as Entitlement.contentId
{
    console.log('Showing PDLC Store for: "' + genericId + '"');
};

// Switches to the store tab and shows the home page of the store
ClientNavigator.prototype.showStoreHome = function()
{
    console.log('Showing store home');
};

// Switches to the store tab and shows the user's order history
ClientNavigator.prototype.showStoreOrderHistory = function()
{
    console.log('Showing Store Order History');
};

// Switches to the store tab and shows the subscription page
ClientNavigator.prototype.showMyAccountPath = function(path)
{
    console.log('Showing Account Page: ' + path);
};

ClientNavigator.prototype.showStoreMasterTitlePage = function(masterTitleId)
{
    console.log('Showing store master title Page: ' + masterTitleId);
};

// Switches to the store tab and shows the product page for the passed product ID
ClientNavigator.prototype.showStoreProductPage = function(productId) // Product ID of the product to show. This is exposed as Entitlement.productId
{
    console.log('Showing store product page for: "' + productId + '"');
};

// Switches to the store tab and shows the free games page
ClientNavigator.prototype.showStoreFreeGames = function()
{
    this.launchExternalBrowser('http://www.ea.com/free');
};

ClientNavigator.prototype.showAvatarChooser = function()
{
    console.log("Showing avatar chooser");
};

ClientNavigator.prototype.showStoreUrl = function(url)
{
    console.log("Showing store URL: " + url);
};

ClientNavigator.prototype.showPromoManager = function(productId, promoType, scope)
{
    console.log("Invoking promo manager with: product ID = " + productId + ", promoType = " + promoType + ", scope = " + scope);
};

// Create the Singleton, and expose to the world
window.clientNavigation = new ClientNavigator();


/**
 * ENUM: Availability
 * Describes the advertised availability of a contact
 */
var Availability =
{
    'AWAY' : 'AWAY',        // Contact is away
    'UNAVAILABLE': 'UNAVAILABLE',    // Contact is offline or invisible
    'ONLINE': 'ONLINE'        // Contact is online and available
};

/**
 * CLASS: PlayingGame
 *
 */
var PlayingGame = function(data)
{
    data = $.extend(
    {
        productId: '', // Content ID of the game the contact is playing
        gameTitle: '', // Human readable title of the game the contact is playing in plain text. This value is remotely provided and must not be trusted. In particular, it may contain malicious HTML special characters.
        joinable: false // Indicates if the game is joinable
    }, data);

    this.productId = data.productId;
    this.gameTitle = data.gameTitle;
    this.joinable = data.joinable;
};


/**
 * CLASS: RosterContact
 * Object for representing a roster contact
 */
var RosterContact = function(data)
{
    data = $.extend(
    {
        avatarUrl : '', // URL of the contact's avatar or null if the avatar hasn't been determined yet. This may be a data: URI.
        id: '', // Unique ID for the contact. This value shoudl be treated as opaque and not exposed to the user.
        // ESCAPE USING INNER TEXT
        nickname: '', // Plain text nickname to be displayed for the contact. This is typically the user's Origin ID. This value is remotely provided and must not be trusted. In particular, it may contain malicious HTML special characters
        playingGame: null, // Information about the game the user is playing through Origin or null if no game is being played
        availability: null
    }, data);

    this._avatarUrl = data.avatarUrl;
    this.id = data.id;
    this.nickname = data.nickname;
    this.playingGame = data.playingGame;
    this._availability = data.availability;
    this._owns = data.owns;

    // Emitted whenever the contact's avatar changes.
    this.avatarChanged = new Connector();

    // Emitted whenever the contact's availability or playingGame properties change
    this.presenceChanged = new Connector();
};

RosterContact.prototype.showProfile = function(source)
{
    console.log('Showing profile for RosterContact: "' + this.nickname + '"');
};

RosterContact.prototype.__defineGetter__('avatarUrl', function()
{
    return this._avatarUrl;
});
RosterContact.prototype.__defineSetter__('avatarUrl', function(val)
{
    if (val !== this._avatarUrl)
    {
        this._avatarUrl = val;
        this.avatarChanged.trigger();
    }
});

RosterContact.prototype.__defineGetter__('availability', function()
{
    return this._availability;
});
RosterContact.prototype.__defineSetter__('availability', function(val)
{
    if (!Availability[val])
    {
        console.log('RosterContact.availability:: Error - invalid precense "' + val + '"!');
    }
    if (val !== this._availability)
    {
        this._availability = val;
        this.presenceChanged.trigger();
    }
});


/**
 * CLASS: RosterView
 * Dynamic view of the current user's roster
 */
var RosterView = function()
{
    // Contacts that are currently a member of this view
    this.contacts = [];

    // Emitted whenever a contact has been added or removed from the roster view.
    // This is not emitted for changed to individual contacts.
    this.changed = new Connector();

    // Emitted whenever a contact has been added to the roster view
    this.contactAdded = new Connector();

    // Emitted whenever a contact has been removed the roster view
    this.contactRemoved = new Connector();
};

var CurrentUser = function ()
{
    this.id = '12';
}

CurrentUser.prototype.achievementSharingState = function ()
{
    return null;
}


/**
 * CLASS: OriginSocial
 * Root object of Origin social support
 */
var OriginSocial = function()
{
    this.roster = new RosterView();
    this.roster.contacts = [
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg',
            id: '2409845695',
            nickname: 'CarlSpoon',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-5nbDT63Kgv4/AAAAAAAAAAI/AAAAAAAAAAA/McAM4co9Ukc/s48-c-k/photo.jpg',
            id: '2438595621',
            nickname: 'markpro1',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', 'darkspore_dd']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh5.googleusercontent.com/-93eORAcEkAk/AAAAAAAAAAI/AAAAAAAAAAA/nHIkBlogF2E/s48-c-k/photo.jpg',
            id: '2258992822',
            nickname: 'eircon',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', '71067']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg',
            id: '2258992823',
            nickname: 'CrashOverride',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', 'darkspore_dd', '71304']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-c3Wq0wg8Q80/AAAAAAAAAAI/AAAAAAAAAAA/10h-AAHXFig/s48-c-k/photo.jpg',
            id: '2258992824',
            nickname: 'AcidBurn',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', '71308']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-jlEexHhc7G4/AAAAAAAAAAI/AAAAAAAAAAA/doOn2x5RLok/s48-c-k/photo.jpg',
            id: '2258992825',
            nickname: 'ZeroCool',
            playingGame: null,
            availability: Availability.ONLINE,
            owns: ['dragonage_na', 'darkspore_dd', '71067', 'moh_le_eastore_dd']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg',
            id: '2258992826',
            nickname: 'CerealKiller',
            playingGame: null,
            availability: Availability.AWAY,
            owns: []
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-NHhVz7mBzP8/AAAAAAAAAAI/AAAAAAAAAAA/m_Uu94sFaLI/s48-c-k/photo.jpg',
            id: '2258992827',
            nickname: 'PaulAtreides',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', 'darkspore_dd', '71304']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-l9AijjgWVZM/AAAAAAAAAAI/AAAAAAAAAAA/yTRk98j2Wo0/s48-c-k/photo.jpg',
            id: '2258992828',
            nickname: 'DukeLeto',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', '71067']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh3.googleusercontent.com/-IxGHfn0otBg/AAAAAAAAAAI/AAAAAAAAAAA/lj1f0Sl5Wbo/s48-c-k/photo.jpg',
            id: '2258992829',
            nickname: 'GurneyHalleck',
            playingGame: null,
            availability: Availability.AWAY,
            owns: ['dragonage_na', 'darkspore_dd', '71308']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
            id: '2408965432',
            nickname: 'BleysAmber',
            playingGame: null,
            availability: Availability.UNAVAILABLE,
            owns: ['dragonage_na']
        }),
        new RosterContact(
        {
            avatarUrl : 'https://lh4.googleusercontent.com/-E4BMaaIv89U/AAAAAAAAAAI/AAAAAAAAAAA/46KkSkaMxn8/s48-c-k/photo.jpg',
            id: '2429332934',
            nickname: 'SaphireDragon1',
            playingGame: new PlayingGame(
            {
                productId: 'dragonage_na',
                gameTitle: 'Dragon Age(TM): Origins',
                joinable: false
            }),
            availability: Availability.ONLINE,
            owns: ['dragonage_na', 'darkspore_dd', '71067', '71304', 'moh_le_eastore_dd']
        })
    ];

    this.currentUser = new CurrentUser();
};

// Creates a dynamic roster view of all contacts owning a specific game
//    contentId: Game product ID to filter on
OriginSocial.prototype.createOwningRosterView = function(productId)
{
    var contacts = $.grep(this.roster.contacts, function(contact, idx){ return (contact._owns.indexOf(productId) !== -1); });
    var rosterView = new RosterView();
    rosterView.contacts = contacts;
    return rosterView;
};

// Creates a dynamic roster view of all contacts playing a game
//    productId: Game content ID to filter on or null for all contacts currently playing a game
OriginSocial.prototype.createPlayingRosterView = function(productId)
{
    var contacts = $.grep(this.roster.contacts, function(contact, idx){ return (contact.playingGame && contact.playingGame.productId === productId); });
    var rosterView = new RosterView();
    rosterView.contacts = contacts;
    return rosterView;
};

// Returns the roster contact with the passed ID or null if no such contact exists
//    id: contact id
OriginSocial.prototype.getRosterContactById = function(contactId)
{
    var contacts = $.grep(this.roster.contacts, function(contact, idx){ return (contact.id === contactId); });
    return (contacts.length > 0)? contacts[0] : null;
};

OriginSocial.prototype.currentUser = function()
{
    return
};
// Create the Singleton, and expose to the world
window.originSocial = new OriginSocial();

// originUser stub
window.originUser = {commerceAllowed: true, socialAllowed: true};

var AchievementManager = function()
{
    this.achieved = 0;
    this.achievementPortfolios = [];
    this.achievementSets = [];
    this.enabled = false;
    this.rp = 0;
    this.total = 0;
    this.xp = 0;

    this.getAchievementPortfolioByPersonaId = function()
    {
        return null;
    };

    this.getAchievementPortfolioByUserId = function()
    {
        return null;
    };

    this.getAchievementSet = function()
    {
        return null;
    };

    this.getAchievementStatistics = function()
    {
        return null;
    };

    this.updateAchievementPortfolio = function()
    {
    };

    this.updateAchievementSet = function()
    {
    };

    this.updatePoints = function()
    {
    };

    this.achievementGranted = new Connector();
    this.achievementPortfolioAdded = new Connector();
    this.achievementPortfolioRemoved = new Connector();
    this.achievementSetAdded = new Connector();
    this.achievementSetRemoved = new Connector();
    this.enabledChanged = new Connector();
    this.rpChanged = new Connector();
    this.xpChanged = new Connector();
};

window.achievementManager = new AchievementManager;






var OnTheHouseQuery = function()
{
    var tempOtHOffer = "<iframe></iframe>"
    var OnTheHouseQueryInfo = {"html":"tempOtHOffer", "version":"test1234"};
    this.succeeded = new Connector();
    this.failed = new Connector();
    this.startQuery = function()
    {
        setTimeout(this.succeeded.trigger(OnTheHouseQueryInfo), 2000);
    };
};

window.onTheHouseQuery = new OnTheHouseQuery;

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
    return entitlementManager.getEntitlementById(this._entitlementsQueued[0], true);
});

ContentOperationQueueController.prototype.enqueue = function(entitlement)
{
    this._entitlementsQueued = $.merge($.merge([], this._entitlementsQueued),[entitlement.productId]);
    this.enqueued.trigger(entitlement);
    entitlement.startDownload();
    console.log(this._entitlementsQueued);

    if(entitlement.productId === this.head.productId)
    {
        entitlement.downloadOperation.resume();
    }
}

ContentOperationQueueController.prototype.pushToTop = function(id)
{
    var newHead = entitlementManager.getEntitlementById(id, true),
        oldhead = this.head;

    console.log("push to top: " + newHead.productId);

    oldhead.currentOperation.pause();

    // remove from list view and add to the top
    var newHeads = [];
    var i = this.index(newHead.id);
    for(; i < this._entitlementsQueued.length;)
    {
        if(this._entitlementsQueued[i] && this._entitlementsQueued[i].parent === newHead)
        {
            newHeads = $.merge($.merge([], newHeads), this._entitlementsQueued[i]);
        }
        else
        {
            break;
        }
    }
    newHeads = this._entitlementsQueued = $.merge($.merge([], [newHead]), newHeads);
    this.remove(newHead.productId, true);
    this._entitlementsQueued = $.merge($.merge([], newHeads), this._entitlementsQueued);

    // Create a list item for the old head

    this.enqueued.trigger(oldhead);

    for(i=0; i < newHeads.length;i++)
    {
        this.enqueued.trigger(newHead);
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
    console.log(this._entitlementsQueued);
}

ContentOperationQueueController.prototype.removeFromQueueAndAddToComplete = function(entitlement)
{
    this.remove(entitlement.productId);
    this._entitlementsCompleted = $.merge($.merge([], this._entitlementsCompleted), [entitlement]);
    this.addedToComplete.trigger(entitlement);
    if(this.head)
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
}


ContentOperationQueueController.prototype.remove = function(_id)
{
    console.log("QueueController::remove " + _id);
    this._entitlementsQueued = $.grep(this._entitlementsQueued, function(id) {return id != _id;});
    this._entitlementsCompleted = $.grep(this._entitlementsCompleted, function(id) {return id != _id;});
    this.removed.trigger(_id);
    console.log(this._entitlementsQueued);
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
        if(this._entitlementsQueued[i] === _id)
            return i;
    }
    return -1;
};


ContentOperationQueueController.prototype.isInQueue = function(_id)
{
    for(var i=0;i<this._entitlementsQueued.length;i++)
    {
        if(this._entitlementsQueued[i] === _id)
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
window.contentOperationQueueController = new ContentOperationQueueController();

})(jQuery);
