;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

Origin.views.gameTile = {};

Origin.views.gameTile.DEFAULT_BOXART_IMAGE = './mygames/images/card/default-boxart.jpg';
Origin.views.gameTile.NOG_BACKGROUND_IMAGE = './mygames/images/card/default-non-origin-boxart.png';

Origin.views.gameTile.getGameHtml = function(game)
{
    var src = game.effectiveBoxartUrl || Origin.views.gameTile.DEFAULT_BOXART_IMAGE;
    var html = '<li data-id="' + game.entitlement.id + '"' + ' data-title="' + game.entitlement.title + '" class="game' + ((game.isPlayable)? ' playable' : '') + '">';
    html += '<div class="boxart-container">';
    html += '<img class="boxart" src="' + src + '" alt="" />';
    html += '</div>';
    html += '<div class="blend-overlay"></div>';
    html += '<div class="violator"></div>';
    html += '<div class="otk-operation-controller-mini"></div>';
    html += '</li>';
    return html;
};

Origin.views.gameTile.updateGameHtml = function($tile, game, containerWidth, containerHeight)
{
    if (containerWidth == null)
    {
        containerWidth = $tile.width();
    }
    if (containerHeight == null)
    {
        containerHeight = $tile.height();
    }

    var entitlement = game.entitlement;
    var className = null;
    
    // Boxart
    var src = game.effectiveBoxartUrl || Origin.views.gameTile.DEFAULT_BOXART_IMAGE;
    var $img = $tile.find('> div.boxart-container > img.boxart');
    if (src !== $img.attr('src'))
    {
        $img.attr('src', src);
    }
    game.addBoxartTransformations(containerWidth, containerHeight, $img);

    // Violators
    // First, reset the violators
    $tile.removeClass('has-violator').find('.violator')[0].className = 'violator';
    var notifications = game.getNotifications();
    var ignoreHash = { 'preloaded': 1, 'preload-date': 1, 'available-date': 1, 'internal-debug': 1, 'offline' : 1, 'platform-none' : 1, 'platform-pc' : 1, 'platform-mac' : 1, 'non-origin-game' : 1, 'preannouncement-date' : 1, 'dynamic-content' : 1, 'dynamic-content-playable-at' : 1, 'preview-content' : 1, 'release-coming' : 1, 'subs-windows-only' : 1, 'subs-windows-only-purchasable' : 1};
    var violators = $.grep(notifications, function(notification, idx){ return (!ignoreHash[notification.type]); });
    for(var i = 0; i < violators.length; i++)
    {
        // Play takes priority over all classes.
        if (violators[i].type === 'playable')
        {
            className = violators[i].type;
            break;
        }
    }

    // Consider games added in the last 72 hours as new (20 minutes for trials)
    var isNew = game.isNewType;
    
    var isUnowned = !game.entitlement.owned && game.entitlement.purchasable;

    var entitleDate = entitlement.entitleDate;

    if(className)
    {
        $tile.addClass('has-violator').find('.violator').addClass(className);
    }
    // If there are any violators, then determine what type of violator to show
    else if (entitlement.testCode !== null || isNew || isUnowned || violators.length > 0)
    {
        if (entitlement.testCode !== null)
        {
            className = 'internal-debug';
        }
        else if (isNew)
        {
            className = 'new';
        }
        else if (isUnowned)
        {
            className = entitlement.newlyPublished ? 'new-dlc' : 'unowned-content';
        }
        else
        {
            className = 'generic';
            if (violators.length === 1)
            {
                switch (violators[0].type)
                {
                    case ('offline'):
                    {
                        className = 'offline';
                        break;
                    }
                    case ('expires'):
                    {
                        className = 'expires';
                        break;
                    }
                    case ('timed-trial-admin-disable'):
                    case ('timed-trial-expired'):
                    case ('trial-expired'):
                    {
                        className = 'trial-expired';
                        break;
                    }
                    case ('early-access'):
                    case ('trial'):
                    {
                        className = 'trial';
                        break;
                    }
                    case ('demo'):
                    {
                        if (game.entitlement.itemSubType === 'BETA')
                        {
                            className = 'beta';
                        }
                        else
                        {
                            className = 'demo';
                        }
                        break;
                    }
                    case ('update-available'):
                    {
                        className = 'update-available';
                        break;
                    }
                    case ('upgrade-available-base-only'):
                    {
                        className = 'upgrade-available-base-only';
                        break;
                    }
                    case ('upgrade-available-dlc-only'):
                    {
                        className = 'upgrade-available-dlc-only';
                        break;
                    }
                    case ('upgrade-available-base-dlc'):
                    {
                        className = 'upgrade-available-base-dlc';
                        break;
                    }
                    case ('subs-membership-expired'):
                    {
                        className = 'subs-membership-expired';
                        break;
                    }
                    case ('subs-membership-expiring'):
                    {
                        className = 'subs-membership-expiring';
                        break;
                    }
                    case ('subs-entitlement-retired'):
                    {
                        className = 'subs-entitlement-retired';
                        break;
                    }
                    case ('subs-entitlement-retiring'):
                    {
                        className = 'subs-entitlement-retiring';
                        break;
                    }
                    case ('new-dlc-available'):
                    {
                        className = 'new-dlc-available';
                        break;
                    }
                    case ('new-dlc'):
                    {
                        className = 'new-dlc';
                        break;
                    }
                    case ('playable'):
                    {
                        className = 'playable';
                        break;
                    }
                    default:
                    {
                        console.log('MyGamesView.updateGame:: Warning! 1 violator, but no icon available for type "' + violators[0].type + '".');
                        break;
                    }
                }
            }
            else
            {
                if (violators.length <= 5)
                {
                    className = 'multi-' + violators.length;
                }
                else
                {
                    console.log('MyGamesView.updateGame:: Warning! More than 5 violators, no icon available.');
                }
            }
        }
        $tile.addClass('has-violator').find('.violator').addClass(className);
    }
    
    var progressBar = $tile.find('> .otk-operation-controller-mini');
    var isActive = Origin.util.updateGameProgressBar(game, progressBar);

    var isPlaying = game.entitlement.playing;
    var isPlayable = game.isPlayable;
    var isDisabled = false;

    if($tile.hasClass('expansion-tile'))
    {
        // OFM-10570: Don't disable the tile if it's PULC and owned.
        if(game.entitlement.isPULC && game.entitlement.owned)
        {
            isDisabled = false;
        }
        // OFM-8891: Show disabled tile for expansions that aren't installed
        else if(!game.entitlement.installed || !game.entitlement.owned)
        {
            isDisabled = true;
        }
        // OFM-4373: DLC game tiles should be disabled if the content is unreleased., unless they are downloadable, downloading, or playable.  
        // Note that in some cases, clicking Download/Play on the expansion tile will download/play the base game.
        else if(game.isUnreleased && !isPlayable && !isActive && !game.isDownloadable && !game.entitlement.parent.downloadable)
        {
            isDisabled = true;
        }
    }
    else
    {
        // If we have no default action and nothing is happening show as disabled
        isDisabled = !isActive && !isPlayable && !game.isDownloadable && !game.isInstallable && !isPlaying;
    }

    // lets do one more check if its not disabled
    // Sometimes games are downloadable but not playable on this platform so disable to not confuse user
    if(!isDisabled)
        isDisabled = game.platformCompatibleStatus() !== 'COMPATIBLE';
        
    if(!isDisabled)
        isDisabled = !game.entitlement.playableBitSet;

    // "playing" is used purely for automation. Do not remove without talking
    // to them
    $tile.toggleClass('playing', isPlaying)
         .toggleClass('playable', isPlayable)
         .toggleClass('disabled', isDisabled);
};

})(jQuery);
