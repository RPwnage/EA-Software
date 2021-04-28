/**
 * @file /store/pack/scripts/item.js
 */
(function(){
    'use strict';

    function originStorePackItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@'
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
     * @param {OcdPath} ocd-path OCD Path of the product
     * @description
     *
     * Store Pack Item
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-pack-item
     *              ocd-path="OFR-123">
     *          </origin-store-pack-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePackItem', originStorePackItem);
}());
