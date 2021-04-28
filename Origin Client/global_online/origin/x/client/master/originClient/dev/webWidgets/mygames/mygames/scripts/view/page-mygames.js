;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var DEFAULT_WIDTH = 231;
var DEFAULT_HEIGHT = 326;
var DEFAULT_MARGIN = 30;
var OTH_MAX_LAST_QUERY_TIME = clientSettings.readSetting("OverrideOnTheHouseQueryIntervalMS");
//Prepare a script to inject in to the iframe that will send a message to the window when the document is ready
var OTH_READY_SCRIPT = "<script>$(document).ready(function(){window.parent.postMessage({'type': 'ready'}, '*');});</script>";
var OTH_OVERRIDE = window.clientSettings.readSetting("OnTheHouseOverridePromoURL");
var MyGamesView = function()
{
	// Clear out the games search. At this point the searchFilter is "". 
	// We have to do this super early so the user won't see it. This fixes EBIBUGS-18207.
	$('input[name="games-search"]').val("");
	
    // Bind to model signals
    Origin.model.gamesInitEvent.add($.proxy(this, 'onGamesInitialized'));
    Origin.model.gamesChangeEvent.add($.proxy(this, 'onGamesChanged'));
    Origin.model.gamesResizeEvent.add($.proxy(this, 'onGamesResized'));
    Origin.model.gameAddEvent.add($.proxy(this, 'onGameAdded'));
    Origin.model.gameRemoveEvent.add($.proxy(this, 'onGameRemoved'));
    Origin.model.gameHideEvent.add($.proxy(this, 'onGameHideChange'));
    Origin.model.gameFavoriteEvent.add($.proxy(this, 'onGameFavoriteChange'));
    Origin.model.subscriptionChangeEvent.add($.proxy(this, 'onSubscriptionChange'));
    Origin.model.gamePlayEvent.add($.proxy(this, 'onGamePlay'));
    Origin.model.refreshStartEvent.add($.proxy(this, 'onRefreshStart'));
    Origin.model.refreshCompleteEvent.add($.proxy(this, 'onRefreshComplete'));
    Origin.model.onlineStatusChangeEvent.add($.proxy(this, 'onOnlineStatusChange'));
    Origin.model.gameCloudSavesChangeEvent.add($.proxy(this, 'onGameUpdated'));
    onTheHouseQuery.succeeded.connect($.proxy(this, 'onOtHSuccessful'));
    onTheHouseQuery.failed.connect($.proxy(this, 'onOtHFailure'));

    if (window.originSocial)
    {
        originSocial.roster.changed.connect($.proxy(this, 'updateHovercardContacts'));
    }
    
    // CSV used to compare current elms with new elms when filtering or sorting
    this._visibleEntitlementIds = null;
    this._activeAnimation = 0;
    this._filterUpdateTimer = 0;
    this._isPlayLaunching = false;
    this._playLaunchTimer = null;
    this._previousActualTilesPerRow = null;
    
    $('<span class="wait-screen"></span><span class="loader"></span>').appendTo($('#main')).on('mouseover mouseout click contextmenu', function(evt){ evt.stopImmediatePropagation(); });
    
    var $body = $(document.body);
    var $main = $('#main');
    
    // Add our platform attribute for the benefit of CSS
    $(document.body).attr('data-platform', Origin.views.currentPlatform());
    
    $( document ).ready(function() {
        var userIsWasSubscriber = subscriptionManager.firstSignupDate !== "";
        // ORSUBS-1678: only show vault and purchase filter if the user is/was a subscriber
        $('.dropdown-option[data-value="vault"]').toggle(userIsWasSubscriber);
        $('.dropdown-option[data-value="purchased"]').toggle(userIsWasSubscriber);
    });
    
    // Resize the main content area on resize of the browser to ensure that scrollbars are present when needed
    var mainPadding = $main.outerHeight(true) - $main.height();
    var bodyPadding = $body.outerHeight() - $body.height();
    var resizeInit = false;
    $(window).on('resize', $.proxy(function(evt)
    {
        $('#main').height(window.innerHeight - bodyPadding - mainPadding);
        if (resizeInit)
        {
            this.onGamesResized(Origin.model.zoomFactor);
        }
        resizeInit = true;
    }, this)).resize();
	
    $(window).blur(function()
    {
        OriginDropDown.get('game-status').hideOptionDropDown();
        OriginDropDown.get('game-platform').hideOptionDropDown();
    });
    
    $main.on('scroll', $.proxy(function(evt)
    {
        // Move the hovercard when scrolling
        Origin.HovercardManager.updatePosition(true);
    }, this));
    
    var $viewOptions = $('#view-options');
    
    $viewOptions.on('submit', function(evt){ evt.preventDefault(); return false; });
    
    // --------------------------
    // Refresh Games
    // Bind to the refresh click event
    $viewOptions.on('click', '#view-options > button.refresh', function(evt)
    {
        // This is to fix a strange bug where this event gets triggered when hitting "enter/return" while in the search field
        if (evt.clientX === 0 && evt.clientY === 0) { return; }
        
        Origin.model.refreshGames();
    });
    
    // Set the tooltip 
    var $refreshButton = $('#view-options > button.refresh');
    $refreshButton.attr('title', $refreshButton.text());

    // Refresh isn't possible when we're offline
    $refreshButton.prop('disabled', !onlineStatus.onlineState);

    // --------------------------
    // Game Status Filter
    // Build the options
    var selector = 'select[name="game-status"]';
    var $select = $viewOptions.find(selector);
    var select = $select[0];
    var selectedVal = Origin.model.statusFilter;
    // if the selected Value is subs and subscriptions are not available,
    // go back to the all view - should be an edge case
    //if (//subs.isEnabled() &&
    //    (selectedVal == 'vault_games' || selectedVal == 'owned_games')) 
    //{
    //    selectedVal = 'all';
    //}
    for (var val in Origin.model.statusFilters)
    {
        // Create the real option
        select.add(new Option(Origin.model.statusFilters[val], val, (val === selectedVal), (val === selectedVal)));
    }
    // Bind to the game-status (filter) change event
    $viewOptions.on('change', selector, function(evt)
    {
        Origin.model.statusFilter = $(this).val();
    });
    $select.dropdown();
    
    
    // --------------------------
    // Game Platform Filter
    // Build the options
    selector = 'select[name="game-platform"]';
    $select = $viewOptions.find(selector);
    select = $select[0];
    selectedVal = Origin.model.platformFilter;
    for (var val in Origin.model.platformFilters)
    {
        // Create the real option
        select.add(new Option(Origin.model.platformFilters[val], val, false, (val == selectedVal))); //.options[select.options.length]
    }
    // Bind to the game-platform (filter) change event
    $viewOptions.on('change', selector, function(evt)
    {
        Origin.model.platformFilter = $(this).val();
    });
    $select.dropdown();
    
    
    // --------------------------
    // Games Resizer
    // Bind to the resize slider change event
    $viewOptions.find('#size-control')[0].value = (Origin.model.zoomFactor * 100.0);
    $viewOptions.on('change', '#size-control', function(evt)
    {
        Origin.model.zoomFactor = (parseInt($(this).attr('value'), 10) / 100.0);
    });
    
    // --------------------------
    // Games Search
    // Bind to the search keyup event
    $viewOptions.on('keyup search', 'input[name="games-search"]', function(evt)
    {
        evt.stopImmediatePropagation();
        Origin.model.searchFilter = $(this).val();

        return false;
    });
    
    // --------------------------
    // Game Cards
    var $gameLibrary = $('#game-library');
    
    // Bind to the hover event
    $gameLibrary
        .on('hover', '#game-library > li.game', function(evt)
        {
            evt.stopImmediatePropagation();
        
            var $this = $(this);
            var id = $this.attr('data-id');
            if (!id) { return; }
            var $img = $this.find('> div.boxart-container > img');
            switch (evt.type)
            {
                case ('mouseenter'):
                {
                    if (!Origin.views.myGames._activeAnimation && !Origin.GameContextMenuManager.isVisible() && !Origin.views.myGames._isPlayLaunching)
                    {
                        Origin.HovercardManager.showDelayed($this, Origin.views.myGames, Origin.model.getGameById(id), id);
                    }
                    break;
                }
                case ('mouseleave'):
                {
                    Origin.HovercardManager.hideDelayed();
                    break;
                }
            }
        })
        .on('click', '#game-library > li.game', function(evt)
        {
            evt.stopImmediatePropagation();
            var $this = $(this);
            var id = $this.attr('data-id');
            if (!id) { return; }

            Origin.HovercardManager.show($this, Origin.views.myGames, Origin.model.getGameById(id), id);
        })
        .on('dblclick', '#game-library > li.game', function(evt)
        {
            evt.stopImmediatePropagation();
            var $this = $(this);
            var id = $this.attr('data-id');
            if (!id) { return; }

            var game = Origin.model.getGameById(id);
            game.performPrimaryAction("gametile");
        })
        .on('dragstart', '#game-library > li.game > div.boxart-container > img.boxart', function(evt)
        {
            // Don't let the user drag the box art images around
            evt.preventDefault();
        });
    
    // Bind to the right click of the game tile
    //$(document).bind('contextmenu',function(evt){ evt.preventDefault(); return false; });
    $gameLibrary.on('contextmenu', '#game-library > li.game', function(evt)
    {
        evt.stopImmediatePropagation();
        evt.preventDefault();
        
        var view = Origin.views.myGames;
        if (view._activeAnimation) { return; }
        
        // Hide the hovercard when the context menu is open
        Origin.HovercardManager.hide(true);
        
        var id = $(this).attr('data-id');

        Origin.GameContextMenuManager.popup(Origin.model.getGameById(id));
        
        $('li.game[data-id="' + id + '"]').addClass('selected');
    });
    
    // Bind to the progressbar of the game tile
    $gameLibrary.on('click', '#game-library > li.game > .otk-operation-controller-mini', function(evt)
    {
        evt.stopImmediatePropagation();
        //var game = Origin.model.getGameById($(this).parent().attr('data-id'));
        //game.gotoDetails();
        clientNavigation.showDownloadProgressDialog();
    });
    
    
    // --------------------------
    // Game Hovercard
    
    // Bind to the game hovercard favorite button
    $body.on('click', '.hovercard > .content > header > button', function(evt)
    {
        evt.stopImmediatePropagation();
        var $btn = $(this);
        var game = Origin.model.getGameById($btn.parents('.hovercard:first').attr('data-id'));
        game.isFavorite = !$btn.hasClass('favorite');
    });
    
    // Bind to the game hovercard footer action buttons
    $body.on('mouseup', '.hovercard > .content > footer > table.footer-table > tbody > tr > td > button', function(evt)
    {
        evt.stopImmediatePropagation();
        var $btn = $(this);
        var game = Origin.model.getGameById($btn.parents('.hovercard:first').attr('data-id'));
        Origin.util.handleHovercardButtonClick(game, $btn);
    });
    
    // Friends
    $body.tooltip({ selector: '.hovercard > .content > .info > .friends > ul > li' });
    
    $body.on('click', '.hovercard > .content > .info > .friends > ul > li', function(evt)
    {
        evt.stopImmediatePropagation();

        var contactId = $(this).attr('data-id');

        if (window.originSocial)
        {
            telemetryClient.sendProfileGameSource("HOVERCARD");
            originSocial.getRosterContactById(contactId).showProfile("MYGAMES");
        }
    });
    
	// Achievements
    $body.on('click', '.hovercard > .content > .info > .achievements > ul > li, .hovercard > .content > .info > .achievements > .link-view-achievements', function(evt)
    {
        evt.stopImmediatePropagation();

        var achievementSetId = $(this).attr('data-id');

        if (window.achievementManager)
        {
            var game = Origin.model.getGameById($(this).parents('.hovercard:first').attr('data-id'));
            telemetryClient.sendHovercardAchievementsClick(game.entitlement.productId);
            clientNavigation.showAchievementSetDetails(achievementSetId);
        }
    });	
	
	
	
    // ------------------
    // Notifications
    
    // Bind to the notification dismiss button
    $body.on('click', 'ul.notifications > li.dismissable > button', function(evt)
    {
        var $notification = $(this).parent();
        var notificationKey = $notification.attr('data-key');
        var game = Origin.model.getGameById($notification.parents('[data-id]:first').attr('data-id'));
        
        // Remove the notification
        $notification.slideUp('fast', function()
        {
            $(this).remove(); 

            if (notificationKey)
            {
                game.dismissNotification(notificationKey);
            }
        });
        
    });
    
    // Bind to the notification buttons
    $body.on('click', 'ul.notifications > li > .message > button', function(evt)
    {
        evt.stopImmediatePropagation();
        var $btn = $(this);
        var game = Origin.model.getGameById($btn.parents('[data-id]:first').attr('data-id'));
        
        if ($btn.parent().parent().hasClass('type-expires'))
        {
            if ($btn.hasClass('update'))
            {
                game.installUpdate();
            }
            else if ($btn.hasClass('download'))
            {
                this.startDownload();
            }
        }
        else
        {
            if ($btn.hasClass('download'))
            {
                game.startDownload();
            }
            else if ($btn.hasClass('play'))
            {
                game.play();
            }
            else if ($btn.hasClass('update'))
            {
                if (Origin.HovercardManager.isVisible(game.entitlement.id))
                    telemetryClient.sendHovercardUpdateClick(game.entitlement.productId);
                game.installUpdate();
            }
            else if ($btn.hasClass('view'))
            {
                game.gotoDetails();
            }
            else if ($btn.hasClass('hide-game'))
            {
                game.isHidden = true;
            }
            else if ($btn.hasClass('unhide-game'))
            {
                game.isHidden = false;
            }
            else if ($btn.hasClass('uninstall'))
            {
                game.uninstall();
            }
            else if ($btn.hasClass('buy'))
            {
                if( game.isTrialType )
                {
                    var expired = ((game.secondsUntilTermination === 0) ? '1' : '0');
                    telemetryClient.sendFreeTrialPurchase(game.entitlement.productId, expired, 'hovercard');
                }

                game.viewStoreMasterTitlePage();
            }
            else if ($btn.hasClass('trial-view-offer'))
            {
                var promoType = game.secondsUntilTermination === 0 ? "FreeTrialExpired" : "FreeTrialExited";
                var scope = "GameHovercard";                
                clientNavigation.showPromoManager(game.entitlement.productId, promoType, scope);
            }
            else if ($btn.hasClass('more-details'))
            {
                game.gotoDetails();
            }
            else if ($btn.hasClass('upgrade'))
            {
                game.entitlement.upgrade();
            }
            else if ($btn.hasClass('renew-subs-membership'))
            {
                telemetryClient.sendSubscriptionUpsell('hovercard');
                clientNavigation.showMyAccountPath("Subscription");
            }
            else if ($btn.hasClass('remove-entitlement'))
            {
                game.removeFromLibrary();
            }
            else if ($btn.hasClass('go-online'))
            {
                onlineStatus.requestOnlineMode();
            }
        }
    });
    
    // Fix up the placeholder text to be translated
    // Our dodgy translation system doesn't support translating attributes
    $('input[name="games-search"]').attr('placeholder', tr('ebisu_client_search'));

    // Hook up our hovercard show/hide to set our selected class
    // This is so we visually indicate which game the hovercard is associated with
    Origin.HovercardManager.showEvent.add(function(id)
    {
        $('li.game[data-id="' + id + '"]').addClass('selected');
    });

    Origin.HovercardManager.hideEvent.add(function()
    {
        $('li.game').removeClass('selected');
    });

    // Do the same for context menus
    Origin.GameContextMenuManager.hideEvent.add(function()
    {
        $('li.game').removeClass('selected');
    });

    // If we're simply refreshing the page, used the cached version and HTML.
    // If no cached value exists, this must be initial startup.
    if(window.sessionStorage.getItem('oth_cache_version') == null || window.sessionStorage.getItem('oth_cache_html') == null)
    {
        onTheHouseQuery.startQuery();
    }
    else
    {
        var version = window.sessionStorage.getItem('oth_cache_version');
        var html = window.sessionStorage.getItem('oth_cache_html');
        this.writeOtHOffer(version, html);
    }
        
    window.setInterval(function(){onTheHouseQuery.startQuery()}, OTH_MAX_LAST_QUERY_TIME);
};

