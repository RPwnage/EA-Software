
/** 
 * @file store/scripts/row.js
 */ 
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreRow() {
        return {
            restrict: 'E',
            scope: {},
            template: '<div ng-transclude class="l-origin-store-row"></div>',
            transclude: true,
            replace: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreRow
     * @restrict E
     * @element ANY
     * @scope
     * @description Store row container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-row />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreRow', originStoreRow);
}()); 
