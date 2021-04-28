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

    $('#mygames-list').find('> tbody').on('click', 'tr', function(event) {
        Origin.views.buildViewer.init($(this).attr('data-id'));
        Origin.views.navbar.currentTab = "build-viewer";
    });

    $('#btn-logout').on('click', shiftQuery.logoutAsync);
};

MyGamesOverrideViews.prototype.init = function()
{
    var $myGames = $('#mygames-list > tbody');
    var games = [];

    $.each(entitlementManager.topLevelEntitlements, $.proxy(function(index, entitlement)
    {
        games.push(entitlement);
    }, this));

    this.sortMyGames(games);

    $myGames.html('');

    var html = '';
    $.each(games, $.proxy(function(index, entitlement)
    {
        html += this.renderEachGame(entitlement);
    }, this));
    $myGames.html(html);
    Origin.util.badImageRender();

    if(!html)
    {
        $myGames.parent().hide();
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

MyGamesOverrideViews.prototype.renderEachGame = function(entitlement)
{
    var overrides = [];
    if (originCommon.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.productId) !== '' ||
        originCommon.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.contentId) !== '')
    {
        overrides.push('Download');
    }
    if (originCommon.readOverride('[Connection]', 'OverrideBuildIdentifier', entitlement.productId) !== '' ||
        originCommon.readOverride('[Connection]', 'OverrideBuildIdentifier', entitlement.contentId) !== '' ||
        originCommon.readOverride('[Connection]', 'OverrideBuildReleaseVersion', entitlement.productId) !== '' ||
        originCommon.readOverride('[Connection]', 'OverrideBuildReleaseVersion', entitlement.contentId) !== '')
    {
        overrides.push('Build Identifier');
    }
    
    var boxartUrl = entitlement.boxartUrls.length > 0 ? entitlement.boxartUrls[0] : Origin.model.constants.DEFAULT_IMAGE_PATH;
    var boxartHtml = '<div> <img class="boxart" src="'+ boxartUrl +'" title="'+ entitlement.title +'"/>';
    var overridesHtml = (overrides.length === 0) ? '' : '<dt class="overridden"> Override(s) Currently Set </dt><dd class="overridden">'+overrides.join(', ')+'</dd>';
    var titleHtml = '<div> <h2>' + entitlement.title + '</h2> <dl>' + overridesHtml + '</dl> </div>';
    var offerIdHtml = '<div><span class="title">Offer ID</span><span class="value">'+ entitlement.productId +'</span></div>';

    var data = '<tr data-id="' + entitlement.productId + '">'
        + '<td class="mygames-boxart">' + boxartHtml + '</td>'
        + '<td class="mygames-title"  colspan="2">' + titleHtml + '</td>'
        + '<td class="mygames-offer-id">' + offerIdHtml + '</td>'
        + '</tr>';
    return data;
};

Origin.views.myGamesOverrides = new MyGamesOverrideViews();

})(jQuery);