MyGamesView.prototype.onGamePlay = function(game)
{
    if (this._playLaunchTimer) { window.clearTimeout(this._playLaunchTimer); }
    if (!this._isPlayLaunching)
    {
        this._isPlayLaunching = true;
        // Hide the hovercard when launching game
        Origin.HovercardManager.hide(true);
    }
    this._playLaunchTimer = window.setTimeout($.proxy(function(){ this._isPlayLaunching = false; }, this), 5000);
};

MyGamesView.prototype.onGamesInitialized = function(games, allGames)
{
    var $gameLibrary = $('#game-library').css({ visibility: 'visible' }).empty();
    
    // Make sure we hook up our updated event after the game tile HTML is added
    // Otherwise we can get an updated signal for a game that hasn't been added yet and crash
    Origin.model.gameUpdateEvent.add($.proxy(this, 'onGameUpdated'));

    this.updateVisibleEntitlements(true);
};

MyGamesView.prototype.onOnlineStatusChange = function()
{
    $('#view-options > button.refresh').prop('disabled', !onlineStatus.onlineState);

    Origin.model.getAllGames().forEach(function(game)
    {
        this.onGameUpdated(game);
    }, this);
};

// This event gets triggered when the games list has been filtered or sorted
MyGamesView.prototype.onGamesChanged = function(games)
{
    this.updateVisibleEntitlements(false);
}

