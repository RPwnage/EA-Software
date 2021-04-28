;
(function ($) {
    "use strict";

    if (!window.Origin) {window.Origin = {};}
    if (!Origin.views) {Origin.views = {};}

    var TileManager = function ()
    {
        this.headItemId = null;
        this.updateOperationController = {
            play: '',
            pause: tr('state_pause'),
            cancel: tr('ebisu_client_cancel'),
            resume: tr('state_resume'),
            playTooltip: '',
            disabled: tr('ebisu_client_uninterruptible_action_in_progress'),
            debug: ''
        };
    };

    TileManager.prototype.staticOperationControllerStrings = function ()
    {
        return {
            play: tr('ebisu_client_play'),
            pause: tr('state_pause'),
            cancel: tr('ebisu_client_cancel'),
            resume: tr('state_resume'),
            playTooltip: '',
            disabled: tr('ebisu_client_uninterruptible_action_in_progress'),
            debug: 'Debug'
        };
    };

    TileManager.prototype.setupHead = function ()
    {
        var headEnt = contentOperationQueueController.head,
            $operationController = $('#operation-controller'),
            $progressBarDetailed = $operationController.otkOperationController("progressbardetailed"),
            configuredUrls = null,
            boxart = null,
            iUrl = 0,
            subtitle = "",
            operation = headEnt ? headEnt.currentOperation : null;

        if (headEnt)
        {
            $operationController.attr('data-id', headEnt.productId);
            this.headItemId = headEnt.productId;
            configuredUrls = headEnt.boxartUrls;
            if(configuredUrls.length === 0 && headEnt.parent)
                configuredUrls = headEnt.parent.boxartUrls;

            for (iUrl = 0; iUrl < configuredUrls.length; iUrl++)
            {
                if (Origin.BadImageCache.shouldTryUrl(configuredUrls[iUrl]))
                {
                    boxart = configuredUrls[iUrl];
                    break;
                }
            }

            $operationController.otkOperationController({
                boxartUrl: boxart,
                strings: TileManager.prototype.staticOperationControllerStrings()
            });
            $operationController.otkOperationController("update");

            if(operation)
            {
                subtitle = TileManager.prototype.subTitle(headEnt);
            }

            $progressBarDetailed.otkProgressBarDetailed({
                title: headEnt.title,
                subtitle: subtitle
            });
            $progressBarDetailed.otkProgressBarDetailed("update");

            TileManager.prototype.updateHead(headEnt);
        }
        else
        {
            $('#operation-controller').hide();
        }
    };

    TileManager.prototype.clearHead = function ()
    {
        this.headItemId = null;
        var $operationController = $('#operation-controller'),
            $progressBarDetailed = $operationController.otkOperationController("progressbardetailed");
        $operationController.removeAttr('data-id');
        $operationController.otkOperationController({
            boxartUrl: null,
            playable: false,
            pausable: false,
            cancellable: false,
            resumable: false
        });
        $operationController.otkOperationController("update");
        $progressBarDetailed.otkProgressBarDetailed({
            title: "",
            subtitle: "",
            bytesDownloaded: 0,
            totalFileSize: 0,
            bytesPerSecond: 0,
            percent: 0
        });
        $progressBarDetailed.otkProgressBarDetailed("update");
        $operationController.otkOperationController("buttonPlay").toggle(false);
        $operationController.otkOperationController("progressbar").otkProgressBar("removeAllProgressFlags");
    }

    TileManager.prototype.updateHead = function (_ent)
    {
        var currentProgress = _ent.currentOperation,
            $operationController = $('#operation-controller'),
            playableAt = 0,
            isPlayable = _ent.playable,
            $progressbar = $operationController.otkOperationController("progressbar"),
            $progressBarDetailed = $operationController.otkOperationController("progressbardetailed"),
            isDynamicOperation = false,
            progressState = "",
            operationProgress = 0,
            operationBytesPerSecond = 0,
            operationPhase = "";

        if (currentProgress)
        {
            operationProgress = currentProgress.progress;
            operationBytesPerSecond = currentProgress.bytesPerSecond;
            isDynamicOperation = currentProgress.isDynamicOperation;
            playableAt = isDynamicOperation ? currentProgress.playableAt : 0;
            operationPhase = currentProgress.phase;

            ////// section 2: Never takes more than 1 mili sec.
            if((operationBytesPerSecond || operationPhase === "RUNNING") && currentProgress.type !== "ITO")
            {
                progressState = operationBytesPerSecond;
            }
            else
            {
                progressState = currentProgress.phaseDisplay;
            }

            if(isDynamicOperation
                && $progressbar.is(":visible"))
            {
                if(playableAt > 0)
                {
                    $progressbar.otkProgressBar("setFlagSettings", "playableAt", playableAt, tr("ebisu_client_playable_marker"), "Color-Green");
                }
                $progressbar.otkProgressBar("toggleColorProgressFlag", "playableAt", currentProgress.shouldLightFlag("playableAt"));

                if(this.updateOperationController)
                {
                    this.updateOperationController.playTooltip = '';
                    if(_ent.playing)
                    {
                        this.updateOperationController.playTooltip = tr('ebisu_client_game_currently_running');
                    }
                    else if(isPlayable === false)
                    {
                        this.updateOperationController.playTooltip = tr('ebisu_client_not_yet_playable');
                    }
                }
            }
            $operationController.otkOperationController("buttonPlay").toggle(isDynamicOperation);
            ////// end section 2

            $operationController.otkOperationController({
                boxartUrl: "",
                playable: isPlayable,
                pausable: currentProgress.pausable,
                cancellable: currentProgress.cancellable,
                resumable: currentProgress.resumable,
                strings: this.updateOperationController ? this.updateOperationController : TileManager.prototype.staticOperationControllerStrings()
            });
            $operationController.otkOperationController("update");

            $progressBarDetailed.otkProgressBarDetailed({
                title: null,
                subtitle: null,
                progress: operationProgress,
                totalFileSize: currentProgress.totalFileSize,
                bytesDownloaded: currentProgress.bytesDownloaded,
                secondsRemaining: currentProgress.secondsRemaining !== 0 ? dateFormat.formatShortInterval(currentProgress.secondsRemaining) : 0,
                bytesPerSecond: progressState
            });
            $progressBarDetailed.otkProgressBarDetailed("update");

            $progressbar.otkProgressBar({
                progress: operationProgress,
                state: currentProgress.progressState,
                text: ''
            });
            $progressbar.otkProgressBar("update");
        }
    };


    TileManager.prototype.setupQueueItem = function (_ent, _list, _index, mini)
    {
        var $queueItemTemplate,
            action,
            $otkItem;
        if (TileManager.prototype.item(_ent.productId).length === 0)
        {
            $queueItemTemplate = $('<li></li>');
            $queueItemTemplate.attr('data-id', _ent.productId);
            action = TileManager.prototype.setupQueueActionLink(_ent);

            $otkItem = $.otkOperationQueueItemCreate($("<div></div>"), {
                title: _ent.title,
                subtitle: TileManager.prototype.subTitle(_ent),
                action: mini ? tr("ebisu_client_waiting") : action.string,
                isMini: mini,
                strings: {
                    cancel: tr('ebisu_client_cancel')
                }
            });
            $queueItemTemplate.append($otkItem);

            if(mini === false)
            {
                $otkItem.otkOperationQueueItem("action").attr("head-dependent", action.headDependent);
            }

            // Set close button functionality
            $otkItem.otkOperationQueueItem("closeButton").click(function () {
                var entitlement = entitlementManager.getEntitlementByProductId(this.attr('data-id'), true),
                    operation = entitlement ? entitlement.currentOperation : null;
                if (operation && operation.cancellable)
                {
                    telemetryClient.sendQueueItemRemoved(entitlement.productId, "queue-list-item");
                    operation.cancel();
                }
                else
                {
                    contentOperationQueueController.remove(entitlement.productId, false);
                }
            }.bind($queueItemTemplate));

            // Set action link functionality
            if (action.function)
            {
                $otkItem.otkOperationQueueItem("action").click(action.function);
            }

            // Append to the end or insert at an index?
            if ((_list.find("li").length === 0) || (_index >= _list.find("li").length))
            {
                _list.append($queueItemTemplate);
            }
            else
            {
                _list.find("li").eq(_index).before($queueItemTemplate);
            }
        }
        TileManager.prototype.updateQueueItem(_ent);
    };


    TileManager.prototype.setupQueueActionLink = function (_ent)
    {
        var progress = _ent.currentOperation,
            actionStr = "",
            actionFunction = null,
            dependent = false;
        if (_ent.installable)
        {
            dependent = true;
            actionStr = tr("ebisu_client_ready_to_install");
            actionFunction = function () {
                var head = contentOperationQueueController.head;
                telemetryClient.sendQueueOrderChanged(head ? head.productId : "", this.productId, "queue-list-item");
                this.install();
            }.bind(_ent);
        }
        else if (progress)
        {
            dependent = true;
            if (progress.phase !== "ENQUEUED" && progress.progress !== 0)
            {
                actionStr = progress.specificPhaseDisplay("RESUME");
            }
            else
            {
                actionStr = progress.typeDisplay;
            }
            actionFunction = function () {
                telemetryClient.sendQueueOrderChanged(contentOperationQueueController.head.productId, this.productId, "queue-list-item");
                contentOperationQueueController.pushToTop(this.productId);
            }.bind(_ent);
        }
        else if (_ent.playable)
        {
            actionStr = tr("ebisu_client_play");
            actionFunction = function () {
                telemetryClient.sendGamePlaySource(this.productId, "queue-line-items");
                this.play();
            }.bind(_ent);
        }
        else if (_ent.parent && _ent.parent.playable)
        {
            actionStr = tr("ebisu_client_play");
            actionFunction = function () {
                telemetryClient.sendGamePlaySource(this.productId, "queue-line-items");
                this.parent.play();
            }.bind(_ent);
        }
        return {string: actionStr, function: actionFunction, headDependent: dependent};
    };


    TileManager.prototype.updateQueueItem = function (ent)
    {
        var $item = TileManager.prototype.item(ent.productId),
            $otkItem = $item.length ? $item.find("> .otk-operation-queue-item") : null,
            operation = null,
            actionVisibleValue = true,
            headDependent = '';
        if ($otkItem)
        {
            operation = ent.currentOperation;
            headDependent = $otkItem.otkOperationQueueItem("action").attr("head-dependent");

            $otkItem.otkOperationQueueItem({
                cancellable: (operation ? operation.cancellable : false),
                progressPercent: (operation ? operation.progress : 0.0),
                bytesDownloaded: (operation ? operation.bytesDownloaded : 0),
                totalFileSize: (operation ? operation.totalFileSize : 0),
                strings: {
                    percentNotation: tr('percent_complete').replace('%1', tr('ebisu_client_percent_notation'))
                }
            });
            $otkItem.otkOperationQueueItem("update");

            if(operation && operation.phase === "CANCELLING")
            {
                $otkItem.otkOperationQueueItem("action").text(operation.phaseDisplay);
                $otkItem.otkOperationQueueItem("action").attr("disabled", "disabled");
            }
            else if((headDependent === 'false' && (ent.playing || (ent.parent && ent.parent.playing)))
               || (headDependent === 'true' && !contentOperationQueueController.queueSkippingEnabled(ent.productId)))
            {
                actionVisibleValue = false;
            }
            $otkItem.otkOperationQueueItem("action").css("display", actionVisibleValue ? "block" : "none");
        }
    };


    TileManager.prototype.removeQueueItem = function (id)
    {
        var $item = $("[data-id='" + id + "']");
        // If the list is empty, hide it.
        if ($item.siblings('li').size() === 0)
        {
            // If it's a sub list (e.g. mini list - addons) remove the list
            if ($item.parent('ul').hasClass("otk-mini-list"))
            {
                $item.parent('ul').remove();
            }
            // If the list item was apart of one of our main list, just hide the list
            else
            {
                $item.parent('ul').hide();
            }
        }
        $item.remove();
    };


    TileManager.prototype.createMiniList = function (parentId)
    {
        var $miniList = $('<ul class="otk-mini-list"></ul>');
        $miniList.attr("data-id-list", parentId);
        return $miniList;
    };


    TileManager.prototype.miniList = function (topLevelEntId)
    {
        return $('[data-id-list="' + topLevelEntId + '"]');
    };


    TileManager.prototype.removeItemsInMiniList = function (topLevelEntId)
    {
        // remove any children
        if (TileManager.prototype.miniList(topLevelEntId).length !== 0)
        {
            TileManager.prototype.miniList(topLevelEntId).find("li").each(function (index) {
                contentOperationQueueController.remove($(this).attr('data-id'));
            });
        }
    };


    TileManager.prototype.item = function (topLevelEntId)
    {
        return $('[data-id="' + topLevelEntId + '"]');
    };


    TileManager.prototype.isHeadItem = function (topLevelEntId)
    {
        return topLevelEntId === this.headItemId;
    };

    TileManager.prototype.subTitle = function (entitlement)
    {
        var text = '',
            operation = entitlement ? entitlement.currentOperation : null;
        if (entitlement.parent)
        {
            text += tr("ebisu_client_game_dlc").replace('%1', entitlement.parent.title) + " ";
        }
        if (operation && (operation.type === "UPDATE" || operation.type === "REPAIR"))
        {
            text += operation.typeDisplay;
        }
        return text;
    };

    TileManager.prototype.onHeadBusy = function (isBusy)
    {
        // Hide all actions to change the header if the game is busy
        var $listItemActions = $('.otk-operation-queue-item .otk-hyperlink[head-dependent=true]');
        // show() will set the display to inline-block - which is not what we want.
        $listItemActions.css("display", isBusy ? "none" : "block");
    };

    Origin.views.TileManager = new TileManager();

})(jQuery);
