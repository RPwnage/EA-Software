
/**
 * @file store/carousel/product/scripts/indicators.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselProductIndicators(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                carouselState: '=',
                indicators: '='
            },
            controller: 'OriginStoreCarouselHeroControlsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/product/views/indicators.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductIndicators
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product-indicators />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductIndicators', originStoreCarouselProductIndicators);
}());
