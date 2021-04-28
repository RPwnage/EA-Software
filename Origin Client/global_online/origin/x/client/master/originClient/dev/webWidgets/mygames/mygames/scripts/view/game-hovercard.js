;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.util) { Origin.util = {}; }

Origin.util.getGameHovercardHtml = function(game, expansionView)
{
    function contactHtml(contact) 
    {
        var html = '';
        html += '<li data-tooltip="' + $('<span></span>').text(contact.nickname).text() + '" data-id="' + contact.id + '">';
        html += '<img src="' + contact.avatarUrl + '" alt="" />';
        html += '</li>';

        return html;
    }

    var html = '<header>';
    var notificationLinkClasses = '';
    var showPrice = true;
	var owned = game.entitlement.owned;
	// This is the true state of owned. However, it's too risky to fix it at this point.
	if(owned)
	{
		if(game.entitlement.isEntitledFromSubscription && !subscriptionManager.isActive)
		{
			owned = false;
		}
	}

    // Cut up the title in case it's overly long with no white-space
    function wbr(str, num) 
    {
        return str.replace(RegExp("(\\w{" + num + "})(\\w)", "g"), function(all,text,char)
        {
            return text + "<wbr>" + char;
        });
    };
	
	function sortByCompletionDate(achievement1, achievement2) {

	  // organized by not hidden and hidden
	  // achieved and not achieved
	  // complete date

		if ( achievement1.xp == "--" && achievement2.xp != "--" ) 
		{
			return 1;
		} else if ( achievement1.xp != "--" && achievement2.xp == "--" )
		{
			return -1;
		} else if ( achievement1.xp == "--" && achievement2.xp == "--" ) 
		{
			return 0;
		} else if ( achievement1.xp != "--" && achievement2.xp != "--" ) 
		{

			if ( achievement1.achieved && !achievement2.achieved )
			{
				return -1;
			} else if ( !achievement1.achieved && achievement2.achieved )
			{
				return 1;
			} else if ( !achievement1.achieved && !achievement2.achieved ) 
			{

				if ( achievement1.name.toLowerCase() > achievement2.name.toLowerCase() ) return 1;
				if ( achievement1.name.toLowerCase() < achievement2.name.toLowerCase() ) return -1;
				return 0;

			} else if ( achievement1.achieved && achievement2.achieved ) 
			{

				if ( achievement1.updateTime < achievement2.updateTime ) return 1;
				if ( achievement1.updateTime > achievement2.updateTime ) return -1;
				return 0;

			}

		}

	};	

    var title = wbr(game.entitlement.title, 12);
    if (expansionView && window.clientSettings.readSetting("AddonPreviewAllowed") && !game.entitlement.owned && !game.entitlement.published)
    {
           title = '(HIDDEN) '+ title;
    }
    
    // This calculation is shared for the (i) button and the game title on hovercard
    var gameDetailsLinkType = "none";
    
    // No details page for non-Origin game so we don't generate a hyperlinked title
    if(!game.entitlement.nonOriginGame)
    {
        if(game.entitlement.owned)
        {
            // Only get an game details link for owned games or games+expansions displayLocation
            if(game.entitlement.displayLocation === 'GAMES' || game.entitlement.displayLocation === 'GAMES|EXPANSIONS')
            {
                if(game.platformCompatibleStatus() === 'INCOMPATIBLE_SUBS' && game.entitlement.purchasable)
                {
                    gameDetailsLinkType = "none";
                }
                else
                {
                    gameDetailsLinkType = "game-details-page";
                }
            }
        }
        else
        {
            gameDetailsLinkType = "view-in-store";
        }
    }    
    
    if(gameDetailsLinkType === "game-details-page")
    {
        html += '<h3><a href="' + game.getDetailsUrl() + '">' + title + '</a></h3>';
    }
    else if(gameDetailsLinkType === "view-in-store")
    {
        html += '<h3><a class="view-in-store" href="#">' + title + '</a></h3>'; 
    }
    else if(gameDetailsLinkType === "show-pdlc-store")
    {
        html += '<h3><a class="show-pdlc-store" href="#">' + title + '</a></h3>';
    }
    else
    {
        html += '<h2>' + title + '</h2>';
    }

    if (!expansionView && onlineStatus.onlineState)
    {
        html += '<button title="';
        if (game.isFavorite)
        {
            html += $('<span></span>').text(tr('ebisu_client_unfavorite_tooltip')).text() + '"';
            html += ' class="favorite"';
        }
        else
        {
            html += $('<span></span>').text(tr('ebisu_client_favorite_tooltip')).text() + '"';
        }
        html += '></button>';
    }

    html += '</header>';
    
    // Notifications
    var notifications = game.getNotifications();
    var currentOperation = game.entitlement.currentOperation;

    if (notifications.length > 0 || game.isUnreleased || expansionView)
    {
        html += '<ul class="notifications">';
        $.each(notifications, $.proxy(function(idx, notification)
        {
            var classes = 'type-' + (notification.type || 'generic') + ((notification.dismissable)? ' dismissable' : '');
            html += '<li class="' + classes + '" data-key="' + notification.key + '">';
            html += '<div class="message">';
			var viewOfferButtonHtml = '';
            switch (notification.type)
            {
                case ('offline'):
                {
                    html += tr('ebisu_client_cant_download_offline');
                    break;
                }
                    
                case ('dynamic-content-playable-at'):
                {
                    html += tr('ebisu_client_play_game_while_downloading_launch_at_X').replace('%1', Math.floor(currentOperation.playableAt * 100));
                    break;
                }
                    
                case ('playable'):
                {
                    html += tr('ebisu_client_play_game_while_downloading_play_now');
                    break;
                }
                    
                case ('dynamic-content'):
                {
                    html += tr('ebisu_client_play_game_while_downloading');
                    break;
                }

                // Non-Origin Game
                case ('non-origin-game'):
                {
                    html += tr('ebisu_client_non_origin_shortcut');
                    break;
                }

                // Platforms
                case ('platform-pc'):
                {
                    html += tr('ebisu_client_platform_available_only_on_windows');
                    break;
                }
                case ('platform-mac'):
                {
                    html += tr('ebisu_client_platform_available_only_on_osx');
                    break;
                }
                case ('platform-none'):
                {
                    html += tr('ebisu_client_platform_available_only_on_other');
                    break;
                }

                case ('internal-debug'):
                {
                    html += '<h4>' + tr('ebisu_client_notranslate_internal_debug_info') + '</h4><ul>';

                    // Only show summary - the rest lives on the game details page
                    $.each(notification.data, function(key, value)
                    {
                        html += '<li>' + key + ': ' + value + '</li>';
                    });

                    html += '</ul>';
                    break;
                }
                // Expiration
                // TODO - QUESTION verify conditionals
                case ('expires'):
                {
                    if (notification.seconds > 0)
                    {
                        html += tr('ebisu_client_expires_in').replace('%1', dateFormat.formatInterval(notification.seconds));
                    }
                    else
                    {
                        html += tr('ebisu_client_expired'); // TODO - QUESTION what is this message supposed to be?
                    }
                    break;
                }
                // Free trials
                case ('trial'):
                {
					// Only display "view special offer" option when we're online
					if (onlineStatus.onlineState)
					{
						viewOfferButtonHtml = '<br/><button class="trial-view-offer">' + tr('ebisu_client_view_special_offer') + '</button>';
					}

                    //we can have a null case for limited trials
                    if(notification.seconds !== null)
                        html += tr('ebisu_client_trial_expires_in').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'HOURS')) + viewOfferButtonHtml;
                    else
                        html += tr('ebisu_free_trial_not_started') + viewOfferButtonHtml; 
                        
                    break;
                }
                // Expired free trials
                case ('trial-expired'):
                {
                    html += tr('ebisu_client_trial_expired'); 

                    if (game.isHidden)
                    {
                        html += '<br/><button class="unhide-game">' + tr('ebisu_client_unhide_game') + '</button>';
                    }
                    else
                    {
                        html += '<br/><button class="hide-game">' + tr('ebisu_client_hide_game_in_library') + '</button>';
                    }

                    if (game.isInstalled)
                    {
                        html += ' | <button class="uninstall" ' + (game.entitlement.playing ? 'disabled="disabled"' : '') + '>' + tr('ebisu_client_uninstall') + '</button>';
                    }
                    break;
                }
                // timed trial
                case ('early-access'):
                {
					// Only display "view special offer" option when we're online
					if (onlineStatus.onlineState)
					{
                        viewOfferButtonHtml = '<br/><button class="buy">' + tr('ebisu_subs_trial_hovercard_upsell') + '</button>';
					}

                    //we can have a null case for limited trials
                    if(notification.seconds !== null)
                        html += tr('ebisu_subs_trial_time_expire1').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'HOURS')) + viewOfferButtonHtml;
                    else
                        html += tr('ebisu_subs_trial_hovercard_not_started') + viewOfferButtonHtml; 
                        
                    break;
                }
                // timed trial admin disable
                case ('timed-trial-admin-disable'):
                {
                    html += tr('ebisu_subs_trial_unavailable'); 

                    // Only display "view special offer" option when we're online
					if (onlineStatus.onlineState)
					{
                        html += '<br/><button class="buy">' + tr('ebisu_client_view_in_store') + '</button> | ';
					}
					
					var entitlementsBaseByMasterTitleId = entitlementManager.getBaseEntitlementsByMasterTitleId(game.entitlement.masterTitleId);
					console.log(entitlementsBaseByMasterTitleId);
					// If game is installed and there isn't a preorder or full version avaliable, show uinstall option. Otherwise, show hide.
                    if(game.entitlement.installed && entitlementsBaseByMasterTitleId.length === 1)
					{
                        html += '<button class="uninstall" ' + (game.entitlement.playing ? 'disabled="disabled"' : '') + '>' + tr('ebisu_client_uninstall') + '</button>';
					}
					else
					{
						if (game.isHidden)
						{
							html += '<br/><button class="unhide-game">' + tr('ebisu_client_unhide_game') + '</button>';
						}
						else
						{
							html += '<br/><button class="hide-game">' + tr('ebisu_client_hide_game_in_library') + '</button>';
						}
					}
                    
                    break;
                }
                // Expired timed trial
                case ('timed-trial-expired'):
                {
                    html += tr('ebisu_subs_trial_hovercard_trial_expired'); 

                    // Only display "view special offer" option when we're online
					if (onlineStatus.onlineState)
					{
                        html += '<br/><button class="buy">' + tr('ebisu_client_view_in_store') + '</button> | ';
					}
                    
                    if (game.isHidden)
                    {
                        html += '<br/><button class="unhide-game">' + tr('ebisu_client_unhide_game') + '</button>';
                    }
                    else
                    {
                        html += '<br/><button class="hide-game">' + tr('ebisu_client_hide_game_in_library') + '</button>';
                    }
                    break;
                }
                // timed trial offline
                case ('timed-trial-offline'):
                {
                    html += tr('ebisu_subs_trial_goonline_notification');
                    break;
                }
                // Demo that hasn't expired
                case ('demo'):
                {
                    html += tr('ebisu_client_try_before_you_buy');
                    
                    if (game.isPlayable)
                    {
                        html += '<br/><button class="play">' + tr('ebisu_client_free_games_play_demo') + '</button>';
                    }
                    else if(game.isDownloadable)
                    {
                        html += '<br/><button class="download">' + tr('ebisu_client_free_games_download_demo') + '</button>';
                    }
                    break;
                }
                // Expired alpha/beta/demo
                case ('alpha-beta-expired'):
                case ('demo-expired'):
                {
                    html += tr('ebisu_client_expired'); 

                    if (game.isHidden)
                    {
                        html += '<br/><button class="unhide-game">' + tr('ebisu_client_unhide_game') + '</button>';
                    }
                    else
                    {
                        html += '<br/><button class="hide-game">' + tr('ebisu_client_hide_game_in_library') + '</button>';
                    }

                    if (game.isInstalled)
                    {
                        html += ' | <button class="uninstall" ' + (game.entitlement.playing ? 'disabled="disabled"' : '') + '>' + tr('ebisu_client_uninstall') + '</button>';
                    }
                    break;
                }
                // Subscription is Windows only
                case ('subs-windows-only'):
                {
                    html += tr('ebisu_subs_edition_windows_only_notification');
                    break;
                }
                case ('subs-windows-only-purchasable'):
                {
                    html += tr('ebisu_subs_windows_only_purchasable_notification');
                    break;
                }
                // Subscription membership ending
                case ('subs-membership-expired'):
                case ('subs-membership-expiring'):
                {
                    if(notification.seconds)
                    {
                        html += tr('Subs_Library_HoverCard_Expires_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS'));
                    }
                    else
                    {
                        html += tr('Subs_Membership_Expired');
                    }
                    
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += '<br/><button class="renew-subs-membership ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Renew') + '</button>';
                    if(notification.type === 'subs-membership-expired')
                    {
                        html += ' | <button class="buy ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Buy') + '</button>';
                    }
                    break;
                }
                // Subscription entitlement expiring
                case ('subs-entitlement-retired'):
                case ('subs-entitlement-retiring'):
                {
                    if(notification.seconds)
                    {
                        html += tr('Subs_Library_HoverCard_VaultExpire_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS'));
                    }
                    else
                    {
                        html += tr('Subs_Library_HoverCard_VaultExpire_Notification');
                    }
                    
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += '<br/><button class="buy ' + notificationLinkClasses + '">' + tr('Subs_Library_HoverCard_Buy') + '</button>';

                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState || game.entitlement.playing)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += ' | <button class="remove-entitlement ' + notificationLinkClasses + '">' + tr('ebisu_client_remove_from_library') + '</button>'
                    break;
                }
                // Subscription offline play is expiring
                case ('subs-offline-play-expiring'):
                {
                    html += tr('Subs_Library_HoverCard_Offline_Expires_Countdown').replace('%1', dateFormat.formatInterval(notification.seconds, 'MINUTES', 'DAYS'));
                    break;
                }
                // Subscription offline play expired
                case ('subs-offline-play-expired'):
                {
                    html += tr('Subs_Library_HoverCard_Offline_Expired_Notification');
                    break;
                }
                // Preload available
                case ('preload-available'):
                {
                    html += tr('ebisu_client_can_now_preload') + '<br/><button class="download">' + game.downloadVerb + '</button>';
                    break;
                }
                case ('preloaded'):
                {
                    html += tr('ebisu_client_game_is_preloaded');
                    break;
                }
                // Preload date
                case ('preload-date'):
                {
                    html += '<span class="label">' + tr('ebisu_client_preload') + '</span> <span class="value">' + dateFormat.formatLongDateTime(notification.date) + '</span>';
                    break;
                }
                // Available date
                case ('available-date'):
                {
                    html += '<span class="label">' + tr('ebisu_client_available') + '</span> <span class="value">' + dateFormat.formatLongDateTime(notification.date) + '</span>';
                    break;
                }
                // Update available
                case ('update-available'):
                {
                    html += tr('ebisu_client_game_update_available') + '<br/><button class="update">' + tr('ebisu_client_update_now') + '</button>';
                    break;
                }
                // Subscription Upgrade available
                case ('upgrade-available-base-only'):
                {
                    html += tr('ebisu_client_subs_upgrade_edition');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    break;
                }
                case ('upgrade-available-dlc-only'):
                {
                    html += tr('ebisu_client_subs_upgrade_expansions');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    break;
                }
                case ('upgrade-available-base-dlc'):
                {
                    html += tr('ebisu_client_subs_upgrade_edition_expansions');
                    notificationLinkClasses = '';
                    if(!onlineStatus.onlineState)
                    {
                        notificationLinkClasses = 'disabled';
                    }
                    html += '<br/><button class="upgrade ' + notificationLinkClasses + '">' + tr('Subs_Upgrade_Action') + '</button>';
                    break;
                }
                // New DLC available
                case ('new-dlc-available'):
                {
                    html += tr('ebisu_client_new_dlc_notification');
                    break;
                }
                // New DLC
                case ('new-dlc'):
                {
                    html += tr('ebisu_client_new_dlc_notification') + '<br/><button class="view-in-store">' + tr('ebisu_client_view_in_store') + '</button>';
                    break;
                }
                // Unowned content available
                case ('unowned-content-available'):
                {
                    html += tr('ebisu_client_hovercard_dlc_available');
                    break;
                }

                // Just released
                // Custom Icon? No
                case ('just-released'):
                {
                    // TODO - what does this look like?
                    html += tr('ebisu_client_just_released');
                    break;
                }

                //Application is located within the Trash
                case ('trashed'):
                {
                    html += tr('ebisu_client_hovercard_game_trashed');
                    break;
                }
                
                case ('preannouncement-date'):
                {
                    html += '<span class="label">' + notification.date + '</span>';
                    break;
                }

                // Game is not playable because the user doesn't have bit spec requirements
                case ('64-only'):
                {
                    // We can just list the current platform. This notification won't show up if the
                    // game isn't compatible with the user's current OS.
                    var platform = Origin.views.currentPlatform() === "PC" ? tr('ebisu_client_windows') : tr('ebisu_client_osx');
                    html += tr('ebisu_client_64_only').replace('%1', platform);
                    break;
                }
                
                // Preview content is content that may be downloaded without an entitlement.
                case ('preview-content'):
                {
                    html += game.isInstalled ? tr('ebisu_client_preview_content_description_installed') : tr('ebisu_client_preview_content_description_downloadable');
                    
                    if(game.isDownloadable)
                    {
                        html += '<br/><button class="download">' + game.downloadVerb + '</button>'
                    }
                    else if(game.isInstallable)
                    {
                        html += '<br/><button class="install">' + tr('ebisu_client_install') + '</button>'
                    }
                    else if (game.isInstalled && game.isUninstallable)
                    {
                        html += '<br/><button class="uninstall" ' + (game.entitlement.playing ? 'disabled="disabled"' : '') + '>' + tr('ebisu_client_uninstall') + '</button>';
                    }
                    break;
                }

                case ('release-coming'):
                {
                    html += '<span class="label">' + tr('ebisu_client_not_yet_released') + '</span>';
                    if(game.entitlement.isPreorder)
                    {
                        html += '<span class="value">' + tr('ebisu_client_not_yet_released_text').replace('%1', game.entitlement.title) + '</span>';
                    }
                }
                
                default:
                {
                    html += notification.message || '';
                    break;
                }
            }
            html += '</div>';
            if (notification.dismissable)
            {
                html += '<button>x</button>';
            }
            html += '</li>';
        }, game));
        html += '</ul>';
    }
    
    var infoHtml = '';

    // Description for expansions
    var description = game.entitlement.shortDescription;

    if (expansionView && description)
    {      
        infoHtml += '<div class="description' + (description.length > 120 ? ' multi-line' : '') + '" data-id="' + game.entitlement.id + '">' + description + '</div>';                    
    }

    // Last played date
    var displayHoursPlayed = Math.floor(game.totalSecondsPlayed / (60 * 60));

    if (game.entitlement.lastPlayedDate)
    {
        infoHtml += '<div class="info-fixed-data last-played-date"><span class="label">' + tr('ebisu_client_last_played') + '</span> <span class="value">' + dateFormat.formatLongDate(game.entitlement.lastPlayedDate) + '</span></div>';
    }

    // Total hours played
    if (displayHoursPlayed > 0)
    {
        var hoursString;

        if (displayHoursPlayed === 1)
        {
            hoursString = tr("ebisu_client_interval_hour_singular").replace("%1", 1);
        }
        else
        {
            hoursString = tr("ebisu_client_interval_hour_plural").replace("%1", displayHoursPlayed);
        }

        infoHtml += '<div class="info-fixed-data"><span class="label">' + tr('ebisu_client_time_played') + '</span> <span class="value">' + hoursString + '</span></div>';
    }
    
    // Friends playing
    if (window.originSocial && onlineStatus.onlineState )
    {
        var playingView = originSocial.createPlayingRosterView(game.entitlement.productId);
        if (playingView.contacts.length)
        {
            infoHtml += '<div class="friends playing">'
                + '<h3> ' + tr('ebisu_client_currently_playing') + '</h3>'
                + '<ul>';
            $.each(playingView.contacts, function(idx, contact)
            {
                infoHtml += contactHtml(contact);
            });
            infoHtml += '</ul>'
                 + '</div>';
        }
        
        // Friends owning
        var owningView = originSocial.createOwningRosterView(game.entitlement.productId);
        if (owningView.contacts.length )
        {
            infoHtml += '<div class="friends owning">'
                + '<h3> ' + tr('ebisu_client_friends_who_play') + '</h3>'
                + '<ul>';
            $.each(owningView.contacts.slice(0,14), function(idx, contact)
            {
                infoHtml += contactHtml(contact);
            });
            infoHtml += '</ul>'
                 + '</div>';
        }
    }
	
    // Info (body)
    if (infoHtml !== '')
    {
        html += '<div class="info">';
        html += infoHtml;
        html += '</div>';
    }

	if ( game.hasAchievements && onlineStatus.onlineState && originUser.commerceAllowed )
	{
		var achievementHtml = '';	
		achievementHtml += '<div class="info">';
		achievementHtml += '<div class="achievements">';		
		achievementHtml += '<h3>' + tr('ebisu_client_achievements') + '</h3>';	;
		achievementHtml += '<a href="#" class="link-view-achievements" data-id="' + game.achievementId + '">' + tr('ebisu_client_achievements_album_viewall') + '</a>';

		var achievements = game.achievements;
		achievements.sort( sortByCompletionDate );

		achievementHtml += '<ul>';
		
		$(achievements).each( function( index, achievement ){
		    if ( index > 6 ){ return false; }
			achievementHtml += '<li data-id="' + game.achievementId + '" class="';
			achievementHtml += achievement.achieved ? 'achieved' : 'unachieved';
			achievementHtml += '"><a href="#" class="" ><img src="' + achievement.getImageUrl(40) +'" alt="' + achievement.name + '" /></a></li>';
		});
		
		achievementHtml += '</ul>';
		achievementHtml += '</div>';
		achievementHtml += '</div>';	

		html += achievementHtml;
	}
	
	


    // Addons only have hovercards for debug purposes; it's not possible to act on them through the hovercard
    if (game.entitlement.displayLocation !== 'ADDONS')
    {
        // Actions
        html += '<footer>';
        html += '<table class="footer-table"><tr><td>';
        
        if (game.isPurchasable)
        {
            var currentPrice = game.entitlement.currentPrice;
            var originalPrice = game.entitlement.originalPrice;
            var priceDesc = game.entitlement.priceDescription;

            if (currentPrice)
            {
                html += '<span class="current-price-value">' + currentPrice + '</span>';
                if (priceDesc)
                {
                    html += '<span class="current-price-description">' + priceDesc + '</span>';
                }
            }

            if (originalPrice)
            {
                html += '<br><span class="original-price-value">' + originalPrice + '</span>';
                if (priceDesc)
                {
                    html += '<span class="original-price-description">' + priceDesc + '</span>';
                }
            }

            html += '</td></tr><tr><td>';
        }

        if(game.entitlement.isEntitledFromSubscription
			// ORSUBS-1642 We want to show the logo for timed trial objects.
		   || game.entitlement.itemSubType === 'TIMED_TRIAL_ACCESS')
        {
            html += '<div class="footer-subscriptions"></div>';
        }

        if(gameDetailsLinkType === "game-details-page")
        {
            html += '<button class="more-details secondary"' + (!onlineStatus.onlineState && expansionView ? ' disabled="disabled"' : '') + ' title="' + tr('ebisu_client_more_details') + '">i</button>';
        }
        else if(gameDetailsLinkType === "view-in-store")
        {
            html += '<button class="more-details secondary view-in-store"' + (!onlineStatus.onlineState && expansionView ? ' disabled="disabled"' : '') + ' title="' + tr('ebisu_client_more_details') + '">i</button>';
        }
        else if(gameDetailsLinkType === "show-pdlc-store")
        {
            html += '<button class="more-details secondary pdlc-store-button"' + (!onlineStatus.onlineState && expansionView ? ' disabled="disabled"' : '') + ' title="' + tr('ebisu_client_more_details') + '">i</button>';
        }

        if (game.isTrialType && game.secondsUntilTermination === 0)
        {
            // Trial is expired, ignore install flow state and prompt user to view the offer.
            html += '<button class="trial-view-offer primary"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + tr('ebisu_client_view_special_offer') + '</button>';
        }
        else if (game.isDemoType && game.secondsUntilExpiration === 0)
        {
            // Demo has expired
            // Future: Buy game action
        }
        else if (game.inTrash)
        {
            html += '<button class="restore primary">' + tr('ebisu_client_restore') + '</button>';
        }
        else if(game.platformCompatibleStatus() === 'INCOMPATIBLE_SUBS' && game.entitlement.purchasable)
        {
            html += '<button class="primary buyNow"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + game.buyNowVerb + '</button>';
        }
        else if (game.isPlayable)
        {
            html += '<button class="play primary">' + tr('ebisu_client_play') + '</button>';
        }
        else if (owned && game.isInstalled)
        {
            // We're installed but not playable
            html += '<button class="play primary" disabled="disabled">' + tr('ebisu_client_play') + '</button>';
        }
        else if (game.isDownloadable)
        {
            html += '<button class="download primary">' + game.downloadVerb + '</button>';
        }
        else if (game.isInstallable)
        {
            html += '<button class="install primary">' + tr('ebisu_client_install') + '</button>';
        }
        else if (owned && game.entitlement.parent && !currentOperation)
        {
            if(game.entitlement.isPULC)
            {
                html+= '<button class="primary" disabled="disabled">' + tr('ebisu_client_purchased') + '</button>';
            }
            else if(game.entitlement.releaseStatus !== 'UNRELEASED' || game.entitlement.downloadDateBypassed)
            {
                if(game.entitlement.parent.downloadable || game.entitlement.parent.installable)
                {
                    html+= '<button class="primary install-parent">' + game.downloadVerb + '</button>';
                }
                else
                {
                    html+= '<button class="primary install-parent" disabled="disabled">' + game.downloadVerb + '</button>';
                }
            }
        }
        else if(contentOperationQueueController.index(game.entitlement.productId) > 0)
        {
            html+= '<button class="primary skipqueue" ' + (contentOperationQueueController.queueSkippingEnabled(game.entitlement.productId) ? '' : 'disabled="disabled"') + '>' + tr('ebisu_client_download_now') + '</button>';
        }
        // See if an operation is happening
        else if (currentOperation)
        {
            if (currentOperation.pausable)
            {
                html += '<button class="primary pause">' + currentOperation.specificPhaseDisplay("PAUSE") + '</button>';
            }
            else if (currentOperation.resumable)
            {
                html += '<button class="primary resume">' + currentOperation.specificPhaseDisplay("RESUME") + '</button>';
            }
            else
            {
                html += '<button class="primary" disabled="disabled">' + currentOperation.phaseDisplay + '</button>';
            }
        }
        // If user has a subscription, and game is avaliable for that reason subscription, but hasn't been redeemed yet...
        else if(game.entitlement.displayLocation === 'EXPANSIONS' && subscriptionManager.isActive && game.entitlement.isAvailableFromSubscription && !game.entitlement.isEntitledFromSubscription)
        {
            html += '<button class="primary upgrade-parent"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + tr('ebisu_client_free_games_download_client_button') + '</button>';
        }
        else if (!owned && game.entitlement.purchasable)
        {
            html += '<button class="primary buyNow"' + (!onlineStatus.onlineState ? ' disabled="disabled"' : '') + '>' + game.buyNowVerb + '</button>';
        }

        html += '</td></tr></table>';
        html += '</footer>';
    }

    return html;
};

