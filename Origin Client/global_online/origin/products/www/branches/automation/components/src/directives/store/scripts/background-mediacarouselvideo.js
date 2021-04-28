/**
 * @file store/about/scripts/background-mediacarouselvideo.js
 */

(function () {
    'use strict';

    var THROTTLE_DELAY_MS = 300,
        VIDEO_WIDTH = '1920px',
        VIDEO_HEIGHT = '1080px',
        CONTEXT_NAMESPACE = 'origin-background-mediacarousel-video';

    function originBackgroundMediacarouselVideo(ComponentsConfigFactory, DirectiveScope, YoutubeFactory, AnimateFactory, ScreenFactory) {

        function originBackgroundMediacarouselVideoLink(scope, element, attributes, backgroundMediacarouselCtrl) {

            scope.timeoutInterval = backgroundMediacarouselCtrl.getTimeoutInterval();
            scope.isSmallScreen = ScreenFactory.isXSmall();

            function init() {
                scope.imageUrl = scope.imageUrl || YoutubeFactory.getStillImage(scope.videoId);
                scope.assignedIndex = backgroundMediacarouselCtrl.registerCarouselItem(scope.notify);
                AnimateFactory.addResizeEventHandler(scope, adjustSize, THROTTLE_DELAY_MS);
            }


            var isTimerRunning = false;
            var videoPlayer;

            function createPlayer() {
                var videoParentElem = element.find('.origin-video-player');

                return YoutubeFactory
                    // Video needs to be set to an explicit 16/9 aspect ratio and 1920/1080 is the 
                    // maximum reasonable native size that the videos will be merchandized to
                    .loadYoutubePlayer(videoParentElem, scope.videoId, {autoplay: 1}, VIDEO_WIDTH, VIDEO_HEIGHT) 
                    .then(cachePlayer);
            }

            function cachePlayer(player) {
                videoPlayer = player;
                return videoPlayer;
            }

            /**
             * Play the video
             * @returns {Promise} for chain-ability
             */
            function playVideo() {
                videoPlayer.playVideo();
                videoPlayer.mute();
                return Promise.resolve();
            }

            function playVideoIfTimerRunning() {
                if (isTimerRunning) {
                    playVideo();
                }
            }

            function stopVideo() {
                videoPlayer.stopVideo();
            }

            /**
             * Starts countdown to stop video
             * @returns {Promise} for chain-ability
             */
            function startLoop() {
                return new Promise(function (resolve) {
                    isTimerRunning = true;
                    scope.timerHandle = setTimeout(function () {
                        isTimerRunning = false;
                        resolve();
                    }, scope.timeoutInterval);
                });
            }

            /**
             * Scale the component correctly with the window ie make it responsive
             */
            function adjustSize() {
                /*
                 Firefox doesn't allow access to the youtube API unless the element is visible.  Because of
                 this we have get the screen size and digest so the ng-hide is updated before initializing the you
                 tube video.
                 */
                scope.isSmallScreen = ScreenFactory.isXSmall();
                scope.$digest();

                if (!scope.isSmallScreen && isTimerRunning) { // Not a small screen
                    if (!videoPlayer) {// The player API doesn't exist
                        createPlayer().then(playVideoIfTimerRunning);
                    } else {// Player has been initialized
                        playVideoIfTimerRunning();
                    }
                } else if (videoPlayer) { // The screen is small and the player is initialized.
                   stopVideo();
                }
            }

            /**
             * Notify is called when active index changes in carousel.
             * @param index new active index
             * @returns {Prmoise} returns a promise when rotation is complete.
             */
            scope.notify = function (index) {
                if (scope.isOnTop && scope.assignedIndex === index) {
                    // If already on top, and got notified again, then must be the only child
                    //just reset the timer and resolve after it expires
                    return startLoop();
                } else {
                    scope.isOnTop = (scope.assignedIndex === index);
                    scope.$digest();

                    if (!scope.isOnTop && videoPlayer) {
                        videoPlayer.stopVideo();
                    }

                    //create a promise and resolve when done.
                    return new Promise(function (resolve) {
                        if (scope.isOnTop) {

                            if (!scope.isSmallScreen) {
                                if (!videoPlayer) {
                                    createPlayer()
                                        .then(playVideo)
                                        .then(startLoop)
                                        .then(resolve);
                                } else {
                                    playVideo()
                                        .then(startLoop)
                                        .then(resolve);
                                }
                            } else { //small screen must disable rotation
                                startLoop().then(resolve);
                            }

                        } else {
                            resolve();
                        }
                    });
                }

            };

            //watch ocdPath
           DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
            .then(init);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@',
                videoId: '@',
                imageUrl: '@'
            },
            require: '^originBackgroundMediacarousel',
            link: originBackgroundMediacarouselVideoLink,
            controller: 'originBackgroundMediacarouselItemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/background-mediacarouselitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originBackgroundMediacarouselVideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {OcdPath} ocd-path OCD Path
     * @param {Video} video-id youtube video ID
     * @param {ImageUrl} image-url image alternative for small devices
     * @description Displays a packArt and background video.
     *
     * @example
     * <origin-store-row>
     *     <origin-background-mediacarousel>
     *          <origin-background-mediacarousel-video ocd-path="asdasfsa" video-id="asdasdasd"></origin-background-mediacarousel-video>
     *     </origin-background-mediacarousel>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originBackgroundMediacarouselVideo', originBackgroundMediacarouselVideo);
})();

