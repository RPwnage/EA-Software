
/**
 * @file store/pdp/sections/scripts/mediacarouselvideo.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-media-carousel-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-media-carousel-video';

    function originStorePdpMediaCarouselVideo(ComponentsConfigFactory, DirectiveScope) {

        function originStorePdpMediaCarouselVideoLink(scope, element, attributes, mediaCarouselCtrl) {

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE], scope.ocdPath).then(function() {
                if (scope.videoId) {
                    scope.position = mediaCarouselCtrl.register({
                        type: 'video',
                        videoId : scope.videoId,
                        matureContent: scope.matureContent
                    });
                    scope.openModal = mediaCarouselCtrl.openModal;

                    scope.$digest();
                } 
            });
        }

        return {
            restrict: 'E',
            replace: true,
            require: '^originStorePdpMediaCarousel',
            scope: {
                videoId: '@',
                matureContent: '@',
                ocdPath: '@'
            },
            link: originStorePdpMediaCarouselVideoLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/sections/views/mediacarouselvideo.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpMediaCarouselVideo
     * @restrict E
     * @element ANY
     * @scope
     * @description Carousel Video for PDP
     * @param {Video} video-id Youtube video id
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {OcdPath} ocd-path OCD Path
     *
     * @example
     *  <origin-store-pdp-media-carousel-video video-id="">
     *  </origin-store-pdp-media-carousel-video>
     */
    angular.module('origin-components')
        .directive('originStorePdpMediaCarouselVideo', originStorePdpMediaCarouselVideo);
}());

