;(function($){
"use strict";

if (window.originSocial) { return; }

var conversationWindow = null, hushedUserIds = [], ExampleProductDatabase = {},
    pokeInToJoinable = {}, simulateSubcriptionCreation = {}, RemoteUsers, ExtraMembers = [],
    openConversation, pokeNicknameLoad = {}, conversationMap = {},
    Groups, nextConversationId = 1, windowWidth = 452, windowHeight=480;

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
Connector.prototype.disconnect = function(callback)
{
    this.connections = this.connections.filter(function(other)
    {
        return callback !== other;
    });
};
Connector.prototype.trigger = function()
{
    var args = Array.prototype.slice.call(arguments);
    this.connections.forEach(function(callback)
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
DateFormatter.prototype.formatTime = function(date, includeSeconds)
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
        new Setting({ name : 'Callout_SocialUserAreaShown', value: true }),
        new Setting({ name : 'Callout_GroupsTabShown', value: true }),          // change to false to see the callout
        new Setting({ name : 'Callout_VoipButtonOneToOneShown', value: true }), // change to false to see the callout
        new Setting({ name : 'Callout_VoipButtonGroupShown', value: true }),    // change to false to see the callout
        new Setting({ name : 'EnablePushToTalk', value: false })
    ];
};

ClientSettings.prototype.writeSetting = function(_name, _value)
{
    this.getSettingByName(_name).value = _value;
};

ClientSettings.prototype.readSetting = function(name)
{
    return this.getSettingByName(name).value;
};

ClientSettings.prototype.readBoolSetting = function(name)
{
    return clientSettings.readSetting(name) === true ? true : false;
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

    this._menuAction = null;
    this._actions = [];
    this._html = '';
    this._title = '';

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

Menu.prototype.addObjectName = function(newName)
{
    // this is a no-op because the real function adds an Qt object name
}

Menu.prototype.addMenu = function(toAdd)
{
    var submenu = nativeMenu.createMenu();
    submenu._setTitle(toAdd);
    submenu._menuAction = new MenuAction(toAdd, this._actions.length, this);
    submenu._menuAction.triggered.connect(function()
    {
            var position =
            {
                "x": (nativeMenu.mouseX - 20),
                "y": (nativeMenu.mouseY - 20)
            };

            submenu.popup(position);
    });
    this._actions.push(submenu._menuAction);
    return submenu;
};

Menu.prototype.__defineGetter__('menuAction', function()
{
    if (this._menuAction)
    {
        return this._menuAction;
    }
    else
    {
        console.log(this + "<-- This was not a submenu");
    }
});

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

// Pops the menu up at the specified  position
Menu.prototype.popup = function(position)
{
    var $menu = this._getElm(false, false).attr('data-id', this._id);

    if (!$menu.hasClass('hidden'))
    {
        // Do this to trigger aboutToHide
        this.hide();
    }

    this.aboutToShow.trigger();

    if (!position)
    {
        position =
        {
            "x": (nativeMenu.mouseX - 20),
            "y": (nativeMenu.mouseY - 20)}
    }

    this._html = '';
    $menu.children('ul').empty();
    this._draw($menu);
    $menu
        .css({'top': position.y + 'px', 'left': position.x + 'px'})
        .removeClass('hidden')
        .show()
        .css({opacity: 0})
        .animate({opacity: 1}, 200);
};

//
Menu.prototype.clear = function()
{
    if (this._actions.length === 0) { return; }
    this._actions = [];
};

Menu.prototype.setMinWidth = function(w)
{
    // No-op
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
        selector += ':not(.hidden)';
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
        $menu.children('ul').html(this._html);
    }
};

Menu.prototype._setTitle = function(title)
{
    this._title = title;
}

Menu._get = function(id)
{
    return Menu._instances[id];
};

Menu._count = 0;

Menu._instances = {};

Menu._init = function()
{
    var $body = $(document.body);

    $('<div id="context-menu" class="hidden" data-id=""><ul></ul></div>')
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
};

Menu._close = function()
{
    var $menu = $('#context-menu:not(.hidden)');
    if ($menu.length === 0) { return; }
    var menu = Menu._get($menu.attr('data-id'));
    if (!menu) { return; }

    menu.aboutToHide.trigger();
    $menu.addClass('hidden').hide();
};

Menu.prototype.hide = Menu._close;

Menu._init();

/**
 * CLASS: ClientNavigator
 * Provides high-level methods in-client navigation
 */
var ClientNavigator = function()
{
};

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
    console.log('Showing Game Details for: "' + contentId + '"');
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

ClientNavigator.prototype.showCreateRoom = function(name, groupGuid)
{
    console.log("showCreateRoom()");
};

ClientNavigator.prototype.showEditGroup = function(name, groupGuid)
{
    console.log("showEditGroup()");
};

ClientNavigator.prototype.showDeleteGroup = function(name, groupGuid)
{
    console.log("showDeleteGroup()");
};

ClientNavigator.prototype.showLeaveGroup = function(name, groupGuid)
{
    console.log("showLeaveGroup()");
};

ClientNavigator.prototype.showYouNeedFriendsDialog = function(name, groupGuid)
{
    console.log("showYouNeedFriendsDialog()");
};

ClientNavigator.prototype.showInviteFriendsToGroupDialog = function(guid)
{
    var index = Origin.utilities.getGroupIndex(guid);

    var group = originSocial.groupList.groups[index];
    
    //Populate window with members.
    window.setTimeout(function() {        
        group.membersLoadFinished.trigger()
    }, 500);
    
    var windowUrl = "invite-friends-window.html?groupGuid=" + guid;
    window.open(windowUrl, 'invite-friends-window', 'toolbar=0,location=0,status=0,width='+windowWidth+',height='+windowHeight+'');

};

ClientNavigator.prototype.showInviteFriendsToRoomDialog = function(name, guid, channel, convId)
{
    console.log("TR Show invite friends to group channel: " + name + " groupGuid: " + guid);
    console.log("Removed");
};


ClientNavigator.prototype.showGroupMembersDialog = function(name, guid, id)
{    
    var theGroup = originSocial.groupList.groups[0];

    // Simulate loading pages of 100 members
    var delay = 1000;
    window.setTimeout(function() {
        theGroup.membersLoadFinished.trigger();
    }, delay);

    var windowUrl = "group-members-window.html?groupGuid=" + guid;
    window.open(windowUrl, 'group-members-window', 'toolbar=0,location=0,status=0,width='+windowWidth+',height='+windowHeight+'');

};

ClientNavigator.prototype.showTransferGroupOwnerDialog = function(name, guid, id)
{
    console.log("TR Show transfor group owner dialog: " + name + " groupGuid: " + guid);
    console.log("Removed");
};

ClientNavigator.prototype.showVoiceSurveyPrompt = function()
{
    console.log("Show voip survey prompt");
};


/**
 * CLASS: AudioPlayer
 * Object for playing sounds
 */

var AudioPlayer = function()
{
    this.index = 0;
};

AudioPlayer.prototype.play = function(path, numLoops)
{
    console.log("AudioPlayer.play(" + path + ")");
    return this.index++;
};

AudioPlayer.prototype.stop = function(index)
{
    console.log("AudioPlayer.stop(" + index + ")");
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
 * CLASS: PlayingGame
 *
 */
var PlayingGame = function(productId, joinable, broadcastUrl)
{
    this.productId = productId;
    this.gameTitle = ExampleProductDatabase[productId].title;
    this.joinable = joinable;
    this.broadcastUrl = broadcastUrl;
    if (productId.indexOf('nog_') !== 0)
    {
        this.purchasable = true;
    }
    else
    {
        this.purchasable = false;
    }
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
 * CLASS: Connection
 *
 * Information about the current connection
 */
var Connection = function()
{
    var established, delayFor, disconnectDelay;
    this.changed = new Connector;

    if ($.getQueryString("delayConnectFor"))
    {
        delayFor = parseInt($.getQueryString("delayConnectFor"), 10);
    }

    if (delayFor > 0)
    {
        established = false;
        // Delay load for some ms
        window.setTimeout(function()
        {
            established = true;
            this.changed.trigger();
        }.bind(this), delayFor);
    }
    else
    {
        established = true;
    }

    if ($.getQueryString("delayDisconnectFor"))
    {
        disconnectDelay = parseInt($.getQueryString("delayDisconnectFor"));        
    }

    if (disconnectDelay > 0)
    {
        // Delay load for some ms
        window.setTimeout(function()
        {
            established = false;
            this.changed.trigger();
        }.bind(this), disconnectDelay);
        
        window.setTimeout(function()
        {
            established = true;
            this.changed.trigger();
        }.bind(this), disconnectDelay*2);
        
    }

    Connection.prototype.__defineGetter__('established', function()
    {
        return established;
    });

    // The client API defines this as read only but give it a setter for testing
    Connection.prototype.__defineSetter__('established', function(state)
    {
        established = state;
        this.changed.trigger();
    });

};

function randIntBetween(min, max)
{
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

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
    user.id = '1';
    user.nickname = 'hiro'
    user.realName = {firstName: 'Hiro', lastName: 'Protagonist'};

    playingGame = joinableBf3PlayingGame;
    availability = Availability.ONLINE;
    requestedAvailability = Availability.ONLINE;

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

    user.userTalking = new Connector();
    user.userTalking = new Connector();
    user.userStoppedTalking = new Connector();
    user.userMuted = new Connector();
    user.userUnMuted = new Connector();
    user.userJoined = new Connector();
    user.userLeft = new Connector();

    // Emitted whenever the user's large avatar changes.
    user.largeAvatarChanged = new Connector();

    // Emitted whenever the user's availability or playingGame properties change
    user.presenceChanged = new Connector();

    // Emitted whenever the user's name changes
    user.nameChanged = new Connector();

    // Emitted whenever the user's visibility changes
    user.visibilityChanged = new Connector();

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

    return user;
}();

openConversation = function(conversationId)
{
    var message, windowUrl;

    // Create a conversation on our side so it appears in originChat.conversations
    originChat.getConversationById(conversationId);

    if (conversationWindow && !conversationWindow.closed)
    {
        // Poke the existing window
        message = {type: 'raiseConversation', 'conversationId': conversationId};
        conversationWindow.postMessage(message, '*');
        conversationWindow.focus();
    }
    else
    {
        // Open a new window
        windowUrl = "chat-window.html?initialConversations=" + conversationId;
        conversationWindow = window.open(windowUrl, 'chat-window', 'toolbar=0,location=0,status=0,width='+700+',height='+500+'');
    }
}


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

    // Emitted whenever the user's availability or playingGame properties change
    this.presenceChanged = new Connector();

    // Emitted whenever the user's subscriptionState property changes
    this.subscriptionStateChanged = new Connector();

    // Emitted whenever the user's name changes
    this.nameChanged = new Connector();

    this.addedToRoster = new Connector();
    this.removedFromRoster = new Connector();

    this.blockChanged = new Connector();
    
    this.capabilitiesChanged = new Connector();

    // Emitted for voice state changes
    this.userTalking = new Connector();
    this.userStoppedTalking = new Connector();
    this.userMuted = new Connector();
    this.userUnMuted = new Connector();
    this.userJoined = new Connector();
    this.userLeft = new Connector();

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

    this.__defineGetter__('capabilities', function()
    {
        return ["VOIP"];
    });

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
    //
    this.randomizePresence = function () {
        selectRandomPresence();
    };
    selectRandomPresence();
};

RemoteUser.prototype.showProfile = function()
{
    console.log('Showing profile for RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.startConversation = function()
{
    var conversation, id=0, key, conv, remoteUser;

    // If we're already chatting with this person just open that existing conversation
    for (key in conversationMap)
    {
        if (conversationMap.hasOwnProperty(key))
        {
            conv = conversationMap[key];
            if (conv.type === "ONE_TO_ONE")
            {
                remoteUser = conv.participants[0];
                if (remoteUser.id === this.id)
                {
                    id = conv.id;
                }
            }
        }
    }

    if (id === 0)
    {
        // Create a new 1:1 conversation
        conversation = new Conversation([this], "ONE_TO_ONE");
        id = conversation.id;
        conversationMap[id] = conversation;
    }

    openConversation(id);
};

RemoteUser.prototype.inviteToGame = function()
{
    console.log('Inviting to game for RemoteUser: "' + this.nickname + '"');
};

RemoteUser.prototype.joinGame = function(source)
{
    console.log('Joining game of RemoteUser: "' + this.nickname + '" from: ' + source);
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
};

RemoteUser.prototype.removeAndBlock = function()
{
    originSocial.roster.removeContact(this);
    this.block();
};

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

/**
 * CLASS: GroupList
 * Dynamic view of the current user's groups
 */
var GroupList = function()
{
    this.groups = [];

    // Emitted when a chat group is added
    this.addChatGroup = new Connector();

    // Emitted when a chat group is removed
    this.removeChatGroup = new Connector();

    // Emitted when a chat group is changed
    this.changeChatGroup = new Connector();

    // Emitted when a chat channel is added
    this.addChatChannel = new Connector();

    // Emitted when a chat channel is removed
    this.removeChatChannel = new Connector();

    // Emitted when a chat channel is changed
    this.changeChatChannel = new Connector();
}
GroupList.prototype.groupForChannel = function(channel)
{
    var group = null;

    this.groups.forEach(function(g)
    {
        g.channels.forEach(function(c)
        {
            if (c.id === channel.id)
            {
                group = g;
                return false; // break
            }
        });
    });

    return group;
};

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

    // Emitted whenever any contact in the roster updates any of: name, presence, avatar+
    this.anyUserChange = new Connector();
};

RosterView.prototype.hasFriends = function()
{
    return true;
}

RosterView.prototype.removeContact = function(contact)
{
    if ((contact.subscriptionState.direction === 'NONE') &&
        !contact.subscriptionState.pendingCurrentUserApproval &&
        !contact.subscriptionState.pendingContactApproval)
    {
        console.log('Attempted to remove non-contact');
        return;
    }

    contact.subscriptionState = {
        direction: 'NONE',
        pendingCurrentUserApproval: false,
        pendingContactApproval: false
    };

    // Emit the signal
    contact.removedFromRoster.trigger();
    this.contactRemoved.trigger(contact);
};

// Create RemoteUsers
RemoteUsers = [
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg',
        id: '1',
        nickname: 'CarlSpoon',
        realName: {firstName: 'Bob', lastName: 'Loblaw'},
        owns: ['dragonage_na']
    }),
    new RemoteUser(
    {
        avatarUrl : null,
        id: '2',
        nickname: 'TheQueen',
        realName: {firstName: 'Amber', lastName: 'Macx'},
        owns: ['nog_calculator']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-5nbDT63Kgv4/AAAAAAAAAAI/AAAAAAAAAAA/McAM4co9Ukc/s48-c-k/photo.jpg',
        id: '3',
        nickname: 'markpro1',
        owns: ['dragonage_na', 'darkspore_dd']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh5.googleusercontent.com/-93eORAcEkAk/AAAAAAAAAAI/AAAAAAAAAAA/nHIkBlogF2E/s48-c-k/photo.jpg',
        id: '4',
        nickname: 'eircon',
        realName: {firstName: 'Eric', lastName: 'Neugebauer'},
        owns: ['dragonage_na', '71067']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-RcvtfuoErt8/AAAAAAAAAAI/AAAAAAAAAAA/zo6M0EUjpys/s48-c-k/photo.jpg',
        id: '5',
        nickname: 'CrashOverride',
        realName: {firstName: 'Dade', lastName: 'Murphy'},
        owns: ['dragonage_na', 'darkspore_dd', '71304']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-c3Wq0wg8Q80/AAAAAAAAAAI/AAAAAAAAAAA/10h-AAHXFig/s48-c-k/photo.jpg',
        id: '6',
        nickname: 'AcidBurn',
        realName: {firstName: 'Kate', lastName: 'Libby'},
        owns: ['dragonage_na', '71308']
    }),
    new RemoteUser(   
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '7',
        nickname: 'Banshee',
        realName: {firstName: 'Sibohan', lastName: 'Eire'},
        owns: []
    }),    
    new RemoteUser(   
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '8',
        nickname: 'Licorice',
        realName: {firstName: 'Salty', lastName: 'Sweets'},
        owns: []
    }),
    new RemoteUser(   
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '9',
        nickname: 'Ozone',
        realName: {firstName: 'Firster', lastName: 'Laster'},
        owns: []
    }), 
    new RemoteUser(   
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '10',
        nickname: 'Shasta',
        realName: {firstName: 'Mountain', lastName: 'Lion'},
        owns: []
    }),    
    new RemoteUser(   
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '11',
        nickname: 'MeatyOne',
        realName: {firstName: 'Mountain', lastName: 'Lion'},
        owns: []
    }),    
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-jlEexHhc7G4/AAAAAAAAAAI/AAAAAAAAAAA/doOn2x5RLok/s48-c-k/photo.jpg',
        id: '100',
        nickname: 'ZeroCool',
        realName: null,
        owns: ['dragonage_na', 'darkspore_dd', '71067', 'moh_le_eastore_dd'],
        subscriptionState: {pendingContactApproval: false, pendingCurrentUserApproval: true, direction: 'NONE'}
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-70JuwFWyc98/AAAAAAAAAAI/AAAAAAAAAAA/QyI64VUlhJY/s48-c-k/photo.jpg',
        id: '101',
        nickname: 'CerealKiller',
        realName: {firstName: 'Emmanuel', lastName: 'Goldstein '},
        owns: []
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-NHhVz7mBzP8/AAAAAAAAAAI/AAAAAAAAAAA/m_Uu94sFaLI/s48-c-k/photo.jpg',
        id: '102',
        nickname: 'PaulAtreides',
        realName: {firstName: 'Muad', lastName: 'Dib'},
        owns: ['dragonage_na', 'darkspore_dd', '71304']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-l9AijjgWVZM/AAAAAAAAAAI/AAAAAAAAAAA/yTRk98j2Wo0/s48-c-k/photo.jpg',
        id: '103',
        nickname: 'DukeLeto',
        realName: {firstName: 'Leto', lastName: 'Ateides'},
        owns: ['dragonage_na', '71067']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-IxGHfn0otBg/AAAAAAAAAAI/AAAAAAAAAAA/lj1f0Sl5Wbo/s48-c-k/photo.jpg',
        id: '104',
        nickname: 'GurneyHalleck',
        owns: ['dragonage_na', 'darkspore_dd', '71308']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
        id: '105',
        nickname: 'BleysAmber',
        owns: ['dragonage_na']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh4.googleusercontent.com/-E4BMaaIv89U/AAAAAAAAAAI/AAAAAAAAAAA/46KkSkaMxn8/s48-c-k/photo.jpg',
        id: '106',
        nickname: 'SaphireDragon1',
        owns: ['dragonage_na', 'darkspore_dd', '71067', '71304', 'moh_le_eastore_dd']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-Y86IN-vEObo/AAAAAAAAAAI/AAAAAAABTfQ/Tnz_ZfOMSkw/s48-c-k/photo.jpg',
        id: '107',
        nickname: 'Very Long Origin ID That Wants to be Truncated',
        realName: {firstName: 'Evenlongerworth', lastName: 'McLonglastnamington'},
        owns: ['dragonage_na', 'darkspore_dd', '71308']
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-pqiGNzk-VN8/AAAAAAAAAAI/AAAAAAAAABI/p0-yuG38-xk/s48-c-k/photo.jpg',
        id: '108',
        nickname: 'leFlambeur',
        realName: "Lou",
        owns: [],
        subscriptionState: {pendingContactApproval: true, pendingCurrentUserApproval: false, direction: 'NONE'}
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-Y86IN-vEObo/AAAAAAAAAAI/AAAAAAAAAAA/xOvnPfMX1XU/s48-c-k/photo.jpg',
        id: '109',
        nickname: 'Xaviar S. Scriptly',
        realName: {firstName: '<script>alert("0wned!")</script>', lastName: '<script>console.log("LOL");</script>'},
        owns: [],
    }),
    new RemoteUser(
    {
        avatarUrl : 'https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg',
        id: '110',
        nickname: 'Xenia',
        realName: "Xenia Phobia",
        owns: [],
        subscriptionState: {pendingContactApproval: false, pendingCurrentUserApproval: false, direction: 'NONE'},
        delayMetadataBy: randIntBetween(100, 200)
    }),
    new RemoteUser(   {
                    avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                    id: '111',
                    nickname: 'Jango',
                    realName: {firstName: 'Silent', lastName: 'Dee'},
                    owns: [],
                }),
    new RemoteUser(   {
                    avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                    id: '112',
                    nickname: 'Banshee',
                    realName: {firstName: 'Sibohan', lastName: 'Eire'},
                    owns: [],
                }),
    new RemoteUser(   {
                    avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                    id: '113',
                    nickname: 'Licorice',
                    realName: {firstName: 'Salty', lastName: 'Sweets'},
                    owns: [],
                })
];

