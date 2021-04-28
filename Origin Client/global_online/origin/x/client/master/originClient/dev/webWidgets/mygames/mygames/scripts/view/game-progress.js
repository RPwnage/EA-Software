; (function ($) {
    "use strict";

    if (!window.Origin) { window.Origin = {}; }
    if (!Origin.util) { Origin.util = {}; }

    Origin.util.updateGameProgressBar = function (game, $progressBarContainer, showFlags)
    {
        var operation = game.entitlement.currentOperation,
            $progressBar = $progressBarContainer.find('.otk-progressbar');
        operation = operation ? operation : null;

        if ($progressBarContainer.length && operation)
        {
            // Describe our progress
            var progressBarLabel = operation.phaseDisplay;
            var phase = operation.phase;
            var progress = operation.progress;
            var playableAt = operation.playableAt;

            // If the game isn't the head item - always show "In Queue"
            if(progress)
            {
                /* READDRESS THIS after the progressbar is a toolkit component:
                - Come up with a better way to represent the percentage so it's more robust to break out from the label if needed for overflow reasons
                - I suggest that any string that wants to show a percentage should be represented as something like "Installing: %p" where %p would be
               replaced by the percentage by the progressbar component. Then we could remove the icky ebisu_client_colon_placement and ebisu_client_percent_notation strings.
                */
                progressBarLabel = tr('ebisu_client_colon_placement').replace('%0', progressBarLabel).replace('%1', Math.floor(progress * 100));
                progressBarLabel = tr('ebisu_client_percent_notation').replace('%1', progressBarLabel);
            }
            else if (phase !== 'PAUSED' && phase !== 'ENQUEUED')
            {
                // Indicate an ongoing action as we're switching phases
                progressBarLabel += tr('ebisu_client_ellipsis');
            }
            
            if(phase === 'PAUSED' && contentOperationQueueController.index(game.entitlement.productId) > 0)
            {
                progressBarLabel = tr('ebisu_client_queued');
            }

            $progressBar.otkProgressBar(
            {
                progress: progress,
                state: operation.progressState,
                text: ''
            });
            $progressBar.otkProgressBar("update");

            if(phase === 'ENQUEUED' || (phase === 'PAUSED' && !progress))
            {
                $progressBarContainer.addClass("label-only");
            }
            else
            {
                $progressBarContainer.removeClass("label-only");
            }
            
            if($progressBarContainer.find('.progress-info').length)
            {
                $progressBarContainer.find('.progress-info').text(progressBarLabel);
            }
            $progressBarContainer.show();

            if(operation.isDynamicOperation
               && $progressBar.is(":visible"))
            {
                if($progressBarContainer.find('.otk-divider').length === 0)
                {
                    $progressBar.otkProgressBar("addProgressFlag", "playableAt", 0, "", "Color-Green");
                }
                if(playableAt > 0)
                {
                    $progressBar.otkProgressBar("setFlagSettings", "playableAt", playableAt, showFlags ? tr("ebisu_client_playable_marker") : "", "Color-Green");
                    $progressBarContainer.attr("has-Flags", showFlags);
                }
                
                $progressBar.otkProgressBar("toggleColorProgressFlag",  "playableAt", operation.shouldLightFlag("playableAt"));
            }
        }
        else
        {
            if($progressBarContainer && $progressBarContainer.is(":visible"))
            {
                $progressBarContainer.find('.progress-info').text("");
                $progressBar.otkProgressBar("removeAllProgressFlags");
                $progressBarContainer.attr("has-Flags", false);
                $progressBarContainer.hide();
            }
        }

        return operation !== null;
    };

})(jQuery);