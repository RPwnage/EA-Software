
/** 
 * @file /store/pack/scripts/item.js
 */ 
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStorePackItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/pack/views/item.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePackItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id of the product
     * @description
     * 
     * Store Pack Item
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-pack-item
     *              offerid="OFR-123">
     *          </origin-store-pack-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePackItem', originStorePackItem);
}()); 
