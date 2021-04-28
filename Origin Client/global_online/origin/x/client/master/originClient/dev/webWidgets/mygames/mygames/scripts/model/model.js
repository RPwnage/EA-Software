;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }

// Constants
var DEFAULT_STATUS_FILTER = 'all';
var DEFAULT_PLATFORM_FILTER = 'all';
var ONE_DAY_MS = 1000*60*60*24;

// private members
var initialized = false;
var games = [];
var statusFilters =
{
    'all': tr('ebisu_client_all'),
    'played_this_week': tr('ebisu_client_played_this_week'),
    'favorites': tr('ebisu_client_favorites'),
    'downloading': tr('ebisu_client_downloading'),
    'hidden': tr('ebisu_client_hidden_games'),
    'installed': tr('ebisu_client_installed_games'),
    'non_origin_games': tr('ebisu_client_non_origin_games'),
    'vault': tr('Subs_Library_Filter_Vault'),
    'purchased': tr('subs_ebisu_client_purchased_games')
};

var platformFilters =
{
    'all': tr('ebisu_client_all_platforms'),
    'pc': tr('ebisu_client_PC'),
    'mac': tr('ebisu_client_mac')
};

var searchFilter = '';

var Model = function()
{
    this.gameInitEvent = $.Callbacks(); // Signal gets triggered when the entitlement is first retrieved and added wrapped in Game - used when initializing with one entitlement
    this.gamesInitEvent = $.Callbacks(); // Signal gets triggered when the entitlements are first retrieved and added wrapped in Games to the array of games - used when initializing with multiple entitlements
    this.gamesChangeEvent = $.Callbacks(); // Signal gets triggered when sorting or filtering changes the order or items in the array of games
    this.gamesResizeEvent = $.Callbacks(); // Signal gets triggered when the games are resized
    this.gameAddEvent = $.Callbacks(); // Signal gets triggered when an entitlement gets added and added wrapped in Games to the array of games
    this.gameUpdateEvent = $.Callbacks(); // Signal gets triggered when an entitlement gets updated
    this.gameRemoveEvent = $.Callbacks();

    this.gameExpansionAddEvent = $.Callbacks(); 
    this.gameExpansionUpdateEvent = $.Callbacks();
    this.gameExpansionRemoveEvent = $.Callbacks(); 

    this.gameAddonAddEvent = $.Callbacks();
    this.gameAddonUpdateEvent = $.Callbacks();
    this.gameAddonRemoveEvent = $.Callbacks();

    this.gameHideEvent = $.Callbacks(); // Signal gets triggered when an entitlement gets hidden / unhidden
    this.gameFavoriteEvent = $.Callbacks(); // Signal gets triggered when an entitlement gets favorited/unfavorited
    this.gamePlayEvent = $.Callbacks(); // Signal gets triggered when a game is played through the model
    this.gameCloudSavesCurrentUsageChangeEvent = $.Callbacks(); // Signal gets triggered when an entitlement's cloud saves current usage change event fires
    this.gameCloudSavesChangeEvent = $.Callbacks(); // Signal gets triggered when an entitlement's cloud saves change event fires
    this.refreshStartEvent = $.Callbacks(); // Signal gets triggered when EntitlementMagager has started refreshing
    this.refreshCompleteEvent = $.Callbacks(); // Signal gets triggered when EntitlementMagager is finished refreshing
    this.onlineStatusChangeEvent = $.Callbacks(); // Signal gets triggered when online state changes
    this.subscriptionChangeEvent = $.Callbacks(); // This signal is fired when the subscription is updated.
    this.subscriptionEntitlementRemoved = $.Callbacks(); // This signal is fired when the subscription game is removed.
};

// Returns the first game
Model.prototype.__defineGetter__('game', function()
{
    return games[0];
});

Model.prototype.__defineGetter__('games', function()
{
    return games;
});

Model.prototype.__defineGetter__('initialized', function()
{
    return initialized;
});

