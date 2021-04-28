/**
 * @file dialog/content/youtube.js
 */
(function() {
    'use strict';
    
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    // Player options object for youtube player
    var playerOptions = {
        autoplay: 0,
        loop: 0,
        controls: 1
    };

    // Youtube iframe height and width
    var PLAYER_HEIGHT = '720',
        PLAYER_WIDTH = '1280',
        DESTINATION_NAME = 'my_home',
        VIDEO_STARTED = 'started',
        VIDEO_ENDED = 'complete',
        VIDEO_MODAL_CLOSED = 'end_click';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-youtube';

    function OriginDialogContentYoutubeCtrl($scope, $sce, AppCommFactory, NavigationFactory, UrlHelper, UtilFactory, ScopeHelper) {
        // we need URL for PIN telemetry
        $scope.trustedSrc = UrlHelper.buildYouTubeEmbedUrl($scope.videoId);
        $scope.showPackart = $scope.hidePackart !== BooleanEnumeration.true;

        $scope.restrictedMessage = UtilFactory.getLocalizedStr($scope.restrictedMessage, CONTEXT_NAMESPACE, 'restrictedmessagestr');

        $scope.strings = {
            contentLink: UtilFactory.getLocalizedStr($scope.contentLink, CONTEXT_NAMESPACE, 'content-link'),
            contentLinkBtnText: UtilFactory.getLocalizedStr($scope.contentLinkBtnText, CONTEXT_NAMESPACE, 'content-link-btn-text'),
            contentText: UtilFactory.getLocalizedStr($scope.contentText, CONTEXT_NAMESPACE, 'content-text')
        };

        this.processContentLink = function(){
            if ($scope.contentLink) {
                $scope.contentLinkAbsoluteUrl = NavigationFactory.getAbsoluteUrl($scope.contentLink);
                $scope.isContentLinkExternal = UtilFactory.isAbsoluteUrl($scope.contentLink);
            }
        };

        this.onScopeResolved = function() {
            this.processContentLink();
            ScopeHelper.digestIfDigestNotInProgress($scope);
        };
    }

    function originDialogContentYoutube(ComponentsConfigFactory, DialogFactory, YoutubeFactory, DirectiveScope, NavigationFactory) {

        function originDialogContentYoutubeLink (scope, elem, attrs, OriginDialogContentYoutubeCtrl) {

            var youtubeElement = angular.element(elem).find('.origin-dialogcontent-youtube-player'),
                PLAYER_STATES = YoutubeFactory.PLAYER_STATES,
                youtubePlayer;

            /**
             * Holds on to a youtubePlayer reference
             * @param player
             * @returns {Object} youtubePlayer
             */
            function cachePlayer(player) {
                youtubePlayer = player;
            }

            /**
             * Send telemetry to GA/Pin when video starts playing
             */
            function sendStartVideoTelemetryEvent() {
                YoutubeFactory.sendStartVideoTelemetryEvent({
                    videoId: scope.videoId,
                    status: VIDEO_STARTED,
                    source: scope.trustedSrc,
                    destinationName: DESTINATION_NAME
                });
            }

            /**
             * Send telemetry to GA/Pin when video is stopped or modal is closed with the current video time.
             */
            function sendStopVideoTelemetryEvent(status) {
                if(youtubePlayer) {
                    // send telemetry when user closed the dialog
                    YoutubeFactory.sendStopVideoTelemetryEvent({
                        videoId: scope.videoId,
                        playedTime: Math.floor((youtubePlayer.getCurrentTime() / youtubePlayer.getDuration()) * 100).toFixed(0),// we need to send % to telemetry
                        status: status,
                        source: scope.trustedSrc,
                        destinationName: DESTINATION_NAME
                    });
                }
            }

            /**
             * Listens for video state (Playing, Ended)
             * @param event
             */
            function handleVideoStateChange(event) {
                if(event.data === PLAYER_STATES.PLAYING) {
                    sendStartVideoTelemetryEvent();
                }
                else if(event.data === PLAYER_STATES.ENDED) {
                    sendStopVideoTelemetryEvent(VIDEO_ENDED);
                }
            }

            /**
             * Stop the player when user close the video modal
             */
            function stopPlayer() {
                if (youtubePlayer) {
                    youtubePlayer.mute();
                    youtubePlayer.stopVideo();
                }
            }

            YoutubeFactory.loadYoutubePlayer(youtubeElement, scope.videoId, playerOptions, PLAYER_WIDTH, PLAYER_HEIGHT, {'onStateChange': handleVideoStateChange}).then(cachePlayer);

            scope.closeDialog = function () {
                DialogFactory.close(scope.dialogId);
            };

            scope.$on('cta:clicked', scope.closeDialog);

            scope.openLink = function (href, $event) {
                $event.preventDefault();
                scope.closeDialog();
                NavigationFactory.openLink(href);
            };

            if (scope.ocdPath) {
                DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                    .then(OriginDialogContentYoutubeCtrl.onScopeResolved);
            }else{
                OriginDialogContentYoutubeCtrl.processContentLink();
            }

            function onDestroy() {
                sendStopVideoTelemetryEvent(VIDEO_MODAL_CLOSED);
                stopPlayer();
            }

            // when the element is destroyed make sure that we cleanup
            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                videoId: '@',
                matureContent: '@',
                ocdPath: '@',
                contentLink: '@',
                contentLinkBtnText: '@',
                contentText: '@',
                hidePackart: '@'
            },
            controller: 'OriginDialogContentYoutubeCtrl',
            link: originDialogContentYoutubeLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youtube.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYoutube
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id the video id of the youtube video
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {LocalizedString} restrictedmessagestr *Only the value from defaults is used. The string the user sees when they are not old enough to view a string.
     * @param {OcdPath} ocd-path OCD path for pack art and CTA
     * @param {Url} content-link (Optional) URL that the CTA below the video will link to (overrides OCD path CTA)
     * @param {LocalizedString} content-link-btn-text (Optional) Content link CTA text
     * @param {LocalizedString} content-text (Optional) Text to display underneath the video for more information etc.
     * @param {BooleanEnumeration} hide-packart (Optional) Suppress pack art from showing when OCD path is provided
     * @description
     *
     * youtube video dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-youtube
     *             video-id="hlEKa6tUPlU"
     *             mature-content="true">
     *         </origin-dialog-content-youtube>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentYoutubeCtrl', OriginDialogContentYoutubeCtrl)
        .directive('originDialogContentYoutube', originDialogContentYoutube);

}());