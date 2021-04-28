;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var DEFAULT_BANNER_IMAGE = './mygames/images/game-details/generic-banner.jpg';

var DEFAULT_EXTRA_CONTENT_SORT_BY = "sortOrderDescending";
var DEFAULT_EXTRA_CONTENT_SORT_DIRECTION = -1;
var DEFAULT_EXTRA_CONTENT_SORT_TIEBREAKER = "displayUnlockStartDate";

var DEFAULT_EXTRA_CONTENT_SECTION_SORT_ORDER_VALUE = 2147483648; // C++ INT_MAX + 1

// Private helper for building a roster contact HTML
function contactHtml(contact)
{
    var html = '';
    html += '<li data-tooltip="' + $('<span></span>').text(contact.nickname).text() + '" data-id="' + contact.id + '">';
    html += '<img src="' + contact.avatarUrl + '" alt="" />';
    html += '</li>';

    return html;
}

function sortEntitlements(entitlements, prop, direction, tiebreaker)
{
    entitlements.sort(function(a, b)
    {
        if (a.entitlement[prop] > b.entitlement[prop])
        {
            return direction;
        }
        else if (a.entitlement[prop] < b.entitlement[prop])
        {
            return direction * -1;
        }
        else
        {
            if(a.entitlement[tiebreaker] > b.entitlement[tiebreaker])
            {
                return direction;
            }
            else
            {
                return direction * -1;
            }
        }
    });
}

function sortExtraContent(extraContent, prop, direction, tiebreaker)
{
    var extraContentWithSortOrder = [];
    var extraContentWithoutSortOrder = [];

    $.each(extraContent, function(idx, entitlement)
    {
        if(entitlement.entitlement[prop] !== null)
        {
            extraContentWithSortOrder.push(entitlement);
        }
        else
        {
            extraContentWithoutSortOrder.push(entitlement);
        }
    });

    sortEntitlements(extraContentWithSortOrder, prop, direction, tiebreaker);
    sortEntitlements(extraContentWithoutSortOrder, prop, direction, tiebreaker);

    // Addons and expansions are sorted by releaseDate.
    // However, if they have a custom sort order, that takes precedence.
    extraContent = $.merge([], extraContentWithSortOrder);
    extraContent = $.merge(extraContent, extraContentWithoutSortOrder);

    return extraContent;
}

function sortExtraContentSections(a, b)
{
    var sectionA = parseInt($(a).attr('data-sort'));
    var sectionB = parseInt($(b).attr('data-sort'));
    return (sectionA - sectionB);
}