// Status Filter
Model.prototype.__defineGetter__('statusFilters', function()
{
    return statusFilters;
});
Model.prototype.__defineGetter__('statusFilter', function()
{
    // Retrieve from local storage
    return $.jStorage.get('games-status-filter') || 'all';
});
Model.prototype.__defineSetter__('statusFilter', function(val)
{
    if (!this.statusFilters[val])
    {
        console.log('Game.statusFilter:: ' + val + ' is not a valid status filter');
        return;
    }
    // Store in local storage
    $.jStorage.set('games-status-filter', val);
    this.updateGames();
});

// Platform Filter
Model.prototype.__defineGetter__('platformFilters', function()
{
    return platformFilters;
});
Model.prototype.__defineGetter__('platformFilter', function()
{
    // Retrieve from local storage
    return $.jStorage.get('games-platform-filter') || 'all';
});
Model.prototype.__defineSetter__('platformFilter', function(val)
{
    if (!this.platformFilters[val])
    {
        console.log('Game.platformFilter:: ' + val + ' is not a valid platform filter');
        return;
    }
    // Store in local storage
    $.jStorage.set('games-platform-filter', val);
    this.updateGames();
});

// Search Filter
Model.prototype.__defineGetter__('searchFilter', function()
{
    return searchFilter;
});
Model.prototype.__defineSetter__('searchFilter', function(val)
{
    val = val || '';
    if (val === searchFilter) { return; }
    searchFilter = val || '';
    this.updateGames();
});

// Zoom Factor
Model.prototype.__defineGetter__('zoomFactor', function()
{
    // Retrieve from local storage
    var zoomFactor = $.jStorage.get('games-zoom-factor');
    if (typeof(zoomFactor) === 'undefined' || zoomFactor === null)
    {
        zoomFactor = 1;
    }
    else
    {
        zoomFactor = Number(zoomFactor);
    }
    return zoomFactor;
});
Model.prototype.__defineSetter__('zoomFactor', function(val)
{
    // Store in local storage
    if (isNaN(val))
    {
        val = 1;
    }
    val = Number(val);
    $.jStorage.set('games-zoom-factor', val);
    this.gamesResizeEvent.fire(val);
});


Model.prototype.init = function(id)
{
    if (this.initialized)
    {
        console.log('Origin.model.init:: Already initialized - exiting.');
        return;
    }

    if (id)
    {
        var entitlement = entitlementManager.getEntitlementById(id, true);
        if (!entitlement)
        {
            console.log('Model.init:: Error, entitlement with id "' + id + '" does not exist in the user\'s entitlements.');
            return;
        }
        
        initialized = true;
        
        var _game = this.addGame(entitlement);
        $.each(_game.expansions, $.proxy(function(idx, expansion)
        {
            expansion.changeEvent.add(this.onGameExpansionChange.delegate(this));
        }, this));
        $.each(_game.addons, $.proxy(function(idx, addon)
        {
            addon.changeEvent.add(this.onGameAddonChange.delegate(this));
        }, this));
            
        _game.addonAddEvent.add(this.onGameAddonAdd.delegate(this));
        _game.addonRemoveEvent.add(this.onGameAddonRemove.delegate(this));

        _game.expansionAddEvent.add(this.onGameExpansionAdd.delegate(this));
        _game.expansionRemoveEvent.add(this.onGameExpansionRemove.delegate(this));
        
        // Bind to online status signal
        onlineStatus.onlineStateChanged.connect(this.onOnlineStatusChanged.delegate(this));
        
        // Bind to the subscription manager update signal
        subscriptionManager.updated.connect(this.onSubscriptionChange.delegate(this));
        subscriptionManager.entitlementUpgraded.connect(this.onSubscriptionChange.delegate(this));
        subscriptionManager.entitlementRemoved.connect(this.onSubscriptionChange.delegate(this));
        
        // Trigger the gameInit signal
        this.gameInitEvent.fire(this.games[0]);
    }
    else
    {
        // Add each game
        $.each(entitlementManager.topLevelEntitlements, function(idx, entitlement)
        {
            this.addGame(entitlement);
        }.delegate(this));
        
        // Bind to online status signal
        onlineStatus.onlineStateChanged.connect(this.onOnlineStatusChanged.delegate(this));
        
        // Bind to the subscription manager update signal
        subscriptionManager.updated.connect(this.onSubscriptionChange.delegate(this));
        subscriptionManager.entitlementUpgraded.connect(this.onSubscriptionChange.delegate(this));
        subscriptionManager.entitlementRemoved.connect(this.onSubscriptionChange.delegate(this));
        
        // Bind to signals
        entitlementManager.added.connect(this.onEntitlementAdded.delegate(this));
        entitlementManager.removed.connect(this.onEntitlementRemoved.delegate(this));
        entitlementManager.refreshCompleted.connect(this.onRefreshComplete.delegate(this));
        entitlementManager.refreshStarted.connect(this.onRefreshStart.delegate(this));
        
        // Trigger the gamesInit signal
        this.gamesInitEvent.fire(this.getGames(), this.games.slice(0));

        // Check if we have no entitlements
        if (entitlementManager.initialRefreshCompleted)
        {
            this.checkForNoEntitlements();
        }
    }
};

