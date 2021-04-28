
/**
 * @file store/carousel/scripts/hero/indicators.js
 */
(function(){
    'use strict';
        /**
    * The directive
    */
    function originStoreCarouselHeroIndicators(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            controller: 'OriginStoreCarouselHeroControlsCtrl',
            scope: {
                carouselState: '=',
                indicators: '=',
                current: '='
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/hero/views/indicators.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroIndicators
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-hero-indicators />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselHeroIndicators', originStoreCarouselHeroIndicators);
}());