MyGamesView.prototype.updateVisibleEntitlements = function(fromInit)
{
    var games = Origin.model.getGames();
    var previousEntitlementIds = this._visibleEntitlementIds;

    this._visibleEntitlementIds = games.map(function(game)
    {
        return game.entitlement.id;
    });

    // Disable empty filters
    var dropdown = OriginDropDown.get('game-status');
    for(var value in Origin.model.statusFilters)
    {
        // Don't ever disable 'all'
        if (value !== 'all')
        {
            if (Origin.model.getGames(value).length === 0)
            {
                dropdown.disableOption(value);
            }
            else
            {
                dropdown.enableOption(value);
            }
        }
    }

    if ((games.length === 0) && (Origin.model.statusFilter === 'hidden' || Origin.model.statusFilter === 'favorites'))
    {
        // We're out of games, switch to the all view
        $('select[name="game-status"]').val('all').change();
    }
    
    // Set the current elm ids for use in next games list change
    // For performance, let's only do layout if there is a
    // difference in the new games list and what is in the HTML
    if (previousEntitlementIds === null ||  
       (previousEntitlementIds.join(',') !== this._visibleEntitlementIds.join(',')))    
    {
        // Hide any visible hovercards while we are filtering (animating)
        Origin.HovercardManager.hide();

        // Create any new tiles we need
        var $newGameTiles = $(games.filter(function(game)
        {
            return $('#game-library > li[data-id="' + game.entitlement.id + '"]').length === 0;
        }).map(function(game)
        {
            return this.createGameTile(game);
        }, this));

        $newGameTiles.appendTo($('#game-library'));

        // Update all the progress bars
        $newGameTiles.each(function(idx, newTile)
        {
            var $progressBar = $(newTile).find('> .otk-operation-controller-mini');
            var game = Origin.model.getGameById($(newTile).attr('data-id'));
            Origin.util.updateGameProgressBar(game, $progressBar, false);
        });

        this.layoutTiles($newGameTiles, !fromInit, $.proxy(this.onLayoutComplete, this));
    }
};

