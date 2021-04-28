
/**
 * @file store/scripts/productlist.js
 */
(function(){
    'use strict';
   
    /**
     * A list of tile type options
     * @readonly
     * @enum {string}
     */
    var TileTypeEnumeration = {
        "fixed": "fixed",
        "responsive": "responsive",
        "compact": "compact"
    };

    function originStoreProductlist(ComponentsConfigFactory) {

        function originStoreProductlistLink(scope) {
            scope.type = scope.type || TileTypeEnumeration.responsive;
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                type: '@'
            },
            replace: true,
            link: originStoreProductlistLink,
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
     * @param {TileTypeEnumeration} type can be one of three values 'fixed'/'responsive'/'compact'. this sets the style of the tile. Default is responsive
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
