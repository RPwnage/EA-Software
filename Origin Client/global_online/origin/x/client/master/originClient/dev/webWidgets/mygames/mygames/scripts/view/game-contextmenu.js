;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }

var GameContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    this.debugActions = [];
    
    this.addAction("startDownload", "", function()
    {
        this.game.startDownload();
    });
    
    this.addAction("downloadNow", "", function()
    {
        telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, this.game.entitlement.productId, "mygames-contextmenu");
        contentOperationQueueController.pushToTop(this.game.entitlement.productId);
    });

    this.addAction("pauseDownload", "", function()
    {
        this.game.pauseDownload();
    });
    
    this.addAction("resumeDownload", "", function()
    {
        this.game.resumeDownload();
    });
    
    this.addAction("cancelDownload", "", function()
    {
        this.game.cancelOperation();
    });

    this.addAction("play", tr('ebisu_client_play'), function()
    {
        telemetryClient.sendGamePlaySource(this.game.entitlement.productId, "contextmenu");
        this.game.play();
    });
    
    this.addAction("cancelCloudSync", tr('ebisu_client_cancel_cloud_sync'), function()
    {
        this.game.cancelCloudSync();
    });

    this.addAction("restore", tr('ebisu_client_restore'), function()
    {
        this.game.restoreFromTrash();
    });

    this.addAction("install", tr('ebisu_client_install'), function()
    {
        this.game.install();
    });

    this.addAction("installParent", "", function()
    {
        // Prompt the user to install the base game
        entitlementManager.dialogs.promptForParentInstallation(this.game.entitlement.productId);
    });
    
    this.addAction("pauseInstall", tr('ebisu_client_pause_ito_installation'), function()
    {
        this.game.pauseInstall();
    });
    
    this.addAction("resumeInstall", tr('ebisu_client_resume_ito_installation'), function()
    {
        this.game.resumeInstall();
    });
    
    this.addAction("cancelInstall", tr('ebisu_client_cancel_ito_installation'), function()
    {
        this.game.cancelOperation();
    });
    
    this.addAction("renewSubsMembership", tr("Subs_Library_HoverCard_Renew"), function()
    {
        telemetryClient.sendSubscriptionUpsell('context_menu');
        clientNavigation.showMyAccountPath("Subscription");
    });

    this.addAction("purchaseGame", "", function()
    {
        this.game.purchase('context_menu');
    });
    
    this.addAction("upgrade", tr("Subs_Upgrade_Action"), function()
    {
        this.game.entitlement.upgrade();
    });

    this.addAction("viewOffer", tr('ebisu_client_view_special_offer_ellipses'), function()
    {
        // Note that a negative "seconds until expiration" means the trial doesn't have
        // an expiration yet (which means the trial hasn't started).
        var promoType = this.game.secondsUntilTermination === 0 ? "FreeTrialExpired" : "FreeTrialExited";
        var scope = "GameTileContextMenu";
        clientNavigation.showPromoManager(this.game.entitlement.productId, promoType, scope);
    });
    
    this.addAction("showGameDetails", tr('ebisu_client_view_game_details'), function()
    {
        // Unowned non-NucleusDD expansions link to the parent pdlc store
        if (this.game.isUnownedNonNucleusDDExpansion && this.game.entitlement.parent)
        {
            // send telemetry when users click "more details" on a purchasable item
            telemetryClient.sendContentSalesViewInStore(this.game.entitlement.productId, this.game.entitlement.productType, 'context_menu');
            clientNavigation.showPDLCStore(this.game.entitlement.parent.productId);
        }
        // Unowned NucleusDD expansions link to the origin store PDP
        else if (this.game.isUnownedNucleusDDExpansion)
        {
            // send telemetry when users click "more details" on a purchasable item
            telemetryClient.sendContentSalesViewInStore(this.game.entitlement.productId, this.game.entitlement.productType, 'context_menu');
            this.game.viewStoreMasterTitlePage();
        }
        else
        {
            this.game.gotoDetails();
        }
    });
    
    this.addAction("showAchievement", tr('ebisu_client_achievements_view_achievements_button'), function()
	{		
		clientNavigation.showAchievementSetDetails( this.game.achievementSet.id );
	});	
    
    this.addAction("favorite", tr('ebisu_client_add_to_favorites'), function()
    {
        this.game.isFavorite = true;
    });

    this.addAction("unfavorite", tr('ebisu_client_unfavorite'), function()
    {
        this.game.isFavorite = false;
    });
    
    // Top separator
    this.topSeparator = this.addSeparator();

    this.addAction("checkForUpdate", tr('ebisu_client_game_update_check_for_updates'), function()
    {
        this.game.checkForUpdateAndInstall();
    });
    
    this.addAction("installUpdate", tr('ebisu_client_update'), function()
    {
        this.game.installUpdate();
    });
    
    this.addAction("updateNow", tr('ebisu_client_update_now'), function()
    {
        telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, this.game.entitlement.productId, "mygames-contextmenu");
        contentOperationQueueController.pushToTop(this.game.entitlement.productId);
    });
    
    this.addAction("pauseUpdate", tr('ebisu_client_game_update_pause_update'), function()
    {
        this.game.pauseUpdate();
    });
    
    this.addAction("resumeUpdate", tr('ebisu_client_game_update_resume_update'), function()
    {
        this.game.resumeUpdate();
    });
    
    this.addAction("cancelUpdate", tr('ebisu_client_game_update_cancel_update'), function()
    {
        this.game.cancelOperation();
    });

    this.addAction("repair", tr('ebisu_client_game_update_check_for_verify'), function()
    {
        this.game.repair();
    });
    
    this.addAction("repairNow", tr('ebisu_client_repair_now'), function()
    {
        telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, this.game.entitlement.productId, "mygames-contextmenu");
        contentOperationQueueController.pushToTop(this.game.entitlement.productId);
    });

    this.addAction("pauseRepair", tr('ebisu_client_pause_repair'), function()
    {
        this.game.pauseRepair();
    });
    
    this.addAction("resumeRepair", tr('ebisu_client_resume_repair'), function()
    {
        this.game.resumeRepair();
    });
    
    this.addAction("cancelRepair", tr('ebisu_client_cancel_repair'), function()
    {
        this.game.cancelOperation();
    });

    this.addAction("customizeBoxart", tr('ebisu_client_customize_boxart_ellipses'), function()
    {
        entitlementManager.dialogs.customizeBoxart(this.game.entitlement.productId);
    });
    
    this.addAction("modifyProperties", tr('ebisu_client_game_properties_ellipsis'), function()
    {
		entitlementManager.dialogs.showProperties(this.game.entitlement.productId);
    });

    // Bottom separator
    this.bottomSeparator = this.addSeparator();

    this.addAction("hide", tr('ebisu_client_hide'), function()
    {
        this.game.isHidden = true;
    });

    this.addAction("unhide", tr('ebisu_client_unhide'), function()
    {
        this.game.isHidden = false;
    });

    this.addAction("removeFromLibrary", tr('ebisu_client_remove_from_library'), function()
    {
        this.game.removeFromLibrary();
    });
    
    this.addAction("uninstall", tr('ebisu_client_uninstall'), function()
    {
        this.game.uninstall();
    });
    
    this.debugSeparator = this.addSeparator();
};

GameContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

// This is used because download can change names
GameContextMenu.prototype.updateActionStrings = function(game)
{
    this.actions.downloadNow.text = game.downloadVerb;
    this.actions.startDownload.text = game.downloadVerb;
    this.actions.installParent.text = game.downloadVerb;
    this.actions.pauseDownload.text = tr('ebisu_client_pause_game').replace('%1', game.downloadNoun);
    this.actions.resumeDownload.text = tr('ebisu_client_resume_game').replace('%1', game.downloadNoun);
    this.actions.cancelDownload.text = tr('ebisu_client_cancel_game').replace('%1', game.downloadNoun);
    this.actions.purchaseGame.text = game.buyNowVerb;
};

GameContextMenu.prototype.hide = function()
{
    this.platformMenu.hide();
};

GameContextMenu.prototype.updateDebugActions = function(game)
{
    // Remove all the old actions
    for(var idx in this.debugActions)
    {
        this.platformMenu.removeAction(this.debugActions[idx]);
    }

    // Add all the new ones
    this.debugActions = game.entitlement.debugActions;

    for(var idx in this.debugActions)
    {
        this.platformMenu.addAction(this.debugActions[idx]);
    }

    this.debugSeparator.visible = (this.debugActions.length > 0);
};

// Updates the context menu for the passed game
// This can either be the same game with a new state or a completely new game
// It returns true if any menu items are visible
GameContextMenu.prototype.updateForGame = function(game)
{
    var actionIsVisible = []; 
    var menuIsEmpty = true;
    
    if (game !== this.game)
    {
        this.updateDebugActions(game);
    }

    // Helper for showing actions
    var makeVisible = $.proxy(function(id, enabled)
    {
        var action = this.actions[id];
        action.visible = true;

        if (typeof enabled === 'undefined')
        {
            action.enabled = true;
        }
        else
        {
            action.enabled = enabled;
        }
        
        actionIsVisible[id] = true;
        menuIsEmpty = false;
    }, this);

    // Helper for showing operation action
    function makeOperationVisible(operation, pauseAction, resumeAction, cancelAction, nowAction)
    {
        var cancellable = true;
        var indexInQueue = contentOperationQueueController.index(game.entitlement.productId);
        if(operation)
        {
            var phase = operation.phase;
        
			// Only show resume/pause if it's the head of the download queue.
			if(indexInQueue === 0)
			{
				if ((phase === 'PAUSED') || (phase === 'PAUSING'))
				{
					makeVisible(resumeAction, operation.resumable);
				}
				else
				{
					makeVisible(pauseAction, operation.pausable);
				}
			}
            cancellable = operation.cancellable;
        }
        
        // index in queue is valid (not -1) and isn't head (0)
        if(nowAction && indexInQueue > 0)
        {
            makeVisible(nowAction, contentOperationQueueController.queueSkippingEnabled(game.entitlement.productId));
        }
        
        makeVisible(cancelAction, cancellable);
    }

    var entitlement = game.entitlement;
    this.game = game;

    if (game.isTrialType)
    {
        makeVisible("viewOffer", onlineStatus.onlineState);
    }
	else if (game.isPurchasable)
	{
		makeVisible("purchaseGame", onlineStatus.onlineState);
	}

    // Time left is 5 days (432000 seconds)
    if(entitlement.isEntitledFromSubscription && subscriptionManager.status !== "ENABLED" && subscriptionManager.timeRemaining < 432000)
    {
        makeVisible("renewSubsMembership", onlineStatus.onlineState);
    }

    if (entitlement.upgradeTypeAvailable !== "NONE")
    {
        makeVisible("upgrade", onlineStatus.onlineState);
    }
	
    if (game.inTrash)
    {
        makeVisible("restore", true);
    }
    else if (game.isPlayable && !game.isCloudSyncInProgress)
    {
        makeVisible("play", !entitlement.playing);
    }
    else if ((entitlement.displayLocation === 'GAMES' || entitlement.displayLocation === 'GAMES|EXPANSIONS') && game.isInstalled && !game.isCloudSyncInProgress)
    {
        // It's normal for PDLC to not be playable once installed
        // For everything else show a greyed out play
        makeVisible("play", false);
    }
    
    // Update our action strings
    this.updateActionStrings(game);

    if (game.isCloudSyncInProgress)
    {
        makeVisible("cancelCloudSync", game.entitlement.cloudSaves.syncOperation.cancellable);
    }
    
    if (entitlement.updateOperation)
    {
        makeOperationVisible(entitlement.updateOperation,
                "pauseUpdate",
                "resumeUpdate",
                "cancelUpdate",
                "updateNow");

        makeVisible("uninstall", false);
    }
    else if (entitlement.downloadOperation)
    {
        makeOperationVisible(entitlement.downloadOperation,
                "pauseDownload",
                "resumeDownload",
                "cancelDownload",
                "downloadNow");
    }
    else if (entitlement.installOperation)
    {
        makeOperationVisible(entitlement.installOperation,
                "pauseInstall",
                "resumeInstall",
                "cancelInstall");

        if (entitlement.repairSupported)
        {
            makeVisible("repair", false);
        }
    }
    else if (entitlement.repairOperation)
    {
        makeOperationVisible(entitlement.repairOperation,
                "pauseRepair",
                "resumeRepair",
                "cancelRepair",
                "repairNow");
                
        makeVisible("uninstall", false);
    }
    else if (game.isDownloadable)
    {
        makeVisible("startDownload", game.isDownloadable);
    }
    else if (game.isInstalled)
    {
        if (entitlement.updateSupported)
        {
            var enableMenuOption = !entitlement.playing && !game.isCloudSyncInProgress;
        
            if (entitlement.availableUpdateVersion !== null)
            {
                makeVisible("installUpdate", enableMenuOption);
            }
            else
            {
                makeVisible("checkForUpdate", enableMenuOption);
            }
        }
        if (entitlement.repairSupported)
        {
            makeVisible("repair", !entitlement.playing && !game.isCloudSyncInProgress);
        }
    }
    else if (game.isInstallable)
    {
        makeVisible("install");

        if (entitlement.repairSupported)
        {
            makeVisible("repair", true);
        }
    }
    else if (game.entitlement.owned && game.entitlement.parent && !game.entitlement.isPULC && 
            (game.entitlement.parent.downloadable || game.entitlement.parent.installable) && (game.entitlement.releaseStatus !== 'UNRELEASED' || game.entitlement.downloadDateBypassed))
    {
        makeVisible("installParent");
    }
    
    var showGameDetails = false;
    if (!entitlement.nonOriginGame)
    {
        if(game.entitlement.owned)
        {
            // Only get a game details option for owned games or games+expansions displayLocation
            if(game.entitlement.displayLocation === 'GAMES' || game.entitlement.displayLocation === 'GAMES|EXPANSIONS')
            {
                if(game.platformCompatibleStatus() === 'INCOMPATIBLE_SUBS' && game.entitlement.purchasable)
                {
                    showGameDetails = false;
                }
                else
                {
                    showGameDetails = true;
                }
            }
        }
        else
        {
            // For unowned NucleusDD expansions, the option links to the store
            if (game.isUnownedNucleusDDExpansion)
            {
                showGameDetails = true;
            }
            // For unowned non-NucleusDD expansions, the option links to the parent pdlc store
            else if (game.isUnownedNonNucleusDDExpansion)
            {
                // TODO: re-enable "show-pdlc-store" when new addon "quick-view" page is up and running
                // showGameDetails = true;
            }
        }
    }
    
    if(showGameDetails)
    {
        makeVisible("showGameDetails");
    }
	
	if ( game.hasAchievements && originUser.commerceAllowed )
	{
		makeVisible("showAchievement", onlineStatus.onlineState);
	}	
    
    this.topSeparator.visible = true;

    var hasBottomSection = false;

    if ((entitlement.displayLocation === 'GAMES' || entitlement.displayLocation === 'GAMES|EXPANSIONS'))
    {
        hasBottomSection = true;

        if (entitlement.nonOriginGame || entitlement.owned)
        {
            makeVisible("customizeBoxart");
        }

        if(!entitlement.playing && entitlement.nonOriginGame)
        {
            makeVisible("removeFromLibrary");
        }
        else if(entitlement.isEntitledFromSubscription)
        {
            makeVisible("removeFromLibrary", onlineStatus.onlineState && !entitlement.playing);
        }
        
        if (game.isInstalled && !entitlement.isBrowserGame)
        {
            makeVisible("modifyProperties", !entitlement.playing);
        }

        if (game.isFavorite)
        {
            makeVisible("unfavorite", onlineStatus.onlineState);
        }
        else
        {
            makeVisible("favorite", onlineStatus.onlineState);
        }
        
        if (game.isHidden)
        {
            makeVisible("unhide", onlineStatus.onlineState);
        }
        else
        {
            makeVisible("hide", onlineStatus.onlineState);
        }
    }

    if (game.isUninstallable)
    {
        hasBottomSection = true;

        makeVisible("uninstall", !entitlement.playing && !game.isCloudSyncInProgress);
    }

    this.bottomSeparator.visible = hasBottomSection;
    
    // Hide and disable all actions we didn't explicitly make visible
    for(var id in this.actions)
    {
        if (actionIsVisible[id] !== true)
        {
            this.actions[id].visible = false;
            this.actions[id].enabled = false;
        }
    }

    return !menuIsEmpty;
};

GameContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

//
// Singleton modelled after hovercard manager
// 
var GameContextMenuManager = function()
{
    this.gameMenu = null;
    this.showingGame = null;

    this.hideEvent = $.Callbacks();
    this.showEvent = $.Callbacks();
};

GameContextMenuManager.prototype.isVisible = function()
{
    return this.showingGame !== null;
};

GameContextMenuManager.prototype._createGameMenu = function()
{
    var menu = new GameContextMenu();

    menu.platformMenu.aboutToHide.connect($.proxy(function()
    {
        this.hideEvent.fire(this.showingGame);
        this.showingGame = null;
    }, this));

    return menu;
};

GameContextMenuManager.prototype.popup = function(game)
{
    // Lazily build our context menu
    if (this.gameMenu === null)
    {
        this.gameMenu = this._createGameMenu();
    }

    // Update our state
    this.showingGame = game;

    if (this.gameMenu.updateForGame(game))
    {
        // Non-empty menu
        this.gameMenu.platformMenu.popup();
        this.showEvent.fire(game);
        return true;
    }
    else
    {
        this.gameMenu.hide();
    }

    return false;
};

GameContextMenuManager.prototype.updateGameIfVisible = function(game)
{
    if (this.showingGame === game)
    {
        if (this.gameMenu.updateForGame(game) === false)
        {
            this.gameMenu.hide();
        }
    }
};

Origin.GameContextMenuManager = new GameContextMenuManager();

})(jQuery);
