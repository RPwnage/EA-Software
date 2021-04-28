
/**
 * @file store/carousel/product/scripts/gameitem.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselProductGameitem(ComponentsConfigFactory) {
        return {
            replace: true,
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/product/views/gameitem.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductGameitem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} offerid the offer id of the product
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product-gameitem />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductGameitem', originStoreCarouselProductGameitem);
}());
