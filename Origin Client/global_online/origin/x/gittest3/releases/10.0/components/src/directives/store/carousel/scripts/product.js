
/**
 * @file store/carousel/scripts/product.js
 */
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreCarouselProduct(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                title: '@',
                viewAllStr: '@',
                viewAllHref: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/product.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProduct
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title - How the carousel text should be aligned affects every slide
     * @param {LocalizedString} viewAllStr - The number of milliseconds to sit on a slide without changing to the next slide
     * @param {string} viewAllHref - boolean as to if auto rotation should be on or off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProduct', originStoreCarouselProduct);
}());
