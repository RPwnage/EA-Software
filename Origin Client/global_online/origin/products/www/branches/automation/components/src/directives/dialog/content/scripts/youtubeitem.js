/**
 * @file store/carousel/media/youtubeitem.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var PLAYER_HEIGHT = '100%',
        PLAYER_WIDTH = '100%';

    var AGEGATE_PASSED_EVENT = 'AGEGATE:PASSED';
    var AGEGATE_FAILED_EVENT = 'AGEGATE:FAILED';

    function originStoreCarouselMediaYoutubeitem(ComponentsConfigFactory, YoutubeFactory, SessionStorageFactory) {

        function OriginStoreCarouselMediaYoutubeitemLink(scope, element) {
            var playerOptions = {
                autoplay: 0,
                autohide: 1,
                loop: 1,
                controls: 1,
                modestbranding: 0,
                playsinline: 1,
                rel: 0,
                showinfo: 0,
                playlist: scope.videoId
            };

            var youtubeElement = angular.element(element).find('.origin-youtube-player');
            var youtubePlayer;
            scope.agegatePassed = 'false';

            function cachePlayer(player) {
                youtubePlayer = player;
            }

            function stopPlayer() {
                if (youtubePlayer) {
                    youtubePlayer.mute();
                    youtubePlayer.stopVideo();
                }
            }
            var stopListeningToMediaChange = scope.$on('dialogContent:mediaCarousel:mediaChanged', stopPlayer);

            YoutubeFactory.loadYoutubePlayer(youtubeElement, scope.videoId, playerOptions, PLAYER_WIDTH, PLAYER_HEIGHT).then(cachePlayer);

            scope.$on('$destroy', function() {
                stopPlayer();
                stopListeningToMediaChange();
            });

            // This directive needs awareness of agegate
            // for styling of video and agegate's parent div
            // In particular, height. (100vh for agegate, 100% otherwise)
            scope.$on(AGEGATE_PASSED_EVENT, function() {
                SessionStorageFactory.set('agegatepassed', 'yes');
                scope.agegatePassed = 'yes';
            });
            scope.$on(AGEGATE_FAILED_EVENT, function() {
                scope.agegatePassed = 'false';
                SessionStorageFactory.set('agegatepassed', 'no');
            });

            if(SessionStorageFactory.get('agegatepassed') === 'no') {
                scope.agegatePassed = 'no';
            } else if (SessionStorageFactory.get('agegatepassed') === 'yes'){
                scope.agegatePassed = 'yes';
            }

        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                videoId: '@',
                matureContent: '@'
            },
            link: OriginStoreCarouselMediaYoutubeitemLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youtubeitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYoutubeitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id the video id of the youtube video
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @description
     *
     * youtube video dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-carousel-media-youtubeitem>
     *             video-id="hlEKa6tUPlU">
     *         </origin-store-carousel-media-youtubeitem>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStoreCarouselMediaYoutubeitem', originStoreCarouselMediaYoutubeitem);

}());
