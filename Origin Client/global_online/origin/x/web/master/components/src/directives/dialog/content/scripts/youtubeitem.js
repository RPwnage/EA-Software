/**
 * @file store/carousel/media/youtubeitem.js
 */
(function() {
    'use strict';

    function originStoreCarouselMediaYoutubeitem(ComponentsConfigFactory, YoutubeFactory) {

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

            var player = YoutubeFactory.getPlayer(element);
            var playerId = player.attr('id');
            var isPlayerReady = false;
            function initializePlayer() {
                player = YoutubeFactory.initYoutubeVideo(playerId, playerOptions, scope.videoId, onPlayerReady, scope.width, scope.height);
            }

            function onPlayerReady () {
                isPlayerReady = true;
            }

            function stopPlayer() {
                if (isPlayerReady) {
                    player.mute();
                    player.stopVideo();
                }
            }

            function pausePlayer() {
                if (isPlayerReady) {
                    player.mute();
                    player.pauseVideo();
                }
            }

            var stopListeningToStopMedia = scope.$on('dialogContent:mediaCarousel:mediaChanged', pausePlayer);

            YoutubeFactory.events.once(YoutubeFactory.onReady, initializePlayer);
            YoutubeFactory.loadYoutubeScript();


            scope.$on('$destroy', function() {
                stopPlayer();
                stopListeningToStopMedia();
            });
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                videoId: '@videoid',
                restrictedAge: '@restrictedage',
                width: '=',
                height: '='
            },
            link: OriginStoreCarouselMediaYoutubeitemLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youtubeitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYoutube
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} videoid the video id of the youtube video
     * @param {Number} restrictedage restricted age for a user
     * @description
     *
     * youtube video dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-carousel-media-youtubeitem>
     *             videoid="hlEKa6tUPlU"
     *             restrictedage="18">
     *         </origin-store-carousel-media-youtubeitem>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStoreCarouselMediaYoutubeitem', originStoreCarouselMediaYoutubeitem);

}());