var GameDetailsView = function()
{
    // Bind to model signals
    Origin.model.gameInitEvent.add($.proxy(this, 'onGameInitialized'));
    Origin.model.gameUpdateEvent.add($.proxy(this, 'onGameUpdated'));
    Origin.model.gamesChangeEvent.add($.proxy(this, 'onGamesChanged'));

    Origin.model.gameExpansionAddEvent.add($.proxy(this, 'onGamePDLCAdded'));
    Origin.model.gameExpansionUpdateEvent.add($.proxy(this, 'onGameExpansionUpdated'));
    Origin.model.gameExpansionRemoveEvent.add($.proxy(this, 'onGamePDLCRemoved'));

    Origin.model.gameAddonAddEvent.add($.proxy(this, 'onGamePDLCAdded'));
    Origin.model.gameAddonUpdateEvent.add($.proxy(this, 'onGameAddonUpdated'));
    Origin.model.gameAddonRemoveEvent.add($.proxy(this, 'onGamePDLCRemoved'));

    Origin.model.gameCloudSavesCurrentUsageChangeEvent.add($.proxy(this, 'onGameCloudSavesCurrentUsageChange'));
    Origin.model.gameCloudSavesChangeEvent.add($.proxy(this, 'onGameCloudSavesChange'));

    Origin.model.onlineStatusChangeEvent.add($.proxy(this, 'onOnlineStatusChange'));

    this.hideText = tr('ebisu_client_hide_game_in_library');
    this.showText = tr('ebisu_client_unhide_game');

    $('#cloudOfflineTooltip > span').text(tr('ebisu_client_must_be_online_to_change_setting'));
    $(".origin-ux-tooltip a").on("click", function ( event ) {
        event.preventDefault();
    });

    this._releaseNotificationsHtml = '';
    this._otherNotificationsHtml = '';
    this._moreInfoHtml = '';
    this._expansionsHtml = '';
    this._addonsHtml = {};

    this._addonSortBy = {};
    this._addonSortDirection = {};
    this._addonSortTiebreaker = DEFAULT_EXTRA_CONTENT_SORT_TIEBREAKER;

    this._expansionSortBy = DEFAULT_EXTRA_CONTENT_SORT_BY;
    this._expansionSortDirection = DEFAULT_EXTRA_CONTENT_SORT_DIRECTION;
    this._expansionSortTiebreaker = DEFAULT_EXTRA_CONTENT_SORT_TIEBREAKER;

    var $body = $(document.body);
    var $main = $('#main');

    // Add our platform attribute for the benefit of CSS
    $(document.body).attr('data-platform', Origin.views.currentPlatform());

    function scrollCheck()
    {
        // Are we showing a scrollbar?
        if ($("#main").height() < $("#main")[0].scrollHeight)
        {
            $('body').addClass('contents-scrolling');
        }
        else
        {
            $('body').removeClass('contents-scrolling');
        }
    }

    // Resize the main content area on resize of the browser to ensure that scrollbars are present when needed
    var mainPadding = $main.outerHeight(true) - $main.height();
    var bodyPadding = $body.outerHeight() - $body.height();
    $(window).on('resize', function(evt)
    {
        // + 1 so the background image will fill the whole background. Without it, it shows a
        // white line while the client is maximized.
        $('#main').height((window.innerHeight - bodyPadding - mainPadding) + 1);

        scrollCheck();

        // Make sure columns don't have weird wrapping on detail tab
        var artHeight = $("#art-col").height();
        if (artHeight > 0)
        {
            var notificationHeight = $('#release-notifications').height() + $('#other-notifications').height();
            $('#links').css('min-height', (artHeight - notificationHeight + 30) + 'px');
        }

        // When resizing, tooltips on addon titles might be needed or not
        $.each($('.addons').find('tr .addon-title div'), function(id, item)
        {
            if(item.scrollWidth > item.clientWidth)
            {
                $(item).attr('title', $(item).text());
            }
            else
            {
                $(item).removeAttr('title');
            }
        });

        // When resizing, make sure the favorite star moves as needed
        var $titleBlock = $('#game-info > header > h1');
        var $title = $titleBlock.find('span');
        var $favoriteButton = $titleBlock.find('button');
        if ($favoriteButton.length > 0)
        {
            $favoriteButton.css('left', $title.outerWidth());
        }
    }).resize();

    $main.on('scroll', $.proxy(function(evt)
    {
        // Move the hovercard when scrolling
        Origin.HovercardManager.updatePosition(true);
    }, this));

    var $viewOptions = $('#view-options');

    $viewOptions.on('submit', function(evt){ evt.preventDefault(); return false; });


    // --------------------------
    // Back to Library
    // Bind to the back click event
    $viewOptions.on('click', '#view-options > button.back', function(evt)
    {
        evt.preventDefault();

        window.location = 'index.html';
    });


    var $gameInfo = $('#game-info');

    // Favorite button
    $gameInfo.on('click', '#game-info > header > h1 > button', function(evt)
    {
        evt.preventDefault();
        Origin.model.game.isFavorite = !$(this).hasClass('favorite');
    });

    // --------------------------
    // Header Actions
    $gameInfo.on('mouseup', '#game-info > header > .action > button', function(evt)
    {
        evt.preventDefault();
        evt.stopImmediatePropagation();

        var $btn = $(this);
        if ($btn.hasClass('buy'))
        {
            if( Origin.model.game.isTrialType )
            {
                var expired = ((Origin.model.game.secondsUntilTermination === 0) ? '1' : '0');
                telemetryClient.sendFreeTrialPurchase(Origin.model.game.entitlement.productId, expired, 'gamedetails');
            }

            Origin.model.game.viewStoreMasterTitlePage();
        }
        else if ($btn.hasClass('trial-view-offer'))
        {
            var promoType = Origin.model.game.secondsUntilTermination === 0 ? "FreeTrialExpired" : "FreeTrialExited";
            var scope = "OwnedGameDetails";
            clientNavigation.showPromoManager(Origin.model.game.entitlement.productId, promoType, scope);
        }
        else if ($btn.hasClass('push-to-top'))
        {
            telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, Origin.model.game.entitlement.productId, "mygames-game-details");
            contentOperationQueueController.pushToTop(Origin.model.game.entitlement.productId);
        }
        else if ($btn.hasClass('download'))
        {
            Origin.model.game.startDownload();
        }
        else if ($btn.hasClass('play'))
        {
            telemetryClient.sendGamePlaySource(Origin.model.game.entitlement.productId, "gamedetails");
            Origin.model.game.play();
        }
        else if ($btn.hasClass('install'))
        {
            Origin.model.game.install();
        }
    });

    // --------------------------
    // Hovercard with debug/ORC info
    $main.on('hover', 'li.game,td.internal-debug', function(evt)
    {
        evt.stopImmediatePropagation();
        var $this = $(this);
        // Addons have attr on their parent
        var game = Origin.model.game.getExpansionById($this.attr('data-id')) ||
            Origin.model.game.getAddonById($this.parent().attr('data-id'));

        if (!game) { return; }

        switch (evt.type)
        {
            case ('mouseenter'):
            {
                Origin.HovercardManager.showDelayed($this, Origin.views.gameDetails, game, game.entitlement.id);
                break;
            }
            case ('mouseleave'):
            {
                Origin.HovercardManager.hideDelayed();
                break;
            }
        }
    })
    .on('click', 'li.game', function(evt)
    {
        evt.stopImmediatePropagation();
        var $this = $(this);
        var id = $this.attr('data-id');
        if (!id) { return; }

        var game = Origin.model.game.getExpansionById(id);
        Origin.HovercardManager.show($this, Origin.views.gameDetails, game, id);
    })
    .on('dblclick', 'li.game', function(evt)
    {
        evt.stopImmediatePropagation();
        var $this = $(this);
        var id = $this.attr('data-id');
        if (!id) { return; }

        var game = Origin.model.game.getExpansionById(id);
        game.performPrimaryAction("gamedetails");
    });

    // Bind to the game expansion details button
    $body.on('click', '.hovercard > .content > footer > table.footer-table > tbody > tr > td > button.pdlc-store-button', function(evt)
    {
        evt.stopImmediatePropagation();

        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));

        // send telemetry when users click "more details" on a purchasable item
        telemetryClient.sendContentSalesViewInStore(game.entitlement.productId, game.entitlement.productType, 'hovercard_button');
        clientNavigation.showPDLCStore(Origin.model.game.entitlement.productId);
    });

    // Bind to the game hovercard footer action buttons
    $body.on('click', '.hovercard > .content > footer > table.footer-table > tbody > tr > td > button', function(evt)
    {
        evt.stopImmediatePropagation();
        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));
        Origin.util.handleHovercardButtonClick(game, $btn);
    });

    // Bind to the game expansion hovercard title
    $body.on('click', '.hovercard > .content > header > h3 > a.show-pdlc-store', function(evt)
    {
        evt.stopImmediatePropagation();

        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));

        // send telemetry when users click "more details" on a purchasable item
        telemetryClient.sendContentSalesViewInStore(game.entitlement.productId, game.entitlement.productType, 'hovercard_title');
        clientNavigation.showPDLCStore(Origin.model.game.entitlement.contentId);
    });

    $body.on('click', '.hovercard > .content > ul.notifications > li.type-new-dlc > div.message > button.view-in-store', function(evt)
    {
        evt.stopImmediatePropagation();
        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));
        Origin.util.handleHovercardButtonClick(game, $btn);
    });

    $body.on('click', '.hovercard > .content > header > h3 > a.view-in-store', function(evt)
    {
        evt.stopImmediatePropagation();

        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));

        // send telemetry when users click "more details" on a purchasable item
        telemetryClient.sendContentSalesViewInStore(game.entitlement.productId, game.entitlement.productType, 'hovercard_title');
        game.viewStoreProductPage();
    });

    // --------------------------
    // Tabs
    $gameInfo.on('click', '#game-info > .tabs-content > .tabs > ul > li > a', function(evt)
    {
        evt.preventDefault();
        evt.stopImmediatePropagation();

        var $this = $(this);
        var tab = $this.attr('href').replace(/^#?/, '');
        var $li = $this.parent();
        if ($li.hasClass('current')) { return; }

        $li.siblings('.current').removeClass('current');
        $li.addClass('current');

        var $tabsContent = $('#main > #game-info > .tabs-content > ul');
        $tabsContent.find('> li.current').removeClass('current');
        $tabsContent.find('> li.' + tab).addClass('current');

        if (tab === 'cloud-storage' && Origin.model.game.entitlement.cloudSaves)
        {
            var $cloudText = $('#cloud_text');
            $cloudText.html(tr('ebisu_client_cloud_text').replace(/%1/g, 'Origin'));

            telemetryClient.sendMyGamesCloudStorageTabView(Origin.model.game.entitlement.productId);

            Origin.views.gameDetails.onGameCloudSavesChange(Origin.model.game);
            Origin.model.game.entitlement.cloudSaves.pollCurrentUsage();
        }
    });

    // --------------------------
    // Tabs Content: Game Details

    $gameInfo.on('click', 'a.properties', function(evt)
    {
        evt.preventDefault();
        entitlementManager.dialogs.showProperties(Origin.model.game.entitlement.productId);
    });

    $gameInfo.on('click', 'a.hide-game', function(evt)
    {
        evt.preventDefault();

        var $this = $(this);
        var text = $this.html();
        if (text === Origin.views.gameDetails.hideText)
        {
            $this.html(Origin.views.gameDetails.showText);
            Origin.model.game.isHidden = true;
        }
        else
        {
            $this.html(Origin.views.gameDetails.hideText);
            Origin.model.game.isHidden = false;
        }
    });
    
    $gameInfo.on('click', 'a.remove-entitlement', function(evt)
    {
        evt.preventDefault();
        evt.stopImmediatePropagation();
        Origin.model.game.removeFromLibrary();
    });
	
	$gameInfo.on('click', 'a.upgrade-backup-restore', function(evt)
    {
        evt.preventDefault();
        evt.stopImmediatePropagation();
        Origin.model.game.entitlement.restoreSubscriptionUpgradeBackupSave();
    });

    $gameInfo.on('click', 'a.uninstall', function(evt)
    {
        evt.preventDefault();
        evt.stopImmediatePropagation();

        Origin.model.game.uninstall();
    });

    $gameInfo.find('.pdlc-store-button').on('click', function(evt)
    {
        evt.stopImmediatePropagation();
        clientNavigation.showPDLCStore(Origin.model.game.entitlement.contentId);
    });

    // ------------------
    // Notifications

    // Bind to the game notification dismiss button
    $body.on('click', '#game-info > div.tabs-content > ul > li.details > div.info > div.notifications > ul.notifications > li.dismissable > button', function(evt)
    {
        evt.preventDefault();

        var $notification = $(this).parent();
        var notificationKey = $notification.attr('data-key');
        var game = Origin.model.game;

        // Remove the notification
        if ($notification.parent().find('> li').length === 1)
        {
            $notification = $notification.parents('div.notifications:first');
        }

        $notification.slideUp('fast', function()
        {
            $(this).remove();

            if (notificationKey)
            {
                game.dismissNotification(notificationKey);
            }
        });

    });

    // Bind to the expansion notification dismiss button on hovercards
    $body.on('click', '#hovercard > div.content > ul.notifications > li.dismissable > button', function(evt)
    {
        var $notification = $(this).parent();
        var notificationKey = $notification.attr('data-key');
        var game = Origin.model.game.getExpansionById($notification.parents('[data-id]:first').attr('data-id'));

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
    $body.on('click', 'div.notifications > ul.notifications > li > .message > button', function(evt)
    {
        //handle events for page game details
        var $btn = $(this);
        evt.stopImmediatePropagation();
        Origin.views.gameDetails.handlePageClick(Origin.model.game, $btn);
    });

    $body.on('click', '#hovercard > div.content > ul.notifications > li > .message > button', function(evt)
    {
        //handle events for expansion hover card
        var $btn = $(this);
        var game = Origin.model.game.getExpansionById($btn.parents('.hovercard:first').attr('data-id'));

        evt.stopImmediatePropagation();
        Origin.views.gameDetails.handlePageClick(game, $btn);
    });
    
    $body.on('click', '#main > #addons-area > table.addons > tbody > tr > td > table.addon-description > tbody > tr > td.addon-description  a', function(evt)
    {
        //handle events for expansion hover card
        var $btn = $(this);
        var game = Origin.model.game.getAddonById($btn.parents('tr.addon-description').attr('data-id-desc'));
        evt.stopImmediatePropagation();
        Origin.views.gameDetails.handlePageClick(game, $btn);
    });

    // Cloud Storage
    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));
    $gameInfo.on('click', '#game-info > .tabs-content > ul > li.cloud-storage > .usage > form > label > :checkbox', function(evt)
    {
        if(clientSettings.areServerUserSettingsLoaded === false)
        {
            evt.preventDefault();
        }
        clientSettings.writeSetting("cloud_saves_enabled", this.checked);
    });
    this.onUpdateSettings();

    $gameInfo.on('click', '#game-info > .tabs-content > ul > li.cloud-storage > .backup > button', function(evt)
    {
        Origin.model.game.entitlement.cloudSaves.lastBackup.restore();
    });


    /* Detail Links */
    var $details = $gameInfo.find('> .tabs-content > ul > li.details');

    $details.on('click', '.game-links a.manual', function(evt)
    {
        evt.preventDefault();

        telemetryClient.sendMyGamesManualLinkClick(Origin.model.game.entitlement.productId);
        clientNavigation.launchExternalBrowser(Origin.model.game.entitlement.manualUrl);
    });

    /* Epilepsy Warning Link */
    $details.on('click', '.game-links a.epilepsy-warning', function(evt)
    {
        evt.preventDefault();
        var locale = window.clientSettings.readSetting("locale");
        if (locale)
        {
            var epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/%0/GENERIC_EPI_%1.png";
            if (locale === "el_GR")
            {
                epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/gr_GK/GENERIC_EPI_EL.png";
            }
            else if (locale === "sl_SK")
            {
                epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/sl_SK/GENERIC_EPI_SK.png";
            }
            else if (locale === "zh_TW" || locale === "zh_CN")
            {
                epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/zh_TW/GENERIC_EPI_TC.png";
            }
            else if (locale === "pt_BR")
            {
                epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/pt_PT/GENERIC_EPI_PT.png";
            }
            else if (locale === "es_MX")
            {
                epilepsy_warning_url = "http://akamai.cdn.ea.com/eadownloads/u/f/manuals/EPILEPSY/es_ES/GENERIC_EPI_ES.png";
            }
            else
            {
                epilepsy_warning_url = epilepsy_warning_url.replace("%0", locale).replace("%1", locale.split("_")[0].toUpperCase());
            }

            clientNavigation.launchExternalBrowser(epilepsy_warning_url);
        }
    });

    /* Friends */
    $details.find('> .info > .friends-info > .friends > ul').tooltip({ selector: 'li' });

    // This captures both the expansion hovercard and our broken out social
    // area for the top level entitlement
    $body.on('click', '.friends > ul > li', function(evt)
    {
        evt.stopImmediatePropagation();

        var contactId = $(this).attr('data-id');

        if (window.originSocial)
        {
            // find out of it came from the friends list or the hovercard
            var parentDetails = $(this).closest(".details");
            if (parentDetails.length > 0)
                telemetryClient.sendProfileGameSource("DETAILS");
            else
                telemetryClient.sendProfileGameSource("EXPANSION");
            originSocial.getRosterContactById(contactId).showProfile("GAME_DETAILS");
        }
    });

    /* Expansions */
    var $expansionsArea = $('#main > #expansions-area');
    $expansionsArea.on('contextmenu', '.expansions .content ul li.game', function(evt)
    {
        var expansion = Origin.model.game.getExpansionById($(this).attr('data-id'));

        if (Origin.GameContextMenuManager.popup(expansion))
        {
            evt.stopImmediatePropagation();
            evt.preventDefault();
        }
    });
    $expansionsArea.on('click', '.expansions .content ul li.game .otk-progressbar', function(evt)
    {
        evt.stopImmediatePropagation();
        clientNavigation.showDownloadProgressDialog();
    });

    /* Addons */
    var $addonsArea = $('#main > #addons-area');
    $addonsArea.on('click', 'table.addons thead th', function(evt)
    {
        var $this = $(this);
        var id = $this.closest('table').attr('id');

        if ($this.hasClass("title"))
        {
            Origin.views.gameDetails.setAddonSortField(id, "title");
        }
        else if ($this.hasClass("date"))
        {
            Origin.views.gameDetails.setAddonSortField(id, "displayUnlockStartDate");
        }
        else
        {
            // Can't sort by this
            return;
        }

        // Update the sorting indicator
        $addonsArea.find('#' + id + '.addons > thead > tr > th')
            .removeClass("sorting")
            .removeClass("ascending")
            .removeClass("descending");

        $this.addClass("sorting");
        if (Origin.views.gameDetails._addonSortDirection[id] === 1)
        {
            $this.addClass("ascending");
        }
        else
        {
            $this.addClass("descending");
        }
    });

    // Expand description
    $addonsArea.on('click', 'table.addons tbody .addon-desc-expander-button', function(evt)
    {
        if (!$(this).hasClass('collapse'))
        {
            var trParent = $(this).parent().parent();
            telemetryClient.sendAddonDescriptionExpanded(trParent.attr("data-id"));
         }
        $(this).parent("td").parent("tr").parents("tr").next().slideToggle("fast");
        $(this).parent("td").parent("tr").parents("tr").toggleClass("expanded");
        $(this).toggleClass("collapse");
    });
    // TODO - implement
    /*
    $addonsArea.on('click', 'table.addons tbody tr', function(evt)
    {
        console.log('select row');
    });
    */
    $addonsArea.on('click', 'table.addons tbody button', function(evt)
    {
        evt.stopImmediatePropagation();
        var $this = $(this);
        var id = $this.parents('tr:first').attr('data-id');
        var addon = Origin.model.game.getAddonById(id);
        if ($this.hasClass('primary buy'))
        {
            telemetryClient.sendAddonBuyClick(id);
            addon.purchase('addon_table_button');
        }
        else if ($this.hasClass('play'))
        {
            addon.play();
        }
        else if ($this.hasClass('download'))
        {
            addon.startDownload();
        }
        else if ($this.hasClass('install-parent'))
        {
            // Prompt the user to install the base game
            entitlementManager.dialogs.promptForParentInstallation(addon.entitlement.productId);
        }
        else if($this.hasClass('upgrade-parent'))
        {
            addon.entitlement.upgrade();
        }
        else if ($this.hasClass('install'))
        {
            addon.install();
        }
    });
    $addonsArea.on('click', 'table.addons tbody .otk-progressbar', function(evt)
    {
        evt.stopImmediatePropagation();
        clientNavigation.showDownloadProgressDialog();
    });
};