MyGamesView.prototype.onLayoutComplete = function()
{
    var isContextMenuOpen = Origin.GameContextMenuManager.isVisible();
    var isPlayLaunching = this._isPlayLaunching;
    
    // Loop through current ids
    $.each(this._visibleEntitlementIds, function(idx, id)
    {
        var $game = $('#game-library > li[data-id="' + id + '"]');
        // If the mouse is over the element, show the hovercard
        if ($game.is(':hover'))
        {
            if (!isContextMenuOpen && !isPlayLaunching)
            {
                Origin.HovercardManager.show($game, Origin.views.myGames, Origin.model.getGameById(id), id);
            }
            else
            {
                Origin.HovercardManager.hide();
            }
            
            // break out of the loop
            return false;
        }
    });
};

// This event gets triggered when the games have been resized
MyGamesView.prototype.onGamesResized = function()
{
    if (this.resizeId) {
        window.clearTimeout(this.resizeId);
    }
    
    this.resizeId = window.setTimeout($.proxy(function() {
        this.layoutTiles($());
    }, this), 30);
    
};

// This event gets triggered when a game has been added
MyGamesView.prototype.onGameAdded = function(game)
{
// We need to check the filter status and type of game here to ensure
// that if a user adds a NOG and if filtering by NOG's the filter doesn't
// get reset on them.  This fixes https://developer.origin.com/support/browse/EBIBUGS-24478
    if (Origin.model.statusFilter === 'non_origin_games')
    {
        if(!game.isNonOriginGame)
        {
            $('select[name="game-status"]').val('all').change();
        }
    }
    else
    {
        $('select[name="game-status"]').val('all').change();
    }
    this.updateVisibleEntitlements();
};

