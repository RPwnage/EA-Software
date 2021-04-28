/**
 * @file backgroundvideo.js
 */
(function(){
    'use strict';

    function originBackgroundVideo(ComponentsConfigFactory, UtilFactory, $window, YoutubeFactory) {

        function originBackgroundVideoLink(scope, element) {

            var playerOptions = {
                    autoplay: 1,
                    autohide: 1,
                    loop: 1,
                    controls: 0,
                    modestbranding: 0,
                    playsinline: 1,
                    rel: 0,
                    showinfo: 0,
                    playlist: scope.videoId
            };

            var player = YoutubeFactory.getPlayer(element);
            var playerId = player.attr('id');
            function initializePlayer() {
                player = YoutubeFactory.initYoutubeVideo(playerId, playerOptions, scope.videoId, onPlayerReady, element.width(), element.height());
            }

            function onPlayerReady() {
                player.mute();
                player.playVideo();
            }

            YoutubeFactory.events.once(YoutubeFactory.onReady, initializePlayer);
            YoutubeFactory.loadYoutubeScript();
        }

        return {
            restrict: 'E',
            scope: {
                videoId: '@'
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
     * @param {string} video youtube id of video
     * @param {TextColorsEnumeration} textColor text color: light or dark
     * @param {TextAlignEnumeration} textAlign text alignment
     * @param {TextSlideEnumeration} textSlide boolean to determine if text slides in or not
     * @param {LocalizedString} header (optional) header text
     * @param {LocalizedString} text (optional) main paragraph
     * @param {ButtonTypeEnumeration} ctaType type of CTA button
     * @param {LocalizedString} ctaDescription CTA description text
     * @param {Url} ctaHref URL of CTA link
     * @description
     *
     * Large feature video PDP section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-video
     *         video="3eVF9uBbuqc"
     *         text-color="light"
     *         text-align="center"
     *         text-slide="true"
     *         header="Immerse yourself in your Star Wars Battlefront fantasies"
     *         text="Photorealistic visuals and authentic sound design from the talented team at DICE. Prepate to be transported to a galaxy far, far away."
     *         cta-type="transparent"
     *         cta-description="Watch the E3 Gameplay Trailer"
     *         cta-href="#"
     *     ></origin-store-pdp-feature-video>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originBackgroundVideo', originBackgroundVideo);
}());
