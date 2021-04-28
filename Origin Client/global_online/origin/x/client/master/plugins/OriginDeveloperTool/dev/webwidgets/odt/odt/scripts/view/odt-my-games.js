;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var MyGamesOverrideViews = function()
{
	this._myGamesSortBy = Origin.model.constants.DEFAULT_MYGAMES_SORT_BY;
	this._myGamesSortDirection = Origin.model.constants.DEFAULT_MYGAMES_SORTING_DIRECTION;
	
	$('#my-games > table').find('> tbody').on('click', 'tr', function(event)
    {
		Origin.views.gameDetailsOverride.init($(this).attr('data-id'));
        Origin.views.navbar.currentTab = "game-details";
	});
	
	$('#btn-add-game').on('click', function(event)
    {
        Origin.views.addGameOverride.init();
        Origin.views.navbar.currentTab = "add-game";
	});
};

MyGamesOverrideViews.prototype.init = function()
{
	var $myGames = $('#mygames-list > tbody');
	var ownedGames = [];

	$.each(window.entitlementManager.topLevelEntitlements, $.proxy(function(index, entitlement)
    {
        ownedGames.push(entitlement);
	}, this));
		
	this.sortMyGames(ownedGames);

	$myGames.html('');
    
	var html = '';
    $.each(ownedGames, $.proxy(function(index, entitlement)
    {
        html += this.renderEachGame(entitlement, true);
    }, this));
	$myGames.html(html);
	Origin.util.badImageRender();

	if(!html)
    {
		$myGames.parent().hide();
	}
    
    var unownedIds = window.originDeveloper.unownedIds;
    if(unownedIds.length > 0)
    {
        var unownedQuery = productQuery.startQuery(unownedIds);
        unownedQuery.succeeded.connect(function(products)
        {
            console.info('Product query succeeded for ' + window.originDeveloper.unownedIds.join(', '));
            
            var unownedGames = [];        
            $.each(products, function(id, product)
            {
                unownedGames.push(product);
            });
            
            Origin.views.myGamesOverrides.renderUnownedGames(unownedGames);
        });
        
        unownedQuery.failed.connect(function(ids)
        {
            console.info('Product query failed for ' + ids.join(', '));
            
            var unownedGames = [];
            $.each(ids, function(id, productId)
            {
                var product = $.extend({}, Origin.model.constants.DUMMY_ENTITLEMENT);
                product.productId = productId;
                unownedGames.push(product);
            });
            
            Origin.views.myGamesOverrides.renderUnownedGames(unownedGames);
        });
    }
    else
    {
        $('#unowned-games-list > tbody').html('');
    }
};

MyGamesOverrideViews.prototype.sortMyGames = function(games)
{
	// for now we are only sorting by title
	games.sort(function(a,b){
		var a1= a.title, b1= b.title;
    	if(a1== b1) return 0;
   		return a1> b1? 1: -1;
	});
};

MyGamesOverrideViews.prototype.renderUnownedGames = function(games)
{
    var $unownedGames = $('#unowned-games-list > tbody');
    $unownedGames.html('');
            
    this.sortMyGames(games);
    console.info(games);
    
    var unownedHtml = '';
    $.each(games, $.proxy(function(id, game)
    {
        unownedHtml += this.renderEachGame(game, false);
    }, this));
    $unownedGames.html(unownedHtml);

    this.removeGameEventDelegation();
	Origin.util.badImageRender();
};