// This event gets triggered when a game's data or state has been updated
MyGamesView.prototype.onGameUpdated = function(game)
{
    var $game = $('#game-library > li[data-id="' + game.entitlement.id + '"]'); 

    if ($game.length > 0)
    {
        this.updateGame($game, game);
    }

    // Do a delayed update of filters if one isn't queued
    if (!this._filterUpdateTimer)
    {
        this._filterUpdateTimer = setTimeout($.proxy(function() 
        {
            this._filterUpdateTimer = 0;
            this.updateVisibleEntitlements();
        }, this), 500);
    }
};

// This event gets triggered when a game has been removed
MyGamesView.prototype.onGameRemoved = function(game)
{
    this.updateVisibleEntitlements();
};

// This event gets triggered when a game is hidden or unhidden
MyGamesView.prototype.onGameHideChange = function(game)
{
    this.updateVisibleEntitlements();
};

// This event gets triggered when a game is favorited or unfavorited
MyGamesView.prototype.onGameFavoriteChange = function(game)
{
    this.updateVisibleEntitlements();
};

// This event gets triggered when a user's subscription status changes
MyGamesView.prototype.onSubscriptionChange = function()
{
    var userIsWasSubscriber = subscriptionManager.firstSignupDate !== "";
    this.updateVisibleEntitlements();

    // ORSUBS-1678: only show vault and purchase filter if the user is/was a subscriber
    $('.dropdown-option[data-value="vault"]').toggle(userIsWasSubscriber);
    $('.dropdown-option[data-value="purchased"]').toggle(userIsWasSubscriber);
};