//create extra contacts  / remote users.
var extraContactNum = 2; 
var list = RemoteUsers;
var startId = 114
var alphabet=["a","b","c","d","e","f","g","h","h","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"];

for(var j = 0;j < extraContactNum;j++) {
    var ran = Math.floor(Math.random()*alphabet.length);
    var aName = alphabet[ran] + "name_"+j;
    var id = startId+j;
    var contact = new RemoteUser({
//        avatarUrl : 'chat/images/common/user-image.png',
        id: id,
        nickname: aName,
        realName: {firstName: 'Silent', lastName: 'Dee'},
        availability:Availability.UNAVAILABLE,
        owns: []
    });
    list.push(contact)
};    

/**
 * CLASS: OriginSocial
 * Root object of Origin social support
 */
var OriginSocial = function()
{
    this.currentUser = connectedUser;
    this.connection = new Connection;
    this.roster = new RosterView();
    this.roster.hasLoaded = true;
    this.roster.loaded = new Connector();
    this.groupList = new GroupList();
    this.groupListLoadSuccess = true;

    this.roster.__defineGetter__('contacts', function()
    {
        return RemoteUsers.filter(function(user)
        {
            return (user.subscriptionState.direction !== 'NONE') ||
                    user.subscriptionState.pendingCurrentUserApproval ||
                    user.subscriptionState.pendingContactApproval;
        });
    });

    this.groupList.__defineGetter__('groups', function()
    {
        return Groups.filter(function(group)
        {
            return true;
        });
    });
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

    if (users.length > 0) {
        return users[0];
    }
    
    var users = ExtraMembers.filter(function(user)
    {
        return (user.id === userId);
    });    
    
    return (users.length > 0) ? users[0] : null;
};