Origin.util.handleHovercardButtonClick = function(game, $btn)
{
    if ($btn.hasClass('buyNow'))
    {
        telemetryClient.sendHovercardBuyNowClick(game.entitlement.productId);
        game.purchase('hovercard_button');
    }
    else if ($btn.hasClass('buy'))
    {
        if( game.isEarlyAccessType )
        {
            var expired = ((game.secondsUntilTermination === 0) ? '1' : '0');
            telemetryClient.sendFreeTrialPurchase(game.entitlement.productId, expired, 'earlyaccess-hovercard');
        }
        else if( game.isTrialType )
        {
            var expired = ((game.secondsUntilTermination === 0) ? '1' : '0');
            telemetryClient.sendFreeTrialPurchase(game.entitlement.productId, expired, 'hovercard');
        }

        telemetryClient.sendHovercardBuyClick(game.entitlement.productId);
        game.viewStoreMasterTitlePage();
    }
    else if ($btn.hasClass('upgrade-parent'))
    {
        game.entitlement.upgrade();
    }
    else if ($btn.hasClass('trial-view-offer'))
    {
        var promoType = game.secondsUntilTermination === 0 ? "FreeTrialExpired" : "FreeTrialExited";
        var scope = "GameHovercard";
        clientNavigation.showPromoManager(game.entitlement.productId, promoType, scope);
    }
    else if ($btn.hasClass('view-in-store'))
    {
        // send telemetry when users click "more details" on a purchasable item
        telemetryClient.sendContentSalesViewInStore(game.entitlement.productId, game.entitlement.productType, 'hovercard_button');
        game.viewStoreProductPage();
    }
    else if ($btn.hasClass('download'))
    {
        telemetryClient.sendHovercardDownloadClick(game.entitlement.productId);
        game.startDownload();
    }
    else if ($btn.hasClass('play'))
    {
        telemetryClient.sendGamePlaySource(game.entitlement.productId, "hovercard");
        game.play();
    }
    else if ($btn.hasClass('install'))
    {
        game.install();
    }
    else if ($btn.hasClass('more-details'))
    {
        telemetryClient.sendHovercardDetailsClick(game.entitlement.productId);
        game.gotoDetails();
    }
    else if ($btn.hasClass('install-parent'))
    {
        // Prompt the user to install the base game
        entitlementManager.dialogs.promptForParentInstallation(game.entitlement.productId);
    }
    else if($btn.hasClass('skipqueue'))
    {
        telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, game.entitlement.productId, "mygames-hovercard");
        contentOperationQueueController.pushToTop(game.entitlement.productId);
    }
    else if ($btn.hasClass('pause'))
    {
        game.entitlement.currentOperation.pause();
    }
    else if ($btn.hasClass('resume'))
    {
        game.entitlement.currentOperation.resume();
    }
    else if ($btn.hasClass('restore'))
    {
        game.restoreFromTrash();
    }
	else if ($btn.hasClass('view-achievements'))
	{
        telemetryClient.sendHovercardAchievementsClick(game.entitlement.productId);
		clientNavigation.showAchievementSetDetails( game.achievementSet.id );	
	}
};

})(jQuery);