MyGamesView.prototype.onRefreshStart = function()
{
    // Add dimmed state with animated spinner
    $(document.body).addClass('wait');
};

MyGamesView.prototype.onRefreshComplete = function()
{
    $(document.body).removeAttr('platform', Origin.views.currentPlatform());    
    $(document.body).removeClass('wait');
};

MyGamesView.prototype.updateHovercardContacts = function()
{
    Origin.model.getGames().forEach(function(game)
    {
        // If there the game's hovercard is visible, update it
        if (Origin.HovercardManager.isVisible(game.entitlement.id))
        {
            Origin.HovercardManager.updateHtml(this.getHovercardHtml(game));
            return false;
        }
    }, this);
};

MyGamesView.prototype.createGameTile = function(game)
{
    var $game = $(Origin.views.gameTile.getGameHtml(game));
    
    // Add a title tag that's used for automation
    // Do it here instead of in the HTML string so we don't have to escape it
    $game.attr("data-title", game.entitlement.title);
        
    // Add progreess bar widget
    var $progressContainer = $.otkOperationControllerMiniCreate($game.find('> .otk-operation-controller-mini'), {showButtons:false});
    
    // Add image onerror event to revert to default image
    $game.find('> div.boxart-container > img.boxart').on('error', function(evt)
    {
        if ($(this).attr('src') !== Origin.views.gameTile.DEFAULT_BOXART_IMAGE)
        {
            Origin.BadImageCache.urlFailed($(this).attr('src'));

            // Marking the URL as bad will make effectiveBoxartUrl return the next one
            var effectiveUrl = game.effectiveBoxartUrl;
            var url = effectiveUrl ? effectiveUrl : Origin.views.gameTile.DEFAULT_BOXART_IMAGE;
            $(this).attr('src', url);
        }
    });

    // Update the newly created tile
    this.updateGame($game, game);
    
    return $game[0];
};

MyGamesView.prototype.updateGame = function($tile, game)
{
    var zoomFactor = Origin.model.zoomFactor;
    var tileWidth = Math.round(DEFAULT_WIDTH * zoomFactor);
    var tileHeight = Math.round(DEFAULT_HEIGHT * zoomFactor);

    Origin.views.gameTile.updateGameHtml($tile, game, tileWidth, tileHeight);
    
    // If there the game's hovercard is visible, update it
    if (Origin.HovercardManager.isVisible(game.entitlement.id))
    {
        Origin.HovercardManager.updateHtml(this.getHovercardHtml(game));
    }
};