// Returns the group with the passed ID or null if no such group exists
OriginSocial.prototype.getGroupById = function(groupId)
{
    var groups = Groups.filter(function(group)
    {
        return (group.id === groupId);
    });

    return (groups.length > 0) ? groups[0] : null;
};

// Returns the channel with the passed ID or null if no such channel exists
OriginSocial.prototype.getChannelById = function(channelId)
{
    var retChannel;
    var groups = Groups.filter(function(group)
    {
        if (group.channels)
        {
            group.channels.forEach(function(channel)
            {
                if (channel.id === channelId)
                {
                    retChannel = channel;
                    return true;
                }
                return false;
            });
        }
        else
        {
            return null;
        }
    });

    return (retChannel) ? retChannel : null;
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
}

OriginSocial.prototype.showCreateMultiUserConversationDialog = function ()
{
    console.log('Show invite to group dialog');
}

OriginSocial.prototype.getGroupByGuid = function(guid)
{
    var groups = Groups.filter(function(group)
    {
        return (group.groupGuid === guid);
    });

    return (groups.length > 0) ? groups[0] : null;
}

var Conversation = function(participants, type)
{
    var startTime = new Date, endTime = null, appendMessage,
        appendMultiplayerInvite, changeThreadId,
        voiceParticipants = [], participantMuteStates = {},
        enumMuteStates = { 0: 'VOICEMUTE_NONE', 1: 'VOICECALL_MUTED', 2: 'REMOTE_VOICE_LOCAL_MUTE', 4: 'REMOTE_VOICE_REMOTE_MUTE'},
        changeParticipants, changeVoiceParticipants, changeVoiceParticipantsTalkingStatus, changeVoiceParticipantsRemoteMuteStatus,
        getMuteStateForUser, addMuteStateForUser, removeMuteStateForUser, 
        participantNicknames, inviteEvent, addNewEvent, prevConvKey,
        finishReason = null;

    this.id = nextConversationId++;
    this.events = [];
    this.finishOnClose = false;
    this.typeChanged = new Connector();

    this.participants = participants.slice(0);

    this.voiceStarted = new Connector();
    this.voiceFinished = new Connector();

    this.eventAdded = new Connector();

    this.leaveRequested = new Connector();
    this.updateGroupName = new Connector();
    this.remoteAnsweredVoiceCall = new Connector();

    this.chatGroup =  originSocial.groupList.groups[0];
    
    this.groupName = "";
    this.groupGuid = "";
    this.channelName = "";
    this.channelId = "";

    this.isVoiceCallInProgress = false; // Is this still needed?

    this.voiceCallState = "VOICECALL_DISCONNECTED";
    
    this.voiceCallState = "SDK";

    prevConvKey = 'prev-conv-' + this.id;

    addNewEvent = function(evt)
    {
        evt.time = evt.time || new Date;
        this.events.push(evt);
        this.eventAdded.trigger(evt);
    }.bind(this);

    this.addNewEvent = addNewEvent;

    this.__defineGetter__('type', function()
    {
        return type;
    });

    this.finish = function()
    {
        var messageEvents;

        if (this.type === 'ONE_TO_ONE')
        {
            // Save this conversation
            messageEvents = this.events.filter(function(evt)
            {
                return evt.type === 'MESSAGE';
            })
            .map(function(messageEvent)
            {
                return {
                    body: messageEvent.body,
                    from: messageEvent.from.id,
                    threadId: messageEvent.threadId,
                    time: messageEvent.time.getTime(),
                    state: messageEvent.state
                };
            });

            if (messageEvents.length)
            {
                localStorage.setItem(prevConvKey, JSON.stringify(messageEvents));
            }
        }
    }

    this.__defineGetter__('previousConversations', function()
    {
        var prevConv;

        if (prevConvKey)
        {
            prevConv = JSON.parse(localStorage.getItem(prevConvKey));

            if (prevConv)
            {
                return [{
                    events: prevConv.map(function(storedMessage)
                    {
                        return {
                            type: 'MESSAGE',
                            body: storedMessage.body,
                            from: originSocial.getUserById(storedMessage.from),
                            threadId: storedMessage.threadId,
                            time: new Date(storedMessage.time)
                        };
                    })
                }];
            }
        }

        return [];
    });

    this.__defineGetter__('finishReason', function ()
    {
        return finishReason;
    });

    this.__defineGetter__('active', function ()
    {
        return (type !== 'FINISHED');
    });

    this.inviteRemoteUser = function(user)
    {
    };

    this.joinVoice = function()
    {
        var user;

        if (this.type === "ONE_TO_ONE")
        {
            user = this.participants[0];

            if (this.voiceCallState === "VOICECALL_DISCONNECTED")
            {
                // Start an outgoing call
                this.voiceCallState = "VOICECALL_OUTGOING";

                this.addNewEvent({
                    type: 'VOICE_LOCAL_CALLING'
                });
            }
            else if (this.voiceCallState === "VOICECALL_INCOMING")
            {
                // Answer an incoming call
                this.voiceCallState = "VOICECALL_CONNECTED";

                this.addNewEvent({
                    type: 'VOICE_STARTED',
                    from: user
                });
            }
        }
        else
        {
            if (this.voiceCallState === "VOICECALL_DISCONNECTED")
            {
                // Connect voice
                this.voiceCallState = "VOICECALL_CONNECTED";
                this.addNewEvent({
                    type: 'VOICE_STARTED'
                });
            }
        }
    };

    this.leaveVoice = function()
    {
        var user;

        console.log("LEAVE VOICE");

        if (this.type === "ONE_TO_ONE")
        {
            if (this.voiceCallState !== "VOICECALL_DISCONNECTED")
            {
                user = this.participants[0];

                this.voiceCallState = "VOICECALL_DISCONNECTED";

                this.addNewEvent({
                    type: 'VOICE_ENDED',
                    from: user
                });
            }
        }
        else
        {
            if (this.voiceCallState !== "VOICECALL_DISCONNECTED")
            {
                this.voiceCallState = "VOICECALL_DISCONNECTED";
                this.addNewEvent({
                    type: 'VOICE_ENDED'
                });
            }
        }
    };

    this.muteSelfInVoice = function()
    {
        addMuteStateForUser(originSocial.currentUser, 'VOICECALL_MUTED');
        this.addNewEvent({
            type: 'VOICE_LOCAL_MUTED',
            from: originSocial.currentUser
        });
    };

    this.unmuteSelfInVoice = function()
    {
        removeMuteStateForUser(originSocial.currentUser, 'VOICECALL_MUTED');
        this.addNewEvent({
            type: 'VOICE_LOCAL_UNMUTED',
            from: originSocial.currentUser
        });
    };

    this.muteUserInVoice = function(user)
    {
        addMuteStateForUser(user, 'REMOTE_VOICE_LOCAL_MUTE');
        this.addNewEvent({
            type: 'VOICE_LOCAL_MUTED',
            from: user
        });
    };

    this.unmuteUserInVoice = function(user)
    {
        removeMuteStateForUser(user, 'REMOTE_VOICE_LOCAL_MUTE');
        this.addNewEvent({
            type: 'VOICE_LOCAL_UNMUTED',
            from: user
        });
    };

/*
    this.voiceCallState = function()
    {
        return getMuteStateForUser(originSocial.currentUser);
    };
*/

    this.remoteUserVoiceCallState = function(user)
    {
        return getMuteStateForUser(user);
    };

    getMuteStateForUser = function(remoteUser)
    {
        var stateString="", stateValue = participantMuteStates[originSocial.currentUser.id];
        
        if (stateValue === 0)
        {
            stateString = enumMuteStates[0];
        }
        else
        {
            if (stateValue & 1)
            {
                stateString += enumMuteStates[1];
            }
            if (stateValue & 2)
            {
                stateString += enumMuteStates[2];
            }
            if (stateValue & 4)
            {
                stateString += enumMuteStates[4];
            }
            if (stateValue & 8)
            {
                stateString += enumMuteStates[8];
            }
        }

        return stateString;
    };

    addMuteStateForUser = function(remoteUser, state)
    {
        if (state === enumMuteStates[0])
        {
            participantMuteStates[originSocial.currentUser.id] = 0;
        }
        else if (state === enumMuteStates[1])
        {
            participantMuteStates[originSocial.currentUser.id] |= 1;
        }
        else if (state === enumMuteStates[2])
        {
            participantMuteStates[originSocial.currentUser.id] |= 2;
        }
        else if (state === enumMuteStates[4])
        {
            participantMuteStates[originSocial.currentUser.id] |= 4;
        }
        else if (state === enumMuteStates[8])
        {
            participantMuteStates[originSocial.currentUser.id] |= 8;
        }
    };

    removeMuteStateForUser = function(remoteUser, state)
    {
        if (state === enumMuteStates[0])
        {
            // You can't remove "VOICEMUTE_NONE"
        }
        else if (state === enumMuteStates[1])
        {
            participantMuteStates[originSocial.currentUser.id] = (~participantMuteStates[originSocial.currentUser.id]) & 1;
        }
        else if (state === enumMuteStates[2])
        {
            participantMuteStates[originSocial.currentUser.id] = (~participantMuteStates[originSocial.currentUser.id]) & 2;
        }
        else if (state === enumMuteStates[4])
        {
            participantMuteStates[originSocial.currentUser.id] = (~participantMuteStates[originSocial.currentUser.id]) & 4;
        }
        else if (state === enumMuteStates[8])
        {
            participantMuteStates[originSocial.currentUser.id] = (~participantMuteStates[originSocial.currentUser.id]) & 8;
        }
    };

    // Look for presence changes in our participants
    RemoteUsers.forEach(function(user)
    {
        var previousAvailability, previousStatusText, previousPlayingGame,
            previousSubscriptionState = $.extend({}, user.subscriptionState),
            savePresence, isParticipant, previousBroadcastUrl;

        isParticipant = (function()
        {
           return this.participants.indexOf(user) !== -1;
        }).bind(this);

        savePresence = function()
        {
            previousAvailability = user.availability;
            previousStatusText = user.statusText;
            previousPlayingGame = user.playingGame;
            previousBroadcastUrl = user.broadcastUrl;
        };

        savePresence();

        user.presenceChanged.connect($.proxy(function()
        {
            if (!isParticipant())
            {
                return;
            }

            addNewEvent({
                type: 'PRESENCE_CHANGE',
                user: user,
                availability: user.availability,
                statusText: user.statusText,
                playingGame: user.playingGame,
                previousAvailability: previousAvailability,
                previousStatusText: previousStatusText,
                previousPlayingGame: previousPlayingGame
            });

            savePresence();
        }, this));

        user.subscriptionStateChanged.connect($.proxy(function()
        {
            if (!isParticipant())
            {
                return;
            }

            addNewEvent({
                type: 'SUBSCRIPTION_STATE_CHANGE',
                user: user,
                subscriptionState: user.subscriptionState,
                previousSubscriptionState: previousSubscriptionState
            });

            previousSubscriptionState = user.subscriptionState;
        }, this));
    }, this);

    changeThreadId = $.proxy(function()
    {
        // Create a new thread ID
        this.threadId = this.generateUniqueThreadId();
        window.setTimeout(changeThreadId, randIntBetween(0, 5 * 60 * 1000));
    }, this);

    // Helper to append a message to the conversation from the other user
    appendMessage = function(messageBody)
    {
        var sender;

        if (this.participants.length === 0)
        {
            return;
        }

        // Pick a random sender
        sender = this.participants[randIntBetween(0, this.participants.length - 1)];

        if (hushedUserIds.indexOf(sender.id) !== -1)
        {
            // We've been hushed
            return;
        }

        addNewEvent({
            type: 'MESSAGE',
            body: messageBody,
            threadId: this.threadId,
            from: sender
        });

    }.bind(this);

    changeParticipants = function()
    {
        var nonParticipants, randomIndex, randomUser, newEvent;

        if (this.type == "MULTI_USER")
        {
            randomIndex = randIntBetween(0, this.participants.length - 1);
            randomUser = this.participants[randomIndex];
            if (randomUser.id !== originSocial.currentUser.id)
            {
                if ((randIntBetween(0, 1) === 0) && (this.participants.length > 0))
                {
                    // Make someone leave

                    // Kill the user
                    this.participants.splice(randomIndex, 1);

                    addNewEvent({
                        type: 'PARTICIPANT_EXIT',
                        user: randomUser
                    });
                }
                else
                {
                    // Make someone join
                    nonParticipants = RemoteUsers.filter(function(user)
                    {
                        return this.participants.indexOf(user) === -1;
                    }, this);

                    if (nonParticipants.length > 0)
                    {
                        this.participants.push(randomUser);

                        addNewEvent({
                            type: 'PARTICIPANT_ENTER',
                            user: randomUser,
                            nickname: randomUser.nickname
                        });
                    }
                }
            }
            window.setTimeout(changeParticipants, randIntBetween(0, 90 * 1000));
        }
    }.bind(this);

    changeVoiceParticipants = function()
    {
        var randomIndex, randomUser, newEvent;

        if (this.type == "MULTI_USER")
        {
            randomIndex = randIntBetween(0, this.participants.length - 1);
            randomUser = this.participants[randomIndex];
            if (randomUser.id !== originSocial.currentUser.id)
            {
                if (randIntBetween(0, 1) === 0)
                {
                    // Make someone leave voice
                    voiceParticipants.splice(randomIndex,1);
                    delete participantMuteStates[randomUser.id];
                    addNewEvent({
                        type: 'VOICE_LEAVE',
                        from: randomUser
                    });
                }
                else
                {
                    // Make someone join voice
                    voiceParticipants.push(randomUser);
                    participantMuteStates[randomUser.id] = 0;
                    addNewEvent({
                        type: 'VOICE_JOIN',
                        from: randomUser
                    });
                }
            }
            window.setTimeout(changeVoiceParticipants, randIntBetween(0, 10 * 1000));
        }
    }.bind(this);

    changeVoiceParticipantsTalkingStatus = function()
    {
        var randomIndex, randomUser, newEvent;

        // Me
        if (this.voiceCallState === "VOICECALL_CONNECTED")
        {
            if (randIntBetween(0, 1) === 0)
            {
                if (randIntBetween(0, 1) === 0)
                {
                    // Start talking
                    addNewEvent({
                        type: 'VOICE_START_TALKING',
                        from: originSocial.currentUser
                    });
                }
                else
                {
                    // Stop talking
                    addNewEvent({
                        type: 'VOICE_STOP_TALKING',
                        from: originSocial.currentUser
                    });
                }
            }
        }

        // Everyone else
        if (this.type == "MULTI_USER")
        {
            if (voiceParticipants.length)
            {
                randomIndex = randIntBetween(0, voiceParticipants.length - 1);
                randomUser = voiceParticipants[randomIndex];
                if (randIntBetween(0, 1) === 0)
                {
                    // Make someone start talking
                    addNewEvent({
                        type: 'VOICE_START_TALKING',
                        from: randomUser
                    });
                }
                else
                {
                    // Make someone stop talking
                    addNewEvent({
                        type: 'VOICE_STOP_TALKING',
                        from: randomUser
                    });
                }
            }
        }

        window.setTimeout(changeVoiceParticipantsTalkingStatus, randIntBetween(0, 1 * 1000));
    }.bind(this);

    changeVoiceParticipantsRemoteMuteStatus = function()
    {
        var randomIndex, randomUser, newEvent;

        if (this.type == "MULTI_USER")
        {
            if (voiceParticipants.length)
            {
                randomIndex = randIntBetween(0, voiceParticipants.length - 1);
                randomUser = voiceParticipants[randomIndex];
                if (randIntBetween(0, 1) === 0)
                {
                    // Make someone mute themselves
                    addMuteStateForUser(randomUser, 'VOICEMUTE_REMOTE');
                    addNewEvent({
                        type: 'VOICE_REMOTE_MUTED',
                        from: randomUser
                    });
                }
                else
                {
                    // Make someone unmute themselves
                    removeMuteStateForUser(randomUser, 'VOICEMUTE_REMOTE');
                    addNewEvent({
                        type: 'VOICE_REMOTE_UNMUTED',
                        from: randomUser
                    });
                }
            }
        }

        window.setTimeout(changeVoiceParticipantsRemoteMuteStatus, randIntBetween(0, 1 * 1000));
    }.bind(this);

    appendMultiplayerInvite = function()
    {
        addNewEvent({
            type: 'MULTIPLAYER_INVITE',
            message: null,
            from: participants[0],
            productId: participants[0].playingGame.productId,
            multiplayerId: participants[0].playingGame.productId
        });
    }.bind(this);

    var receiveRandomMessage = $.proxy(function()
    {
        var messageBody, newEvent, messageType;

        messageType = randIntBetween(0, 2);

        if (messageType === 0)
        {
            // Emulate a long message
            var loremIpsum = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
           appendMessage(loremIpsum.substr(0, randIntBetween(0, loremIpsum.length)));
        }
        else
        {
            if (participants.length && (participants[0].playingGame && participants[0].playingGame.joinable))
            {
                appendMultiplayerInvite();
            }
            else
            {
                // Emulate a single line message
                appendMessage('lol');
            }
        }

        window.setTimeout(receiveRandomMessage, randIntBetween(0, 60 * 1000));        
    }, this);

    this.sendMessage = function(body, threadId)
    {
        if (typeof threadId === "undefined")
        {
            threadId = this.generateUniqueThreadId();
        }

        if (body.match(/(stfu|shut (the fuck )?up|hush|shut your (mouth|face|piehole))/i))
        {
            console.log('Hushing ' + this.participants[0].nickname);
            // Stop sending events
            hushedUserIds.push(this.participants[0].id);
        }
        else if (body.match(/let'?s play( a game)?/i))
        {
            window.setTimeout(function()
            {
                var user = this.participants[0];

                if (pokeInToJoinable[user.id]())
                {
                    window.setTimeout(function()
                    {
                        appendMultiplayerInvite();
                    }, randIntBetween(0, 1000));
                }
                else
                {
                    appendMessage("Sorry broski can't play right now");
                }
            }.bind(this), 500);
        }
        else if (body.match(/request me/i))
        {
            window.setTimeout(function()
            {
                var user = this.participants[0];

                if ((user.subscriptionState.direction === 'NONE') &&
                    !user.subscriptionState.pendingCurrentUserApproval &&
                    !user.subscriptionState.pendingContactApproval)
                {
                    user.subscriptionState = {
                        direction: 'NONE',
                        pendingContactApproval: false,
                        pendingCurrentUserApproval: true
                    };
                    user.subscriptionStateChanged.trigger();

                    user.addedToRoster.trigger();
                    originSocial.roster.contactAdded.trigger(user);
                }
                else
                {
                    appendMessage('lol wut u talking about brohammer?');
                }
            }.bind(this), randIntBetween(100, 300));
        }
        else if (body.match(/accept me/i))
        {
            window.setTimeout(function()
            {
                var user = this.participants[0];

                if ((user.subscriptionState.direction === 'NONE') &&
                    !user.subscriptionState.pendingCurrentUserApproval &&
                    user.subscriptionState.pendingContactApproval)
                {
                    simulateSubcriptionCreation[user.id]();
                }
                else
                {
                    appendMessage('u high right now Brohammed Ali? that make no sense lololol!');
                }
            }.bind(this), randIntBetween(100, 300));
        }
        else if (body.match(/start broadcast/i))
        {
            window.setTimeout(function()
            {
                var user = this.participants[0];
                if (user.playingGame && user.playingGame.broadcastUrl)
                {
                    appendMessage("Already broadcasting");
                }
                else
                {
                    var game = new PlayingGame("71067", true, "http://www.twitch.tv/phantoml0rd");
                    user.playingGame = game;

                    addNewEvent({
                        type: 'PRESENCE_CHANGE',
                        user: user,
                        playingGame: game
                    });
                }
            }.bind(this), 500);
        }
        else if (body.match(/stop broadcast/i))
        {
            window.setTimeout(function()
            {
                var user = this.participants[0];
                if (!user.playingGame.broadcastUrl)
                {
                    appendMessage("Not broadcasting");
                }
                else
                {
                    var game = new PlayingGame("71067", true, "");
                    user.playingGame = game;
                    addNewEvent({
                        type: 'PRESENCE_CHANGE',
                        user: user,
                        playingGame: user.playingGame
                    });
                }
            }.bind(this), 500);
        }
        else if (body.match(/call me/i))
        {
            if (type === "ONE_TO_ONE")
            {
                window.setTimeout(function()
                {
                    var user = this.participants[0];
                    console.log("call state = " + this.voiceCallState);
                    if (this.voiceCallState !== "VOICECALL_DISCONNECTED")
                    {
                        appendMessage("I can't right now");
                    }
                    else
                    {
                        this.voiceCallState = "VOICECALL_INCOMING";
                        addNewEvent({
                            type: 'VOICE_REMOTE_CALLING',
                            from: user
                        });
                    }
                }.bind(this), 500);
            }
        }
        else if (body.match(/answer/i))
        {
            if (type === "ONE_TO_ONE")
            {
                window.setTimeout(function()
                {
                    var user = this.participants[0];
                    if (this.voiceCallState === "VOICECALL_OUTGOING")
                    {
                        this.voiceCallState = "VOICECALL_CONNECTED";
                        addNewEvent({
                            type: 'VOICE_STARTED',
                            from: user
                        });
                    }
                    else
                    {
                        appendMessage("Call me first");
                    }
                }.bind(this), 500);
            }
        }

        addNewEvent({
            type: 'MESSAGE',
            body: body,
            threadId: threadId,
            from: connectedUser,
            state: 'active'
        });
    };

    this.sendComposing = function() {};
    this.sendPaused = function() {};
    this.sendStopComposing = function() {};

    changeThreadId();

    if (Math.random() > 0.5)
    {
        // Receive a message right away
        receiveRandomMessage();
    }
    else
    {
        // Receive a message in a bit
        window.setTimeout(receiveRandomMessage, randIntBetween(0, 30 * 1000));
    }
    
    if (type === 'MULTI_USER')
    {
        window.setTimeout(changeParticipants, randIntBetween(0, 60 * 1000));
        window.setTimeout(changeVoiceParticipants, randIntBetween(0, 10 * 1000));
    }
    window.setTimeout(changeVoiceParticipantsTalkingStatus, randIntBetween(0, 1 * 1000));
    window.setTimeout(changeVoiceParticipantsRemoteMuteStatus, randIntBetween(0, 10 * 1000));
};

Conversation.prototype.generateUniqueThreadId = function()
{
    return Math.floor(Math.random() * Math.pow(2, 40)).toString(16);
};


var OriginChat = function()
{
    var showTimestamps = true;

    this.getConversationById = function(id)
    {
        return conversationMap[id];
    };

    this.showTimestampsChanged = new Connector;

    this.multiUserChatCapabilityChanged = new Connector;
    originSocial.currentUser.visibilityChanged.connect(function()
    {
        this.multiUserChatCapabilityChanged.trigger();
    }.bind(this));

    this.__defineGetter__('conversations', function()
    {
        var key, ret = [];

        if (conversationWindow && conversationWindow.closed)
        {
            // No active conversations
            conversationMap = {};
            return [];
        }

        for(key in conversationMap)
        {
            if (conversationMap.hasOwnProperty(key))
            {
                ret.push(conversationMap[key]);
            }
        }

        return ret;
    });

    this.__defineGetter__('showTimestamps', function()
    {
        return showTimestamps;
    });

    // The client API defines this as read only but give it a setter for testing
    this.__defineSetter__('showTimestamps', function(show)
    {
        showTimestamps = show;
        this.showTimestampsChanged.trigger();
    });
}

OriginChat.prototype.__defineGetter__('maximumOutgoingMessageBytes', function()
{
    // This is what the client returns
    return 2048;
});

OriginChat.prototype.__defineGetter__('multiUserChatCapable', function()
{
    return originSocial.currentUser.visibility === Visibility.VISIBLE;
});


var GroupRoles =
{
    'superuser': 'superuser', // This user is the owner of the group
    'admin': 'admin', // This user is an admin of the group
    'member': 'member', // This is a normal user
};

var Group = function(data)
{
    // Channels that are in this group
    this.channels = data.channels;

    // The group name
    this.name = data.name;

    // Id
    this.id = data.id;

    // Status
    this.status = data.status;

    // Role
    this.role = data.role;

    // Group GUID
    this.groupGuid = data.groupGuid;
    
    // Invite group
    this.inviteGroup = data.inviteGroup;

    // Group members
    this.members = data.members;

    // Group admins
    this.admins = data.admins;
    
    // Group owners
    this.owners = data.owners;
        
    // Emitted when the member list is finished updating
    this.membersLoadFinished = new Connector();
    
    // Emitted if a room entry fails
    this.enterMucRoomFailed = new Connector();
    
    // Emitted when a room rejoin is successful
    this.rejoinMucRoomSuccess = new Connector();

    // Emitted when the member list is updated
    this.groupMembersChange = new Connector();
    
    // Emitted any time a social user is changed in this group
    this.anyChange = new Connector();

    this.getChannelPresence = function()
    {

    }
    //ommitted from normalized-dev when changed in chat-group.js
    this.getGroupChannelInformation = function()
    {
	    
    }
    
    this.isUserInGroup  = function(user)
    {
        var x = randIntBetween(0, 1);
        return !!x;
    }
    
    this.getRemoteUserRole = function(user)
    {
        return "admin";
    }
    
    this.acceptGroupInvite = function()
    {
        this.inviteGroup = false;
    }
    
    this.rejectGroupInvite = function()
    {
        console.log(this);
    }
    
    this.invitedBy = data.invitedBy;
}

Group.prototype.getChannelPresence = function()
{
    // KEVIN - FIX THIS :-)
}

var Channel = function(data)
{
    // The channel name
    this.name = data.name;

    // If the channel is locked
    this.locked = data.locked;

    // Channel password
    this.password = data.password;

    // Id
    this.id = data.id;

    // Status
    this.status = data.status;

    // Admin
    this.admin = data.admin;

    // Current users
    this.channelMembers = data.channelMembers;

    this.getChannelPresence = function()
    {

    }
}

Channel.prototype.joinChannel = function()
{
    var conversation,
        participants = [],
        group,
        i=0;

    // Create some fake participants
    for (var x = 0; x< Math.min(64, RemoteUsers.length); x++)
    {
        participants[x] = RemoteUsers[x];
    }

    conversation = new Conversation(participants, "MULTI_USER");
    
    group = originSocial.groupList.groupForChannel(this);
    conversation.groupName = group.name;
    conversation.groupGuid = group.groupGuid;
    conversation.channelName = this.name;
    conversation.chatChannel = this;
    conversationMap[conversation.id] = conversation;

    openConversation(conversation.id);

    // Simulate what the client does and sent join events for each participant
    participants.forEach(function(user)
    {
        conversation.addNewEvent({
            type: 'PARTICIPANT_ENTER',
            user: user,
            nickname: user.nickname
        });
    });
}

Channel.prototype.rejoinChannel = function()
{
    alert("rejoin Channel");
};


// Create Groups
Groups = [
    new Group(
    {
        name : 'Super Duper Group',
        id : '0',
        groupGuid: '1234-567890-1234-1234-1234',
        status : '3 in chat, 1 voice chat',
        inviteGroup : false,
        role : GroupRoles.superuser,
        channels : [
            new Channel(
            {
                name : 'Lobby',
                locked : false,
                password : '',
                id : '0',
                status : '3 in chat, 1 voice chat',
                admin: true,
                channelMembers : [RemoteUsers[1],RemoteUsers[3], RemoteUsers[4], RemoteUsers[5],RemoteUsers[2], connectedUser]
            }),
            new Channel(
            {
                name : 'Raid',
                locked : false,
                password : '',
                id : '1',
                status : '1 in chat',
                admin: true
            }),
            new Channel(
            {
                name : 'Music',
                locked : true,
                password : 'lyrics',
                id : '2',
                status : '1 in chat',
                admin: true
            })
        ],
        admins: [],
        members: [
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '6',
                nickname: 'Jango',
                realName: {firstName: 'Silent', lastName: 'Dee'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '7',
                nickname: 'Banshee',
                realName: {firstName: 'Sibohan', lastName: 'Eire'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '8',
                nickname: 'Licorice',
                realName: {firstName: 'Salty', lastName: 'Sweets'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '9',
                nickname: 'Ozone',
                realName: {firstName: 'Firster', lastName: 'Laster'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '10',
                nickname: 'Shasta',
                realName: {firstName: 'Mountain', lastName: 'Lion'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '11',
                nickname: 'MeatyOne',
                realName: {firstName: 'Mountain', lastName: 'Lion'},
                owns: [],
            })
        ],        
        owners: [
            new RemoteUser(
                {
                    avatarUrl : 'https://lh3.googleusercontent.com/-l6hhlM_4x1A/AAAAAAAAAAI/AAAAAAAAAAA/6NTKD3n-Wug/s48-c-k/photo.jpg',
                    id: '1',
                    nickname: 'Xenia3',
                    realName: 'foo xen',
                    owns: [],
                    subscriptionState: {pendingContactApproval: false, pendingCurrentUserApproval: false, direction: 'NONE'},
                    delayMetadataBy: randIntBetween(100, 200)
                }),
            new RemoteUser(   {
                avatarUrl : 'https://lh4.googleusercontent.com/-W5UHZpvxKDI/AAAAAAAAAAI/AAAAAAAAAAA/Os1BlLFcptI/s48-c-k/photo.jpg',
                id: '2',
                nickname: 'Super User A',
                realName: {firstName: '<script>alert("0wned!")</script>', lastName: '<script>console.log("LOL");</script>'},
                owns: [],
            }),
            new RemoteUser(   {
                avatarUrl : 'https://lh3.googleusercontent.com/-Y86IN-vEObo/AAAAAAAAAAI/AAAAAAAAAAA/xOvnPfMX1XU/s48-c-k/photo.jpg',
                id: '4',
                nickname: 'Super User B',
                realName: {firstName: '<script>alert("0wned!")</script>', lastName: '<script>console.log("LOL");</script>'},
                owns: [],
            })
        ]
    }),
    new Group(
    {
        name : 'Friday Night',
        id : '1',
        groupGuid: '1234-567890-1234-1234-1234',
        status : '0 in chat',
        inviteGroup : false,
        role : GroupRoles.admin,
        channels : [
            new Channel(
            {
                name : 'Lobby',
                locked : false,
                password : '',
                id : '3',
                admin: true
            })
        ],
        owners: [],
        admins: [],        
        members: []
    }),
    new Group(
    {
        name : 'Origin Team',
        id : '2',
        groupGuid: '1234-567890-1234-1234-1234',
        status : '10 in chat',
        inviteGroup : false,
        role : GroupRoles.member,
        channels : [
            new Channel(
            {
                name : 'Lobby',
                locked : false,
                password : '',
                id : '4',
                admin: false
            })
        ]
    }),
    new Group(
    {
        name : 'Invite Group',
        invitedBy: { nickname: "Joe Cool" },
        id : '3',
        groupGuid: '1234-567890-1234-1234-1234',
        status : '',
        inviteGroup : true,
        role : GroupRoles.member,
        channels : [
            new Channel(
            {
                name : 'Lobby',
                locked : false,
                password : '',
                id : '4',
                admin: false
            })
        ]
    }),
    new Group(
    {
        name : 'An Invite Group 2',
        invitedBy: { nickname: "Username" },
        id : '4',
        groupGuid: '1234-567890-1234-1234-1234',
        status : '',
        inviteGroup : true,
        role : GroupRoles.member,
        channels : [
            new Channel(
            {
                name : 'Lobby',
                locked : false,
                password : '',
                id : '4',
                admin: false
            })
        ]
    })
];


//create extra users.
var startId = Groups[0].members.length
var alphabet=["a","b","c","d","e","f","g","h","h","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"];
var extraUsers=2000;
for(var i = 0;i < extraUsers;i++) {
    var ran = Math.floor(Math.random()*alphabet.length);
    var aName = alphabet[ran] + "_name_"+i;
    var id = startId+i;//(1234+(111*i));
    var role =(i % 500 == 0)?"admin":"member";

    var newmember = new RemoteUser(
                {
                    id: ""+id,
                    nickname: aName,
                    realName: "foo bar",
                    owns: [],
                    availability: Availability.ONLINE,
                    playingGame: null,
                    subscriptionState: {pendingContactApproval: false, pendingCurrentUserApproval: false, direction: 'NONE'},
                    delayMetadataBy: randIntBetween(100, 200)
                });
    if (role==="member") {    
        Groups[0].members.push(newmember);
    } else {
        Groups[0].admins.push(newmember);
    }
    ExtraMembers.push(newmember);
};
    
var ProductQuery = function()
{
    this.startQuery = function(productIds)
    {
        var multiQuery, delayMs, ProductQueryOperation, queryOp;

        if (!(productIds instanceof Array))
        {
            // Array-ify the product IDs
            productIds = [productIds];
            multiQuery = false;
        }
        else
        {
            multiQuery = true;
        }

        // Should we return "instantly"?
        if (!!randIntBetween(0, 1))
        {
            delayMs = 0;
        }
        else
        {
            delayMs = randIntBetween(100, 1000);
        }

        // Make our query operation
        // Use a stub class so the correct class name appears in the console
        ProductQueryOperation = function()
        {
            this.succeeded = new Connector;
            this.failed = new Connector;
        };

        queryOp = new ProductQueryOperation;

        window.setTimeout(function()
        {
            var i, productData = [];

            for(i = 0; i < productIds.length; i++)
            {
                if (!ExampleProductDatabase.hasOwnProperty(productIds[i]))
                {
                    // Can't look up
                    queryOp.failed.trigger(productIds);
                    return;
                }

                productData.push(ExampleProductDatabase[productIds[i]]);
            }

            if (multiQuery)
            {
                // We were queried with an array
                queryOp.succeeded.trigger(productData);
            }
            else
            {
                // We were queried with a single item
                queryOp.succeeded.trigger(productData[0]);
            }
        }, delayMs);

        return queryOp;
    };

    this.userOwnsProductId = function(productId)
    {
        return ['dragonage_na', '71308'].indexOf(productId) !== -1;
    };

    this.userOwnsMultiplayerId = this.userOwnsProductId;
}


var GroupMembersWindow = function()
{
    this.refresh = new Connector();
    this.callQueryGroupMembers = function() {};
    return this;
}

var FriendInviteWindow = function()
{
    this.dialogType = 0; // Group
    this.refresh = new Connector();
    this.callQueryGroupMembers = function() {};
    this.inviteUsersToGame = function(users) { alert("Users invited to game! Count: " + users.length) };
    this.inviteUsersToGroup = function(users, guid) { alert("Users invited to group: " + guid + " Count: " + users.length) };
    return this;
}


/**
 * CLASS: OriginVoice
 * 
 */
var OriginVoice = function()
{
    var v=0;

    /*
    this.deviceAdded = new Connector();
    this.deviceRemoved = new Connector();
    this.audioInputDevices = [];
    this.audioOutputDevices = [];
    */

    this.voiceLevel = new Connector();
    this.overThreshold = new Connector();
    this.underThreshold = new Connector();

    this.supported = true;

    this.networkQuality = 3;

    // Simulate voice input level changing
    setInterval(function()
    {
        v -= .1;
        if (v < -.1){ v = Math.random(); }
        window.originVoice.voiceLevel.trigger(v * 100);
    }, 50);

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

//
// Since we attach our fake client proxy objects to the window object,
// every window would normally get a seperate instance of each of the proxy objects
// which means that changing something in the roster for instance wouldn't have
// any effect on the chat window, etc. We want a common data source between all social
// windows to better simulate the actual client. Here we use a hack to pass the proxy
// objects from one window to the next. It's not perfect, but as long as you keep the
// initial window open it seems to work.
// Chrome requires that you set the following command line arguments for this to work:
//      --allow-file-access-from-files --disable-web-security
// While Safari and Firefox seem to work properly without any special treatment.
//
try {
    if (window.opener && (typeof window.opener!=="undefined") && (window.name==="chat-window"))
    {
        window.originSocial = window.opener.originSocial;
        window.dateFormat = window.opener.dateFormat;
        window.onlineStatus = window.opener.onlineStatus;
        window.clientNavigation = window.opener.clientNavigation;
        window.originChat = window.opener.originChat;
        window.productQuery = window.opener.productQuery;
        window.audioPlayer = window.opener.audioPlayer;
        window.originVoice = window.opener.originVoice;
        window.clientSettings = window.opener.clientSettings;

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

        return;
    }

    if (window.opener && (typeof window.opener!=="undefined") && ((window.name==="group-members-window") || (window.name==="invite-friends-window")))
    {
        window.groupMembersWindow = window.opener.groupMembersWindow;
        window.friendInviteWindow = window.opener.friendInviteWindow;
        window.originSocial = window.opener.originSocial;
        window.dateFormat = window.opener.dateFormat;
        window.onlineStatus = window.opener.onlineStatus;
        window.clientNavigation = window.opener.clientNavigation;
        window.originChat = window.opener.originChat;
        window.productQuery = window.opener.productQuery;
        window.audioPlayer = window.opener.audioPlayer;
        window.originVoice = window.opener.originVoice;
        window.clientSettings = window.opener.clientSettings;

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

        return;
    }

    if (window.parent.name === "chat-window")
    {
        window.originSocial = window.parent.opener.originSocial;
        window.dateFormat = window.parent.opener.dateFormat;
        window.onlineStatus = window.parent.opener.onlineStatus;
        window.clientNavigation = window.parent.opener.clientNavigation;
        window.originChat = window.parent.opener.originChat;
        window.productQuery = window.parent.opener.productQuery;
        window.audioPlayer = window.parent.opener.audioPlayer;
        window.originVoice = window.parent.opener.originVoice;
        window.clientSettings = window.parent.opener.clientSettings;

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

        return;
    }

} catch(err) {
    // If the above throws an error, continue through and re-initialize variables
    console.log("Unable to set window variables from window.opener, Check security flags such as --disable-web-security on Chrome. Error:" + err);
}

// Expose to the world
window.dateFormat = new DateFormatter();

// Create the Singleton
window.onlineStatus = new OnlineStatus();

/**
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


// Create the Singleton, and expose to the world
window.clientNavigation = new ClientNavigator();

window.audioPlayer = new AudioPlayer();

window.originSocial = new OriginSocial();

window.originChat = new OriginChat();

window.productQuery = new ProductQuery;

window.groupMembersWindow = new GroupMembersWindow;
window.friendInviteWindow = new FriendInviteWindow;

})(jQuery);
