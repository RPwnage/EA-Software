/**
 * @file store/pdp/sections/scripts/mediacarouselimg.js
 */
(function () {
    'use strict';

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-media-carousel-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-media-carousel-img';


    function originStorePdpMediaCarouselImage(ComponentsConfigFactory, DirectiveScope) {

        function originStorePdpMediaCarouselImageLink(scope, element, attributes, mediaCarouselCtrl) {

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE], scope.ocdPath).then(function () {
                if (scope.src) {
                    scope.position = mediaCarouselCtrl.register({
                        type: 'image',
                        src: scope.src
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
                src: '@',
                ocdPath: '@'
            },
            link: originStorePdpMediaCarouselImageLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/sections/views/mediacarouselimg.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpMediaCarouselImg
     * @restrict E
     * @element ANY
     * @scope
     * @description Carousel Image for PDP
     * @param {ImageUrl} src image source
     * @param {OcdPath} ocd-path OCD Path
     *
     * @example
     *  <origin-store-pdp-media-carousel-img src="https://someimage.jpeg">
     *  </origin-store-pdp-media-carousel-img>
     */
    angular.module('origin-components')
        .directive('originStorePdpMediaCarouselImg', originStorePdpMediaCarouselImage);
}());