MyGamesView.prototype.layoutTiles = function($newTiles, filterInitiated, callback)
{
    var $tilesContainer = $('#game-library');
    var $tiles = $tilesContainer.children('li');
    var zoomFactor = Origin.model.zoomFactor;
    var animationDuration = 0;

    var tileWidth = Math.round(DEFAULT_WIDTH * zoomFactor);
    var tileHeight = Math.round(DEFAULT_HEIGHT * zoomFactor);
    var tileMargin = Math.round(DEFAULT_MARGIN * zoomFactor);

    // Zoom all of our tiles first
    $tiles
        .css({ width: (tileWidth + 'px'), height: (tileHeight + 'px')})
        .find('.violator,.otk-operation-controller-mini').css({ '-webkit-transform': 'scale(' + zoomFactor + ')' });

    // Cut out detail below a certain zoom
    $tiles.toggleClass('tiny', zoomFactor < 0.6);

    // We require absolute position
    // Don't do this in the stylesheet so we still work with "static" layout
    $tiles.css({position: 'absolute'});

    if (filterInitiated)
    {
        // Fade new tiles in but don't animate their position
        $newTiles.css({opacity: 0, '-webkit-transition': 'opacity 750ms ease 0'});
        // Animate the position of existing tile
        $tiles.not($newTiles).css({'-webkit-transition': 'left 750ms ease 0, top 750ms ease 0, opacity 750ms ease 0'});

        $tiles.one('webkitTransitionEnd', function() { $(this).css({'-webkit-transition': ''}); });

        animationDuration = Math.max(animationDuration, 750);
    }

    // Find the visible and hidden tiles
    var visibleTiles = [];
    var invisibleTiles = [];

    $.each($tiles, $.proxy(function(idx, tile)
    {
        var tileIndex = this._visibleEntitlementIds.indexOf($(tile).attr('data-id'));

        if (tileIndex !== -1)
        {
            visibleTiles[tileIndex] = tile;
        }
        else
        {
            invisibleTiles.push(tile);
        }
    }, this));

    // Fade out the hidden tiles
    var $invisibleTiles = $(invisibleTiles);
    $invisibleTiles.css({opacity: "0"});

    // Calulate tile sizes including margins
    var fullTileWidth = tileWidth + tileMargin;
    var fullTileHeight = tileHeight + tileMargin;

    var containerWidth = $tilesContainer.width();
    var tilesPerRow = Math.floor((containerWidth - tileMargin) / fullTileWidth);

    var leftStart;
    var actualTilesPerRow;

    if (visibleTiles.length <= tilesPerRow)
    {
        actualTilesPerRow = visibleTiles.length;
        leftStart = tileMargin;
    }
    else
    {
        actualTilesPerRow = tilesPerRow;
        leftStart = (containerWidth - (tilesPerRow * fullTileWidth) + tileMargin) / 2;
    }

    // Determine if we should do our reflow animation
    var animateReflow = false;
    if (!filterInitiated &&
            (this._previousActualTilesPerRow !== null) &&
            (this._previousActualTilesPerRow !== actualTilesPerRow))
    {
        animateReflow = true;
        animationDuration = Math.max(animationDuration, 300);
    }

    this._previousActualTilesPerRow = actualTilesPerRow;

    // Position the tiles
    var currentLeft = leftStart;
    var currentTop = tileMargin;
    var currentRow = 0;
    
    var tW,
        tH;

    $.each(visibleTiles, function(idx, tile)
    {
        var $tile = $(tile);

        // Have we overflowed?
        if ((containerWidth - tileMargin - currentLeft) < tileWidth)
        {
            // Wrap
            currentLeft = leftStart;
            currentTop += fullTileHeight;
            currentRow++;
        }
        
        $tile.css({
            left: currentLeft, 
            top: currentTop, 
            opacity: "1", 
            visibility: "visible",
            zIndex: idx
        }).removeClass('invisible');

        // Update our state
        currentLeft += fullTileWidth;

        // Handle reflow animations
        var tilePreviousRow = $tile.data('row');
        if ($tile.data('row') != currentRow)
        {
            $tile.data('row', currentRow);

            if (animateReflow)
            {
                // We changed rows; animate our left and top
                $tile.css({'-webkit-transition': 'left 300ms linear, top 300ms linear'})
                    .one('webkitTransitionEnd', function() { $(this).css({'-webkit-transition': ''}); });
            }
        }
        else if (animateReflow)
        {
            // Just animate our left
            $tile.css({'-webkit-transition': 'left 300ms linear'})
                .one('webkitTransitionEnd', function() { $(this).css({'-webkit-transition': ''}); });
        }

        var game = Origin.model.getGameById($tile.attr('data-id'));
        var $img = $tile.find('> div.boxart-container > img.boxart');

        if (!tW) {
            // i don't know if this is a valid assumption but lets try
            tW = $tile.width();
            tH = $tile.height();
        }

        game.addBoxartTransformations(tW, tH, $img);
    });
    
    // Adjust our container height to the final height after the animation completes
    var containerHeight = currentTop + fullTileHeight;
    $tilesContainer.height(containerHeight);

    // Are we showing a scrollbar?
    if (containerHeight > $("#main").height())
    {
        $('body').addClass('contents-scrolling');
        $("#on-the-house-frame").addClass('scrolling');
        //$('#feedback').css('right', $('#feedback').width()*-.447+19);
    }
    else
    {
        $('body').removeClass('contents-scrolling');
        $("#on-the-house-frame").removeClass('scrolling');
        //$('#feedback').css('right', $('#feedback').width()*-.447);
    }
        
    // Hook up our cleanup function
    var finishAnimation = function()
    {
        this._activeAnimation = null;

        // Remove the invisible tiles from the DOM
        $invisibleTiles.remove();

        if (callback)
        {
            callback();
        }
    };

    if (animationDuration > 0)
    {
        // Cancel any previous cleanup function
        if (this._activeAnimation)
        {
            clearTimeout(this._activeAnimation)
        }

        this._activeAnimation = setTimeout($.proxy(finishAnimation, this), animationDuration);
    }
    else if (!this._activeAnimation)
    {
        finishAnimation.apply(this);
    }
    else
    {
        // Let the existing animation finish
    }
};