GameDetailsView.prototype.updateGame = function(game)
{
    var entitlement = game.entitlement;
    var html;
    var showOperationButtons = false;

    var $gameInfo = $('#game-info');

    // Game title
    var $titleBlock = $gameInfo.find('> header > h1');
    var $title = $titleBlock.find('span').text(game.entitlement.title);
    var titleWidth = $title.outerWidth(); // for offline title drawing issue
    var $favoriteButton = $titleBlock.find('button');
    var playHoverText = '';
    var $removeEntitlementLink = '';
    var notificationLinkClasses = '';
    var notificationHTMLVal = '';

	// ORSUBS-1642 We want to show the logo for timed trial objects.
    $('.game-info-subscription').toggle(entitlement.isEntitledFromSubscription || entitlement.itemSubType === 'TIMED_TRIAL_ACCESS');

    if (onlineStatus.onlineState && (entitlement.displayLocation === 'GAMES' || entitlement.displayLocation === 'GAMES|EXPANSIONS'))
    {
        $favoriteButton.remove();
        var $button = $('<button' + ((game.isFavorite)? ' class="favorite"' : '') + ' title="' + $('<span></span>').text(game.isFavorite ? tr('ebisu_client_unfavorite_tooltip') : tr('ebisu_client_favorite_tooltip')).text() + '"></button>').appendTo($titleBlock);
        $button.css('left', titleWidth);
    }
    else
    {
        $favoriteButton.hide();
    }

    // Time played
    var subtitle;

    if (!game.isUnreleased)
    {
        subtitle = game.hoursPlayedString;
    }

    $gameInfo.find('> header > h6 > span').html(subtitle);

    var $actionButton = $gameInfo.find('> header > .action > button');
    $actionButton.removeClass('play').removeClass('install').removeClass('download').removeClass('push-to-top');

    var $progressBarContainer = $gameInfo.find('> header > .action > .otk-operation-controller-mini'),
        operation = game.entitlement.currentOperation;

    Origin.util.updateGameProgressBar(game, $progressBarContainer, operation ? operation.isDynamicOperation : false);
    if (operation)
    {
        // If we are in the queue, but not the head and don't have progress - show Download Now
        if(contentOperationQueueController.index(game.entitlement.productId) > 0 && !(operation.progress))
        {
            $actionButton.addClass('push-to-top').text(operation.specificPhaseDisplay('READYTOSTART'));
            if (contentOperationQueueController.queueSkippingEnabled(game.entitlement.productId))
            {
                $actionButton.removeAttr('disabled');
            }
            else
            {
                // Button should be disabled if we can't make it go to the top of the download queue
                $actionButton.attr('disabled', 'disabled');
            }
            $actionButton.show();
            $progressBarContainer.hide();
            $gameInfo.find('> header > .action').removeClass('active-operation');
        }
        // Else hide the button and show the progress bar
        else
        {
            // Operation is happening, show progress bar
            $gameInfo.find('> header > .action > button').hide();

            if(game.entitlement.playable)
            {
                playHoverText = tr('ebisu_client_play');
            }
            else if(game.entitlement.playing)
            {
                playHoverText = tr('ebisu_client_game_currently_running');
            }
            else
            {
                playHoverText = tr('ebisu_client_not_yet_playable');
            }

            // Edge case where we are the head and are in the enqueued state
            if(operation.phase !== 'ENQUEUED')
            {
                showOperationButtons = operation.isDynamicOperation && (operation.totalFileSize === 0 || operation.playableAt > 0);
                // Update our progress bar
                $progressBarContainer.otkOperationControllerMini({
                    playable: game.entitlement.playable,
                    // for playableAt at we need totalfilesize to be > 0. So if it's 0 its most likely installing.
                    showButtons: showOperationButtons,
                    playableHoverText: playHoverText,
                    // Updated in Origin.util.updateGameProgressBar
                    text: $progressBarContainer.find('.progress-info').text()
                });
                $progressBarContainer.show();
                $gameInfo.find('> header > .action').addClass('active-operation');
            }

            // Something is happening; disable the prompt to install us
            $('#main > #addons-area > .addons > tbody button.install-parent').attr('disabled', 'disabled');
        }
    }
    else
    {
        // Nothing is happen - see if there's an action we can perform
        $progressBarContainer.hide();
        $gameInfo.find('> header > .action').removeClass('active-operation');

        // Let addons prompt for parent installation again
        $('#main > #addons-area > .addons > tbody button.install-parent').removeAttr('disabled');

        if (game.isTrialType && game.secondsUntilTermination === 0)
        {
            $actionButton.addClass('trial-view-offer').text(tr('ebisu_client_view_special_offer'));

            if (onlineStatus.onlineState)
            {
                $actionButton.removeAttr('disabled');
            }
            else
            {
                // Button should be disabled when we're offline
                $actionButton.attr('disabled', 'disabled');
            }

            $actionButton.show();
        }
        else if (game.isDownloadable)
        {
            $actionButton.addClass('download').text(game.downloadVerb).removeAttr('disabled');
            $actionButton.show();
        }
        else if (game.isInstallable)
        {
            $actionButton.addClass('install').text(tr('ebisu_client_install')).removeAttr('disabled');
            $actionButton.show();
        }
        else if (game.isPlayable)
        {
            $actionButton.addClass('play').text(tr('ebisu_client_play'));
            $actionButton.removeAttr('disabled');

            $actionButton.show();
        }
        else if (game.isInstalled)
        {
            $actionButton.addClass('play').text(tr('ebisu_client_play'));
            $actionButton.attr('disabled', 'disabled');

            $actionButton.show();
        }
        else
        {
            $actionButton.hide();
        }
    }

    if(showOperationButtons)
    {
        $gameInfo.find('> header > .action').addClass('has-flag');
    }
    else
    {
        $gameInfo.find('> header > .action').removeClass('has-flag');
    }

    var $details = $gameInfo.find('> .tabs-content > ul > li.details');

    // Boxart
    var src = game.effectiveBoxartUrl || Origin.views.gameTile.DEFAULT_BOXART_IMAGE;
    var $boxart = $details.find('> div > .boxart > div > img');
    if ($boxart.attr('src') !== src)
    {
        $boxart.attr('src', src);
    }

    var $cropbox = $details.find('> div > .boxart > div');
    game.addBoxartTransformations($cropbox.width(), $cropbox.height(), $boxart);

    // Shop for add-ons
    var hasPDLCStore = Origin.model.game.hasPDLCStore;
    if(hasPDLCStore)
    {
        $details.parent().find('.pdlc-store-button').show();
        // If offline - disable the button
        if(!onlineStatus.onlineState)
        {
            $details.parent().find('.pdlc-store-button').attr('disabled', 'disabled');
        }
        else
        {
            $details.parent().find('.pdlc-store-button').removeAttr('disabled');
        }
    }

    if (game.entitlement.debugInfo)
    {
        // Debug
        if ($gameInfo.find('> .tabs-content > ul > li.debug-info').length === 0)
        {
            $gameInfo.find('> .tabs-content > .tabs > ul').append('<li><a href="#debug-info"><span>' + tr('ebisu_client_notranslate_debug_info') + '</span></a></li>');
            $gameInfo.find('> .tabs-content > ul').append('<li class="debug-info"></li>');
        }

        var html = '<dl>';
        $.each(game.entitlement.debugInfo.summary, function(key, value)
        {
            html += '<dt>' + key + '</dt>';
            html += '<dd>' + value + '</dd>';
        });
        $.each(game.entitlement.debugInfo.details, function(key, value)
        {
            html += '<dt>' + key + '</dt>';
            html += '<dd>' + value + '</dd>';
        });
        html += '</dl>';
        $gameInfo.find('> .tabs-content > ul > li.debug-info').html(html);
    }

    // Cloud storage
    var $cloudStorageTab = $gameInfo.find('> .tabs-content > .tabs > ul > li.cloud-saves');
    if (game.entitlement.cloudSaves === null)
    {
        $cloudStorageTab.hide();
    }
    else
    {
        $cloudStorageTab.show();
    }


    if(onlineStatus.onlineState)
    {
        $('#cloudOfflineTooltip').hide();
        $('#cloud-storage-enabled').prop('disabled', false);
    }
    else
    {
        $('#cloudOfflineTooltip').show();
        $('#cloud-storage-enabled').prop('disabled', true);
    }

    // Notifications
    var notifications = game.getNotifications();

    var $releaseNotifications = $('#release-notifications');
    var $otherNotifications = $('#other-notifications');

    var releaseHtml = '';
    var otherHtml = '';
    var lowestTimeNotification = -1;

    if (notifications.length > 0 || game.isUnreleased)
    {
        $.each(notifications, $.proxy(function(idx, notification)
        {
            var viewOfferButtonHtml = '';

            if (notification.type === 'internal-debug') { return true; }

            function notificationHtml(message)
            {
                var classes = 'type-' + (notification.type || 'generic') + ((notification.dismissable)? ' dismissable' : '');
                var html = '<li class="' + classes + '" data-key="' + notification.key + '">';

                html += '<div class="message">';
                html += message;
                html += '</div>';

                if (notification.dismissable)
                {
                    html += '<button>x</button>';
                }

                return html;
            }

            switch (notification.type)
            {
                case ('offline'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_cant_download_offline'));
                    break;
                }
                // Platforms
                case ('platform-pc'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_platform_available_only_on_windows'));
                    break;
                }
                case ('platform-mac'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_platform_available_only_on_osx'));
                    break;
                }
                case ('platform-none'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_platform_available_only_on_other'));
                    break;
                }
                // Expiration
                // TODO - QUESTION verify conditionals
                case ('expires'):
                {
                    if (notification.seconds > 0)
                    {
                        if(lowestTimeNotification === -1 || notification.seconds < lowestTimeNotification)
                            lowestTimeNotification = notification.seconds;
                        otherHtml += notificationHtml(tr('ebisu_client_expires_in').replace('%1', dateFormat.formatInterval(notification.seconds, 'HOURS', 'HOURS')));
                    }
                    else
                    {
                        otherHtml += notificationHtml(tr('ebisu_client_expired'));
                    }
                    break;
                }
                // Free trials
                case ('trial'):
                {
                    // Only display "view offer" link when we're online
                    if (onlineStatus.onlineState)
                    {
                        viewOfferButtonHtml = '<br/><button class="trial-view-offer">' + tr('ebisu_client_view_special_offer') + '</button>';
                    }

                    if(notification.seconds !== null)
                    {
                        if(lowestTimeNotification === -1 || notification.seconds < lowestTimeNotification)
                            lowestTimeNotification = notification.seconds;
                        otherHtml += notificationHtml(tr('ebisu_client_trial_expires_in').replace('%1', dateFormat.formatInterval(notification.seconds,'MINUTES', 'HOURS')) + viewOfferButtonHtml);
                    }
                    else
                        otherHtml += notificationHtml(tr('ebisu_free_trial_not_started') + viewOfferButtonHtml);
                    break;
                }
                // Expired free trials
                case ('trial-expired'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_trial_expired'));
                    break;
                }
                // timed trial
                case ('early-access'):
                {
                    // Only display "view offer" link when we're online
                    if (onlineStatus.onlineState)
                    {
                        viewOfferButtonHtml = '<br/><button class="buy">' + tr('ebisu_subs_trial_hovercard_upsell') + '</button>';
                    }
                    if(notification.seconds !== null)
                        otherHtml += notificationHtml(tr('ebisu_subs_trial_time_expire1').replace('%1', dateFormat.formatInterval(notification.seconds,'MINUTES', 'HOURS')) + viewOfferButtonHtml);
                    else
                        otherHtml += notificationHtml(tr('ebisu_subs_trial_hovercard_not_started') + viewOfferButtonHtml);
                    break;
                }
                // timed trial admin disable
                case ('timed-trial-admin-disable'):
                {
                    if (onlineStatus.onlineState)
                    {
                        viewOfferButtonHtml = '<br/><button class="buy">' + tr('ebisu_subs_trial_hovercard_upsell') + '</button>';
                    }
                    otherHtml += notificationHtml(tr('ebisu_subs_trial_unavailable') + viewOfferButtonHtml);
                    break;
                }
                // Expired timed trial
                case ('timed-trial-expired'):
                {
                    if (onlineStatus.onlineState)
                    {
                        viewOfferButtonHtml = '<br/><button class="buy">' + tr('ebisu_subs_trial_hovercard_upsell') + '</button>';
                    }
                    otherHtml += notificationHtml(tr('ebisu_subs_trial_hovercard_trial_expired') + viewOfferButtonHtml);
                    break;
                }
                // timed trial offline
                case ('timed-trial-offline'):
                {
                    otherHtml += notificationHtml(tr('ebisu_subs_trial_goonline_notification'));
                    break;
                }
                // Preload available
                case ('preload-available'):
                {
                    releaseHtml += notificationHtml(tr('ebisu_client_can_now_preload') + '<br/><button class="download">' + tr('ebisu_client_preload_now') + '</button>');
                    break;
                }
                case ('preloaded'):
                {
                    releaseHtml += notificationHtml(tr('ebisu_client_game_is_preloaded'));
                    break;
                }
                // Preload date
                case ('preload-date'):
                {
                    releaseHtml += notificationHtml('<span class="label">' + tr('ebisu_client_preload') + '</span> <span class="value">' + dateFormat.formatLongDateTime(notification.date) + '</span>');
                    break;
                }
                // Available date
                case ('available-date'):
                {
                    releaseHtml += notificationHtml('<span class="label">' + tr('ebisu_client_available') + '</span> <span class="value">' + dateFormat.formatLongDateTime(notification.date) + '</span>');
                    break;
                }
                // Update available
                case ('update-available'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_game_update_available') + '<br/><button class="update">' + tr('ebisu_client_update_now') + '</button>');
                    break;
                }
                // Subscription Upgrade available
                case ('upgrade-available-base-only'):
                {
                    notificationHTMLVal = tr('ebisu_client_subs_upgrade_edition');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    notificationHTMLVal += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    otherHtml += notificationHtml(notificationHTMLVal);
                    break;
                }
                case ('upgrade-available-dlc-only'):
                {
                    notificationHTMLVal = tr('ebisu_client_subs_upgrade_expansions');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    notificationHTMLVal += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    otherHtml += notificationHtml(notificationHTMLVal);
                    break;
                }
                // Subscription Upgrade available
                case ('upgrade-available-base-dlc'):
                {
                    notificationHTMLVal = tr('ebisu_client_subs_upgrade_edition_expansions');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    notificationHTMLVal += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    otherHtml += notificationHtml(notificationHTMLVal);
                    break;
                }
                // Subscription is Windows only
                case ('subs-windows-only'):
                {
                    otherHtml += notificationHtml(tr('ebisu_subs_edition_windows_only_notification'));
                    break;
                }
                case ('subs-windows-only-purchasable'):
                {
                    otherHtml += notificationHtml(tr('ebisu_subs_windows_only_purchasable_notification'));
                    break;
                }
                // Subscription membership ending
                case ('subs-membership-expired'):
                case ('subs-membership-expiring'):
                {
                    if(notification.seconds)
                    {
                        if(lowestTimeNotification === -1 || notification.seconds < lowestTimeNotification)
                            lowestTimeNotification = notification.seconds;
                        notificationHTMLVal = 
                            tr('Subs_Library_HoverCard_Expires_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS'));
                    }
                    else
                    {
                        notificationHTMLVal = tr('Subs_Membership_Expired');
                    }
                    
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    notificationHTMLVal += '<br/><button class="renew-subs-membership ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Renew') + '</button>';
                    if(notification.type === 'subs-membership-expired')
                    {
                        notificationHTMLVal += ' | <button class="buy ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Buy') + '</button>';
                    }
                    otherHtml += notificationHtml(notificationHTMLVal);
                    break;
                }
                // Subscription entitlement expiring
                case ('subs-entitlement-retired'):
                case ('subs-entitlement-retiring'):
                {
                    if(notification.seconds)
                    {
                        if(lowestTimeNotification === -1 || notification.seconds < lowestTimeNotification)
                            lowestTimeNotification = notification.seconds;
                        notificationHTMLVal = 
                            tr('Subs_Library_HoverCard_VaultExpire_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS'));
                    }
                    else
                    {
                        notificationHTMLVal = tr('Subs_Library_HoverCard_VaultExpire_Notification');
                    }
                    
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState || game.entitlement.playing)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    notificationHTMLVal += '<br/><button class="buy ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Buy') + '</button>';
                    notificationHTMLVal += ' | <button class="remove-entitlement ' + notificationLinkClasses + '">' + tr('ebisu_client_remove_from_library') + '</button>'
                    
                    otherHtml += notificationHtml(notificationHTMLVal);
                    break;
                }
                // Subscription offline play is expiring
                case ('subs-offline-play-expiring'):
                {
                    if(lowestTimeNotification === -1 || notification.seconds < lowestTimeNotification)
                        lowestTimeNotification = notification.seconds;
                    otherHtml += notificationHtml(tr('Subs_Library_HoverCard_Offline_Expires_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS')));
                    break;
                }
                // Subscription offline play expired
                case ('subs-offline-play-expired'):
                {
                    otherHtml += notificationHtml(tr('Subs_Library_HoverCard_Offline_Expired_Notification'));
                    break;
                }
                // New downloadable content available
                case ('new-dlc-available'):
                {
                    otherHtml += notificationHtml(tr('ebisu_client_new_dlc_notification'));
                    break;
                }
                // Just released
                // Custom Icon? No
                case ('just-released'):
                {
                    // TODO - what does this look like?
                    otherHtml += notificationHtml(tr('ebisu_client_just_released'));
                    break;
                }
                case ('preannouncement-date'):
                {
                    releaseHtml += '<li class="type-preannouncement-date"><div class="message"><span class="label">' + game.entitlement.preAnnouncementDisplayString + '</span>';
                    break;
                }
                case ('release-coming'):
                {
                    releaseHtml += '<li class="type-release-coming"><div class="message"><span class="label">' + tr('ebisu_client_not_yet_released') + '</span>'
                    if(game.entitlement.isPreorder)
                    {
                        releaseHtml += '<span class="value">' + tr('ebisu_client_not_yet_released_text').replace('%1', game.entitlement.title) + '</span>';
                    }
                    break;
                }
                default:
                {
                    if (notification.message)
                    {
                        otherHtml += notificationHtml(notification.message);
                    }
                }
            }

            html += '</div>';

            if (notification.dismissable)
            {
                html += '<button>x</button>';
            }
            html += '</li>';
        }, game));

        if (otherHtml !== this._otherNotificationsHtml)
        {
            this._otherNotificationsHtml = otherHtml;
            $otherNotifications.find('> ul.notifications').html(otherHtml);
        }

        if (releaseHtml !== this._releaseNotificationsHtml)
        {
            this._releaseNotificationsHtml = releaseHtml;
            $releaseNotifications.find('> ul.notifications').html(releaseHtml);
        }

        $otherNotifications[(otherHtml)? 'show' : 'hide']();

        // Offline and has valid time
        if(!onlineStatus.onlineState && lowestTimeNotification > 0)
        {
            // Less than a hour (seconds)
            if(lowestTimeNotification < 3600)
            {
                console.log('Update once a minute');
                // Update every minute (milliseconds)
                setTimeout(function(){ this.updateGame(Origin.model.game); }.bind(this), 60000);
            }
            // Less than a day (seconds)
            else if(lowestTimeNotification < 86400)
            {
                console.log('Update once an hour');
                // Update every hour (milliseconds)
                setTimeout(function(){ this.updateGame(Origin.model.game); }.bind(this), 3600000);
            }
            else
            {
                console.log('Update once a day');
                // Update once a day (milliseconds)
                setTimeout(function(){ this.onOnlineStatusChange(Origin.model.game); }, 86400000);
            }
        }
    }
    else
    {
        $otherNotifications.hide().find('> ul.notifications').empty();
    }

    // Only care about release information if available on this platform
    // Otherwise would show unreleased state for wrong platform at this time
    if (game.platformCompatibleStatus() === 'COMPATIBLE')
    {
        $releaseNotifications[(game.isUnreleased) ? 'show' : 'hide']();
    }
    // -----------------------
    // Links

    // Hide / Unhide game
    var $supportLinks = $details.find('.support-links');
    $supportLinks.hide();

    // TODO - remaining links
    // official-site
    // forum
    // customer-support
    // view in store

    var $gameLinks = $details.find(".game-links");

    // Manual Url
    var $manualLink = $gameLinks.find('a.manual');
    var hasManualLink = false;
    if (entitlement.manualUrl)
    {
        $manualLink.show();
    }
    else
    {
        $manualLink.hide();
    }

    // Epilepsy Warning Url always
    var $epilepsyWarningLink = $gameLinks.find('a.epilepsy-warning');
    $epilepsyWarningLink.show();
    $gameLinks.show();

    if ((entitlement.displayLocation === 'GAMES' || entitlement.displayLocation === 'GAMES|EXPANSIONS') && onlineStatus.onlineState)
    {
        var $showHideLink = $details.find('a.hide-game');
        $showHideLink.html((!game.isHidden)? this.hideText : this.showText);
    }

    // Actions (uninstall)
    var $uninstallLink = $details.find('a.uninstall');
    if (game.isInstalled && game.isUninstallable && !entitlement.playing)
    {
        $uninstallLink.show();
    }
    else
    {
        $uninstallLink.hide();
    }

    $removeEntitlementLink = $details.find('a.remove-entitlement');
    if(!game.entitlement.isEntitledFromSubscription)
    {
        $removeEntitlementLink.hide();
    }
    else if (!onlineStatus.onlineState || game.entitlement.playing)
    {
        $removeEntitlementLink.addClass('disabled');
    }
    else
    {
        $removeEntitlementLink.removeClass('disabled');
    }
	
	$details.find('a.upgrade-backup-restore').toggle(game.entitlement.isUpgradeBackupSaveAvailable);

    if (game.isInstalled === false || entitlement.isBrowserGame)
    {
        $details.find('a.properties').hide();
    }

    $('#links').show();

    html = '';
    var multiLaunchTitles = entitlement.multiLaunchTitles;
    if (game.isInstalled && multiLaunchTitles.length > 1)
    {
        var editLink = "<a href=\"#\" onclick=\"entitlementManager.dialogs.showProperties('"+entitlement.productId+"')\" id=\"launchOptionsLink\">" + tr('ebisu_client_edit') + "</a>";
        html += '<dt>' + tr('ebisu_client_launch_options') + editLink + '</dt>';

        html += '<dd><div>';
        var defaultIndicator = entitlement.multiLaunchDefault != "" && entitlement.multiLaunchDefault != "show_all_titles" ? "<i>" + tr('ebisu_client_settings_default_prefix') + "</i> " : "";
        multiLaunchTitles[0] = defaultIndicator + multiLaunchTitles[0];
        if(multiLaunchTitles.length > 3)
        {
            html += multiLaunchTitles.slice(0,3).join("<br/>") + "<br/>";
            html += "<div id=\"hiddenMultiLaunchList\" style=\"display: none\">" + multiLaunchTitles.slice(3).join("<br/>") + "<br/>" + "</div>";
            html += "<button onclick=\"javascript:Origin.views.gameDetails.toggleMultiLaunchList()\" id=\"btnExpandList\">" + tr('ebisu_client_show_more') + "</button>";
        }
        else
        {
            html += "<div>" + multiLaunchTitles.join("<br/>") + "</div>";
        }
        html += '<hr/>';
        html += '</div></dd>';
    }
    var platformsSupported = entitlement.platformsSupported;
    var platformsCompatible = [];
    if (platformsSupported.length > 0)
    {
        for(var i = 0; i < platformsSupported.length; i++)
        {
            if(game.platformCompatibleStatus(platformsSupported[i]) === 'COMPATIBLE')
            {
                platformsCompatible.push(platformsSupported[i]);
            }
        }
        html += '<dt>' + tr('ebisu_client_platforms_supported') + '</dt><dd>' + platformsCompatible.join(", ") + '</dd>';
    }
    if (entitlement.lastPlayedDate)
    {
        html += '<dt>' + tr('ebisu_client_last_played_date') + '</dt><dd>' + dateFormat.formatLongDate(entitlement.lastPlayedDate) + '</dd>';
    }
    if (entitlement.entitleDate)
    {
        html += '<dt>' + tr('ebisu_client_added_to_my_library') + '</dt><dd>' + dateFormat.formatLongDate(entitlement.entitleDate) + '</dd>';
    }
    // Design does not want to show product code for free trials.
    if (entitlement.registrationCode && !game.isTrialType)
    {
        html += '<dt>' + tr('ebisu_client_product_code') + '</dt><dd class="registration-code">' + entitlement.registrationCode + '</dd>';
    }
    if (html !== this._moreInfoHtml)
    {
        this._moreInfoHtml = html;
        $details.find('> .info > .more-info > dl').html(html);
    }
    if (!html)
    {
        $details.find('> .info > .more-info > dl').empty().hide();
    }

    // Make sure columns don't have weird wrapping on detail tab
    var artHeight = $("#art-col").height();
    if (artHeight > 0)
    {
        var notificationHeight = $('#release-notifications').height() + $('#other-notifications').height();
        $('#links').css('min-height', (artHeight - notificationHeight + 30) + 'px');
    }

    // -----------------
    // Cloud Storage
    var $cloudStorage = $gameInfo.find('> .tabs-content > ul > li.cloud-storage');

    $cloudStorage.find('> .usage > h3 > strong').html(entitlement.title);

    //$cloudStorage.find('> form > label > :checkbox').();


    // ------------------
    // Expansions
    var expansions = $.merge([], Origin.model.getExpansions(game));
    expansions = sortExtraContent(expansions, this._expansionSortBy, this._expansionSortDirection, this._expansionSortTiebreaker);

    var $orphanExpansions = $('#main > #expansions-area > .expansions > .content > ul > li');
    $.each(expansions, $.proxy(function(idx, expansion)
    {
        var $expansionTile = $('#main > #expansions-area > .expansions > .content > ul > li[data-id="' + expansion.entitlement.id + '"]');
        if ($expansionTile.length === 0)
        {
            this.addExpansion(expansion);
        }
        else
        {
            $orphanExpansions = $orphanExpansions.not($expansionTile);
        }
    }, this));

    // Remove all the expansions that have disappeared
    $orphanExpansions.remove();

    // Remove any expansion sections with no content.
    var $expansionSections = $('#main > #expansions-area > .expansions');
    $expansionSections.each(function(idx, section) {
        if($(this).find('.content > ul > li').length === 0)
        {
            $(this).remove();
        }
    });

    // Detach, sort, and reattach expansion sections
    $expansionSections.detach().sort(sortExtraContentSections);
    $('#main > #expansions-area').append($expansionSections);

    // ------------------
    // Add Ons
    this.updateAddonsArea(game);

    // ------------------
    // Achievements

    if ( game.hasAchievements && originUser.socialAllowed && onlineStatus.onlineState ){
        $("#origin-point").text( game.achievementSet.earnedXp );
        $("#origin-point-total").text( game.achievementSet.totalXp );
        $("#achievements-completed").text( game.achievementSet.achieved );
        $("#achievements-total").text( game.achievementSet.total );


        $("#button-view-achievements").on("click", function( event ){
            event.preventDefault();
            clientNavigation.showAchievementSetDetails( game.entitlement.achievementSet );
        });

        $(".achievements").show();

    } else {

        $(".achievements").hide();

    }
};

