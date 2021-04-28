(function($){
"use strict";

var $hud, $history, hudVisible = false, lastSetWidth = null;

function initConversationHUD(chatPartner)
{
    var $joinGameButton, $joinBroadcastButton;

    $hud = $('div#hud');
    $history = $('ol#history');

    $('<span class="game-name">').appendTo($hud);

    $joinBroadcastButton = $('<button class="primary join-broadcast">').appendTo($hud);
    $joinGameButton = $('<button class="primary join-game">').appendTo($hud);

    $('<span class="join-broadcast">')
        .text(tr('ebisu_client_watch'))
        .appendTo($joinBroadcastButton);

    $('<span class="join-game">')
        .text(tr('ebisu_client_action_join'))
        .appendTo($joinGameButton);

    // Handle clicking the game name
    $hud.on('click', 'span.game-name[data-product-id]', function(evt)
    {
        var productId = $(this).attr('data-product-id');
        window.clientNavigation.showStoreProductPage(productId);
        return false;
    });

    // Handle clicking the join game button
    $hud.on('click', 'button.join-game', function(evt)
    {
        chatPartner.joinGame();
        return false;
    });
    
    // Handle clicking the join broadcast button
    $hud.on('click', 'button.join-broadcast', function(evt)
    {
        telemetryClient.sendBroadcastJoined('ConversationHUD');
        clientNavigation.launchExternalBrowser(chatPartner.playingGame.broadcastUrl);
        return false;
    });
}

function showConversationHUD(show)
{
    hudVisible = show;
    $hud.toggle(show);

    if (show)
    {
        resizeConversationHUD();
    }
}

function updateConversationHUD(playingGame)
{
    var $gameName, $joinButton, $broadcastButton;

    $gameName = $hud.children('span.game-name');
    $joinButton = $hud.children('button.join-game');
    $broadcastButton = $hud.children('button.join-broadcast');

    if (playingGame !== null)
    {
        $gameName.text(playingGame.gameTitle);
        // Only make this clickable if we have a product ID and its not embargoed
        if(playingGame.productId 
           && !(playingGame.gameTitle == tr('ebisu_client_notranslate_embargomode_title')))
        {
            $gameName.attr('data-product-id', playingGame.productId);
        }
        $joinButton.toggle(playingGame.joinable);
        $broadcastButton.toggle(playingGame.broadcastUrl);
    }
    else
    {
        $gameName.text("");
    }
}

function resizeConversationHUD()
{
    if (hudVisible)
    {
        // Account for the width of the scrollbar if present
        var scrollbarWidth = $history.width() - $history[0].scrollWidth;

        // Avoid a style recalc if we can
        if (scrollbarWidth !== lastSetWidth)
        {
            $hud.css('margin-right', scrollbarWidth+'px');
            lastSetWidth = scrollbarWidth;
        }
    }
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.initConversationHUD = initConversationHUD;
window.Origin.views.showConversationHUD = showConversationHUD;
window.Origin.views.updateConversationHUD = updateConversationHUD;
window.Origin.views.resizeConversationHUD = resizeConversationHUD;

}(Zepto));
