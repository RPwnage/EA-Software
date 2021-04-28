; (function ($) {
    "use strict";

    if (!window.Origin) { window.Origin = {}; }
    if (!Origin.views) { Origin.views = {}; }

    var GlobalProgressView = function ()
    {
        $('html').addClass(Origin.views.currentPlatform());
        var $operationController = $('#operation-controller');
        $.otkOperationControllerCreate($operationController, {
                debugable: clientSettings.readSetting("DownloadDebugEnabled")
        });
        $operationController.otkOperationController("progressbardetailed").otkProgressBarDetailed({
                strings: {
                    percentNotation: tr('ebisu_client_percent_notation'),
                    timeRemaining: tr('cc_caption_time_remaining'),
                    seconds: tr('seconds'),
                    bps: tr('ebisu_client_network_speed')
                },
                formatBytesFunction: dateFormat.formatBytes
        });
        $operationController.otkOperationController("buttonPause").click(function (){
            var entitlement = contentOperationQueueController.head;
            if (entitlement && entitlement.currentOperation)
            {
                entitlement.currentOperation.pause();
            }
        });

        $operationController.otkOperationController("buttonCancel").click(function (event) {
            var entitlement = contentOperationQueueController.head;
            if (entitlement && entitlement.currentOperation)
            {
                telemetryClient.sendQueueItemRemoved(entitlement.productId, "queue-window-head");
                entitlement.currentOperation.cancel();
            }
        });

        $operationController.otkOperationController("buttonResume").click(function () {
            var entitlement = contentOperationQueueController.head;
            if (entitlement && entitlement.currentOperation)
            {
                entitlement.currentOperation.resume();
            }
        });

        $operationController.otkOperationController("buttonPlay").click(function () {
            var entitlement = contentOperationQueueController.head;
            if (entitlement)
            {
                telemetryClient.sendGamePlaySource(entitlement.productId, "queue-head");
                entitlement.play();
            }
        });

        $operationController.otkOperationController("buttonDebug").click(function () {
            var entitlement = contentOperationQueueController.head;
            if (entitlement)
            {
                entitlementManager.dialogs.showDownloadDebug(entitlement.productId);
            }
        });

        $.otkStickyListCreate($('#entitlement-list'), {});
        $('#operation-controller').hide();
        $('#queued-list').hide();
        $('#completed-list').hide();
        $('#entitlement-list').hide();
        $('#completed-list > section > a').click(function () {
            contentOperationQueueController.clearCompleteList();
        });

        $(document).ready(function () {
            // Listen for a change in our current top level entitlements.
            contentOperationQueueController.entitlementsQueued.forEach(GlobalProgressView.prototype.onGameAdded);
            contentOperationQueueController.enqueued.connect(GlobalProgressView.prototype.onGameAdded);
            contentOperationQueueController.removed.connect(GlobalProgressView.prototype.onGameRemoved);

            contentOperationQueueController.entitlementsCompleted.forEach(GlobalProgressView.prototype.onGameToComplete);
            contentOperationQueueController.addedToComplete.connect(GlobalProgressView.prototype.onGameToComplete);
            contentOperationQueueController.completeListCleared.connect(GlobalProgressView.prototype.onCompleteListCleared);
            contentOperationQueueController.headBusy.connect(Origin.views.TileManager.onHeadBusy);

            onlineStatus.onlineStateChanged.connect($.proxy(GlobalProgressView.prototype, 'onOnlineStatusChange'));
            clientSettings.updateSettings.connect($.proxy(GlobalProgressView.prototype, 'onUpdateSettings'));
            GlobalProgressView.prototype.onUpdateSettings();
        });
    };

    GlobalProgressView.prototype.onOnlineStatusChange = function ()
    {
        contentOperationQueueController.entitlementsQueued.forEach(GlobalProgressView.prototype.onGameChanged);
    };
    
    GlobalProgressView.prototype.onUpdateSettings = function ()
    {
        $('.otk-banner').toggle(clientSettings.readSetting("EnableDownloaderSafeMode"));
    };

    // This event gets triggered when a game has been added to the queue
    GlobalProgressView.prototype.onGameAdded = function (ent)
    {
        ent.changed.connect($.proxy(GlobalProgressView.prototype, 'onGameChanged', ent));
        GlobalProgressView.prototype.possiblyCreateTile(ent);
    };

    // This event gets triggered when a game has been removed
    GlobalProgressView.prototype.onGameRemoved = function (id, removeChildren, queueUpNext)
    {
        var dlcOfferIDs = [],
            entitlement = null;
        if (Origin.views.TileManager.isHeadItem(id))
        {
            Origin.views.TileManager.clearHead();
            if (removeChildren)
            {
                $("#content-container > .otk-mini-list").remove();
            }

            if(queueUpNext)
            {
                var $miniList;
                // Queue up next item in list
                if (contentOperationQueueController.head)
                {
                    // Get the mini list of the list item before we remove it
                    $miniList = Origin.views.TileManager.miniList(contentOperationQueueController.head.productId);
                    $miniList.addClass("otk-medium-scrollbar");
                    $miniList.find("li").each(function( index ) {
                        dlcOfferIDs.push($(this).attr('data-id'));
                    });
                    $miniList.empty();
                    Origin.views.TileManager.removeQueueItem(contentOperationQueueController.head.productId);
                }
                Origin.views.TileManager.setupHead();
                $("#operation-controller").after($miniList);
                // Because the close button won't work if just move the list, we have to re-add all the entitlements.
                $.each(dlcOfferIDs, function( index, value ) {
                    entitlement = entitlementManager.getEntitlementByProductId(value);
                    GlobalProgressView.prototype.possiblyCreateTile(entitlement);
                });
            }
        }
        else
        {
            Origin.views.TileManager.removeQueueItem(id);
        }

        if (contentOperationQueueController.entitlementsQueued.length === 0 && contentOperationQueueController.entitlementsCompleted.length === 0)
        {
            $('#content-container').hide();
            $('#no-active-entitlements').show();
        }
        $('#entitlement-list').toggle($('#entitlement-list li').length > 0);
    };

    GlobalProgressView.prototype.onGameChanged = function (ent)
    {
        // Instead of doing a slow C++ comparison if(contentOperationQueueController.head === ent).
        // We are just going figure it out here
        if (Origin.views.TileManager.isHeadItem(ent.productId))
        {
            Origin.views.TileManager.updateHead(ent);
        }
        else
        {
            Origin.views.TileManager.updateQueueItem(ent);
        }
    }

    GlobalProgressView.prototype.possiblyCreateTile = function (_ent)
    {
        var $parent = contentOperationQueueController.parentInQueue(_ent.productId);
        // If this is the first in the list
        if (contentOperationQueueController.head === _ent)
        {
            Origin.views.TileManager.clearHead();
            Origin.views.TileManager.setupHead();
            $('#operation-controller').show();
            // EBIBUGS-28175: The issue is the flags/dividers aren't being set until after we show the progress bar.
            // The proper way of fixing this would be to update the dividers/flags when
            // the size changes on the progress bar.
            // However, javascript hates me and the resize event isn't being sent out. Luckily the window isn't
            // resizable so I don't have to worry about it now. Origin 10 coming + bug fixing + low bug = no time for this.
            // TODO: Fix it to be right.
            // Also this function is only called when a head is being setup. It isn't expensive.
            Origin.views.TileManager.updateHead(_ent);
        }
        // Is addon or expansion
        else if ($parent)
        {
            var $item = Origin.views.TileManager.item($parent.productId),
                $miniList = Origin.views.TileManager.miniList($parent.productId);

            if ($miniList.length === 0)
            {
                $miniList = Origin.views.TileManager.createMiniList($parent.productId);

                if ($parent === contentOperationQueueController.head)
                {
                    $miniList.addClass("otk-medium-scrollbar");
                    $("#operation-controller").after($miniList);
                }
                else
                {
                    $item.append($miniList);
                    $item.find(".otk-operation-queue-item").otkOperationQueueItem("setExpandable", true, "[data-id-list='" + $parent.productId + "']");
                }
            }
            Origin.views.TileManager.setupQueueItem(_ent, $miniList, $miniList.children().size(), true);
        }
        else
        {
            // entitlement is the top level entitlement or parent isn't on the list
            Origin.views.TileManager.setupQueueItem(_ent, $("#queued-list"), contentOperationQueueController.index(_ent.productId) - 1, false);
            $('#entitlement-list').show();
            $('#queued-list').show();
        }

        $('#no-active-entitlements').hide();
        $('#content-container').show();
    };

    GlobalProgressView.prototype.onGameToComplete = function (_ent)
    {
        var parent = contentOperationQueueController.parentInQueue(_ent.productId);
        if(parent)
        {
            parent.changed.connect($.proxy(GlobalProgressView.prototype, 'onGameChanged', _ent));
        }
        GlobalProgressView.prototype.onGameRemoved(_ent.productId, false, true);
        Origin.views.TileManager.setupQueueItem(_ent, $("#completed-list"), 0, false);
        $('#entitlement-list').show();
        $('#completed-list').show();
        $('#no-active-entitlements').hide();
    };

    GlobalProgressView.prototype.onCompleteListCleared = function ()
    {
        $('#completed-list').find("li").remove();
        $('#completed-list').hide();
        $('#entitlement-list').toggle($('#entitlement-list li').length > 0);
        if (contentOperationQueueController.entitlementsQueued.length === 0 && contentOperationQueueController.entitlementsCompleted.length === 0)
        {
            $('#content-container').hide();
            $('#no-active-entitlements').show();
        }
    };

    // Expose to the world
    Origin.views.globalProgress = new GlobalProgressView();

})(jQuery);