
/**
 * @file store/carousel/scripts/image.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselImage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                title: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/image.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselImage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title - How the carousel text should be aligned affects every slide
     * @param {LocalizedString} viewAllStr - The number of milliseconds to sit on a slide without changing to the next slide
     * @param {string} viewAllHref - boolean as to if auto rotation should be on or off.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-image title="Image/Video Slider"></origin-store-carousel-image>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselImage', originStoreCarouselImage);
}());
