/**
 * @file backgroundvideo.js
 */
(function(){
    'use strict';

    var THROTTLE_DELAY_MS = 300;
    var SECOND_TO_MS = '1000';
    var REWIND_THRESHOLD_MS = 75;
    var VIDEO_TIME_CHECK_INTERVAL = 50; //in ms

    function originBackgroundVideo(ComponentsConfigFactory, YoutubeFactory, ScreenFactory, AnimateFactory, GamesDataFactory) {

        function originBackgroundVideoLink(scope, element) {
            var youtubePlayer,
                videoCheckInternal,
                restartVideoInterval;
            scope.isVisible = false; // initially the video player is hidden so we don't see the YT "loading" spinner
            scope.isSmallScreen = ScreenFactory.isXSmall();
            scope.youtubeImageUrl = scope.imageUrl || YoutubeFactory.getStillImage(scope.videoId);
            scope.backgroundColor = '#141b20';

            function handleVideoStateChange(event) {
                if (event.data === YoutubeFactory.PLAYER_STATES.PLAYING && !scope.isVisible) {
                    // video started playing
                    scope.isVisible = true;
                    loopVideo();
                    scope.$digest();
                }
            }

            /**
             * Restarts the video
             */
            function restartVideo(){
                youtubePlayer.seekTo(0);
            }

            /**
             *  If remaining video time is 250ms or less, restart the video.
             *  After first restart, uses interval because video is now fully loaded
             *
             */
            function checkVideoTime(){
                var duration = youtubePlayer.getDuration();
                if ((duration - youtubePlayer.getCurrentTime()) <= REWIND_THRESHOLD_MS){
                    clearInterval(videoCheckInternal);
                    restartVideoInterval = setInterval(restartVideo, duration * SECOND_TO_MS);
                    restartVideo();
                }
            }

            /**
             * Checks video time every 100ms to determine when to restart the video
             */
            function loopVideo(){
                videoCheckInternal = setInterval(checkVideoTime, VIDEO_TIME_CHECK_INTERVAL);
            }

            function toggleVideoPlay(play) {
                if (youtubePlayer && scope.videoId && !scope.isSmallScreen){
                    if (play){
                        playVideo();
                    }else{
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

            Origin.events.on(Origin.events.CLIENT_VISIBILITY_CHANGED, toggleVideoPlay);
            GamesDataFactory.events.on('games:finishedPlaying', onStopPlayingGame);
            GamesDataFactory.events.on('games:startedPlaying', onStartPlayingGame);
            /**
             * Initialize youtube player
             * @returns {Promise}
             */
            function initializePlayerAndPlay() {
                if (!youtubePlayer && scope.videoId) {
                    var youtubeElement = element.find('.origin-background-video-player');
                    return YoutubeFactory.loadYoutubePlayer(youtubeElement, scope.videoId, {loop: 0}, element.width(), element.height(), {'onStateChange': handleVideoStateChange})
                        .then(cachePlayer)
                        .then(playVideo);
                }
            }

            /**
             * Holds on to a youtubePlayer reference
             * @param player
             * @returns {Object} youtubePlayer
             */
            function cachePlayer(player) {
                youtubePlayer = player;
                return player;
            }

            /**
             * if playing is false start playing video
             */
            function playVideoIfNotPlayingGame(playing) {
                if(!playing) {
                    youtubePlayer.playVideo();
                    youtubePlayer.mute();                    
                }
            }

            /**
             * When the video loads start playing it with zero volume
             */
            function playVideo() {
                return Origin.client.games.isGamePlaying()
                    .then(playVideoIfNotPlayingGame);
            }

            /**
             * Scale the component correctly with the window ie make it responsive
             */
            function adjustSize(){
                /*
                 Firefox doesn't allow access to the youtube API unless the element is visible.  Because of
                 this we have get the screen size and digest so the ng-hide is updated before initializing the you
                 tube video.
                 */
                scope.isSmallScreen = ScreenFactory.isXSmall();
                scope.$digest();

                if (!scope.isSmallScreen) { // Not a small screen
                    if(!youtubePlayer) {// The player API doesn't exist
                        initializePlayerAndPlay();
                    } else {// Player has been initialized
                        playVideo();
                    }
                } else if(youtubePlayer) { // The screen is small and the player is initialized.
                    youtubePlayer.stopVideo();
                }
            }

            if (!scope.isSmallScreen) {
                initializePlayerAndPlay();
            }

            AnimateFactory.addResizeEventHandler(scope, adjustSize, THROTTLE_DELAY_MS);

            function onDestroy() {
                if(youtubePlayer) {
                    youtubePlayer.stopVideo();
                    youtubePlayer.destroy();
                }

                clearInterval(restartVideoInterval);
                clearInterval(videoCheckInternal);
                Origin.events.off(Origin.events.CLIENT_VISIBILITY_CHANGED, toggleVideoPlay);
                GamesDataFactory.events.off('games:finishedPlaying', onStopPlayingGame);
                GamesDataFactory.events.off('games:startedPlaying', onStartPlayingGame);
            }

            // when the element is destroyed make sure that we cleanup
            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            scope: {
                videoId: '@',
                imageUrl: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/backgroundvideo.html'),
            link: originBackgroundVideoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originBackgroundVideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} image-url image alternative for small devices
     * @description Background video. Note background videos will be disabled for small screen devices.
     * Please specify an image
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-background-video video-id="someId" image-url="imageUrl"></origin-background-video>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originBackgroundVideo', originBackgroundVideo);
}());