Model.prototype.addGame = function(entitlement)
{
    if(!entitlement.id || Origin.model.getGameById(entitlement.id) != undefined) {return;}
    var game = new Origin.Game(entitlement, true);
    games.push(game);
    game.changeEvent.add(this.onGameChange.delegate(this));
    game.hideEvent.add(this.onGameHideChange.delegate(this));
    game.favoriteEvent.add(this.onGameFavoriteChange.delegate(this));
    game.playEvent.add(this.onGamePlay.delegate(this));
    game.cloudSavesChangeEvent.add(this.onGameCloudSavesChange.delegate(this));
    game.cloudSavesCurrentUsageChangeEvent.add(this.onGameCloudSavesCurrentUsageChange.delegate(this));
    return game;
};

Model.prototype.refreshGames = function()
{
    entitlementManager.refresh();
};

Model.prototype.getGameById = function(id)
{
    if (!id)
    {
        console.log('Model.getGameById:: id is a required parameter');
        return null;
    }
    var gameTile;
    $.each(games, function(idx, game)
    {
        if (game.entitlement.id.toString() === id.toString())
        {
            gameTile = game;
            return false;
        }
    });
    return gameTile;
};

Model.prototype.getGames = function(statusFilter)
{
    // Make a copy of the games list
    var filteredGames = games.slice(0);
    
    if (typeof statusFilter === 'undefined')
    {
        statusFilter = this.statusFilter;
    }

    filteredGames = filteredGames.filter(function(game, idx, games)
    {
        return game.filterGroups.indexOf(statusFilter) !== -1;
    });

    // Platform Filter
    switch (this.platformFilter)
    {
        case ('pc'):
        {
            filteredGames = filteredGames.filter(function(game, idx, games)
            {
                return game.platformCompatibleStatus('PC') === 'COMPATIBLE';
            });
            this.sortByTitle(filteredGames);            
            break;
        }
        case ('mac'):
        {
            filteredGames = filteredGames.filter(function(game, idx, games)
            {
                return game.platformCompatibleStatus('MAC') === 'COMPATIBLE';
            });
            this.sortByTitle(filteredGames);
            break;
        }
        case ('all'):
        default:
        {
            filteredGames = this.sortByPlatform(Origin.views.currentPlatform(), filteredGames);    
            break;
        }
    }
    
    if (searchFilter)
    {
        var normalizeText = function(str)
        {
            // Remove all punctuation and replace multiple whitespace with a single space
            return str.replace(XRegExp('[\\p{Punctuation}~`=\\$\\<\\>]', 'g'), ' ').replace(/\s{2,}/g, ' ');
        };

        var searchExpressions = [];
        $.each(normalizeText(searchFilter).split(' '), function(idx, part)
        {
            // The .split function can sometimes return an empty string
            if (part.length>0) 
            {
                // The \b "word boundary" flag only works for ASCII characters                 
                if (XRegExp('^[a-z0-9]+$', 'i').test(part)) 
                {
                    searchExpressions.push(new XRegExp('\\b' + part, 'i'));
                }
                else if (XRegExp('^[\\p{Latin}\\p{Common}]+$').test(part)) 
                {
                    // For European characters outside ASCII, we can use the following to match word boundary
                    // More languages could be specifically added to the above 'else' clause if necessary
                    searchExpressions.push(new XRegExp('(?:^|\\s)' + part, 'i'));
                }
                else 
                {
                    // Ignore word boundary search, may not work and depending on language may not be relevant
                    searchExpressions.push(new XRegExp(part, 'i'));
                }
            }
        });
        filteredGames = filteredGames.filter(function(game, idx)
        {
            var passes = true;
            var normalizedTitle = normalizeText(game.entitlement.title);
            $.each(searchExpressions, function(i, searchExp)
            {
                return (passes = searchExp.test(normalizedTitle));
            });
            return passes;
        });
    }
    return filteredGames;
};