GameDetailsView.prototype.toggleMultiLaunchList = function()
{
    var hiddenMultiLaunchListVisible = $('#hiddenMultiLaunchList').is(':hidden');
    if (hiddenMultiLaunchListVisible)
    {
        $("#hiddenMultiLaunchList").show();
        $("#btnExpandList").text(tr('ebisu_client_show_less'));
    }
    else
    {
        $("#hiddenMultiLaunchList").hide();
        $("#btnExpandList").text(tr('ebisu_client_show_more'));
    }
}

GameDetailsView.prototype.addExpansion = function(expansion)
{
    var displayName = tr("ebisu_client_expansions_uppercase");
    var groupKey = 'expansions';
    var sortOrder = DEFAULT_EXTRA_CONTENT_SECTION_SORT_ORDER_VALUE;

    var displayGroupInfo = expansion.entitlement.extraContentDisplayGroupInfo;
    if(displayGroupInfo !== null)
    {
        // If the content is assigned to a custom display group, use custom values.
        displayName = displayGroupInfo["displayName"];
        groupKey = displayGroupInfo["groupKey"];
        sortOrder = displayGroupInfo["sortOrderAscending"];
    }

    var $expansions = $('#main > #expansions-area > #' + groupKey + '.expansions');
    if($expansions.length === 0)
    {
        // If no existing custom group found, create one and insert it
        // (We'll worry about sorting groups later)
        $expansions = $('#main > #expansions-template').clone();

        $expansions.find('header > h2').text(displayName);
        $expansions.attr('id', groupKey);
        $expansions.attr('data-sort', sortOrder);

        $('#main > #expansions-area').append($expansions);
    }
    $expansions.show();

    var $expansionList = $expansions.find('.content > ul');
    var $tile = $(Origin.views.gameTile.getGameHtml(expansion)).appendTo($expansionList);
    var $operationController = $tile.find('> .otk-operation-controller-mini');

    // We're too small for normal presentation
    $tile.addClass("tiny");

    // Just so we can differentiate between full game tiles and expansion tiles.
    $tile.addClass("expansion-tile");

    // Add progreess bar widget
    $.otkOperationControllerMiniCreate($operationController, {showButtons:false});

    // Add image onerror event to revert to default image, and flag with bad boxart url
    $tile.find('> div.boxart-container > img.boxart').on('error', function(evt)
    {
        if ($(this).attr('src') !== Origin.views.gameTile.DEFAULT_BOXART_IMAGE)
        {
            Origin.BadImageCache.urlFailed($(this).attr('src'));

            // Marking the URL as bad will make effectiveBoxartUrl return the next one
            var effectiveUrl = expansion.effectiveBoxartUrl;
            var url = effectiveUrl ? effectiveUrl : Origin.views.gameTile.DEFAULT_BOXART_IMAGE;
            $(this).attr('src', url);
        }
    });

    this.onGameExpansionUpdated(expansion);
};

