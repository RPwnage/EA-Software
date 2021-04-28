
/**
 * @file store/pdp/sections/scripts/video.js
 */
(function(){
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var ASPECT_RATIO = 9/16, //Video aspect ratio for 16:9
        VIDEO_PLAY_EVENT = 'YoutubePlayer:Play',
        VIDEO_STOP_EVENT = 'YoutubePlayer:Stop';

    function originStorePdpSectionsVideo(ComponentsConfigFactory, YoutubeFactory, AnimateFactory, $timeout, GamesDataFactory) {

        function originStorePdpSectionsVideoLink(scope, elem, attrs, ctrl) {

            var element = elem.find('.origin-pdp-hero-video');
            var youtubeElement = elem.find('.origin-pdp-hero-video-frame');
            var youtubePlayer;
            var PLAYER_STATES = YoutubeFactory.PLAYER_STATES;

            function cachePlayer(player) {
                youtubePlayer = player;
            }

            function callYouTubePlayerPlayVideo(){
                youtubePlayer.playVideo();
            }

            function playVideoIfNotPlayingGame(playing) {
                if(!playing) {
                    scope.showVideo = true;
                    element.height(element.width() * ASPECT_RATIO);
                    if(!scope.matureContent || scope.matureContent === BooleanEnumeration.false || scope.agePassed) {
                        youtubePromise.then(callYouTubePlayerPlayVideo);
                    } else {
                        elem.find('.origin-agegate:hidden').show();
                    }
                }
            }

            function playVideo() {
                return Origin.client.games.isGamePlaying()
                    .then(playVideoIfNotPlayingGame);                
            }

            scope.hide = function() {

                element.height(0);

                scope.$emit(VIDEO_STOP_EVENT);

                $timeout(function() {
                    youtubePlayer.stopVideo();
                    scope.showVideo = false;
                }, 200);
            };

            function resize() {
                var frame = elem.find('.origin-pdp-hero-video-frame');
                var width = frame.width();

                frame.height(width * ASPECT_RATIO);

                if (scope.showVideo) {
                    element.height('');
                    ctrl[0].updateMarginTop();
                }                
            }

            function toggleVideoPlay(play) {
                var playerState;
                if (youtubePlayer && scope.videoId) {
                    playerState = youtubePlayer.getPlayerState();
                    if (play && playerState === PLAYER_STATES.PAUSED){   // only start the video again if we stopped it.
                        playVideo();
                    }else if (!play && playerState === PLAYER_STATES.PLAYING){
                        youtubePlayer.pauseVideo();
                    }
                }
            }

            function onStartPlayingGame() {
                toggleVideoPlay(false);
            }

            function onStopPlayingGame() {
                toggleVideoPlay(true);
            }

            Origin.events.on(Origin.events.CLIENT_VISIBILITY_CHANGED, toggleVideoPlay );
            Origin.events.on(Origin.events.CLIENT_NAV_REDRAW_VIDEO_PLAYER, resize);
            GamesDataFactory.events.on('games:finishedPlaying', onStopPlayingGame);
            GamesDataFactory.events.on('games:startedPlaying', onStartPlayingGame);

            function onDestroy() {
                Origin.events.off(Origin.events.CLIENT_VISIBILITY_CHANGED, toggleVideoPlay);
                Origin.events.off(Origin.events.CLIENT_NAV_REDRAW_VIDEO_PLAYER, resize);
                GamesDataFactory.events.off('games:finishedPlaying', onStopPlayingGame);
                GamesDataFactory.events.off('games:startedPlaying', onStartPlayingGame);
            }

            // when the element is destroyed make sure that we cleanup
            scope.$on('$destroy', onDestroy);

            if (scope.videoId) {
                scope.$on('AGEGATE:PASSED', function () {
                    scope.agePassed = true;
                    // hide the age gate after is been passed so users can click on the video controls
                    elem.find('.origin-agegate').hide();
                    if (scope.showVideo) {
                        youtubePlayer.playVideo();
                    }
                });

                scope.$on('AGEGATE:FAILED', function () {
                    scope.agePassed = false;
                });

                AnimateFactory.addResizeEventHandler(scope, resize);

                var youtubePromise = YoutubeFactory.loadYoutubePlayer(youtubeElement, scope.videoId, {controls: 1}, element.width(), element.width() * ASPECT_RATIO)
                    .then(cachePlayer);

                scope.$on(VIDEO_PLAY_EVENT, playVideo);
            }
        }
        return {
            restrict: 'E',
            require: ['^originStorePdpHero'],
            scope: {
                videoId: '@',
                matureContent: '@',
                restrictedMessage: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/video.html'),
            link: originStorePdpSectionsVideoLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsVideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id id of the video
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {LocalizedString} restricted-message the message to show the restricted user. This will override the dict value.
     * @description
     *
     * PDP section video
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-sections-video />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsVideo', originStorePdpSectionsVideo);
}());
