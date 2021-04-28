
/**
 * @file store/pdp/sections/scripts/video.js
 */
(function(){
    'use strict';
    var ASPECT_RATIO = '0.5625';

    function originStorePdpSectionsVideo(ComponentsConfigFactory, YoutubeFactory, AppCommFactory, $window, $timeout) {

        function originStorePdpSectionsVideoLink(scope, elem) {

            var player = YoutubeFactory.getPlayer(elem);
            var playerAPI = {};
            var element = elem.find('.origin-pdp-hero-video');
            var options = YoutubeFactory.mergeOptions({});
            scope.agePassed = false;

            function initializePlayer() {

                var playerId = player.attr('id');
                playerAPI = YoutubeFactory.initYoutubeVideo(
                    playerId,
                    options,
                    scope.videoId,
                    onPlayerReady,
                    element.width(),
                    element.width() * ASPECT_RATIO
                );
            }

            function onPlayerReady() {
                scope.videoReady = true;
            }

            YoutubeFactory.events.once(YoutubeFactory.onReady, initializePlayer);
            YoutubeFactory.loadYoutubeScript();
            scope.$on('YoutubePlayer:Play', function(){
                element.height(element.width() * ASPECT_RATIO);
                if(!scope.restrictedAge || scope.agePassed) {
                    playerAPI.playVideo();
                }
            });

            scope.hide = function() {

                element.height(0);

                $timeout(function() {
                    playerAPI.stopVideo();
                    scope.showVideo = false;
                }, 200);
            };

            function resize() {
                var frame = elem.find('.origin-pdp-hero-video-frame');
                var width = frame.width();
                frame.height(width * ASPECT_RATIO);
            }

            AppCommFactory.events.on('AGEGATE:PASSED', function(){
                scope.agePassed = true;
                if(scope.showVideo) {
                    playerAPI.playVideo();
                }
            });

            AppCommFactory.events.on('AGEGATE:FAILED', function(){
                scope.agePassed = false;
            });

            angular.element($window).on('resize', resize);
        }
        return {
            restrict: 'E',
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
     * @description
     * @param {string=}
     *
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