MyGamesView.prototype.getHovercardHtml = function(game)
{
    return Origin.util.getGameHovercardHtml(game, false);
};

/////////////////////////////////////////////////////////////////////////////////////////////
//////////On the House Promotion code  //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
MyGamesView.prototype.onOtHSuccessful = function(info)
{
    var lastDismissedOffer = '',
        version = info.version,
        html = info.html + OTH_READY_SCRIPT;
    
    if(OTH_OVERRIDE === "")
    {
        lastDismissedOffer = window.clientSettings.readSetting("last_dissmissed_oth_offer");
    
        if (lastDismissedOffer === info.version)
        {
            // Show nothing if the user has already dismissed this promo.
            version = "";
            html = "";
        }
    }
    
    // Cache current version and HTML so we don't need to query the promo service again on page refresh
    window.sessionStorage.setItem("oth_cache_version", version);
    window.sessionStorage.setItem("oth_cache_html", html);
    // Set its HTML
    this.writeOtHOffer(version, html);
};

MyGamesView.prototype.onOtHFailure = function()
{
    // Delete our cached version and HTML so that a promo request will
    // be made when this page is reloaded (i.e. game details page and back).
    window.sessionStorage.removeItem("oth_cache_version");
    window.sessionStorage.removeItem("oth_cache_html");
    this.writeOtHOffer("", "");
};

MyGamesView.prototype.writeOtHOffer = function(version, html)
{
    var othIframe = document.getElementById('on-the-house-promo');
    
    // Remember our version for when we close
    $(othIframe).attr('data-version', version);
        
    var iframeDoc = othIframe.contentDocument || othIframe.contentWindow.document;
    if (iframeDoc)
    {
        iframeDoc.open();
        iframeDoc.write(html);
        iframeDoc.close();
    }
};

MyGamesView.prototype.onhideOtH = function()
{
    var $iframe = $(document.getElementById('on-the-house-promo'));
    var $frame = $("#on-the-house-frame");
    var version = $iframe.attr('data-version');
    
    // Clear cached version and HTML.  Note that we're not using 
    // removeItem because that would trigger another promo request
    // when the page is reloaded (i.e. game details page and back).
    window.sessionStorage.setItem("oth_cache_version", "");
    window.sessionStorage.setItem("oth_cache_html", "");
    // Set its version and HTML to empty string
    Origin.views.myGames.writeOtHOffer("", "");
    // Hide the iframe and save version to settings
    $iframe.addClass('hidden');
    $frame.addClass('hidden');
    clientSettings.appendStringSetting("OnTheHouseVersionsShown", version);
    window.clientSettings.writeSetting("last_dissmissed_oth_offer", version);
};

MyGamesView.prototype.showOtH = function()
{
    telemetryClient.sendOnTheHouseBannerLoaded();
    var $iframe = $("#on-the-house-promo"); 
    $iframe.height( $iframe[0].contentDocument.body.scrollHeight+'px' );
    $iframe.width( $iframe[0].contentDocument.body.scrollWidth +'px'  );
    
    var $frame = $("#on-the-house-frame");
    var $library = $("#game-library");
    $frame.height($iframe.height());
    $frame.removeClass('hidden');
    $iframe.removeClass('hidden');
}

$(window).on("message", function(ev)
{
    switch(ev.originalEvent.data.type)
    {
        case 'closeOthPromo':
            var $othIframe = $('#on-the-house-promo');

            if ($othIframe.length === 0)
            {
                console.warn('Tried to removed non-existant promo??');
                return;
            }
            this.Origin.views.myGames.onhideOtH();
        break;
        case 'ready':
            Origin.views.myGames.showOtH();
        break;
    }
});
// Expose to the world
Origin.views.myGames = new MyGamesView();

})(jQuery);