Model.prototype.getExpansions = function(parent)
{
    // Make a copy of the expansions list
    var filteredExpansions = parent.expansions.slice(0);

    // Platform Filter
    switch (this.platformFilter)
    {
        case ('pc'):
        {
            filteredExpansions = filteredExpansions.filter(function(expansion, idx, expansions)
            {
                return expansion.platformCompatibleStatus('PC') === 'COMPATIBLE';
            });            
            break;
        }
        case ('mac'):
        {
            filteredExpansions = filteredExpansions.filter(function(expansion, idx, expansions)
            {
                return expansion.platformCompatibleStatus('MAC') === 'COMPATIBLE';
            });
            break;
        }
        case ('all'):
        default:
        {
            break;
        }
    }

    return filteredExpansions;
}

Model.prototype.getAddons = function(parent)
{
    // Make a copy of the addons list
    var filteredAddons = parent.addons.slice(0);

    // Platform Filter
    switch (this.platformFilter)
    {
        case ('pc'):
        {
            filteredAddons = filteredAddons.filter(function(addon, idx, addons)
            {
                return addon.platformCompatibleStatus('PC') === 'COMPATIBLE';
            });            
            break;
        }
        case ('mac'):
        {
            filteredAddons = filteredAddons.filter(function(addon, idx, addons)
            {
                return addon.platformCompatibleStatus('MAC') === 'COMPATIBLE';
            });
            break;
        }
        case ('all'):
        default:
        {
            break;
        }
    }

    return filteredAddons;
};

Model.prototype.sortByPlatform = function(platform, filteredGames)
{
    var platformGames = [];
    var otherGames = [];

    $.each(filteredGames, function(idx, game)
    {
        if(game.platformCompatibleStatus(platform) === 'COMPATIBLE')
        {
            platformGames.push(game);
        }
        else
        {
            otherGames.push(game);
        }
    });

    platformGames = this.sortByTitle(platformGames);
    otherGames = this.sortByTitle(otherGames);
    
    return platformGames.concat(otherGames);
};

Model.prototype.sortByTitle = function(games)
{
    // Normalize the title
    var sortTitle = function(entitlement)
    {
        // First flatten the case out
        var title = entitlement.title.toLocaleLowerCase();
        
        // Remove everything after :
        // This makes game series sort in a more natural order
        title = title.replace(/:.*/, '');
        
        // The following regular expression matches these common articles at the beginning of the title only and removes them from sorting consideration
        // Articles in common languages
        // English:  a,an,the
        // German:   der,die,das,des,dem,den,ein,eine,einer,eines,einem,einen
        // Dutch:    de,het,een
        // Spanish:  el,la,lo,los,las,un,una,unos,unas
        // French:   le,la,l',les,un,une,des
        // Italian:  il,lo,la,i,gli,le,un',uno,una,un
        title = title.replace(/^(a|an|the|der|die|das|des|dem|den|ein|eine|einer|eines|einem|einen|de|het|een|el|la|lo|los|las|un|una|unos|unas|le|l'|les|une|il|i|gli|un'|uno)\s/, '');

        // Remove all punctuation
        title = title.replace(/[^\w\s]/g, '');

        // Simplify whitespace
        title = title.replace(/\s{2,}/g, ' ');

        // Trim any leading to trailing whitespace
        return title.trim();
    };

    function sortByTitle(a, b) 
    {
        var diff = sortTitle(a.entitlement).localeCompare(sortTitle(b.entitlement));
        
        // If two titles resolve to the same "sort title", compare their full titles as a fallback.
        if(diff === 0)
        {
            diff = a.entitlement.title.toLocaleLowerCase().localeCompare(b.entitlement.title.toLocaleLowerCase());
        }
        
        return diff;
    };

    // Sort by title
    games = games.sort(function(a, b)
    {
        return sortByTitle(a, b);
    });
    return games;
};

