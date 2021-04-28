
/**
 * @file store/carousel/scripts/featured.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselFeatured(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                title: '@',
                text: '@',
                description: '@',
                href: '@',
                imageSrc: '@',
                startColor: '@',
                endColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/featured.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselFeatured
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title The title of the module
     * @param {LocalizedString} text Descriptive text describing the products
     * @param {LocalizedString} description The text in the call to action
     * @param {Url} href the The destination of the cta
     * @param {ImageUrl} image-src The sorce for the image
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-featured />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselFeatured', originStoreCarouselFeatured);
}());
