
/**
 * @file store/carousel/product/scripts/controls.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselProductControls(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                carouselState: '=',
                current : '='
            },
            controller: 'OriginStoreCarouselHeroControlsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/product/views/controls.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductControls
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product-controls />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductControls', originStoreCarouselProductControls);
}());