MyGamesOverrideViews.prototype.renderEachGame = function(entitlement, isOwned)
{
    var boxartUrl = entitlement.boxartUrls.length > 0 ? entitlement.boxartUrls[0] : Origin.model.constants.DEFAULT_IMAGE_PATH;
	var boxartHtml = '<div> <img class="boxart" src="'+ boxartUrl +'" title="'+ entitlement.title +'"/>';
	
	var contentIdHtml = '<div><span class="title">Content ID</span><span class="value">'+ entitlement.contentId +'</span></div>';
	
	var offerIdHtml = '<div><span class="title">Offer ID</span><span class="value">'+ entitlement.productId +'</span></div>';
	
	var itemNumHtml = '<div><span class="title">Item Number</span><span class="value">'+ entitlement.expansionId +'</span></div>';
    
    var platforms = (entitlement.platformsSupported === undefined)? '' : entitlement.platformsSupported.join(', ');
	var platformHtml = '<div><span class="title">Platforms</span><span class="value">'+ platforms +'</span></div>';
	
	var removeGameHtml = '<a data-id="' + entitlement.productId + '" id="btn-remove-game" href><span></span></a>';
	
	var supLangHtml = '';
	if(entitlement.localesSupported.length > 0)
    {
        supLangHtml = '<dt> Supported Languages </dt><dd>'+entitlement.localesSupported.join(', ')+'</dd>';
    }
    
	var overridesHtml = '';
    var overrides = [];    
    if (window.originCommon.readOverride('[Connection]', 'OverrideSNOFolder', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideSNOFolder', entitlement.contentId) !== '')
    {
        overrides.push('Shared Network Location');
    }
    else if (window.originCommon.readOverride('[Connection]', 'OverrideBuildIdentifier', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideBuildIdentifier', entitlement.contentId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideBuildReleaseVersion', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideBuildReleaseVersion', entitlement.contentId) !== '')
    {
        overrides.push('Build Identifier');
    }
    else if (window.originCommon.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.contentId) !== '')
    {
        overrides.push('Download');
    }

    if (window.originCommon.readOverride('[Connection]', 'OverrideDownloadSyncPackagePath', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideDownloadSyncPackagePath', entitlement.contentId) !== '')
    {
        overrides.push('Sync Package');
    }
    if (window.originCommon.readOverride('[Connection]', 'OverrideVersion', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideVersion', entitlement.contentId) !== '')
    {
        overrides.push('Download Version');
    }
    if (window.originCommon.readOverride('[Connection]', 'OverrideInstallFilename', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideInstallFilename', entitlement.contentId) !== '')
    {
        overrides.push('Installer');
    }
    if (window.originCommon.readOverride('[Connection]', 'OverrideInstallRegistryKey', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideInstallRegistryKey', entitlement.contentId) !== '')
    {
        overrides.push('Install Registry Key');
    }
    if (window.originCommon.readOverride('[Connection]', 'OverrideExecuteRegistryKey', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideExecuteRegistryKey', entitlement.contentId) !== '')
    {
        overrides.push('Execute Registry Key');
    }
    if (window.originCommon.readOverride('[Connection]', 'OverrideExecuteFilename', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[Connection]', 'OverrideExecuteFilename', entitlement.contentId) !== '')
    {
        overrides.push('Executable');
    }
    if (window.originCommon.readOverride('[QA]', 'DisableTwitchBlacklist', entitlement.productId) !== '' ||
        window.originCommon.readOverride('[QA]', 'DisableTwitchBlacklist', entitlement.contentId) !== '')
    {
        overrides.push('Twitch Blacklist');
    }   
	(overrides.length === 0) ? overridesHtml = '' : overridesHtml = '<dt class="overridden"> Override(s) Currently Set </dt><dd class="overridden">'+overrides.join(', ')+'</dd>';
	
	var titleHtml = '<div> <h2>'+ entitlement.title +'</h2>' 
				+	'<dl>'
				+	supLangHtml
				+	overridesHtml
				+	'</dl>'
				+ 	'</div>';
	
	var data = '<tr data-id="' + entitlement.productId + '">'
		+ '<td class="mygames-boxart">' + boxartHtml + '</td>' 
		+ '<td class="mygames-title"  colspan="2">' + titleHtml + '</td>'
		+ '<td class="mygames-content-id">' + contentIdHtml + '</td>'
		+ '<td class="mygames-offer-id">' + offerIdHtml + '</td>'
		+ '<td class="mygames-item-id">' + itemNumHtml + '</td>'
		+ '<td class="mygames-platforms">' + (isOwned ? platformHtml : removeGameHtml) + '</td>'
		+ '</tr>';
	return data;
};			

MyGamesOverrideViews.prototype.removeGameEventDelegation = function()
{
    $("#unowned-games-list").find("#btn-remove-game").each(function()
    {
        $(this).on('click', function(event)
        {
            event.preventDefault();
            event.stopImmediatePropagation();
            
            var parent = $(this).parents('tr');
            var id = $(this).attr('data-id');
            window.originDeveloper.removeContent(id);
            parent.hide('slow');
        });
    });
};

Origin.views.myGamesOverrides = new MyGamesOverrideViews();

})(jQuery);