Model.prototype.getAllGames = function()
{
    return games;
};

// This gets called when the games list changes either by sorting or filtering
Model.prototype.updateGames = function()
{    
    this.gamesChangeEvent.fire(this.getGames());
};

Model.prototype.checkForNoEntitlements = function()
{
    if (games.length === 0)
    {
        var workingStore = onlineStatus.onlineState && originUser.commerceAllowed;
        window.location.href = workingStore ? './nogames-store.html' : './nogames-nostore.html';
        return true;
    }

    return false;
};


/*************************
    EVENTS */

Model.prototype.onOnlineStatusChanged = function()
{
    this.onlineStatusChangeEvent.fire();
//    if(onlineStatus.onlineState)
//    {
//        $feedback.removeClass('disabled');
//    }
//    else
//    {
//        $feedback.addClass('disabled');
//    }
};

Model.prototype.onEntitlementAdded = function(entitlement)
{
    var game = this.addGame(entitlement);
    this.gameAddEvent.fire(game);
};

Model.prototype.onEntitlementRemoved = function(entitlement)
{
    $.each(games, $.proxy(function(idx, game)
    {
        if (game.entitlement.id === entitlement.id)
        {
            games.splice(idx, 1);
            this.gameRemoveEvent.fire(game);
            return false;
        }
    }, this));
};

Model.prototype.onRefreshStart = function()
{
    this.refreshStartEvent.fire();
};

Model.prototype.onRefreshComplete = function()
{
    this.checkForNoEntitlements();
    this.refreshCompleteEvent.fire();
};

Model.prototype.onGameHideChange = function(game)
{
    this.gameHideEvent.fire(game);
};

Model.prototype.onSubscriptionChange = function(game)
{
    this.subscriptionChangeEvent.fire();
};

Model.prototype.onGameFavoriteChange = function(game)
{
    this.gameFavoriteEvent.fire(game);
};

Model.prototype.onGameChange = function(game, refilter)
{
    this.gameUpdateEvent.fire(game);
    if (refilter)
    {
        this.gamesChangeEvent.fire(this.getGames(), game);
    }
};

Model.prototype.onGameExpansionAdd = function(parent, expansion)
{
    expansion.changeEvent.add(this.onGameExpansionChange.delegate(this));
    this.gameExpansionAddEvent.fire(expansion);
};

Model.prototype.onGameExpansionChange = function(expansion)
{
    this.gameExpansionUpdateEvent.fire(expansion);
};

Model.prototype.onGameExpansionRemove = function(parent, expansion)
{
    this.gameExpansionRemoveEvent.fire(expansion);
};

Model.prototype.onGameAddonAdd = function(parent, addon)
{
    addon.changeEvent.add(this.onGameAddonChange.delegate(this));
    this.gameAddonAddEvent.fire(addon);
};

Model.prototype.onGameAddonChange = function(addon)
{
    this.gameAddonUpdateEvent.fire(addon);
};

Model.prototype.onGameAddonRemove = function(parent, addon)
{
    this.gameAddonRemoveEvent.fire(addon);
};

Model.prototype.onGamePlay = function(game)
{
    this.gamePlayEvent.fire(game);
};

Model.prototype.onGameCloudSavesCurrentUsageChange = function(game)
{
    this.gameCloudSavesCurrentUsageChangeEvent.fire(game);
};

Model.prototype.onGameCloudSavesChange = function(game, currentUsage)
{
    this.gameCloudSavesChangeEvent.fire(game, currentUsage);
};


// Create the Singleton
Origin.model = new Model();

})(jQuery);
