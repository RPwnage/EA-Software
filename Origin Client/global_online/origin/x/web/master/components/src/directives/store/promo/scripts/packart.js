
/** 
 * @file store/promo/scripts/packart.js
 */ 
(function(){
    'use strict';

    function originStorePromoPackart(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/packart.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoPackart
     * @restrict E
     * @element ANY
     * @description Takes in an offerid and show-packart attributes from merchandized parent and adds packart to a banner.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-packart><origin-store-promo-packart />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoPackart', originStorePromoPackart);
}()); 