GameDetailsView.prototype.updateAddonsArea = function(game)
{
    var allAddons = $.merge([], Origin.model.getAddons(game));

    // Compile list of addon groups.
    var addonGroups = {};
    $.each(allAddons, function(idx, content)
    {
        var displayName = tr("ebisu_client_game_details_addons_and_bonuses_uppercase");
        var groupKey = "addons";
        var sortOrder = DEFAULT_EXTRA_CONTENT_SECTION_SORT_ORDER_VALUE;

        var displayGroupInfo = content.entitlement.extraContentDisplayGroupInfo;
        if(displayGroupInfo !== null)
        {
            // If the content is assigned to a custom display group, use custom values.
            displayName = displayGroupInfo["displayName"];
            groupKey = displayGroupInfo["groupKey"];
            sortOrder = displayGroupInfo["sortOrderAscending"];
        }

        if(addonGroups[groupKey] === undefined)
        {
            addonGroups[groupKey] = { "displayName": displayName, "sortOrder": sortOrder, "addons": [] };
        }

        addonGroups[groupKey]["addons"].push(content);
    });

    // Create the table for each addon group.
    $.each(addonGroups, $.proxy(function(groupKey, value)
    {
        var displayName = addonGroups[groupKey]["displayName"];
        var sortOrder = addonGroups[groupKey]["sortOrder"];
        var addons = addonGroups[groupKey]["addons"];

        var html = '';
        var $operationController = '';
        var $addons = $('#main > #addons-area > #' + groupKey + '.addons');

        // If our sortBy/sortDirection maps don't contain information on this addon group, use default values.
        var sortBy = this._addonSortBy[groupKey] !== undefined ? this._addonSortBy[groupKey] : DEFAULT_EXTRA_CONTENT_SORT_BY;
        var sortDirection = this._addonSortDirection[groupKey] !== undefined ? this._addonSortDirection[groupKey] : DEFAULT_EXTRA_CONTENT_SORT_DIRECTION;

        addons = sortExtraContent(addons, sortBy, sortDirection, this._addonSortTiebreaker);

        $.each(addons, $.proxy(function(idx, content)
        {
            html += this.getAddonHtml(idx, content);
        }, this));

        if (html !== this._addonsHtml[groupKey])
        {
            this._addonsHtml[groupKey] = html;

            if($addons.length === 0)
            {
                // If no existing custom group found, create one and insert it
                // (We'll worry about sorting groups later)
                $addons = $('#main > #addons-template').clone();

                $addons.find('thead > tr > th.title > h2').text(displayName);
                $addons.attr('id', groupKey);
                $addons.attr('data-sort', sortOrder);

                $('#main > #addons-area').append($addons);
            }
            $addons.show();
            $addons.find('tbody').html(html);
            $operationController = $addons.find('.otk-operation-controller-mini:not([data-id])');
            $.otkOperationControllerMiniCreate($operationController, {showButtons:false});

            //$addons.find('.button:primary buy').attr('disabled', 'disabled');

            $.each($addons.find('tbody tr .addon-title div'), function(id, item)
            {
                if(item.scrollWidth > item.clientWidth) $(item).attr('title', $(item).text());
            });

            $addons.on('contextmenu', 'tbody > tr', function(evt)
            {
                var addon = game.getAddonById($(this).attr('data-id'));
                if (Origin.GameContextMenuManager.popup(addon))
                {
                    evt.stopImmediatePropagation();
                    evt.preventDefault();
                }
            });

            $.each(addons, $.proxy(function(idx, addon)
            {
                var associations = addon.entitlement.associations;

                // Bundle of addons
                if (associations && associations.length > 0)
                {
                    for (var i = 0; i < associations.length; i++)
                    {
                        var $association = associations[i];
                        if ($association)
                        {
                            // Add image onerror event to hide the cell, and flag with bad thumbnail url
                            var $thumbnailCell = $('#main > #addons-area > table.addons > tbody > tr > td > table.addon-description > tbody > tr > td.thumbnail[data-id="' + $association.id + '"]');
                            var $descriptionCell = $('#main > #addons-area > table.addons > tbody > tr > td > table.addon-description > tbody > tr > td.addon-description[data-id="' + $association.id + '"]');
                            $thumbnailCell.find('> img.thumbnail').on('error', function(evt)
                            {
                                // Mark the URL as bad
                                Origin.BadImageCache.urlFailed($(this).attr('src'));

                                var validThumbnailFound = false;
                                var thumbnailUrls = $association.thumbnailUrls;
                                if (thumbnailUrls)
                                {
                                    for (var j = 0; j < thumbnailUrls.length; j++)
                                    {
                                        if (thumbnailUrls[j].toString().length > 0)
                                        {
                                            if (Origin.BadImageCache.shouldTryUrl(thumbnailUrls[j]))
                                            {
                                                $(this).attr('src', thumbnailUrls[j]);
                                                validThumbnailFound = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if(validThumbnailFound === false)
                                {
                                    $thumbnailCell.remove();
                                    $descriptionCell.attr('colspan', '2');
                                }
                            });
                        }
                    }
                }
                // Standalone addon
                else
                {
                    // Add image onerror event to hide the cell, and flag with bad thumbnail url
                    var $thumbnailCell = $('#main > #addons-area > table.addons > tbody > tr > td > table.addon-description > tbody > tr > td.thumbnail[data-id="' + addon.entitlement.id + '"]');
                    $thumbnailCell.find('> img.thumbnail').on('error', function(evt)
                    {
                        // Marking the URL as bad will make effectiveThumbnailUrl return the next one
                        Origin.BadImageCache.urlFailed($(this).attr('src'));
                        var newSrc = addon.effectiveThumbnailUrl;

                        if(newSrc !== null)
                        {
                            $(this).attr('src', newSrc);
                        }
                        else
                        {
                            $thumbnailCell.remove();
                        }
                    });
                }
            }, this));

            $.each(addons, $.proxy(function(idx, addon)
            {
                // Fill out the correct status
                this.onGameAddonUpdated(addon);
            }, this));
        }

        if(!onlineStatus.onlineState)
        {
            $addons.find('.buy').attr('disabled', 'disabled');
            $addons.find('.download').attr('disabled', 'disabled');
            $addons.find('.placeholder-button').attr('disabled', 'disabled');
            $addons.find('.install-parent').attr('disabled', 'disabled');
            $addons.find('.upgrade-parent').attr('disabled', 'disabled');
        }
        else
        {
            $addons.find('.buy').removeAttr('disabled', 'disabled');
            $addons.find('.download').removeAttr('disabled', 'disabled');
            $addons.find('.install-parent').removeAttr('disabled', 'disabled');
            $addons.find('.upgrade-parent').removeAttr('disabled', 'disabled');
        }

        if (!html)
        {
            $addons.hide();
        }
    }, this));

    // Remove any addon sections with no content.
    var $addonSections = $('#main > #addons-area > table.addons');
    $addonSections.each($.proxy(function(idx, section) {
        var groupKey = $(this).attr('id');
        if(addonGroups[groupKey] === undefined)
        {
            $(this).remove();
            delete this._addonsHtml[groupKey];
        }
    }, this));

    // Detach, sort, and reattach addons sections
    $addonSections.detach().sort(sortExtraContentSections);
    $('#main > #addons-area').append($addonSections);
};

GameDetailsView.prototype.getExpandedHtml = function(addon)
{
    var buttonClasses;
    var html = '<tr class="addon-description" data-id-desc=' + addon.entitlement.productId +'><td colspan="3" style="padding-top:20px">';
    html += '<table class="addon-description">';

    var associations = addon.entitlement.associations;
	var subscriptionActive = subscriptionManager.isActive;
    // The addon is a bundle
    if (associations && addon.entitlement.associations.length > 0)
    {
        var associations = addon.entitlement.associations;
        // Can't use addon.effectiveThumbnailUrl here because a bundled item may already be owned or only available via bundle.
        // In these scenarios, we would not have access to a 'game' object for the entitlement.
        for (var i = 0; i < associations.length; i++)
        {
            if (associations[i])
            {
                html += '<tr>';
                var thumbnailUrls = associations[i].thumbnailUrls;
                if (thumbnailUrls)
                {
                    for (var j = 0; j < thumbnailUrls.length; j++)
                    {
                        if (thumbnailUrls[j].toString().length > 0)
                        {
                            if (Origin.BadImageCache.shouldTryUrl(thumbnailUrls[j]))
                            {
                                html += '<td data-id="' + associations[i].id + '" class="thumbnail"><img class="thumbnail" src="' + thumbnailUrls[j] + '" alt=""></img></td>';
                                break;
                            }
                        }
                    }
                }

                html += '<td data-id="' + associations[i].id + '" class="addon-description"><h3>' + associations[i].title + '</h3>' + associations[i].longDescription;

                if(associations[i].isEntitledFromSubscription)
                {
                    switch (subscriptionManager.status)
                    {
                        case ('EXPIRED'):
                        case ('DISABLED'):
                        {
                            html += '<p/><b>' + tr('Subs_OGD_AddOn_ExpandedView_ExpiredMessage') + '<b>';
        
                            if(!onlineStatus.onlineState)
                            {
                                buttonClasses = 'disabled';
                            }
                            html += '<p/><a class="buy ' + buttonClasses + '">' + tr('ebisu_client_buy_now') + '</a>';
                            html += ' | <a class="renew-subs-membership ' + buttonClasses + '">' + tr('Subs_Library_HoverCard_Renew') + '</a>';
        
                            if(!onlineStatus.onlineState || !addon.isUninstallable)
                            {
                                buttonClasses = 'disabled';
                            }
                            html += ' | <a class="uninstall ' + buttonClasses + '">' + tr('ebisu_client_uninstall') + '</a>';
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
                if(Origin.views.currentPlatform() === 'MAC' && associations[i].purchasable && !associations[i].owned && associations[i].isAvailableFromSubscription)
                {
                    console.log('showing ' + associations[i].title);
                    html += '<p/><b>' + tr('ebisu_subs_windows_only_purchasable_notification') + '<b>';
                }

                html += '</td></tr>';
            }
        }
    }
    // The addon is not a bundle
    else
    {
        if(addon.effectiveThumbnailUrl)
        {
            html += '<td data-id="' + addon.entitlement.id + '" class="thumbnail"><img class="thumbnail" src="' + addon.effectiveThumbnailUrl + '" alt=""></img></td>';
        }
        html += '<td class="addon-description">' + addon.entitlement.longDescription;
        if(addon.entitlement.isEntitledFromSubscription)
        {
            // - Subscription Membership Ending -
            switch (subscriptionManager.status)
            {
                case ('EXPIRED'):
                case ('DISABLED'):
                {
                    html += '<p/><b>' + tr('Subs_OGD_AddOn_ExpandedView_ExpiredMessage') + '<b>';
        
                    if(!onlineStatus.onlineState)
                    {
                        buttonClasses = 'disabled';
                    }
                    html += '<p/><a class="buy ' + buttonClasses + '">' + tr('ebisu_client_buy_now') + '</a>';
                    html += ' | <a class="renew-subs-membership ' + buttonClasses + '">' + tr('Subs_Library_HoverCard_Renew') + '</a>';
        
                    if(!onlineStatus.onlineState || !addon.isUninstallable)
                    {
                        buttonClasses = 'disabled';
                    }
                    html += ' | <a class="uninstall ' + buttonClasses + '">' + tr('ebisu_client_uninstall') + '</a>';
                }
                default:
                {
                    break;
                }
            }
        }
        
        if(Origin.views.currentPlatform() === 'MAC' && addon.entitlement.purchasable && !addon.entitlement.owned && addon.entitlement.isAvailableFromSubscription)
        {
            console.log('showing ' + addon.entitlement.title);
            html += '<p/><b>' + tr('ebisu_subs_windows_only_purchasable_notification') + '<b>';
        }
        
        html += '</td>';
    }

    html += '</tr></table>';
    html += '</td></tr>';

    return html;
}

GameDetailsView.prototype.getAddonHtml = function(idx, addon)
{
    var description = true; // set this to be whether or not the addon has a description
    var preAnnouncementDate = addon.entitlement.preAnnouncementDisplayString;
    var showUnlockDate = false;
    // if there is an unlock date, check whether it should be shown
    if (addon.entitlement.displayUnlockStartDate) {
        if (addon.entitlement.releaseStatus === 'AVAILABLE')
        {
            showUnlockDate = true;
        }
        else {
            // I admit, I'm not going to take a leap year into account. We'll get over that.
            var nextYearDate = new Date();
            nextYearDate.setFullYear(nextYearDate.getFullYear() + 1);
            var unlockDateMS = addon.entitlement.displayUnlockStartDate.getTime();
            var nextYearMS = nextYearDate.getTime();
            var millisecondsPerYear = 24 * 60 * 60 * 1000 * 365;
            var diffInDates = nextYearMS - unlockDateMS;
            if (addon.entitlement.displayUnlockStartDate && (diffInDates > 0) && (diffInDates < millisecondsPerYear))
            {
                    showUnlockDate = true;
            }
        }
    }

    // now put up the right date
    var releaseDateTitle = '';
    var releaseDateText = '';
    var releaseDateHtml = '';
    if (preAnnouncementDate && preAnnouncementDate != 'undefined' && preAnnouncementDate.length > 0)
    {
        releaseDateTitle = preAnnouncementDate;
        releaseDateText = preAnnouncementDate;
    }
    else if (showUnlockDate)
    {
        releaseDateTitle = $('<span></span>').text(dateFormat.formatLongDateTimeWithWeekday(addon.entitlement.displayUnlockStartDate)).text();
        releaseDateText = dateFormat.formatLongDate(addon.entitlement.displayUnlockStartDate);
    }
    else if(addon.entitlement.releaseStatus !== 'AVAILABLE')
    {
        // If the game is unreleased and either is a year out or has a malformed or missing date show unreleased
        releaseDateText = tr('ebisu_client_unreleased');
    }

    if (releaseDateText)
    {
        if (releaseDateTitle)
        {
            releaseDateTitle = ' title="' + releaseDateTitle + '"';
        }
        releaseDateHtml = '<div class="addon-release-date"' + releaseDateTitle + '>' + tr('ebisu_client_game_details_release_date') + ': ' + releaseDateText + '</div>';
    }


    var titleHtml = '<td class="addon-title"><div>';
    if (addon.entitlement.testCode)
    {
        titleHtml += '(' + addon.entitlement.testCode + ') ';
    }
    if(window.clientSettings.readSetting("AddonPreviewAllowed") && !addon.entitlement.owned && !addon.entitlement.published)
    {
        titleHtml += '(HIDDEN) ';
    }
    titleHtml += addon.entitlement.title + '</div>' + releaseDateHtml + '</td>';

    var progressBar = '<div class="otk-operation-controller-mini"></div>';

    // This is stub HTML filled out by onGameAddonUpdated
    var status = '<div class="status"></div>';

    var hasDescription = addon.entitlement.longDescription.toString().length != 0;
    var expandButtonHtml = '';
    var expandedHtml = '';
    if (hasDescription)
    {
        expandButtonHtml = '<td class="addon-desc-expander">';
        expandButtonHtml += '<span class="addon-desc-expander-button expand" id="' + addon.entitlement.id + '"></span>';
        expandButtonHtml += '</td>';
        expandedHtml = this.getExpandedHtml(addon);
    }

    var html = '<tr data-id="' + addon.entitlement.id + '"' + ((idx % 2 === 0)? ' class="even"' : '') + '>'
        + '<td class="addon-title"><table class="addon-title"><tr data-id="' + addon.entitlement.id + '">'
        + expandButtonHtml + titleHtml
        + '</tr></table></td>'
        + '<td class="addon-price"></td>'
        + '<td>' + progressBar + status + '</td>'
        + '</tr>'
        + expandedHtml;

    return html;
};

GameDetailsView.prototype.updateFriendsCurrentlyPlaying = function()
{
    var $friends = $('#game-info > .tabs-content > ul > li.details > .info > .friends-info > .friends.playing > ul');

    var rosterView = this.playingRosterView;
    if (rosterView.contacts.length)
    {
        var html = '';
        $.each(rosterView.contacts, function(idx, contact)
        {
            html += contactHtml(contact);
        });
        $friends.html(html).parent().show();
    }
    else
    {
        $friends.empty().parent().hide();
    }
};

GameDetailsView.prototype.updateFriendsOwning = function()
{
    var $friends = $('#game-info > .tabs-content > ul > li.details > .info > .friends-info > .friends.owning > ul');

    var rosterView = this.owningRosterView;
    if (rosterView.contacts.length)
    {
        var html = '';
        $.each(rosterView.contacts, function(idx, contact)
        {
            html += contactHtml(contact);
        });
        $friends.html(html).parent().show();
    }
    else
    {
        $friends.empty().parent().hide();
    }
};


// ----------------
// Events

GameDetailsView.prototype.onOnlineStatusChange = function()
{
    this.updateGame(Origin.model.game);
};

GameDetailsView.prototype.onGameInitialized = function(game)
{
    var $operationController = $('#game-info > header > .action > .otk-operation-controller-mini');
    // Add image onerror event to revert to default image, and flag with bad boxart url
    $('#game-info > .tabs-content > ul > li.details > div > .boxart > div > img')
        .on('load', function(evt)
        {
            Origin.BadImageCache.urlSucceeded($(this).attr('src'));
            $(this).parent().parent().addClass('loaded');
        })
        .on('error', function(evt)
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

    // Background banner image
    var bannerUrl = game.effectiveBannerUrl;

    if (bannerUrl === null)
    {
        // We know there's no banner, set the default directly
        $("#main").css({"background-image": "url(" + DEFAULT_BANNER_IMAGE + ")"});
    }
    else
    {
        // Create a scout for our background image
        var $scoutImage = $("<img>");
        $scoutImage.attr("src", bannerUrl);

        $scoutImage
            .on('load', function(evt)
            {
                // It exists, set as the real background
                Origin.BadImageCache.urlSucceeded(bannerUrl);
                $("#main").css({"background-image": "url(" + bannerUrl + ")"});
                $scoutImage.remove();
            })
            .on('error', function(evt)
            {
                Origin.BadImageCache.urlFailed(bannerUrl);
                $("#main").css({"background-image": "url(" + DEFAULT_BANNER_IMAGE + ")"});
                $scoutImage.remove();
            });
    }

    if (window.originSocial)
    {
        // Initialize playing/owning views
        this.playingRosterView = originSocial.createPlayingRosterView(game.entitlement.productId);
        this.playingRosterView.changed.connect($.proxy(this.updateFriendsCurrentlyPlaying, this));
        this.updateFriendsCurrentlyPlaying();

        this.owningRosterView = originSocial.createOwningRosterView(game.entitlement.productId);
        this.owningRosterView.changed.connect($.proxy(this.updateFriendsOwning, this));
        this.updateFriendsOwning();
    }

    // ----------------------------
    // Top level game progress bar
    $.otkOperationControllerMiniCreate($operationController, {});
    $operationController.otkOperationControllerMini("buttonPlay").click(function () {
        if (Origin.model.game.entitlement)
        {
            telemetryClient.sendGamePlaySource(Origin.model.game.entitlement.productId, "gamedetails");
            Origin.model.game.entitlement.play();
        }
    });

    $operationController.otkOperationControllerMini("progressbar").on('click', function(evt) {
        evt.stopImmediatePropagation();
        clientNavigation.showDownloadProgressDialog();
    });

    this.updateGame(game);

    game.entitlement.fetchUnownedContentPricing();
};

GameDetailsView.prototype.onGameUpdated = function(game)
{
    this.updateGame(game);
};

GameDetailsView.prototype.onGamesChanged = function(game)
{
    this.updateGame(game);
};

GameDetailsView.prototype.onGamePDLCAdded = function(pdlc)
{
    this.updateGame(Origin.model.game);

    if (!pdlc.entitlement.owned && pdlc.entitlement.purchasable)
    {
        Origin.model.game.entitlement.fetchUnownedContentPricing();
    }
};

GameDetailsView.prototype.onGamePDLCRemoved = function(pdlc)
{
    this.updateGame(Origin.model.game);
};

GameDetailsView.prototype.onGameExpansionUpdated = function(expansion)
{
    var $tile = $('#main > #expansions-area > .expansions > .content > ul > li[data-id="' + expansion.entitlement.id + '"]');
    Origin.views.gameTile.updateGameHtml($tile, expansion);
};


GameDetailsView.prototype.setAddonSortField = function(id, field)
{
    if (this._addonSortBy[id] != field)
    {
        this._addonSortBy[id] = field;
        this._addonSortDirection[id] = DEFAULT_EXTRA_CONTENT_SORT_DIRECTION;
    }
    else
    {
        // Flip the sort
        this._addonSortDirection[id] *= -1;
    }

    // Refresh the game
    this.updateAddonsArea(Origin.model.game);
};

GameDetailsView.prototype.getAddonStatus = function(addon)
{
    var entitlement = addon.entitlement;

    if(addon.platformCompatibleStatus() === 'INCOMPATIBLE_PLATFORM')
    {
        switch(entitlement.platformsSupported[0])
        {
            case 'MAC':
                return tr('ebisu_client_mac_only');
            case 'PC':
                return tr('ebisu_client_pc_only');
            default:
                return tr('ebisu_client_platform_available_only_on_other');
        }
    }

    // If the user is on Windows
    // If user has a subscription, and game is avaliable for that reason subscription, but hasn't been redeemed yet...
    if (!entitlement.owned && subscriptionManager.isActive && entitlement.isAvailableFromSubscription && !entitlement.isEntitledFromSubscription)
    {
        return '<button class="primary upgrade-parent"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + tr('ebisu_client_free_games_download_client_button') + '</button>';
    }
    else if (!entitlement.owned && entitlement.purchasable)
    {
        return '<button class="primary buy"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + addon.buyNowVerb + '</button>';
    }
    else if (addon.isPlayable)
    {
        return '<button class="play primary">' + tr('ebisu_client_play') + '</button>';
    }
    else if (addon.entitlement.installed === true)
    {
        // Installed check must happen before downloadable because the cached install state might be invalid
        // until we check it again.  Installed addons will always fail the downloadable check.
        return tr('ebisu_client_installed');
    }
    else if (addon.isDownloadable)
    {
        // Do this before the release status tests as it's possible to have a downloadable
        // unreleased title if permissions overrides are in effect (1102/1103)
        return '<button class="primary download">' + addon.downloadVerb + '</button>';
    }
    else if ((addon.entitlement.parent.downloadable ||
        addon.entitlement.parent.downloadOperation ||
        addon.entitlement.parent.installOperation ||
        addon.entitlement.parent.installable) &&
        (addon.entitlement.releaseStatus !== 'UNRELEASED' ||
        addon.entitlement.downloadDateBypassed))
    {
        // EBIBUGS-20596
        // If parent game is not installed, show nothing for PULC to prevent confusion.
        return (addon.entitlement.isPULC) ? '' : '<button class="primary install-parent">' + addon.downloadVerb + '</button>';
    }
    //we need to check release status before parent status.
    else if (entitlement.releaseStatus == 'UNRELEASED')
    {
        return tr('ebisu_client_unreleased');
    }
    else if (entitlement.releaseStatus == 'EXPIRED')
    {
        return tr('ebisu_client_expired');
    }
    // Make sure isPreloaded is checked before isInstalled as preloaded implies installed
    else if (addon.isPreloaded)
    {
        return tr('ebisu_client_preloaded');
    }
    else if (addon.isInstallable)
    {
        return '<button class="install primary">' + tr('ebisu_client_install') + '</button>';
    }
    // If parent is installed, show any PULC as "installed" to not confuse the user
    else if (addon.entitlement.isPULC && addon.entitlement.parent.installed === true)
    {
        return tr('ebisu_client_installed');
    }
    else if (!onlineStatus.onlineState)
    {
        // Write in a disabled downloadVerb button
        return '<button class="primary placeholder-button">' + addon.downloadVerb + '</button>';
    }

    return '';
};

GameDetailsView.prototype.onGameAddonUpdated = function(addon)
{
    var entitlement = addon.entitlement;
    var $tr = $('#main > #addons-area > .addons > tbody > tr[data-id="' + entitlement.id + '"]');

    var $progressBarContainer = $tr.find('> td > .otk-operation-controller-mini');
    var $status = $tr.find('> td > .status');

    if (Origin.util.updateGameProgressBar(addon, $progressBarContainer, false))
    {
        $status.hide();
        $progressBarContainer.show();
    }
    else
    {
        $status.show().html(this.getAddonStatus(addon));
        $progressBarContainer.hide();
    }

    $tr.find('> td.addon-price').html(this.getAddonPrice(addon));
};

GameDetailsView.prototype.getAddonPrice = function(addon)
{
    var priceHtml = '';

    if (addon && addon.isPurchasable)
    {
        var entitlement = addon.entitlement;
        var priceDescription = entitlement.priceDescription;
        var originalPrice = entitlement.originalPrice;

        priceHtml += '<span class="addon-price">';
        priceHtml += '<span class="addon-current-price">' + entitlement.currentPrice + '</span>';
        if (originalPrice)
        {
            priceHtml += '<span class="addon-original-price">' + originalPrice + '</span>';
        }
        priceHtml += '</span>';

        if (priceDescription)
        {
            priceHtml += '<span class=addon-price-description>' + priceDescription + '</span>';
        }
    }

    console.log(priceHtml);
    return priceHtml;
};

GameDetailsView.prototype.onGameCloudSavesCurrentUsageChange = function(game, currentUsage)
{

};

GameDetailsView.prototype.onGameAddonCloudSavesCurrentUsageChange = function(addon, currentUsage)
{

};

GameDetailsView.prototype.onGameCloudSavesChange = function(game)
{
    var cloudSaves = game.entitlement.cloudSaves;

    var $cloudStorage = $('#game-info > .tabs-content > ul > li.cloud-storage');

    var percent = Math.round((cloudSaves.currentUsage / cloudSaves.maximumUsage) * 100);

    var $usage = $cloudStorage.find('> .usage > .cloud-storage-usage');
    $usage.find('> .fill').css('width', percent + '%');

    if (cloudSaves.currentUsage !== null)
    {
        var formattingBytes = dateFormat.formatBytes;
        $usage.find('> .amount').html(formattingBytes(cloudSaves.currentUsage) + " " + tr('cc_bytes_of_bytes') + " " + formattingBytes(cloudSaves.maximumUsage));
    }
    else
    {
        $usage.find('> .amount').html('');
    }

    if (percent < 99)
    {
        $cloudStorage.find('> .usage > .warning').hide();
    }
    else
    {
        $cloudStorage.find('> .usage > .warning').slideDown('fast');
    }

    if (cloudSaves.lastBackup)
    {
        $cloudStorage.find('> .backup > button').removeAttr('disabled');
        $cloudStorage.find('> .backup > .last-modified').html(tr('ebisu_client_cloud_restore_last_modified').replace('%1', dateFormat.formatLongDateTime(cloudSaves.lastBackup.created))).show();
    }
    else
    {
        $cloudStorage.find('> .backup > button').attr('disabled', 'disabled');
        $cloudStorage.find('> .backup > .last-modified').hide();
    }
};

GameDetailsView.prototype.onGameAddonCloudSavesChange = function(addon)
{
    var cloudSaves = addon.entitlement.cloudSaves;

    // TODO - implement
};

GameDetailsView.prototype.onUpdateSettings = function()
{
    $('#game-info > .tabs-content > ul > li.cloud-storage > .usage > form > label > :checkbox').prop("checked", clientSettings.readSetting("cloud_saves_enabled"));
};

GameDetailsView.prototype.handlePageClick = function(game, $btn)
{
    if ($btn.parent().parent().hasClass('type-expires'))
    {
        if ($btn.hasClass('update'))
        {
            telemetryClient.sendMyGamesDetailsUpdateClick(game.entitlement.productId);
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
        else if ($btn.hasClass('install'))
        {
            game.install();
        }
        else if ($btn.hasClass('uninstall'))
        {
            game.uninstall();
        }
        else if ($btn.hasClass('update'))
        {
           telemetryClient.sendMyGamesDetailsUpdateClick(game.entitlement.productId);
            game.installUpdate();
        }
        else if ($btn.hasClass('view'))
        {
            game.gotoDetails();
        }
        else if ($btn.hasClass('upgrade'))
        {
            game.entitlement.upgrade();
        }
        else if ($btn.hasClass('renew-subs-membership'))
        {
            telemetryClient.sendSubscriptionUpsell('gamedetails');
            clientNavigation.showMyAccountPath('Subscription');
        }
        else if ($btn.hasClass('remove-entitlement'))
        {
            game.removeFromLibrary();
        }
        else if ($btn.hasClass('go-online'))
        {
            onlineStatus.requestOnlineMode();
        }
        else if ($btn.hasClass('buy'))
        {
            if( Origin.model.game.isEarlyAccessType )
            {
                var expired = ((game.secondsUntilTermination === 0) ? '1' : '0');
                telemetryClient.sendFreeTrialPurchase(game.entitlement.productId, expired, 'earlyaccess-gamedetails');
            }
            else if( Origin.model.game.isTrialType )
            {
                var expired = ((game.secondsUntilTermination === 0) ? '1' : '0');
                telemetryClient.sendFreeTrialPurchase(game.entitlement.productId, expired, 'gamedetails');
            }

            game.viewStoreMasterTitlePage();
        }
        else if ($btn.hasClass('trial-view-offer'))
        {
            var promoType = game.secondsUntilTermination === 0 ? "FreeTrialExpired" : "FreeTrialExited";
            var scope = "OwnedGameDetails";
            clientNavigation.showPromoManager(game.entitlement.productId, promoType, scope);
        }
    }
}

GameDetailsView.prototype.getHovercardHtml = function(game)
{
    return Origin.util.getGameHovercardHtml(game, true);
};

// Expose to the world
Origin.views.gameDetails = new GameDetailsView();

})(jQuery);
