
/**
 * @file store/scripts/productlist.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreProductlist(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/productlist.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreProductlist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-productlist></origin-store-productlist>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreProductlist', originStoreProductlist);
}());
